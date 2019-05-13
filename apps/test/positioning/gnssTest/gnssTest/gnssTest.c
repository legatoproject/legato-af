 /**
  * This module implements a test for GNSS device starting.
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include "interfaces.h"

// Include macros for printing out values
#include "le_print.h"

//--------------------------------------------------------------------------------------------------
/**
 *  Position handler reference
 */
//--------------------------------------------------------------------------------------------------
static le_gnss_PositionHandlerRef_t PositionHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 *  Certificate for SUPL testing
 */
//--------------------------------------------------------------------------------------------------
char ShortSuplCertificate[50]={0};

//--------------------------------------------------------------------------------------------------
/**
 *  Number of seconds elapsed since January 1, 1970, not counting leap seconds
 */
//--------------------------------------------------------------------------------------------------
static uint64_t EpochTime=0;

//--------------------------------------------------------------------------------------------------
/**
 *  Time uncertainty in milliseconds
 */
//--------------------------------------------------------------------------------------------------
static uint32_t TimeAccuracy=0;

//--------------------------------------------------------------------------------------------------
/**
 *  DOP resolution
 */
//--------------------------------------------------------------------------------------------------
static le_gnss_Resolution_t DopRes = LE_GNSS_RES_ONE_DECIMAL;

//--------------------------------------------------------------------------------------------------
/**
 *  Semaphore to synchronize position handler with main test thread
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t PositionHandlerSem;

//--------------------------------------------------------------------------------------------------
/**
 *  Maximum wait time for 3D fix
 */
//--------------------------------------------------------------------------------------------------
#define WAIT_MAX_FOR_3DFIX  60

//--------------------------------------------------------------------------------------------------
/**
 *  Unknown constellation bitmask
 */
//--------------------------------------------------------------------------------------------------
#define UNKNOWN_CONSTELLATION  0x80

//--------------------------------------------------------------------------------------------------
/**
 *  These flags are used as conditions to skip tests
 */
//--------------------------------------------------------------------------------------------------
#ifdef SIERRA_MDM9X40
#define MDM9X40_PLATFORM 1
#else
#define MDM9X40_PLATFORM 0
#endif

#ifdef SIERRA_MDM9X28
#define MDM9X28_PLATFORM 1
#else
#define MDM9X28_PLATFORM 0
#endif

#ifdef LE_CONFIG_LINUX
#define LINUX_OS 1
#else
#define LINUX_OS 0
#endif

//--------------------------------------------------------------------------------------------------
//                                       Test Functions
//--------------------------------------------------------------------------------------------------

