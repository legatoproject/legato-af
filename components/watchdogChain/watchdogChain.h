//--------------------------------------------------------------------------------------------------
/**
 * @page c_wdogChain Watchdog Chain API
 *
 * @subpage watchdogChain.h "API Reference"
 *
 * <HR>
 *
 * This component provides an API for creating a watchdog chain.  The watchdog chain provides
 * an easy method to allow allows multiple tasks in a process to cooperate in kicking the
 * application's watchdog.  The watchdog will be kicked when all non-stopped tasks on the
 * chain have requested a kick.  It also provides a method to easily monitor event loops are
 * still processing events.  See @ref conceptsRuntimeArch_watchdogChain for more information.
 *
 * @section Initialization
 *
 * To initialize the watchdog chain, call @c le_wdogChain_Init() with the number of chain
 * elements to set up.  When @c le_wdogChain_Init() is used, all watchdog elements will be created
 * started.
 *
 * An alternative initialization function is @c le_wdogChain_InitSome() to set up a number of chain
 * elements, but only start a subset of the chain elements.
 *
 * In a typical system, the watchdog chain is used to monitor a fixed set of worker threads which
 * are all started on application startup.  In this case @c le_wdogChain_Init() is the preferred
 * initialization method, as starting all watchdogs on startup will allow the watchdog system to
 * detect a failure of any thread to start for any reason.
 *
 * @section Manual Watchdog Chain Control
 *
 * The watchdog chain provides a function @c le_wdogChain_Kick() to kick an element of the chain.
 * Once all chain elements are kicked, the watchdog chain will call @c le_wdog_Kick() to kick
 * kick the system watchdog.
 *
 * The watchdog chain also provides a function @c le_wdogChain_Stop() to stop monitoring a chain
 * element.  In this case the watchdog chain will not wait for this element to be kicked before
 * calling @c le_wdog_Kick().  If all elements of the chain are stopped, the chain will call
 * @c le_wdog_Stop() to stop monitoring this process.
 *
 * @section Automatic Watchdog Chain Control
 *
 * A chain element can also be kicked automatically in the event loop by calling
 * @c le_wdogChain_MonitorEventLoop().  This will set up a timer to automatically kick the watchdog
 * chain from within an event loop.
 *
 * @section Example
 *
 * A typical use of the watchdog chain is to monitor the main event loop of a process.  In
 * this code example @c MS_WDOG_INTERVAL is a timeout several times less than the watchdog
 * timeout set for the application.  This gives several opportunities to kick the watchdog
 * before a watchdog failure will be reported.
 *
 * @code
 * #include "watchdogChain.h"
 *
 * COMPONENT_INIT
 * {
 *     // Try to kick a couple of times before each timeout.
 *     le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
 *     le_wdogChain_Init(1);
 *     le_wdogChain_MonitorEventLoop(0, watchdogInterval);
 *
 *     // ... other initialization ...
 * }
 * @endcode
 */
/**
 * @file watchdogChain.h
 *
 * Legato @ref c_wdogChain include file.
 */
/*
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_WATCHDOG_CHAIN_INCLUDE_GUARD
#define LEGATO_WATCHDOG_CHAIN_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Start watchdogs 0..N-1.  Typically this is used in @c COMPONENT_INIT to start all watchdogs needed
 * by the process.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_wdogChain_Init
(
    uint32_t wdogCount
);

//--------------------------------------------------------------------------------------------------
/**
 * Start an arbitrary set of the watchdogs out of the range 0..N-1.  Typically this is used in
 * COMPONENT_INIT to start the initial watchdogs needed by the process, but defer starting others
 * until later.  Later watchdogs can be started with an explicit kick, or by starting monitoring.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_wdogChain_InitSome
(
    uint32_t wdogCount, ///< Total number of watchdogs in the chain, whether enabled or not.
    uint32_t which      ///< Bitmask indicating (by set bits) which watchdogs to immediately start.
);

//--------------------------------------------------------------------------------------------------
/**
 * Begin monitoring the event loop on the current thread.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_wdogChain_MonitorEventLoop
(
    uint32_t watchdog,          ///< Watchdog to use for monitoring
    le_clk_Time_t watchdogInterval ///< Interval at which to check event loop is functioning
);


//--------------------------------------------------------------------------------------------------

/**
 * Kick a watchdog on the chain.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_wdogChain_Kick
(
    uint32_t watchdog
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop a watchdog.
 *
 * This can also cause the chain to be completely kicked, so check it.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_wdogChain_Stop
(
    uint32_t watchdog
);

#endif /* LEGATO_WATCHDOG_CHAIN_INCLUDE_GUARD */
