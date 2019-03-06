/** @file appCtrl.c
 *
 * Control Legato applications.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "limit.h"
#include "fileDescriptor.h"
#include "user.h"
#include "cgroups.h"
#include "sysPaths.h"

/// @todo Use the appCfg component instead of reading from the config directly.


/// Pointer to application name argument from command line.
static const char* AppNamePtr = NULL;

/// Pointer to process name argument from command line.
static const char* ProcNamePtr = NULL;

/// Address of the command function to be executed.
static void (*CommandFunc)(void);


//--------------------------------------------------------------------------------------------------
/**
 * Minimum and maximum realtime priority levels.
 */
//--------------------------------------------------------------------------------------------------
#define MIN_RT_PRIORITY                         1
#define MAX_RT_PRIORITY                         32


//--------------------------------------------------------------------------------------------------
/**
 * Index of the application name on the command line if applicable.
 */
//--------------------------------------------------------------------------------------------------
#define APP_NAME_INDEX          1


//--------------------------------------------------------------------------------------------------
/**
 * The application's info file.
 */
//--------------------------------------------------------------------------------------------------
#define APP_INFO_FILE                                   "info.properties"


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
 * Type for functions that prints some information for an application.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*PrintAppFunc_t)(const char* appNamePtr);


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of threads to display.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_THREADS_TO_DISPLAY             100


//--------------------------------------------------------------------------------------------------
/**
 * Estimated maximum number of processes per app.
 */
//--------------------------------------------------------------------------------------------------
#define EST_MAX_NUM_PROC                        29


//--------------------------------------------------------------------------------------------------
/**
 * Process object used to store process information.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char procName[LIMIT_MAX_PATH_BYTES];    // The name of the process.
    pid_t procID;                           // The process ID.
    le_sls_List_t threadList;               // The list of thread in this process.
}
ProcObj_t;


//--------------------------------------------------------------------------------------------------
/**
 * Thread object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    pid_t threadID;                         // The thread ID.
    le_sls_Link_t link;                     // The link in the process's list of threads.
}
ThreadObj_t;


//--------------------------------------------------------------------------------------------------
/**
 * Process name object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char* procName;     // The name of the process.
    le_sls_Link_t link; // The link in the list of process names.
}
ProcName_t;


//--------------------------------------------------------------------------------------------------
/**
 * Process name list.
 */
//--------------------------------------------------------------------------------------------------
static le_sls_List_t ProcNameList = LE_SLS_LIST_INIT;  // Process name list.


//--------------------------------------------------------------------------------------------------
/**
 * Debug name list.
 */
//--------------------------------------------------------------------------------------------------
static le_sls_List_t DebugNameList = LE_SLS_LIST_INIT;  // Process name list.


//--------------------------------------------------------------------------------------------------
/**
 * Pool of process objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ProcObjPool;


//--------------------------------------------------------------------------------------------------
/**
 * Pool of thread objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ThreadObjPool;


//--------------------------------------------------------------------------------------------------
/**
 * Pool of process name objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ProcNamePool;


//--------------------------------------------------------------------------------------------------
/**
 * Hash map of processes in the current app.
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t ProcObjMap;


//--------------------------------------------------------------------------------------------------
/**
 * Prints help to stdout and exits.
 */
