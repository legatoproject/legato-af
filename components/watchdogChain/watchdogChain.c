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

#ifndef MAX_WATCHDOG_CHAINS
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
#define MAX_WATCHDOG_CHAINS                  32
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Create enough monitor functions for monitoring a small number of event loops.  If an application
 * needs more than this many event loops it should define monitor functions for those itself.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_EVENT_LOOPS                  8

#if MAX_WATCHDOG_CHAINS > 16
typedef uint64_t watchdog_t;
#   define WATCHDOG_C(x) UINT64_C(x)
#   define PRIXWDC       PRIX64
#else
typedef uint32_t watchdog_t;
#   define WATCHDOG_C(x) UINT32_C(x)
#   define PRIXWDC       PRIX32
#endif

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
    le_thread_Ref_t monitoredLoop;      ///< Event loop being monitored, or NULL if not
                                        ///< monitoring an event loop.
    bool isConnected;                   ///< Is this thread connected to watchdog service
    bool shouldConnect;                 ///< Should this thread try to connect to watchdog service?
                                        ///< If not bound to a watchdog service, don't try to
                                        ///< reconnect
}
WatchdogObj_t;

//--------------------------------------------------------------------------------------------------
/**
 * Static memory for the watchdog chain pool
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(WatchdogChain, MAX_WATCHDOG_CHAINS, sizeof(WatchdogObj_t));

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for watchdog chain.
 *
 * On RTOS this is shared across all components.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t WatchdogPool;

LE_CDATA_DECLARE({
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
watchdog_t WatchdogChain;

//--------------------------------------------------------------------------------------------------
/**
 * Current watchdog count.
 */
//--------------------------------------------------------------------------------------------------
volatile uint32_t WatchdogCount;

//--------------------------------------------------------------------------------------------------
/**
 * Array of watchdogs
 */
//--------------------------------------------------------------------------------------------------
WatchdogObj_t* WatchdogList[MAX_WATCHDOG_CHAINS];

//--------------------------------------------------------------------------------------------------
/**
 * Trace reference used for controlling tracing in this module.
 */
//--------------------------------------------------------------------------------------------------
le_log_TraceRef_t TraceRef;
});

/// Macro used to generate trace output in this module.
/// Takes the same parameters as LE_DEBUG() et. al.
#define TRACE(...) LE_TRACE(LE_CDATA_THIS->TraceRef, ##__VA_ARGS__)

//--------------------------------------------------------------------------------------------------
/**
 *  Mark one watchdog in the chain as having been started.
 *
 *  @return Updated watchdog chain bitfield.
 */