//! [GnssEnable]
//--------------------------------------------------------------------------------------------------
/**
 * Test: Legato GNSS functions.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestLeGnssDevice
(
    void
)
{
    uint32_t ttffValue;
    uint32_t acqRate;
    uint8_t  minElevation;
    le_result_t result;
    int32_t altitudeOnWgs84=0;
    int64_t altitudeOnPZ90;

    le_gnss_ConstellationBitMask_t constellationMask;
    le_gnss_NmeaBitMask_t nmeaMask = 0;
    le_gnss_ConstellationArea_t constellationArea;

    LE_INFO("Start Test Testle_gnss_DeviceTest");
    // GNSS device enabled by default
    LE_TEST_OK(((le_gnss_GetState()) == LE_GNSS_STATE_READY), "Get GNSS state");
    LE_TEST_OK((le_gnss_Enable()) == LE_DUPLICATE, "Enable GNSS");
    // Disable GNSS device (DISABLED state)
    LE_TEST_OK((le_gnss_Disable()) == LE_OK, "Disable GNSS");
    LE_TEST_OK((le_gnss_Disable()) == LE_DUPLICATE, "Duplicate disable");
    // Check Disabled state
    LE_TEST_OK((le_gnss_GetState()) == LE_GNSS_STATE_DISABLED, "Get GNSS state");
    LE_TEST_OK((le_gnss_Start()) == LE_NOT_PERMITTED, "Start GNSS in disabled state");
    LE_TEST_OK((le_gnss_ForceHotRestart()) == LE_NOT_PERMITTED, "Hot restart in disabled state");
    LE_TEST_OK((le_gnss_ForceWarmRestart()) == LE_NOT_PERMITTED, "Warm restart in disabled state");
    LE_TEST_OK((le_gnss_ForceColdRestart()) == LE_NOT_PERMITTED, "Cold restart in disabled state");
    LE_TEST_OK((le_gnss_ForceFactoryRestart()) == LE_NOT_PERMITTED,
            "Factory restart in disabled state");
    LE_TEST_OK((le_gnss_StartMode(LE_GNSS_HOT_START)) == LE_NOT_PERMITTED,
               "Hot start in disabled state");
    LE_TEST_OK((le_gnss_StartMode(LE_GNSS_WARM_START)) == LE_NOT_PERMITTED,
               "Warm start in disabled state");
    LE_TEST_OK((le_gnss_StartMode(LE_GNSS_COLD_START)) == LE_NOT_PERMITTED,
               "Cold start in disabled state");
    LE_TEST_OK((le_gnss_StartMode(LE_GNSS_FACTORY_START)) == LE_NOT_PERMITTED,
               "Factory start in disabled state");

    LE_TEST_OK((le_gnss_GetTtff(&ttffValue)) == LE_NOT_PERMITTED, "Get TTFF in disabled state");
    LE_TEST_OK((le_gnss_Stop()) == LE_NOT_PERMITTED, "Stop GNSS in disabled state");
    LE_TEST_OK((le_gnss_SetConstellation(LE_GNSS_CONSTELLATION_GPS)) == LE_NOT_PERMITTED,
            "Set constellation in disabled state");
    LE_TEST_OK((le_gnss_GetConstellation(&constellationMask)) == LE_NOT_PERMITTED,
            "Get constellation in disabled state");

    LE_TEST_BEGIN_SKIP(!LINUX_OS, 6);
    LE_TEST_OK(LE_NOT_PERMITTED == le_gnss_SetConstellationArea(LE_GNSS_SV_CONSTELLATION_GPS,
                                                                LE_GNSS_WORLDWIDE_AREA),
            "Set GPS constellation area in disabled state");
    LE_TEST_OK(LE_NOT_PERMITTED == le_gnss_GetConstellationArea(LE_GNSS_SV_CONSTELLATION_GPS,
                                                                &constellationArea),
            "Get GPS constellation area in disabled state");

    LE_TEST_OK(LE_NOT_PERMITTED == le_gnss_SetConstellationArea(LE_GNSS_SV_CONSTELLATION_GLONASS,
                                                                LE_GNSS_WORLDWIDE_AREA),
            "Set GLONASS constellation area in disabled state");
    LE_TEST_OK(LE_NOT_PERMITTED == le_gnss_GetConstellationArea(LE_GNSS_SV_CONSTELLATION_GLONASS,
                                                                &constellationArea),
            "Get GLONASS constellation area in disabled state");

    LE_TEST_OK((le_gnss_GetAcquisitionRate(&acqRate)) == LE_NOT_PERMITTED,
            "Get acquisition rate in disabled state");
    result = le_gnss_SetAcquisitionRate(acqRate);
    LE_TEST_OK((result == LE_NOT_PERMITTED) || (result == LE_OUT_OF_RANGE),
            "Set acquisition rate in disabled state");
    LE_TEST_END_SKIP();

    LE_TEST_OK((le_gnss_SetNmeaSentences(nmeaMask)) == LE_NOT_PERMITTED,
            "Set NMEA sentences in disabled state");
    LE_TEST_OK((le_gnss_GetNmeaSentences(&nmeaMask)) == LE_NOT_PERMITTED,
            "Get NMEA sentences in disabled state");

    // test le_gnss_Get/SetMinElevation when GNSS device is disabled and the engine is not started.
    LE_TEST_BEGIN_SKIP(!LINUX_OS, 3);
    minElevation = 40;
    LE_TEST_OK((le_gnss_SetMinElevation(minElevation)) == LE_OK, "Set min elevation");
    LE_TEST_OK((le_gnss_GetMinElevation(&minElevation)) == LE_OK, "Get min elevation");
    LE_INFO("GNSS min elevation obtained: %d",minElevation);
    LE_TEST_OK(minElevation == 40, "Confirm min elevation is set to %d", minElevation);
    LE_TEST_END_SKIP();

    // Enable GNSS device (READY state)
    LE_TEST_OK((le_gnss_Enable()) == LE_OK, "Enable GNSS");
    LE_TEST_OK((le_gnss_GetState()) == LE_GNSS_STATE_READY, "Get GNSS state");
    LE_TEST_OK((le_gnss_Disable()) == LE_OK, "Disable GNSS");
    LE_TEST_OK((le_gnss_GetState()) == LE_GNSS_STATE_DISABLED, "Get GNSS state");
    LE_TEST_OK((le_gnss_Enable()) == LE_OK, "Enable GNSS");
    LE_TEST_OK((le_gnss_GetState()) == LE_GNSS_STATE_READY, "Get GNSS state");

    LE_TEST_OK((le_gnss_Stop()) == LE_DUPLICATE, "Duplicate GNSS stop");

    // Unpermitted force restart in READY state
    LE_TEST_OK((le_gnss_ForceHotRestart()) == LE_NOT_PERMITTED, "Unpermitted hot restart");
    LE_TEST_OK((le_gnss_ForceWarmRestart()) == LE_NOT_PERMITTED, "Unpermitted warm restart");
    LE_TEST_OK((le_gnss_ForceColdRestart()) == LE_NOT_PERMITTED, "Unpermitted cold restart");
    LE_TEST_OK((le_gnss_ForceFactoryRestart()) == LE_NOT_PERMITTED, "Unpermitted factory restart");

    LE_TEST_OK(le_gnss_SetConstellation(LE_GNSS_CONSTELLATION_GPS) == LE_OK,
            "Set constellation GPS");
    LE_TEST_OK(le_gnss_GetConstellation(&constellationMask) == LE_OK, "Get contellation");
    LE_TEST_OK(constellationMask == LE_GNSS_CONSTELLATION_GPS,
            "Confirm constellation is set to %d", LE_GNSS_CONSTELLATION_GPS);

    LE_TEST_BEGIN_SKIP(!LINUX_OS, 7);
    LE_TEST_OK(LE_BAD_PARAMETER == (le_gnss_SetConstellationArea(LE_GNSS_SV_CONSTELLATION_GALILEO,
                                                                LE_GNSS_UNSET_AREA)),
            "Set invalid Galileo constellation area");

    LE_TEST_OK(LE_OK == le_gnss_SetConstellationArea(LE_GNSS_SV_CONSTELLATION_GALILEO,
                                              LE_GNSS_OUTSIDE_US_AREA),
            "Set Galileo constellation area outside US");
    LE_TEST_OK(LE_OK == le_gnss_GetConstellationArea(LE_GNSS_SV_CONSTELLATION_GALILEO,
                                              &constellationArea),
            "Get Galileo constellation area");
    LE_TEST_OK(LE_GNSS_OUTSIDE_US_AREA == constellationArea,
            "Confirm Galileo constellation area is set to %d", LE_GNSS_OUTSIDE_US_AREA);

    LE_TEST_OK(LE_OK == le_gnss_SetConstellationArea(LE_GNSS_SV_CONSTELLATION_GALILEO,
                                              LE_GNSS_WORLDWIDE_AREA),
            "Set Galileo constellation area worldwide");
    LE_TEST_OK(LE_OK == le_gnss_GetConstellationArea(LE_GNSS_SV_CONSTELLATION_GALILEO,
                                              &constellationArea),
            "Get Galileo constellation area");
    LE_TEST_OK(LE_GNSS_WORLDWIDE_AREA == constellationArea,
            "Confirm Galileo constellation area is set to %d", LE_GNSS_WORLDWIDE_AREA);
    LE_TEST_END_SKIP();

    // Get/Set AcquisitionRate
    LE_TEST_BEGIN_SKIP(!LINUX_OS, 3);
    LE_TEST_OK((le_gnss_GetAcquisitionRate(&acqRate)) == LE_OK, "Get acquisition rate");
    acqRate = 0;
    LE_TEST_OK(le_gnss_SetAcquisitionRate(acqRate) == LE_OUT_OF_RANGE,
            "Set invalid acquisition rate");
    acqRate = 1100;
    LE_TEST_OK((le_gnss_SetAcquisitionRate(acqRate)) == LE_OK, "Set acquisition rate");
    LE_TEST_END_SKIP();

    LE_TEST_OK((le_gnss_GetNmeaSentences(&nmeaMask)) == LE_OK, "Get NMEA sentences");
    LE_TEST_INFO("NMEA mask: %x",nmeaMask);
    LE_TEST_OK((le_gnss_SetNmeaSentences(nmeaMask)) == LE_OK, "Set NMEA sentences");

    // test le_gnss_Get/SetMinElevation when GNSS device is enabled and the engine is not started.
    LE_TEST_BEGIN_SKIP(!LINUX_OS, 3);
    minElevation = 0;
    LE_TEST_OK((le_gnss_SetMinElevation(minElevation)) == LE_OK, "Set min elevation");
    LE_TEST_OK((le_gnss_GetMinElevation(&minElevation)) == LE_OK, "Get min elevation");
    LE_TEST_INFO("GNSS min elevation obtained: %d",minElevation);
    LE_TEST_OK(minElevation == 0, "Confirm min elevation is set to 0");
    LE_TEST_END_SKIP();

    // Start GNSS device (ACTIVE state)
    LE_TEST_ASSERT(((le_gnss_Start()) == LE_OK), "Start GNSS");
    LE_TEST_OK((le_gnss_GetState()) == LE_GNSS_STATE_ACTIVE, "Get GNSS state");
    LE_TEST_OK((le_gnss_Start()) == LE_DUPLICATE, "Duplicate GNSS start");
    LE_TEST_OK((le_gnss_Enable()) == LE_DUPLICATE, "Duplicate GNSS enable");
    LE_TEST_OK((le_gnss_Disable()) == LE_NOT_PERMITTED, "Disable in wrong state");
    LE_TEST_OK((le_gnss_SetConstellation(LE_GNSS_CONSTELLATION_GPS)) == LE_NOT_PERMITTED,
            "Set constellation in wrong state");
    LE_TEST_OK((le_gnss_GetConstellation(&constellationMask)) == LE_NOT_PERMITTED,
            "Get constellation in wrong state");

    // Test le_gnss_StartMode() in ACTIVE state
    LE_TEST_OK((le_gnss_StartMode(LE_GNSS_HOT_START)) == LE_DUPLICATE,
               "Hot start in active state");
    LE_TEST_OK((le_gnss_StartMode(LE_GNSS_WARM_START)) == LE_DUPLICATE,
               "Warm start in active state");
    LE_TEST_OK((le_gnss_StartMode(LE_GNSS_COLD_START)) == LE_DUPLICATE,
               "Cold start in active state");
    LE_TEST_OK((le_gnss_StartMode(LE_GNSS_FACTORY_START)) == LE_DUPLICATE,
               "Factory start in active state");

    LE_TEST_BEGIN_SKIP(!LINUX_OS, 2);
    LE_TEST_OK((le_gnss_GetAcquisitionRate(&acqRate)) == LE_NOT_PERMITTED,
            "Get acquisition rate in wrong state");
    result = le_gnss_SetAcquisitionRate(acqRate);
    LE_TEST_OK((result == LE_NOT_PERMITTED) || (result == LE_OUT_OF_RANGE),
            "Set acquisition rate in wrong state");
    LE_TEST_END_SKIP();

    LE_TEST_OK((le_gnss_SetNmeaSentences(nmeaMask)) == LE_NOT_PERMITTED,
            "Set NMEA sentences in wrong state");
    LE_TEST_OK((le_gnss_GetNmeaSentences(&nmeaMask)) == LE_NOT_PERMITTED,
            "Get NMEA sentences in wrong state");

    // test le_gnss_Get/SetMinElevation when le_gnss_ENABLE ON and le_gnss_Start ON
    LE_TEST_BEGIN_SKIP(!LINUX_OS, 4);
    minElevation = LE_GNSS_MIN_ELEVATION_MAX_DEGREE;
    LE_TEST_OK((le_gnss_SetMinElevation(minElevation)) == LE_OK, "Set minimum elevation");
    LE_TEST_OK((le_gnss_GetMinElevation(&minElevation)) == LE_OK, "Get minimum elevation");
    LE_TEST_INFO("GNSS min elevation obtained: %d",minElevation);
    LE_TEST_OK(minElevation == LE_GNSS_MIN_ELEVATION_MAX_DEGREE,
            "Confirm min elevation is set to %d", LE_GNSS_MIN_ELEVATION_MAX_DEGREE);

    // test le_gnss_SetMinElevation wrong value (when le_gnss_ENABLE ON and le_gnss_Start ON)
    minElevation = LE_GNSS_MIN_ELEVATION_MAX_DEGREE+1;
    LE_TEST_OK((le_gnss_SetMinElevation(minElevation)) == LE_OUT_OF_RANGE,
            "Set invalid min elevation");
    LE_TEST_END_SKIP();

    // Stop GNSS device (READY state)
    LE_TEST_OK((le_gnss_Stop()) == LE_OK, "Stop GNSS");
    LE_TEST_OK((le_gnss_GetState()) == LE_GNSS_STATE_READY, "Confirm GNSS is ready");
    LE_TEST_OK((le_gnss_Enable()) == LE_DUPLICATE, "Duplicate GNSS enable");
    LE_TEST_OK((le_gnss_Disable()) == LE_OK, "Disable GNSS");
    LE_TEST_OK((le_gnss_GetState()) == LE_GNSS_STATE_DISABLED, "Confirm GNSS is disabled");
    LE_TEST_OK((le_gnss_Enable()) == LE_OK, "Enable GNSS");
    LE_TEST_OK((le_gnss_GetState()) == LE_GNSS_STATE_READY, "Confirm GNSS is ready");
    LE_TEST_OK((le_gnss_SetConstellation(LE_GNSS_CONSTELLATION_GPS)) == LE_OK,
            "Set GPS constellation");
    LE_TEST_OK((le_gnss_GetConstellation(&constellationMask)) == LE_OK, "Get constellation");
    LE_TEST_OK(constellationMask == LE_GNSS_CONSTELLATION_GPS,
            "Confirm constellation is set to GPS");
    LE_TEST_OK((le_gnss_Stop()) == LE_DUPLICATE, "Duplicate GNSS stop");

    LE_TEST_BEGIN_SKIP(!LINUX_OS, 2);
    LE_TEST_OK((le_gnss_GetAcquisitionRate(&acqRate)) == LE_OK, "Get acquisition rate");
    LE_TEST_OK((le_gnss_SetAcquisitionRate(acqRate)) == LE_OK, "Set acquisition rate");
    LE_TEST_END_SKIP();

    LE_TEST_OK((le_gnss_GetNmeaSentences(&nmeaMask)) == LE_OK, "Get NMEA sentences");
    LE_TEST_OK((le_gnss_SetNmeaSentences(nmeaMask)) == LE_OK, "Set NMEA sentences");

    // test le_gnss_ConvertDataCoordinate error cases
    LE_TEST_BEGIN_SKIP(!LINUX_OS, 5);
    LE_TEST_OK(LE_FAULT == (le_gnss_ConvertDataCoordinateSystem(LE_GNSS_COORDINATE_SYSTEM_WGS84,
                                                               LE_GNSS_COORDINATE_SYSTEM_PZ90,
                                                               LE_GNSS_POS_LATITUDE,
                                                               altitudeOnWgs84,
                                                               NULL)),
            "ConvertDataCoordinateSystem error test: NULL pointer");
    LE_TEST_OK(LE_BAD_PARAMETER == (le_gnss_ConvertDataCoordinateSystem(
                                                         LE_GNSS_COORDINATE_SYSTEM_MAX,
                                                         LE_GNSS_COORDINATE_SYSTEM_PZ90,
                                                         LE_GNSS_POS_LATITUDE,
                                                         altitudeOnWgs84,
                                                         &altitudeOnPZ90)),
            "ConvertDataCoordinateSystem error test: invalid source coordinate");
    LE_TEST_OK(LE_BAD_PARAMETER == (le_gnss_ConvertDataCoordinateSystem(
                                                         LE_GNSS_COORDINATE_SYSTEM_PZ90,
                                                         LE_GNSS_COORDINATE_SYSTEM_PZ90,
                                                         LE_GNSS_POS_LATITUDE,
                                                         altitudeOnWgs84,
                                                         &altitudeOnPZ90)),
            "ConvertDataCoordinateSystem error test: wrong source coordinate");
    LE_TEST_OK(LE_BAD_PARAMETER == (le_gnss_ConvertDataCoordinateSystem(
                                                         LE_GNSS_COORDINATE_SYSTEM_WGS84,
                                                         LE_GNSS_COORDINATE_SYSTEM_PZ90,
                                                         LE_GNSS_POS_MAX,
                                                         altitudeOnWgs84,
                                                         &altitudeOnPZ90)),
            "ConvertDataCoordinateSystem error test: invalid data type");
    LE_TEST_OK(LE_FAULT == (le_gnss_ConvertDataCoordinateSystem(LE_GNSS_COORDINATE_SYSTEM_PZ90,
                                                         LE_GNSS_COORDINATE_SYSTEM_WGS84,
                                                         LE_GNSS_POS_ALTITUDE,
                                                         altitudeOnWgs84,
                                                         &altitudeOnPZ90)),
            "ConvertDataCoordinateSystem error test: mismatched coordinates");
    LE_TEST_END_SKIP();
}
//! [GnssEnable]

//! [GnssPosition]
//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Position Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PositionHandlerFunction
(
    le_gnss_SampleRef_t positionSampleRef,
    void* contextPtr
)
{
    le_result_t result;
    // Date parameters
    uint16_t year;
    uint16_t month;
    uint16_t day;
    // Time parameters
    uint16_t hours;
    uint16_t minutes;
    uint16_t seconds;
    uint16_t milliseconds;
    // GPS time
    uint32_t gpsWeek;
    uint32_t gpsTimeOfWeek;
    // Leap seconds in advance
    uint8_t leapSeconds = 0;
    // Position state
    le_gnss_FixState_t state;
    // Location
    int32_t     latitude;
    int32_t     longitude;
    int64_t     latitudeOnPZ90;
    int64_t     longitudeOnPZ90;
    int32_t     altitude;
    int32_t     altitudeOnWgs84;
    int64_t     altitudeOnPZ90;
    int32_t     hAccuracy;
    int32_t     vAccuracy;
    int32_t     magneticDeviation;
    // DOP parameter
    uint16_t dop;
    // Horizontal speed
    uint32_t hSpeed;
    uint32_t hSpeedAccuracy;
    // Vertical speed
    int32_t vSpeed = 0;
    int32_t vSpeedAccuracy = 0;
    // Direction
    uint32_t direction = 0;
    uint32_t directionAccuracy = 0;
    le_gnss_DopType_t dopType = LE_GNSS_PDOP;
    le_gnss_Resolution_t dataRes = LE_GNSS_RES_ZERO_DECIMAL;
    uint16_t satIdPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satIdNumElements = NUM_ARRAY_MEMBERS(satIdPtr);
    le_gnss_Constellation_t satConstPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satConstNumElements = NUM_ARRAY_MEMBERS(satConstPtr);
    bool satUsedPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satUsedNumElements = NUM_ARRAY_MEMBERS(satUsedPtr);
    uint8_t satSnrPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satSnrNumElements = NUM_ARRAY_MEMBERS(satSnrPtr);
    uint16_t satAzimPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satAzimNumElements = NUM_ARRAY_MEMBERS(satAzimPtr);
    uint8_t satElevPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satElevNumElements = NUM_ARRAY_MEMBERS(satElevPtr);
    uint8_t satsInViewCount;
    uint8_t satsTrackingCount;
    uint8_t satsUsedCount;
    int i;

    static const char *tabDop[] =
    {
        "Position dilution of precision (PDOP)",
        "Horizontal dilution of precision (HDOP)",
        "Vertical dilution of precision (VDOP)",
        "Geometric dilution of precision (GDOP)",
        "Time dilution of precision (TDOP)"
    };

    if (NULL == positionSampleRef)
    {
        LE_ERROR("New Position sample is NULL!");
    }
    else
    {
        LE_DEBUG("New Position sample %p", positionSampleRef);
    }

    // Get position state
    result = le_gnss_GetPositionState(positionSampleRef, &state);
    if(state == LE_GNSS_STATE_FIX_NO_POS)
    {
        le_gnss_ReleaseSampleRef(positionSampleRef);
        return;
    }


    LE_TEST_OK((LE_OK == result), "Get position state");
    LE_TEST_INFO("Position state: %s", (LE_GNSS_STATE_FIX_NO_POS == state)?"No Fix"
                                 :(LE_GNSS_STATE_FIX_2D == state)?"2D Fix"
                                 :(LE_GNSS_STATE_FIX_3D == state)?"3D Fix"
                                 : "Unknown");

    LE_TEST_OK(LE_OK == le_gnss_Stop(), "Stop GNSS after getting a fix");

    // Get UTC date
    result = le_gnss_GetDate(positionSampleRef, &year, &month, &day);
    LE_TEST_OK((LE_OK == result) || (LE_OUT_OF_RANGE == result), "Get date");

    // Get UTC time
    result = le_gnss_GetTime(positionSampleRef, &hours, &minutes, &seconds, &milliseconds);
    LE_TEST_OK((LE_OK == result) || (LE_OUT_OF_RANGE == result), "Get time");

    // Get Epoch time
    result = le_gnss_GetEpochTime(positionSampleRef, &EpochTime);
    LE_TEST_OK((LE_OK == result) || (LE_OUT_OF_RANGE == result), "Get epoch time");

   // Display time/date format 13:45:30 2009-06-15
    LE_TEST_INFO("%02d:%02d:%02d %d-%02d-%02d,", hours, minutes, seconds, year, month, day);

    // Display Epoch time
    LE_TEST_INFO("epoch time: %llu:", (unsigned long long int) EpochTime);

    LE_TEST_BEGIN_SKIP(!LINUX_OS, 4);
    LE_TEST_OK(LE_OK == le_gnss_InjectUtcTime(EpochTime , 0), "Inject UTC time");

    // Get GPS time
    result = le_gnss_GetGpsTime(positionSampleRef, &gpsWeek, &gpsTimeOfWeek);
    LE_TEST_OK((LE_OK == result) || (LE_OUT_OF_RANGE == result), "Get GPS time");

    LE_TEST_INFO("GPS time W %02d:ToW %dms", gpsWeek, gpsTimeOfWeek);

    // Get time accuracy
    result = le_gnss_GetTimeAccuracy(positionSampleRef, &TimeAccuracy);
    LE_TEST_OK((LE_OK == result) || (LE_OUT_OF_RANGE == result), "Get time accuracy");

    LE_TEST_INFO("GPS time acc %d", TimeAccuracy);

    // Get UTC leap seconds in advance
    result = le_gnss_GetGpsLeapSeconds(positionSampleRef, &leapSeconds);
    LE_TEST_OK((LE_OK == result) || (LE_OUT_OF_RANGE == result), "Get GPS leap seconds");
    LE_TEST_END_SKIP();

    LE_TEST_INFO("UTC leap seconds in advance %d", leapSeconds);

    // Get Location
    result = le_gnss_GetLocation(positionSampleRef, &latitude, &longitude, &hAccuracy);
    LE_TEST_OK((LE_OK == result) || (LE_OUT_OF_RANGE == result), "Get location");

    if (LE_OK == result)
    {
        LE_INFO("Position lat.%f, long.%f, hAccuracy.%f",
                (float)latitude/1000000.0,
                (float)longitude/1000000.0,
                (float)hAccuracy/100.0);

        // Latitude conversion
        result = le_gnss_ConvertDataCoordinateSystem(LE_GNSS_COORDINATE_SYSTEM_WGS84,
                                                     LE_GNSS_COORDINATE_SYSTEM_PZ90,
                                                     LE_GNSS_POS_LATITUDE,
                                                     latitude,
                                                     &latitudeOnPZ90);
        LE_TEST_OK((LE_OK == result) || (LE_UNSUPPORTED == result),
                "Convert latitude from WGS84 to PZ90");
        if (LE_OK == result)
        {
            LE_INFO("Latitude: On WGS84 %d, On PZ90 %" PRId64 ", float %f",
                    latitude,
                    latitudeOnPZ90,
                    (float)latitudeOnPZ90/1000000.0);
        }

        // Longitude conversion
        result = le_gnss_ConvertDataCoordinateSystem(LE_GNSS_COORDINATE_SYSTEM_WGS84,
                                                     LE_GNSS_COORDINATE_SYSTEM_PZ90,
                                                     LE_GNSS_POS_LONGITUDE,
                                                     longitude,
                                                     &longitudeOnPZ90);
        LE_TEST_OK((LE_OK == result) || (LE_UNSUPPORTED == result),
                "Convert longitude from WGS84 to PZ90");
        if (LE_OK == result)
        {
            LE_TEST_INFO("Longitude: On WGS84 %d, On PZ90 %" PRId64 ", float %f",
                    longitude,
                    longitudeOnPZ90,
                    (float)longitudeOnPZ90/1000000.0);
        }
    }
    else
    {
        if (INT32_MAX != latitude)
        {
            LE_TEST_INFO("Latitude %f", (float)latitude/1000000.0);
        }
        else
        {
            LE_TEST_INFO("Latitude unknown %d", latitude);
        }

        if (INT32_MAX != longitude)
        {
            LE_TEST_INFO("Latitude %f", (float)longitude/1000000.0);
        }
        else
        {
            LE_TEST_INFO("Longitude unknown %d", longitude);
        }

        if (INT32_MAX != hAccuracy)
        {
            LE_TEST_INFO("Horizontal accuracy %f", (float)hAccuracy/100.0);
        }
        else
        {
            LE_TEST_INFO("Horizontal accuracy unknown %d", hAccuracy);
        }
    }

    // Get altitude
    LE_TEST_INFO("Test SetDataResolution() for vAccuracy parameter of le_gnss_GetAltitude() function");

    for (dataRes=LE_GNSS_RES_ZERO_DECIMAL; dataRes<LE_GNSS_RES_UNKNOWN; dataRes++)
    {
        LE_TEST_BEGIN_SKIP(!LINUX_OS, 1);
        LE_TEST_OK(LE_OK == le_gnss_SetDataResolution(LE_GNSS_DATA_VACCURACY, dataRes),
                "Set data resolution for vAccuracy");
        LE_TEST_END_SKIP();

        result = le_gnss_GetAltitude( positionSampleRef, &altitude, &vAccuracy);
        LE_TEST_OK((LE_OK == result) || (LE_OUT_OF_RANGE == result), "Get altitude");

        if (LE_OK == result)
        {
            switch(dataRes)
            {
                case LE_GNSS_RES_ZERO_DECIMAL:
                    LE_TEST_INFO("Resolution: %d decimal place, altitude.%f, vAccuracy.%f",
                            dataRes, (float)altitude/1000.0, (float)vAccuracy);
                    break;
                case LE_GNSS_RES_ONE_DECIMAL:
                    LE_TEST_INFO("Resolution: %d decimal place, altitude.%f, vAccuracy.%f",
                            dataRes, (float)altitude/1000.0, (float)vAccuracy/10.0);
                    break;
                case LE_GNSS_RES_TWO_DECIMAL:
                    LE_TEST_INFO("Resolution: %d decimal place, altitude.%f, vAccuracy.%f",
                            dataRes, (float)altitude/1000.0, (float)vAccuracy/100.0);
                    break;
                case LE_GNSS_RES_THREE_DECIMAL:
                    LE_TEST_INFO("Resolution: %d decimal place, altitude.%f, vAccuracy.%f",
                            dataRes, (float)altitude/1000.0, (float)vAccuracy/1000.0);
                    break;
                default:
                    LE_TEST_INFO("Unknown resolution.");
                    break;
            }
        }
        else
        {
            LE_TEST_INFO("Altitude unknown [%d,%d]", altitude, vAccuracy);
        }
    }

    // Get altitude in meters, between WGS-84 earth ellipsoid
    // and mean sea level [resolution 1e-3]
    result = le_gnss_GetAltitudeOnWgs84(positionSampleRef, &altitudeOnWgs84);
    LE_TEST_OK((LE_OK == result) || (LE_OUT_OF_RANGE == result), "Get altitude on WGS84");

    if (LE_OK == result)
    {
        LE_TEST_INFO("AltitudeOnWgs84: %f", (float)altitudeOnWgs84/1000.0);

        result = le_gnss_ConvertDataCoordinateSystem(LE_GNSS_COORDINATE_SYSTEM_WGS84,
                                                     LE_GNSS_COORDINATE_SYSTEM_PZ90,
                                                     LE_GNSS_POS_ALTITUDE,
                                                     altitudeOnWgs84,
                                                     &altitudeOnPZ90);
        LE_TEST_OK((LE_OK == result) || (LE_UNSUPPORTED == result),
                "Convert altitude from WGS84 to PZ90");
        if (LE_OK == result)
        {
            LE_TEST_INFO("Altitude: On WGS84: %d, On PZ90 %" PRId64 ", float %f",
                    altitudeOnWgs84,
                    altitudeOnPZ90,
                    (float)altitudeOnPZ90/1000.0);
        }
    }
    else
    {
        LE_TEST_INFO("AltitudeOnWgs84 unknown [%d]", altitudeOnWgs84);
    }

    LE_TEST_INFO("Dop parameters: \n");

    LE_TEST_OK(LE_OK == le_gnss_SetDopResolution(DopRes), "Set DOP resolution");
    LE_TEST_INFO("Set DOP resolution: %d decimal place\n", DopRes);

    do
    {
        // Get DOP parameter
        result = le_gnss_GetDilutionOfPrecision(positionSampleRef, dopType, &dop);
        LE_TEST_OK((result == LE_OK) || (result == LE_OUT_OF_RANGE), "Get dopType:%d", dopType);
        if (LE_OK == result)
        {
            switch(DopRes)
            {
                case LE_GNSS_RES_ZERO_DECIMAL:
                     LE_TEST_INFO("resolution: %d decimal place, %s %.1f\n",
                            DopRes, tabDop[dopType], (float)dop);
                     break;
                case LE_GNSS_RES_ONE_DECIMAL:
                     LE_TEST_INFO("resolution: %d decimal place, %s %.1f\n",
                            DopRes, tabDop[dopType], (float)(dop)/10);
                     break;
                case LE_GNSS_RES_TWO_DECIMAL:
                     LE_TEST_INFO("resolution: %d decimal place, %s %.2f\n",
                            DopRes, tabDop[dopType], (float)(dop)/100);
                     break;
                case LE_GNSS_RES_THREE_DECIMAL:
                default:
                     LE_TEST_INFO("resolution: %d decimal place, %s %.3f\n",
                            DopRes, tabDop[dopType], (float)(dop)/1000);
                     break;
            }
        }
        else
        {
            LE_TEST_INFO("%s invalid %d\n", tabDop[dopType], dop);
        }
        dopType++;
    }
    while (dopType != LE_GNSS_DOP_LAST);

    // Get horizontal speed
    LE_TEST_INFO("Test SetDataResolution() for hSpeedAccuracy parameter of le_gnss_GetHorizontalSpeed() \
            function");

    for (dataRes=LE_GNSS_RES_ZERO_DECIMAL; dataRes<LE_GNSS_RES_UNKNOWN; dataRes++)
    {
        LE_TEST_BEGIN_SKIP(!LINUX_OS, 1);
        LE_TEST_OK(LE_OK == le_gnss_SetDataResolution(LE_GNSS_DATA_HSPEEDACCURACY, dataRes),
                "Set data resolution for hSpeedAccuracy");
        LE_TEST_END_SKIP();

        result = le_gnss_GetHorizontalSpeed( positionSampleRef, &hSpeed, &hSpeedAccuracy);
        LE_TEST_OK((LE_OK == result) || (LE_OUT_OF_RANGE == result), "Get horizontal speed");

        if (LE_OK == result)
        {
            switch(dataRes)
            {
                case LE_GNSS_RES_ZERO_DECIMAL:
                    LE_TEST_INFO("Resolution: %d decimal place, hSpeed %u - Accuracy %.3f",
                            dataRes, hSpeed/100, (float)hSpeedAccuracy);
                    break;
                case LE_GNSS_RES_ONE_DECIMAL:
                    LE_TEST_INFO("Resolution: %d decimal place, hSpeed %u - Accuracy %.3f",
                            dataRes, hSpeed/100, (float)hSpeedAccuracy/10);
                    break;
                case LE_GNSS_RES_TWO_DECIMAL:
                    LE_TEST_INFO("Resolution: %d decimal place, hSpeed %u - Accuracy %.3f",
                            dataRes, hSpeed/100, (float)hSpeedAccuracy/100);
                    break;
                case LE_GNSS_RES_THREE_DECIMAL:
                    LE_TEST_INFO("Resolution: %d decimal place, hSpeed %u - Accuracy %.3f",
                            dataRes, hSpeed/100, (float)hSpeedAccuracy/1000);
                    break;
                default:
                    LE_TEST_INFO("Unknown resolution.");
                    break;
            }
        }
        else
        {
            LE_TEST_INFO("hSpeed unknown [%u,%.3f]", hSpeed, (float)hSpeedAccuracy);
        }
    }

    // Get vertical speed
    for (dataRes=LE_GNSS_RES_ZERO_DECIMAL; dataRes<LE_GNSS_RES_UNKNOWN; dataRes++)
    {
        LE_TEST_BEGIN_SKIP(!LINUX_OS, 2);
        LE_TEST_OK(LE_OK == le_gnss_SetDataResolution(LE_GNSS_DATA_VSPEEDACCURACY, dataRes),
                "Set data resolution for vSpeedAccuracy");
        result = le_gnss_GetVerticalSpeed( positionSampleRef, &vSpeed, &vSpeedAccuracy);
        LE_TEST_OK((LE_OK == result) || (LE_OUT_OF_RANGE == result), "Get vertical speed");
        LE_TEST_END_SKIP();

        if (LE_OK == result)
        {
            switch(dataRes)
            {
                case LE_GNSS_RES_ZERO_DECIMAL:
                    LE_TEST_INFO("Resolution: %d decimal place, vSpeed %d - Accuracy %.3f",
                            dataRes, vSpeed/100, (float)vSpeedAccuracy);
                    break;
                case LE_GNSS_RES_ONE_DECIMAL:
                    LE_TEST_INFO("Resolution: %d decimal place, vSpeed %d - Accuracy %.3f",
                            dataRes, vSpeed/100, (float)vSpeedAccuracy/10);
                    break;
                case LE_GNSS_RES_TWO_DECIMAL:
                    LE_TEST_INFO("Resolution: %d decimal place, vSpeed %d - Accuracy %.3f",
                            dataRes, vSpeed/100, (float)vSpeedAccuracy/100);
                    break;
                case LE_GNSS_RES_THREE_DECIMAL:
                    LE_TEST_INFO("Resolution: %d decimal place, vSpeed %d - Accuracy %.3f",
                            dataRes, vSpeed/100, (float)vSpeedAccuracy/1000);
                    break;
                default:
                    LE_TEST_INFO("Unknown resolution.");
                    break;
            }
        }
        else
        {
            LE_TEST_INFO("vSpeed unknown [%d,%.3f]", vSpeed, (float)vSpeedAccuracy);
        }
    }

    // Get direction
    LE_TEST_BEGIN_SKIP(!LINUX_OS, 1);
    LE_TEST_OK((LE_OK == (result = le_gnss_GetDirection(positionSampleRef, &direction,
                        &directionAccuracy)) || (LE_OUT_OF_RANGE == result)), "Get direction");
    LE_TEST_END_SKIP();

    LE_TEST_BEGIN_SKIP(LINUX_OS, 1);
    LE_TEST_OK((LE_OK == (result = le_gnss_GetDirection(positionSampleRef, &direction, NULL)) ||
               (LE_OUT_OF_RANGE == result)), "Get direction");
    LE_TEST_END_SKIP();

    if (LE_OK == result)
    {
        LE_TEST_INFO("direction %u - Accuracy %u", direction/10, directionAccuracy/10);
    }
    else
    {
        LE_TEST_INFO("direction unknown [%u,%u]", direction, directionAccuracy);
    }

    // Get the magnetic deviation
    result = le_gnss_GetMagneticDeviation( positionSampleRef, &magneticDeviation);
    LE_TEST_OK((LE_OK == result) || (LE_OUT_OF_RANGE == result), "Get magnetic deviation");
    if (LE_OK == result)
    {
        LE_TEST_INFO("magnetic deviation %d", magneticDeviation/10);
    }
    else
    {
        LE_TEST_INFO("magnetic deviation unknown [%d]",magneticDeviation);
    }

    /* Satellites status */
    LE_TEST_BEGIN_SKIP(!LINUX_OS, 2);
    result =  le_gnss_GetSatellitesStatus(positionSampleRef,
                                          &satsInViewCount,
                                          &satsTrackingCount,
                                          &satsUsedCount);

    LE_TEST_OK((LE_OK == result) || (LE_OUT_OF_RANGE == result),
            "Get satellite status");

    LE_TEST_INFO("satsInView %d - satsTracking %d - satsUsed %d",
            satsInViewCount,
            satsTrackingCount,
            satsUsedCount);

    /* Satellites information */
    result =  le_gnss_GetSatellitesInfo(positionSampleRef,
                                        satIdPtr,
                                        &satIdNumElements,
                                        satConstPtr,
                                        &satConstNumElements,
                                        satUsedPtr,
                                        &satUsedNumElements,
                                        satSnrPtr,
                                        &satSnrNumElements,
                                        satAzimPtr,
                                        &satAzimNumElements,
                                        satElevPtr,
                                        &satElevNumElements);

    LE_TEST_OK((LE_OK == result) || (LE_OUT_OF_RANGE == result),
            "Get satellite info");
    LE_TEST_END_SKIP();

    // Satellite Vehicle information
    for (i=0; i<satIdNumElements; i++)
    {
        if ((0 != satIdPtr[i]) && (UINT16_MAX != satIdPtr[i]))
        {
            LE_TEST_INFO("[%02d] SVid %03d - C%01d - U%d - SNR%02d - Azim%03d - Elev%02d",
                    i,
                    satIdPtr[i],
                    satConstPtr[i],
                    satUsedPtr[i],
                    satSnrPtr[i],
                    satAzimPtr[i],
                    satElevPtr[i]);

            if (LE_GNSS_SV_CONSTELLATION_SBAS == satConstPtr[i])
            {
                LE_TEST_INFO("SBAS category : %d", le_gnss_GetSbasConstellationCategory(satIdPtr[i]));
            }
        }
    }

    // Release provided Position sample reference
    le_gnss_ReleaseSampleRef(positionSampleRef);

    le_sem_Post(PositionHandlerSem);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Add Position Handler
 *
