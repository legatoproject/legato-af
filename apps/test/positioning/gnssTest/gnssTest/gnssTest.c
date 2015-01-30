 /**
  * This module implements a test for GNSS device starting.
  *
  * Copyright (C) 2014 Sierra Wireless, Inc. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
//                                       Test Function
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Test: Restart to Cold start.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_pos_RestartTest
(
    void
)
{
    le_posCtrl_ActivationRef_t      activationRef;

    LE_INFO("Start Test le_pos_RestartTest");

    LE_ASSERT((activationRef = le_posCtrl_Request()) != NULL);

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
    le_posCtrl_Release(activationRef);

}


//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
        LE_INFO("======== GNSS device Start Test  ========");
        Testle_pos_RestartTest();
        LE_INFO("======== GNSS device Start Test SUCCESS ========");
}
