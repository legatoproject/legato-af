#include "legato.h"
#include "interfaces.h"

#define dhubIO_DataType_t io_DataType_t

static bool IsEnabled = false;

static le_timer_Ref_t Timer;

#define LATITUDE_NAME "location/value/latitude"
#define LONGITUDE_NAME "location/value/longitude"
#define PERIOD_NAME "location/period"
#define ENABLE_NAME "location/enable"

//--------------------------------------------------------------------------------------------------
/**
 * Function called when the timer expires.
 */
//--------------------------------------------------------------------------------------------------
static void TimerExpired
(
    le_timer_Ref_t timer    ///< Timer reference
)
//--------------------------------------------------------------------------------------------------
{
    static unsigned int counter = 0;
    int32_t latitude = 0;
    int32_t longitude = 0;
    int32_t horizontalAccuracy = 0;

    counter += 1;

    le_result_t result = le_pos_Get2DLocation(&latitude, &longitude, &horizontalAccuracy);
    if (LE_OK != result)
    {
        LE_ERROR("Error %d getting position", result);
    }
    LE_INFO("Location: Latitude %d Longitude %d Accuracy %d", latitude, longitude, horizontalAccuracy);

    // introduce oscillations (20 meters) to latitude
    latitude += counter % 200;

    // Location units have to be converted from 1e-6 degrees to degrees
    io_PushNumeric(LATITUDE_NAME, IO_NOW, latitude * 0.000001);
    io_PushNumeric(LONGITUDE_NAME, IO_NOW, longitude * 0.000001);
}


//--------------------------------------------------------------------------------------------------
/**
 * Call-back function called when an update is received from the Data Hub for the "period"
 * config setting.
 */
//--------------------------------------------------------------------------------------------------
static void PeriodUpdateHandler
(
    double timestamp,   ///< time stamp
    double value,       ///< period value, seconds
    void* contextPtr    ///< not used
)
//--------------------------------------------------------------------------------------------------
{
    LE_INFO("Received update to 'period' setting: %lf (timestamped %lf)", value, timestamp);

    uint32_t ms = (uint32_t)(value * 1000);

    if (ms == 0)
    {
        le_timer_Stop(Timer);
    }
    else
    {
        le_timer_SetMsInterval(Timer, ms);

        // If the sensor is enabled and the timer is not already running, start it now.
        if (IsEnabled && (!le_timer_IsRunning(Timer)))
        {
            le_timer_Start(Timer);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Call-back function called when an update is received from the Data Hub for the "enable"
 * control.
 */
//--------------------------------------------------------------------------------------------------
static void EnableUpdateHandler
(
    double timestamp,   ///< time stamp
    bool value,         ///< whether input is enabled
    void* contextPtr    ///< not used
)
//--------------------------------------------------------------------------------------------------
{
    LE_INFO("Received update to 'enable' setting: %s (timestamped %lf)",
            value == false ? "false" : "true",
            timestamp);

    IsEnabled = value;

    if (value)
    {
        // If the timer has a non-zero interval and is not already running, start it now.
        if ((le_timer_GetMsInterval(Timer) != 0) && (!le_timer_IsRunning(Timer)))
        {
            le_timer_Start(Timer);
        }
    }
    else
    {
        le_timer_Stop(Timer);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Call-back for receiving notification that an update is happening.
 */
//--------------------------------------------------------------------------------------------------
static void UpdateStartEndHandler
(
    bool isStarting,    //< input is starting
    void* contextPtr    //< Not used.
)
//--------------------------------------------------------------------------------------------------
{
    LE_INFO("Configuration update %s.", isStarting ? "starting" : "finished");
}


COMPONENT_INIT
{
    le_result_t result;

    io_AddUpdateStartEndHandler(UpdateStartEndHandler, NULL);

    // This will be provided to the Data Hub.
    result = io_CreateInput(LATITUDE_NAME, IO_DATA_TYPE_NUMERIC, "degrees");
    LE_ASSERT(result == LE_OK);
    result = io_CreateInput(LONGITUDE_NAME, IO_DATA_TYPE_NUMERIC, "degrees");
    LE_ASSERT(result == LE_OK);

    // This is my configuration setting.
    result = io_CreateOutput(PERIOD_NAME, IO_DATA_TYPE_NUMERIC, "s");
    LE_ASSERT(result == LE_OK);

    // Register for notification of updates to our configuration setting.
    io_AddNumericPushHandler(PERIOD_NAME, PeriodUpdateHandler, NULL);

    // This is my enable/disable control.
    result = io_CreateOutput(ENABLE_NAME, IO_DATA_TYPE_BOOLEAN, "");
    LE_ASSERT(result == LE_OK);

    // Set the defaults: enable the sensor, set period to 1s
    io_SetBooleanDefault(ENABLE_NAME, true);
    io_SetNumericDefault(PERIOD_NAME, 1);

    // Register for notification of updates to our enable/disable control.
    io_AddBooleanPushHandler(ENABLE_NAME, EnableUpdateHandler, NULL);

    // Create a repeating timer that will call TimerExpired() each time it expires.
    // Note: we'll start the timer when we receive our configuration setting.
    Timer = le_timer_Create("gpsSensorTimer");
    le_timer_SetRepeat(Timer, 0);
    le_timer_SetHandler(Timer, TimerExpired);
}
