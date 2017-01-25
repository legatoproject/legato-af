//--------------------------------------------------------------------------------------------------
/** @file faultTest.c
 *
 * This program is a fault test program it's main purpose is to run and fail in different ways to
 * allow the Supervisor to monitor, detect and handle the faults.  This program must be provided
 * with the appName and the fault to perform in the command-line argument.
 *
 * Multiple instances of this program can be called with different arguments to exercise the
 * different recovery actions of the Supervisor.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#include "legato.h"

#define SIG_FAULT       "sigFault"
#define PROG_FAULT      "progFault"
#define NEVER_EXIT      "noExit"
#define NO_FAULT        "noFault"
#define FORK_CHILD      "forkChild"


COMPONENT_INIT
{
    // Get the app name.
    const char* appName = le_arg_GetArg(0);
    LE_ASSERT(appName != NULL);

    // Get the process name.
    const char* procName = le_arg_GetProgramName();
    LE_ASSERT(procName != NULL);

    LE_INFO("======== Start '%s/%s' Test ========", appName, procName);

    // Get the type of fault to perform.
    const char* faultTypeStr = le_arg_GetArg(1);
    LE_ASSERT(faultTypeStr != NULL);

    // Perform fault.
    if (strcmp(faultTypeStr, SIG_FAULT) == 0)
    {
        // sleep for 2 seconds so that we do not hit the fault limit.
        sleep(2);

        int i = *(volatile int*)0;
        LE_DEBUG("i is: %d", i);
    }
    else if (strcmp(faultTypeStr, PROG_FAULT) == 0)
    {
        // sleep for 2 seconds so that we do not hit the fault limit.
        sleep(2);

        LE_FATAL("Exiting with failure code.");
    }
    else if (strcmp(faultTypeStr, NO_FAULT) == 0)
    {
        LE_INFO("======== Test '%s/%s' Ended Normally ========", appName, procName);
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(faultTypeStr, FORK_CHILD) == 0)
    {
        // Forking child and the parent dies.  This can be used to test the handling of forked
        // children.
        pid_t pid = fork();

        if (pid == 0)
        {
            LE_INFO("Forked child.");
        }
        else
        {
            exit(EXIT_SUCCESS);
        }
    }
}
