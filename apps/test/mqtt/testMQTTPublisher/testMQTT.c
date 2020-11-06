#include "legato.h"
#include "interfaces.h"

#include "le_mqttClientLib.h"

#include "defaultDerKey.h"

static bool Secure = false;
static char* Host = "broker.hivemq.com";
static int Port = 1883;
static char* Topic = "testTopic";

static void PublishThread(void)
{
    le_result_t result = LE_OK;
    le_mqttClient_SessionRef_t  sessionRef = NULL;
    struct le_mqttClient_Configuration config;

    // Initialize the session configuration structure
    config.host = Host;
    config.port = Port;
    config.version = 3;
    config.clientId = "mqtt_pub";
    config.keepAliveIntervalMs = 120000;
    config.cleanSession = 0;
    config.connectionTimeoutMs = 120000;
    config.userStr = "";
    config.passwordStr = "";
    config.readTimeoutMs = 3000;
    config.secure = Secure;
    config.certPtr = DefaultDerKey;
    config.certLen = DEFAULT_DER_KEY_LEN;

    LE_INFO("[PUB]: Creating MQTT Publisher Client to %s:%d", Host, Port);
    sessionRef = le_mqttClient_CreateSession(&config);
    LE_ASSERT(sessionRef);

    LE_INFO("[PUB]: Connecting to %s:%d", Host, Port);
    result = le_mqttClient_StartSession(sessionRef);
    LE_INFO("[PUB]: Connected %d", result);

    int count = 0;
    while (count < 10)
    {
        char payload[20];
        sprintf(payload, "MQTT test msg %d", ++count);

        LE_INFO("[PUB]: Publishing message: [%s]", payload);
        result = le_mqttClient_Publish(sessionRef, Topic, payload, 0, LE_MQTT_CLIENT_QOS2);

        // sleep for a short while before next publish
        sleep(1);
    }

    LE_INFO("[PUB]: Disconnecting publisher...");
    result = le_mqttClient_StopSession(sessionRef);
    result = le_mqttClient_DeleteSession(sessionRef);
    LE_INFO("[PUB]: Publisher disconnected.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Main entry to the command. Handle argument and call the appropriate function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Wait for MQTT Subsciber component to initialize
    sleep(10);

    // Initialize MQTT Client session and publish messages to broker
    PublishThread();
}
