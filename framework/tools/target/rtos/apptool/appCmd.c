//--------------------------------------------------------------------------------------------------
/** @file appCmd.c
 *
 * App tool for RTOS.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

#include "microSupervisor.h"

// Maximum number of arguments for "runProc"
#define MAX_ARGS_FOR_RUN_PROC   10

// Argument list for runProc, the last one is preserved for NULL
static const char *argV[MAX_ARGS_FOR_RUN_PROC + 1];

// App name for runProc
static const char* appNameStr = NULL;

// Proc name for runProc
static const char* procNameStr = NULL;

// Number of arguments for runProc
static int argC = 0;

// Counter of collected arguments
static int argsCounter = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Command-line argument handler callback for "--help".
 */
//--------------------------------------------------------------------------------------------------
static void CommandHelpHandler
(
    void
)
{
    printf("Usage: app status\n"
           "       app start appName\n"
           "       app runProc appName procName [-- <args> ]\n");
}

//--------------------------------------------------------------------------------------------------
/**
 * Command-line argument handler callback for "start" an app by the given name argument.
 */
//--------------------------------------------------------------------------------------------------
static void StartAppHandler
(
    const char* arg
)
{
    le_microSupervisor_StartApp(arg);
}

//--------------------------------------------------------------------------------------------------
/**
 * Command-line argument handler callback for the "--" arguments of "runProc".
 */
//--------------------------------------------------------------------------------------------------
static void RunProcArgsHandler
(
    const char* arg
)
{
    if (strcmp(arg, "--") != 0 &&
        argsCounter < argC)
    {
        // Collect an argument
        argV[argsCounter++] = arg;

        if (argsCounter == argC)
        {
            // Add NULL
            argV[argsCounter] = NULL;

            le_microSupervisor_RunProc(appNameStr, procNameStr, argC, argV);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Command-line argument handler callback for the proc name argument of "runProc".
 */
//--------------------------------------------------------------------------------------------------
static void RunProcProcNameHandler
(
    const char* arg
)
{
    procNameStr = arg;
    int totalArgs = le_arg_NumArgs();

    if (totalArgs == 3)
    {
        le_microSupervisor_RunProc(appNameStr, procNameStr, 0, NULL);
    }
    else if (totalArgs > 4 && strcmp(le_arg_GetArg(3), "--") == 0)
    {
        // Set the number of arguments after "--" that we need to collect
        argC = totalArgs - 4;

        // Initialize the counter of collected arguments to 0
        argsCounter = 0;

        // Allow one callback handler to deal with all rest arguments
        le_arg_AllowMorePositionalArgsThanCallbacks();

        le_arg_AddPositionalCallback(RunProcArgsHandler);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Command-line argument handler callback for the app name argument of "runProc".
 */
//--------------------------------------------------------------------------------------------------
static void RunProcAppNameHandler
(
    const char* arg
)
{
    appNameStr = arg;

    le_arg_AddPositionalCallback(RunProcProcNameHandler);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function called by command line argument scanner when the command argument is found.
 **/
//--------------------------------------------------------------------------------------------------
static void CommandArgHandler
(
    const char *command
)
{
    if (strcmp(command, "status") == 0)
    {
        le_microSupervisor_DebugAppStatus();
    }
    else if (strcmp(command, "start") == 0)
    {
        le_arg_AddPositionalCallback(StartAppHandler);
    }
    else if (strcmp(command, "runProc") == 0)
    {
        le_arg_AddPositionalCallback(RunProcAppNameHandler);
    }
    else
    {
        CommandHelpHandler();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initializer.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_arg_AddPositionalCallback(&CommandArgHandler);

    le_arg_Scan();
}
