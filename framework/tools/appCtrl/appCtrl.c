/** @file appCtrl.c
 *
 * Control Legato applications.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "limit.h"


/// Pointer to application name argument from command line.
static const char* AppNamePtr = NULL;


/// Address of the command function to be executed.
static void (*CommandFunc)(void);


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

    exit(EXIT_SUCCESS);
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
    void
)
{
    le_sup_ctrl_ConnectService();

    // Start the application.
    switch (le_sup_ctrl_StartApp(AppNamePtr))
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
            fprintf(stderr, "There was an error.  Application '%s' could not be started.\n", AppNamePtr);
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
    le_sup_ctrl_ConnectService();

    // Stop the application.
    switch (le_sup_ctrl_StopApp(AppNamePtr))
    {
        case LE_OK:
            exit(EXIT_SUCCESS);

        case LE_NOT_FOUND:
            printf("Application '%s' was not running.\n", AppNamePtr);
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
 * Implements the "status" command.
 **/
//--------------------------------------------------------------------------------------------------
static void PrintStatus
(
    void
)
{
    if (AppNamePtr == NULL)
    {
        ListInstalledApps(true);
    }
    else
    {
        PrintAppState(AppNamePtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Implements the "list" command.
 **/
//--------------------------------------------------------------------------------------------------
static void ListApps
(
    void
)
{
    ListInstalledApps(false);
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

    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn("/apps");
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
    }
    else if (strcmp(command, "stop") == 0)
    {
        CommandFunc = StopApp;

        le_arg_AddPositionalCallback(AppNameArgHandler);
    }
    else if (strcmp(command, "stopLegato") == 0)
    {
        CommandFunc = StopLegato;
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
    else
    {
        fprintf(stderr, "Unknown command '%s'.  Try --help.\n", command);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_arg_SetFlagCallback(PrintHelp, "h", "help");

    le_arg_AddPositionalCallback(CommandArgHandler);

    le_arg_Scan();

    CommandFunc();
}
