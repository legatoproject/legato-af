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

static le_gnss_PositionHandlerRef_t PositionHandlerRef = NULL;

//Wait up to 60 seconds for a 3D fix
#define WAIT_MAX_FOR_3DFIX  60

// Unknown constallation bitmask
#define UNKNOWN_CONSTELLATION  0x80

char ShortSuplCertificate[50]={0};

// The epoch time is the number of seconds elapsed since January 1, 1970
// not counting leaps seconds.
static uint64_t EpochTime=0;
// Time uncertainty in Milliseconds
static uint32_t TimeAccuracy=0;

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

    le_gnss_ConstellationBitMask_t constellationMask;
    le_gnss_NmeaBitMask_t nmeaMask = 0;

    LE_INFO("Start Test Testle_gnss_DeviceTest");
    // GNSS device enabled by default
    LE_ASSERT((le_gnss_GetState()) == LE_GNSS_STATE_READY);
    LE_ASSERT((le_gnss_Enable()) == LE_DUPLICATE);
    // Disable GNSS device (DISABLED state)
    LE_ASSERT((le_gnss_Disable()) == LE_OK);
    LE_ASSERT((le_gnss_Disable()) == LE_DUPLICATE);
    // Check Disabled state
    LE_ASSERT((le_gnss_GetState()) == LE_GNSS_STATE_DISABLED);
    LE_ASSERT((le_gnss_Start()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_ForceHotRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_ForceWarmRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_ForceColdRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_ForceFactoryRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_GetTtff(&ttffValue)) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_Stop()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_SetConstellation(LE_GNSS_CONSTELLATION_GPS)) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_GetConstellation(&constellationMask)) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_GetAcquisitionRate(&acqRate)) == LE_NOT_PERMITTED);
    result = le_gnss_SetAcquisitionRate(acqRate);
    LE_ASSERT((result == LE_NOT_PERMITTED)||(result == LE_OUT_OF_RANGE));
    LE_ASSERT((le_gnss_SetNmeaSentences(nmeaMask)) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_GetNmeaSentences(&nmeaMask)) == LE_NOT_PERMITTED);

    // test le_gnss_Get/SetMinElevation when GNSS device is disabled and the engine is not started.
    minElevation = 40;
    LE_ASSERT((le_gnss_SetMinElevation(minElevation)) == LE_OK);
    LE_ASSERT((le_gnss_GetMinElevation(&minElevation)) == LE_OK);
    LE_INFO("GNSS min elevation obtained: %d",minElevation);
    LE_ASSERT(minElevation == 40);

    // Enable GNSS device (READY state)
    LE_ASSERT((le_gnss_Enable()) == LE_OK);
    LE_ASSERT((le_gnss_GetState()) == LE_GNSS_STATE_READY);
    LE_ASSERT((le_gnss_Disable()) == LE_OK);
    LE_ASSERT((le_gnss_GetState()) == LE_GNSS_STATE_DISABLED);
    LE_ASSERT((le_gnss_Enable()) == LE_OK);
    LE_ASSERT((le_gnss_GetState()) == LE_GNSS_STATE_READY);
    LE_ASSERT((le_gnss_SetConstellation(LE_GNSS_CONSTELLATION_GPS)) == LE_OK);
    LE_ASSERT((le_gnss_GetConstellation(&constellationMask)) == LE_OK);
    LE_ASSERT(constellationMask == LE_GNSS_CONSTELLATION_GPS);
    LE_ASSERT((le_gnss_Stop()) == LE_DUPLICATE);
    LE_ASSERT((le_gnss_ForceHotRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_ForceWarmRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_ForceColdRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_ForceFactoryRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_GetAcquisitionRate(&acqRate)) == LE_OK);
    acqRate = 0;
    LE_ASSERT((le_gnss_SetAcquisitionRate(acqRate)) == LE_OUT_OF_RANGE);
    acqRate = 1100;
    LE_ASSERT((le_gnss_SetAcquisitionRate(acqRate)) == LE_OK);
    LE_ASSERT((le_gnss_GetNmeaSentences(&nmeaMask)) == LE_OK);
    LE_ASSERT((le_gnss_SetNmeaSentences(nmeaMask)) == LE_OK);

    // test le_gnss_Get/SetMinElevation when GNSS device is enabled and the engine is not started.
    minElevation = 0;
    LE_ASSERT((le_gnss_SetMinElevation(minElevation)) == LE_OK);
    LE_ASSERT((le_gnss_GetMinElevation(&minElevation)) == LE_OK);
    LE_INFO("GNSS min elevation obtained: %d",minElevation);
    LE_ASSERT(minElevation == 0);

    // Start GNSS device (ACTIVE state)
    LE_ASSERT((le_gnss_Start()) == LE_OK);
    LE_ASSERT((le_gnss_GetState()) == LE_GNSS_STATE_ACTIVE);
    LE_ASSERT((le_gnss_Start()) == LE_DUPLICATE);
    LE_ASSERT((le_gnss_Enable()) == LE_DUPLICATE);
    LE_ASSERT((le_gnss_Disable()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_SetConstellation(LE_GNSS_CONSTELLATION_GPS)) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_GetConstellation(&constellationMask)) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_GetAcquisitionRate(&acqRate)) == LE_NOT_PERMITTED);
    result = le_gnss_SetAcquisitionRate(acqRate);
    LE_ASSERT((result == LE_NOT_PERMITTED)||(result == LE_OUT_OF_RANGE));
    LE_ASSERT((le_gnss_SetNmeaSentences(nmeaMask)) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_GetNmeaSentences(&nmeaMask)) == LE_NOT_PERMITTED);

    // test le_gnss_Get/SetMinElevation when le_gnss_ENABLE ON and le_gnss_Start ON
    minElevation = LE_GNSS_MIN_ELEVATION_MAX_DEGREE;
    LE_ASSERT((le_gnss_SetMinElevation(minElevation)) == LE_OK);
    LE_ASSERT((le_gnss_GetMinElevation(&minElevation)) == LE_OK);
    LE_INFO("GNSS min elevation obtained: %d",minElevation);
    LE_ASSERT(minElevation == LE_GNSS_MIN_ELEVATION_MAX_DEGREE);

    // test le_gnss_SetMinElevation wrong value (when le_gnss_ENABLE ON and le_gnss_Start ON)
    minElevation = LE_GNSS_MIN_ELEVATION_MAX_DEGREE+1;
    LE_ASSERT((le_gnss_SetMinElevation(minElevation)) == LE_OUT_OF_RANGE);

    // Stop GNSS device (READY state)
    LE_ASSERT((le_gnss_Stop()) == LE_OK);
    LE_ASSERT((le_gnss_GetState()) == LE_GNSS_STATE_READY);
    LE_ASSERT((le_gnss_Enable()) == LE_DUPLICATE);
    LE_ASSERT((le_gnss_Disable()) == LE_OK);
    LE_ASSERT((le_gnss_GetState()) == LE_GNSS_STATE_DISABLED);
    LE_ASSERT((le_gnss_Enable()) == LE_OK);
    LE_ASSERT((le_gnss_GetState()) == LE_GNSS_STATE_READY);
    LE_ASSERT((le_gnss_SetConstellation(LE_GNSS_CONSTELLATION_GPS)) == LE_OK);
    LE_ASSERT((le_gnss_GetConstellation(&constellationMask)) == LE_OK);
    LE_ASSERT(constellationMask == LE_GNSS_CONSTELLATION_GPS);
    LE_ASSERT((le_gnss_Stop()) == LE_DUPLICATE);
    LE_ASSERT((le_gnss_ForceHotRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_ForceWarmRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_ForceColdRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_ForceFactoryRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_GetAcquisitionRate(&acqRate)) == LE_OK);
    LE_ASSERT((le_gnss_SetAcquisitionRate(acqRate)) == LE_OK);
    LE_ASSERT((le_gnss_GetNmeaSentences(&nmeaMask)) == LE_OK);
    LE_ASSERT((le_gnss_SetNmeaSentences(nmeaMask)) == LE_OK);
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
    uint8_t leapSeconds;
    // Position state
    le_gnss_FixState_t state;
    // Location
    int32_t     latitude;
    int32_t     longitude;
    int32_t     altitude;
    int32_t     altitudeOnWgs84;
    int32_t     hAccuracy;
    int32_t     vAccuracy;
    int32_t     magneticDeviation;
    // DOP parameter
    uint16_t hdop;
    uint16_t vdop;
    uint16_t pdop;
    // Horizontal speed
    uint32_t hSpeed;
    uint32_t hSpeedAccuracy;
    // Vertical speed
    int32_t vSpeed;
    int32_t vSpeedAccuracy;
    // Direction
    uint32_t direction;
    uint32_t directionAccuracy;

    if(positionSampleRef == NULL)
    {
        LE_ERROR("New Position sample is NULL!");
    }
    else
    {
        LE_DEBUG("New Position sample %p", positionSampleRef);
    }

    // Get UTC date
    result = le_gnss_GetDate(positionSampleRef
                            , &year
                            , &month
                            , &day);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));
    // Get UTC time
    result = le_gnss_GetTime(positionSampleRef
                            , &hours
                            , &minutes
                            , &seconds
                            , &milliseconds);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));

    // Get Epoch time
    LE_ASSERT(le_gnss_GetEpochTime(positionSampleRef, &EpochTime) == LE_OK);

    // Display time/date format 13:45:30 2009-06-15
    LE_INFO("%02d:%02d:%02d %d-%02d-%02d,"
            , hours, minutes, seconds
            , year, month, day);

    // Display Epoch time
    LE_INFO("epoch time: %llu:", (unsigned long long int) EpochTime);

    // Get GPS time
    result = le_gnss_GetGpsTime(positionSampleRef
                            , &gpsWeek
                            , &gpsTimeOfWeek);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));

    LE_INFO("GPS time W %02d:ToW %dms"
            , gpsWeek
            , gpsTimeOfWeek);

    // Get time accuracy
    result = le_gnss_GetTimeAccuracy(positionSampleRef, &TimeAccuracy);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));

    LE_INFO("GPS time acc %d", TimeAccuracy);

    // Get UTC leap seconds in advance
    result = le_gnss_GetGpsLeapSeconds(positionSampleRef, &leapSeconds);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));

    LE_INFO("UTC leap seconds in advance %d", leapSeconds);

    // Get position state
    result = le_gnss_GetPositionState( positionSampleRef, &state);
    LE_ASSERT((result == LE_OK));
    LE_DEBUG("Position state: %s", (state == LE_GNSS_STATE_FIX_NO_POS)?"No Fix"
                                 :(state == LE_GNSS_STATE_FIX_2D)?"2D Fix"
                                 :(state == LE_GNSS_STATE_FIX_3D)?"3D Fix"
                                 : "Unknown");

    // Get Location
    result = le_gnss_GetLocation( positionSampleRef
                                , &latitude
                                , &longitude
                                , &hAccuracy);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));
    if(result == LE_OK)
    {
         LE_INFO("Position lat.%d, long.%d, hAccuracy.%d"
            , latitude, longitude, hAccuracy/100);
    }
    else
    {
        LE_INFO("Position unknown [%d,%d,%d]"
            , latitude, longitude, hAccuracy);
    }

    // Get altitude
    result = le_gnss_GetAltitude( positionSampleRef
                                , &altitude
                                , &vAccuracy);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));

    if(result == LE_OK)
    {
         LE_INFO("Altitude.%d, vAccuracy.%d"
                , altitude/1000, vAccuracy/10);
    }
    else
    {
         LE_INFO("Altitude unknown [%d,%d]"
                , altitude, vAccuracy);
    }

    // Get altitude in meters, between WGS-84 earth ellipsoid
    // and mean sea level [resolution 1e-3]
    result = le_gnss_GetAltitudeOnWgs84(positionSampleRef, &altitudeOnWgs84);
    LE_ASSERT((LE_OK == result)||(LE_OUT_OF_RANGE == result));

    if (LE_OK == result)
    {
         LE_INFO("AltitudeOnWgs84.%d", altitudeOnWgs84/1000);
    }
    else
    {
         LE_INFO("AltitudeOnWgs84 unknown [%d]", altitudeOnWgs84);
    }

    // Get DOP parameter
    result = le_gnss_GetDop( positionSampleRef
                            , &hdop
                            , &vdop
                            , &pdop);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));
    LE_INFO("DOP [H%.3f,V%.3f,P%.3f]"
            , (float)(hdop)/1000, (float)(vdop)/1000, (float)(pdop)/1000);

    // Get horizontal speed
    result = le_gnss_GetHorizontalSpeed( positionSampleRef
                            , &hSpeed
                            , &hSpeedAccuracy);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));

    if(result == LE_OK)
    {
        LE_INFO("hSpeed %u - Accuracy %u"
                , hSpeed/100, hSpeedAccuracy/10);
    }
    else
    {
        LE_INFO("hSpeed unknown [%u,%u]"
                , hSpeed, hSpeedAccuracy);
    }

    // Get vertical speed
    result = le_gnss_GetVerticalSpeed( positionSampleRef
                            , &vSpeed
                            , &vSpeedAccuracy);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));
    if(result == LE_OK)
    {
        LE_INFO("vSpeed %d - Accuracy %d"
                , vSpeed/100, vSpeedAccuracy/10);
    }
    else
    {
        LE_INFO("vSpeed unknown [%d,%d]"
                , vSpeed, vSpeedAccuracy);
    }

    // Get direction
    result = le_gnss_GetDirection( positionSampleRef
                            , &direction
                            , &directionAccuracy);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));
    if(result == LE_OK)
    {
        LE_INFO("direction %u - Accuracy %u"
                , direction/10, directionAccuracy/10);
    }
    else
    {
        LE_INFO("direction unknown [%u,%u]"
                , direction, directionAccuracy);
    }

    // Get the magnetic deviation
    result = le_gnss_GetMagneticDeviation( positionSampleRef
                                         , &magneticDeviation);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));

    if(result == LE_OK)
    {
        LE_INFO("magnetic deviation %d", magneticDeviation/10);
    }
    else
    {
        LE_INFO("magnetic deviation unknown [%d]",magneticDeviation);
    }


    /* Satellites status */
    uint8_t satsInViewCount;
    uint8_t satsTrackingCount;
    uint8_t satsUsedCount;
    result =  le_gnss_GetSatellitesStatus(positionSampleRef
                                            , &satsInViewCount
                                            , &satsTrackingCount
                                            , &satsUsedCount);

    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));

    LE_INFO("satsInView %d - satsTracking %d - satsUsed %d"
            , satsInViewCount, satsTrackingCount, satsUsedCount);

    /* Satellites information */
    uint16_t satIdPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satIdNumElements = sizeof(satIdPtr);
    le_gnss_Constellation_t satConstPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satConstNumElements = sizeof(satConstPtr);
    bool satUsedPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satUsedNumElements = sizeof(satUsedPtr);
    uint8_t satSnrPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satSnrNumElements = sizeof(satSnrPtr);
    uint16_t satAzimPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satAzimNumElements = sizeof(satAzimPtr);
    uint8_t satElevPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satElevNumElements = sizeof(satElevPtr);
    int i;

    result =  le_gnss_GetSatellitesInfo(positionSampleRef
                                            , satIdPtr
                                            , &satIdNumElements
                                            , satConstPtr
                                            , &satConstNumElements
                                            , satUsedPtr
                                            , &satUsedNumElements
                                            , satSnrPtr
                                            , &satSnrNumElements
                                            , satAzimPtr
                                            , &satAzimNumElements
                                            , satElevPtr
                                            , &satElevNumElements);

    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));

    // Satellite Vehicle information
    for(i=0; i<satIdNumElements; i++)
    {
        if((satIdPtr[i] != 0)&&(satIdPtr[i] != UINT16_MAX))
        {
            LE_INFO("[%02d] SVid %03d - C%01d - U%d - SNR%02d - Azim%03d - Elev%02d"
                    , i
                    , satIdPtr[i]
                    , satConstPtr[i]
                    , satUsedPtr[i]
                    , satSnrPtr[i]
                    , satAzimPtr[i]
                    , satElevPtr[i]);

            if (LE_GNSS_SV_CONSTELLATION_SBAS == satConstPtr[i])
            {
                LE_INFO("SBAS category : %d", le_gnss_GetSbasConstellationCategory(satIdPtr[i]));
            }
        }
    }

    // Release provided Position sample reference
    le_gnss_ReleaseSampleRef(positionSampleRef);
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
    LE_ASSERT((PositionHandlerRef != NULL));

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
    le_gnss_SampleRef_t positionSampleRef;
    uint64_t epochTime;
    uint32_t ttff = 0;
    uint8_t  minElevation;

    LE_INFO("Start Test Testle_gnss_PositionHandlerTest");

    // NMEA frame GPGSA is checked that no SV with elevation below 10
    // degrees are given.
    minElevation = 10;
    LE_ASSERT_OK(le_gnss_SetMinElevation(minElevation));
    LE_INFO("Set minElevation %d",minElevation);

    LE_INFO("Start GNSS");
    LE_ASSERT_OK(le_gnss_Start());
    LE_INFO("Wait 5 seconds");
    sleep(5);

    // Add Position Handler Test
    positionThreadRef = le_thread_Create("PositionThread",PositionThread,NULL);
    le_thread_Start(positionThreadRef);

    // test Cold Restart boosted by le_gnss_InjectUtcTime
    // EpochTime and timeAccuracy should be valid and saved by now
    sleep(2);

    LE_INFO("Ask for a Cold restart");
    LE_ASSERT_OK(le_gnss_ForceColdRestart());

    // Last accurate epochTime and timeAccuracy are used
    LE_ASSERT(0 != EpochTime);
    LE_INFO("TimeAccuracy %d",TimeAccuracy);
    LE_ASSERT_OK(le_gnss_InjectUtcTime(EpochTime , TimeAccuracy));

    // Get TTFF,position fix should be still in progress for the FACTORY start
    result = le_gnss_GetTtff(&ttff);
    LE_ASSERT(LE_BUSY == result);
    LE_INFO("TTFF is checked as not available immediatly after a Cold restart");

    // Wait for a 3D fix
    LE_INFO("Wait 60 seconds for a 3D fix");
    sleep(60);
    // Get TTFF
    result = le_gnss_GetTtff(&ttff);
    LE_ASSERT((LE_OK == result)||(LE_BUSY == result));
    if(result == LE_OK)
    {
        LE_INFO("TTFF cold restart = %d msec", ttff);
    }
    else
    {
        LE_INFO("TTFF cold restart not available");
    }

    le_gnss_RemovePositionHandler(PositionHandlerRef);

    LE_INFO("Wait 5 seconds");
    sleep(5);

    // stop thread
    le_thread_Cancel(positionThreadRef);

    // Get Epoch time, get last position sample
    positionSampleRef = le_gnss_GetLastSampleRef();
    LE_ASSERT_OK(le_gnss_GetEpochTime(positionSampleRef, &epochTime));

    // Display epoch time
    LE_INFO("epoch time: %llu:", (unsigned long long int) epochTime);

    LE_INFO("Stop GNSS");
    LE_ASSERT_OK(le_gnss_Stop());
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


    LE_INFO("Start Test Testle_gnss_StartTest");

    LE_ASSERT((le_gnss_GetAcquisitionRate(&rate)) == LE_OK);
    LE_INFO("Acquisition rate %d ms", rate);
    LE_ASSERT((le_gnss_SetAcquisitionRate(rate)) == LE_OK);

    LE_ASSERT(le_gnss_GetConstellation(&constellationMask) == LE_OK);
    LE_INFO("Constellation 0x%X", constellationMask);
    LE_ASSERT(le_gnss_SetConstellation(constellationMask) == LE_OK);

    LE_ASSERT((le_gnss_GetNmeaSentences(&nmeaMask)) == LE_OK);
    LE_INFO("Enabled NMEA sentences 0x%08X", nmeaMask);
    LE_ASSERT((le_gnss_SetNmeaSentences(nmeaMask)) == LE_OK);

    LE_INFO("Start GNSS");
    LE_ASSERT((le_gnss_Start()) == LE_OK);

    /* Wait for a position fix */
    LE_INFO("Wait 120 seconds for a 3D fix");
    sleep(120);

    // Get TTFF
    result = le_gnss_GetTtff(&ttff);
    LE_ASSERT((result == LE_OK)||(result == LE_BUSY));
    if(result == LE_OK)
    {
        LE_INFO("TTFF start = %d msec", ttff);
    }
    else
    {
        LE_INFO("TTFF start not available");
    }

    LE_INFO("Stop GNSS");
    LE_ASSERT((le_gnss_Stop()) == LE_OK);
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

    LE_INFO("Start Test le_pos_RestartTest");


    LE_ASSERT((le_gnss_Start()) == LE_OK);

    /* Wait for a position fix */
    LE_INFO("Wait 60 seconds for a 3D fix");
    sleep(60);
    // Get TTFF
    result = le_gnss_GetTtff(&ttff);
    LE_ASSERT((result == LE_OK)||(result == LE_BUSY));
    if(result == LE_OK)
    {
        LE_INFO("TTFF start = %d msec", ttff);
    }
    else
    {
        LE_INFO("TTFF start not available");
    }

    /* HOT Restart */
    LE_INFO("Ask for a Hot restart in 3 seconds...");
    sleep(3);
    LE_ASSERT(le_gnss_ForceHotRestart() == LE_OK);
    // Wait for a 3D fix
    LE_INFO("Wait 60 seconds for a 3D fix");
    sleep(60);
    // Get TTFF
    result = le_gnss_GetTtff(&ttff);
    LE_ASSERT((result == LE_OK)||(result == LE_BUSY));
    if(result == LE_OK)
    {
        LE_INFO("TTFF Hot restart = %d msec", ttff);
    }
    else
    {
        LE_INFO("TTFF Hot restart not available");
    }

    /* WARM Restart */
    LE_INFO("Ask for a Warm restart in 3 seconds...");
    sleep(3);
    LE_ASSERT(le_gnss_ForceWarmRestart() == LE_OK);
    // Wait for a 3D fix
    LE_INFO("Wait 60 seconds for a 3D fix");
    sleep(60);
    // Get TTFF
    result = le_gnss_GetTtff(&ttff);
    LE_ASSERT((result == LE_OK)||(result == LE_BUSY));
    if(result == LE_OK)
    {
        LE_INFO("TTFF Warm restart = %d msec", ttff);
    }
    else
    {
        LE_INFO("TTFF Warm restart not available");
    }

    /* COLD Restart */
    LE_INFO("Ask for a Cold restart in 3 seconds...");
    sleep(3);
    LE_ASSERT(le_gnss_ForceColdRestart() == LE_OK);

    sleep(1);
    // Get Epoch time : it should be 0 after a COLD restart
    positionSampleRef = le_gnss_GetLastSampleRef();
    LE_ASSERT((LE_OUT_OF_RANGE == le_gnss_GetEpochTime(positionSampleRef, &epochTime)));
    LE_ASSERT(0 == epochTime);

    // Wait for a 3D fix
    LE_INFO("Wait 60 seconds for a 3D fix");
    sleep(60);
    // Get TTFF
    result = le_gnss_GetTtff(&ttff);
    LE_ASSERT((result == LE_OK)||(result == LE_BUSY));
    if(result == LE_OK)
    {
        LE_INFO("TTFF Cold restart = %d msec", ttff);
    }
    else
    {
        LE_INFO("TTFF Cold restart not available");
    }

    /* FACTORY Restart */
    LE_INFO("Ask for a Factory restart in 3 seconds...");
    sleep(3);
    LE_ASSERT(le_gnss_ForceFactoryRestart() == LE_OK);
    // Get TTFF,position fix should be still in progress for the FACTORY start
    result = le_gnss_GetTtff(&ttff);
    LE_ASSERT(result == LE_BUSY);
    LE_INFO("TTFF is checked as not available immediatly after a FACTORY start");

    sleep(1);
    // Get Epoch time : it should be 0 after a FACTORY restart
    positionSampleRef = le_gnss_GetLastSampleRef();
    LE_ASSERT((LE_OUT_OF_RANGE == le_gnss_GetEpochTime(positionSampleRef, &epochTime)));
    LE_ASSERT(0 == epochTime);

    // Wait for a 3D fix
    LE_INFO("Wait 60 seconds for a 3D fix");
    sleep(60);
    // Get TTFF
    result = le_gnss_GetTtff(&ttff);
    LE_ASSERT((result == LE_OK)||(result == LE_BUSY));
    if(result == LE_OK)
    {
        LE_INFO("TTFF Factory restart = %d msec", ttff);
    }
    else
    {
        LE_INFO("TTFF Factory restart not available");
    }

    /* Stop GNSS engine*/
    sleep(1);
    LE_ASSERT((le_gnss_Stop()) == LE_OK);
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
        LE_ASSERT((result == LE_OK)||(result == LE_BUSY));
        if(result == LE_OK)
        {
            LE_INFO("TTFF start = %d msec", *ttffPtr);
        }
        else
        {
            cnt++;
            LE_INFO("TTFF not calculated (Position not fixed) BUSY");
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
    le_thread_Ref_t positionThreadRef;

    LE_INFO("Start Test Testle_gnss_ttffTest");

    LE_INFO("Start GNSS");
    LE_ASSERT((le_gnss_Start()) == LE_OK);

    // Add Position Handler Test
    positionThreadRef = le_thread_Create("PositionThread",PositionThread,NULL);
    le_thread_Start(positionThreadRef);

    LE_INFO("loop to Wait for a 3D fix");
    LoopToGet3Dfix(&ttff);
    ttffSave = ttff;

    /* HOT Restart */
    LE_INFO("Ask for a Hot restart in 3 seconds...");
    sleep(3);
    LE_ASSERT(le_gnss_ForceHotRestart() == LE_OK);

    LE_INFO("loop to Wait for a 3D fix");
    LoopToGet3Dfix(&ttff);

    le_gnss_RemovePositionHandler(PositionHandlerRef);
    LE_INFO("Wait 5 seconds");
    sleep(5);

    // stop thread
    le_thread_Cancel(positionThreadRef);

    LE_INFO("Stop GNSS");
    LE_ASSERT((le_gnss_Stop()) == LE_OK);
    EpochTime=0;
    TimeAccuracy=0;

    LE_INFO("TTFF start = %d msec", ttffSave);
    LE_INFO("TTFF Hot restart = %d msec", ttff);
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

    LE_INFO("Start Test TestLeGnssConstellationsTest");

    // error test
    constellationMask = 0;
    LE_ASSERT((LE_UNSUPPORTED == le_gnss_SetConstellation(constellationMask)));
    constellationMask = LE_GNSS_CONSTELLATION_SBAS;
    LE_ASSERT((LE_UNSUPPORTED == le_gnss_SetConstellation(constellationMask)));

    // Gps+Glonass selection
    constellationMask = LE_GNSS_CONSTELLATION_GPS | LE_GNSS_CONSTELLATION_GLONASS;
    LE_ASSERT(LE_OK == le_gnss_SetConstellation(constellationMask));
    LE_ASSERT(LE_OK == le_gnss_GetConstellation(&constellationMask));
    LE_ASSERT((LE_GNSS_CONSTELLATION_GPS | LE_GNSS_CONSTELLATION_GLONASS) == constellationMask);

    // GPS constellation is not set and Beidou is unknown for mdm9x15
    constellationMask = LE_GNSS_CONSTELLATION_BEIDOU;
    LE_ASSERT((LE_UNSUPPORTED == le_gnss_SetConstellation(constellationMask)));

    LE_ASSERT(le_gnss_GetConstellation(&constellationMask) == LE_OK);
    // test constellationMask has not changed after previous error
    LE_ASSERT((LE_GNSS_CONSTELLATION_GPS | LE_GNSS_CONSTELLATION_GLONASS) == constellationMask);

    // next tests have same results as test4 for mdm9x15
#if defined(SIERRA_MDM9X40) || defined(SIERRA_MDM9X28)

    // Gps selection (LE_GNSS_CONSTELLATION_SBAS and LE_GNSS_CONSTELLATION_QZSS are present
    // in the constellationMask)
    constellationMask = LE_GNSS_CONSTELLATION_GPS |
                        LE_GNSS_CONSTELLATION_SBAS |
                        LE_GNSS_CONSTELLATION_QZSS;

    LE_ASSERT(LE_OK == le_gnss_SetConstellation(constellationMask));
    LE_ASSERT(LE_OK == le_gnss_GetConstellation(&constellationMask));
    LE_ASSERT((LE_GNSS_CONSTELLATION_GPS | LE_GNSS_CONSTELLATION_QZSS) == constellationMask);


    // Gps+Glonass+Beidou selection
    constellationMask = LE_GNSS_CONSTELLATION_GPS |
                        LE_GNSS_CONSTELLATION_GLONASS |
                        LE_GNSS_CONSTELLATION_BEIDOU;

    LE_ASSERT(LE_OK == le_gnss_SetConstellation(constellationMask));
    LE_ASSERT(LE_OK == le_gnss_GetConstellation(&constellationMask));
    LE_ASSERT((LE_GNSS_CONSTELLATION_GPS |
               LE_GNSS_CONSTELLATION_GLONASS |
               LE_GNSS_CONSTELLATION_BEIDOU) == constellationMask);

    // Gps+Glonass+Beidou+Galileo+Qzss selection
    constellationMask = LE_GNSS_CONSTELLATION_GPS |
                        LE_GNSS_CONSTELLATION_GLONASS |
                        LE_GNSS_CONSTELLATION_BEIDOU |
                        LE_GNSS_CONSTELLATION_GALILEO |
                        LE_GNSS_CONSTELLATION_QZSS;

    LE_ASSERT(LE_OK == le_gnss_SetConstellation(constellationMask));
    LE_ASSERT(LE_OK == le_gnss_GetConstellation(&constellationMask));
    LE_ASSERT((LE_GNSS_CONSTELLATION_GPS |
               LE_GNSS_CONSTELLATION_GLONASS |
               LE_GNSS_CONSTELLATION_BEIDOU |
               LE_GNSS_CONSTELLATION_GALILEO |
               LE_GNSS_CONSTELLATION_QZSS) == constellationMask);

    // add unknown constellation
    constellationMask |= UNKNOWN_CONSTELLATION;

    // test constellationMask has not changed after previous error
    LE_ASSERT(LE_OK == le_gnss_SetConstellation(constellationMask));
    LE_ASSERT(LE_OK == le_gnss_GetConstellation(&constellationMask));
    LE_ASSERT((LE_GNSS_CONSTELLATION_GPS |
               LE_GNSS_CONSTELLATION_GLONASS |
               LE_GNSS_CONSTELLATION_BEIDOU |
               LE_GNSS_CONSTELLATION_GALILEO |
               LE_GNSS_CONSTELLATION_QZSS) == constellationMask);
#endif
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

    LE_INFO("Start Test TestLeGnssNmeaSentences");

    // Test 1: bit mask too big, error
    nmeaMask = (LE_GNSS_NMEA_SENTENCES_MAX << 1) | 1;
    LE_ASSERT(LE_BAD_PARAMETER == le_gnss_SetNmeaSentences(nmeaMask));

    // Test 2: test all bits from the bit mask
    le_gnss_NmeaBitMask_t nmeaSentencesList[] = {
        LE_GNSS_NMEA_MASK_GPGGA,
        LE_GNSS_NMEA_MASK_GPGSA,
        LE_GNSS_NMEA_MASK_GPGSV,
        LE_GNSS_NMEA_MASK_GPRMC,
        LE_GNSS_NMEA_MASK_GPVTG,
        LE_GNSS_NMEA_MASK_GLGSV,
        LE_GNSS_NMEA_MASK_GNGNS,
        LE_GNSS_NMEA_MASK_GNGSA,
        // ToDo: Uncomment when FW issue is fixed
//        LE_GNSS_NMEA_MASK_GAGGA,
//        LE_GNSS_NMEA_MASK_GAGSA,
//        LE_GNSS_NMEA_MASK_GAGSV,
//        LE_GNSS_NMEA_MASK_GARMC,
//        LE_GNSS_NMEA_MASK_GAVTG,
        LE_GNSS_NMEA_MASK_PSTIS,
        LE_GNSS_NMEA_MASK_PQXFI,
        0
    };

    for (i = 0; nmeaSentencesList[i]; i++)
    {
        LE_ASSERT_OK(le_gnss_SetNmeaSentences(nmeaSentencesList[i]));
        LE_ASSERT_OK(le_gnss_GetNmeaSentences(&nmeaMask));
        LE_ASSERT(nmeaMask == nmeaSentencesList[i]);
    }
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

    //Gets the SUPL Assisted-GNSS LE_GNSS_STANDALONE_MODE mode.
    LE_ASSERT(LE_OK == (le_gnss_GetSuplAssistedMode(&gnssMode)));
    LE_INFO("Supl Assisted Mode obtained: %d",gnssMode);

    //Set the SUPL Assisted-GNSS mode.
    LE_ASSERT(LE_OK == (le_gnss_SetSuplAssistedMode(LE_GNSS_STANDALONE_MODE)));
    LE_INFO("SUPL Stand alone mode set");

    //Gets the SUPL Assisted-GNSS mode.
    LE_ASSERT(LE_OK == (le_gnss_GetSuplAssistedMode(&gnssMode)));
    LE_INFO("Supl Assisted Mode obtained: %d",gnssMode);
    LE_ASSERT(LE_GNSS_STANDALONE_MODE == gnssMode);

    //Set the SUPL Assisted-GNSSLE_GNSS_MS_BASED_MODE mode.
    LE_ASSERT(LE_OK == (le_gnss_SetSuplAssistedMode(LE_GNSS_MS_BASED_MODE)));
    LE_INFO("SUPL Ms based mode set");

    //Gets the SUPL Assisted-GNSS mode.
    LE_ASSERT(LE_OK == (le_gnss_GetSuplAssistedMode(&gnssMode)));
    LE_INFO("Supl Assisted Mode obtained: %d",gnssMode);
    LE_ASSERT(LE_GNSS_MS_BASED_MODE == gnssMode);

    //Set the SUPL Assisted-GNSS mode LE_GNSS_MS_ASSISTED_MODE.
    LE_ASSERT(LE_OK == (le_gnss_SetSuplAssistedMode(LE_GNSS_MS_ASSISTED_MODE)));
    LE_INFO("SUPL Assisted mode set");

    //Gets the SUPL Assisted-GNSS mode.
    LE_ASSERT(LE_OK == (le_gnss_GetSuplAssistedMode(&gnssMode)));
    LE_INFO("Supl Assisted Mode obtained: %d",gnssMode);
    LE_ASSERT(LE_GNSS_MS_ASSISTED_MODE == gnssMode);

    //Set the SUPL Assisted-GNSS mode LE_GNSS_MS_ASSISTED_MODE.
    LE_ASSERT((le_gnss_SetSuplAssistedMode(LE_GNSS_MS_ASSISTED_MODE+10)) == LE_UNSUPPORTED);

    //Gets the SUPL Assisted-GNSS mode.
    LE_ASSERT(LE_OK == (le_gnss_GetSuplAssistedMode(&gnssMode)));
    LE_INFO("Supl Assisted Mode obtained: %d",gnssMode);
    LE_ASSERT(LE_GNSS_MS_ASSISTED_MODE == gnssMode);

    //Set the SUPL server URL
    LE_ASSERT(LE_OK == (le_gnss_SetSuplServerUrl("http://sls1.sirf")));

    //Set the SUPL server URL
    LE_ASSERT(LE_OK == (le_gnss_SetSuplServerUrl("http://sls1.sirf.com")));
    LE_INFO("le_gnss_SetSuplServerUrl OK");

    //Injects the SUPL certificate with lenght zero :
    LE_ASSERT(LE_BAD_PARAMETER == (le_gnss_InjectSuplCertificate(0,0,ShortSuplCertificate)));
    //Injects the SUPL certificate with ID error
    LE_ASSERT(LE_BAD_PARAMETER == (le_gnss_InjectSuplCertificate(10,
                               strlen(ShortSuplCertificate),ShortSuplCertificate)));

    //Injects the SUPL certificate to be used in A-GNSS sessions
    LE_ASSERT(LE_OK == (le_gnss_InjectSuplCertificate(0,
                               strlen(ShortSuplCertificate),ShortSuplCertificate)));

    // cannot test certificate with lenght greater than LE_GNSS_SUPL_CERTIFICATE_MAX_BYTES
    // there is no return code in this case.
    //Delete the SUPL certificate 10 (out of range)
    LE_ASSERT(LE_BAD_PARAMETER == (le_gnss_DeleteSuplCertificate(10)));

    //Delete a SUPL certificate not used in A-GNSS sessions
    LE_ASSERT(LE_FAULT == (le_gnss_DeleteSuplCertificate(1)));

    //Delete the SUPL certificate used in A-GNSS sessions
    LE_ASSERT(LE_OK == (le_gnss_DeleteSuplCertificate(0)));

}


//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
     LE_INFO("======== GNSS device Test  ========");
     TestLeGnssDevice();
     LE_INFO("======== GNSS device Start Test  ========");
     TestLeGnssStart();
     LE_INFO("======== GNSS device Restart Test  ========");
     TestLeGnssRestart();
     LE_INFO("======== GNSS position handler Test  ========");
     TestLeGnssPositionHandler();
     LE_INFO("======== GNSS TTFF Test  ========");
     TestLeGnssTtffMeasurement();
     LE_INFO("======== GNSS Constellation Test  ========");
     TestLeGnssConstellations();
     LE_INFO("======== GNSS NMEA sentences Test  ========");
     TestLeGnssNmeaSentences();
     LE_INFO("======== Supl Certificate Test  ========");
     TestSuplCertificate();
     LE_INFO("======== GNSS Test SUCCESS ========");
     exit(EXIT_SUCCESS);
}
