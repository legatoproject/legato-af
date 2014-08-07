/** @file gdbCfg.c
 *
 * Tool used to configure an application so that gdb can be used to start the application's
 * processes individually.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "limit.h"
#include "configInstaller.h"


//--------------------------------------------------------------------------------------------------
/**
 * Debug tool node in the config.  Used to indicate the debug tool that has modified an
 * application's configuration.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_DEBUG_TOOL                  "debugTool"


//--------------------------------------------------------------------------------------------------
/**
 * Import object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char src[LIMIT_MAX_PATH_BYTES];
    char dest[LIMIT_MAX_PATH_BYTES];
}
ImportObj_t;


//--------------------------------------------------------------------------------------------------
/**
 * Files and directories to import for gdb.
 */
//--------------------------------------------------------------------------------------------------
const ImportObj_t GdbImports[] = { {.src = "/usr/bin/gdbserver",    .dest = "/bin/"},
                                   {.src = "/lib/libdl.so.2",       .dest = "/lib/"},
                                   {.src = "/lib/libgcc_s.so.1",    .dest = "/lib/"},
                                   {.src = "/proc",                 .dest = "/"} };


//--------------------------------------------------------------------------------------------------
/**
 * Prints a generic message on stderr so that the user is aware there is a problem, logs the
 * internal error message and exit.
 */
