//--------------------------------------------------------------------------------------------------
/**
 * @file wdog.c
 *
 * @section wd_intro        Introduction
 *
 * The watchdog provides a means of recovering the system if apps or components cease
 * functioning due to infinite loops, deadlocks and waiting on things that never happen.
 * By having a process call the le_wdog_Kick() method that process now becomes watched and if
 * le_wdog_Kick() is not called again within the configured time the device will be restarted.
 */
//--------------------------------------------------------------------------------------------------
 /** @section wd_tedious_detail            More involved discussion follows
 *
 * The watchdog runs as a service which monitors critical processes on the system to check
 * if they are alive, and takes corrective action, such as restarting the process,  if not.
 *
 * Apps should configure a default time out before they make use of the
 * watchdog. If a timeout is not configured a warning will be issued in the logs on the first use
 * of the le_wdog and a timeout of 30 seconds will be used. The following sections in the adef file
 * control watchdog behaviour.
 *
 *      watchdogTimeout: <number of millisecond>
 *      maxWatchdogTimeout: <number of millisecond>
 *
 * For critical processes a maximum timeout should be given so the process cannot accidentally disable
 * the watchdog.  This can be given in the adef file in a maximumWatchdogTimeout section.
 *
 * Algorithm
 * When a process kicks us, if we have no timer for it we will register
 * the process with the watchdog service with the specified timeout.
 * If the timer times out before the next kick then the watchdog will reboot the device
 *
 * Besides le_wdog_Kick(), a command to temporarily change the timeout is provided.
 * le_wdog_Timeout(milliseconds) will adjust the current timeout and restart the timer.
 * This timeout will be effective for one time only reverting to the default value at the next
 * le_wdog_Kick().
 *
 * There are two special timeout values, LE_WDOG_TIMEOUT_NOW and LE_WDOG_TIMEOUT_NEVER.
 *
 * LE_WDOG_TIMEOUT_NEVER will cause a timer to never time out. The largest attainable timeout value
 * that does time out is (LE_WDOG_TIMEOUT_NEVER - 1) which gives a timeout of about 49 days. If 49
 * days is not long enough for your purposes then LE_WDOG_TIMEOUT_NEVER will make sure that the
 * process can live indefinately without calling le_wdog_Kick(). If you find yourself using this
 * special value often you might want to reconsider whether you really want to use a watchdog timer
 * for your process.
 *
 * LE_WDOG_TIMEOUT_NOW could be used in development to see how the app responds to a timeout
 * situation though it could also be abused as a way to restart the app for some reason.
 *
 * @note Critical systems rely on the watchdog daemon to ensure system liveness, so all
 * unrecoverable errors in the watchdogDaemon are considered fatal to the system, and will
 * cause a system reboot by calling LE_FATAL or LE_ASSERT.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "microSupervisor.h"
#include "interfaces.h"
#include "pa_rtos_wdog.h"

#define DEFAULT_APP_TIMEOUT 30000

typedef struct WatchdogObj_t
{
    pthread_t threadId;
    void* handle;
    uint32_t currTimeout;
    uint32_t watchdogTimeoutMs;
    uint32_t maxWatchdogTimeoutMs;
}
WatchdogObj;

LE_MEM_DEFINE_STATIC_POOL(WatchdogPool, LE_CONFIG_WDOG_HASHTABLE_SIZE, sizeof(WatchdogObj));
LE_HASHMAP_DEFINE_STATIC(WatchdogRefs, LE_CONFIG_WDOG_HASHTABLE_SIZE);
static le_mem_PoolRef_t WatchdogPool;  ///< The container we use to keep track of wdogs
static le_hashmap_Ref_t WatchdogRefsContainer;  ///< The container we use to keep track of wdogs

/**
 *  Attempts to get the thread ID of the main task using the
 *  current thread ID.
 */
static pthread_t GetThreadId
(
    void
)
{
    pid_t clientProcId;
    le_msg_SessionRef_t sessionRef = le_wdog_GetClientSessionRef();
    if (LE_OK != le_msg_GetClientProcessId(sessionRef, &clientProcId))
    {
        LE_ERROR("Unable to retrieve caller threadId");
    }

    // on RTOS pid_t is the same as the pthread id
    return (pthread_t) clientProcId;
}

