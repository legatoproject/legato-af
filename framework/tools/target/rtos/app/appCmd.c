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
#define MAX_ARGS_FOR_RUN_PROC   32

// Argument list for runProc, the last one is preserved for NULL
static const char *ArgV[MAX_ARGS_FOR_RUN_PROC + 1];

// App name for runProc
static const char* AppNameStr = NULL;

// Proc name for runProc
static const char* ProcNameStr = NULL;

// Number of arguments for runProc
static int ArgC = 0;

// Has the -- argument separator been found?
static bool FoundArgSeparator = false;

// Command which to run
enum {
    CMD_UNKNOWN,
    CMD_STATUS,
    CMD_START_APP,
    CMD_START_PROC,
    CMD_ERROR
} Command;

//--------------------------------------------------------------------------------------------------
/**
 * Error handler
 */
//--------------------------------------------------------------------------------------------------
static size_t ErrorHandler
(
    size_t argIndex,          ///< [IN] Argument where the error occured
    le_result_t errorCode     ///< [IN] Detected error code
)
{
    const char * errorString;

    Command = CMD_ERROR;

    switch (errorCode)
    {
        case LE_OVERFLOW:

            errorString = "Too many arguments";
            break;

        case LE_UNDERFLOW:

            errorString = "Too few arguments";
            break;

        default:
            errorString = "Unknown error while processing arguments";
    }

    const char* programNamePtr = le_arg_GetProgramName();

    printf("* %s: at argument %" PRIuS ": %s.\n", programNamePtr, argIndex + 1,
        errorString);
    printf("Try '%s --help'.\n", programNamePtr);

    return 0;
}


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
    AppNameStr = arg;

    Command = CMD_START_APP;
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
    if (!FoundArgSeparator)
    {
        if (strcmp(arg, "--") == 0)
        {
            FoundArgSeparator = true;
        }
        else
        {
            printf("app runProc: unrecognized argument %s\n", arg);
            Command = CMD_ERROR;
        }
    }
    else if (Command != CMD_ERROR)
    {
        // Collect an argument
        ArgV[ArgC++] = arg;

        if (ArgC > MAX_ARGS_FOR_RUN_PROC)
        {
            printf("%s: too many arguments\n", ArgV[0]);
            Command = CMD_ERROR;
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
    ProcNameStr = arg;

    Command = CMD_START_PROC;

    // Initialize the counter of collected arguments to 0
    ArgC = 0;

    // Have not found separator "--" yet
    FoundArgSeparator = false;

    // Allow one callback handler to deal with all rest arguments
    le_arg_AllowMorePositionalArgsThanCallbacks();

    le_arg_AddPositionalCallback(RunProcArgsHandler);
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
    AppNameStr = arg;

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
        Command = CMD_STATUS;
    }
    else if (strcmp(command, "start") == 0)
    {
        le_arg_AddPositionalCallback(StartAppHandler);
    }
    else if (strcmp(command, "runProc") == 0)
    {
        le_arg_AddPositionalCallback(RunProcAppNameHandler);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initializer.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_arg_SetErrorHandler(&ErrorHandler);
    le_arg_AddPositionalCallback(&CommandArgHandler);

    le_arg_Scan();

    switch (Command)
    {
        case CMD_UNKNOWN:
            CommandHelpHandler();
            break;
        case CMD_ERROR:
            // Error occurred -- do nothing as error has already been reported
            break;
        case CMD_STATUS:
            le_microSupervisor_DebugAppStatus();
            break;
        case CMD_START_APP:
            le_microSupervisor_StartApp(AppNameStr);
            break;
        case CMD_START_PROC:
            ArgV[ArgC] = NULL;
            le_microSupervisor_RunProc(AppNameStr, ProcNameStr, ArgC, ArgV);
            break;
    }
    le_thread_Exit(NULL);
}
