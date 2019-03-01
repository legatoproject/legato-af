#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
* Temperature value update event name
*
*/
//--------------------------------------------------------------------------------------------------
#define EVENT_NAME "Sensor Temperature Updated Value"

//--------------------------------------------------------------------------------------------------
/**
* Temperature value update event ID
*
*/
//--------------------------------------------------------------------------------------------------
static le_event_Id_t TemperatureUpdatedEvent;

//--------------------------------------------------------------------------------------------------
/**
* Input options
*
*/
//--------------------------------------------------------------------------------------------------
static int MonitorTime;

//-------------------------------------------------------------------------------------------------
/**
* Structure to hold the updated temperature value
*/
//-------------------------------------------------------------------------------------------------
typedef struct
{
    int32_t sensorTemp;
} TemperatureUpdatedValue_t;

//-------------------------------------------------------------------------------------------------
/**
* Structure to hold the context of UpdatedValueHandlerFunc function.
*/
//-------------------------------------------------------------------------------------------------
typedef struct {
    bool shouldMonitorAmplifier;  ///< True for monitoring amplifier temp. False for monitoring controller temp.
    int  timeCounter;             ///< Number of monitoring iterations
} MonitorTemperatureContext_t;

//-------------------------------------------------------------------------------------------------
/**
* Thread for temperature displaying in Celsius.
*/
//-------------------------------------------------------------------------------------------------
static void* DisplayTempThread
(
    void* context             ///< [IN] Context
)
{

    MonitorTemperatureContext_t * tempContextPtr = (MonitorTemperatureContext_t *)(context);
    TemperatureUpdatedValue_t tempUpdated;
    static int32_t sensorTemp;
    le_temp_ConnectService();

    LE_INFO("DisplayTempThread Start");
    while(1)
    {
        if (tempContextPtr->shouldMonitorAmplifier)
        {
          le_temp_SensorRef_t paSensorRef = le_temp_Request("POWER_AMPLIFIER");
          le_temp_GetTemperature(paSensorRef, &sensorTemp);
          LE_INFO("Count %d: Amplifier Temperature: %d", tempContextPtr->timeCounter++, sensorTemp);
      }
      else
      {
          le_temp_SensorRef_t pcSensorRef = le_temp_Request("POWER_CONTROLLER");
          le_temp_GetTemperature(pcSensorRef, &sensorTemp);
          LE_INFO("Count %d: Controller Temperature: %d", tempContextPtr->timeCounter++, sensorTemp);
      }

      tempUpdated.sensorTemp = sensorTemp;
      le_event_Report(TemperatureUpdatedEvent, &tempUpdated, sizeof(tempUpdated));
      sleep(1);

      if (tempContextPtr->timeCounter >= MonitorTime)
      {
          tempContextPtr->timeCounter = 0;
          break;
      }
  }

  le_event_RunLoop();
  return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
* Monitor Power Amplifier Temperature
*
*/
//-------------------------------------------------------------------------------------------------
void temperature_MonitorAmpTemp
(
    int32_t monitorTime           ///< [IN] Monitor time in seconds
)
{
    MonitorTime = monitorTime;
    static MonitorTemperatureContext_t context = {
      .shouldMonitorAmplifier = true,
      .timeCounter = 0,
  };

  le_thread_Ref_t thread = le_thread_Create("tempThread", DisplayTempThread, &context);
  le_thread_Start(thread);
}

//-------------------------------------------------------------------------------------------------
/**
* Monitor Power Controller Temperature
*
*/
//-------------------------------------------------------------------------------------------------
void temperature_MonitorCtrlTemp
(
    int32_t monitorTime           ///< [IN] Monitor time in seconds
)
{
    MonitorTime = monitorTime;
    static MonitorTemperatureContext_t context = {
      .shouldMonitorAmplifier = false,
      .timeCounter = 0,
  };

  le_thread_Ref_t thread = le_thread_Create("tempThread", DisplayTempThread, &context);
  le_thread_Start(thread);
}

//-------------------------------------------------------------------------------------------------
/**
* First layer handler function. Temperature is converted to Fahrenheit here
*
*/
//-------------------------------------------------------------------------------------------------
static void FirstLayerStateHandler
(
    void* reportPtr,                        ///< [IN]
    void* secondLayerHandlerFunc            ///< [IN]
)
{
    TemperatureUpdatedValue_t* tempValuePtr = reportPtr;
    temperature_UpdatedValueHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    // Convert C to F
    int32_t sensorTempF = (tempValuePtr->sensorTemp * 9/5 + 32);
    // Call the client handler
    clientHandlerFunc(sensorTempF, le_event_GetContextPtr());
}

//-------------------------------------------------------------------------------------------------
/**
* Function to create the layered handler and send temperature to client
*
*/
//-------------------------------------------------------------------------------------------------
temperature_UpdatedValueHandlerRef_t temperature_AddUpdatedValueHandler
(
    temperature_UpdatedValueHandlerFunc_t handlerPtr,        ///< [IN]
    void* contextPtr                                         ///< [IN]
)
{
    // Create the layered handler.
    // In our example, we receive temperature in C, convert it to F and then send it to the client
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(EVENT_NAME, TemperatureUpdatedEvent,
        FirstLayerStateHandler, (le_event_HandlerFunc_t) handlerPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);

    return (temperature_UpdatedValueHandlerRef_t) (handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
* Remove handler function for EVENT 'temperature_UpdatedValue'
*/
//--------------------------------------------------------------------------------------------------
void temperature_RemoveUpdatedValueHandler
(
    temperature_UpdatedValueHandlerRef_t addHandlerRef        ///< [IN]
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t) addHandlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
* Temperature monitoring server side.
* A client can request to monitor Power Amplifier Temperature or Power controller temperature
* Monitors sensor temperatures, with every updated value an event will occur upon which client is notified
*/
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{

    LE_INFO("psServer Started!");
    TemperatureUpdatedEvent = le_event_CreateId(EVENT_NAME, sizeof(TemperatureUpdatedValue_t));
}
