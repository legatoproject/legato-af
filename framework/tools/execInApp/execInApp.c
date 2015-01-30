/** @file execInApp.c
 *
 * Tool used to execute a process in a running application's sandbox.
 *
 * The executed process will retain the standard streams of the terminal that calls this tool.  The
 * terminals environment variable will also be passed to the executed process.
 *
 * The executed process will not be monitored by the Legato Supervisor.  However, the process will
 * run as the same user as the application and thus will be killed when the application stops.
 *
 * The executable and all required libs, devices, etc. must already be in the application's sandbox
 * before the process can be started.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "limit.h"
#include "user.h"
#include "app.h"
#include "proc.h"
#include "sandbox.h"
#include "smack.h"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of arguments that can be passed to the process (including the name of the process
 * itself).
 **/
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_PROC_ARGS 255


//--------------------------------------------------------------------------------------------------
/**
 * The name of the application specified on the command line.
 **/
//--------------------------------------------------------------------------------------------------
static const char* AppName = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * The path to the executable specified on the command line.
 **/
//--------------------------------------------------------------------------------------------------
static const char* ExecPath = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * The name of the process specified on the command line.
 **/
//--------------------------------------------------------------------------------------------------
static const char* ProcName = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Array of pointers to arguments to be passed to the process when it is started.
 **/
//--------------------------------------------------------------------------------------------------
static const char* ProcArgs[MAX_NUM_PROC_ARGS + 1]; // Add space for the NULL pointer at the end.


//--------------------------------------------------------------------------------------------------
/**
 * The number of arguments in the ProcArgs array.
 **/
//--------------------------------------------------------------------------------------------------
static size_t NumProcArgs = 1;  // The first entry is reserved for the process name.


//--------------------------------------------------------------------------------------------------
/**
 * Priority level to use.
 **/
//--------------------------------------------------------------------------------------------------
static const char* Priority = "medium"; // Default to medium.


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
        "    execInApp - Executes a process in a running application's sandbox.\n"
        "\n"
        "SYNOPSIS:\n"
        "    execInApp appName [OPTIONS] execPath [ARGS]\n"
        "\n"
        "DESCRIPTION:\n"
        "    Runs the specified executable in the specified application's sandbox.\n"
        "\n"
        "    The appName is the name of the running application that the process should start in.\n"
        "    The appName cannot start with a '-'.\n"
        "\n"
        "    The execPath is the path in the sandbox to the executable file that will be executed.\n"
        "    The execPath cannot start with a '-'.\n"
        "\n"
        "    The excutable and all required libraries, resources, etc. must exist in the\n"
        "    application's sandbox.\n"
        "\n"
        "    The executed process will inherit the environment variables and file descriptors of\n"
        "    the terminal.\n"
        "\n"
        "OPTIONS:\n"
        "    --procName=NAME\n"
        "        Starts the process with NAME as its name.  If this option is not used the\n"
        "        executable name is used as the process name.\n"
        "\n"
        "    --priority=PRIORITY\n"
        "        Sets the priority of the process to PRIORITY.  PRIORITY must be either 'idle',\n"
        "        'low', 'medium', 'high', 'rt1', 'rt2'...'rt32'.\n"
        "\n"
        "    --help\n"
        "        Display this help and exit.\n"
        "\n"
        "ARGS:\n"
        "   This is a list of arguments that will be passed to the executed process.\n"
        );

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the priority level for the calling process based on the priority specified on the command
 * line.
 *
 * @note This function kills the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static void SetPriority
