#include "legato.h"
#include "interfaces.h"

#include "le_mqttClientLib.h"

#include "defaultDerKey.h"

static bool Secure = false;
static char* Host = "broker.hivemq.com";
static int Port = 1883;
static char* Topic = "testTopic";

static int Count = 0;
static le_sem_Ref_t  TestSemaphoreRef = NULL;

// Callback function to receive MQTT Client events
static void EventReceiveHandler
(
    le_mqttClient_SessionRef_t   sessionRef,    ///< Session reference.
    enum le_mqttClient_Event_t   event,         ///< Event which occurred.
    char*                        topicName,     ///< Event Topic Name
        ///  (Only set for LE_MQTT_CLIENT_MSG_EVENT event type)
    char*                        message,       ///< Event message
        ///  (Only set for LE_MQTT_CLIENT_MSG_EVENT event type)
    void                        *contextPtr     ///< User data that was associated at callback
                                                ///  registration.
)
{
    LE_UNUSED(sessionRef);
    LE_UNUSED(contextPtr);

    switch (event)
    {
        case LE_MQTT_CLIENT_MSG_EVENT:
            LE_INFO(">>> [SUB]: Received event: Message - topicName [%s], message [%s]",
                    topicName, message);

            char checkStr[20];
            sprintf(checkStr, "MQTT test msg %d", ++Count);

            // Test the topic message string is correct
            LE_TEST_OK((strncmp(checkStr, message, strlen(message)) == 0),
                       "Confirm message string: %s",
                       checkStr);

            if (Count == 10)
            {
                // We are done testing!
                le_sem_Post(TestSemaphoreRef);
            }
        break;

        case LE_MQTT_CLIENT_CONNECTION_UP:
            LE_INFO(">>> [SUB]: Received event: Network Connection Up");
        break;

        case LE_MQTT_CLIENT_CONNECTION_DOWN:
            LE_INFO(">>> [SUB]: Received event: Network Connection Down");
        break;

        default:
            LE_INFO(">>> [SUB]: Received event: Unknown");
        break;
    }
}

static void* SubscribeThread(void* context)
{
    le_result_t result = LE_OK;
    le_mqttClient_SessionRef_t  sessionRef = NULL;
    struct le_mqttClient_Configuration config;

    // Initialize the session configuration structure
    config.host = Host;
    config.port = Port;
    config.version = 3;
    config.clientId = "mqtt_sub";
    config.keepAliveIntervalMs = 120000;
    config.cleanSession = 0;
    config.connectionTimeoutMs = 120000;
    config.userStr = "";
    config.passwordStr = "";
    config.readTimeoutMs = 3000;
    config.secure = Secure;
    config.certPtr = DefaultDerKey;
    config.certLen = DEFAULT_DER_KEY_LEN;

    LE_INFO("[SUB]: Creating MQTT Subscriber Client to %s:%d", Host, Port);
    sessionRef = le_mqttClient_CreateSession(&config);
    LE_ASSERT(sessionRef);

    le_mqttClient_EnableLastWillAndTestament(sessionRef,
                                             Topic,
                                             "Publisher has closed",
                                             0,
                                             LE_MQTT_CLIENT_QOS2);

    LE_INFO("[SUB]: Registering Event Receive Handler function");
    result = le_mqttClient_AddReceiveHandler(sessionRef, EventReceiveHandler, NULL);
    LE_ASSERT(result == LE_OK);
    LE_INFO("[SUB]: Registered Event Receive Handler function, result %d", result);

    LE_INFO("[SUB]: Connecting to %s:%d", Host, Port);
    result = le_mqttClient_StartSession(sessionRef);
    LE_ASSERT(result == LE_OK);
    LE_INFO("[SUB]: Connected %d", result);

    LE_INFO("[SUB]: Subscribing to topic:[%s]", Topic);
    result = le_mqttClient_Subscribe(sessionRef, Topic, LE_MQTT_CLIENT_QOS2);
    LE_ASSERT(result == LE_OK);
    LE_INFO("[SUB]: Subscribed to topic:[%s], result %d", Topic, result);

    return sessionRef;
}

static void* TestThread(void* context)
{
    le_clk_Time_t testTimeout = { 0, 0 };
    testTimeout.sec = 120;

    LE_TEST_INFO("======== BEGIN MQTT TEST ========");
    LE_TEST_PLAN(11);

    // Wait up to 120 seconds for Asychronous Received Handler to post it is done
    le_result_t result = le_sem_WaitWithTimeOut(TestSemaphoreRef, testTimeout);

    LE_TEST_OK((result == LE_OK), "Confirm all test message have been received: %s",
               LE_RESULT_TXT(result));

    LE_TEST_INFO("======== END MQTT TEST ========");
    LE_TEST_EXIT;

    le_thread_Exit(NULL);
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Main entry to the command. Handle argument and call the appropriate function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    TestSemaphoreRef = le_sem_Create("MQTT-TestSemaphore", 0);
    LE_ASSERT(TestSemaphoreRef);

    // Initialize MQTT Client session and subscribe to topic messages from broker
    SubscribeThread(NULL);

    // Create a test thread
    le_thread_Ref_t  testThread = le_thread_Create("Test_Thread", TestThread, NULL);
    le_thread_Start(testThread);
}
