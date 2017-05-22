/**
 * This module implements the unit tests for VOICE CALL API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{

    // To reactivate for all DEBUG logs
    //le_log_SetFilterLevel(LE_LOG_DEBUG);

    LE_INFO("======== START UnitTest of VOICE CALL API ========");

    LE_INFO("======== UnitTest of VOICE CALL API FINISHED ========");
    exit(EXIT_SUCCESS);
}
