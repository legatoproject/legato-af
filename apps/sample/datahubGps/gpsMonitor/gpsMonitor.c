#include "legato.h"
#include "interfaces.h"

#define COUNTER_NAME "counter/value"
#define LATITUDE_NAME "location/value/latitude"
#define LONGITUDE_NAME "location/value/longitude"

#define LATITUDE_SENSOR "/app/gpsSensor/location/value/latitude"
#define LONGITUDE_SENSOR "/app/gpsSensor/location/value/longitude"

#define LATITUDE_OBS "latitudeOffLimits"
#define LONGITUDE_OBS "longitudeOffLimits"

#define LATITUDE_LOWER_LIMIT 50.000000
#define LATITUDE_UPPER_LIMIT 50.000100

#define LONGITUDE_LOWER_LIMIT -97.000100
#define LONGITUDE_UPPER_LIMIT -97.000000

//--------------------------------------------------------------------------------------------------
/**
 * Call-back function called when an update is received from the Data Hub for the latitude value.
 */
//--------------------------------------------------------------------------------------------------
static void LatitudeUpdateHandler
(
    double timestamp,   ///< time stamp
    double value,       ///< latitude value, degrees
    void* contextPtr    ///< not used
)
{
    LE_DEBUG("latitude = %lf (timestamped %lf)", value, timestamp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Call-back function called when an update is received from the Data Hub for the longitude value.
 */
//--------------------------------------------------------------------------------------------------
static void LongitudeUpdateHandler
(
    double timestamp,   ///< time stamp
    double value,       ///< longitude value, degrees
    void* contextPtr    ///< not used
)
{
    LE_DEBUG("longitude = %lf (timestamped %lf)", value, timestamp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Call-back function called when an update is received from the Data Hub for the latitude value.
 */
//--------------------------------------------------------------------------------------------------
static void LatitudeObservationUpdateHandler
(
    double timestamp,   ///< time stamp
    double value,       ///< latitude value, degrees
    void* contextPtr    ///< not used
)
{
    LE_INFO("Observed filtered latitude = %lf (timestamped %lf)", value, timestamp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Call-back function called when an update is received from the Data Hub for the latitude value.
 */
//--------------------------------------------------------------------------------------------------
static void LongitudeObservationUpdateHandler
(
    double timestamp,   ///< time stamp
    double value,       ///< longitude value, degrees
    void* contextPtr    ///< not used
)
{
    LE_INFO("Observed filtered longitude = %lf (timestamped %lf)", value, timestamp);
}

COMPONENT_INIT
{
    le_result_t result;

    // This will be received from the Data Hub.
    result = io_CreateOutput(LATITUDE_NAME, IO_DATA_TYPE_NUMERIC, "degrees");
    LE_ASSERT(result == LE_OK);
    result = io_CreateOutput(LONGITUDE_NAME, IO_DATA_TYPE_NUMERIC, "degrees");
    LE_ASSERT(result == LE_OK);

    // Register for notification of updates to the counter value.
    io_AddNumericPushHandler(LATITUDE_NAME, LatitudeUpdateHandler, NULL);
    io_AddNumericPushHandler(LONGITUDE_NAME, LongitudeUpdateHandler, NULL);

    // Connect to the sensor
    result = admin_SetSource("/app/gpsMonitor/" LATITUDE_NAME, LATITUDE_SENSOR);
    LE_ASSERT(result == LE_OK);
    result = admin_SetSource("/app/gpsMonitor/" LONGITUDE_NAME, LONGITUDE_SENSOR);
    LE_ASSERT(result == LE_OK);

    // Create observation (filter) for latitude. To set up the "dead band" filter,
    // lower limit assigned to high limit and vice versa (see admin.io doc for details).
    admin_CreateObs(LATITUDE_OBS);
    admin_SetLowLimit(LATITUDE_OBS, LATITUDE_UPPER_LIMIT);
    admin_SetHighLimit(LATITUDE_OBS, LATITUDE_LOWER_LIMIT);
    result = admin_SetSource("/obs/" LATITUDE_OBS, LATITUDE_SENSOR);
    LE_ASSERT(result == LE_OK);
    admin_AddNumericPushHandler("/obs/" LATITUDE_OBS, LatitudeObservationUpdateHandler, NULL);

    // Create observation for longitude (with dead band filter).
    admin_CreateObs(LONGITUDE_OBS);
    admin_SetLowLimit(LONGITUDE_OBS, LONGITUDE_UPPER_LIMIT);
    admin_SetHighLimit(LONGITUDE_OBS, LONGITUDE_LOWER_LIMIT);
    result = admin_SetSource("/obs/" LONGITUDE_OBS, LONGITUDE_SENSOR);
    LE_ASSERT(result == LE_OK);
    admin_AddNumericPushHandler("/obs/" LONGITUDE_OBS, LongitudeObservationUpdateHandler, NULL);

    LE_ASSERT(result == LE_OK);
}
