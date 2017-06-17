/*
 * Sample showing how to monitor a critical process with a watchdog.
 *
 * In this case logging "Hello World" every 10s or so
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "watchdogChain.h"

COMPONENT_INIT
{
    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = 8 };
    LE_INFO("Hello World!");
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);
    sleep(20);
}
