//--------------------------------------------------------------------------------------------------
/** @file watchdogChain.c
 *
 * Provides a watchdog chain to allow multiple tasks in a process to cooperate in kicking the
 * watchdog.  The watchdog will be kicked when all non-stopped tasks on the chain have requested
 * a kick.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "watchdogChain.h"

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of watchdogs supported by the watchdog chain.
 *
 * The chain uses a 64-bit bitstring to store both start/stop state and kicked state, which
 * sets a hard limit of 32 watchdogs on the chain.
 *
 * Should maybe be decreased to 16 to better support 32-bit platforms, but in that case
 * code below should also be modified to match.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_WATCHDOGS                  32

//--------------------------------------------------------------------------------------------------
/**
 * Create enough monitor functions for monitoring a small number of event loops.  If an application
 * needs more than this many event loops it should define monitor functions for those itself.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_EVENT_LOOPS                  8

/// Macro used to generate trace output in this module.
/// Takes the same parameters as LE_DEBUG() et. al.
#define TRACE(...) LE_TRACE(TraceRef, ##__VA_ARGS__)

/// Macro used to query current trace state in this module
#define IS_TRACE_ENABLED LE_IS_TRACE_ENABLED(TraceRef)

//--------------------------------------------------------------------------------------------------
/**
 * Trace reference used for controlling tracing in this module.
 */
//--------------------------------------------------------------------------------------------------
static le_log_TraceRef_t TraceRef;

//--------------------------------------------------------------------------------------------------
/**
 * Watchdog chain.  Statically allocated with the maximum number of allowed watchdogs.
 *
 * First half of chain (bits 0..31) are kicked (1) not kicked (0);
 * second half of chain (bits 32..63) are not stopped (1) or stopped (0).
 *
 * @note using same value for kicked/not stopped allows kicking and starting a watchdog in a
 * single operation.
 */
//--------------------------------------------------------------------------------------------------
static uint64_t WatchdogChain = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Current watchdog count.
 */
//--------------------------------------------------------------------------------------------------
static volatile uint32_t WatchdogCount = 0;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t WatchdogPool;

//--------------------------------------------------------------------------------------------------
/**
 *  Definition of watchdog. Container for managing the timer for every task monitored in
 *  a process.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t watchdog;                  ///< Watchdog to use for monitoring
    le_timer_Ref_t timer;               ///< The timer this watchdog uses
    bool isConnected;                   ///< Is this thread connected to watchdog service
}
WatchdogObj_t;

//--------------------------------------------------------------------------------------------------
/**
 * Array of watchdogs
 */
//--------------------------------------------------------------------------------------------------
static WatchdogObj_t* WatchdogList[MAX_WATCHDOGS];

//--------------------------------------------------------------------------------------------------
/**
 * Timer to queue function to kick watchdog chain. If our queued function is called, it implies
 * the event loop is still running.
 */