//--------------------------------------------------------------------------------------------------
static inline watchdog_t MarkOneStarted
(
    uint32_t watchdog   ///< Index of watchdog to mark as started.
)
{
    watchdog_t mask =   (WATCHDOG_C(1) << watchdog) |
                        (WATCHDOG_C(1) << (watchdog + MAX_WATCHDOG_CHAINS));
    return __sync_or_and_fetch(&LE_CDATA_THIS->WatchdogChain, mask);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Mark one watchdog in the chain as having been stopped.
 *
 *  @return Updated watchdog chain bitfield.
 */
//--------------------------------------------------------------------------------------------------
static inline watchdog_t MarkOneStopped
(
    uint32_t watchdog   ///< Index of watchdog to mark as stopped.
)
{
    watchdog_t mask = ~(WATCHDOG_C(1) << (watchdog + MAX_WATCHDOG_CHAINS));
    return __sync_and_and_fetch(&LE_CDATA_THIS->WatchdogChain, mask);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Mark several watchdogs in the chain as having been started.
 *
 *  @return Updated watchdog chain bitfield.
 */
//--------------------------------------------------------------------------------------------------
static inline watchdog_t MarkManyStarted
(
    watchdog_t watchdogs    ///< Bitmask of watchdogs to mark as started.
)
{
    watchdog_t mask = watchdogs << MAX_WATCHDOG_CHAINS;
    return __sync_or_and_fetch(&LE_CDATA_THIS->WatchdogChain, mask);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Produce the value with the lowest N bits set.
 *
 *  @return Value with only the N lowest bits set.
 */
//--------------------------------------------------------------------------------------------------
static inline watchdog_t SetBits
(
    watchdog_t count    ///< Number of bits to set.
)
{
    return (WATCHDOG_C(1) << count) - 1;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Clear the kicked state of all watchdogs in the chain.
 *
 *  @return Updated watchdog chain bitfield.
 */
//--------------------------------------------------------------------------------------------------
static inline watchdog_t MarkAllUnkicked(void)
{
    watchdog_t mask = ~SetBits(MAX_WATCHDOG_CHAINS);
    return __sync_and_and_fetch(&LE_CDATA_THIS->WatchdogChain, mask);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Determine if all active watchdogs in a chain have been kicked.
 *
 *  @return The combined kicked state.
 */
//--------------------------------------------------------------------------------------------------
static inline bool AllHaveBeenKicked
(
    watchdog_t chain    ///< Watchdog chain to examine.
)
{
    return ((~chain & (chain >> MAX_WATCHDOG_CHAINS)) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Determine if all watchdogs in a chain are stopped.
 *
 *  @return The combined stopped state.
 */
//--------------------------------------------------------------------------------------------------
static inline bool AllAreStopped
(
    watchdog_t chain    ///< Watchdog chain to examine.
)
{
    return ((chain >> MAX_WATCHDOG_CHAINS) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a new watchdog, initialized to default values.
 */
//--------------------------------------------------------------------------------------------------
static WatchdogObj_t *CreateNewWatchdog
(
    watchdog_t watchdog                       ///< [IN] Watchdog number to create
)
{
    WatchdogObj_t* watchdogPtr = le_mem_ForceAlloc(WatchdogPool);
    watchdogPtr->watchdog = watchdog;
    watchdogPtr->isConnected = false;
    watchdogPtr->shouldConnect = true;
    watchdogPtr->monitoredLoop = NULL;
    LE_CDATA_THIS->WatchdogList[watchdog] = watchdogPtr;
    watchdogPtr->timer = NULL;

    return watchdogPtr;
}


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

    le_wdogChain_Kick(watchdog);
    le_timer_Restart(watchdogPtr->timer);
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if the watchdog chain is all kicked, and if so kick the process watchdog.
 */
//--------------------------------------------------------------------------------------------------
static void CheckChain
(
    watchdog_t watchdogChain
)
{
    TRACE("Checking chain %08" PRIXWDC, watchdogChain);

    // Calculate if all watchdogs are either kicked or stopped
    if (AllHaveBeenKicked(watchdogChain))
    {
        // Yes; kick watchdog and reset kick list.  Could potentially be double kicked if
        // another thread calls le_wdogChain_Kick in here somewhere, but a double kick is not
        // a problem.
        TRACE("Complete watchdog chain kicked, kicking watchdog.");

        le_wdog_Kick();
        MarkAllUnkicked();
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if a watchdog is connected to the watchdog daemon, and if not, try to connect.
 */
//--------------------------------------------------------------------------------------------------
static bool VerifyConnection
(
    WatchdogObj_t* watchdogPtr
)
{
    if (watchdogPtr->shouldConnect && !watchdogPtr->isConnected)
    {
        le_result_t result;

        result = le_wdog_TryConnectService();
        if (LE_NOT_PERMITTED == result)
        {
            // No binding established for watchdog.  This won't change, so never try
            // to connect
            LE_INFO("Executable not bound to watchdog service; watchdog disabled");
            watchdogPtr->shouldConnect = false;
        }
        else if (LE_OK != result)
        {
            LE_WARN("Failed to connect to watchdog service; watchdog not kicked");
        }
        else
        {
            watchdogPtr->isConnected = true;
        }
    }

    return watchdogPtr->isConnected;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start watchdogs 0..N-1.  Typically this is used in COMPONENT_INIT to start all watchdogs needed
 * by the process.
 *
 * @note Generally the first watchdog is used to monitor the main event loop.  To support this
 * usage with multiple components, le_wdogChain_Init() can be called multiple times.  If this
 * is done, watchdog 0 must be used to monitor the main event loop, and all but one call to
 * le_wdogChain_Init() must initialize 1 watchdog.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_Init
(
    uint32_t wdogCount
)
{
    le_wdogChain_InitSome(wdogCount, SetBits(wdogCount));
}


//--------------------------------------------------------------------------------------------------
/**
 * Start an arbitrary set of the watchdogs out of the range 0..N-1.  Typically this is used in
 * COMPONENT_INIT to start the initial watchdogs needed by the process, but defer starting others
 * until later.  Later watchdogs can be started with an explicit kick, or by starting monitoring.
 *
 * @note Generally the first watchdog is used to monitor the main event loop.  To support this
 * usage with multiple components, le_wdogChain_Init() can be called multiple times.  If this
 * is done, watchdog 0 must be used to monitor the main event loop, and all but one call to
 * le_wdogChain_Init() must initialize 1 watchdog.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_InitSome
(
    uint32_t wdogCount,
    uint32_t which
)
{
    // Ensure watchdog count is within allowable range.
    LE_ASSERT(wdogCount <= MAX_WATCHDOG_CHAINS);

    // Allow multiple init if all but one of the init are for only 1 watchdog.  Assume that
    // in general only one watchdog means just monitoring the main loop.
    uint32_t currentWdogCount = __sync_add_and_fetch(&LE_CDATA_THIS->WatchdogCount, 0);
    LE_FATAL_IF((currentWdogCount > 1) && (wdogCount > 1),
                "Watchdog already initialized with multiple watchdogs");

    // If we need to initialize more watchdogs, do so now.
    if (wdogCount > currentWdogCount)
    {
        LE_FATAL_IF(!__sync_bool_compare_and_swap(&LE_CDATA_THIS->WatchdogCount,
                                                 currentWdogCount,
                                                  wdogCount),
                    "Race while initializing watchdogs."
                    "  Watchdogs should be initialized"
                    " in one thread");
    }

    // And start some watchdogs.
    MarkManyStarted(which);
    TRACE("Starting initial watchdog chain %08" PRIXWDC " (%" PRIu32 " watchdogs total)",
          LE_CDATA_THIS->WatchdogChain, LE_CDATA_THIS->WatchdogCount);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get watchdog from our watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
static WatchdogObj_t* GetWatchdogChain
(
    uint32_t watchdog         ///< Watchdog to use for monitoring
)
{
    return LE_CDATA_THIS->WatchdogList[watchdog];
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

    WatchdogObj_t* watchdogPtr = GetWatchdogChain(watchdog);

    if (watchdogPtr == NULL)
    {
        watchdogPtr = CreateNewWatchdog(watchdog);
        watchdogPtr->monitoredLoop = le_thread_GetCurrent();
    }
    else
    {
        if (watchdogPtr->shouldConnect)
        {
            le_thread_Ref_t currentThread = le_thread_GetCurrent();
            LE_FATAL_IF(watchdogPtr->monitoredLoop != currentThread,
                        "Watchdog %"PRIu32" conflict: monitoring loop %p, but attempting to monitor"
                        " loop %p", watchdog, watchdogPtr->monitoredLoop, currentThread);
        }
    }

    // Check connection
    VerifyConnection(watchdogPtr);

    // If we aren't even trying to connect (i.e. not bound to watchdog daemon), don't start
    // the timer either.
    if (watchdogPtr->shouldConnect && watchdogPtr->timer == NULL)
    {
        char timerName[8];
        snprintf(timerName, sizeof(timerName), "Chain%02" PRId32, watchdog);
        watchdogPtr->timer = le_timer_Create(timerName);
        le_timer_SetHandler(watchdogPtr->timer, CheckEventLoopHandler);
        le_timer_SetContextPtr(watchdogPtr->timer, watchdogPtr);
        le_timer_SetInterval(watchdogPtr->timer, watchdogInterval);
        le_timer_SetWakeup(watchdogPtr->timer, false);
        le_timer_Start(watchdogPtr->timer);
    }

    // Immediately kick watchdog, and schedule next kick.
    le_wdogChain_Kick(watchdog);
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
        watchdogPtr = CreateNewWatchdog(watchdog);
    }

    // Verify connection -- if not connected, just return.
    if (!VerifyConnection(watchdogPtr))
    {
        return;
    }

    LE_FATAL_IF(watchdog >= LE_CDATA_THIS->WatchdogCount, "Trying to kick out of range watchdog");

    // Kick and start the watchdog.
    TRACE("Kicking chained watchdog: %" PRIu32, watchdog);
    watchdog_t watchdogChain = MarkOneStarted(watchdog);
    CheckChain(watchdogChain);
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
    LE_FATAL_IF(watchdog >= LE_CDATA_THIS->WatchdogCount, "Trying to kick out of range watchdog");
    // Mark watchdog as stopped.
    watchdog_t watchdogChain = MarkOneStopped(watchdog);

    WatchdogObj_t* watchdogPtr = GetWatchdogChain(watchdog);
    if (watchdogPtr == NULL)
    {
        LE_INFO("Stopping already stopped watchdog");
        return;
    }

    if (watchdogPtr->timer)
    {
        le_timer_Stop(watchdogPtr->timer);
        le_timer_Delete(watchdogPtr->timer);
        watchdogPtr->timer = NULL;
    }

    if (AllAreStopped(watchdogChain))
    {
        // All watchdogs are stopped -- stop process watchdog (if allowed).  If not allowed,
        // process should not have stopped all watchdogs on the chain.
        le_wdog_Timeout(LE_WDOG_TIMEOUT_NEVER);
    }
    else
    {
        CheckChain(watchdogChain);
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

COMPONENT_INIT_ONCE
{
    WatchdogPool = le_mem_InitStaticPool(WatchdogChain, MAX_WATCHDOG_CHAINS, sizeof(WatchdogObj_t));
}

COMPONENT_INIT
{
    // Get a reference to the trace keyword that is used to control tracing in this module.
    LE_CDATA_THIS->TraceRef = le_log_GetTraceRef("wdog");
}