*/
//--------------------------------------------------------------------------------------------------
static void* PositionThread
(
    void* context
)
{

    le_gnss_ConnectService();

    LE_INFO("======== Position Handler thread  ========");
    PositionHandlerRef = le_gnss_AddPositionHandler(PositionHandlerFunction, NULL);
    LE_TEST_OK((PositionHandlerRef != NULL),
            "Confirm position handler was added successfully");

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: GNSS position handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestLeGnssPositionHandler
(
    void
)
{
    le_result_t result;
    le_thread_Ref_t positionThreadRef;
    uint32_t ttff = 0;
    uint8_t  minElevation;
    le_clk_Time_t time;
    le_gnss_NmeaBitMask_t mask = LE_GNSS_NMEA_MASK_GPGGA | LE_GNSS_NMEA_MASK_GPGLL |
                                 LE_GNSS_NMEA_MASK_GPRMC | LE_GNSS_NMEA_MASK_GPGNS |
                                 LE_GNSS_NMEA_MASK_GPVTG | LE_GNSS_NMEA_MASK_GPZDA |
                                 LE_GNSS_NMEA_MASK_GPGST | LE_GNSS_NMEA_MASK_GPGSA |
                                 LE_GNSS_NMEA_MASK_GPGSV;

    LE_TEST_INFO("Start Test Testle_gnss_PositionHandlerTest");

    // All NMEA sentences must be enabled to get full position data on alt1250
    LE_TEST_BEGIN_SKIP(LINUX_OS, 1);
    LE_TEST_OK(LE_OK == le_gnss_SetNmeaSentences(mask), "Enable all supported NMEA sentences");
    LE_TEST_END_SKIP();

    // NMEA frame GPGSA is checked that no SV with elevation below 10
    // degrees are given.
    LE_TEST_BEGIN_SKIP(!LINUX_OS, 1);
    minElevation = 10;
    result = le_gnss_SetMinElevation(minElevation);
    LE_TEST_OK((LE_OK == result) || (LE_OUT_OF_RANGE == result), "Set min elevation to 10");
    if (LE_OK == result)
    {
        LE_INFO("Set minElevation %d",minElevation);
    }
    LE_TEST_END_SKIP();

    // Test le_gnss_SetDataResolution() before starting GNSS
    LE_TEST_INFO("Sanity test for le_gnss_SetDataResolution");
    LE_TEST_OK(LE_BAD_PARAMETER == le_gnss_SetDataResolution(LE_GNSS_DATA_UNKNOWN,
                                                            LE_GNSS_RES_ONE_DECIMAL),
            "Set invalid data resolution");
    LE_TEST_INFO("Start GNSS");
    LE_TEST_ASSERT(LE_OK == le_gnss_Start(), "Start GNSS");
    LE_TEST_INFO("Wait 5 seconds");
    sleep(5);

    // Test le_gnss_SetDataResolution() after starting GNSS
    LE_TEST_OK(LE_BAD_PARAMETER == le_gnss_SetDataResolution(LE_GNSS_DATA_VACCURACY,
                                                            LE_GNSS_RES_UNKNOWN),
            "Set invalid data resolution for vAccuracy");

    // Add Position Handler Test
    positionThreadRef = le_thread_Create("PositionThread",PositionThread,NULL);
    le_thread_Start(positionThreadRef);

    // test Cold Restart boosted by le_gnss_InjectUtcTime
    // EpochTime and timeAccuracy should be valid and saved by now
    sleep(5);
    LE_TEST_BEGIN_SKIP(!LINUX_OS, 3);
    LE_TEST_OK(LE_OK == le_gnss_ForceColdRestart(), "Force cold restart");

    // Last accurate epochTime and timeAccuracy are used
    LE_TEST_OK(0 != EpochTime, "Confirm EpochTime is not 0");
    LE_TEST_INFO("TimeAccuracy %d EpochTime %llu",TimeAccuracy, (unsigned long long int)EpochTime);

    LE_TEST_OK(LE_OK == le_gnss_InjectUtcTime(EpochTime , TimeAccuracy), "Inject UTC time");
    LE_TEST_END_SKIP();

    // Get TTFF,position fix should be still in progress for the FACTORY start
    result = le_gnss_GetTtff(&ttff);
    LE_TEST_OK(LE_BUSY == result, "Confirm TTFF is unavailable");

    // Wait for a 3D fix
    LE_TEST_INFO("Wait 60 seconds for a 3D fix");
    time.sec = WAIT_MAX_FOR_3DFIX;
    time.usec = 0;
    LE_TEST_OK(LE_OK == le_sem_WaitWithTimeOut(PositionHandlerSem, time),
            "Wait until position handler has executed successfully");

    // Get TTFF
    result = le_gnss_GetTtff(&ttff);
    LE_TEST_OK((LE_OK == result) || (LE_BUSY == result), "Get TTFF");
    if(result == LE_OK)
    {
        LE_TEST_INFO("TTFF cold restart = %d msec", ttff);
    }
    else
    {
        LE_TEST_INFO("TTFF cold restart not available");
    }

    le_gnss_RemovePositionHandler(PositionHandlerRef);
    LE_TEST_INFO("Wait 5 seconds");
    sleep(5);

    // stop thread
#ifdef LE_CONFIG_LINUX
    le_thread_Cancel(positionThreadRef);
#endif

    EpochTime=0;
    TimeAccuracy=0;
}

//! [GnssPosition]

//! [GnssControl]
//--------------------------------------------------------------------------------------------------
/**
 * Test: GNSS Position request.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestLeGnssStart
(
    void
)
{
    uint32_t rate = 0;
    le_gnss_ConstellationBitMask_t constellationMask;
    le_gnss_NmeaBitMask_t nmeaMask;
    uint32_t ttff = 0;
    le_result_t result = LE_FAULT;


    LE_TEST_INFO("Start Test Testle_gnss_StartTest");

    LE_TEST_BEGIN_SKIP(!LINUX_OS, 2);
    LE_TEST_OK(le_gnss_GetAcquisitionRate(&rate) == LE_OK, "Get acquisition rate");
    LE_TEST_INFO("Acquisition rate %d ms", rate);
    LE_TEST_OK(le_gnss_SetAcquisitionRate(rate) == LE_OK, "Set acquisition rate");
    LE_TEST_END_SKIP();

    LE_TEST_OK(le_gnss_GetConstellation(&constellationMask) == LE_OK, "Get constellation");
    LE_TEST_INFO("Constellation 0x%X", constellationMask);
    LE_TEST_OK(le_gnss_SetConstellation(constellationMask) == LE_OK, "Set constellation");

    LE_TEST_OK((le_gnss_GetNmeaSentences(&nmeaMask)) == LE_OK, "Get NMEA sentences");
    LE_TEST_INFO("Enabled NMEA sentences 0x%08X", nmeaMask);
    LE_TEST_OK((le_gnss_SetNmeaSentences(nmeaMask)) == LE_OK, "Set NMEA sentences");

    LE_TEST_INFO("Start GNSS");
    LE_TEST_ASSERT((le_gnss_Start()) == LE_OK, "Start GNSS");

    /* Wait for a position fix */
    LE_TEST_INFO("Wait 120 seconds for a 3D fix");
    sleep(120);

    // Get TTFF
    result = le_gnss_GetTtff(&ttff);
    LE_TEST_OK((result == LE_OK) || (result == LE_BUSY), "Get TTFF");
    if(result == LE_OK)
    {
        LE_TEST_INFO("TTFF start = %d msec", ttff);
    }
    else
    {
        LE_TEST_INFO("TTFF start not available");
    }

    LE_TEST_INFO("Stop GNSS");
    LE_TEST_OK((le_gnss_Stop()) == LE_OK, "Stop GNSS");

    // Test le_gnss_StartMode()
    // HOT start
    LE_TEST_INFO("Ask for a Hot start in 3 seconds...");
    sleep(3);
    LE_TEST_OK(le_gnss_StartMode(LE_GNSS_UNKNOWN_START) == LE_BAD_PARAMETER, "Hot start");

    LE_TEST_ASSERT((le_gnss_StartMode(LE_GNSS_HOT_START)) == LE_OK, "le_gnss_StartMode(Hot start)");

    // Wait 5 sec
    sleep(5);
    // Stop GNSS engine
    LE_TEST_ASSERT((le_gnss_Stop()) == LE_OK, "Stop GNSS");

    /* WARM start */
    LE_TEST_INFO("Ask for a Warm start in 3 seconds...");
    sleep(3);
    LE_TEST_ASSERT((le_gnss_StartMode(LE_GNSS_WARM_START)) == LE_OK,
                   "le_gnss_StartMode(Warm start)");

    // Wait 5 sec
    sleep(5);
    // Stop GNSS engine
    LE_TEST_ASSERT((le_gnss_Stop()) == LE_OK, "Stop GNSS");

    /* COLD Restart */
    LE_TEST_INFO("Ask for a Cold start in 3 seconds...");
    sleep(3);
    LE_TEST_ASSERT((le_gnss_StartMode(LE_GNSS_COLD_START)) == LE_OK,
                   "le_gnss_StartMode(Cold start)");

    // Wait 5 sec
    sleep(5);
    // Stop GNSS engine
    LE_TEST_ASSERT((le_gnss_Stop()) == LE_OK, "Stop GNSS");

    // FACTORY start
    LE_TEST_INFO("Ask for a Factory start in 3 seconds...");
    sleep(3);
    LE_TEST_ASSERT((le_gnss_StartMode(LE_GNSS_FACTORY_START)) == LE_OK,
                   "le_gnss_StartMode(Factory start)");

    // Wait 5 sec
    sleep(5);
    // Stop GNSS engine
    LE_TEST_ASSERT((le_gnss_Stop()) == LE_OK, "Stop GNSS");

    LE_TEST_BEGIN_SKIP(LINUX_OS, 6);
    LE_TEST_OK(LE_OK == le_gnss_EnableExternalLna(), "Enable external LNA");
    LE_TEST_ASSERT(LE_OK == le_gnss_Start(), "Start GNSS");
    LE_TEST_INFO("GNSS running, confirm EXT_GPS_LNA_EN signal is high");
    LE_TEST_INFO("Wait 30 seconds");
    sleep(30);

    LE_TEST_OK(LE_NOT_PERMITTED == le_gnss_DisableExternalLna(),
               "Try to disable LNA when GNSS active");
    LE_TEST_OK(LE_NOT_PERMITTED == le_gnss_EnableExternalLna(),
               "Try to enable LNA when GNSS active");

    LE_TEST_ASSERT(LE_OK == le_gnss_Stop(), "Start GNSS");
    LE_TEST_OK(LE_OK == le_gnss_DisableExternalLna(), "Disable external LNA");
    LE_TEST_END_SKIP();

    EpochTime=0;
    TimeAccuracy=0;
}
//! [GnssControl]

