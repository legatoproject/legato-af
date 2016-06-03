/** @file gdbCfg.c
 *
 * Tool used to configure an application so that gdb or strace can be used to start the
 * application's processes individually.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "limit.h"
#include "appConfig.h"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of processes that can be disabled.
 **/
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_PROCS 256
#define DEFAULT_LIMIT_MAX_FILE_SYSTEM_BYTES     131072
#define ADD_FILE_SYSTEM_BYTES                   (512 * 1024)  //512 KBytes


//--------------------------------------------------------------------------------------------------
/**
 * Debug tool node in the config.  Used to indicate the debug tool that has modified an
 * application's configuration.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_DEBUG_TOOL                  "debugTool"


//--------------------------------------------------------------------------------------------------
/**
 * Application name provided on the command line.
 **/
//--------------------------------------------------------------------------------------------------
static const char* AppName;


//--------------------------------------------------------------------------------------------------
/**
 * List of process names that have been provided on the command line.
 **/
//--------------------------------------------------------------------------------------------------
static const char* ProcNames[MAX_NUM_PROCS];


//--------------------------------------------------------------------------------------------------
/**
 * Number of process names that have been provided on the command line.
 **/
//--------------------------------------------------------------------------------------------------
static size_t NumProcs = 0;


//--------------------------------------------------------------------------------------------------
/**
 * true if the --reset option was specified on the command line.
 **/
//--------------------------------------------------------------------------------------------------
static bool DoReset = false;


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
 * Files to import for gdb.
 */
//--------------------------------------------------------------------------------------------------
const ImportObj_t GdbFilesImports[] = { {.src = "/usr/bin/gdbserver",    .dest = "/bin/"},
                                        {.src = "/lib/libdl.so.2",       .dest = "/lib/"},
                                        {.src = "/lib/libgcc_s.so.1",    .dest = "/lib/"} };

const ImportObj_t StraceFilesImports[] = { {.src = "/usr/bin/strace",    .dest = "/bin/"} };

//--------------------------------------------------------------------------------------------------
/**
 * Directories to import for gdb.
 */
//--------------------------------------------------------------------------------------------------
const ImportObj_t GdbDirsImports[] = { {.src = "/proc", .dest = "/"} };


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
 * Prints gdbCfg help to stdout.
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

    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Prints straceCfg help to stdout - which is almost the same as gdbCfg with gdbCfg replaced by
 * straceCfg... but not quite.
 */
