/** @file sdirTool.c
 *
 * This file implements the @ref sdirTool.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "sdirToolProtocol.h"
#include "limit.h"
#include "user.h"


//--------------------------------------------------------------------------------------------------
/// Reference to IPC session with the Service Directory
//--------------------------------------------------------------------------------------------------
static le_msg_SessionRef_t SessionRef = NULL;


//--------------------------------------------------------------------------------------------------
/// True if an error has occurred at some point.
//--------------------------------------------------------------------------------------------------
static bool ErrorOccurred = false;


//--------------------------------------------------------------------------------------------------
/// Command string.
//--------------------------------------------------------------------------------------------------
static const char* CommandPtr = NULL;


//--------------------------------------------------------------------------------------------------
/// Format option string.
//--------------------------------------------------------------------------------------------------
static const char* FormatPtr = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Prints help to stdout and exits with EXIT_SUCCESS.
 */
//--------------------------------------------------------------------------------------------------
static void PrintHelpAndExit
(
    void
)
{
    puts(
        "NAME:\n"
        "    sdir - Service Directory tool.\n"
        "\n"
        "SYNOPSIS:\n"
        "    sdir list\n"
        "    sdir list --format=json\n"
        "    sdir load\n"
        "\n"
        "DESCRIPTION:\n"
        "    sdir list\n"
        "            Lists bindings, services, and waiting clients.\n"
        "\n"
        "    sdir list --format=json\n"
        "            Lists bindings, services, and waiting clients in json format.\n"
        "\n"
        "    sdir load\n"
        "            Updates the Service Directory's bindings with the current state.\n"
        "            of the binding configuration settings in the configuration tree.\n"
        "\n"
        "            The tool will not exit until it gets confirmation from\n"
        "            the Service Directory that the changes have been applied.\n"
        "\n"
        "    All output is always sent to stdout and error messages to stderr.\n"
        "\n"
        );

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handles the Service Directory closing the IPC session.
 **/
//--------------------------------------------------------------------------------------------------
static void SessionCloseHandler
(
    le_msg_SessionRef_t sessionRef, // not used.
    void* contextPtr // not used.
)
//--------------------------------------------------------------------------------------------------
{
    if (ErrorOccurred)
    {
        exit(EXIT_FAILURE);
    }
    else
    {
        exit(EXIT_SUCCESS);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Opens an IPC session with the Service Directory.
 */
//--------------------------------------------------------------------------------------------------
static void ConnectToServiceDirectory
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_ProtocolRef_t protocolRef = le_msg_GetProtocolRef(LE_SDTP_PROTOCOL_ID,
                                                             sizeof(le_sdtp_Msg_t));
    SessionRef = le_msg_CreateSession(protocolRef, LE_SDTP_INTERFACE_NAME);

    le_msg_SetSessionCloseHandler(SessionRef, SessionCloseHandler, NULL);

    le_result_t result = le_msg_TryOpenSessionSync(SessionRef);
    if (result != LE_OK)
    {
        printf("***ERROR: Can't communicate with the Service Directory.\n");

        switch (result)
        {
            case LE_UNAVAILABLE:
                printf("Service not offered by Service Directory.\n"
                       "Bug in the Service Directory?\n");
                break;

            case LE_NOT_PERMITTED:
                printf("Missing binding to service.\n"
                       "System misconfiguration detected.\n");
                break;

            case LE_COMM_ERROR:
                printf("Service Directory is unreachable.\n"
                       "Perhaps the Service Directory is not running?\n");
                break;

            default:
                printf("Unexpected result code %d (%s)\n", result, LE_RESULT_TXT(result));
                break;
        }
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print error message and exit.
 */
//--------------------------------------------------------------------------------------------------
static void ExitWithErrorMsg
(
    const char* errorMsg
)
//--------------------------------------------------------------------------------------------------
{
    const char* programNamePtr = le_arg_GetProgramName();

    fprintf(stderr, "* %s: %s\n", programNamePtr, errorMsg);
    fprintf(stderr, "Try '%s --help'.\n", programNamePtr);

    exit(EXIT_FAILURE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Execute a 'list' command.
 */
//--------------------------------------------------------------------------------------------------
static void List
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t msgRef = le_msg_CreateMsg(SessionRef);

    le_msg_SetFd(msgRef, 1);

    le_sdtp_Msg_t* msgPtr = le_msg_GetPayloadPtr(msgRef);

    if (FormatPtr == NULL)
    {
        msgPtr->msgType = LE_SDTP_MSGID_LIST;
    }
    else
    {
        // Currently only json format is accepted.
        msgPtr->msgType = LE_SDTP_MSGID_LIST_JSON;
    }

    msgRef = le_msg_RequestSyncResponse(msgRef);

    if (msgRef == NULL)
    {
        ExitWithErrorMsg("Communication with Service Directory failed.");
    }

    le_msg_ReleaseMsg(msgRef);

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Send an "Unbind All" request to the Service Directory.
 */
//--------------------------------------------------------------------------------------------------
static void SendUnbindAllRequest
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t msgRef = le_msg_CreateMsg(SessionRef);
    le_sdtp_Msg_t* msgPtr = le_msg_GetPayloadPtr(msgRef);

    msgPtr->msgType = LE_SDTP_MSGID_UNBIND_ALL;

    msgRef = le_msg_RequestSyncResponse(msgRef);

    if (msgRef == NULL)
    {
        ExitWithErrorMsg("Communication with Service Directory failed.");
    }

    le_msg_ReleaseMsg(msgRef);
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the user ID for a given app name.
 *
 * @return LE_OK if successful.
 **/
//--------------------------------------------------------------------------------------------------
static le_result_t GetServerUid
(
    le_cfg_IteratorRef_t    i,  ///< [in] Config tree iterator positioned at binding config.
    uid_t*  uidPtr              ///< [out] The application's user ID.
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;

    char userName[LIMIT_MAX_USER_NAME_BYTES];

    // If an app name is present in the binding config,
    if (le_cfg_NodeExists(i, "app"))
    {
        // Make sure there isn't also a user name.
        if (le_cfg_NodeExists(i, "user"))
        {
            char path[LIMIT_MAX_PATH_BYTES];
            le_cfg_GetPath(i, "", path, sizeof(path));
            LE_CRIT("Both server user and app nodes appear under binding (@ %s)", path);
            return LE_DUPLICATE;
        }

        // Get the app name.
        char appName[LIMIT_MAX_APP_NAME_BYTES];
        result = le_cfg_GetString(i, "app", appName, sizeof(appName), "");
        if (result != LE_OK)
        {
            char path[LIMIT_MAX_PATH_BYTES];
            le_cfg_GetPath(i, "app", path, sizeof(path));
            LE_CRIT("Server app name too big (@ %s)", path);
            return result;
        }
        if (appName[0] == '\0')
        {
            char path[LIMIT_MAX_PATH_BYTES];
            le_cfg_GetPath(i, "app", path, sizeof(path));
            LE_CRIT("Server app name empty (@ %s)", path);
            return LE_NOT_FOUND;
        }

        // Find out if the server app is sandboxed.  If not, it runs as root.
        char path[LIMIT_MAX_PATH_BYTES];
        if (snprintf(path, sizeof(path), "/apps/%s/sandboxed", appName) >= sizeof(path))
        {
            LE_CRIT("Config node path too long (app name '%s').", appName);
            return LE_OVERFLOW;
        }
        if (!le_cfg_GetBool(i, path, true))
        {
            *uidPtr = 0;

            return LE_OK;
        }

        // It is not sandboxed.  Convert the app name into a user name.
        result = user_AppNameToUserName(appName, userName, sizeof(userName));
        if (result != LE_OK)
        {
            LE_CRIT("Failed to convert app name '%s' into a user name.", appName);

            return result;
        }
    }
    // If a server app name is not present in the binding config,
    else
    {
        // Get the server user name instead.
        result = le_cfg_GetString(i, "user", userName, sizeof(userName), "");
        if (result != LE_OK)
        {
            char path[LIMIT_MAX_PATH_BYTES];

            le_cfg_GetPath(i, "user", path, sizeof(path));
            LE_CRIT("Server user name too big (@ %s)", path);

            return result;
        }
        if (userName[0] == '\0')
        {
            char path[LIMIT_MAX_PATH_BYTES];

            le_cfg_GetPath(i, "", path, sizeof(path));
            LE_CRIT("Server user name or app name missing (@ %s)", path);

            return LE_NOT_FOUND;
        }
    }

    // Convert the server's user name into a user ID.
    result = user_GetUid(userName, uidPtr);
    if (result != LE_OK)
    {
        // Note: This can happen if the server application isn't installed yet.
        //       When the server application is installed, sdir load will be run
        //       again and the bindings will be correctly set up at that time.
        if (strncmp(userName, "app", 3) == 0)
        {
            LE_DEBUG("Couldn't get UID for application '%s'.  Perhaps it is not installed yet?",
                     userName + 3);
        }
        else
        {
            char path[LIMIT_MAX_PATH_BYTES];
            le_cfg_GetPath(i, "", path, sizeof(path));
            LE_CRIT("Couldn't convert server user name '%s' to UID (%s @ %s)",
                    userName,
                    LE_RESULT_TXT(result),
                    path);
        }

        return result;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Send a binding from a configuration tree iterator's current node to the Service Directory.
 */
//--------------------------------------------------------------------------------------------------
static void SendBindRequest
(
    uid_t uid,                  ///< [in] Unix user ID of the client whose binding is being created.
    le_cfg_IteratorRef_t i      ///< [in] Configuration read iterator.
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;

    le_msg_MessageRef_t msgRef = le_msg_CreateMsg(SessionRef);
    le_sdtp_Msg_t* msgPtr = le_msg_GetPayloadPtr(msgRef);

    msgPtr->msgType = LE_SDTP_MSGID_BIND;
    msgPtr->client = uid;

    // Fetch the client's service name.
    result = le_cfg_GetNodeName(i,
                                "",
                                msgPtr->clientInterfaceName,
                                sizeof(msgPtr->clientInterfaceName));
    if (result != LE_OK)
    {
        char path[LIMIT_MAX_PATH_BYTES];
        le_cfg_GetPath(i, "", path, sizeof(path));
        LE_CRIT("Configured client service name too long (@ %s)", path);
        return;
    }

    // Fetch the server's user ID.
    result = GetServerUid(i, &msgPtr->server);
    if (result != LE_OK)
    {
        return;
    }

    // Fetch the server's service name.
    result = le_cfg_GetString(i,
                              "interface",
                              msgPtr->serverInterfaceName,
                              sizeof(msgPtr->serverInterfaceName),
                              "");
    if (result != LE_OK)
    {
        char path[LIMIT_MAX_PATH_BYTES];
        le_cfg_GetPath(i, "interface", path, sizeof(path));
        LE_CRIT("Server interface name too big (@ %s)", path);
        return;
    }
    if (msgPtr->serverInterfaceName[0] == '\0')
    {
        char path[LIMIT_MAX_PATH_BYTES];
        le_cfg_GetPath(i, "interface", path, sizeof(path));
        LE_CRIT("Server interface name missing (@ %s)", path);
        return;
    }

    msgRef = le_msg_RequestSyncResponse(msgRef);

    if (msgRef == NULL)
    {
        ExitWithErrorMsg("Communication with Service Directory failed.");
    }

    le_msg_ReleaseMsg(msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the Unix user ID for the user configuration node that a given configuration iterator
 * is currently positioned at.
 *
 * @return LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetUserUid
(
    le_cfg_IteratorRef_t i, ///< [in] Configuration tree iterator.
    uid_t*          uidPtr  ///< [out] Pointer to where the user ID will be put if successful.
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;

    char userName[LIMIT_MAX_USER_NAME_BYTES];
    result = le_cfg_GetNodeName(i, "", userName, sizeof(userName));

    if (result != LE_OK)
    {
        LE_CRIT("Configuration node name too long under 'system/users/'.");
        return LE_OVERFLOW;
    }

    // Convert the user name into a user ID.
    result = user_GetUid(userName, uidPtr);
    if (result != LE_OK)
    {
        LE_CRIT("Failed to get user ID for user '%s'. (%s)", userName, LE_RESULT_TXT(result));
        return LE_NOT_FOUND;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the Unix user ID for the app configuration node that a given configuration iterator
 * is currently positioned at.
 *
 * @return LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetAppUid
(
    le_cfg_IteratorRef_t i, ///< [in] Configuration tree iterator.
    uid_t*          uidPtr  ///< [out] Pointer to where the user ID will be put if successful.
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;

    char appName[LIMIT_MAX_APP_NAME_BYTES];
    result = le_cfg_GetNodeName(i, "", appName, sizeof(appName));
    if (result != LE_OK)
    {
        LE_CRIT("Configuration node name too long under 'system/apps/'.");
        return LE_OVERFLOW;
    }

    // If this is an "unsandboxed" app, use the root user ID.
    if (le_cfg_GetBool(i, "sandboxed", true) == false)
    {
        char path[256];
        le_cfg_GetPath(i, "", path, sizeof(path));
        LE_DEBUG("'%s' = <root>", path);

        *uidPtr = 0;
        return LE_OK;
    }

    // Convert the app name into a user name by prefixing it with "app".
    char userName[LIMIT_MAX_USER_NAME_BYTES] = "app";
    result = le_utf8_Append(userName, appName, sizeof(userName), NULL);
    if (result != LE_OK)
    {
        LE_CRIT("Failed to convert app name into user name.");
        return LE_OVERFLOW;
    }

    // Convert the app user name into a user ID.
    result = user_GetUid(userName, uidPtr);
    if (result != LE_OK)
    {
        LE_CRIT("Failed to get user ID for user '%s'. (%s)", userName, LE_RESULT_TXT(result));
        return LE_NOT_FOUND;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Execute a 'load' command.
 */
//--------------------------------------------------------------------------------------------------
static void Load
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;

    // Connect to the Configuration API server.
    le_cfg_ConnectService();

    // Initialize the "User API".
    user_Init();

    // Start a read transaction on the root of the "system" configuration tree.
    le_cfg_IteratorRef_t i = le_cfg_CreateReadTxn("system:");

    // Tell the Service Directory to delete all existing bindings.
    SendUnbindAllRequest();

    // Iterate over the users collection.
    le_cfg_GoToNode(i, "/users");
    result = le_cfg_GoToFirstChild(i);
    while (result == LE_OK)
    {
        uid_t uid;
        if (GetUserUid(i, &uid) == LE_OK)
        {
            // For each user, iterate over the bindings collection, sending the binding to
            // the Service Directory.
            le_cfg_GoToNode(i, "bindings");
            result = le_cfg_GoToFirstChild(i);
            while (result == LE_OK)
            {
                SendBindRequest(uid, i);

                result = le_cfg_GoToNextSibling(i);
            }

            // Go back up to the user name node.
            le_cfg_GoToNode(i, "../..");
        }

        // Move on to the next user.
        result = le_cfg_GoToNextSibling(i);
    }

    // Iterate over the apps collection.
    le_cfg_GoToNode(i, "/apps");
    result = le_cfg_GoToFirstChild(i);
    while (result == LE_OK)
    {
        uid_t uid;

        if (GetAppUid(i, &uid) == LE_OK)
        {
            // For each app, iterate over the bindings collection, sending the binding to
            // the Service Directory.
            le_cfg_GoToNode(i, "bindings");
            result = le_cfg_GoToFirstChild(i);
            while (result == LE_OK)
            {
                SendBindRequest(uid, i);

                result = le_cfg_GoToNextSibling(i);
            }

            // Go back up to the app's node.
            le_cfg_GoToNode(i, "../..");
        }

        // Move on to the next app.
        result = le_cfg_GoToNextSibling(i);
    }


    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Positional argument callback function that gets called with the command argument from the
 * command line.
 **/
//--------------------------------------------------------------------------------------------------
static void CommandArgHandler
(
    const char* commandPtr  ///< Pointer to the command string.
)
//--------------------------------------------------------------------------------------------------
{
    CommandPtr = commandPtr;

    if ((strcmp(CommandPtr, "list") != 0) && (strcmp(CommandPtr, "load") != 0))
    {
       char errorMessage[255];

       snprintf(errorMessage, sizeof(errorMessage), "Unrecognized command '%s'.", CommandPtr);

       ExitWithErrorMsg(errorMessage);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function called by command line argument scanner when the --format= option is given.
 **/
//--------------------------------------------------------------------------------------------------
static void FormatArgHandler
(
    const char* formatPtr  ///< Pointer to the format string.
)
//--------------------------------------------------------------------------------------------------
{
    FormatPtr = formatPtr;

    if (strcmp(FormatPtr, "json") != 0)
    {
      char errorMessage[255];

      snprintf(errorMessage, sizeof(errorMessage), "Unrecognized command '%s'.", FormatPtr);

      ExitWithErrorMsg(errorMessage);
    }
}


//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // The first (and only) positional argument is the command.
    // CommandArgHandler() will set the CommandPtr to point to the command argument string.
    le_arg_AddPositionalCallback(CommandArgHandler);

    // Print help and exit if the "-h" or "--help" options are given.
    le_arg_SetFlagCallback(PrintHelpAndExit, "h", "help");

    // --format=json option specifies to dump sdir list in json format.
    le_arg_SetStringCallback(FormatArgHandler, NULL, "format");

    // Scan the command-line argument list.
    le_arg_Scan();

    // Check if the user is asking for help.
    if (strcmp(CommandPtr, "help") == 0)
    {
        PrintHelpAndExit();
    }

    ConnectToServiceDirectory();

    // Act on the command. Right now only two command(load and list) is allowed.
    if (strcmp(CommandPtr, "list") == 0)
    {
        List();
    }
    else
    {
        Load();
    }

}
