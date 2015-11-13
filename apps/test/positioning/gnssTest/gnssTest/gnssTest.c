 /**
  * This module implements a test for GNSS device starting.
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include "interfaces.h"


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
    // Enable GNSS device (READY state)
    LE_ASSERT((le_gnss_Enable()) == LE_OK);
    LE_ASSERT((le_gnss_Disable()) == LE_OK);
    LE_ASSERT((le_gnss_Enable()) == LE_OK);
    LE_ASSERT((le_gnss_SetConstellation(LE_GNSS_CONSTELLATION_GPS)) == LE_OK);
    LE_ASSERT((le_gnss_Stop()) == LE_DUPLICATE);
    LE_ASSERT((le_gnss_ForceHotRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_ForceWarmRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_ForceColdRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_ForceFactoryRestart()) == LE_NOT_PERMITTED);
    // Start GNSS device (ACTIVE state)
    LE_ASSERT((le_gnss_Start()) == LE_OK);
    LE_ASSERT((le_gnss_Start()) == LE_DUPLICATE);
    LE_ASSERT((le_gnss_Enable()) == LE_DUPLICATE);
    LE_ASSERT((le_gnss_Disable()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_SetConstellation(LE_GNSS_CONSTELLATION_GPS)) == LE_NOT_PERMITTED);
    // Stop GNSS device (READY state)
    LE_ASSERT((le_gnss_Stop()) == LE_OK);
    LE_ASSERT((le_gnss_Enable()) == LE_DUPLICATE);
    LE_ASSERT((le_gnss_Disable()) == LE_OK);
    LE_ASSERT((le_gnss_Enable()) == LE_OK);
    LE_ASSERT((le_gnss_SetConstellation(LE_GNSS_CONSTELLATION_GPS)) == LE_OK);
    LE_ASSERT((le_gnss_Stop()) == LE_DUPLICATE);
    LE_ASSERT((le_gnss_ForceHotRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_ForceWarmRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_ForceColdRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_ForceFactoryRestart()) == LE_NOT_PERMITTED);
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
    LE_INFO("Start Test Testle_gnss_StartTest");

    LE_ASSERT((le_gnss_Start()) == LE_OK);

    /* Wait for a position fix */
    LE_INFO("Wait 120 seconds for a 3D fix");
    sleep(120);

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
        // GNSS Init
        LE_INFO("======== GNSS device Test  ========");
        Testle_gnss_DeviceTest();
        LE_INFO("======== GNSS device Start Test  ========");
        Testle_gnss_StartTest();
        LE_INFO("======== GNSS device Restart Test  ========");
        Testle_gnss_RestartTest();
        LE_INFO("======== GNSS device Start Test SUCCESS ========");
}
