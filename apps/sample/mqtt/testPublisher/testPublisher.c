/**
 * @file testPublisher.c
 *
 * MQTT Publisher sample application
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Whether connection to MQTT server is secured
 */
//--------------------------------------------------------------------------------------------------
#define MQTT_SERVER_USE_SECURED_CONNECTION 0

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of a Publish topic string, defined as
 * IO_MAX_RESOURCE_PATH_LEN + length_of_an_imei + 1 (for / separator) + 1 (for null terminator)
 */
//--------------------------------------------------------------------------------------------------
#define PUBLISH_STR_MAX_LEN  (IO_MAX_RESOURCE_PATH_LEN + LE_INFO_IMEI_MAX_BYTES + 1 + 1)

//--------------------------------------------------------------------------------------------------
/**
 * Device IMEI, used as a unique device identifier
 */
//--------------------------------------------------------------------------------------------------
char DeviceIMEI[LE_INFO_IMEI_MAX_BYTES];

//--------------------------------------------------------------------------------------------------
/**
 * MQTT session reference
 */
//--------------------------------------------------------------------------------------------------
mqtt_SessionRef_t MQTTSession;

//--------------------------------------------------------------------------------------------------
/**
 * Call-back function called on lost connection
 */
//--------------------------------------------------------------------------------------------------
static void OnConnectionLost
(
    void* context
)
{
    LE_ERROR("Connection lost!");
}

//--------------------------------------------------------------------------------------------------
/**
 * Call-back function called on arrived message
 */
//--------------------------------------------------------------------------------------------------
static void OnMessageArrived
(
    const char* topic,
    const uint8_t* payload,
    size_t payloadLen,
    void* context
)
{
    LE_ERROR("The publisher received a message!");
}


//--------------------------------------------------------------------------------------------------
/**
 * Publish the payload.
 */
//--------------------------------------------------------------------------------------------------
static void PublishData
(
    const char* pathPtr,
    const char* payloadPtr,
    size_t payloadLen
)
{
    char publishTopic[PUBLISH_STR_MAX_LEN];
    snprintf(publishTopic, sizeof(publishTopic), "%s%s", DeviceIMEI, pathPtr);
    const bool retain = false;
    const le_result_t publishResult = mqtt_Publish(
        MQTTSession,
        publishTopic,
        (const uint8_t*)payloadPtr,
        payloadLen,
        MQTT_QOS0_TRANSMIT_ONCE,
        retain);

    LE_DEBUG("Published Topic %s data %s result %s", publishTopic, payloadPtr,
            LE_RESULT_TXT(publishResult));
}


//--------------------------------------------------------------------------------------------------
/**
 * Publish the entry in the resource tree at a given path.
 */
//--------------------------------------------------------------------------------------------------
static void PublishEntry
(
    const char* path
)
//--------------------------------------------------------------------------------------------------
{
    admin_EntryType_t entryType = admin_GetEntryType(path);

    if (entryType == ADMIN_ENTRY_TYPE_NONE)
    {
        LE_ERROR("No resource at path '%s'.\n", path);
        exit(EXIT_FAILURE);
    }
    else if (entryType == ADMIN_ENTRY_TYPE_NAMESPACE)
    {
        // There's not much to do for a namespace.
    }
    else // Resource
    {
        double timestamp;
        char value[IO_MAX_STRING_VALUE_LEN + 1];

        le_result_t result = query_GetJson(path, &timestamp, value, sizeof(value));

        if (result == LE_OK)
        {
            PublishData(path, value, strlen(value));
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Publish all the entries in datahub to mqtt server
 */
//--------------------------------------------------------------------------------------------------
static void PublishDataHubList
(
    const char* path
)
//--------------------------------------------------------------------------------------------------
{
    PublishEntry(path);

    char childPath[IO_MAX_RESOURCE_PATH_LEN + 1];

    le_result_t result = admin_GetFirstChild(path, childPath, sizeof(childPath));
    LE_ASSERT(result != LE_OVERFLOW);

    while (result == LE_OK)
    {
        PublishDataHubList(childPath);

        result = admin_GetNextSibling(childPath, childPath, sizeof(childPath));

        LE_ASSERT(result != LE_OVERFLOW);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Timer handler for periodically publishing data
 */
//--------------------------------------------------------------------------------------------------
static void PublishTimerHandler
(
    le_timer_Ref_t timer
)
{
    PublishDataHubList("/");
}


COMPONENT_INIT
{
    // server is running on the Linux workstation connected to the target
#if MQTT_SERVER_USE_SECURED_CONNECTION
    const char mqttBrokerURI[] = "ssl://192.168.2.3:8883";
    const uint8_t mqttPassword[] = {'S', 'W', 'I'};
#else
    const char mqttBrokerURI[] = "tcp://192.168.2.3:1883";
#endif

    LE_ASSERT_OK(le_info_GetImei(DeviceIMEI, NUM_ARRAY_MEMBERS(DeviceIMEI)));
    char clientId[32];
    snprintf(clientId, sizeof(clientId), "%s-pub", DeviceIMEI);
    LE_ASSERT_OK(mqtt_CreateSession(mqttBrokerURI, clientId, &MQTTSession));

    const uint16_t keepAliveInSeconds = 60;
    const bool cleanSession = true;
    const char* username = DeviceIMEI;
    const uint16_t connectTimeout = 20;
    const uint16_t retryInterval = 10;
    mqtt_SetConnectOptions(
        MQTTSession,
        keepAliveInSeconds,
        cleanSession,
        username,
#if MQTT_SERVER_USE_SECURED_CONNECTION
        mqttPassword,
        NUM_ARRAY_MEMBERS(mqttPassword),
#else
        NULL,
        0,
#endif
        connectTimeout,
        retryInterval);

    mqtt_AddConnectionLostHandler(MQTTSession, &OnConnectionLost, NULL);
    mqtt_AddMessageArrivedHandler(MQTTSession, &OnMessageArrived, NULL);

    int rc = mqtt_Connect(MQTTSession);
    if (rc != LE_OK)
    {
        LE_ERROR("Connection failed! error %d", rc);
    }
    else
    {
        LE_INFO("Connected to server '%s'", mqttBrokerURI);
        le_timer_Ref_t timer = le_timer_Create("MQTT Publish");
        LE_ASSERT_OK(le_timer_SetHandler(timer, &PublishTimerHandler));
        LE_ASSERT_OK(le_timer_SetMsInterval(timer, 10000));
        LE_ASSERT_OK(le_timer_SetRepeat(timer, 0));
        LE_ASSERT_OK(le_timer_Start(timer));
        LE_INFO("Publish timer started");
    }
}