//--------------------------------------------------------------------------------------------------
#define INTERNAL_ERR(formatString, ...)                                                 \
            { fprintf(stderr, "Internal error check logs for details.\n");              \
              LE_FATAL(formatString, ##__VA_ARGS__); }


//--------------------------------------------------------------------------------------------------
/**
 * If the condition is true, print a generic message on stderr so that the user is aware there is a
 * problem, log the internal error message and exit.
 */
//--------------------------------------------------------------------------------------------------
#define INTERNAL_ERR_IF(condition, formatString, ...)                                   \
        if (condition) { INTERNAL_ERR(formatString, ##__VA_ARGS__); }


//--------------------------------------------------------------------------------------------------
/**
 * Prints help to stdout.
 */
//--------------------------------------------------------------------------------------------------
static void PrintHelp
(
    void
)
{
    puts(
        "NAME:\n"
        "    gdbCfg - Modify an application's configuration settings to make it suitable to run\n"
        "             gdb.\n"
        "\n"
        "SYNOPSIS:\n"
        "    gdbCfg appName [processName ...]\n"
        "    gdbCfg appName --reset\n"
        "\n"
        "DESCRIPTION:\n"
        "    gdbCfg appName [processName ...].\n"
        "       Adds gdbserver and /proc to the application's files section.  Removes the\n"
        "       specified processes from the application's procs section.\n"
        "\n"
        "    gdbCfg appName --reset\n"
        "       Resets the application to its original configuration.\n"
        "\n"
        "    gdbCfg --help\n"
        "        Display this help and exit.\n"
        );
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks the command line arguments to see if the specified option was selected.
 *
 * @return
 *      true if the option was selected on the command-line.
 *      false if the option was not selected.
 *
 * @todo
 *      This function should be replaced by a general command-line options utility.
 */
//--------------------------------------------------------------------------------------------------
static bool IsOptionSelected
(
    const char* option
)
{
    size_t numArgs = le_arg_NumArgs();
    size_t i = 0;

    char argBuf[LIMIT_MAX_ARGS_STR_BYTES];

    // Search the list of command line arguments for the specified option.
    for (; i < numArgs; i++)
    {
        INTERNAL_ERR_IF(le_arg_GetArg(i, argBuf, sizeof(argBuf)) != LE_OK,
                        "Wrong number of arguments.");

        char* subStr = strstr(argBuf, option);

        if ( (subStr != NULL) && (subStr == argBuf) )
        {
            return true;
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the application name to start the process in from the command-line.
 *
 * @note This function kills the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static void GetAppName
(
    char* bufPtr,           ///< [OUT] Buffer to store the application name in.
    size_t bufSize          ///< [IN] Buffer size.
)
{
    // The app name should be the first argument on the command-line.
    le_result_t result = le_arg_GetArg(0, bufPtr, bufSize);

    if (result == LE_NOT_FOUND)
    {
        fprintf(stderr, "Please specify an application.\n");
        exit(EXIT_FAILURE);
    }

    if (result == LE_OVERFLOW)
    {
        fprintf(stderr, "The application name is too long.\n");
        exit(EXIT_FAILURE);
    }

    if (bufPtr[0] == '-')
    {
        fprintf(stderr, "Please specify an application.  Application name cannot start with '-'.\n");
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds the files neede by gdb.
 */
//--------------------------------------------------------------------------------------------------
static void AddGdbFiles
(
    le_cfg_IteratorRef_t cfgIter            ///< Iterator to the application config.
)
{
    // Find the last node under the 'files' section.
    size_t nodeNum = 0;
    char nodePath[LIMIT_MAX_PATH_BYTES];

    while (1)
    {
        int n = snprintf(nodePath, sizeof(nodePath), "files/%zd", nodeNum);

        INTERNAL_ERR_IF(n >= sizeof(nodePath), "Node name is too long.");
        INTERNAL_ERR_IF(n < 0, "Format error.  %m");

        if (!le_cfg_NodeExists(cfgIter, nodePath))
        {
            break;
        }

        nodeNum++;
    }

    // Start adding files at the end of the current list.
    int i;
    for (i = 0; i < NUM_ARRAY_MEMBERS(GdbImports); i++)
    {
        // Add the source.
        int n = snprintf(nodePath, sizeof(nodePath), "files/%zd/src", nodeNum + i);

        INTERNAL_ERR_IF(n >= sizeof(nodePath), "Node name is too long.");
        INTERNAL_ERR_IF(n < 0, "Format error.  %m");

        le_cfg_SetString(cfgIter, nodePath, GdbImports[i].src);

        // Add the destination.
        n = snprintf(nodePath, sizeof(nodePath), "files/%zd/dest", nodeNum + i);

        INTERNAL_ERR_IF(n >= sizeof(nodePath), "Node name is too long.");
        INTERNAL_ERR_IF(n < 0, "Format error.  %m");

        le_cfg_SetString(cfgIter, nodePath, GdbImports[i].dest);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Configures the application for gdb.  Adds the gdbserver executable and required libraries to the
 * 'files' section and removes the list of processes from the 'procs' section in the config.
 */
//--------------------------------------------------------------------------------------------------
static void ConfigureGdb
(
    const char* appNamePtr,         ///< [IN] Application to configure.
    char procNames[][LIMIT_MAX_PROCESS_NAME_BYTES], ///< [IN] List of processes to remove from
                                                    ///       the app.
    size_t numProcs                 ///< [IN] Number of processes to remove from the app.
)
{
    le_cfg_StartClient("le_cfg");
    le_cfgAdmin_StartClient("le_cfgAdmin");

    // Get a write iterator to the application node.
    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateWriteTxn("/apps");
    le_cfg_GoToNode(cfgIter, appNamePtr);

    // Check if this is a temporary configuration that was previously created by this or a similar
    // tool.
    if (!le_cfg_IsEmpty(cfgIter, CFG_DEBUG_TOOL))
    {
        char debugTool[LIMIT_MAX_PATH_BYTES];

        // Don't need to check return code because the value is just informative and does not matter
        // if it is truncated.
        le_cfg_GetString(cfgIter, CFG_DEBUG_TOOL, debugTool, sizeof(debugTool), "");

        fprintf(stderr, "This application has already been configured for %s debug mode.\n", debugTool);
        exit(EXIT_FAILURE);
    }

    // Write into the config's debug tool node to indicate that this configuration has been modified.
    le_cfg_SetString(cfgIter, CFG_DEBUG_TOOL, "gdb");

    // Add gdbserver, libs and /proc to the app's files section.
    AddGdbFiles(cfgIter);

    // Delete the list of processes.
    int i;
    for (i = 0; i < numProcs; i++)
    {
        char nodePath[LIMIT_MAX_PATH_BYTES];

        int n = snprintf(nodePath, sizeof(nodePath), "procs/%s", procNames[i]);

        INTERNAL_ERR_IF(n >= sizeof(nodePath), "Node name is too long.");
        INTERNAL_ERR_IF(n < 0, "Format error.  %m");

        le_cfg_DeleteNode(cfgIter, nodePath);
    }

    le_cfg_CommitTxn(cfgIter);
}


//--------------------------------------------------------------------------------------------------
/**
 * Resets the application from its gdb configuration to its original configuration.
 */
//--------------------------------------------------------------------------------------------------
static void ResetApp
(
    const char* appNamePtr
)
{
    le_cfg_StartClient("le_cfg");
    le_cfgAdmin_StartClient("le_cfgAdmin");

    // Get a write iterator to the application node.
    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateWriteTxn("/apps");
    le_cfg_GoToNode(cfgIter, appNamePtr);

    // Check if this is a temporary configuration that was previously created by this or a similar
    // tool.
    if (le_cfg_IsEmpty(cfgIter, CFG_DEBUG_TOOL))
    {
        fprintf(stderr, "This application already has its original configuration.\n");
        exit(EXIT_FAILURE);
    }

    // Blow away what's in there now.
    le_cfg_GoToNode(cfgIter, "/apps");
    le_cfg_DeleteNode(cfgIter, appNamePtr);

    le_cfg_CommitTxn(cfgIter);

    // NOTE: Currently there is a bug in the config DB where deletions and imports cannot be done in
    //       the same transaction so we must do it in two transactions.
    cfgInstall_Add(appNamePtr);
}


COMPONENT_INIT
{
    if (IsOptionSelected("--help"))
    {
        PrintHelp();
        exit(EXIT_SUCCESS);
    }

    char appName[LIMIT_MAX_APP_NAME_BYTES];
    GetAppName(appName, sizeof(appName));

    if (IsOptionSelected("--reset"))
    {
        ResetApp(appName);
    }
    else
    {
        // Get the list of processes.
        size_t numProcs = le_arg_NumArgs() - 1;
        char processes[numProcs][LIMIT_MAX_PROCESS_NAME_BYTES];

        int i;
        for (i = 0; i < numProcs; i++)
        {
            INTERNAL_ERR_IF(le_arg_GetArg(i+1, processes[i], LIMIT_MAX_PROCESS_NAME_BYTES) != LE_OK,
                            "Could not access argument at index %d.", i+1);
        }

        ConfigureGdb(appName, processes, numProcs);
    }

    exit(EXIT_SUCCESS);
}
