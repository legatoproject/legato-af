/** @file init.c
 *
 * Legato framework library constructor implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

#include "args.h"
#include "atomFile.h"
#include "eventLoop.h"
#include "fs.h"
#include "json.h"
#include "killProc.h"
#include "log.h"
#include "mem.h"
#include "messaging.h"
#include "pathIter.h"
#include "pipeline.h"
#include "properties.h"
#include "rand.h"
#include "safeRef.h"
#include "signals.h"
#include "test.h"
#include "thread.h"
#include "timer.h"


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Legato framework library.
 *
 * The linker/loader will automatically run this function when the library is loaded into a
 * process's address space at runtime.
 *
 * It initializes all the individual module in the framework in the correct order.
 *
 * @note
 *      On failure, the process exits.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((constructor)) void _legato_InitFramework
(
    void
)
{
    // The order of initialization is important.  Ideally, logging would be initialized first,
    // because before that, any logging calls will report the wrong component, and pretty much
    // everything uses logging.  However, the logging uses memory pools, so memory pools must be
    // initialized before logging.  Fortunately, most logging macros work even if log_Init()
    // hasn't been called yet.  Keep it that way.  Also, be careful when using logging inside
    // the memory pool module, because there is the risk of creating infinite recursion.

    rand_Init();        // Does not use any other resource.  Initialize first so that randomness is
                        // available for other modules' initialization.
    mem_Init();         // Many things rely on memory pools, so initialize them as soon as possible.
    log_Init();         // Uses memory pools.
    sig_Init();         // Uses memory pools.
    safeRef_Init();     // Uses memory pools and hash maps.
    pathIter_Init();    // Uses memory pools and safe references.
    mutex_Init();       // Uses memory pools.
    sem_Init();         // Uses memory pools.
    event_Init();       // Uses memory pools.
    timer_Init();       // Uses event loop.
    thread_Init();      // Uses event loop, memory pools and safe references.
    arg_Init();         // Uses memory pools.
    msg_Init();         // Uses event loop.
    kill_Init();        // Uses memory pools and timers.
    properties_Init();  // Uses memory pools and safe references.
    json_Init();        // Uses memory pools.
    pipeline_Init();    // Uses memory pools and FD Monitors.
    atomFile_Init();    // Uses memory pools.
    fs_Init();          // Uses memory pools and safe references.
    test_Init();        // Initialize test infrastructure last.

    // This must be called last, because it calls several subsystems to perform the
    // thread-specific initialization for the main thread.
    thread_InitThread();
}


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Legato framework library.
 *
 * Applications should call this function explicitly when liblegato is linked statically.
 *
 * @note
 *    The constructor initalizes the library. Application need to call this function only
 *    to not get optimized out.
 */
//--------------------------------------------------------------------------------------------------
void InitFramework
(
    void
)
{
    // Do nothing
}
