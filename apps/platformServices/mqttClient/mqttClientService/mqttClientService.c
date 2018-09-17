/**
 * @file mqttClientService.c
 *
 * Implementation of MQTT Client Interface
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"

#include "MQTTClient.h"
#include "Socket.h"

//--------------------------------------------------------------------------------------------------
/**
 * Path to the SSL certificates file
 */
//--------------------------------------------------------------------------------------------------
static const char *SslCaCertsPathPtr = "/etc/ssl/certs/ca-certificates.crt";

//--------------------------------------------------------------------------------------------------
/**
 * MQTT Session structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct mqtt_Session
{
    MQTTClient client;
    MQTTClient_connectOptions connectOptions;
    MQTTClient_SSLOptions sslOptions;
    mqtt_MessageArrivedHandlerFunc_t messageArrivedHandler;
    void* messageArrivedHandlerContextPtr;
    mqtt_ConnectionLostHandlerFunc_t connectionLostHandler;
    void* connectionLostHandlerContextPtr;
    // The legato client session that owns this MQTT session
    le_msg_SessionRef_t clientSession;
} mqtt_Session;

static int QosEnumToValue(mqtt_Qos_t qos);
static void ConnectionLostHandler(void* contextPtr, char* causePtr);
static void ConnectionLostEventHandler(void* reportPtr);
static int MessageArrivedHandler(
    void* contextPtr, char* topicNamePtr, int topicLen, MQTTClient_message* messagePtr);
static void MessageReceivedEventHandler(void* reportPtr);
static void DestroySessionInternal(mqtt_Session* sessionPtr);

//--------------------------------------------------------------------------------------------------
/**
 * Safe ref map for mqtt_Session objects returned to clients.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t SessionRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Event id that is used to signal that a message has been received from the MQTT broker.  Events
 * must be used because paho spawns a thread for receiving messages and that means that the
 * callback can't call IPC methods because it is from a non-legato thread.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t ReceiveThreadEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Event id for connection lost events from paho.  The justification for this event is the same as
 * for ReceiveThreadEventId.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t ConnectionLostThreadEventId;

//--------------------------------------------------------------------------------------------------
/**
 * MQTT session memory pool.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t MQTTSessionPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Username memory pool.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t UsernamePoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Password memory pool.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PasswordPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Message memory pool.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t MessagePoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Topic memory pool.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t TopicPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Payload memory pool.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PayloadPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Represents a message which has been received from the MQTT broker.
 */
//--------------------------------------------------------------------------------------------------
typedef struct mqtt_Message
{
    // Safe reference to mqtt_Session
    mqtt_SessionRef_t sessionRef;
    char* topicPtr;
    size_t topicLength;
    uint8_t* payloadPtr;
    size_t payloadLength;
} mqtt_Message;


//--------------------------------------------------------------------------------------------------
/**
 * Creates an MQTT session object.
 *
 * @return
 *      LE_OK on success or LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t mqtt_CreateSession
(
    const char* brokerURIPtr,           ///< [IN] The URI of the MQTT broker to connect to.  Should be in
                                        ///  the form protocol://host:port. eg. tcp://1.2.3.4:1883 or
                                        ///  ssl://example.com:8883
    const char* clientIdPtr,            ///< [IN] Any unique string.  If a client connects to an MQTT
                                        ///  broker using the same clientId as an existing session, then
                                        ///  the existing session will be terminated.
    mqtt_SessionRef_t* sessionRefPtr    ///< [OUT] The created session if the return result is LE_OK
)
{
    mqtt_Session* s = le_mem_ForceAlloc(MQTTSessionPoolRef);
    LE_ASSERT(s);
    memset(s, 0, sizeof(*s));
    const MQTTClient_connectOptions initConnOpts = MQTTClient_connectOptions_initializer;
    memcpy(&(s->connectOptions), &initConnOpts, sizeof(initConnOpts));

    const MQTTClient_SSLOptions initSslOpts = MQTTClient_SSLOptions_initializer;
    memcpy(&(s->sslOptions), &initSslOpts, sizeof(initSslOpts));
    s->sslOptions.trustStore = SslCaCertsPathPtr;

    const int createResult = MQTTClient_create(
            &(s->client),
            brokerURIPtr,
            clientIdPtr,
            MQTTCLIENT_PERSISTENCE_NONE,
            NULL);
    if (createResult != MQTTCLIENT_SUCCESS)
    {
        LE_ERROR("Couldn't create MQTT session.  Paho error code: %d", createResult);
        le_mem_Release(s);
        return LE_FAULT;
    }

    le_msg_SessionRef_t clientSession = mqtt_GetClientSessionRef();
    s->clientSession = clientSession;

    *sessionRefPtr = le_ref_CreateRef(SessionRefMap, s);

    LE_ASSERT(MQTTClient_setCallbacks(
            s->client,
            *sessionRefPtr,
            &ConnectionLostHandler,
            &MessageArrivedHandler,
            NULL) == MQTTCLIENT_SUCCESS);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Destroy the given session.
 *
 * @note
 *      All MQTT sessions associated with the client session will be destroyed automatically when
 *      the client disconnects from the MQTT service.
 */
