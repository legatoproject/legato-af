/**
 * @file pa_wdog.h
 *
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PA_WATCH_DOG_INCLUDE_GUARD
#define LEGATO_PA_WATCH_DOG_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Shutdown action to take if a service is not kicking
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_wdog_Shutdown
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Kick the external watchdog
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_wdog_Kick
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Initialize PA watchdog
 **/
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_wdog_Init
(
    void
);


#endif // LEGATO_PA_WATCH_DOG_INCLUDE_GUARD