//--------------------------------------------------------------------------------------------------
static void StracePrintHelp
(
    void
)
{
    puts(
        "NAME:\n"
        "    straceCfg - Modify an application's configuration settings to make it suitable to run\n"
        "             strace.\n"
        "\n"
        "SYNOPSIS:\n"
        "    straceCfg appName [processName ...]\n"
        "    straceCfg appName --reset\n"
        "\n"
        "DESCRIPTION:\n"
        "    straceCfg appName [processName ...].\n"
        "       Adds strace to the application's files section.  Removes the\n"
        "       specified processes from the application's procs section.\n"
        "\n"
        "    straceCfg appName --reset\n"
        "       Resets the application to its original configuration.\n"
        "\n"
        "    straceCfg --help\n"
        "        Display this help and exit.\n"
        );

    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds files or directories to be imported to the application sandbox.
 */
//--------------------------------------------------------------------------------------------------
static void AddImportFiles
(
    le_cfg_IteratorRef_t cfgIter,           ///< [IN] Iterator to the application config.
    const ImportObj_t (*importsPtr)[],      ///< [IN] Imports to include in the application.
    size_t numImports                       ///< [IN] Number of import elements.
)
{
    // Find the last node under the 'files' or 'dirs' section.
    size_t nodeNum = 0;
    char nodePath[LIMIT_MAX_PATH_BYTES];

    while (1)
    {
        int n = snprintf(nodePath, sizeof(nodePath), "%zd", nodeNum);

        INTERNAL_ERR_IF(n >= sizeof(nodePath), "Node name is too long.");
        INTERNAL_ERR_IF(n < 0, "Format error.  %m");

        if (!le_cfg_NodeExists(cfgIter, nodePath))
        {
            break;
        }

        nodeNum++;
    }

    // Start adding files/directories at the end of the current list.
    int i;
    for (i = 0; i < numImports; i++)
    {
        // Add the source.
        int n = snprintf(nodePath, sizeof(nodePath), "%zd/src", nodeNum + i);

        INTERNAL_ERR_IF(n >= sizeof(nodePath), "Node name is too long.");
        INTERNAL_ERR_IF(n < 0, "Format error.  %m");

        le_cfg_SetString(cfgIter, nodePath, (*importsPtr)[i].src);

        // Add the destination.
        n = snprintf(nodePath, sizeof(nodePath), "%zd/dest", nodeNum + i);

        INTERNAL_ERR_IF(n >= sizeof(nodePath), "Node name is too long.");
        INTERNAL_ERR_IF(n < 0, "Format error.  %m");

        le_cfg_SetString(cfgIter, nodePath, (*importsPtr)[i].dest);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if this is a temporary configuration that was previously created by this or a similar
 * tool. This function does not return if we are already configured for a debug tool.
 */
//--------------------------------------------------------------------------------------------------
static void CheckCfg
(
    le_cfg_IteratorRef_t cfgIter
)
{
    if (!le_cfg_IsEmpty(cfgIter, CFG_DEBUG_TOOL))
    {
        char debugTool[LIMIT_MAX_PATH_BYTES];

        // Don't need to check return code because the value is just informative and does not matter
        // if it is truncated.
        le_cfg_GetString(cfgIter, CFG_DEBUG_TOOL, debugTool, sizeof(debugTool), "");

        fprintf(stderr, "This application has already been configured for %s debug mode.\n", debugTool);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete procs from the config so that they won't be started when the app is started.
 * Does best effort - does not fatal but logs problems with proc names.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteProcs
(
    le_cfg_IteratorRef_t cfgIter
)
{
    int i;
    char startNode[LIMIT_MAX_PATH_BYTES] = { 0 };
    snprintf(startNode, sizeof(startNode), "/apps/%s", AppName);

    le_cfg_GoToNode(cfgIter, startNode);

    for (i = 0; i < NumProcs; i++)
    {
        char nodePath[LIMIT_MAX_PATH_BYTES];

        int n = snprintf(nodePath, sizeof(nodePath), "procs/%s", ProcNames[i]);

        INTERNAL_ERR_IF(n >= sizeof(nodePath), "Node name is too long.");
        INTERNAL_ERR_IF(n < 0, "Format error.  %m");

        le_cfg_DeleteNode(cfgIter, nodePath);
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
    void
)
{
    le_cfg_ConnectService();
    le_cfgAdmin_ConnectService();

    // Get a write iterator to the application node.
    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateWriteTxn("/apps");
    le_cfg_GoToNode(cfgIter, AppName);

    CheckCfg(cfgIter);

    // Write into the config's debug tool node to indicate that this configuration has been modified.
    le_cfg_SetString(cfgIter, CFG_DEBUG_TOOL, "gdb");

    // Add 512K to the maxFileSytemBytes so that we can debug this app in sandboxed mode
    uint32_t maxBytes;

    maxBytes = le_cfg_GetInt(cfgIter, "maxFileSystemBytes", DEFAULT_LIMIT_MAX_FILE_SYSTEM_BYTES);
    maxBytes += ADD_FILE_SYSTEM_BYTES;  // add an additional 512KBytes
    LE_INFO("Resetting maxFileSystemBytes to %d bytes", maxBytes);
    le_cfg_SetInt(cfgIter, "maxFileSystemBytes", maxBytes);

    // Add gdbserver and libs to the app's 'requires/files' section.
    le_cfg_GoToNode(cfgIter, "requires/files");
    AddImportFiles(cfgIter, &GdbFilesImports, NUM_ARRAY_MEMBERS(GdbFilesImports));

    // Add /proc to the app's dirs section.
    le_cfg_GoToParent(cfgIter);
    le_cfg_GoToNode(cfgIter, "dirs");
    AddImportFiles(cfgIter, &GdbDirsImports, NUM_ARRAY_MEMBERS(GdbDirsImports));

    DeleteProcs(cfgIter);

    le_cfg_CommitTxn(cfgIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configures the application for strace.  Adds the strace executable to the 'files' section.
 */
//--------------------------------------------------------------------------------------------------
static void ConfigureStrace
(
    void
)
{
    le_cfg_ConnectService();
    le_cfgAdmin_ConnectService();

    // Get a write iterator to the application node.
    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateWriteTxn("/apps");
    le_cfg_GoToNode(cfgIter, AppName);

    CheckCfg(cfgIter);

    // Write into the config's debug tool node to indicate that this configuration has been modified.
    le_cfg_SetString(cfgIter, CFG_DEBUG_TOOL, "strace");

    le_cfg_GoToNode(cfgIter, "requires/files");
    AddImportFiles(cfgIter, &StraceFilesImports, NUM_ARRAY_MEMBERS(StraceFilesImports));

    DeleteProcs(cfgIter);

    le_cfg_CommitTxn(cfgIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Resets the application from its gdb configuration to its original configuration.
 */
//--------------------------------------------------------------------------------------------------
static void ResetApp
(
    void
)
{
    le_cfg_ConnectService();
    le_cfgAdmin_ConnectService();

    // Get a write iterator to the application node.
    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateWriteTxn("/apps");
    le_cfg_GoToNode(cfgIter, AppName);

    // Check if this is a temporary configuration that was previously created by this or a similar
    // tool.
    if (le_cfg_IsEmpty(cfgIter, CFG_DEBUG_TOOL))
    {
        fprintf(stderr, "This application already has its original configuration.\n");
        exit(EXIT_FAILURE);
    }

    // Blow away what's in there now.
    le_cfg_GoToNode(cfgIter, "/apps");
    le_cfg_DeleteNode(cfgIter, AppName);

    le_cfg_CommitTxn(cfgIter);

    // NOTE: Currently there is a bug in the config DB where deletions and imports cannot be done in
    //       the same transaction so we must do it in two transactions.
    cfgInstall_Add(AppName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function called with the app name from the command line.
 **/
//--------------------------------------------------------------------------------------------------
static void HandleAppName
(
    const char* appName
)
{
    AppName = appName;

    // Now that we have received the only mandatory argument, we can allow less positional
    // arguments than callbacks.
    le_arg_AllowLessPositionalArgsThanCallbacks();
}


//--------------------------------------------------------------------------------------------------
/**
 * Function called with each process name from the command line.
 **/
//--------------------------------------------------------------------------------------------------
static void HandleProcessName
(
    const char* procName
)
{
    if (NumProcs >= MAX_NUM_PROCS)
    {
        fprintf(stderr, "Too many process names provided.\n");
        exit(EXIT_FAILURE);
    }

    ProcNames[NumProcs++] = procName;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the application is sandboxed.
 **/
//--------------------------------------------------------------------------------------------------
static bool IsSandboxed
(
    const char* appNamePtr
)
{
    le_cfg_ConnectService();

    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn("/apps");
    le_cfg_GoToNode(cfgIter, appNamePtr);

    bool sandboxed = le_cfg_GetBool(cfgIter, "sandboxed", true);

    le_cfg_CancelTxn(cfgIter);

    return sandboxed;
}


//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    void (*helpFn)(void) = PrintHelp;
    void (*configureFn)(void) = ConfigureGdb;

    if (0 == strcmp(le_arg_GetProgramName(), "straceCfg"))
    {
        helpFn = StracePrintHelp;
        configureFn = ConfigureStrace;
    }
    // SYNOPSIS:
    //     gdbCfg appName [processName ...]
    le_arg_AddPositionalCallback(HandleAppName);
    le_arg_AddPositionalCallback(HandleProcessName);
    le_arg_AllowMorePositionalArgsThanCallbacks();

    //     gdbCfg appName --reset
    //     Resets the application to its original configuration.
    le_arg_SetFlagVar(&DoReset, NULL, "reset");

    //     gdbCfg --help
    //         Display help and exit.
    le_arg_SetFlagCallback(helpFn, NULL, "help");

    le_arg_Scan();

    if (!IsSandboxed(AppName))
    {
        // Do nothing for non-sandboxed apps.
        exit(EXIT_SUCCESS);
    }

    if (DoReset)
    {
        if (NumProcs != 0)
        {
            fprintf(stderr, "List of processes not valid with --reset option.\n");
            exit(EXIT_FAILURE);
        }
        ResetApp();
    }
    else
    {
        configureFn();
    }

    exit(EXIT_SUCCESS);
}