//! [GnssReStart]
//--------------------------------------------------------------------------------------------------
/**
 * Test: Restart to Cold start.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestLeGnssRestart
(
    void
)
{
    uint32_t ttff = 0;
    uint64_t epochTime;
    le_gnss_SampleRef_t positionSampleRef;
    le_result_t result = LE_FAULT;

    LE_TEST_INFO("Start Test le_pos_RestartTest");

    LE_TEST_ASSERT((le_gnss_Start()) == LE_OK, "Start GNSS");

    /* Wait for a position fix */
    LE_TEST_INFO("Wait 60 seconds for a 3D fix");
    sleep(60);
    // Get TTFF
    result = le_gnss_GetTtff(&ttff);
    LE_TEST_OK((result == LE_OK) || (result == LE_BUSY), "Get TTFF");
    if(result == LE_OK)
    {
        LE_TEST_INFO("TTFF start = %d msec", ttff);
    }
    else
    {
        LE_TEST_INFO("TTFF start not available");
    }

    /* HOT Restart */
    LE_TEST_INFO("Ask for a Hot restart in 3 seconds...");
    sleep(3);
    LE_TEST_OK(le_gnss_ForceHotRestart() == LE_OK, "Force hot restart");
    // Wait for a 3D fix
    LE_TEST_INFO("Wait 60 seconds for a 3D fix");
    sleep(60);
    // Get TTFF
    result = le_gnss_GetTtff(&ttff);
    LE_TEST_OK((result == LE_OK) || (result == LE_BUSY), "Get TTFF");
    if(result == LE_OK)
    {
        LE_TEST_INFO("TTFF Hot restart = %d msec", ttff);
    }
    else
    {
        LE_TEST_INFO("TTFF Hot restart not available");
    }

    /* WARM Restart */
    LE_TEST_INFO("Ask for a Warm restart in 3 seconds...");
    sleep(3);
    LE_TEST_OK(le_gnss_ForceWarmRestart() == LE_OK, "Force warm restart");
    // Wait for a 3D fix
    LE_TEST_INFO("Wait 60 seconds for a 3D fix");
    sleep(60);
    // Get TTFF
    result = le_gnss_GetTtff(&ttff);
    LE_TEST_OK((result == LE_OK) || (result == LE_BUSY), "Get TTFF");
    if(result == LE_OK)
    {
        LE_TEST_INFO("TTFF Warm restart = %d msec", ttff);
    }
    else
    {
        LE_TEST_INFO("TTFF Warm restart not available");
    }

    /* COLD Restart */
    LE_TEST_INFO("Ask for a Cold restart in 3 seconds...");
    sleep(3);
    LE_TEST_OK(le_gnss_ForceColdRestart() == LE_OK, "Force cold restart");

    sleep(5);
    // Get Epoch time : it should be 0 after a COLD restart
    positionSampleRef = le_gnss_GetLastSampleRef();
    LE_TEST_OK((LE_OUT_OF_RANGE == le_gnss_GetEpochTime(positionSampleRef, &epochTime)),
            "Get epoch time after cold restart");
    LE_TEST_OK(0 == epochTime, "Confirm epoch time is invalid");

    // Wait for a 3D fix
    LE_TEST_INFO("Wait 60 seconds for a 3D fix");
    sleep(60);
    // Get TTFF
    result = le_gnss_GetTtff(&ttff);
    LE_TEST_OK((result == LE_OK) || (result == LE_BUSY), "Get TTFF");
    if(result == LE_OK)
    {
        LE_TEST_INFO("TTFF Cold restart = %d msec", ttff);
    }
    else
    {
        LE_TEST_INFO("TTFF Cold restart not available");
    }

    /* FACTORY Restart */
    LE_TEST_INFO("Ask for a Factory restart in 3 seconds...");
    sleep(3);
    LE_TEST_OK(le_gnss_ForceFactoryRestart() == LE_OK, "Force factory restart");
    // Get TTFF,position fix should be still in progress for the FACTORY start
    result = le_gnss_GetTtff(&ttff);
    LE_TEST_OK(result == LE_BUSY, "Get TTFF after factory restart");
    LE_TEST_INFO("TTFF is checked as not available immediatly after a FACTORY start");

    sleep(5);
    // Get Epoch time : it should be 0 after a FACTORY restart
    positionSampleRef = le_gnss_GetLastSampleRef();
    LE_TEST_OK((LE_OUT_OF_RANGE == le_gnss_GetEpochTime(positionSampleRef, &epochTime)),
            "Get epoch time after factory restart");
    LE_TEST_OK(0 == epochTime, "Confirm epoch time is invalid");

    // Wait for a 3D fix
    LE_TEST_INFO("Wait 60 seconds for a 3D fix");
    sleep(60);
    // Get TTFF
    result = le_gnss_GetTtff(&ttff);
    LE_TEST_OK((result == LE_OK) || (result == LE_BUSY), "Get TTFF");
    if(result == LE_OK)
    {
        LE_TEST_INFO("TTFF Factory restart = %d msec", ttff);
    }
    else
    {
        LE_TEST_INFO("TTFF Factory restart not available");
    }

    /* Stop GNSS engine*/
    sleep(1);
    LE_TEST_ASSERT((le_gnss_Stop()) == LE_OK, "Stop GNSS");
    EpochTime=0;
    TimeAccuracy=0;
}
//! [GnssReStart]

