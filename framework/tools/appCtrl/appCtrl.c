/** @file appCtrl.c
 *
 * Control Legato applications.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "limit.h"



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
        "    appCtrl - Used to start, stop and get the status of Legato applications.\n"
        "\n"
        "SYNOPSIS:\n"
        "    appCtrl --help\n"
        "    appCtrl start APP_NAME\n"
        "    appCtrl stop APP_NAME\n"
        "    appCtrl stopLegato\n"
        "    appCtrl list\n"
        "    appCtrl status [APP_NAME]\n"
        "    appCtrl version APP_NAME\n"
        "\n"
        "DESCRIPTION:\n"
        "    appCtrl --help\n"
        "       Display this help and exit.\n"
        "\n"
        "    appCtrl start APP_NAME\n"
        "       Starts the specified application.\n"
        "\n"
        "    appCtrl stop APP_NAME\n"
        "       Stops the specified application.\n"
        "\n"
        "    appCtrl stopLegato\n"
        "       Stops the Legato framework.\n"
        "\n"
        "    appCtrl list\n"
        "       List all installed applications.\n"
        "\n"
        "    appCtrl status [APP_NAME]\n"
        "       If no name is given, prints the status of all installed applications.\n"
        "       If a name is given, prints the status of the specified application.\n"
        "       The status of the application can be 'stopped', 'running', 'paused' or 'not installed'.\n"
        "\n"
        "    appCtrl version APP_NAME\n"
        "       Prints the version of the specified application.\n"
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
    const char* optionPtr
)
{
    size_t numArgs = le_arg_NumArgs();
    size_t i;

    char argBuf[LIMIT_MAX_ARGS_STR_BYTES];

    // Search the list of command line arguments for the specified option.
    for (i = 0; i < numArgs; i++)
    {
        INTERNAL_ERR_IF(le_arg_GetArg(i, argBuf, sizeof(argBuf)) != LE_OK,
                        "Not able to read argument at index %zd.", i);

        char* subStr = strstr(argBuf, optionPtr);

        if ( (subStr != NULL) && (subStr == argBuf) )
        {
            return true;
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Requests the Supervisor to start an application.
 *
 * @note This function does not return.
 */
