/*
 * Simple positive sanity test on watchdog chain.
 *
 * These tests do not ensure proper functioning of watchdog chain, just that a properly configured
 * watchdog chain does not interfere with normal operation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "watchdogChain.h"

/*
 * How often to kick watchdog
 */
static const le_clk_Time_t watchdogKickTime = { .sec = 2, .usec = 0 };

COMPONENT_INIT
{
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogKickTime);
}

void sleep_Sleep
(
    int32_t sleepTime
)
{
    sleep(sleepTime);
}
