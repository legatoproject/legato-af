/** @file init.c
 *
 * Legato framework library constructor implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

#include "mem.h"
#include "hashmap.h"
#include "safeRef.h"
#include "messaging.h"
#include "log.h"
#include "thread.h"
#include "signals.h"
#include "eventLoop.h"
#include "timer.h"
#include "pathIter.h"
#include "killProc.h"
#include "properties.h"
#include "json.h"
#include "pipeline.h"
#include "atomFile.h"
#include "fs.h"


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

    mem_Init();
    log_Init();        // Uses memory pools.
    sig_Init();        // Uses memory pools.
    safeRef_Init();    // Uses memory pools and hash maps.
    pathIter_Init();   // Uses memory pools and safe references.
    mutex_Init();      // Uses memory pools.
    sem_Init();        // Uses memory pools.
    thread_Init();     // Uses memory pools and safe references.
    event_Init();      // Uses thread API.
    timer_Init();      // Uses event loop.
    msg_Init();        // Uses event loop.
    kill_Init();       // Uses memory pools and timers.
    properties_Init(); // Uses memory pools and safe references.
    json_Init();       // Uses memory pools.
    pipeline_Init();   // Uses memory pools and FD Monitors.
    atomFile_Init();   // Uses memory pools.
    fs_Init();         // Uses memory pools and safe references.

    // This must be called last, because it calls several subsystems to perform the
    // thread-specific initialization for the main thread.
    thread_InitThread();
}
