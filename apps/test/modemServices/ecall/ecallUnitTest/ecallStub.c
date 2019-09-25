/**
 * This module implements some stub for mrc unit test.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Begin monitoring the event loop on the current thread.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_MonitorEventLoop
(
    uint32_t watchdog,          ///< Watchdog to use for monitoring
    le_clk_Time_t watchdogInterval ///< Interval at which to check event loop is functioning
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop a watchdog.
 *
 * This can also cause the chain to be completely kicked, so check it.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_Stop
(
    uint32_t watchdog
)
{
    // do nothing
}
