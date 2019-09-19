/*
 * Simple positive sanity test on watchdog chain.
 *
 * These tests do not ensure proper functioning of watchdog chain, just that a properly configured
 * watchdog chain does not interfere with normal operation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * The handler for external watchdog kicks.
 *
 * Check to ensure all timers are running, and if so kick the external watchdog(s).
 */
//--------------------------------------------------------------------------------------------------
static void ExternalWatchdogHandler
(
    le_timer_Ref_t timerRef
)
{
}

//-------------------------------------------------------------------------------------------------
/**
 * Kicks the watchdog timer.
 *
 * Once the watchdog has been kicked it must be kicked again before the expiration of the current
 * effective timeout else the configured WatchdogAction will be executed.
 */
//-------------------------------------------------------------------------------------------------
void le_wdog_Kick
(
    void
)
{
    LE_INFO("Woof woof!");
}

//-------------------------------------------------------------------------------------------------
/**
 * Set a time out.
 *
 * The watchdog is kicked and a new effective timeout value is set. The new timeout will be
 * effective until the next kick at which point it will revert to the original value.
 */
//-------------------------------------------------------------------------------------------------
le_result_t le_wdog_Timeout
(
    uint32_t milliseconds ///< The number of milliseconds until this timer expires
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the watchdog timeout configured for this process
 *
 * @return
 *      - LE_OK            The watchdog timeout is configured and returned
 *      - LE_NOT_FOUND     The watchdog timeout is not set
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_wdog_GetWatchdogTimeout
(
    uint64_t* millisecondsPtr ///< The watchdog timeout set for this process
)
{
    le_timer_Ref_t timerRef;
    ExternalWatchdogHandler(timerRef);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the max watchdog timeout configured for this process
 *
 * @return
 *      - LE_OK            The max watchdog timeout is configured and returned
 *      - LE_NOT_FOUND     The max watchdog timeout is not set
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_wdog_GetMaxWatchdogTimeout
(
    uint64_t* millisecondsPtr ///< The max watchdog timeout set for this process
)
{
    return LE_OK;
}


COMPONENT_INIT
{
    LE_INFO("Initialized dumb watchdog");
}
