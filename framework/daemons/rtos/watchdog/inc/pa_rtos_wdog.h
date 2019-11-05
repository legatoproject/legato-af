/**
 * @file pa_rtos_wdog.h
 *
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PA_RTOS_WATCH_DOG_INCLUDE_GUARD
#define LEGATO_PA_RTOS_WATCH_DOG_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Shutdown action to take if a service is not kicking
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_rtosWdog_Shutdown
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Kick the external watchdog
 *
 * @returns
 *      0 if successful
 *      -1 on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int pa_rtosWdog_Kick
(
    void* handle       /// Opaque handle of registered client
);


//--------------------------------------------------------------------------------------------------
/**
 * Register a client with the PA watchdog service
 *
 * @return
 *      opaque handle on successful registration
 *      NULL on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void* pa_rtosWdog_Register
(
    pthread_t clientId, /// Client-ID
    uint32_t timeout    /// Timeout (in milliseconds)
);


//--------------------------------------------------------------------------------------------------
/**
 * De-Register a client from the PA watchdog service
 *
 * @return
 *      0 if successful
 *      -1 on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int pa_rtosWdog_Deregister
(
    void* handle       /// Opaque handle of registered client
);


//--------------------------------------------------------------------------------------------------
/**
 * Initialize PA watchdog
 **/
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_rtosWdog_Init
(
    void
);


#endif // LEGATO_PA_RTOS_WATCH_DOG_INCLUDE_GUARD
