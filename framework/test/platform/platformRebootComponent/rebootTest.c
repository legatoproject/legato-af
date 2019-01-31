//--------------------------------------------------------------------------------------------------
/**
 * @file rebootTest.c
 *
 * This file includes tests of platform dependent functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"

COMPONENT_INIT
{
    // reboot() function test
    LE_INFO("\nUnit test for platform dependent function: reboot() call\n");
    reboot(RB_AUTOBOOT);
    // Nothing can be logged at this level after reboot()
}