//--------------------------------------------------------------------------------------------------
/**
 * Test: loop to get the time to first fix
 */
//--------------------------------------------------------------------------------------------------
static void LoopToGet3Dfix
(
    uint32_t *ttffPtr
)
{
    int32_t  cnt=0;
    le_result_t result = LE_BUSY;

    while ((result == LE_BUSY) && (cnt < WAIT_MAX_FOR_3DFIX))
    {
        // Get TTFF
        result = le_gnss_GetTtff(ttffPtr);
        if(result == LE_OK)
        {
            LE_TEST_INFO("TTFF start = %d msec", *ttffPtr);
        }
        else
        {
            cnt++;
            LE_TEST_INFO("TTFF not calculated (Position not fixed) BUSY");
            sleep(1);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: get TTFF
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestLeGnssTtffMeasurement
(
    void
)
{
    uint32_t ttff = 0;
    uint32_t ttffSave = 0;

    LE_TEST_INFO("Start Test Testle_gnss_ttffTest");

    LE_TEST_INFO("Start GNSS");
    LE_TEST_ASSERT((le_gnss_Start()) == LE_OK, "Start GNSS");

    LE_TEST_INFO("loop to Wait for a 3D fix");
    LoopToGet3Dfix(&ttff);
    ttffSave = ttff;

    /* HOT Restart */
    LE_TEST_INFO("Ask for a Hot restart in 3 seconds...");
    sleep(3);
    LE_TEST_OK(le_gnss_ForceHotRestart() == LE_OK, "Force hot restart");

    LE_TEST_INFO("loop to Wait for a 3D fix");
    LoopToGet3Dfix(&ttff);

    LE_TEST_INFO("Wait 5 seconds");
    sleep(5);

    LE_TEST_INFO("Stop GNSS");
    LE_TEST_ASSERT((le_gnss_Stop()) == LE_OK, "Stop GNSS");
    EpochTime=0;
    TimeAccuracy=0;

    LE_TEST_INFO("TTFF start = %d msec", ttffSave);
    LE_TEST_INFO("TTFF Hot restart = %d msec", ttff);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: test Setting/Getting constellation mask
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestLeGnssConstellations
(
    void
)
{
    le_gnss_ConstellationBitMask_t constellationMask;
    le_gnss_ConstellationArea_t constellationArea;

    LE_TEST_INFO("Start Test TestLeGnssConstellationsTest");

    // error test
    constellationMask = 0;
    LE_TEST_OK((LE_UNSUPPORTED == le_gnss_SetConstellation(constellationMask)),
            "Set unsupported constellation %d", constellationMask);
    constellationMask = LE_GNSS_CONSTELLATION_SBAS;
    LE_TEST_OK((LE_UNSUPPORTED == le_gnss_SetConstellation(constellationMask)),
            "Set unsupported constellation %d", constellationMask);

    // GPS+SBAS
    constellationMask = LE_GNSS_CONSTELLATION_GPS | LE_GNSS_CONSTELLATION_SBAS;
    LE_TEST_OK(LE_UNSUPPORTED == le_gnss_SetConstellation(constellationMask),
            "Set unsupported constellation %d", constellationMask);

    // GPS+Glonass selection
    constellationMask = LE_GNSS_CONSTELLATION_GPS | LE_GNSS_CONSTELLATION_GLONASS;
    LE_TEST_OK(LE_OK == le_gnss_SetConstellation(constellationMask),
            "Set constellation %d", constellationMask);
    LE_TEST_OK(LE_OK == le_gnss_GetConstellation(&constellationMask),
            "Get constellation");

    LE_TEST_OK((LE_GNSS_CONSTELLATION_GPS | LE_GNSS_CONSTELLATION_GLONASS) == constellationMask,
            "Confirm constellation is set to %d", constellationMask);

    constellationMask = LE_GNSS_CONSTELLATION_BEIDOU;
    LE_TEST_OK((LE_UNSUPPORTED == le_gnss_SetConstellation(constellationMask)),
            "Set unsupported constellation %d", constellationMask);

    LE_TEST_OK(le_gnss_GetConstellation(&constellationMask) == LE_OK, "Get constellation");
    // test constellationMask has not changed after previous error
    LE_TEST_OK((LE_GNSS_CONSTELLATION_GPS | LE_GNSS_CONSTELLATION_GLONASS) == constellationMask,
            "Confirm constellation is unchanged after error");

    LE_TEST_BEGIN_SKIP(!LINUX_OS, 4)
    LE_TEST_OK(LE_UNSUPPORTED == (le_gnss_SetConstellationArea(LE_GNSS_SV_CONSTELLATION_GPS,
                                                              LE_GNSS_OUTSIDE_US_AREA)),
            "Set unsupported GPS constellation area");

    LE_TEST_OK(LE_UNSUPPORTED == (le_gnss_SetConstellationArea(LE_GNSS_SV_CONSTELLATION_GLONASS,
                                                              LE_GNSS_OUTSIDE_US_AREA)),
            "Set unsupported GLONASS constellation area");

    LE_TEST_OK(le_gnss_GetConstellationArea(LE_GNSS_SV_CONSTELLATION_GLONASS,
                                              &constellationArea),
            "Get GLONASS constellation area");
    LE_TEST_OK(LE_GNSS_WORLDWIDE_AREA == constellationArea,
            "Confirm GLONASS constellation area is worldwide");
    LE_TEST_END_SKIP();

    // next tests have same results as test4 for mdm9x15
    LE_TEST_BEGIN_SKIP(!MDM9X40_PLATFORM && !MDM9X28_PLATFORM, 13)
    // Gps selection (LE_GNSS_CONSTELLATION_SBAS and LE_GNSS_CONSTELLATION_QZSS are present
    // in the constellationMask)
    constellationMask = LE_GNSS_CONSTELLATION_GPS |
                        LE_GNSS_CONSTELLATION_SBAS |
                        LE_GNSS_CONSTELLATION_QZSS;
    LE_TEST_OK(LE_UNSUPPORTED == le_gnss_SetConstellation(constellationMask),
            "Set unsupported constellation %d", constellationMask);

    // Gps+Glonass+Beidou selection
    constellationMask = LE_GNSS_CONSTELLATION_GPS |
                        LE_GNSS_CONSTELLATION_GLONASS |
                        LE_GNSS_CONSTELLATION_BEIDOU;

    LE_TEST_OK(LE_OK == le_gnss_SetConstellation(constellationMask),
            "Set constellation %d", constellationMask);
    LE_TEST_OK(LE_OK == le_gnss_GetConstellation(&constellationMask),
            "Get constellation");
    LE_TEST_OK((LE_GNSS_CONSTELLATION_GPS |
               LE_GNSS_CONSTELLATION_GLONASS |
               LE_GNSS_CONSTELLATION_BEIDOU) == constellationMask,
            "Confirm constellation mask is set to %d", LE_GNSS_CONSTELLATION_GPS |
                                                       LE_GNSS_CONSTELLATION_GLONASS |
                                                       LE_GNSS_CONSTELLATION_BEIDOU);

    // Gps+Glonass+Beidou+Galileo+Qzss selection
    constellationMask = LE_GNSS_CONSTELLATION_GPS |
                        LE_GNSS_CONSTELLATION_GLONASS |
                        LE_GNSS_CONSTELLATION_BEIDOU |
                        LE_GNSS_CONSTELLATION_GALILEO |
                        LE_GNSS_CONSTELLATION_QZSS;

    LE_TEST_OK(LE_OK == le_gnss_SetConstellation(constellationMask),
            "Set constellation %d", constellationMask);
    LE_TEST_OK(LE_OK == le_gnss_GetConstellation(&constellationMask), "Get constellation");
    LE_TEST_OK((LE_GNSS_CONSTELLATION_GPS |
               LE_GNSS_CONSTELLATION_GLONASS |
               LE_GNSS_CONSTELLATION_BEIDOU |
               LE_GNSS_CONSTELLATION_GALILEO |
               LE_GNSS_CONSTELLATION_QZSS) == constellationMask,
            "Confirm constellation mask is set to %d", LE_GNSS_CONSTELLATION_GPS |
                                                       LE_GNSS_CONSTELLATION_GLONASS |
                                                       LE_GNSS_CONSTELLATION_BEIDOU |
                                                       LE_GNSS_CONSTELLATION_GALILEO |
                                                       LE_GNSS_CONSTELLATION_QZSS);

    // add unknown constellation
    constellationMask |= UNKNOWN_CONSTELLATION;

    // test constellationMask has not changed after previous error
    LE_TEST_OK(LE_OK == le_gnss_SetConstellation(constellationMask),
            "Set unknown constellation %d", constellationMask);
    LE_TEST_OK(LE_OK == le_gnss_GetConstellation(&constellationMask), "Get constellation");
    LE_TEST_OK((LE_GNSS_CONSTELLATION_GPS |
               LE_GNSS_CONSTELLATION_GLONASS |
               LE_GNSS_CONSTELLATION_BEIDOU |
               LE_GNSS_CONSTELLATION_GALILEO |
               LE_GNSS_CONSTELLATION_QZSS) == constellationMask,
            "Confirm constellation mask is set to %d", LE_GNSS_CONSTELLATION_GPS |
                                                       LE_GNSS_CONSTELLATION_GLONASS |
                                                       LE_GNSS_CONSTELLATION_BEIDOU |
                                                       LE_GNSS_CONSTELLATION_GALILEO |
                                                       LE_GNSS_CONSTELLATION_QZSS);

    LE_TEST_OK(LE_OK == le_gnss_SetConstellationArea(LE_GNSS_SV_CONSTELLATION_BEIDOU,
                                            LE_GNSS_WORLDWIDE_AREA),
            "Set Beidou constellation area worldwide");
    LE_TEST_OK(LE_OK == le_gnss_GetConstellationArea(LE_GNSS_SV_CONSTELLATION_BEIDOU,
                                            &constellationArea),
            "Get constellation area for Beidou");
    LE_TEST_OK(LE_GNSS_WORLDWIDE_AREA == constellationArea,
            "Confirm Beidou constellation area is set to %d", LE_GNSS_WORLDWIDE_AREA);
    LE_TEST_END_SKIP();
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: test Setting/Getting enabled NMEA sentences mask
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestLeGnssNmeaSentences
(
    void
)
{
    int i = 0;
    le_gnss_NmeaBitMask_t nmeaMask;
    le_gnss_NmeaBitMask_t saveNmeaMask;

    LE_TEST_INFO("Start Test TestLeGnssNmeaSentences");

    // Test 1: bit mask too big, error
    nmeaMask = (LE_GNSS_NMEA_SENTENCES_MAX << 1) | 1;
    LE_TEST_OK(LE_BAD_PARAMETER == le_gnss_SetNmeaSentences(nmeaMask),
            "Set invalid NMEA mask %d", nmeaMask);

    // Test 2: test all bits from the bit mask
    le_gnss_NmeaBitMask_t nmeaSentencesList[] = {
        LE_GNSS_NMEA_MASK_GPGGA,
        LE_GNSS_NMEA_MASK_GPGSA,
        LE_GNSS_NMEA_MASK_GPGSV,
        LE_GNSS_NMEA_MASK_GPRMC,
        LE_GNSS_NMEA_MASK_GPVTG,
        LE_GNSS_NMEA_MASK_GPGLL,
#ifdef LE_CONFIG_LINUX
        LE_GNSS_NMEA_MASK_GLGSV,
        LE_GNSS_NMEA_MASK_GNGNS,
        LE_GNSS_NMEA_MASK_GNGSA,
        LE_GNSS_NMEA_MASK_GAGGA,
        LE_GNSS_NMEA_MASK_GAGSA,
        LE_GNSS_NMEA_MASK_GAGSV,
        LE_GNSS_NMEA_MASK_GARMC,
        LE_GNSS_NMEA_MASK_GAVTG,
#else
        LE_GNSS_NMEA_MASK_GPGNS,
        LE_GNSS_NMEA_MASK_GPZDA,
        LE_GNSS_NMEA_MASK_GPGST,
#endif

        0
    };

    for (i = 0; nmeaSentencesList[i]; i++)
    {
        LE_TEST_OK(LE_OK == le_gnss_SetNmeaSentences(nmeaSentencesList[i]),
                "Set NMEA sentence mask to 0x%08X",nmeaSentencesList[i]);
        LE_TEST_OK(LE_OK == le_gnss_GetNmeaSentences(&nmeaMask), "Get NMEA sentences");
        LE_TEST_OK(nmeaMask == nmeaSentencesList[i],
                "Confirm NMEA sentence mask is set to 0x%08X", nmeaSentencesList[i]);
    }

    // @deprecated, PQXFI is deprecated. PTYPE is used instead.
    LE_TEST_BEGIN_SKIP(!LINUX_OS, 21);
    LE_TEST_OK(LE_OK == le_gnss_SetNmeaSentences(LE_GNSS_NMEA_MASK_PQXFI),
            "Set NMEA sentence mask to %08X", LE_GNSS_NMEA_MASK_PQXFI);
    LE_TEST_OK(LE_OK == le_gnss_GetNmeaSentences(&nmeaMask), "Get NMEA sentences");
    LE_TEST_OK(nmeaMask == (LE_GNSS_NMEA_MASK_PQXFI | LE_GNSS_NMEA_MASK_PTYPE),
            "Confirm NMEA sentence mask is set to %08X",
            (LE_GNSS_NMEA_MASK_PQXFI | LE_GNSS_NMEA_MASK_PTYPE));

    LE_TEST_OK(LE_OK == le_gnss_SetNmeaSentences(LE_GNSS_NMEA_MASK_PTYPE),
            "Set NMEA sentence mask to %08X",LE_GNSS_NMEA_MASK_PTYPE);
    LE_TEST_OK(le_gnss_GetNmeaSentences(&nmeaMask), "Get NMEA sentences");
    LE_TEST_OK(nmeaMask == (LE_GNSS_NMEA_MASK_PQXFI | LE_GNSS_NMEA_MASK_PTYPE),
            "Confirm NMEA sentence mask is set to %08X",
            (LE_GNSS_NMEA_MASK_PQXFI | LE_GNSS_NMEA_MASK_PTYPE));

    LE_TEST_OK(LE_OK == le_gnss_SetNmeaSentences(LE_GNSS_NMEA_MASK_PSTIS),
            "Set NMEA sentence mask to %08X", LE_GNSS_NMEA_MASK_PSTIS);
    LE_TEST_OK(LE_OK == le_gnss_GetNmeaSentences(&nmeaMask), "Get NMEA sentences");
    LE_TEST_OK((LE_GNSS_NMEA_MASK_PSTIS == nmeaMask) || (0 == nmeaMask),
            "Confirm NMEA sentence mask is set to %08X or 0", LE_GNSS_NMEA_MASK_PSTIS);

    LE_TEST_OK(LE_OK == le_gnss_SetNmeaSentences(LE_GNSS_NMEA_MASK_GPGRS),
            "set NMEA sentence mask to %08X", LE_GNSS_NMEA_MASK_GPGRS);
    LE_TEST_OK(LE_OK == le_gnss_GetNmeaSentences(&nmeaMask),
            "Get NMEA sentences");
    LE_TEST_OK((LE_GNSS_NMEA_MASK_GPGRS == nmeaMask) || (0 == nmeaMask),
            "Confirm NMEA sentence mas is set to %08X or 0", LE_GNSS_NMEA_MASK_GPGRS);

    LE_TEST_OK(LE_OK == le_gnss_SetNmeaSentences(LE_GNSS_NMEA_MASK_DEBUG),
            "Set NMEA sentence mask to %08X", LE_GNSS_NMEA_MASK_DEBUG);
    LE_TEST_OK(LE_OK == le_gnss_GetNmeaSentences(&nmeaMask), "Get NMEA sentences");
    LE_TEST_OK((LE_GNSS_NMEA_MASK_DEBUG == nmeaMask) || (0 == nmeaMask),
            "Confirm NMEA sentence mask is set to %08X or 0", LE_GNSS_NMEA_MASK_DEBUG);

    LE_TEST_OK(LE_OK == le_gnss_SetNmeaSentences(LE_GNSS_NMEA_MASK_GPDTM),
            "Set NMEA sentence mask to %08X", LE_GNSS_NMEA_MASK_GPDTM);
    LE_TEST_OK(LE_OK == le_gnss_GetNmeaSentences(&nmeaMask),
            "Get NMEA sentences");
    LE_TEST_OK((LE_GNSS_NMEA_MASK_GPDTM == nmeaMask) || (0 == nmeaMask),
            "Confirm NMEA sentence mask is set to %08X or 0", LE_GNSS_NMEA_MASK_GPDTM);

    LE_TEST_OK(LE_OK == le_gnss_SetNmeaSentences(LE_GNSS_NMEA_MASK_GAGNS),
            "Set NMEA sentences to %08X", LE_GNSS_NMEA_MASK_GAGNS);
    LE_TEST_OK(LE_OK == le_gnss_GetNmeaSentences(&nmeaMask),
            "Get NMEA sentences");
    LE_TEST_OK((LE_GNSS_NMEA_MASK_GAGNS == nmeaMask) || (0 == nmeaMask),
            "Confirm NMEA sentence mask is set to %08X or 0", LE_GNSS_NMEA_MASK_GAGNS);
    LE_TEST_END_SKIP();

    LE_TEST_OK(LE_OK == le_gnss_SetNmeaSentences(LE_GNSS_NMEA_MASK_GPGLL),
            "Set NMEA sentence mask to %08X", LE_GNSS_NMEA_MASK_GPGLL);
    LE_TEST_OK(LE_OK == le_gnss_GetNmeaSentences(&nmeaMask),
            "Get NMEA sentences");
    LE_TEST_OK((LE_GNSS_NMEA_MASK_GPGLL == nmeaMask) || (0 == nmeaMask),
            "Confirm NMEA sentence mask is set to %08X or 0", LE_GNSS_NMEA_MASK_GPGLL);

    // Test 3: test bit mask combinations
    saveNmeaMask = LE_GNSS_NMEA_MASK_GPGGA | LE_GNSS_NMEA_MASK_GPGSA | LE_GNSS_NMEA_MASK_GPGSV;

    LE_TEST_OK(LE_OK == le_gnss_SetNmeaSentences(saveNmeaMask),
            "Set NMEA sentence mask to %08X", saveNmeaMask);
    LE_TEST_OK(LE_OK == le_gnss_GetNmeaSentences(&nmeaMask),
            "Get NMEA sentences");
    LE_TEST_OK(nmeaMask == saveNmeaMask,
            "Confirm NMEA sentence mask is set to %08X", saveNmeaMask);

    LE_TEST_OK(LE_OK == le_gnss_SetNmeaSentences(saveNmeaMask | LE_GNSS_NMEA_MASK_GPRMC),
            "Set NMEA sentence mask to %08X", (saveNmeaMask | LE_GNSS_NMEA_MASK_GPRMC));
    LE_TEST_OK(LE_OK == le_gnss_GetNmeaSentences(&nmeaMask), "Get NMEA sentences");
    LE_TEST_OK(((saveNmeaMask | LE_GNSS_NMEA_MASK_GPRMC) == nmeaMask) || (saveNmeaMask == nmeaMask),
            "Confirm NMEA mask is set correctly");

    LE_TEST_OK(LE_OK == le_gnss_SetNmeaSentences(saveNmeaMask | LE_GNSS_NMEA_MASK_GPGLL),
            "Set NMEA sentence mask to %08X", (saveNmeaMask | LE_GNSS_NMEA_MASK_GPGLL));
    LE_TEST_OK(LE_OK == le_gnss_GetNmeaSentences(&nmeaMask), "Get NMEA sentences");
    LE_TEST_OK(((saveNmeaMask | LE_GNSS_NMEA_MASK_GPGLL) == nmeaMask) || (saveNmeaMask == nmeaMask),
            "Confirm NMEA mask is set correctly");

    LE_TEST_BEGIN_SKIP(!LINUX_OS, 9);
    LE_TEST_OK(LE_OK == le_gnss_SetNmeaSentences(saveNmeaMask | LE_GNSS_NMEA_MASK_DEBUG),
            "Set NMEA sentence mask to %08X", (saveNmeaMask | LE_GNSS_NMEA_MASK_DEBUG));
    LE_TEST_OK(LE_OK == le_gnss_GetNmeaSentences(&nmeaMask), "Get NMEA sentences");
    LE_TEST_OK(((saveNmeaMask | LE_GNSS_NMEA_MASK_DEBUG) == nmeaMask) ||
            (saveNmeaMask == nmeaMask), "Confirm NMEA sentence mask is set correctly");

    LE_TEST_OK(LE_OK == le_gnss_SetNmeaSentences(saveNmeaMask | LE_GNSS_NMEA_MASK_GPDTM),
            "Set NMEA sentence mask to %08X", (saveNmeaMask | LE_GNSS_NMEA_MASK_GPDTM));
    LE_TEST_OK(LE_OK == le_gnss_GetNmeaSentences(&nmeaMask), "Get NMEA sentences");
    LE_TEST_OK(((saveNmeaMask | LE_GNSS_NMEA_MASK_GPDTM) == nmeaMask) || (saveNmeaMask == nmeaMask),
            "Confirm NMEA sentence mask is set correctly");

    LE_TEST_OK(LE_OK == le_gnss_SetNmeaSentences(saveNmeaMask | LE_GNSS_NMEA_MASK_GAGNS),
            "Set NMEA sentence mask to %08X", (saveNmeaMask | LE_GNSS_NMEA_MASK_GAGNS));
    LE_TEST_OK(LE_OK == le_gnss_GetNmeaSentences(&nmeaMask), "Get NMEA sentence mask");
    LE_TEST_OK(((saveNmeaMask | LE_GNSS_NMEA_MASK_GAGNS) == nmeaMask) || (saveNmeaMask == nmeaMask),
            "Confirm NMEA sentence mask is set correctly");
    LE_TEST_END_SKIP();

    LE_TEST_INFO("Test TestLeGnssNmeaSentences OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: test SUPL certificate
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestSuplCertificate
(
    void
)
{
    le_gnss_AssistedMode_t gnssMode;

    memset(&ShortSuplCertificate, 0x69, sizeof(ShortSuplCertificate));

    LE_TEST_BEGIN_SKIP(!LINUX_OS, 21);
    //Gets the SUPL Assisted-GNSS LE_GNSS_STANDALONE_MODE mode.
    LE_TEST_OK(LE_OK == (le_gnss_GetSuplAssistedMode(&gnssMode)), "Get Supl assisted mode");
    LE_TEST_INFO("Supl Assisted Mode obtained: %d",gnssMode);

    //Set the SUPL Assisted-GNSS mode.
    LE_TEST_OK(LE_OK == (le_gnss_SetSuplAssistedMode(LE_GNSS_STANDALONE_MODE)),
            "Set supl mode to standalone");

    //Gets the SUPL Assisted-GNSS mode.
    LE_TEST_OK(LE_OK == (le_gnss_GetSuplAssistedMode(&gnssMode)),
            "Get supl assisted mode");
    LE_TEST_OK(LE_GNSS_STANDALONE_MODE == gnssMode,
            "Confirm supl mode is standalone");

    //Set the SUPL Assisted-GNSSLE_GNSS_MS_BASED_MODE mode.
    LE_TEST_OK(LE_OK == (le_gnss_SetSuplAssistedMode(LE_GNSS_MS_BASED_MODE)),
            "Set supl mode to MS based");

    //Gets the SUPL Assisted-GNSS mode.
    LE_TEST_OK(LE_OK == (le_gnss_GetSuplAssistedMode(&gnssMode)),
            "Get supl assisted mode");
    LE_TEST_OK(LE_GNSS_MS_BASED_MODE == gnssMode,
            "Confirm supl mode is set to MS based");

    //Set the SUPL Assisted-GNSS mode LE_GNSS_MS_ASSISTED_MODE.
    LE_TEST_OK(LE_OK == (le_gnss_SetSuplAssistedMode(LE_GNSS_MS_ASSISTED_MODE)),
            "Set supl mode to MS assisted");

    //Gets the SUPL Assisted-GNSS mode.
    LE_TEST_OK(LE_OK == (le_gnss_GetSuplAssistedMode(&gnssMode)),
            "Get supl assisted mode");
    LE_TEST_OK(LE_GNSS_MS_ASSISTED_MODE == gnssMode,
            "Confirm supl mode is set to MS assisted");

    //Set the SUPL Assisted-GNSS mode LE_GNSS_MS_ASSISTED_MODE.
    LE_TEST_OK((le_gnss_SetSuplAssistedMode(LE_GNSS_MS_ASSISTED_MODE+10)) == LE_UNSUPPORTED,
            "Set invalid supl mode");

    //Gets the SUPL Assisted-GNSS mode.
    LE_TEST_OK(LE_OK == (le_gnss_GetSuplAssistedMode(&gnssMode)),
            "Get supl assisted mode");
    LE_TEST_INFO("Supl Assisted Mode obtained: %d",gnssMode);
    LE_TEST_OK(LE_GNSS_MS_ASSISTED_MODE == gnssMode,
            "Confirm supl mode is unchanged after previous error");

    //Set the SUPL server URL
    LE_TEST_OK(LE_OK == (le_gnss_SetSuplServerUrl("http://sls1.sirf")),
            "Set supl server URL");

    //Set the SUPL server URL
    LE_TEST_OK(LE_OK == (le_gnss_SetSuplServerUrl("http://sls1.sirf.com")),
            "Set supl server URL");

    //Injects the SUPL certificate with lenght zero :
    LE_TEST_OK(LE_BAD_PARAMETER == (le_gnss_InjectSuplCertificate(0,0,ShortSuplCertificate)),
            "Inject 0 length supl certificate");
    //Injects the SUPL certificate with ID error
    LE_TEST_OK(LE_BAD_PARAMETER == (le_gnss_InjectSuplCertificate(10,
                               strlen(ShortSuplCertificate),ShortSuplCertificate)),
            "Inject supl certificate with invalid ID");

    //Injects the SUPL certificate to be used in A-GNSS sessions
    LE_TEST_OK(LE_OK == (le_gnss_InjectSuplCertificate(0,
                               strlen(ShortSuplCertificate),ShortSuplCertificate)),
            "Inject valid supl certificate");

    // cannot test certificate with lenght greater than LE_GNSS_SUPL_CERTIFICATE_MAX_BYTES
    // there is no return code in this case.
    //Delete the SUPL certificate 10 (out of range)
    LE_TEST_OK(LE_BAD_PARAMETER == (le_gnss_DeleteSuplCertificate(10)),
            "Delete out of range supl certificate");

    //Delete a SUPL certificate not used in A-GNSS sessions
    LE_TEST_OK(LE_FAULT == (le_gnss_DeleteSuplCertificate(1)),
            "Delete unused supl certificate");

    //Delete the SUPL certificate used in A-GNSS sessions
    LE_TEST_OK(LE_OK == (le_gnss_DeleteSuplCertificate(0)),
            "Delete the valid supl certificate");
    LE_TEST_END_SKIP();

}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Get leap seconds
 */
//--------------------------------------------------------------------------------------------------
static void TestLeGnssLeapSeconds
(
    void
)
{
    int32_t currentLeapSec = 0, nextLeapSec = 0;
    uint64_t gpsTimeMs = 0, nextEventMs = 0;

    LE_TEST_BEGIN_SKIP(!LINUX_OS, 1);
    LE_TEST_OK(LE_OK == le_gnss_GetLeapSeconds(&gpsTimeMs, &currentLeapSec, &nextEventMs, &nextLeapSec),
            "Get leap seconds");
    LE_TEST_END_SKIP();

    LE_TEST_INFO("Current GPS time %"PRIu64"ms, leap seconds %"PRIi32"ms", gpsTimeMs, currentLeapSec);
    LE_TEST_INFO("Next event in %"PRIu64"ms, next leap seconds %"PRIi32"ms", nextEventMs, nextLeapSec);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: GNSS capabilities APIs
 */
//--------------------------------------------------------------------------------------------------
static void TestLeGnssCapabilities
(
    void
)
{
    le_gnss_ConstellationBitMask_t supportedConstellations;
    le_gnss_NmeaBitMask_t supportedNmeaSentences;
    uint32_t maxNmeaRate;
    uint32_t minNmeaRate;

    LE_TEST_BEGIN_SKIP(LINUX_OS, 5);
    LE_TEST_OK(LE_OK == le_gnss_GetSupportedNmeaSentences(&supportedNmeaSentences),
            "Get supported NMEA sentences");
    LE_TEST_INFO("Supported NMEA sentence mask:0x%08X", supportedNmeaSentences);

    LE_TEST_OK(LE_OK == le_gnss_GetSupportedConstellations(&supportedConstellations),
            "Get supported constellations");
    LE_TEST_INFO("Supported constellation mask:0x%08X", supportedConstellations);

    LE_TEST_OK(LE_OK == le_gnss_GetMinNmeaRate(&minNmeaRate),
            "Get minimum NMEA rate");
    LE_TEST_INFO("Minimum NMEA rate:%"PRIu32, minNmeaRate);

    LE_TEST_OK(LE_OK == le_gnss_GetMaxNmeaRate(&maxNmeaRate),
            "Get maximum NMEA rate");
    LE_TEST_INFO("Maximum NMEA rate:%"PRIu32, maxNmeaRate);
    LE_TEST_END_SKIP();
}

//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    PositionHandlerSem = le_sem_Create("PosHandlerSem", 0);

    LE_TEST_PLAN(322);

    LE_TEST_INFO("======== GNSS device Test  ========");
    TestLeGnssDevice();
    LE_TEST_INFO("======== GNSS device Start Test  ========");
    TestLeGnssStart();
    LE_TEST_INFO("======== GNSS device Restart Test  ========");
    TestLeGnssRestart();
    LE_TEST_INFO("======== GNSS position handler Test  ========");
    TestLeGnssPositionHandler();
    LE_TEST_INFO("======== GNSS TTFF Test  ========");
    TestLeGnssTtffMeasurement();
    LE_TEST_INFO("======== GNSS Constellation Test  ========");
    TestLeGnssConstellations();
    LE_TEST_INFO("======== GNSS NMEA sentences Test  ========");
    TestLeGnssNmeaSentences();
    LE_TEST_INFO("======== GNSS leap seconds Test  ========");
    TestLeGnssLeapSeconds();
    LE_TEST_INFO("======== Supl Certificate Test  ========");
    TestSuplCertificate();
    LE_TEST_INFO("======== GNSS capabilities API test  ========");
    TestLeGnssCapabilities();
    LE_TEST_INFO("======== GNSS Test SUCCESS ========");
    LE_TEST_EXIT;
}
