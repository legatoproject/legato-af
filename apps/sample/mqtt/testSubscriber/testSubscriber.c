/**
 * @file testSubscriber.c
 *
 * MQTT Subscriber sample application
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
void OnConnectionLost
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
void OnMessageArrived
(
    const char* topic,
    const uint8_t* payload,
    size_t payloadLen,
    void* context
)
{
    char payloadStr[payloadLen + 1];
    memcpy(payloadStr, payload, payloadLen);
    payloadStr[payloadLen] = '\0';
    LE_INFO("Received message! topic: \"%s\", payload: \"%s\"", topic, payloadStr);
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
    snprintf(clientId, sizeof(clientId), "%s-sub", DeviceIMEI);
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
        char subscribeTopic[64];
        snprintf(subscribeTopic, sizeof(subscribeTopic), "%s/messages/json", DeviceIMEI);
        LE_FATAL_IF(
            mqtt_Subscribe(MQTTSession, subscribeTopic, MQTT_QOS0_TRANSMIT_ONCE) != LE_OK,
            "failed to subscribe to %s",
            subscribeTopic);
        LE_INFO("Subscribed to topic (%s)", subscribeTopic);

        snprintf(subscribeTopic, sizeof(subscribeTopic), "%s/errors", DeviceIMEI);
        LE_FATAL_IF(
            mqtt_Subscribe(MQTTSession, subscribeTopic, MQTT_QOS0_TRANSMIT_ONCE) != LE_OK,
            "failed to subscribe to %s",
            subscribeTopic);
        LE_INFO("Subscribed to topic (%s)", subscribeTopic);
    }
}