//--------------------------------------------------------------------------------------------------
static void StartApp
(
    const char* appNamePtr
)
{
    le_sup_ctrl_ConnectService();

    // Start the application.
    switch (le_sup_ctrl_StartApp(appNamePtr))
    {
        case LE_OK:
            exit(EXIT_SUCCESS);

        case LE_DUPLICATE:
            fprintf(stderr, "Application '%s' is already running.\n", appNamePtr);
            exit(EXIT_FAILURE);

        case LE_NOT_FOUND:
            fprintf(stderr, "Application '%s' is not installed.\n", appNamePtr);
            exit(EXIT_FAILURE);

        default:
            fprintf(stderr, "There was an error.  Application '%s' could not be started.\n", appNamePtr);
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
    const char* appNamePtr
)
{
    le_sup_ctrl_ConnectService();

    // Stop the application.
    switch (le_sup_ctrl_StopApp(appNamePtr))
    {
        case LE_OK:
            exit(EXIT_SUCCESS);

        case LE_NOT_FOUND:
            printf("Application '%s' was not running.\n", appNamePtr);
            exit(EXIT_FAILURE);

        default:
            INTERNAL_ERR("Unexpected response from the Supervisor.");
    }
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
    le_sup_ctrl_ConnectService();

    // Stop the framework.
    le_result_t result = le_sup_ctrl_StopLegato();
    switch (result)
    {
        case LE_OK:
            exit(EXIT_SUCCESS);

        case LE_NOT_FOUND:
            printf("Legato is being stopped by someone else.\n");
            exit(EXIT_SUCCESS);

        default:
            INTERNAL_ERR("Unexpected response, %d, from the Supervisor.", result);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Prints an installed application's state.
 */
//--------------------------------------------------------------------------------------------------
static void PrintInstalledAppState
(
    const char* appNamePtr
)
{
    le_sup_state_State_t appState;
    const char * appStatePtr;

    appState = le_sup_state_GetAppState(appNamePtr);

    switch (appState)
    {
        case LE_SUP_STATE_STOPPED:
            appStatePtr = "stopped";
            break;

        case LE_SUP_STATE_RUNNING:
            appStatePtr = "running";
            break;

        case LE_SUP_STATE_PAUSED:
            appStatePtr = "paused";
            break;

        default:
            INTERNAL_ERR("Supervisor returned an unknown state for app '%s'.", appNamePtr);
    }

    printf("[%s] %s\n", appStatePtr, appNamePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Prints the list of installed apps.
 *
 * @note This function does not return.
 */
//--------------------------------------------------------------------------------------------------
static void ListInstalledApps
(
    bool withStatus
)
{
    le_cfg_ConnectService();

    if(withStatus)
    {
        le_sup_state_ConnectService();
    }

    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn("/apps");

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

        if (!withStatus)
        {
            printf("%s\n", appName);
        }
        else
        {
            PrintInstalledAppState(appName);
        }
    }
    while (le_cfg_GoToNextSibling(cfgIter) == LE_OK);

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints the application status.
 *
 * @note This function does not return.
 */
//--------------------------------------------------------------------------------------------------
static void PrintAppState
(
    const char* appNamePtr      ///< [IN] Application name to get the status for.
)
{
    le_sup_state_ConnectService();
    le_cfg_ConnectService();

    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn("/apps");

    if (!le_cfg_NodeExists(cfgIter, appNamePtr))
    {
        printf("[not installed] %s\n", appNamePtr);
    }
    else
    {
        PrintInstalledAppState(appNamePtr);
    }

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
    const char* appNamePtr      ///< [IN] Application name to get the version for.
)
{
    le_cfg_ConnectService();

    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn("/apps");
    le_cfg_GoToNode(cfgIter, appNamePtr);

    if (!le_cfg_NodeExists(cfgIter, ""))
    {
        printf("%s is not installed.\n", appNamePtr);
    }
    else
    {
        char version[LIMIT_MAX_PATH_BYTES];

        le_result_t result = le_cfg_GetString(cfgIter, "version", version, sizeof(version), "");

        if (strcmp(version, "") == 0)
        {
            printf("%s has no version\n", appNamePtr);
        }
        else if (result == LE_OK)
        {
            printf("%s %s\n", appNamePtr, version);
        }
        else
        {
            LE_WARN("Version string for app %s is too long.", appNamePtr);
            printf("%s %s...\n", appNamePtr, version);
        }
    }

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the application name from the command-line.
 *
 * @note
 *      On failure this function exits.
 *
 * @note
 *      The application name is assumed to be the second argument on the command-line.
 *
 * @return
 *      true if the app name is available.
 *      false if the app name is not available.
 */
//--------------------------------------------------------------------------------------------------
static bool GetAppName
(
    char* bufPtr,           ///< [OUT] Buffer to store the app name.
    size_t bufSize,         ///< [IN] Buffer size.
    bool isOptional         ///< [IN] Is the app name optional ?
)
{
    le_result_t res;
    bool isAvailable = false;

    res = le_arg_GetArg(1, bufPtr, bufSize);

    if (res == LE_OK)
    {
        isAvailable = (bufPtr[0] != '\0');
    }

    if(!isAvailable)
    {
        if(!isOptional)
        {
            fprintf(stderr, "Please specify an application name.\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            bufPtr[0] = '\0';
        }
    }

    return isAvailable;
}


COMPONENT_INIT
{
    if (IsOptionSelected("--help"))
    {
        PrintHelp();
        exit(EXIT_SUCCESS);
    }

    char argBuf[LIMIT_MAX_APP_NAME_BYTES];

    // Get the first argument which is the command.
    if (le_arg_GetArg(0, argBuf, sizeof(argBuf)) != LE_OK)
    {
        fprintf(stderr, "Please enter a command.  Try --help.\n");
        exit(EXIT_FAILURE);
    }

    // Process the command.
    if (strcmp(argBuf, "start") == 0)
    {
        GetAppName(argBuf, sizeof(argBuf), false);
        StartApp(argBuf);
    }
    else if (strcmp(argBuf, "stop") == 0)
    {
        GetAppName(argBuf, sizeof(argBuf), false);
        StopApp(argBuf);
    }
    else if (strcmp(argBuf, "stopLegato") == 0)
    {
        StopLegato();
    }
    else if (strcmp(argBuf, "list") == 0)
    {
        ListInstalledApps(false);
    }
    else if (strcmp(argBuf, "status") == 0)
    {
        if(GetAppName(argBuf, sizeof(argBuf), true))
        {
            PrintAppState(argBuf);
        }
        else
        {
            ListInstalledApps(true);
        }
    }
    else if (strcmp(argBuf, "version") == 0)
    {
        GetAppName(argBuf, sizeof(argBuf), false);
        PrintAppVersion(argBuf);
    }

    fprintf(stderr, "Unknown command '%s'.  Try --help.\n", argBuf);
    exit(EXIT_FAILURE);
}
