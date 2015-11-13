/** @file init.c
 *
 * Legato framework library constructor implementation.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"

#include "args.h"
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
    int argc,       ///< Number of command-line arguments (including command name)
    char* argv[]    ///< List of command-line arguments (argv[0] is the name of the command).
)
{
    // Gather the program arguments for later processing.
    arg_SetArgs((size_t)argc, (char**)argv);

    // The order of initialization is important.  Ideally, logging would be initialized first,
    // because before that, any logging calls will report the wrong component.  However, the
    // logging uses memory pools, so memory pools must be initialized before logging.

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

    // This must be called last, because it calls several subsystems to perform the
    // thread-specific initialization for the main thread.
    thread_InitThread();
}
