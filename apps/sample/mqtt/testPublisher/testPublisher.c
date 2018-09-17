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
 * Timer handler for periodically publishing data
 */
//--------------------------------------------------------------------------------------------------
static void PublishTimerHandler
(
    le_timer_Ref_t timer
)
{
    static int messageData = 0;
    uint8_t payload[64];
    snprintf((char*)payload, sizeof(payload), "{\"value\":%d}", messageData);
    size_t payloadLen = strlen((char*)payload);
    const bool retain = false;
    char publishTopic[64];
    snprintf(publishTopic, sizeof(publishTopic), "%s/messages/json", DeviceIMEI);
    const le_result_t publishResult = mqtt_Publish(
        MQTTSession,
        publishTopic,
        payload,
        payloadLen,
        MQTT_QOS0_TRANSMIT_ONCE,
        retain);
    LE_INFO("Published Topic %s data %s result %s", publishTopic, payload,
            LE_RESULT_TXT(publishResult));
    messageData++;
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