(
    void
)
{
    if (proc_SetPriority(Priority, 0) != LE_OK)
    {
        fprintf(stderr, "Could not set the priority level to '%s'.\n", Priority);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the user ID and primary group ID for the specified application.
 *
 * @note This function kills the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static void GetAppIds
(
    app_Ref_t appRef,               ///< [IN] App ref.
    const char* appNamePtr,         ///< [IN] App name.
    uid_t* uidPtr,                  ///< [OUT] Location to store the app's user ID.
    gid_t* gidPtr,                  ///< [OUT] Location to store the app's primary group ID.
    char* userNamePtr,              ///< [OUT] Location to store the app user name
    size_t userNameLen              ///< [IN] Size of userNamePtr
)
{

    INTERNAL_ERR_IF(user_AppNameToUserName(appNamePtr, userNamePtr, userNameLen) != LE_OK,
                    "userName buffer too small.");

    *uidPtr = app_GetUid(appRef);
    *gidPtr = app_GetGid(appRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Prepend specified env variable with the given value.
 *
 * @note This function kills the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static void PrependToEnvVariable
(
    const char* envNamePtr,         ///< [IN] Env variable name to change
    const char* valuePtr            ///< [IN] Value to prepend
)
{
    const char * oldValuePtr;
    LE_DEBUG("Var: %s", envNamePtr);

    oldValuePtr = getenv(envNamePtr);

    LE_DEBUG("Old: %s", oldValuePtr);

    if(oldValuePtr != NULL)
    {
        char newValue[LIMIT_MAX_PATH_BYTES] = "";

        INTERNAL_ERR_IF(le_path_Concat(":", newValue, sizeof(newValue), valuePtr, oldValuePtr, NULL) != LE_OK,
                "Buffer size too small.");
        LE_DEBUG("Var %s: %s", envNamePtr, newValue);

        INTERNAL_ERR_IF(setenv(envNamePtr, newValue, true) < 0, "Unable to set env");
    }
    else
    {
        INTERNAL_ERR_IF(setenv(envNamePtr, valuePtr, true) < 0, "Unable to set env");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Prepend specified env variable with the given directory, relative to a sandbox directory.
 *
 * If the directory doesn't exist, env. variable isn't changed.
 *
 * @note This function kills the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static void PrependRelativeDirToEnvVariable
(
    const char* envNamePtr,         ///< [IN] Env variable name to change
    const char* baseDirPtr,      ///< [IN] Sandbox directory
    const char* relativeDirPtr      ///< [IN] Relative path
)
{
    // Adding $sandboxDir/bin to PATH
    char absoluteDir[LIMIT_MAX_PATH_BYTES] = "";
    INTERNAL_ERR_IF(le_path_Concat("/", absoluteDir, sizeof(absoluteDir), baseDirPtr, relativeDirPtr, NULL) != LE_OK,
        "Buffer size too small.");

    if(le_dir_IsDir(absoluteDir))
    {
        PrependToEnvVariable(envNamePtr, absoluteDir);
    }
}

static app_Ref_t GetAppRef
(
    const char * appNamePtr     ///< [IN] App name
)
{
    app_Ref_t appRef;

    char configPath[LIMIT_MAX_PATH_BYTES] = "";
    INTERNAL_ERR_IF(le_path_Concat("/", configPath, LIMIT_MAX_PATH_BYTES,
            "apps", appNamePtr, (char*)NULL) != LE_OK, "Buffer size too small.");

    app_Init();

    appRef = app_Create(configPath);
    LE_FATAL_IF(appRef == NULL, "There was an error when getting app info for '%s'.", appNamePtr);

    return appRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Called when the app name is found on the command line.
 **/
//--------------------------------------------------------------------------------------------------
static void AppNameHandler
(
    const char* appName
)
{
    AppName = appName;
}



//--------------------------------------------------------------------------------------------------
/**
 * Called when the executable path is found on the command line.
 **/
//--------------------------------------------------------------------------------------------------
static void ExecPathHandler
(
    const char* execPath
)
{
    ExecPath = execPath;

    // Now that we have all the mandatory positional arguments, we can allow less args than
    // callbacks.
    le_arg_AllowLessPositionalArgsThanCallbacks();
}



//--------------------------------------------------------------------------------------------------
/**
 * Called when a argument for the process to be started is found on the command line.
 **/
//--------------------------------------------------------------------------------------------------
static void ProcArgHandler
(
    const char* arg
)
{
    if (NumProcArgs >= MAX_NUM_PROC_ARGS)
    {
        fprintf(stderr, "Too many command line arguments.\n");
        exit(EXIT_FAILURE);
    }

    ProcArgs[NumProcArgs++] = arg;
}


//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // SYNOPSIS: execInApp appName [OPTIONS] execPath [ARGS]
    le_arg_AddPositionalCallback(AppNameHandler);
    le_arg_AddPositionalCallback(ExecPathHandler);
    le_arg_AddPositionalCallback(ProcArgHandler);
    le_arg_AllowMorePositionalArgsThanCallbacks();

    // OPTIONS:
    //     --procName=NAME
    //         Starts the process with NAME as its name.  If this option is not used the
    //         executable name is used as the process name.
    le_arg_SetStringVar(&ProcName, NULL, "procName");

    //     --priority=PRIORITY
    //         Sets the priority of the process to PRIORITY.  PRIORITY must be either 'idle',
    //         'low', 'medium', 'high', 'rt1', 'rt2'...'rt32'.
    le_arg_SetStringVar(&Priority, NULL, "priority");

    //     --help
    //         Display this help and exit.
    le_arg_SetFlagCallback(PrintHelp, NULL, "help");

    le_arg_Scan();

    // If the process name wasn't specified, use the executable name as the process name.
    if (ProcName == NULL)
    {
        ProcName = le_path_GetBasenamePtr(ExecPath, "/");
    }

    le_sup_state_ConnectService();

    le_sup_state_State_t appState;
    appState = le_sup_state_GetAppState(AppName);

    switch(appState)
    {
    case LE_SUP_STATE_RUNNING:
    case LE_SUP_STATE_PAUSED:
        break;
    default:
        fprintf(stderr, "Application '%s' is not running.\n", AppName);
        exit(EXIT_FAILURE);
        break;
    }

    app_Ref_t appRef = GetAppRef(AppName);

    // The process's argument list must begin with the process name and end with a NULL.
    ProcArgs[0] = ProcName;
    ProcArgs[NumProcArgs] = NULL;

    // Get the applications info.
    uid_t uid;
    gid_t gid;
    char userName[LIMIT_MAX_USER_NAME_BYTES];
    GetAppIds(appRef, AppName, &uid, &gid, userName, sizeof(userName));
    LE_DEBUG("App: %s uid[%u] gid[%u] user[%s]\n", AppName, uid, gid, userName);

    const char * sandboxDirPtr;
    sandboxDirPtr = app_GetSandboxPath(appRef);

    // Is application sandboxed ?
    if(sandboxDirPtr[0] != '\0')
    {
        LE_DEBUG("Application '%s' is sandboxed in %s", AppName, sandboxDirPtr);

        const char * homeDirPtr = app_GetHomeDirPath(appRef);
        INTERNAL_ERR_IF(homeDirPtr == NULL, "Unable to get home directory.");

        // Set the umask so that files are not accidentally created with global permissions.
        umask(S_IRWXG | S_IRWXO);

        // Unblock all signals that might have been blocked.
        sigset_t sigSet;
        INTERNAL_ERR_IF(sigfillset(&sigSet) != 0, "Could not set signal set.");
        INTERNAL_ERR_IF(pthread_sigmask(SIG_UNBLOCK, &sigSet, NULL) != 0, "Could not set signal mask.");

        SetPriority();

        // Get the smack label for the process.
        char smackLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
        smack_GetAppLabel(AppName, smackLabel, sizeof(smackLabel));

        // Set the process's SMACK label.
        smack_SetMyLabel(smackLabel);

        // Sandbox the process.
        sandbox_ConfineProc(sandboxDirPtr, uid, gid, NULL, 0, homeDirPtr);
    }
    else
    {
        LE_WARN("Application '%s' is unsandboxed", AppName);

        const char * installDirPtr = app_GetInstallDirPath(appRef);
        INTERNAL_ERR_IF(installDirPtr == NULL, "Unable to get install directory.");

        // Adding $sandboxDir/bin to PATH
        PrependRelativeDirToEnvVariable("PATH", installDirPtr, "/bin");

        // Adding $sandboxDir/bin to LD_LIBRARY_PATH
        PrependRelativeDirToEnvVariable("LD_LIBRARY_PATH", installDirPtr, "/lib");

        if( (uid != 0) || (gid != 0) )
        {
            // Sandbox the process as to use proper uid & gid
            sandbox_ConfineProc("/", uid, gid, NULL, 0, "/");
        }
    }

    // Launch the executable program.  This should not return unless there is an error.
    LE_DEBUG("Execing '%s' in application '%s'.\n", ExecPath, AppName);
    execvp(ExecPath, (char* const *)ProcArgs);

    fprintf(stderr, "Could not exec '%s'.  %m.\n", ExecPath);
    exit(EXIT_FAILURE);
}
