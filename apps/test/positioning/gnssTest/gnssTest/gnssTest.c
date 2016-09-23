 /**
  * This module implements a test for GNSS device starting.
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include "interfaces.h"

// Include macros for printing out values
#include "le_print.h"

static le_gnss_PositionHandlerRef_t PositionHandlerRef = NULL;

//Wait up to 60 seconds for a 3D fix
#define GNSS_WAIT_MAX_FOR_3DFIX  60

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
    le_gnss_ConstellationBitMask_t constellationMask;

    LE_INFO("Start Test Testle_gnss_DeviceTest");
    // GNSS device enabled by default
    LE_ASSERT((le_gnss_Enable()) == LE_DUPLICATE);
    // Disable GNSS device (DISABLED state)
    LE_ASSERT((le_gnss_Disable()) == LE_OK);
    LE_ASSERT((le_gnss_Disable()) == LE_DUPLICATE);
    // Check Disabled state
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
    LE_ASSERT((le_gnss_SetAcquisitionRate(acqRate)) == LE_NOT_PERMITTED);
    // Enable GNSS device (READY state)
    LE_ASSERT((le_gnss_Enable()) == LE_OK);
    LE_ASSERT((le_gnss_Disable()) == LE_OK);
    LE_ASSERT((le_gnss_Enable()) == LE_OK);
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
    // Start GNSS device (ACTIVE state)
    LE_ASSERT((le_gnss_Start()) == LE_OK);
    LE_ASSERT((le_gnss_Start()) == LE_DUPLICATE);
    LE_ASSERT((le_gnss_Enable()) == LE_DUPLICATE);
    LE_ASSERT((le_gnss_Disable()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_SetConstellation(LE_GNSS_CONSTELLATION_GPS)) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_GetConstellation(&constellationMask)) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_GetAcquisitionRate(&acqRate)) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_SetAcquisitionRate(acqRate)) == LE_NOT_PERMITTED);
    // Stop GNSS device (READY state)
    LE_ASSERT((le_gnss_Stop()) == LE_OK);
    LE_ASSERT((le_gnss_Enable()) == LE_DUPLICATE);
    LE_ASSERT((le_gnss_Disable()) == LE_OK);
    LE_ASSERT((le_gnss_Enable()) == LE_OK);
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
    // Time accuracy
    uint32_t timeAccuracy;
    // Position state
    le_gnss_FixState_t state;
    // Location
    int32_t     latitude;
    int32_t     longitude;
    int32_t     altitude;
    int32_t     hAccuracy;
    int32_t     vAccuracy;
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
    int32_t direction;
    int32_t directionAccuracy;

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

    // Display time/date format 13:45:30 2009-06-15
    LE_INFO("%02d:%02d:%02d %d-%02d-%02d,"
            , hours, minutes, seconds
            , year, month, day);

    // Get GPS time
    result = le_gnss_GetGpsTime(positionSampleRef
                            , &gpsWeek
                            , &gpsTimeOfWeek);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));

    LE_INFO("GPS time W %02d:ToW %dms"
            , gpsWeek
            , gpsTimeOfWeek);

    // Get time accuracy
    result = le_gnss_GetTimeAccuracy(positionSampleRef
                            , &timeAccuracy);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));

    LE_INFO("GPS time acc %d"
            , timeAccuracy);

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
            , latitude, longitude, hAccuracy/10);
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

    // Get DOP parameter
    result = le_gnss_GetDop( positionSampleRef
                            , &hdop
                            , &vdop
                            , &pdop);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));
    LE_INFO("DOP [H%.1f,V%.1f,P%.1f]"
            , (float)(hdop)/100, (float)(vdop)/100, (float)(pdop)/100);

    // Get horizontal speed
    result = le_gnss_GetHorizontalSpeed( positionSampleRef
                            , &hSpeed
                            , &hSpeedAccuracy);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));

    if(result == LE_OK)
    {
        LE_INFO("hSpeed %d - Accuracy %d"
                , hSpeed/100, hSpeedAccuracy/10);
    }
    else
    {
        LE_INFO("hSpeed unknown [%d,%d]"
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
        LE_INFO("direction %d - Accuracy %d"
                , direction/10, directionAccuracy/10);
    }
    else
    {
        LE_INFO("direction unknown [%d,%d]"
                , direction, directionAccuracy);
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
        }
    }

    // Get satellites latency
    int32_t latencyPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t latencyNumElements = sizeof(latencyPtr);
    result = le_gnss_GetSatellitesLatency(positionSampleRef
                                            , satIdPtr
                                            , &satIdNumElements
                                            , latencyPtr
                                            , &latencyNumElements);

    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));

    // Satellite Vehicle information
    for(i=0; i<latencyNumElements; i++)
    {
        if((satIdPtr[i] != 0)&&(satIdPtr[i] != UINT16_MAX))
        {
            LE_INFO("[%02d] SVid %03d - Latency %d"
                    , i
                    , satIdPtr[i]
                    , latencyPtr[i]);
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
    le_thread_Ref_t positionThreadRef;

    LE_INFO("Start Test Testle_gnss_PositionHandlerTest");

    LE_INFO("Start GNSS");
    LE_ASSERT((le_gnss_Start()) == LE_OK);
    LE_INFO("Wait 5 seconds");
    sleep(5);

    // Add Position Handler Test
    positionThreadRef = le_thread_Create("PositionThread",PositionThread,NULL);
    le_thread_Start(positionThreadRef);

    LE_INFO("Wait for a 3D fix");
    sleep(60);

    le_gnss_RemovePositionHandler(PositionHandlerRef);

    LE_INFO("Wait 5 seconds");
    sleep(5);

    // stop thread
    le_thread_Cancel(positionThreadRef);

    LE_INFO("Stop GNSS");
    LE_ASSERT((le_gnss_Stop()) == LE_OK);
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


    LE_INFO("Start Test Testle_gnss_StartTest");

    LE_ASSERT((le_gnss_GetAcquisitionRate(&rate)) == LE_OK);
    LE_INFO("Acquisition rate %d ms", rate);
    LE_ASSERT((le_gnss_SetAcquisitionRate(rate)) == LE_OK);

    LE_ASSERT(le_gnss_GetConstellation(&constellationMask) == LE_OK);
    LE_INFO("Constellation 0x%X", constellationMask);
    LE_ASSERT(le_gnss_SetConstellation(constellationMask) == LE_OK);

    LE_INFO("Start GNSS");
    LE_ASSERT((le_gnss_Start()) == LE_OK);

    /* Wait for a position fix */
    LE_INFO("Wait 120 seconds for a 3D fix");
    sleep(120);

    LE_INFO("Stop GNSS");
    LE_ASSERT((le_gnss_Stop()) == LE_OK);
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

    while ((result == LE_BUSY) && (cnt < GNSS_WAIT_MAX_FOR_3DFIX))
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

    // test1: error test
    constellationMask = 0;
    LE_ASSERT((le_gnss_SetConstellation(constellationMask)) == LE_UNSUPPORTED);
    sleep(2);

    // test2 : Gps selection
    constellationMask = LE_GNSS_CONSTELLATION_GPS;
    LE_ASSERT(le_gnss_SetConstellation(constellationMask) == LE_OK);
    LE_ASSERT(le_gnss_GetConstellation(&constellationMask) == LE_OK);
    LE_ASSERT(constellationMask == LE_GNSS_CONSTELLATION_GPS)
    sleep(2);

    // test3: Gps+Glonass selection
    constellationMask = LE_GNSS_CONSTELLATION_GPS | LE_GNSS_CONSTELLATION_GLONASS;
    LE_ASSERT(le_gnss_SetConstellation(constellationMask) == LE_OK);
    LE_ASSERT(le_gnss_GetConstellation(&constellationMask) == LE_OK);
    LE_ASSERT(constellationMask == (LE_GNSS_CONSTELLATION_GPS |
                                    LE_GNSS_CONSTELLATION_GLONASS))
    sleep(2);

    // test4: error test (GPS constellation is not set)
    //        and Beidou is unknown for mdm9x15
    constellationMask = LE_GNSS_CONSTELLATION_BEIDOU;
