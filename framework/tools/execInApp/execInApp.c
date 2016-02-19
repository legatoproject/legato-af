//--------------------------------------------------------------------------------------------------
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
#define MAX_NUM_PROC_ARGS           255


//--------------------------------------------------------------------------------------------------
/**
 * Default priority level.
 **/
//--------------------------------------------------------------------------------------------------
#define DEFAULT_PRIORITY            "medium"


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
 * Sets the priority level for the calling process.
 *
 * @note This function kills the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static void SetPriority
(
    const char* priorityPtr         ///< [IN] Priority to set the process to.  If this is NULL the
                                    ///       default priority will be used.
)
{
    const char* priorityStr = DEFAULT_PRIORITY;

    if (priorityPtr != NULL)
    {
        priorityStr = priorityPtr;
    }

    INTERNAL_ERR_IF(proc_SetPriority(priorityStr, 0) != LE_OK,
                    "Could not set the priority level to '%s'.\n", priorityStr);
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
 * Gets the application name from the command-line.
 *
 * @note Does not return on error.
 *
 * @return
 *      The app name.
 */
//--------------------------------------------------------------------------------------------------
static const char* GetAppName
(
    void
)
{
    // The app name should be the first argument on the command-line.
    const char* appNamePtr = le_arg_GetArg(0);

    if (appNamePtr == NULL)
    {
        fprintf(stderr, "Please specify an application.\n");
        exit(EXIT_FAILURE);
    }

    if ( (strcmp(appNamePtr, "--help") == 0) || (strcmp(appNamePtr, "-h") == 0) )
    {
        PrintHelp();
    }

    if (appNamePtr[0] == '-')
    {
        fprintf(stderr, "Please specify an application.  Application name cannot start with '-'.\n");
        exit(EXIT_FAILURE);
    }

    return appNamePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the executable path from the command-line.
 *
 * @note Does not return on error.
 *
 * @return
 *      The executable path.
 */
//--------------------------------------------------------------------------------------------------
static const char* GetExecPath
(
    size_t* indexPtr            ///< [OUT] Index of the exec path argument on the command line.
)
{
    // The executable path is the first argument after the list of options.  Search for the exec
    // path starting from the second argument.
    size_t i = 1;

    while (1)
    {
        const char* argPtr = le_arg_GetArg(i);

        if (argPtr == NULL)
        {
            fprintf(stderr, "Please specify an executable.\n");
            exit(EXIT_FAILURE);
        }

        if (argPtr[0] != '-')
        {
            // This is the executable path.
            *indexPtr = i;
            return argPtr;
        }

        i++;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Returns the string value of a command-line option.  The command-line option must take the form:
 *
 * option=value
 *
 * The portion before the '=' is considered the option.  The portion after the '=' is considered the
 * value.
 *
 * For example with,
 *
 * --priority=low
 *
 * the option would be the string "--priority" and the value would be the string "low".
 *
 * @return
 *      The string value if successful.
 *      Null if the option was not found on the command line.
 */
//--------------------------------------------------------------------------------------------------
static const char* GetOptionValue
(
    const char* option,     ///< [IN] The option.
    size_t startIndex,      ///< [IN] Index of the first argument to search from.
    size_t endIndex         ///< [IN] Index of the last argument to search to.
)
{
    // Make the string to search for.
    char searchStr[LIMIT_MAX_ARGS_STR_BYTES];

    int n = snprintf(searchStr, sizeof(searchStr), "%s=", option);

    if ( (n < 0) || (n >= sizeof(searchStr)) )
    {
        INTERNAL_ERR("Option string is too long.");
    }

    // Search the list of command line arguments for the specified option.
    size_t i = startIndex;

    for (; i <= endIndex; i++)
    {
        const char* argPtr = le_arg_GetArg(i);

        INTERNAL_ERR_IF(argPtr == NULL, "Wrong number of arguments.");

        char* subStr = strstr(argPtr, searchStr);

        if ( (subStr != NULL) && (subStr == argPtr) )
        {
            // The argPtr begins with the searchStr.  The remainder of the argBuf is the value string.
            const char* valueStr = argPtr + strlen(searchStr);

            return valueStr;
        }
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Returns true if the specified flag is found on the command line.
 *
 * @return
 *      true if the flag is on the command line.
 *      false if the flag was not found on the command line.
 */
//--------------------------------------------------------------------------------------------------
static bool GetFlagArg
(
    const char* flag,       ///< [IN] The flag to find
    size_t startIndex,      ///< [IN] Index of the first argument to search from.
    size_t endIndex         ///< [IN] Index of the last argument to search to.
)
{
    // Search the list of command line arguments for the specified option.
    size_t i = startIndex;

    for (; i <= endIndex; i++)
    {
        const char* argPtr = le_arg_GetArg(i);

        INTERNAL_ERR_IF(argPtr == NULL, "Wrong number of arguments.");

        if (strcmp(argPtr, flag) == 0)
        {
            return true;
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Need to parse the command line args in order because the argument order have specific meanings.
    const char* appNamePtr = GetAppName();

    // Get the executable path.
    size_t execIndex;
    const char* execPathPtr = GetExecPath(&execIndex);

    // Check options.
    size_t lastOptionIndex = execIndex - 1;
    size_t firstOptionIndex = 1;
    if ( GetFlagArg("--help", firstOptionIndex, lastOptionIndex) ||
         GetFlagArg("-h", firstOptionIndex, lastOptionIndex) )
    {
        PrintHelp();
    }

    const char* procNamePtr = GetOptionValue("--procName", firstOptionIndex, lastOptionIndex);
    const char* priorityPtr = GetOptionValue("--priority", firstOptionIndex, lastOptionIndex);

    // Get all of the arguments for the process.  The first element in this list stores the process
    // name.  The last element must be NULL to indicate the end of the list.
    const char* procArgs[MAX_NUM_PROC_ARGS + 2] = {NULL};

    int i = 1;  // The first element is for the process name.
    while (i < MAX_NUM_PROC_ARGS)
    {
        // Only include command line arguments after the executable.
        procArgs[i] = le_arg_GetArg(i + execIndex);

        if (procArgs[i] == NULL)
        {
            break;
        }

        i++;
    }

    // If the process name wasn't specified, use the executable name as the process name.
    if (procNamePtr == NULL)
    {
        procArgs[0] = le_path_GetBasenamePtr(execPathPtr, "/");
    }

    // Make sure the app is running.
    le_appInfo_ConnectService();

    le_appInfo_State_t appState;
    appState = le_appInfo_GetState(appNamePtr);

    switch(appState)
    {
        case LE_APPINFO_RUNNING:
            break;

        default:
            fprintf(stderr, "Application '%s' is not running.\n", appNamePtr);
            exit(EXIT_FAILURE);
            break;
    }

    app_Ref_t appRef = GetAppRef(appNamePtr);

    // Get the applications info.
    uid_t uid;
    gid_t gid;
    char userName[LIMIT_MAX_USER_NAME_BYTES];
    GetAppIds(appRef, appNamePtr, &uid, &gid, userName, sizeof(userName));
    LE_DEBUG("App: %s uid[%u] gid[%u] user[%s]\n", appNamePtr, uid, gid, userName);

    const char * sandboxDirPtr;
    sandboxDirPtr = app_GetSandboxPath(appRef);

    // Is application sandboxed ?
    if(sandboxDirPtr[0] != '\0')
    {
        LE_DEBUG("Application '%s' is sandboxed in %s", appNamePtr, sandboxDirPtr);

        // Set the umask so that files are not accidentally created with global permissions.
        umask(S_IRWXG | S_IRWXO);

        // Unblock all signals that might have been blocked.
        sigset_t sigSet;
        INTERNAL_ERR_IF(sigfillset(&sigSet) != 0, "Could not set signal set.");
        INTERNAL_ERR_IF(pthread_sigmask(SIG_UNBLOCK, &sigSet, NULL) != 0, "Could not set signal mask.");

        SetPriority(priorityPtr);

        // Get the smack label for the process.
        char smackLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
        appSmack_GetLabel(appNamePtr, smackLabel, sizeof(smackLabel));

        // Set the process's SMACK label.
        smack_SetMyLabel(smackLabel);

        // Sandbox the process.
        sandbox_ConfineProc(sandboxDirPtr, uid, gid, NULL, 0, "/");
    }
    else
    {
        LE_WARN("Application '%s' is unsandboxed", appNamePtr);

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
    LE_DEBUG("Execing '%s' in application '%s'.\n", execPathPtr, appNamePtr);
    execvp(execPathPtr, (char* const *)procArgs);

    fprintf(stderr, "Could not exec '%s'.  %m.\n", execPathPtr);
    exit(EXIT_FAILURE);
}