/**
 *  Sets the kick timeout and max timeout based on the .adef configurations
 */
static void SetWatchdogTimeouts
(
    WatchdogObj* wdogPtr  ///< [IN] pointer to the watchdog to configure
)
{
    pthread_t threadId = GetThreadId();
    int32_t configMaxWatchdogTimeout = le_microSupervisor_GetMaxWatchdogTimeout(threadId);
    int32_t configWatchdogTimeout = le_microSupervisor_GetWatchdogTimeout(threadId);
    uint32_t timeoutMs = 0;
    uint32_t maxTimeoutMs = (uint32_t) LE_WDOG_TIMEOUT_NEVER;

    // If manual start is set or if no max watchdog, use regular timeout
    if (le_microSupervisor_GetManualStart(threadId) || configMaxWatchdogTimeout == 0)
    {
        if (configWatchdogTimeout != 0)
        {
            timeoutMs = configWatchdogTimeout;
        }
    }
    else
    {
        // Use max watchdog timeout if it is configured
        if (configMaxWatchdogTimeout != 0)
        {
            maxTimeoutMs = (uint32_t) configMaxWatchdogTimeout;
        }

        timeoutMs = maxTimeoutMs;
    }

    // If we still have no timeout configuration at this point
    // provide a default timeout
    if (timeoutMs == 0)
    {
        LE_WARN("No timeout specified, using default timeout of 30 seconds");
        timeoutMs = DEFAULT_APP_TIMEOUT;
    }

    wdogPtr->watchdogTimeoutMs = timeoutMs;
    wdogPtr->maxWatchdogTimeoutMs = maxTimeoutMs;
    LE_DEBUG("Timeout: %ld, maxTimeoutMs %ld", timeoutMs, maxTimeoutMs);
    return;
}

/**
 *  Retrieves watchdog information based on current process.
 *  Creates a new entry in the hashmap if the watchdog currently does
 *  not exist for the process.
 */
static WatchdogObj* GetWatchdogObjPtr
(
    void
)
{
    pthread_t threadId = GetThreadId();
    WatchdogObj* wdogPtr = le_hashmap_Get(WatchdogRefsContainer, &threadId);
    if (!wdogPtr)
    {
        wdogPtr = le_mem_Alloc(WatchdogPool);
        wdogPtr->threadId = threadId;
        wdogPtr->handle = 0;
        wdogPtr->currTimeout = 0;
        SetWatchdogTimeouts(wdogPtr);
        LE_ASSERT(NULL == le_hashmap_Put(WatchdogRefsContainer, &(wdogPtr->threadId), wdogPtr));
    }

    return wdogPtr;
}

/**
 * Helper function to register/deregister/kick the watchdog based on the
 * provided timeout.
 */
