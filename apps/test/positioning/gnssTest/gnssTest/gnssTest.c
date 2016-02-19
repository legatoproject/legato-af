 /**
  * This module implements a test for GNSS device starting.
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include "interfaces.h"

/*
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <semaphore.h>
*/
// Include macros for printing out values
#include "le_print.h"

static le_gnss_PositionHandlerRef_t PositionHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
//                                       Test Function
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Test: GNSS device.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_gnss_DeviceTest
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

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Position Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PositionHandlerFunction(le_gnss_SampleRef_t positionSampleRef, void* contextPtr)
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
    uint32_t hspeed;
    uint32_t hspeedAccuracy;
    // Vertical speed
    int32_t vspeed;
    int32_t vspeedAccuracy;
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

    result = le_gnss_GetDate(positionSampleRef
                            , &year
                            , &month
                            , &day);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));

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
                            , &hspeed
                            , &hspeedAccuracy);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));

    if(result == LE_OK)
    {
        LE_INFO("hSpeed %d - Accuracy %d"
                , hspeed/100, hspeedAccuracy/10);
    }
    else
    {
        LE_INFO("hSpeed unknown [%d,%d]"
                , hspeed, hspeedAccuracy);
    }

    // Get vertical speed
    result = le_gnss_GetVerticalSpeed( positionSampleRef
                            , &vspeed
                            , &vspeedAccuracy);
    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));
    if(result == LE_OK)
    {
        LE_INFO("vSpeed %d - Accuracy %d"
                , vspeed/100, vspeedAccuracy/10);
    }
    else
    {
        LE_INFO("vSpeed unknown [%d,%d]"
                , vspeed, vspeedAccuracy);
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

    // Release provided Position sample reference
    le_gnss_ReleaseSampleRef(positionSampleRef);

}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Add Position Handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void* Test_thread(void* context)
{

    le_gnss_ConnectService();

    LE_INFO("======== Position Handler thread  ========");
    PositionHandlerRef = le_gnss_AddPositionHandler(PositionHandlerFunction, NULL);
    LE_ASSERT((PositionHandlerRef != NULL));

    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: GNSS position handler
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_gnss_PositionHandlerTest
(
    void
)
{
    LE_INFO("Start Test Testle_gnss_PositionHandlerTest");

    LE_INFO("Start GNSS");
    LE_ASSERT((le_gnss_Start()) == LE_OK);
    LE_INFO("Wait 5 seconds");
    sleep(5);

    // Add Position Handler Test
    le_thread_Start(le_thread_Create("Test_GNSS_Handler_Thread",Test_thread,NULL));

    LE_INFO("Wait for a 3D fix");
    sleep(60);

    le_gnss_RemovePositionHandler(PositionHandlerRef);

    LE_INFO("Wait 5 seconds");
    sleep(5);

    LE_INFO("Stop GNSS");
    LE_ASSERT((le_gnss_Stop()) == LE_OK);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: GNSS Position request.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_gnss_StartTest
(
    void
)
{
    uint32_t rate = 0;

    LE_INFO("Start Test Testle_gnss_StartTest");

    LE_ASSERT((le_gnss_GetAcquisitionRate(&rate)) == LE_OK);
    LE_INFO("Acquisition rate %d ms", rate);
    LE_ASSERT((le_gnss_SetAcquisitionRate(rate)) == LE_OK);

    LE_INFO("Start GNSS");
    LE_ASSERT((le_gnss_Start()) == LE_OK);

    /* Wait for a position fix */
    LE_INFO("Wait 120 seconds for a 3D fix");
    sleep(120);

    LE_INFO("Stop GNSS");
    LE_ASSERT((le_gnss_Stop()) == LE_OK);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: Restart to Cold start.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_gnss_RestartTest
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



//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{

        LE_INFO("======== GNSS device Test  ========");
        Testle_gnss_DeviceTest();
        LE_INFO("======== GNSS device Start Test  ========");
        Testle_gnss_StartTest();
        LE_INFO("======== GNSS device Restart Test  ========");
        Testle_gnss_RestartTest();
        LE_INFO("======== GNSS position handler Test  ========");
        Testle_gnss_PositionHandlerTest();
        LE_INFO("======== GNSS device Start Test SUCCESS ========");
}
