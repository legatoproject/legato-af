/**
 * @page c_le_mqttClient MQTT client
 *
 * @ref le_mqttClientLib.h "API Reference"
 *
 * <HR>
 *
 * @section mqtt_overview Overview
 *
 * MQTT client library allows user application to communicate with a remote MQTT brokerage
 * service with or without SSL encryption.
 *
 **/

//--------------------------------------------------------------------------------------------------
/**
 * @file le_mqttClientLib.h
 *
 * MQTT client library @ref c_le_mqttpClient include file
 *
 * Implementation of Legato MQTT client built on top of the
 * PAHO MQTT Embedded C 3rd party library.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------


#ifndef LE_MQTT_CLIENT_LIBRARY_H
#define LE_MQTT_CLIENT_LIBRARY_H

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 *  Reference to the MQTT client context.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mqttClient_Session *le_mqttClient_SessionRef_t;


/// MQTT Client Notification Event types
enum le_mqttClient_Event_t
{
    LE_MQTT_CLIENT_MSG_EVENT,       ///< Topic Message event
    LE_MQTT_CLIENT_CONNECTION_UP,   ///< MQTT Connection Up event
    LE_MQTT_CLIENT_CONNECTION_DOWN, ///< MQTT Connection Down event
};

/// MQTT Client Quality of Service types
typedef enum le_mqttClient_QoS
{
    LE_MQTT_CLIENT_QOS0,   ///< Guaranteed to be delivered 'At most once'
    LE_MQTT_CLIENT_QOS1,   ///< Guaranteed to be delivered 'At least once'
    LE_MQTT_CLIENT_QOS2,   ///< Guaranteed to be delivered 'Exactly once'
} le_mqttClient_QoS_t;


// MQTT Client Session Configuration Data
struct le_mqttClient_Configuration
{
    uint32_t         sessionId;             ///< Session Id
    uint32_t         profileNum;            ///< PDP profile number
    char            *host;                  ///< Host name or IP address of target
                                            ///  MQTT broker.
    uint16_t         port;                  ///< MQTT broker server control port.
    unsigned char    version;               ///< Version of MQTT to be used
                                            ///  (Latest version support is 3)
    char            *clientId;              ///< Client ID string
    unsigned int     keepAliveIntervalMs;   ///< Keep-Alive interval in milliseconds
    bool             cleanSession;          ///< Persistent connection flag
    unsigned int     connectionTimeoutMs;   ///< Connection timeout in milliseconds
    char            *userStr;               ///< User name to log into the server under.
    char            *passwordStr;           ///< Password to use when logging iN
                                            ///  to the server.
    unsigned int     readTimeoutMs;         ///< Read timeout in milliseconds.
    bool             secure;                ///< Secure session
    const uint8_t   *certPtr;               ///< Security certificate pointer
    size_t           certLen;               ///< Length in byte of certificate certPtr
};


//--------------------------------------------------------------------------------------------------
/**
 *  Callback to indicate an asynchronous session event has occurred.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_mqttClient_EventFunc_t)
(
    le_mqttClient_SessionRef_t   sessionRef,    ///< [IN] Session reference.
    enum le_mqttClient_Event_t   event,         ///< [IN] Event which occurred.
    char*                        topicName,     ///< [IN] Event Topic Name
                                                ///  (Only set for LE_MQTT_CLIENT_MSG_EVENT
                                                ///   event type)
    char*                        message,       ///< [IN] Event message
                                                ///  (Only set for LE_MQTT_CLIENT_MSG_EVENT
                                                ///   event type)
    void                        *contextPtr     ///< [IN] User data that was associated at callback
                                                ///  registration.
);


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
);


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
);

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
);


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
);


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
    le_mqttClient_QoS_t         qos            ///< [IN] Publication QoS setting
);


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
    le_mqttClient_QoS_t         qos            ///< [IN] Publication QoS setting
);


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
    le_mqttClient_QoS_t         qos            ///< [IN] Subscription QoS setting
);


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
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 *  Initialize the libaray
 *
 *  @return none
 */
//--------------------------------------------------------------------------------------------------
void le_mqttClient_Init
(
    void
);

#endif /* end LE_MQTT_CLIENT_LIBRARY_H */