static void SetupAppWatchdog
(
    WatchdogObj* wdogPtr,    ///< [IN] the watchdog to setup
    uint32_t timeoutMs       ///< [IN] the timeout to configure
)
{
    /**
     * If a new watchdog is detected or if the timeout is different from
     * the previously configured timeout, reregister the thread with a new timeout
     */
    if (wdogPtr->handle == 0 || (timeoutMs != (wdogPtr->currTimeout)))
    {
        if (timeoutMs > wdogPtr->maxWatchdogTimeoutMs)
        {
            timeoutMs = wdogPtr->maxWatchdogTimeoutMs;
        }
        wdogPtr->currTimeout = timeoutMs;
        wdogPtr->handle = pa_rtosWdog_Register(wdogPtr->threadId, timeoutMs);
    }

    if (timeoutMs == (uint32_t) LE_WDOG_TIMEOUT_NEVER)
    {
        // Stop monitoring the watchdog
        LE_DEBUG("Setting handle %lu to never timeout\n", wdogPtr->handle);
        pa_rtosWdog_Deregister(wdogPtr->handle);
    }
    else
    {
        pa_rtosWdog_Kick(wdogPtr->handle);
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Calling watchdog kick resets the watchdog expiration timer and briefly cheats death.
 */
//--------------------------------------------------------------------------------------------------
void le_wdog_Kick
(
    void
)
{
    WatchdogObj* wdogPtr = GetWatchdogObjPtr();
    if (wdogPtr)
    {
        SetupAppWatchdog(wdogPtr, wdogPtr->watchdogTimeoutMs);
    }
    else
    {
        LE_ERROR("Unable to find watchdog for current client");
    }
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adjust the timeout. This can be used if you need a different interval for the timeout on a
 * specific occassion. The new value of the timeout lasts until expiry or the next kick. On
 * the next kick, the timeout will revert to the original configured value.
 *
 * @notes
 *     LE_WDOG_TIMEOUT_NEVER disables the watchdog (until it is kicked again or a new timeout is set)
 *     LE_WDOG_TIMEOUT_NOW is a zero length interval and causes the watchdog to expire immediately.
 */
//--------------------------------------------------------------------------------------------------
void le_wdog_Timeout
(
    int32_t timeoutMs ///< [IN] number of milliseconds before the watchdog expires
)
{
    WatchdogObj* wdogPtr = GetWatchdogObjPtr();

    if (wdogPtr)
    {
        if ((timeoutMs < 0) && (timeoutMs != LE_WDOG_TIMEOUT_NEVER))
        {
            LE_ERROR("Invalid watchdog timeout %" PRId32, timeoutMs);
            return;
        }

        SetupAppWatchdog(wdogPtr, timeoutMs);
    }
    else
    {
        LE_ERROR("Unable to find watchdog for current client");
    }
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the watchdog timeout configured for this process
 *
 * @return
 *      - LE_OK            The watchdog timeout is configured and returned
 *      - LE_NOT_FOUND     The watchdog timeout is not set
 *      - LE_FAULT         An error has occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_wdog_GetWatchdogTimeout
(
    uint64_t* timeoutMs ///< [OUT] number of milliseconds before the watchdog expires
)
{
    if (timeoutMs == NULL)
    {
        return LE_FAULT;
    }

    pthread_t threadId = GetThreadId();
    WatchdogObj* wdogPtr = le_hashmap_Get(WatchdogRefsContainer, &threadId);
    if (wdogPtr)
    {
        *timeoutMs = wdogPtr->watchdogTimeoutMs;
        return LE_OK;
    }
    else
    {
        return LE_NOT_FOUND;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the max watchdog timeout configured for this process
 *
 * @return
 *      - LE_OK            The max watchdog timeout is configured and returned
 *      - LE_NOT_FOUND     The max watchdog timeout is not set
 *      - LE_FAULT         An error has occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_wdog_GetMaxWatchdogTimeout
(
    uint64_t* timeoutMs ///< [OUT] number of milliseconds before the watchdog expires
)
{
    if (timeoutMs == NULL)
    {
        return LE_FAULT;
    }

    pthread_t threadId = GetThreadId();
    WatchdogObj* wdogPtr = le_hashmap_Get(WatchdogRefsContainer, &threadId);
    if (wdogPtr && (wdogPtr->maxWatchdogTimeoutMs != (uint32_t)LE_WDOG_TIMEOUT_NEVER))
    {
        *timeoutMs = wdogPtr->maxWatchdogTimeoutMs;
        return LE_OK;
    }

    return LE_NOT_FOUND;
}

COMPONENT_INIT
{
    // Initialize hashmaps for storing watchdog information
    WatchdogPool = le_mem_InitStaticPool(
                        WatchdogPool,
                         LE_CONFIG_WDOG_HASHTABLE_SIZE,
                         sizeof(WatchdogObj));
    WatchdogRefsContainer = le_hashmap_InitStatic(
                         WatchdogRefs,
                         LE_CONFIG_WDOG_HASHTABLE_SIZE,
                         le_hashmap_HashUInt32,
                         le_hashmap_EqualsUInt32
                       );

    LE_ASSERT(WatchdogRefsContainer != NULL);
}
