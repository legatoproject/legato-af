/**
 * @file le_mqttClientLib.c
 *
 * Implementation of Legato MQTT client built on top of the
 * PAHO MQTT Embedded C 3rd party library.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#include "legato.h"
#include "le_mqttClientLib.h"
#include "MQTTClient.h"
#include "mqttAdaptor.h"


//--------------------------------------------------------------------------------------------------
/**
 *  MQTT client session defines and definitions.
 */
//--------------------------------------------------------------------------------------------------
#define LE_MQTT_CLIENT_HOSTNAME_MAX_LEN    100
#define LE_MQTT_CLIENT_HOSTNAME_MAX_BYTES  LE_MQTT_CLIENT_HOSTNAME_MAX_LEN + 1

#define LE_MQTT_CLIENT_BUFFER_MAX_BYTES    LE_CONFIG_MQTT_CLIENT_BUFFER_SIZE_MAX_NUM

enum NetworkStatus
{
    LE_MQTT_NETWORK_STATUS_UNKNOWN,
    LE_MQTT_NETWORK_STATUS_UP,
    LE_MQTT_NETWORK_STATUS_DOWN
};


//--------------------------------------------------------------------------------------------------
/**
 *  MQTT client session.
 */
//--------------------------------------------------------------------------------------------------
struct le_mqttClient_Session
{
    struct Network network;                  ///< Network adaptor record
    enum NetworkStatus networkStatus;        ///< Network status (connected or disconnected)
    MQTTClient client;                       ///< Paho MQTT Client record
    MQTTPacket_connectData data;             ///< Paho MQTT Data record
    le_timer_Ref_t keepAliveTimerRef;        ///< Keep-Alive Timer Ref
    unsigned int msgId;                      ///< Unique message ID
    char host[LE_MQTT_CLIENT_HOSTNAME_MAX_BYTES];  ///< Broker server name or address
    uint16_t port;                           ///< Broker TCP port number
    unsigned int connectionTimeoutMs;        ///< Connection timeout (in milliseconds)
    unsigned int readTimeoutMs;              ///< Command read timeout (in milliseconds)
    unsigned char writebuf[LE_MQTT_CLIENT_BUFFER_MAX_BYTES]; ///< Write buffer
    unsigned char readbuf[LE_MQTT_CLIENT_BUFFER_MAX_BYTES];  ///< Read buffer
    le_mqttClient_EventFunc_t  handlerFunc;  ///< Client event handler function
    void *contextPtr;                        ///< Client context pointer
};


//--------------------------------------------------------------------------------------------------
/**
 * This pool is used to allocate memory for the MQTT Client Session record.
 * Initialized the first time a new MQTT Client is created in the funtcion,
 * le_mqttClient_CreateSession().
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(MqttClientSessionPool,
                          LE_CONFIG_MQTT_CLIENT_SESSION_MAX_NUM,
                          sizeof(struct le_mqttClient_Session));

static le_mem_PoolRef_t MqttClientSessionPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 *  Inline function for converting PAHO Client library return code values.
 *
 *  @return void
 */
