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
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "limit.h"
#include "user.h"
#include "proc.h"
#include "sandbox.h"


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
 *      LE_OK if successful.
 *      LE_OVERFLOW if the option was found but the provided buffer was too small to hold the value.
 *                  The buffer will contain a truncated value string.
 *      LE_NOT_FOUND if the option is not available on the command-line.
 *      LE_FAULT if there was an error.
 *
 * @todo
 *      This function should be replaced by a general command-line options utility.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetOptionValue
(
    const char* option,     ///< [IN] The option.
    char* bufPtr,           ///< [OUT] The storage location for the value string.
    size_t bufSize          ///< [IN] The buffer size.
)
{
    // Make the string to search for.
    char searchStr[LIMIT_MAX_ARGS_STR_BYTES + 1];

    int n = snprintf(searchStr, sizeof(searchStr), "%s=", option);

    if ( (n < 0) || (n >= sizeof(searchStr)) )
    {
        LE_ERROR("Option string is too long.");
        return LE_FAULT;
    }

    // Search the list of command line arguments for the specified option.
    size_t numArgs = le_arg_NumArgs();
    size_t i = 0;

    char argBuf[LIMIT_MAX_ARGS_STR_BYTES];

    for (; i < numArgs; i++)
    {
        if (le_arg_GetArg(i, argBuf, sizeof(argBuf)) != LE_OK)
        {
            LE_ERROR("Wrong number of arguments.");
            return LE_FAULT;
        }

        char* subStr = strstr(argBuf, option);

        if ( (subStr != NULL) && (subStr == argBuf) )
        {
            // The argBuf begins with the searchStr.  The remainder of the argBuf is the value string.
            char* valueStr = argBuf + strlen(searchStr);

            // Copy the value to the user buffer.
            return le_utf8_Copy(bufPtr, valueStr, bufSize, NULL);
        }
    }

    return LE_NOT_FOUND;
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
    // Read the priority setting from the command line.
    char priorStr[LIMIT_MAX_PRIORITY_NAME_BYTES] = "medium";
    le_result_t result = GetOptionValue("--priority", priorStr, sizeof(priorStr));

    INTERNAL_ERR_IF(result == LE_FAULT, "Could not read priority from the command line.");

    if (result == LE_OVERFLOW)
    {
        fprintf(stderr, "Unrecognized priority level.\n");
        exit(EXIT_FAILURE);
    }

    if (proc_SetPriority(priorStr, 0) != LE_OK)
    {
        fprintf(stderr, "Could not set the priority level to '%s'.\n", priorStr);
        exit(EXIT_FAILURE);
    }
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
 * Gets the executable path from the command-line.
 *
 * @note This function kills the calling process if there is an error.
 *
 * @return
 *      The command-line arguments index of the executable path.
 */
//--------------------------------------------------------------------------------------------------
static size_t GetExecPath
(
    char* bufPtr,           ///< [OUT] Buffer to store the executable path in.
    size_t bufSize          ///< [IN] Buffer size.
)
{
    // The executable path is the first argument after the list of options.  Search for the exec
    // path starting from the second argument.
    const size_t numArgs = le_arg_NumArgs();
    size_t i = 1;

    for (; i < numArgs; i++)
    {
        char arg[LIMIT_MAX_ARGS_STR_BYTES];

        le_result_t result = le_arg_GetArg(i, arg, sizeof(arg));

        if (result == LE_OVERFLOW)
        {
            fprintf(stderr, "The argument '%s'is too long.\n", arg);
            exit(EXIT_FAILURE);
        }
        else if (result == LE_NOT_FOUND)
        {
            fprintf(stderr, "Please specify an executable.\n");
            exit(EXIT_FAILURE);
        }
        else if (arg[0] != '-')
        {
            // This is the executable path.
            if (le_utf8_Copy(bufPtr, arg, bufSize, NULL) == LE_OK)
            {
                return i;
            }
            else
            {
                fprintf(stderr, "The executable path '%s' is too long.\n", arg);
                exit(EXIT_FAILURE);
            }
        }
    }

    fprintf(stderr, "Please specify an executable.\n");
    exit(EXIT_FAILURE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the name of the process to start.  This is either specified on the command-line with the
 * --procName option or the name of the executable.
 *
 * @note This function kills the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static void GetProcName
(
    char* bufPtr,           ///< [OUT] Buffer to store the process name in.
    size_t bufSize          ///< [IN] Buffer size.
)
{
    // Check if the process name is specified on the command line.
    le_result_t result = GetOptionValue("--procName", bufPtr, bufSize);

    INTERNAL_ERR_IF(result == LE_FAULT, "Could not read the process name from the command line.");

    if (result == LE_OVERFLOW)
    {
        fprintf(stderr, "The process name is too long.\n");
        exit(EXIT_FAILURE);
    }
    else if (result == LE_OK)
    {
        // Got the process name.
        return;
    }

    // The process name was not specified on the command line.  Use the executable name instead.
    char execPath[LIMIT_MAX_PATH_BYTES];
    GetExecPath(execPath, sizeof(execPath));

    result = le_utf8_Copy(bufPtr, le_path_GetBasenamePtr(execPath, "/"), bufSize, NULL);

    if (result == LE_OVERFLOW)
    {
        fprintf(stderr, "The executable name is too long.\n");
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the list of arguments to pass to the process that will be executed.  The process's argument
 * list is all the arguments on the command-line after the executable path.
 *
 * @note This function kills the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static void GetProcArgsList
(
    char argBufs[][LIMIT_MAX_ARGS_STR_BYTES], ///< [OUT] Array of buffers to store the arguments list.
    size_t numBufs,         ///< [IN] Number of buffers in argBufs.
    size_t procArgsIndex    ///< [IN] The index in our list of arguments that is the start of the
                            ///       process's list of arguments.
)
{
    size_t i = 0;
    for (; i < numBufs; i++)
    {
        le_result_t result = le_arg_GetArg(i + procArgsIndex, argBufs[i], LIMIT_MAX_ARGS_STR_BYTES);

        if (result == LE_OVERFLOW)
        {
            fprintf(stderr, "Argument '%s' is too long.\n", argBufs[i]);
            exit(EXIT_FAILURE);
        }

        INTERNAL_ERR_IF(result != LE_OK, "Unexpected end of arguments list.\n");
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
    app_Ref_t appRef,               ///< [IN] App ref.
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

COMPONENT_INIT
{
    if (IsOptionSelected("--help"))
    {
        PrintHelp();
        exit(EXIT_SUCCESS);
    }

    char appName[LIMIT_MAX_APP_NAME_BYTES];
    GetAppName(appName, sizeof(appName));

    le_sup_state_StartClient(LE_SUP_STATE_API_NAME);

    le_sup_state_State_t appState;
    appState = le_sup_state_GetAppState(appName);

    switch(appState)
    {
    case LE_SUP_STATE_RUNNING:
    case LE_SUP_STATE_PAUSED:
        break;
    default:
        fprintf(stderr, "Application '%s' is not running.\n", appName);
        exit(EXIT_FAILURE);
        break;
    }

    app_Ref_t appRef = GetAppRef(appName);

    char procName[LIMIT_MAX_PROCESS_NAME_BYTES];
    GetProcName(procName, sizeof(procName));

    char execPath[LIMIT_MAX_PATH_BYTES];
    size_t procArgsIndex = GetExecPath(execPath, sizeof(execPath)) + 1;

    // Determine the number of arguments to pass to the process.
    size_t numProcArgs = le_arg_NumArgs() - procArgsIndex;

    // Build the process's argument list.  The arguments list must begin with the process name and
    // end with a NULL.
    char argsBuffers[numProcArgs][LIMIT_MAX_ARGS_STR_BYTES];
    char* argsPtr[numProcArgs + 2];

    // The first argument is always the process name.
    argsPtr[0] = procName;

    // The argument list must be NULL-terminated.
    argsPtr[NUM_ARRAY_MEMBERS(argsPtr) - 1] = NULL;

    if (numProcArgs > 0)
    {
        // Get the process's argument list.
        GetProcArgsList(argsBuffers, numProcArgs, procArgsIndex);

        // Point the arguments list to the argument buffers.
        size_t i = 0;
        for (; i < numProcArgs; i++)
        {
            argsPtr[i + 1] = argsBuffers[i];
        }
    }

    // Get the applications info.
    uid_t uid;
    gid_t gid;
    char userName[LIMIT_MAX_USER_NAME_BYTES];
    GetAppIds(appRef, appName, &uid, &gid, userName, sizeof(userName));
    LE_DEBUG("App: %s uid[%u] gid[%u] user[%s]\n", appName, uid, gid, userName);

    const char * sandboxDirPtr;
    sandboxDirPtr = app_GetSandboxPath(appRef);

    // Is application sandboxed ?
    if(sandboxDirPtr[0] != '\0')
    {
        LE_DEBUG("Application '%s' is sandboxed in %s", appName, sandboxDirPtr);

        const char * homeDirPtr = app_GetHomeDirPath(appRef);
        INTERNAL_ERR_IF(homeDirPtr == NULL, "Unable to get home directory.");

        // Set the umask so that files are not accidentally created with global permissions.
        umask(S_IRWXG | S_IRWXO);

        // Unblock all signals that might have been blocked.
        sigset_t sigSet;
        INTERNAL_ERR_IF(sigfillset(&sigSet) != 0, "Could not set signal set.");
        INTERNAL_ERR_IF(pthread_sigmask(SIG_UNBLOCK, &sigSet, NULL) != 0, "Could not set signal mask.");

        SetPriority();

        // Sandbox the process.
        sandbox_ConfineProc(sandboxDirPtr, uid, gid, NULL, 0, homeDirPtr);
    }
    else
    {
        LE_WARN("Application '%s' is unsandboxed", appName);

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
    LE_DEBUG("Execing '%s' in application '%s'.\n", execPath, appName);
    execvp(execPath, argsPtr);

    fprintf(stderr, "Could not exec '%s'.  %m.\n", execPath);
    exit(EXIT_FAILURE);
}