//--------------------------------------------------------------------------------------------------
void mqtt_DestroySession
(
    mqtt_SessionRef_t sessionRef  ///< [IN] Session to destroy
)
{
    mqtt_Session* s = le_ref_Lookup(SessionRefMap, sessionRef);
    if (s == NULL)
    {
        LE_KILL_CLIENT("Session doesn't exist");
        return;
    }
    if (s->clientSession != mqtt_GetClientSessionRef())
    {
        LE_KILL_CLIENT("Session doesn't belong to this client");
        return;
    }

    DestroySessionInternal(s);
    le_ref_DeleteRef(SessionRefMap, sessionRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Destroy the MQTT session: internal cleanup.
 */
//--------------------------------------------------------------------------------------------------
static void DestroySessionInternal
(
    mqtt_Session* sessionPtr
)
{
    MQTTClient_destroy(&(sessionPtr->client));
    // It is necessary to cast to char* from const char* in order to free the memory
    // associated with the username and password.
    le_mem_Release((char*)sessionPtr->connectOptions.username);
    le_mem_Release((char*)sessionPtr->connectOptions.password);
    le_mem_Release(sessionPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the connections options which will be used during subsequent calls to mqtt_Connect().
 */
//--------------------------------------------------------------------------------------------------
void mqtt_SetConnectOptions
(
    mqtt_SessionRef_t sessionRef,   ///< [IN] Session to set connection options in
    uint16_t keepAliveInterval,     ///< [IN] How often to send an MQTT PINGREQ packet if no other
                                    ///  packets are received
    bool cleanSession,              ///< [IN] When false, restore the previous state on a reconnect
    const char* usernamePtr,        ///< [IN] Username to connect with.
                                    ///  NULL if username is not required
    const uint8_t* passwordPtr,     ///< [IN] Password to connect with.
                                    ///  NULL if password is not required
    size_t passwordLength,          ///< [IN] Length of the password in bytes
    uint16_t connectTimeout,        ///< [IN] Connect timeout in seconds
    uint16_t retryInterval          ///< [IN] Retry interval in seconds
)
{
    mqtt_Session* s = le_ref_Lookup(SessionRefMap, sessionRef);
    if (s == NULL)
    {
        LE_KILL_CLIENT("Session doesn't exist");
        return;
    }
    if (s->clientSession != mqtt_GetClientSessionRef())
    {
        LE_KILL_CLIENT("Session doesn't belong to this client");
        return;
    }

    s->connectOptions.keepAliveInterval = keepAliveInterval;
    s->connectOptions.cleansession = cleanSession;

    // username
    if ((char*)s->connectOptions.username != NULL)
    {
        le_mem_Release((char*)s->connectOptions.username);
    }
    if (usernamePtr != NULL)
    {
        s->connectOptions.username = le_mem_ForceAlloc(UsernamePoolRef);
        LE_ASSERT(s->connectOptions.username != NULL);
        strcpy((char*)s->connectOptions.username, usernamePtr);
    }
    else
    {
        s->connectOptions.username = NULL;
    }

    // password
    if ((char*)s->connectOptions.password != NULL)
    {
        le_mem_Release((char*)s->connectOptions.password);
    }
    if (passwordPtr != NULL)
    {
        // paho uses null terminated strings for passwords, so the password may not contain any
        // embedded null characters.
        for (size_t i = 0; i < passwordLength; i++)
        {
            if (passwordPtr[i] == 0)
            {
                LE_KILL_CLIENT(
                    "Password contains embedded null characters and this is not currently "
                    "supported by this implementation");
                break;
            }
        }
        s->connectOptions.password = le_mem_ForceAlloc(PasswordPoolRef);
        LE_ASSERT(s->connectOptions.password != NULL);
        memcpy((uint8_t*)s->connectOptions.password, passwordPtr, passwordLength);
        ((uint8_t*)s->connectOptions.password)[passwordLength] = '\0';
    }
    else
    {
        s->connectOptions.username = NULL;
        s->connectOptions.password = NULL;
        if (usernamePtr != NULL)
        {
            LE_KILL_CLIENT("It is illegal to specify a password without a username");
        }
    }

    s->connectOptions.connectTimeout = connectTimeout;
    s->connectOptions.retryInterval = retryInterval;
    s->connectOptions.ssl = &s->sslOptions;
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect to the MQTT broker using the provided session.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER if the connection options are bad
 *      - LE_FAULT for general failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t mqtt_Connect
(
    mqtt_SessionRef_t sessionRef   ///< [IN] Session to connect
)
{
    mqtt_Session* s = le_ref_Lookup(SessionRefMap, sessionRef);
    if (s == NULL)
    {
        LE_KILL_CLIENT("Session doesn't exist");
        return LE_FAULT;
    }
    if (s->clientSession != mqtt_GetClientSessionRef())
    {
        LE_KILL_CLIENT("Session doesn't belong to this client");
        return LE_FAULT;
    }

    const int connectResult = MQTTClient_connect(s->client, &s->connectOptions);
    le_result_t result;
    switch (connectResult)
    {
        case SOCKET_ERROR:
            LE_WARN("Socket error");
            result = LE_FAULT;
            break;

        case MQTTCLIENT_NULL_PARAMETER:
        case MQTTCLIENT_BAD_STRUCTURE:
        case MQTTCLIENT_BAD_UTF8_STRING:
            result = LE_BAD_PARAMETER;
            break;

        case MQTTCLIENT_SUCCESS:
            result = LE_OK;
            break;

         default:
            LE_WARN("Paho connect returned (%d)", connectResult);
            result = LE_FAULT;
            break;
     }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disconnect a currently connected session
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 *
 * @note
 *      TODO: If the connection is lost right as disconnect is called, I think that this function
 *      will return LE_FAULT and the client will not know why.
 */
//--------------------------------------------------------------------------------------------------
le_result_t mqtt_Disconnect
(
    mqtt_SessionRef_t sessionRef   ///< [IN] Session to disconnect
)
{
    mqtt_Session* s = le_ref_Lookup(SessionRefMap, sessionRef);
    if (s == NULL)
    {
        LE_KILL_CLIENT("Session doesn't exist");
        return LE_FAULT;
    }
    if (s->clientSession != mqtt_GetClientSessionRef())
    {
        LE_KILL_CLIENT("Session doesn't belong to this client");
        return LE_FAULT;
    }

    const int waitBeforeDisconnectMs = 0;
    const int disconnectResult = MQTTClient_disconnect(s->client, waitBeforeDisconnectMs);
    le_result_t result;
    switch (disconnectResult)
    {
        case MQTTCLIENT_SUCCESS:
            result = LE_OK;
            break;

        case MQTTCLIENT_FAILURE:
            result = LE_FAULT;
            break;

        case MQTTCLIENT_DISCONNECTED:
            result = LE_FAULT;
            LE_WARN("Already disconnected");
            break;

        default:
            LE_WARN("Paho disconnect returned (%d)", disconnectResult);
            result = LE_FAULT;
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Publish the supplied payload to the MQTT broker on the given topic.
 *
 * @return
 *      LE_OK on success or LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t mqtt_Publish
(
    mqtt_SessionRef_t sessionRef,   ///< [IN] Session
    const char* topicPtr,           ///< [IN] Topic
    const uint8_t* payloadPtr,      ///< [IN] Message
    size_t payloadLen,              ///< [IN] Message length
    mqtt_Qos_t qos,                 ///< [IN] QoS mode
    bool retain                     ///< [IN] Retain flag for message
)
{
    mqtt_Session* s = le_ref_Lookup(SessionRefMap, sessionRef);
    if (s == NULL)
    {
        LE_KILL_CLIENT("Session doesn't exist");
        return LE_FAULT;
    }
    if (s->clientSession != mqtt_GetClientSessionRef())
    {
        LE_KILL_CLIENT("Session doesn't belong to this client");
        return LE_FAULT;
    }

    MQTTClient_deliveryToken* dtNotUsed = NULL;
    const int publishResult = MQTTClient_publish(
        s->client, topicPtr, payloadLen, (void*)payloadPtr, QosEnumToValue(qos), retain, dtNotUsed);
    le_result_t result = LE_OK;
    if (publishResult != MQTTCLIENT_SUCCESS)
    {
        LE_WARN("Publish failed with error code (%d)", publishResult);
        result = LE_FAULT;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Subscribe to the given topic pattern.  Topics look like UNIX filesystem paths.  Eg.
 * "/bedroom/sensors/motion".  Patterns may include special wildcard characters "+" and "#" to match
 * one or multiple levels of a topic.  For example. "/#/motion" will match topics
 * "/bedroom/sensors/motion" and "/car/data/motion", but not "/bedroom/sensors/motion/enabled".
 *
 * @return
 *      LE_OK on success or LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t mqtt_Subscribe
(
    mqtt_SessionRef_t sessionRef,   ///< [IN] Session
    const char* topicPatternPtr,    ///< [IN] Topic pattern
    mqtt_Qos_t qos                  ///< [IN] QoS mode
)
{
    mqtt_Session* s = le_ref_Lookup(SessionRefMap, sessionRef);
    if (s == NULL)
    {
        LE_KILL_CLIENT("Session doesn't exist");
        return LE_FAULT;
    }
    if (s->clientSession != mqtt_GetClientSessionRef())
    {
        LE_KILL_CLIENT("Session doesn't belong to this client");
        return LE_FAULT;
    }

    const int subscribeResult = MQTTClient_subscribe(s->client, topicPatternPtr, qos);
    le_result_t result = LE_OK;
    if (subscribeResult != MQTTCLIENT_SUCCESS)
    {
        LE_WARN("Subscribe failed with error code (%d)", subscribeResult);
        result = LE_FAULT;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Unsubscribe from the given topic pattern.
 *
 * @return
 *      LE_OK on success or LE_FAULT on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t mqtt_Unsubscribe
(
    mqtt_SessionRef_t sessionRef,   ///< [IN] Session
    const char* topicPatternPtr     ///< [IN] Topic pattern
)
{
    mqtt_Session* s = le_ref_Lookup(SessionRefMap, sessionRef);
    if (s == NULL)
    {
        LE_KILL_CLIENT("Session doesn't exist");
        return LE_FAULT;
    }
    if (s->clientSession != mqtt_GetClientSessionRef())
    {
        LE_KILL_CLIENT("Session doesn't belong to this client");
        return LE_FAULT;
    }

    const int unsubscribeResult = MQTTClient_unsubscribe(sessionRef->client, topicPatternPtr);
    le_result_t result = LE_OK;
    if (unsubscribeResult != MQTTCLIENT_SUCCESS)
    {
        LE_WARN("Unsubscribe failed with error code (%d)", unsubscribeResult);
        result = LE_FAULT;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the connection lost handler for the session.
 *
 * @return
 *      A handle which allows the connection lost handler to be removed.
 *
 * @note
 *      Only one handler may be registered
 */
//--------------------------------------------------------------------------------------------------
mqtt_ConnectionLostHandlerRef_t mqtt_AddConnectionLostHandler
(
    mqtt_SessionRef_t sessionRef,               ///< [IN] Session
    mqtt_ConnectionLostHandlerFunc_t handler,   ///< [IN] Connection lost handler
    void* contextPtr                            ///< [IN] Context
)
{
    mqtt_Session* s = le_ref_Lookup(SessionRefMap, sessionRef);
    if (s == NULL)
    {
        LE_KILL_CLIENT("Session doesn't exist");
        return NULL;
    }
    if (s->clientSession != mqtt_GetClientSessionRef())
    {
        LE_KILL_CLIENT("Session doesn't belong to this client");
        return NULL;
    }

    if (s->connectionLostHandler != NULL)
    {
        LE_KILL_CLIENT("You may only register one connection lost handler");
        return NULL;
    }
    else
    {
        s->connectionLostHandler = handler;
        s->connectionLostHandlerContextPtr = contextPtr;
    }

    return (mqtt_ConnectionLostHandlerRef_t)s;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deregister the connection lost handler for the session.
 */
//--------------------------------------------------------------------------------------------------
void mqtt_RemoveConnectionLostHandler
(
    mqtt_ConnectionLostHandlerRef_t handlerRef ///< [IN] Connection lost handler
)
{
    mqtt_Session* s = le_ref_Lookup(SessionRefMap, handlerRef);
    if (s == NULL)
    {
        LE_KILL_CLIENT("Session doesn't exist");
        return;
    }
    if (s->clientSession != mqtt_GetClientSessionRef())
    {
        LE_KILL_CLIENT("Session doesn't belong to this client");
        return;
    }

    s->connectionLostHandler = NULL;
    s->connectionLostHandlerContextPtr = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the message arrived handler for the session.
 *
 * @return
 *      A handle which can be used to deregister the message arrived handler.
 *
 * @note
 *      Only one message arrived handler may be registered for a session.
 */
//--------------------------------------------------------------------------------------------------
mqtt_MessageArrivedHandlerRef_t mqtt_AddMessageArrivedHandler
(
    mqtt_SessionRef_t sessionRef,               ///< [IN] Session
    mqtt_MessageArrivedHandlerFunc_t handler,   ///< [IN] Message arrived handler
    void* contextPtr                            ///< [IN] Context
)
{
    mqtt_Session* s = le_ref_Lookup(SessionRefMap, sessionRef);
    if (s == NULL)
    {
        LE_KILL_CLIENT("Session doesn't exist");
        return NULL;
    }
    if (s->clientSession != mqtt_GetClientSessionRef())
    {
        LE_KILL_CLIENT("Session doesn't belong to this client");
        return NULL;
    }

    if (s->messageArrivedHandler != NULL)
    {
        LE_KILL_CLIENT("You may only register one message arrived handler per session");
        return NULL;
    }
    else
    {
        s->messageArrivedHandler = handler;
        s->messageArrivedHandlerContextPtr = contextPtr;
    }

    return (mqtt_MessageArrivedHandlerRef_t)s;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deregister the message arrived handler for the session.
 */
//--------------------------------------------------------------------------------------------------
void mqtt_RemoveMessageArrivedHandler
(
    mqtt_MessageArrivedHandlerRef_t handlerRef ///< [IN] Message arrived handler
)
{
    mqtt_Session* s = le_ref_Lookup(SessionRefMap, handlerRef);
    if (s == NULL)
    {
        LE_KILL_CLIENT("Session doesn't exist");
        return;
    }
    if (s->clientSession != mqtt_GetClientSessionRef())
    {
        LE_KILL_CLIENT("Session doesn't belong to this client");
        return;
    }

    s->messageArrivedHandler = NULL;
    s->messageArrivedHandlerContextPtr = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the correct quality of service integer value as defined by the MQTT specification from the
 * enum type.
 *
 * @return
 *      The QoS value as defined by the MQTT specification
 */
//--------------------------------------------------------------------------------------------------
static int QosEnumToValue
(
    mqtt_Qos_t qos  ///< [IN] The QoS enum value to convert.
)
{
    int result = 0;
    switch (qos)
    {
        case MQTT_QOS0_TRANSMIT_ONCE:
            result = 0;
            break;

        case MQTT_QOS1_RECEIVE_AT_LEAST_ONCE:
            result = 1;
            break;

        case MQTT_QOS2_RECEIVE_EXACTLY_ONCE:
            result = 2;
            break;

        default:
            LE_KILL_CLIENT("Invalid QoS setting (%d)", qos);
            result = 0;
            break;
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This is the connection lost callback function that is supplied to the paho library.  The
 * function generates an event rather than calling the client supplied callback because this
 * function will be called on a non-Legato thread.
 */
//--------------------------------------------------------------------------------------------------
static void ConnectionLostHandler
(
    void* contextPtr, ///< context parameter contains the session for which the connection was lost
    char* causePtr    ///< paho library doesn't currently populate this
)
{
    le_event_Report(ConnectionLostThreadEventId, &contextPtr, sizeof(void*));
}

//--------------------------------------------------------------------------------------------------
/**
 * The event handler for the connection lost event that is generated by ConnectionLostHandler.
 * This function calls the handler supplied by the client.
 */
//--------------------------------------------------------------------------------------------------
static void ConnectionLostEventHandler
(
    void* reportPtr
)
{
    mqtt_Session* s = le_ref_Lookup(SessionRefMap, reportPtr);
    if (s == NULL)
    {
        LE_KILL_CLIENT("Session doesn't exist");
        return;
    }

    if (s->connectionLostHandler != NULL)
    {
        s->connectionLostHandler(s->connectionLostHandlerContextPtr);
    }
    else
    {
        LE_WARN("Connection was lost, but no handler is registered to receive the notification");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This is the message arrived callback function that is supplied to the paho library.  The
 * function generates an event rather than calling the client supplied callback because this
 * function will be called on a non-Legato thread.
 */
//--------------------------------------------------------------------------------------------------
static int MessageArrivedHandler
(
    void* contextPtr,
    char* topicNamePtr,
    int topicLen,
    MQTTClient_message* messagePtr
)
{
    mqtt_Session* s = le_ref_Lookup(SessionRefMap, contextPtr);
    if (s == NULL)
    {
        LE_WARN("Session doesn't exist");
        return true;
    }

    mqtt_Message* storedMsgPtr = le_mem_ForceAlloc(MessagePoolRef);
    LE_ASSERT(storedMsgPtr);
    memset(storedMsgPtr, 0, sizeof(*storedMsgPtr));

    LE_DEBUG("MessageArrivedHandler called for topic=%s. Storing session=0x%p", topicNamePtr, contextPtr);
    storedMsgPtr->sessionRef = contextPtr;

    // When topicLen is 0 it means that the topic contains embedded nulls and can't be treated as a
    // normal C string
    storedMsgPtr->topicLength = (topicLen == 0) ? (strlen(topicNamePtr) + 1) : topicLen;
    storedMsgPtr->topicPtr = le_mem_ForceAlloc(TopicPoolRef);
    LE_ASSERT(storedMsgPtr->topicPtr);
    memset(storedMsgPtr->topicPtr, 0, sizeof(MQTT_MAX_TOPIC_LENGTH));
    memcpy(storedMsgPtr->topicPtr, topicNamePtr, storedMsgPtr->topicLength);

    storedMsgPtr->payloadLength = messagePtr->payloadlen;
    storedMsgPtr->payloadPtr = le_mem_ForceAlloc(PayloadPoolRef);
    LE_ASSERT(storedMsgPtr->payloadPtr);
    memset(storedMsgPtr->payloadPtr, 0, sizeof(MQTT_MAX_PAYLOAD_LENGTH));
    memcpy(storedMsgPtr->payloadPtr, messagePtr->payload, storedMsgPtr->payloadLength);

    le_event_Report(ReceiveThreadEventId, &storedMsgPtr, sizeof(mqtt_Message*));

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * The event handler for the message arrived event that is generated by MessageArrivedHandler.
 * This function calls the handler supplied by the client.
 */
//--------------------------------------------------------------------------------------------------
static void MessageReceivedEventHandler
(
    void* reportPtr
)
{
    mqtt_Message* storedMsgPtr = *((mqtt_Message**)reportPtr);

    mqtt_Session* s = le_ref_Lookup(SessionRefMap, storedMsgPtr->sessionRef);
    if (s == NULL)
    {
        LE_WARN("Session lookup failed for session=0x%p", storedMsgPtr->sessionRef);
        return;
    }

    if (s->messageArrivedHandler != NULL)
    {
        if (storedMsgPtr->topicLength <= MQTT_MAX_TOPIC_LENGTH &&
            storedMsgPtr->payloadLength <= MQTT_MAX_PAYLOAD_LENGTH)
        {
            s->messageArrivedHandler(
                storedMsgPtr->topicPtr,
                storedMsgPtr->payloadPtr,
                storedMsgPtr->payloadLength,
                s->messageArrivedHandlerContextPtr);
        }
        else
        {
            LE_WARN(
                "Message arrived from broker, but it is too large to deliver using Legato IPC - "
                "topicLength=%zu, payloadLength=%zu",
                storedMsgPtr->topicLength,
                storedMsgPtr->payloadLength);
        }
    }
    else
    {
        LE_WARN(
            "Message has arrived, but no handler is registered to receive the notification");
    }

    le_mem_Release(storedMsgPtr->topicPtr);
    le_mem_Release(storedMsgPtr->payloadPtr);
    le_mem_Release(storedMsgPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Destroy all owned sessions
 */
//--------------------------------------------------------------------------------------------------
static void DestroyAllOwnedSessions
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    le_ref_IterRef_t it = le_ref_GetIterator(SessionRefMap);
    le_result_t iterRes = le_ref_NextNode(it);
    while (iterRes == LE_OK)
    {
        mqtt_Session* s = le_ref_GetValue(it);
        LE_ASSERT(s != NULL);
        mqtt_SessionRef_t sRef = (mqtt_SessionRef_t)(le_ref_GetSafeRef(it));
        LE_ASSERT(sRef != NULL);
        // Advance the interator before deletion to prevent invalidation
        iterRes = le_ref_NextNode(it);
        if (s->clientSession == sessionRef)
        {
            DestroySessionInternal(s);
            le_ref_DeleteRef(SessionRefMap, sRef);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize MQTT Client service
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Create memory pools
    MQTTSessionPoolRef = le_mem_CreatePool("MQTT session pool", sizeof(mqtt_Session));
    UsernamePoolRef = le_mem_CreatePool("MQTT username pool", MQTT_MAX_USERNAME_LENGTH);
    PasswordPoolRef = le_mem_CreatePool("MQTT password pool", MQTT_MAX_PASSWORD_LENGTH);
    MessagePoolRef = le_mem_CreatePool("MQTT message pool", sizeof(mqtt_Message));
    TopicPoolRef = le_mem_CreatePool("MQTT topic pool", MQTT_MAX_TOPIC_LENGTH);
    PayloadPoolRef = le_mem_CreatePool("MQTT payload pool", MQTT_MAX_PAYLOAD_LENGTH);

    SessionRefMap = le_ref_CreateMap("MQTT sessions", 16);

    ReceiveThreadEventId = le_event_CreateId(
        "MqttClient receive notification", sizeof(mqtt_Message*));
    le_event_AddHandler(
        "MqttClient receive notification", ReceiveThreadEventId, MessageReceivedEventHandler);

    ConnectionLostThreadEventId = le_event_CreateId(
        "MqttClient connection lost notification", sizeof(mqtt_Message*));
    le_event_AddHandler(
        "MqttClient connection lost notification",
        ConnectionLostThreadEventId,
        ConnectionLostEventHandler);

    le_msg_AddServiceCloseHandler(mqtt_GetServiceRef(), DestroyAllOwnedSessions, NULL);

    MQTTClient_init_options initOptions = MQTTClient_init_options_initializer;
    initOptions.do_openssl_init = 1;
    MQTTClient_global_init(&initOptions);
}
