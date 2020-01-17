//--------------------------------------------------------------------------------------------------
/** @file main.c
 *
 * Unit test for verifying watchdog long timeout behavior
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------
#include "legato.h"
#include "interfaces.h"

COMPONENT_INIT
{
    int32_t timeoutSec = 5 * 60;
    int32_t timeoutMs = timeoutSec * 1000;
    int32_t checkIntervalSec = 5;
    le_wdog_Timeout(timeoutMs);
    LE_INFO("Setting timeout to %d ms", timeoutMs);
    for (int32_t timeoutCount = 0;
         timeoutCount < timeoutSec - checkIntervalSec;
         timeoutCount += checkIntervalSec)
    {
        LE_INFO("Alive for %d seconds", timeoutCount);
        le_thread_Sleep(checkIntervalSec);
    }

    LE_INFO("Done test to ensure device doesn't reboot prematurely "
            "but need to ensure device reboots");

    le_thread_Sleep(10);
    LE_INFO("FAILED: Should not reach here");
}