//--------------------------------------------------------------------------------------------------
static void CheckEventLoopHandler
(
    le_timer_Ref_t timerRef
)
{
    WatchdogObj_t* watchdogPtr = le_timer_GetContextPtr(timerRef);
    uint32_t watchdog = watchdogPtr->watchdog;

    if (!watchdogPtr->timer)
    {
        // Watchdog is being stopped, but timer had already fired.
        // Dump this event.
        return;
    }

    if (IS_TRACE_ENABLED)
    {
        TRACE("Kicking watchdog chain: %d", watchdog);
    }

    le_wdogChain_Kick(watchdog);
    le_timer_Restart(watchdogPtr->timer);
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if the watchdog chain is all kicked, and if so kick the process watchdog.
 */
//--------------------------------------------------------------------------------------------------
void CheckChain
(
    uint64_t watchdogChain,
    uint32_t localWatchdogCount
)
{
    // Calculate if all watchdogs are either kicked or stopped
    watchdogChain = ~watchdogChain & (watchdogChain >> MAX_WATCHDOGS);
    if (0 == watchdogChain)
    {
        // Yes; kick watchdog and reset kick list.  Could potentially be double kicked if
        // another thread calls le_wdogChain_Kick in here somewhere, but a double kick is not
        // a problem.
        if (IS_TRACE_ENABLED)
        {
            TRACE("Watchdog chain is all kicked, kick watchdog.");
        }

        le_wdog_Kick();
        __sync_and_and_fetch(&WatchdogChain, ((uint64_t)-(INT64_C(1) << MAX_WATCHDOGS)));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Start watchdogs 0..N-1.  Typically this is used in COMPONENT_INIT to start all watchdogs needed
 * by the process.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_Init
(
    uint32_t wdogCount
)
{
    // Ensure watchdog count is within allowable range.
    LE_ASSERT(wdogCount <= MAX_WATCHDOGS);
    LE_FATAL_IF(!__sync_bool_compare_and_swap(&WatchdogCount, 0, wdogCount),
                "Watchdog already initialized");
    // And start all watchdogs.
    __sync_or_and_fetch(&WatchdogChain, ((UINT64_C(1) << wdogCount) - 1) << MAX_WATCHDOGS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get watchdog from our watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
WatchdogObj_t* GetWatchdogChain
(
    uint32_t watchdog         ///< Watchdog to use for monitoring
)
{
    return WatchdogList[watchdog];
}

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
    LE_ASSERT(watchdog < MAX_EVENT_LOOPS);
    le_result_t result;

    WatchdogObj_t* watchdogPtr = GetWatchdogChain(watchdog);

    if (watchdogPtr == NULL)
    {
        watchdogPtr = le_mem_ForceAlloc(WatchdogPool);
        watchdogPtr->watchdog = watchdog;
        watchdogPtr->isConnected = false;
        WatchdogList[watchdog] = watchdogPtr;
        watchdogPtr->timer = NULL;
    }

    if (watchdogPtr->timer == NULL)
    {
        char timerName[8];
        snprintf(timerName, sizeof(timerName), "Chain%02d", watchdog);
        watchdogPtr->timer = le_timer_Create(timerName);
        le_timer_SetHandler(watchdogPtr->timer, CheckEventLoopHandler);
        le_timer_SetContextPtr(watchdogPtr->timer, watchdogPtr);
        le_timer_SetInterval(watchdogPtr->timer, watchdogInterval);
        le_timer_SetWakeup(watchdogPtr->timer, false);
    }

    if (!watchdogPtr->isConnected)
    {
        result = le_wdog_TryConnectService();
        if (LE_OK != result)
        {
            LE_WARN("Failed to connect to watchdog service; watchdog not kicked");
        }
        else
        {
            watchdogPtr->isConnected = true;
        }
    }

    // Immediately kick watchdog, and schedule next kick.
    le_wdogChain_Kick(watchdog);
    le_timer_Start(watchdogPtr->timer);
}


//--------------------------------------------------------------------------------------------------
/**
 * Kick a watchdog on the chain.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_Kick
(
    uint32_t watchdog
)
{
    WatchdogObj_t* watchdogPtr = GetWatchdogChain(watchdog);

    if (watchdogPtr == NULL)
    {
        watchdogPtr = le_mem_ForceAlloc(WatchdogPool);
        watchdogPtr->watchdog = watchdog;
        watchdogPtr->timer = NULL;
        watchdogPtr->isConnected = false;
        WatchdogList[watchdog] = watchdogPtr;
    }

    // If we can't connect to the watchdog service, not point proceeding
    if (!watchdogPtr->isConnected)
    {
        if (LE_OK != le_wdog_TryConnectService())
        {
            LE_DEBUG("Failed to connect wdog service, not kicked!");
            return;
        }
        watchdogPtr->isConnected = true;
    }

    uint32_t localWatchdogCount = WatchdogCount;
    LE_FATAL_IF(watchdog >= localWatchdogCount, "Trying to kick out of range watchdog");
    // Kick and start the watchdog.
    uint64_t watchdogChain = __sync_or_and_fetch(&WatchdogChain,
                                                   (UINT64_C(1) << watchdog)
                                                 | (UINT64_C(1) << (watchdog+MAX_WATCHDOGS)));

    CheckChain(watchdogChain, localWatchdogCount);
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
    uint32_t localWatchdogCount = WatchdogCount;
    LE_FATAL_IF(watchdog >= localWatchdogCount, "Trying to kick out of range watchdog");
    // Mark watchdog as stopped.
    uint64_t watchdogChain = __sync_and_and_fetch(&WatchdogChain,
                                                  ~(UINT64_C(1) << (watchdog+MAX_WATCHDOGS)));

    WatchdogObj_t* watchdogPtr = GetWatchdogChain(watchdog);
    LE_FATAL_IF(watchdogPtr == NULL, "Failed to find watchdog");

    if (watchdogPtr->timer)
    {
        le_timer_Stop(watchdogPtr->timer);
        le_timer_Delete(watchdogPtr->timer);
        watchdogPtr->timer = NULL;
    }

    if ((watchdogChain >> MAX_WATCHDOGS) == 0)
    {
        // All watchdogs are stopped -- stop process watchdog (if allowed).  If not allowed,
        // process should not have stopped all watchdogs on the chain.
        le_wdog_Timeout(LE_WDOG_TIMEOUT_NEVER);
    }
    else
    {
        CheckChain(watchdogChain, localWatchdogCount);
    }

    /* Always disconnect from the service - it uses reference counting so no need to pass
     * watchdog vale
     */
    if (watchdogPtr->isConnected)
    {
        le_wdog_DisconnectService();
        watchdogPtr->isConnected = false;
    }
}

COMPONENT_INIT
{
    // Get a reference to the trace keyword that is used to control tracing in this module.
    TraceRef = le_log_GetTraceRef("wdog");

    WatchdogPool = le_mem_CreatePool("WatchdogChainPool", sizeof(WatchdogObj_t));
}