//--------------------------------------------------------------------------------------------------
static void PrintHelp
(
    void
)
{
    puts(
        "NAME:\n"
        "    app - Used to start, stop and get the status of Legato applications.\n"
        "\n"
        "SYNOPSIS:\n"
        "    app --help\n"
        "    app start <appName> [<options>]\n"
        "    app stop <appName>\n"
        "    app restart <appName>\n"
        "    app remove <appName>\n"
        "    app stopLegato\n"
        "    app restartLegato\n"
        "    app list\n"
        "    app status [<appName>]\n"
        "    app version <appName>\n"
        "    app info [<appName>]\n"
        "    app runProc <appName> <procName> [options]\n"
        "    app runProc <appName> [<procName>] --exe=<exePath> [options]\n"
        "\n"
        "DESCRIPTION:\n"
        "    app --help\n"
        "       Display this help and exit.\n"
        "\n"
        "    app start <appName>\n"
        "       Starts the specified application.\n"
        "\n"
        "    app start <appName> [<options>]\n"
        "       Runs an app in a modified manner by one or more of the following options:\n"
        "\n"
        "       --norun=<procName1>[,<procName2>,...]\n"
        "           Do not start the specified configured processes. Names are separated by commas\n"
        "           without spaces.\n"
        "       --debug=<procName1>[,<procName2>,...]\n"
        "           Start the specified process stopped, ready to attach a debugger.\n"
        "\n"
        "\n"
        "    app stop <appName>\n"
        "       Stops the specified application.\n"
        "\n"
        "    app restart <appName>\n"
        "       Restarts the specified application.\n"
        "\n"
        "    app remove <appName>\n"
        "       Removes the specified application.\n"
        "\n"
        "    app stopLegato\n"
        "       Stops the Legato framework.\n"
        "\n"
        "    app restartLegato\n"
        "       Restarts the Legato framework.\n"
        "\n"
        "    app list\n"
        "       List all installed applications.\n"
        "\n"
        "    app status [<appName>]\n"
        "       If no name is given, prints the status of all installed applications.\n"
        "       If a name is given, prints the status of the specified application.\n"
        "       The status of the application can be 'stopped', 'running', 'paused' or 'not installed'.\n"
        "\n"
        "    app version <appName>\n"
        "       Prints the version of the specified application.\n"
        "\n"
        "    app info [<appName>]\n"
        "       If no name is given, prints the information of all installed applications.\n"
        "       If a name is given, prints the information of the specified application.\n"
        "\n"
        "    app runProc <appName> <procName> [options]\n"
        "       Runs a configured process inside an app using the process settings from the\n"
        "       configuration database.  If an exePath is provided as an option then the specified\n"
        "       executable is used instead of the configured executable.\n"
        "\n"
        "    app runProc <appName> [<procName>] --exe=<exePath> [options]\n"
        "       Runs an executable inside an app.  The exePath must be provided and the optional\n"
        "       process name must not match any configured processes for the app.  Unless specified\n"
        "       using the options below the executable will be run with default settings.\n"
        "\n"
        "    app runProc takes the following options that can be used to modify the process\n"
        "    settings:\n"
        "\n"
        "       --exe=<exePath>\n"
        "           Use the executable at <exePath>.  <exePath> is from the perspective of the app\n"
        "           (ie. /exe would be at the sandbox root if the app is sandboxed).\n"
        "\n"
        "       --priority=<priorityStr>\n"
        "           Sets the priority of the process.  <priorityStr> can be either 'idle', 'low',\n"
        "           'medium', 'high', 'rt1', 'rt2', ... 'rt32'.\n"
        "\n"
        "       --faultAction=<action>\n"
        "           Sets the fault action for the process.  <action> can be either 'ignore',\n"
        "           'restartProc', 'restartApp', 'stopApp'.\n"
        "\n"
        "       -- [<args> ...]\n"
        "           The -- option is used to specify command line arguments to the process.\n"
        "           Everything following the -- option is taken as arguments to the process to be\n"
        "           started.  Therefore the -- option must be the last option to app runProc.\n"
        "           If the -- option is not used then the configured arguments are used if available.\n"
        );

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Requests the Supervisor to start an application.
 *
 * @note This function does not return.
 */
//--------------------------------------------------------------------------------------------------
static void __attribute__((noreturn)) StartApp
(
    void
)
{
    le_appCtrl_AppRef_t appRef = NULL;

    le_appCtrl_ConnectService();

    // If the --norun= option has been used to suppress the starting of a process,
    // use le_appCtrl_SetRun() to tell the Supervisor not to start those processes when
    // le_appCtrl_Start() is called later.
    // If the --debug= option has been set, use le_appCtrl_SetDebug() to tell the supervisor
    // to start those processes stopped.
    if (!le_sls_IsEmpty(&ProcNameList) || !le_sls_IsEmpty(&DebugNameList))
    {
        // Get the app ref by the app name.
        // This will return NULL if the app is not found or the app's sandbox cannot be created.
        appRef = le_appCtrl_GetRef(AppNamePtr);

        if (appRef == NULL)
        {
            fprintf(stderr,
                    "App '%s' is not installed or its container cannot be created.\n",
                    AppNamePtr);
            exit(EXIT_FAILURE);
        }

        // Set the overrides.
        le_sls_Link_t* procNameLinkPtr = le_sls_Peek(&ProcNameList);

        while (procNameLinkPtr != NULL)
        {
            ProcName_t* procNamePtr = CONTAINER_OF(procNameLinkPtr, ProcName_t, link);

            le_appCtrl_SetRun(appRef, procNamePtr->procName, false);

            procNameLinkPtr = le_sls_PeekNext(&ProcNameList, procNameLinkPtr);
        }

        // Set the overrides.
        procNameLinkPtr = le_sls_Peek(&DebugNameList);

        while (procNameLinkPtr != NULL)
        {
            ProcName_t* procNamePtr = CONTAINER_OF(procNameLinkPtr, ProcName_t, link);

            le_appCtrl_SetDebug(appRef, procNamePtr->procName, true);

            procNameLinkPtr = le_sls_PeekNext(&DebugNameList, procNameLinkPtr);
        }
    }

    // Start the application.
    le_result_t startAppResult = le_appCtrl_Start(AppNamePtr);

    // Release the app ref, if we have one.
    if (appRef != NULL)
    {
        // NOTE: Doing this has the side effect of resetting all the overrides we set for the
        //       --norun= option usage.  So, this must be done after le_appCtrl_Start().
        le_appCtrl_ReleaseRef(appRef);
    }

    // Print msg and exit based on the result.
    switch (startAppResult)
    {
        case LE_OK:
            exit(EXIT_SUCCESS);

        case LE_DUPLICATE:
            fprintf(stderr, "Application '%s' is already running.\n", AppNamePtr);
            exit(EXIT_FAILURE);

        case LE_NOT_FOUND:
            fprintf(stderr, "Application '%s' is not installed.\n", AppNamePtr);
            exit(EXIT_FAILURE);

        default:
            fprintf(stderr,
                    "There was an error.  Application '%s' could not be started.\n"
                    "Check the system log for error messages.\n",
                    AppNamePtr);
            exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Requests the Supervisor to stop an application.
 *
 * @note This function does not return.
 */
//--------------------------------------------------------------------------------------------------
static void StopApp
(
    void
)
{
    le_appCtrl_ConnectService();

    // Stop the application.
    switch (le_appCtrl_Stop(AppNamePtr))
    {
        case LE_OK:
            // If this function is not called from CommandFunc, it could be part of another
            // function call, so don't exit yet.
            if (CommandFunc == StopApp)
            {
                exit(EXIT_SUCCESS);
            }

            break;

        case LE_NOT_FOUND:
            printf("Application '%s' was not running.\n", AppNamePtr);

            if (CommandFunc == StopApp)
            {
                exit(EXIT_FAILURE);
            }

            break;

        default:
            INTERNAL_ERR("Unexpected response from the Supervisor.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Requests the Supervisor to restart an application.
 *
 * @note This function does not return.
 */
//--------------------------------------------------------------------------------------------------
static void RestartApp
(
    void
)
{
    StopApp();
    le_appCtrl_DisconnectService();
    StartApp();
}


//--------------------------------------------------------------------------------------------------
/**
 * Removes an app.
 */
//--------------------------------------------------------------------------------------------------

static void RemoveApp
(
    void
)
{
    le_cfg_ConnectService();

    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn("system:/apps");

    if (!le_cfg_NodeExists(cfgIter, AppNamePtr))
    {
        fprintf(stderr, "App '%s' is not installed.\n", AppNamePtr);
        exit(EXIT_FAILURE);
    }

    le_cfg_CancelTxn(cfgIter);

    printf("Removing app '%s'...\n", AppNamePtr);

    char systemCmd[200] = {0};

    // NOTE: update --remove will make sure the app is stopped first.
    snprintf(systemCmd, sizeof(systemCmd),
             "/legato/systems/current/bin/update --remove %s", AppNamePtr);

    if (system(systemCmd) != 0)
    {
        fprintf(stderr, "***Error: Couldn't remove app '%s'.\n", AppNamePtr);
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Requests the Supervisor to stop the Legato framework.
 *
 * @note This function does not return.
 */
//--------------------------------------------------------------------------------------------------
static void StopLegato
(
    void
)
{
    le_framework_ConnectService();

    // Stop the framework.
    le_result_t result = le_framework_Stop();
    switch (result)
    {
        case LE_OK:
            exit(EXIT_SUCCESS);

        case LE_DUPLICATE:
            printf("Legato is being stopped by someone else.\n");
            exit(EXIT_SUCCESS);

        default:
            INTERNAL_ERR("Unexpected response, %d, from the Supervisor.", result);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Requests the Supervisor to restart the Legato framework.
 *
 * @note This function does not return.
 */
//--------------------------------------------------------------------------------------------------
static void RestartLegato
(
    void
)
{
    le_framework_ConnectService();
    le_result_t result = le_framework_Restart(true);
    switch (result)
    {
        case LE_OK:
            exit(EXIT_SUCCESS);

        case LE_DUPLICATE:
            printf("Legato is being stopped by someone else.\n");
            exit(EXIT_SUCCESS);

        default:
            INTERNAL_ERR("Unexpected response, %d, from the Supervisor.", result);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints the list of installed apps.
 *
 * Iterate over the list of apps and calls the specified printFunc for each app.  If the printFunc
 * is NULL then the name of the app is printed.
 */
//--------------------------------------------------------------------------------------------------
static void ListInstalledApps
(
    PrintAppFunc_t printFunc            ///< [IN] Function to use for printing app information.
)
{
    le_cfg_ConnectService();

    if (printFunc != NULL)
    {
        le_appInfo_ConnectService();
    }

    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn("system:/apps");

    if (le_cfg_GoToFirstChild(cfgIter) == LE_NOT_FOUND)
    {
        LE_DEBUG("There are no installed apps.");
        exit(EXIT_SUCCESS);
    }

    // Iterate over the list of apps.
    do
    {
        char appName[LIMIT_MAX_APP_NAME_BYTES];

        INTERNAL_ERR_IF(le_cfg_GetNodeName(cfgIter, "", appName, sizeof(appName)) != LE_OK,
                        "Application name in config is too long.");

        if (printFunc == NULL)
        {
            printf("%s\n", appName);
        }
        else
        {
            printFunc(appName);
        }
    }
    while (le_cfg_GoToNextSibling(cfgIter) == LE_OK);
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether an installed application is running.
 *
 * @return
 *      true if running.
 *      false if stopped.
 */
//--------------------------------------------------------------------------------------------------
static bool IsAppRunning
(
    const char* appNamePtr          ///< [IN] App name to get the state for.
)
{
    le_appInfo_State_t appState;

    appState = le_appInfo_GetState(appNamePtr);

    switch (appState)
    {
        case LE_APPINFO_STOPPED:
            return false;

        case LE_APPINFO_RUNNING:
            return true;

        default:
            INTERNAL_ERR("Supervisor returned an unknown state for app '%s'.", appNamePtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints the application status.
 */
//--------------------------------------------------------------------------------------------------
static void PrintAppState
(
    const char* appNamePtr      ///< [IN] Application name to get the status for.
)
{
    le_appInfo_ConnectService();
    le_cfg_ConnectService();

    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn("system:/apps");

    if (!le_cfg_NodeExists(cfgIter, appNamePtr))
    {
        printf("[not installed] %s\n", appNamePtr);
    }
    else if (IsAppRunning(appNamePtr))
    {
        printf("[running] %s\n", appNamePtr);
    }
    else
    {
        printf("[stopped] %s\n", appNamePtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Implements the "status" command.
 *
 * @note This function does not return.
 **/
//--------------------------------------------------------------------------------------------------
static void PrintStatus
(
    void
)
{
    if (AppNamePtr == NULL)
    {
        ListInstalledApps(PrintAppState);
    }
    else
    {
        PrintAppState(AppNamePtr);
    }

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a line of the APP_INFO_FILE for display.
 *
 * @todo This is currently a dumb parse of the line string that just replaces the '=' with ': '.
 *
 * @note This function uses a static variable so is not re-entrant.
 *
 * @return
 *      Pointer to the parsed string.
 */
//--------------------------------------------------------------------------------------------------
static const char* ParsedInfoLine
(
    const char* linePtr             ///< [IN] The original line from the file.
)
{
    static char parsedStr[LIMIT_MAX_PATH_BYTES];

    size_t lineIndex = 0;
    size_t parsedIndex = 0;

    while (linePtr[lineIndex] != '\0')
    {
        if (linePtr[lineIndex] == '=')
        {
            LE_FATAL_IF((parsedIndex + 2) >= sizeof(parsedStr),
                        "'%s' is too long for buffer.", linePtr);

            parsedStr[parsedIndex++] = ':';
            parsedStr[parsedIndex++] = ' ';
        }
        else
        {
            LE_FATAL_IF((parsedIndex + 1) >= sizeof(parsedStr),
                        "'%s' is too long for buffer.", linePtr);

            parsedStr[parsedIndex++] = linePtr[lineIndex];
        }

        lineIndex++;
    }

    // Terminate the string.
    parsedStr[parsedIndex++] = '\0';

    return parsedStr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints the information in the APP_INFO_FILE file.
 */
//--------------------------------------------------------------------------------------------------
static void PrintedAppInfoFile
(
    const char* appNamePtr,         ///< [IN] App name.
    const char* prefixPtr           ///< [IN] Prefix to use when printing info lines.  This
                                    ///       prefix can be used to indent the info lines.
)
{
    // Get the path to the app's info file.
    char infoFilePath[LIMIT_MAX_PATH_BYTES] = APPS_INSTALL_DIR;
    INTERNAL_ERR_IF(le_path_Concat("/",
                                   infoFilePath,
                                   sizeof(infoFilePath),
                                   appNamePtr,
                                   APP_INFO_FILE,
                                   NULL) != LE_OK,
                    "Path to app %s's %s is too long.", appNamePtr, APP_INFO_FILE);



    // Open the info file.
    int fd;

    do
    {
        fd = open(infoFilePath, O_RDONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    if (fd == -1)
    {
        if (errno == ENOENT)
        {
            LE_WARN("No %s file for app %s.", infoFilePath, appNamePtr);
            return;
        }
        else
        {
            INTERNAL_ERR("Could not open file %s.  %m.", infoFilePath);
        }
    }

    // Read the file a line at a time and print.
    char infoLine[LIMIT_MAX_PATH_BYTES];

    while (1)
    {
        le_result_t result = fd_ReadLine(fd, infoLine, sizeof(infoLine));

        if (result == LE_OK)
        {
            // Parse the info line and print.
            printf("%s%s\n", prefixPtr, ParsedInfoLine(infoLine));
        }
        else if (result == LE_OUT_OF_RANGE)
        {
            // Nothing else to read.
            break;
        }
        else if (result == LE_OVERFLOW)
        {
            INTERNAL_ERR("Line '%s...' in file %s is too long.", infoLine, infoFilePath);
        }
        else
        {
            INTERNAL_ERR("Error reading file %s.", infoFilePath);
        }
    }

    fd_Close(fd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a process object.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteProcObj
(
    ProcObj_t* procObjPtr
)
{
    // Iterate over the list of threads and free them first
    le_sls_Link_t* threadLinkPtr = le_sls_Pop(&(procObjPtr->threadList));

    while (threadLinkPtr != NULL)
    {
        ThreadObj_t* threadPtr = CONTAINER_OF(threadLinkPtr, ThreadObj_t, link);

        le_mem_Release(threadPtr);

        threadLinkPtr = le_sls_Pop(&(procObjPtr->threadList));
    }

    // Release the proc object.
    le_mem_Release(procObjPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes all of the process objects from a hash map and frees its memory.
 */
//--------------------------------------------------------------------------------------------------
static void ClearProcHashmap
(
    le_hashmap_Ref_t procsMap
)
{
    // Iterate over the hashmap.
    le_hashmap_It_Ref_t iter = le_hashmap_GetIterator(procsMap);

    while (le_hashmap_NextNode(iter) == LE_OK)
    {
        ProcObj_t* procObjPtr = (ProcObj_t*)le_hashmap_GetValue(iter);
        le_hashmap_Remove(procsMap, le_hashmap_GetKey(iter));

        DeleteProcObj(procObjPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the PID of the process this thread belongs to.
 *
 * @return
 *      PID of the process if successful.
 *      LE_NOT_FOUND if the thread could not be found.
 */
//--------------------------------------------------------------------------------------------------
static pid_t GetThreadsProcID
(
    pid_t tid           ///< [IN] Thread ID.
)
{
#define TGID_STR            "Tgid:"

    // Get the proc file name.
    char procFile[LIMIT_MAX_PATH_BYTES];
    INTERNAL_ERR_IF(snprintf(procFile, sizeof(procFile), "/proc/%d/status", tid) >= sizeof(procFile),
                    "File name '%s...' size is too long.", procFile);

    // Open the proc file.
    int fd;

    do
    {
        fd = open(procFile, O_RDONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    if ( (fd == -1) && (errno == ENOENT ) )
    {
        return LE_NOT_FOUND;
    }

    INTERNAL_ERR_IF(fd == -1, "Could not read file %s.  %m.", procFile);

    // Read the Tgid from the file.
    while (1)
    {
        char str[200];
        le_result_t result = fd_ReadLine(fd, str, sizeof(str));

        INTERNAL_ERR_IF(result == LE_OVERFLOW, "Buffer to read PID is too small.");

        if (result == LE_OK)
        {
            if (strstr(str, TGID_STR) == str)
            {
                // Convert the Tgid string to a pid.
                char* pidStrPtr = &(str[sizeof(TGID_STR)-1]);
                pid_t pid;

                INTERNAL_ERR_IF(le_utf8_ParseInt(&pid, pidStrPtr) != LE_OK,
                                "Could not convert %s to a pid.", pidStrPtr);

                fd_Close(fd);

                return pid;
            }
        }
        else
        {
            INTERNAL_ERR("Error reading the %s", procFile);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the process name.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the process could not be found.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetProcName
(
    pid_t pid,              ///< [IN] The process ID.
    char* bufPtr,           ///< [OUT] Buffer to store the process name.
    size_t bufSize          ///< [IN] Buffer size.
)
{
    // Get the proc file name.
    char procFile[LIMIT_MAX_PATH_BYTES];
    INTERNAL_ERR_IF(snprintf(procFile, sizeof(procFile), "/proc/%d/cmdline", pid) >= sizeof(procFile),
                    "File name '%s...' size is too long.", procFile);

    // Open the proc file.
    int fd;

    do
    {
        fd = open(procFile, O_RDONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    if ( (fd == -1) && (errno == ENOENT ) )
    {
        return LE_NOT_FOUND;
    }

    INTERNAL_ERR_IF(fd == -1, "Could not read file %s.  %m.", procFile);

    // Read the name from the file.
    le_result_t result = fd_ReadLine(fd, bufPtr, bufSize);

    INTERNAL_ERR_IF(result == LE_FAULT, "Error reading the %s", procFile);

    fd_Close(fd);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Builds a process object for the specified tid and puts the object in the the specified hasmap.
 */
//--------------------------------------------------------------------------------------------------
static void BuildProcObjs
(
    le_hashmap_Ref_t procsMap,
    pid_t tid
)
{
    // Get the PID of the process this thread belongs to.
    pid_t pid = GetThreadsProcID(tid);

    if (pid == LE_NOT_FOUND)
    {
        // Thread does not exist anymore.
        return;
    }

    // Get the object for this process.
    ProcObj_t* procObjPtr = le_hashmap_Get(procsMap, &pid);

    if (procObjPtr == NULL)
    {
        // Create the object.
        procObjPtr = le_mem_ForceAlloc(ProcObjPool);

        procObjPtr->procID = pid;
        procObjPtr->threadList = LE_SLS_LIST_INIT;

        le_hashmap_Put(procsMap, &(procObjPtr->procID), procObjPtr);
    }

    // Get the name of our process.
    if (pid == tid)
    {
        if (GetProcName(pid, procObjPtr->procName, sizeof(procObjPtr->procName)) != LE_OK)
        {
            // The process no longer exists.  Delete the process object.
            le_hashmap_Remove(procsMap, &(procObjPtr->procID));
            DeleteProcObj(procObjPtr);

            return;
        }
    }

    // Create the thread object.
    ThreadObj_t* threadObjPtr = le_mem_ForceAlloc(ThreadObjPool);

    threadObjPtr->threadID = tid;
    threadObjPtr->link = LE_SLS_LINK_INIT;

    // Add this thread to the process object.
    le_sls_Queue(&(procObjPtr->threadList), &(threadObjPtr->link));
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints the list of process objects.
 */
//--------------------------------------------------------------------------------------------------
static void PrintAppObjs
(
    le_hashmap_Ref_t procsMap,      ///< [IN] Map of process objects to print.
    const char* prefixPtr           ///< [IN] Prefix to use when printing process objects.  This
                                    ///       prefix can be used to indent the list of processes.
)
{
    // Iterate over the hashmap and print the process object data.
    le_hashmap_It_Ref_t iter = le_hashmap_GetIterator(procsMap);

    while (le_hashmap_NextNode(iter) == LE_OK)
    {
        const ProcObj_t* procObjPtr = le_hashmap_GetValue(iter);

        printf("%s%s[%d] (", prefixPtr, procObjPtr->procName, procObjPtr->procID);

        // Iterate over the list of threads and print them.
        bool first = true;
        le_sls_Link_t* threadLinkPtr = le_sls_Peek(&(procObjPtr->threadList));

        while (threadLinkPtr != NULL)
        {
            ThreadObj_t* threadPtr = CONTAINER_OF(threadLinkPtr, ThreadObj_t, link);

            if (first)
            {
                printf("%d", threadPtr->threadID);

                first = false;
            }
            else
            {
                printf(", %d", threadPtr->threadID);
            }

            threadLinkPtr = le_sls_PeekNext(&(procObjPtr->threadList), threadLinkPtr);
        }

        printf(")\n");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints and application's list of running processes and their threads.
 */
//--------------------------------------------------------------------------------------------------
static void PrintAppProcs
(
    const char* appNamePtr,         ///< [IN] App name.
    const char* prefixPtr           ///< [IN] Prefix to use when printing process lines.  This
                                    ///       prefix can be used to indent the list of processes.
)
{
    // Get the list of thread IDs for this app.
    pid_t tidList[MAX_NUM_THREADS_TO_DISPLAY];

    ssize_t numAvailThreads = cgrp_GetThreadList(CGRP_SUBSYS_FREEZE,
                                                 appNamePtr,
                                                 tidList,
                                                 MAX_NUM_THREADS_TO_DISPLAY);

    if (numAvailThreads <= 0)
    {
        // No threads/processes for this app.
        return;
    }

    // Calculate the number of threads to iterate over.
    size_t numThreads = numAvailThreads;

    if (numAvailThreads > MAX_NUM_THREADS_TO_DISPLAY)
    {
        numThreads = MAX_NUM_THREADS_TO_DISPLAY;
    }

    // Iterate over the list of threads and build the process objects.
    size_t i;
    for (i = 0; i < numThreads; i++)
    {
        BuildProcObjs(ProcObjMap, tidList[i]);
    }

    // Print the process object information.
    printf("%srunning processes:\n", prefixPtr);
    PrintAppObjs(ProcObjMap, "    ");

    // Clean-up the list of process objects.
    ClearProcHashmap(ProcObjMap);

    if (numAvailThreads > numThreads)
    {
        // More threads/processes are available.
        printf("...\n");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints an installed application's info.
 */
//--------------------------------------------------------------------------------------------------
static void PrintInstalledAppInfo
(
    const char* appNamePtr
)
{
    printf("%s\n", appNamePtr);

    if (IsAppRunning(appNamePtr))
    {
        printf("  status: running\n");
        PrintAppProcs(appNamePtr, "  ");
    }
    else
    {
        printf("  status: stopped\n");
    }

    PrintedAppInfoFile(appNamePtr, "  ");

    printf("\n");
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints the application information.
 */
//--------------------------------------------------------------------------------------------------
static void PrintAppInfo
(
    const char* appNamePtr      ///< [IN] Application name to get the information for.
)
{
    le_appInfo_ConnectService();
    le_cfg_ConnectService();

    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn("system:/apps");

    if (!le_cfg_NodeExists(cfgIter, appNamePtr))
    {
        printf("[not installed] %s\n", appNamePtr);
        printf("\n");
    }
    else
    {
        PrintInstalledAppInfo(appNamePtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Implements the "info" command.
 *
 * @note This function does not return.
 **/
//--------------------------------------------------------------------------------------------------
static void PrintInfo
(
    void
)
{
    if (AppNamePtr == NULL)
    {
        ListInstalledApps(PrintAppInfo);
    }
    else
    {
        PrintAppInfo(AppNamePtr);
    }

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Implements the "list" command.
 *
 * @note This function does not return.
 **/
//--------------------------------------------------------------------------------------------------
static void ListApps
(
    void
)
{
    ListInstalledApps(NULL);

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints the application version.
 *
 * @note This function does not return.
 */
//--------------------------------------------------------------------------------------------------
static void PrintAppVersion
(
    void
)
{
    le_cfg_ConnectService();

    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn("system:/apps");
    le_cfg_GoToNode(cfgIter, AppNamePtr);

    if (!le_cfg_NodeExists(cfgIter, ""))
    {
        printf("%s is not installed.\n", AppNamePtr);
    }
    else
    {
        char version[LIMIT_MAX_PATH_BYTES];

        le_result_t result = le_cfg_GetString(cfgIter, "version", version, sizeof(version), "");

        if (strcmp(version, "") == 0)
        {
            printf("%s has no version\n", AppNamePtr);
        }
        else if (result == LE_OK)
        {
            printf("%s %s\n", AppNamePtr, version);
        }
        else
        {
            LE_WARN("Version string for app %s is too long.", AppNamePtr);
            printf("%s %s...\n", AppNamePtr, version);
        }
    }

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * A handler that is called when the application process exits.
 */
//--------------------------------------------------------------------------------------------------
static void AppProcStopped
(
    int32_t exitCode,           ///< [IN] Exit code of the process.
    void* contextPtr            ///< [IN] Context of the process.
)
{
    if (WIFEXITED(exitCode))
    {
        exit(WEXITSTATUS(exitCode));
    }

    if (WIFSIGNALED(exitCode))
    {
        fprintf(stderr, "Proc terminated by signal %s.\n", strsignal(WTERMSIG(exitCode)));
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Proc exited for unknown reason, exit code: %d.\n", exitCode);
    exit(EXIT_FAILURE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the app name from the command line.
 *
 * @note
 *      Kills the calling process if there is an error.
 *
 * @return
 *      Pointer to the app name.
 */
//--------------------------------------------------------------------------------------------------
static const char* GetCmdLineAppName
(
    void
)
{
    const char* appNamePtr = le_arg_GetArg(APP_NAME_INDEX);

    if (appNamePtr == NULL)
    {
        fprintf(stderr, "Please provide application name.\n");
        exit(EXIT_FAILURE);
    }

    if ( (appNamePtr[0] == '-') || (strstr(appNamePtr, "/") != NULL) )
    {
        fprintf(stderr, "Invalid application name.\n");
        exit(EXIT_FAILURE);
    }

    return appNamePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the process name from the command line.
 *
 * @note
 *      Kills the calling process if there is an error.
 *
 * @return
 *      Pointer to the process name if available.
 *      An empty string if the process name is not provided.
 */
//--------------------------------------------------------------------------------------------------
static const char* GetCmdLineProcName
(
    int* incrementalArgsUsedPtr         ///< [OUT] Increments this value by the number of args found
                                        ///        in this function.
)
{
    const char* procNamePtr = le_arg_GetArg(APP_NAME_INDEX + 1);

    if ( (procNamePtr == NULL) || (procNamePtr[0] == '-') )
    {
        return "";
    }

    if (strstr(procNamePtr, "/") != NULL)
    {
        fprintf(stderr, "Invalid process name.\n");
        exit(EXIT_FAILURE);
    }

    (*incrementalArgsUsedPtr)++;

    return procNamePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the arguments list (from the command line) for the process we are going to start in an
 * runProc command.  This list of arguments is anything following the '--' option.
 *
 * @note
 *      Kills the calling process if there is an error.
 *
 * @return
 *      The number of process arguments.
 *      -1 if the '--' option is not provided or if no arguments was provided after --.
 */
//--------------------------------------------------------------------------------------------------
static int GetProcessArgs
(
    const char* argsPtr[LIMIT_MAX_NUM_CMD_LINE_ARGS],   ///< [OUT] An array of pointers that will
                                                        ///        point to process arguments.
    int* argsStartIndexPtr,             ///< [OUT] Index of the '--' option on our command line.
    int* incrementalArgsUsedPtr         ///< [OUT] Increments this value by the number of args found
                                        ///        in this function.
)
{
    // Search for the first '--' option starting after the app name.
    *argsStartIndexPtr = -1;
    int i = APP_NAME_INDEX + 1;
    for (; i < le_arg_NumArgs(); i++)
    {
        if (strcmp(le_arg_GetArg(i), "--") == 0)
        {
            *argsStartIndexPtr = i;
            break;
        }
    }

    if (*argsStartIndexPtr == -1)
    {
        // No process args.
        return -1;
    }

    // Get the process args.
    i = *argsStartIndexPtr + 1;
    int j = 0;
    while (j < LIMIT_MAX_NUM_CMD_LINE_ARGS)
    {
        argsPtr[j] = le_arg_GetArg(i);

        if (argsPtr[j] == NULL)
        {
            break;
        }

        j++;
        i++;
    }

    *incrementalArgsUsedPtr += j + 1; // Include the '--'.

    if (j == 0)
    {
        // No args passed after --
        return -1;
    }

    if (j >= LIMIT_MAX_NUM_CMD_LINE_ARGS)
    {
        fprintf(stderr, "Too many process arguments.");
        exit(EXIT_FAILURE);
    }

    return j;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a string option value from the command line searching only up to lastValidIndex.
 *
 * @note
 *      Kills the calling process if there is an error.
 *
 * @return
 *      Pointer to the value string if successful.
 *      NULL if the option was not found.
 */
//--------------------------------------------------------------------------------------------------
static const char* GetStringOption
(
    const char* optionStr,              ///< [IN] Option string to get the value for.
    int lastValidIndex,                 ///< [IN] Index of last argument to search.
    int* incrementalArgsUsedPtr         ///< [OUT] Increments this value by the number of args found
                                        ///        in this function.
)
{
    int optionLen = strlen(optionStr);

    // Start searching after the app name.
    int i = APP_NAME_INDEX + 1;

    while (1)
    {
        if ( (lastValidIndex >= 0) && (i > lastValidIndex) )
        {
            // Reached the end of the list of valid args.
            break;
        }

        const char* argPtr = le_arg_GetArg(i);

        if (argPtr == NULL)
        {
            // No more args.
            break;
        }

        if ( (strncmp(argPtr, optionStr, optionLen) == 0) && (argPtr[optionLen] == '=') )
        {
            // Found option.
            const char* valuePtr = &(argPtr[optionLen + 1]);

            if (valuePtr[0] == '\0')
            {
                // Empty value.
                fprintf(stderr, "Missing value for %s.\n", optionStr);
                exit(EXIT_FAILURE);
            }

            (*incrementalArgsUsedPtr)++;

            return valuePtr;
        }

        i++;
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the executable from the command line.
 *
 * @note
 *      Kills the calling process if there is an error.
 *
 * @return
 *      The executable path if successful.
 *      An empty string if an executable was not specified on the command line.
 */
//--------------------------------------------------------------------------------------------------
static const char* GetCmdLineExe
(
    int lastValidIndex,                 ///< [IN] Index of last argument to search.
    int* incrementalArgsUsedPtr         ///< [OUT] Increments this value by the number of args found
                                        ///        in this function.
)
{
    const char* exePathPtr = GetStringOption("--exe", lastValidIndex, incrementalArgsUsedPtr);

    if (exePathPtr == NULL)
    {
        return "";
    }

    return exePathPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the priority from the command line.
 *
 * @note
 *      Kills the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static const char* GetCmdLinePriority
(
    int lastValidIndex,                 ///< [IN] Index of last argument to search.
    int* incrementalArgsUsedPtr         ///< [OUT] Increments this value by the number of args found
                                        ///        in this function.
)
{
    const char* priorityPtr = GetStringOption("--priority", lastValidIndex, incrementalArgsUsedPtr);

    if (priorityPtr == NULL)
    {
        return NULL;
    }

    // Check if the priority string is valid.
    if ( (strcmp(priorityPtr, "idle") == 0) ||
         (strcmp(priorityPtr, "low") == 0) ||
         (strcmp(priorityPtr, "medium") == 0) ||
         (strcmp(priorityPtr, "high") == 0) )
    {
        return priorityPtr;
    }

    if ( (priorityPtr[0] == 'r') && (priorityPtr[1] == 't') )
    {
        int rtLevel;

        if ( (le_utf8_ParseInt(&rtLevel, &(priorityPtr[2])) == LE_OK) &&
             (rtLevel >= MIN_RT_PRIORITY) &&
             (rtLevel <= MAX_RT_PRIORITY) )
        {
            return priorityPtr;
        }
    }

    fprintf(stderr, "Invalid priority.  Try --help.\n");
    exit(EXIT_FAILURE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the fault action from the command line.
 *
 * @note
 *      Kills the calling process if there is an error.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if no fault action specified.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetCmdLineFaultAction
(
    int lastValidIndex,                 ///< [IN] Index of last argument to search.
    le_appProc_FaultAction_t* faultPtr, ///< [OUT] The fault action.
    int* incrementalArgsUsedPtr         ///< [OUT] Increments this value by the number of args found
                                        ///        in this function.
)
{
    const char* faultActionPtr = GetStringOption("--faultAction", lastValidIndex, incrementalArgsUsedPtr);

    if (faultActionPtr == NULL)
    {
        return LE_NOT_FOUND;
    }

    if (strcmp(faultActionPtr, "ignore") == 0)
    {
        *faultPtr = LE_APPPROC_FAULT_ACTION_IGNORE;
        return LE_OK;
    }

    if (strcmp(faultActionPtr, "restartProc") == 0)
    {
        *faultPtr = LE_APPPROC_FAULT_ACTION_RESTART_PROC;
        return LE_OK;
    }

    if (strcmp(faultActionPtr, "restartApp") == 0)
    {
        *faultPtr = LE_APPPROC_FAULT_ACTION_RESTART_APP;
        return LE_OK;
    }

    if (strcmp(faultActionPtr, "stopApp") == 0)
    {
        *faultPtr = LE_APPPROC_FAULT_ACTION_STOP_APP;
        return LE_OK;
    }

    fprintf(stderr, "Invalid fault action.  Try --help.\n");
    exit(EXIT_FAILURE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Runs a process in an application.
 */
//--------------------------------------------------------------------------------------------------
static void RunProc
(
    void
)
{
    // Keep a counter of the number of useful arguments so we can do a check at the end.  Start off
    // with the runProc command and the app name as these are mandatory.
    int numUsefulArgs = 2;

    // Get app name.
    AppNamePtr = GetCmdLineAppName();

    // Get proc name (optional).
    ProcNamePtr = GetCmdLineProcName(&numUsefulArgs);

    // Get arguments for the process we are going to start.
    const char* procArgs[LIMIT_MAX_NUM_CMD_LINE_ARGS];
    int lastCmdIndex = -1;

    int numProcArgs = GetProcessArgs(procArgs, &lastCmdIndex, &numUsefulArgs);

    if (lastCmdIndex > 0)
    {
        // This is the index of the last valid command line argument for "app runProc".  All
        // proceeding arguments belong to the process to start.
        lastCmdIndex--;
    }

    // Get options.
    const char* exePathPtr = GetCmdLineExe(lastCmdIndex, &numUsefulArgs);
    const char* priorityPtr = GetCmdLinePriority(lastCmdIndex, &numUsefulArgs);
    le_appProc_FaultAction_t faultAction;
    le_result_t faultActionResult = GetCmdLineFaultAction(lastCmdIndex, &faultAction, &numUsefulArgs);

    // Check for extra options.
    if (numUsefulArgs != le_arg_NumArgs())
    {
        fprintf(stderr, "Invalid arguments.  Try --help.\n");
        exit(EXIT_FAILURE);
    }

    // Check if options are valid.
    if ( (strcmp(ProcNamePtr, "") == 0) && (strcmp(exePathPtr, "") == 0) )
    {
        fprintf(stderr, "Please provide a process name or an executable path or both.  Try --help.\n");
        exit(EXIT_FAILURE);
    }

    // Connect to the app proc service.
    le_appProc_ConnectService();

    // Create and configure our application process.
    le_appProc_RefRef_t appProcRef = le_appProc_Create(AppNamePtr, ProcNamePtr, exePathPtr);

    if (appProcRef == NULL)
    {
        fprintf(stderr, "Failed to create proc %s in app %s.\n", ProcNamePtr, AppNamePtr);
        fprintf(stderr, "Check logs for details.\n");
        exit(EXIT_FAILURE);
    }

    // Setup the standard streams.
    le_appProc_SetStdIn(appProcRef, STDIN_FILENO);
    le_appProc_SetStdOut(appProcRef, STDOUT_FILENO);
    le_appProc_SetStdErr(appProcRef, STDERR_FILENO);

    // Set args.
    if (numProcArgs == 0)
    {
        le_appProc_AddArg(appProcRef, "");
    }
    else if (numProcArgs > 0)
    {
        int i = 0;
        for (; i < numProcArgs; i++)
        {
            le_appProc_AddArg(appProcRef, procArgs[i]);
        }
    }

    // Set priority.
    if (priorityPtr != NULL)
    {
        le_appProc_SetPriority(appProcRef, priorityPtr);
    }

    // Set fault action.
    if (faultActionResult == LE_OK)
    {
        le_appProc_SetFaultAction(appProcRef, faultAction);
    }

    // Add our process stop handler.
    // NOTE: To hold the standard in we must not exit and continue to run in the foreground.
    //       program execution will be handled in the stop handler.
    le_appProc_AddStopHandler(appProcRef, AppProcStopped, NULL);

    // Start the process.
    le_result_t result = le_appProc_Start(appProcRef);

    if (result != LE_OK)
    {
        fprintf(stderr, "Failed to start proc %s in app %s.\n", ProcNamePtr, AppNamePtr);
        fprintf(stderr, "Check logs for details.\n");
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called by le_arg_Scan() when it encounters an application name argument on the
 * command line.
 **/
//--------------------------------------------------------------------------------------------------
static void AppNameArgHandler
(
    const char* appNamePtr
)
{
    AppNamePtr = appNamePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler to get the process names specified by the "--norun" option of the startMod command. The
 * process names are placed in a linked list.
 **/
//--------------------------------------------------------------------------------------------------
static void NoRunProcNameArgHandler
(
    const char* noRunProcNamesPtr   ///< [IN] string containing proc names
)
{
    char* procNames = strdup(noRunProcNamesPtr);
    LE_ASSERT(procNames != NULL);

    char delim[2] = ",";
    char* token;

    token = strtok(procNames, delim);

    while (token != NULL)
    {
        ProcName_t* procNamePtr = le_mem_ForceAlloc(ProcNamePool);

        procNamePtr->procName = strdup(token);
        procNamePtr->link = LE_SLS_LINK_INIT;

        le_sls_Queue(&ProcNameList, &(procNamePtr->link));

        token = strtok(NULL, delim);
    }

    free(procNames);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler to get the process names specified by the "--debug" option of the startMod command. The
 * process names are placed in a linked list.
 **/
//--------------------------------------------------------------------------------------------------
static void DebugProcNameArgHandler
(
    const char* debugProcNamesPtr   ///< [IN] string containing proc names
)
{
    char* procNames = strdup(debugProcNamesPtr);
    LE_ASSERT(procNames != NULL);

    char delim[2] = ",";
    char* token;

    token = strtok(procNames, delim);

    while (token != NULL)
    {
        ProcName_t* procNamePtr = le_mem_ForceAlloc(ProcNamePool);

        procNamePtr->procName = strdup(token);
        procNamePtr->link = LE_SLS_LINK_INIT;

        le_sls_Queue(&DebugNameList, &(procNamePtr->link));

        token = strtok(NULL, delim);
    }

    free(procNames);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called by le_arg_Scan() when it encounters the command argument on the
 * command line.
 **/
//--------------------------------------------------------------------------------------------------
static void CommandArgHandler
(
    const char* command
)
{
    if (strcmp(command, "help") == 0)
    {
        PrintHelp();    // Doesn't return;
    }

    if (strcmp(command, "start") == 0)
    {
        CommandFunc = StartApp;

        le_arg_AddPositionalCallback(AppNameArgHandler);
        le_arg_SetStringCallback(NoRunProcNameArgHandler, NULL, "norun");
        le_arg_SetStringCallback(DebugProcNameArgHandler, NULL, "debug");
    }
    else if (strcmp(command, "stop") == 0)
    {
        CommandFunc = StopApp;

        le_arg_AddPositionalCallback(AppNameArgHandler);
    }
    else if (strcmp(command, "restart") == 0)
    {
        CommandFunc = RestartApp;

        le_arg_AddPositionalCallback(AppNameArgHandler);
    }
    else if (strcmp(command, "remove") == 0)
    {
        CommandFunc = RemoveApp;

        le_arg_AddPositionalCallback(AppNameArgHandler);
    }
    else if (strcmp(command, "stopLegato") == 0)
    {
        CommandFunc = StopLegato;
    }
    else if (strcmp(command, "restartLegato") == 0)
    {
        CommandFunc = RestartLegato;
    }
    else if (strcmp(command, "list") == 0)
    {
        CommandFunc = ListApps;
    }
    else if (strcmp(command, "status") == 0)
    {
        CommandFunc = PrintStatus;

        // Accept an optional app name argument.
        le_arg_AddPositionalCallback(AppNameArgHandler);
        le_arg_AllowLessPositionalArgsThanCallbacks();
    }
    else if (strcmp(command, "version") == 0)
    {
        CommandFunc = PrintAppVersion;

        le_arg_AddPositionalCallback(AppNameArgHandler);
    }
    else if (strcmp(command, "info") == 0)
    {
        CommandFunc = PrintInfo;

        // Accept an optional app name argument.
        le_arg_AddPositionalCallback(AppNameArgHandler);
        le_arg_AllowLessPositionalArgsThanCallbacks();
    }
    else
    {
        fprintf(stderr, "Unknown command '%s'.  Try --help.\n", command);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    ProcObjPool = le_mem_CreatePool("ProcObjPool", sizeof(ProcObj_t));
    ThreadObjPool = le_mem_CreatePool("ThreadObjPool", sizeof(ThreadObj_t));
    ProcNamePool = le_mem_CreatePool("ProcNamePool", sizeof(ProcName_t));

    ProcObjMap = le_hashmap_Create("ProcsMap",
                                   EST_MAX_NUM_PROC,
                                   le_hashmap_HashUInt32,
                                   le_hashmap_EqualsUInt32);

    // Parse arguments.
    if ( (le_arg_NumArgs() >= 2) && (strcmp(le_arg_GetArg(0), "runProc") == 0) )
    {
        // For the runProc parse the options manually because the automatic parser does not handle
        // all the options we need.

        RunProc();
    }
    else
    {
        le_arg_SetFlagCallback(PrintHelp, "h", "help");

        le_arg_AddPositionalCallback(CommandArgHandler);

        le_arg_Scan();

        CommandFunc();
    }
}
