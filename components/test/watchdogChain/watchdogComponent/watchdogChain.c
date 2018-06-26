/*
 * Simple positive sanity test on watchdog chain.
 *
 * These tests do not ensure proper functioning of watchdog chain, just that a properly configured
 * watchdog chain does not interfere with normal operation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "watchdogChain.h"

// Number of times to test kicking watchdog.  Needs to be such that KICK_COUNT*SLEEP_TIME is
// greater than watchdog timeout (5s)
#define KICK_COUNT 5

// Amount of time between kicks (in seconds)
#define SLEEP_TIME 2

COMPONENT_INIT
{
    int i;
    // On failure program will exit, so all tests are LE_TEST_OK(true, ...)
    LE_TEST_PLAN(1 + 2*KICK_COUNT);
    le_wdogChain_Init(2);
    LE_TEST_OK(true, "watchdog chain initialized");
    for (i = 0; i < KICK_COUNT; ++i)
    {
        le_wdogChain_Kick(0);
        le_wdogChain_Kick(1);
        sleep(SLEEP_TIME);
        LE_TEST_OK(true, "2/2 active watchdogs: program running after %d seconds", (i+1)*2);
    }
    le_wdogChain_Stop(0);
    for (i = 0; i < KICK_COUNT; ++i)
    {
        le_wdogChain_Kick(1);
        sleep(SLEEP_TIME);
        LE_TEST_OK(true, "1/2 active watchdogs: program running after %d seconds", (i+1)*2);
    }
    LE_TEST_EXIT;
}