//--------------------------------------------------------------------------------------------------
static inline le_result_t ConvertResultCode(int rc)
{
    if (rc == FAILURE)
    {
        return LE_FAULT;
    }
    else if (rc == BUFFER_OVERFLOW)
    {
        return LE_OVERFLOW;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 *  Asynchronous callback function for handling Message Status events
 *  for all MQTT client sessions.
 *
 *  @return void
 */
//--------------------------------------------------------------------------------------------------
static void MessageAsyncRecvHandler
(
    MessageData* md,   ///< [IN] Pointer to a Message data structure
    void* contextPtr   ///< [IN] Client's context pointer
)
{
    char topic[LE_MQTT_CLIENT_BUFFER_MAX_BYTES + 1];
    char msg[LE_MQTT_CLIENT_BUFFER_MAX_BYTES + 1];

    LE_INFO("[%s] Received message: [%s]", __FUNCTION__, msg);

    if (contextPtr == NULL)
    {
        return;
    }

    /* Initialize Topic and Message string buffers */
    memset(topic, 0, sizeof(topic));
    memset(msg, 0, sizeof(msg));

    MQTTString* topicName = md->topicName;
    int length = topicName->lenstring.len;

    if (length > LE_MQTT_CLIENT_BUFFER_MAX_BYTES)
    {
        LE_WARN("MQTT Topic Name is too long for current buffer, topic length [%d], buffer [%d",
                length,
                LE_MQTT_CLIENT_BUFFER_MAX_BYTES);

        length = LE_MQTT_CLIENT_BUFFER_MAX_BYTES;
    }

    /* Copy topic name */
    memcpy(topic, topicName->lenstring.data, length);

    MQTTMessage* message = md->message;
    length = message->payloadlen;

    if (length > LE_MQTT_CLIENT_BUFFER_MAX_BYTES)
    {
        LE_WARN("MQTT Topic Message is too long for current buffer, topic length [%d], buffer [%d",
                length,
                LE_MQTT_CLIENT_BUFFER_MAX_BYTES);

        length = LE_MQTT_CLIENT_BUFFER_MAX_BYTES;
    }

    /* Copy topic message */
    memcpy(msg, message->payload, length);

    /* Extract the MQTT client session from the contextPtr */
    le_mqttClient_SessionRef_t sessionRef =
        (le_mqttClient_SessionRef_t) contextPtr;

    if (sessionRef->handlerFunc)
    {
        LE_INFO("[%s] topic: %s, msg: %s, payload len %d",
                __FUNCTION__,
                topic,
                msg,
                message->payloadlen);

        /* Call the client's message handler */
        sessionRef->handlerFunc(sessionRef,
                                LE_MQTT_CLIENT_MSG_EVENT,
                                topic,
                                msg,
                                sessionRef->contextPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  Asynchronous callback function for handling Network Status events
 *  for all MQTT client sessions.
 *
 *  @return void
 */
//--------------------------------------------------------------------------------------------------
static void NetworkAsyncRecvHandler
(
    short  events,      ///< [IN] Event bit-mask
    void  *contextPtr   ///< [IN] Context Pointer
)
{
    LE_INFO("[%s] Received network events [0x%x]", __FUNCTION__, events);

    if (contextPtr == NULL)
    {
        return;
    }

    /* Extract the MQTT client session from the contextPtr */
    le_mqttClient_SessionRef_t sessionRef =
        (le_mqttClient_SessionRef_t) contextPtr;

    if (sessionRef == NULL)
    {
        LE_INFO("sessionRef is NULL");
        return;
    }

    if (events & POLLIN)
    {
        //
        // Data waiting to be read or written
        //

        if (sessionRef->networkStatus != LE_MQTT_NETWORK_STATUS_UP)
        {
            //
            // Network connection has come up
            //

            // Update the network status to reflect the change
            sessionRef->networkStatus = LE_MQTT_NETWORK_STATUS_UP;

            if (sessionRef->handlerFunc)
            {
                /* Call the client's message handler */
                sessionRef->handlerFunc(sessionRef,
                                        LE_MQTT_CLIENT_CONNECTION_UP,
                                        NULL,
                                        NULL,
                                        sessionRef->contextPtr);
            }
        }

        /* Execute the yield function for the specified MQTT client session */
        int result = MQTTYield(&sessionRef->client, sessionRef->readTimeoutMs);

        if (result != SUCCESS)
        {
            LE_ERROR("Error calling MQTTYield, result %d", ConvertResultCode(result));
        }
    }

    if (events & (POLLRDHUP | POLLHUP | POLLERR | POLLNVAL))
    {
        if (sessionRef->networkStatus != LE_MQTT_NETWORK_STATUS_DOWN)
        {
            //
            // Network connection has gone down
            //

            // Update the network status to reflect the change
            sessionRef->networkStatus = LE_MQTT_NETWORK_STATUS_DOWN;

            if (sessionRef->handlerFunc)
            {
                /* Call the client's message handler */
                sessionRef->handlerFunc(sessionRef,
                                        LE_MQTT_CLIENT_CONNECTION_DOWN,
                                        NULL,
                                        NULL,
                                        sessionRef->contextPtr);
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Expired Session-related Timers
 */
//--------------------------------------------------------------------------------------------------
static void SessionTimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< [IN] This timer has expired
)
{
    le_mqttClient_SessionRef_t  sessionRef;

    LE_INFO("[%s] MQTT Client Keep-Alive triggered",
            __FUNCTION__);

    // Retrieve ContextPtr data (session record)
    sessionRef = le_timer_GetContextPtr(timerRef);

    if (sessionRef == NULL)
    {
        LE_ERROR("Error extracting Session Record from timer event");
        return;
    }

    /* Send Keep-Alive packet to brokerage server */
    int result = MQTTKeepAlive(&sessionRef->client);

    if (result != SUCCESS)
    {
        LE_ERROR("Error calling MQTTKeepAlive, result %d", ConvertResultCode(result));
    }

    // Kick off the timer again
    le_timer_Start(timerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Start MQTT Client Keep-Alive service.
 */
//--------------------------------------------------------------------------------------------------
static void StartNetworkKeepAliveService
(
    le_mqttClient_SessionRef_t sessionRef    ///< [IN] Session reference.
)
{
    if (sessionRef->data.keepAliveInterval == 0)
    {
        LE_WARN("Keep-Alive Interval is zero - service not started");
        return;
    }

    LE_INFO("Starting MQTT Client Keep-Alive Service - frequency is %d seconds",
            sessionRef->data.keepAliveInterval);

    le_clk_Time_t timerInterval = {.sec=sessionRef->data.keepAliveInterval, .usec=0};

    // Create a timer to trigger a Network Keep-Alive check
    sessionRef->keepAliveTimerRef = le_timer_Create("MQTT Client Keep-Alive Service timer");
    le_timer_SetInterval(sessionRef->keepAliveTimerRef, timerInterval);
    le_timer_SetHandler(sessionRef->keepAliveTimerRef, SessionTimerExpiryHandler);
    le_timer_SetWakeup(sessionRef->keepAliveTimerRef, false);

    // Set Network Status record  in the timer event
    le_timer_SetContextPtr(sessionRef->keepAliveTimerRef, sessionRef);

    // Start timer
    le_timer_Start(sessionRef->keepAliveTimerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stop MQTT Client Keep-Alive service.
 */
//--------------------------------------------------------------------------------------------------
static void StopNetworkKeepAliveService
(
    le_mqttClient_SessionRef_t sessionRef    ///< [IN] Session reference.
)
{
    le_timer_Delete(sessionRef->keepAliveTimerRef);
    sessionRef->keepAliveTimerRef = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 *  Create a new MQTT client session.
 *
 *  @return
 *      - A new MQTT session reference on success.
 *      - NULL if an error occured.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_mqttClient_SessionRef_t le_mqttClient_CreateSession
(
    const struct le_mqttClient_Configuration *configPtr  ///< [IN] Pointer to configuration settings
)
{
    if (MqttClientSessionPoolRef == NULL)
    {
        /* Initialize the MQTT Client Session pool */
        MqttClientSessionPoolRef = le_mem_InitStaticPool(MqttClientSessionPool,
                                                         LE_CONFIG_MQTT_CLIENT_SESSION_MAX_NUM,
                                                         sizeof(struct le_mqttClient_Session));
    }

    /* Allocate memory for a new client session */
    le_mqttClient_SessionRef_t  sessionRef =
        le_mem_Alloc(MqttClientSessionPoolRef);

    if (sessionRef == NULL)
    {
        LE_ERROR("Session Reference is NULL");
        return NULL;
    }

    /* Store the MQTT Client Session's broker hostname, port number,
     * connection timeout, network status, and security settings. */
    strncpy(sessionRef->host, configPtr->host, LE_MQTT_CLIENT_HOSTNAME_MAX_LEN);
    sessionRef->port = configPtr->port;
    sessionRef->connectionTimeoutMs = configPtr->connectionTimeoutMs;
    sessionRef->readTimeoutMs = configPtr->readTimeoutMs;
    sessionRef->msgId = 1;
    sessionRef->networkStatus = LE_MQTT_NETWORK_STATUS_UNKNOWN;

    /* Allocate the MQTT Network resources */
    NetworkInit(&sessionRef->network,
                configPtr->secure,
                configPtr->certPtr,
                configPtr->certLen);

    /* Initialize the MQTT Client */
    MQTTClientInit(&sessionRef->client,
                   &sessionRef->network,
                   sessionRef->readTimeoutMs,
                   sessionRef->writebuf,
                   sizeof(sessionRef->writebuf),
                   sessionRef->readbuf,
                   sizeof(sessionRef->readbuf));

    /* Initialize the MQTT Packet Data record */
    MQTTPacket_connectData temp = MQTTPacket_connectData_initializer;
    sessionRef->data = temp;

    sessionRef->data.willFlag = 0;
    sessionRef->data.MQTTVersion = configPtr->version;
    sessionRef->data.clientID.cstring = configPtr->clientId;
    sessionRef->data.username.cstring = configPtr->userStr;
    sessionRef->data.password.cstring = configPtr->passwordStr;
    sessionRef->data.keepAliveInterval = (configPtr->keepAliveIntervalMs / 1000);
    sessionRef->data.cleansession = configPtr->cleanSession;

    LE_INFO("Created client session, clientID [%s], sessionRef [%p]",
            sessionRef->data.clientID.cstring, sessionRef);

    return sessionRef;
}


//--------------------------------------------------------------------------------------------------
/**
 *  Delete a MQTT client session.
 *
 *  @return void
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_mqttClient_DeleteSession
(
    le_mqttClient_SessionRef_t sessionRef    ///< [IN] Session reference.
)
{
    LE_INFO("Deleting client session, sessionRef [%p]", sessionRef);

    /* Free the MQTT Client Session record */
    le_mem_Release(sessionRef);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Start a new MQTT session to the configured server.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_mqttClient_StartSession
(
    le_mqttClient_SessionRef_t sessionRef    ///< [IN] Session reference.
)
{
    le_mdc_ConnectService();

    /* Connect the MQTT Client Session's to the broker server */
    le_result_t result =
        NetworkConnect(&sessionRef->network,
                       sessionRef->host,
                       sessionRef->port,
                       sessionRef->connectionTimeoutMs,
                       NetworkAsyncRecvHandler,
                       sessionRef);

    if (result != LE_OK)
    {
        LE_ERROR("NetworkConnect() failed, result %d", result);
        le_mem_Release(sessionRef);
        return LE_FAULT;
    }

    /* Send MQTT connect packet and wait for a Connack */
    int rc = MQTTConnect(&sessionRef->client, &sessionRef->data);

    LE_INFO("Connected client session, sessionRef [%p], result [%d]", sessionRef, rc);

    if (rc != SUCCESS)
    {
        return ConvertResultCode(rc);
    }

    // Start the keep-alive service
    StartNetworkKeepAliveService(sessionRef);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 *  Stop the active MQTT session.
 *
 *  @return void
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_mqttClient_StopSession
(
    le_mqttClient_SessionRef_t sessionRef    ///< [IN] Session reference.
)
{
    // Stop the keep-alive service
    StopNetworkKeepAliveService(sessionRef);

    /* Send MQTT disconnect packet and close the connection */
    int rc = MQTTDisconnect(&sessionRef->client);

    /* Disconnect from the MQTT Client Session's broker server */
    NetworkDisconnect(&sessionRef->network);

    le_mdc_DisconnectService();

    LE_INFO("Disconnected client session, sessionRef [%p], result [%d]", sessionRef, rc);

    return ConvertResultCode(rc);
}


//--------------------------------------------------------------------------------------------------
/**
 *  Enables the last will and testament (LWT) for the specified MQTT client session.
 *  NOTE: Must be performed before starting the MQTT session to take effect.
 *
 *  @return void
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_mqttClient_EnableLastWillAndTestament
(
    le_mqttClient_SessionRef_t  sessionRef,    ///< [IN] Session reference.
    char                       *topic,         ///< [IN] Last will topic
    char                       *message,       ///< [IN] Last will topic message
    bool                        retained,      ///< [IN] Indicates whether broker will retain the
                                               ///  message on that topic
    enum le_mqttClient_QoS_t    qos            ///< [IN] The quality of service setting
                                               ///  for the LWT message
)
{
    sessionRef->data.willFlag = 1;
    sessionRef->data.will.topicName.cstring = topic;
    sessionRef->data.will.message.cstring = message;
    sessionRef->data.will.retained = retained;

    /* Initialize the MQTT QoS */
    enum QoS mqttQos = (enum QoS) qos;

    sessionRef->data.will.qos = mqttQos;
}


//--------------------------------------------------------------------------------------------------
/**
 *  Publish a message to the MQTT session server.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_mqttClient_Publish
(
    le_mqttClient_SessionRef_t  sessionRef,    ///< [IN] Session reference.
    char                       *topic,         ///< [IN] Subscription Topic
    char                       *message,       ///< [IN] Topic message
    bool                        retained,      ///< [IN] Indicates whether broker will retain the
                                               ///  message on that topic
    enum le_mqttClient_QoS_t    qos            ///< [IN] Publication QoS setting
)
{
    /* Initialize the MQTT QoS */
    enum QoS mqttQos = (enum QoS) qos;

    MQTTMessage msg;
    msg.payload = message;
    msg.payloadlen = strlen(message);
    msg.qos = mqttQos;
    msg.retained = retained;
    msg.dup = 0;

    // Monotonically increasing Msg ID
    msg.id = sessionRef->msgId++;

    /* Publish a message to the specified topic */
    int rc = MQTTPublish(&sessionRef->client, topic, &msg);

    LE_INFO("Published client session message, message [%s], "
            "topic [%s], sessionRef [%p], result [%d]",
            message,
            topic,
            sessionRef,
            rc);

    return ConvertResultCode(rc);
}


//--------------------------------------------------------------------------------------------------
/**
 *  Subscribe to messages for a MQTT session.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_mqttClient_Subscribe
(
    le_mqttClient_SessionRef_t  sessionRef,    ///< [IN] Session reference.
    char                       *topic,         ///< [IN] Subscription Topic
    enum le_mqttClient_QoS_t    qos            ///< [IN] Subscription QoS setting
)
{
    /* Initialize the MQTT QoS */
    enum QoS mqttQos = (enum QoS) qos;

    /* Subscribe the MQTT Client session to the specified topic */
    int rc =
        MQTTSubscribe(&sessionRef->client,
                      topic,
                      mqttQos,
                      MessageAsyncRecvHandler,
                      sessionRef);

    LE_INFO("Subscribed client session to topic [%s], sessionRef [%p], result [%d]",
            topic,
            sessionRef,
            rc);

    return ConvertResultCode(rc);
}


//--------------------------------------------------------------------------------------------------
/**
 *  Unsubscribe to messages for a MQTT session.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_mqttClient_Unsubscribe
(
    le_mqttClient_SessionRef_t  sessionRef,    ///< [IN] Session reference.
    char                       *topic          ///< [IN] Subscription Topic
)
{
    /* Unsubscribe the MQTT Client session from the specified topic */
    int rc = MQTTUnsubscribe(&sessionRef->client, topic);

    LE_INFO("Unsubscribed client session to topic [%s], sessionRef [%p], result [%d]",
            topic,
            sessionRef,
            rc);

    return ConvertResultCode(rc);
}


//--------------------------------------------------------------------------------------------------
/**
 *  Set a callback to be invoked to handle asynchronous session events, relating to
 *  topic messages and network status.  Possible event types are described by
 *  the le_mqttClient_Event_t enum.
 *
 *  @return LE_OK on success or an appropriate error code on failure.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_mqttClient_AddReceiveHandler
(
    le_mqttClient_SessionRef_t   sessionRef,    ///< [IN] Session reference.
    le_mqttClient_EventFunc_t    handlerFunc,   ///< [IN] Handler callback.
    void                        *contextPtr     ///< [IN] Additional data to pass to the handler.
)
{
    /* Store the receive handler function and context pointer */
    sessionRef->handlerFunc = handlerFunc;
    sessionRef->contextPtr = contextPtr;

    return LE_OK;
}
