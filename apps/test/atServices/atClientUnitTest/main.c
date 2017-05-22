/**
 * This module implements the unit tests for AT Client API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
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

    LE_INFO("======== START UnitTest of AT CLIENT API ========");

    LE_INFO("======== UnitTest of AT CLIENT API FINISHED ========");
    exit(EXIT_SUCCESS);
}
