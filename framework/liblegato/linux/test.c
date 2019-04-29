/**
 * @file test.c
 *
 * Unitary test framework functions for Linux
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "fa/test.h"

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the unitary test subsystem.
 */
//--------------------------------------------------------------------------------------------------
void fa_test_Init
(
    void
)
{
    // Do nothing.
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a unitary test suite
 */
//--------------------------------------------------------------------------------------------------
void fa_test_Start
(
    void
)
{
    // Do nothing.
}

//--------------------------------------------------------------------------------------------------
/**
 * Exit a unitary test suite
 */
//--------------------------------------------------------------------------------------------------
void fa_test_Exit
(
    int failedTests
)
{
    // Linux exit codes should be in range 0-127 (except for signals), so if there are more
    // than 126 failed tests, saturate to 126 max.  The value 127 is not used to ease testing
    // because the shell returns that if the executable was not found.
    if (failedTests > 126)
    {
        failedTests = 126;
    }
    exit(failedTests);
}