#if defined(SIERRA_MDM9X40) || defined(SIERRA_MDM9X28)
    LE_ASSERT((le_gnss_SetConstellation(constellationMask)) == LE_FAULT);
#else
    LE_ASSERT((le_gnss_SetConstellation(constellationMask)) == LE_UNSUPPORTED);
#endif

    LE_ASSERT(le_gnss_GetConstellation(&constellationMask) == LE_OK);
    // test constellationMask has not changed after error
    LE_ASSERT(constellationMask == (LE_GNSS_CONSTELLATION_GPS |
                                    LE_GNSS_CONSTELLATION_GLONASS));
    sleep(2);

    // next tests have same results as test4 for mdm9x15
#if defined(SIERRA_MDM9X40) || defined(SIERRA_MDM9X28)

    // test5: Gps+Glonass+Beidou selection
    constellationMask = LE_GNSS_CONSTELLATION_GPS |
                        LE_GNSS_CONSTELLATION_GLONASS |
                        LE_GNSS_CONSTELLATION_BEIDOU;

    LE_ASSERT(le_gnss_SetConstellation(constellationMask) == LE_OK);
    LE_ASSERT(le_gnss_GetConstellation(&constellationMask) == LE_OK);
    LE_ASSERT(constellationMask == (LE_GNSS_CONSTELLATION_GPS |
                                    LE_GNSS_CONSTELLATION_GLONASS |
                                    LE_GNSS_CONSTELLATION_BEIDOU))
    sleep(2);

    // test6: Gps+Beidou selection
    constellationMask = LE_GNSS_CONSTELLATION_GPS |
                        LE_GNSS_CONSTELLATION_BEIDOU;
    LE_ASSERT(le_gnss_SetConstellation(constellationMask) == LE_OK);
    LE_ASSERT(le_gnss_GetConstellation(&constellationMask) == LE_OK);
    LE_ASSERT(constellationMask == (LE_GNSS_CONSTELLATION_GPS |
                                    LE_GNSS_CONSTELLATION_BEIDOU))
    sleep(2);

    // test6: Gps+Glonass+Beidou+Galileo selection
    constellationMask = LE_GNSS_CONSTELLATION_GPS |
                        LE_GNSS_CONSTELLATION_GLONASS |
                        LE_GNSS_CONSTELLATION_BEIDOU |
                        LE_GNSS_CONSTELLATION_GALILEO;

    LE_ASSERT(le_gnss_SetConstellation(constellationMask) == LE_OK);
    LE_ASSERT(le_gnss_GetConstellation(&constellationMask) == LE_OK);
    LE_ASSERT(constellationMask == (LE_GNSS_CONSTELLATION_GPS |
                                    LE_GNSS_CONSTELLATION_GLONASS |
                                    LE_GNSS_CONSTELLATION_BEIDOU |
                                    LE_GNSS_CONSTELLATION_GALILEO))
    sleep(2);

    // test7: Gps+Glonass+Beidou+Galileo+unknown selection
    constellationMask = LE_GNSS_CONSTELLATION_GPS |
                        LE_GNSS_CONSTELLATION_GLONASS |
                        LE_GNSS_CONSTELLATION_BEIDOU |
                        LE_GNSS_CONSTELLATION_GALILEO |
                        LE_GNSS_CONSTELLATION_GALILEO | 5;

    LE_ASSERT(le_gnss_SetConstellation(constellationMask) == LE_OK);
    LE_ASSERT(le_gnss_GetConstellation(&constellationMask) == LE_OK);
    LE_ASSERT(constellationMask == (LE_GNSS_CONSTELLATION_GPS |
                                    LE_GNSS_CONSTELLATION_GLONASS |
                                    LE_GNSS_CONSTELLATION_BEIDOU |
                                    LE_GNSS_CONSTELLATION_GALILEO))
#endif
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
     LE_INFO("======== GNSS device Start Test SUCCESS ========");
}
