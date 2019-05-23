//--------------------------------------------------------------------------------------------------
/**
 * Template main.c file for Legato executables.
 *
 * TODO: Auto-generate this thing.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"

#include "../liblegato/eventLoop.h"
#include "../liblegato/log.h"

#ifndef NO_LOG_CONTROL
#   include "../liblegato/linux/logPlatform.h"
#endif

//--------------------------------------------------------------------------------------------------
/**
 * These definitions must be done per component and probably during code generation.
 */
//--------------------------------------------------------------------------------------------------
le_log_SessionRef_t LE_LOG_SESSION;
le_log_Level_t* LE_LOG_LEVEL_FILTER_PTR;

void _le_event_InitializeComponent(void);

int main(int argc, const char* argv[])
{
    le_arg_SetArgs((size_t)argc, argv);

    LE_DEBUG("== Starting Executable '%s' ==", STRINGIZE(LE_EXECUTABLE_NAME));

    LE_LOG_SESSION = log_RegComponent( STRINGIZE(LE_COMPONENT_NAME), &LE_LOG_LEVEL_FILTER_PTR);

    // Connect to the Log Control Daemon.
    // The sooner we can connect to the Log Control Daemon, the better, because that is when
    // we obtain any non-default log settings that have been set using the interactive log
    // control tool.  However, we can't do that until we have a working IPC messaging system.
    // However, the Log Control Daemon shouldn't try to connect to itself.
    // Also, the Service Directory shouldn't try to use the messaging system, so it can't
    // connect to the Log Control Daemon either.  Besides, the Service Directory starts before
    // the Log Control Daemon starts.
#ifndef NO_LOG_CONTROL
    log_ConnectToControlDaemon();
#endif

    //@todo: Block all signals that the user intends to handle with signal events.

    // Queue up all the component initialization functions to be called by the Event Loop after
    // it processes any messages that were received from the Log Control Daemon.
    event_QueueComponentInit(_le_event_InitializeComponent);


    LE_DEBUG("== Starting Event Processing Loop ==");

    le_event_RunLoop();

    LE_FATAL("SHOULDN'T GET HERE!");
}
