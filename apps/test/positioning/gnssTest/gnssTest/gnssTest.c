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
    LE_INFO("Start Test Testle_gnss_DeviceTest");
    // GNSS device enabled by default
    LE_ASSERT((le_gnss_Enable()) == LE_DUPLICATE);
    // Disable GNSS device (DISABLED state)
    LE_ASSERT((le_gnss_Disable()) == LE_OK);
    LE_ASSERT((le_gnss_Disable()) == LE_DUPLICATE);
    // Check Disabled state
    LE_ASSERT((le_gnss_Start()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_ForceColdRestart()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_Stop()) == LE_NOT_PERMITTED);
    LE_ASSERT((le_gnss_SetConstellation(LE_GNSS_CONSTELLATION_GPS)) == LE_NOT_PERMITTED);
    // Enable GNSS device (READY state)
    LE_ASSERT((le_gnss_Enable()) == LE_OK);
    LE_ASSERT((le_gnss_Disable()) == LE_OK);
    LE_ASSERT((le_gnss_Enable()) == LE_OK);
    LE_ASSERT((le_gnss_SetConstellation(LE_GNSS_CONSTELLATION_GPS)) == LE_OK);
    LE_ASSERT((le_gnss_Stop()) == LE_DUPLICATE);
    LE_ASSERT((le_gnss_ForceColdRestart()) == LE_NOT_PERMITTED);
    // Start GNSS device (ACTIVE state)
    LE_ASSERT((le_gnss_Start()) == LE_OK);
    LE_ASSERT((le_gnss_ForceColdRestart()) == LE_OK);
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
    LE_ASSERT((le_gnss_ForceColdRestart()) == LE_NOT_PERMITTED);
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

    /* Wait for a 3D fix */
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
    LE_INFO("Start Test le_pos_RestartTest");


    LE_ASSERT((le_gnss_Start()) == LE_OK);

    /* Wait for a 3D fix */
    LE_INFO("Wait 60 seconds for a 3D fix");
    sleep(60);

    LE_INFO("Ask for a Cold restart in 3 seconds...");
    sleep(3);
    LE_ASSERT(le_gnss_ForceColdRestart() == LE_OK);

    /* Wait for a 3D fix */
    LE_INFO("Wait 60 seconds for a 3D fix");
    sleep(60);

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
//        gnss_Init();
        LE_INFO("======== GNSS device Test  ========");
        Testle_gnss_DeviceTest();
        LE_INFO("======== GNSS device Start Test  ========");
        Testle_gnss_StartTest();
        LE_INFO("======== GNSS device Restart Test  ========");
        Testle_gnss_RestartTest();
        LE_INFO("======== GNSS device Start Test SUCCESS ========");
}
