/**
 * @file frameworkWdog.c
 *
 * Implements the frameworkWdog.api by setting up a timer in a framework daemon to notify
 * watchdog that the daemon is still alive.
 *
 * Framework daemons cannot use the normal wdogChain API since the watchdog relies on most of
 * the other framework daemons for its operation.  Having the daemons as clients of the watchdog
 * would introduce a circular dependency, leading to eventual deadlocks.
 */

#include "legato.h"
#include "interfaces.h"

/*
 * Token pasting to hack around requirement that no two framework services can provide
 * the same api.
 *
 * Seemingly extra macros are needed so CPP expands FRAMEWORK_WDOG_NAME before pasting.
 */
#define FULL_API_NAME(apiName, objectName) apiName ## _ ## objectName
#define FULL_API_NAME2(apiName, objectName) FULL_API_NAME(apiName, objectName)
#define FRAMEWORK_WDOG(objectName) FULL_API_NAME2(FRAMEWORK_WDOG_NAME, objectName)

/**
 * Function to call every time to kick the watchdog.
 */
FRAMEWORK_WDOG(KickHandlerFunc_t) KickHandler = NULL;

/**
 * Context pointer for kick function.
 */
void* KickContextPtr = NULL;

/**
 * Reference for the watchdog kick timer.
 */
static le_timer_Ref_t KickTimerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Timer handler to periodically call the KickHandler.
 */
//--------------------------------------------------------------------------------------------------
static void KickTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    if (KickHandler)
    {
        KickHandler(KickContextPtr);
        le_timer_Start(timerRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'frameworkWdog_KickEvent'
 *
 * This event is fired by a framework daemon periodically in its event loop to notify
 * the watchdogDaemon it's still alive.
 */
//--------------------------------------------------------------------------------------------------
FRAMEWORK_WDOG(KickEventHandlerRef_t) FRAMEWORK_WDOG(AddKickEventHandler)
(
    uint32_t interval,
    FRAMEWORK_WDOG(KickHandlerFunc_t) handlerPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
{
    if (KickHandler)
    {
        LE_ERROR("Kick function already set.");
        return NULL;
    }

    KickTimerRef = le_timer_Create("WatchdogKick");
    if (!KickTimerRef)
    {
        // Even though this will likely reboot shortly due to watchdog failure, just error
        // now and let watchdog handle error case.
        LE_ERROR("Failed to create watchdog kick timer");
        return NULL;
    }
    le_timer_SetHandler(KickTimerRef, KickTimerHandler);
    le_timer_SetMsInterval(KickTimerRef, interval);
    le_timer_SetRepeat(KickTimerRef, 1);
    le_timer_SetWakeup(KickTimerRef, false);
    le_timer_Start(KickTimerRef);

    KickHandler = handlerPtr;
    KickContextPtr = contextPtr;

    return (void*)1;
}



//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'frameworkWdog_KickEvent'
 */
//--------------------------------------------------------------------------------------------------
void FRAMEWORK_WDOG(RemoveKickEventHandler)
(
    FRAMEWORK_WDOG(KickEventHandlerRef_t) handlerRef
        ///< [IN]
)
{
    // Remove kick function.  Only ever one kick function defined.
    KickHandler = NULL;
    KickContextPtr = NULL;

    if (KickTimerRef)
    {
        le_timer_Delete(KickTimerRef);
        KickTimerRef = NULL;
    }
}
