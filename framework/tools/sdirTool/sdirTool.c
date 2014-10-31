/** @file sdirTool.c
 *
 * This file implements the @ref sdirTool.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "sdirToolProtocol.h"
#include "limit.h"
#include "user.h"
#include "messagingSession.h"

//--------------------------------------------------------------------------------------------------
/// Reference to IPC session with the Service Directory
//--------------------------------------------------------------------------------------------------
static le_msg_SessionRef_t SessionRef = NULL;


//--------------------------------------------------------------------------------------------------
/// True if an error has occurred at some point.
//--------------------------------------------------------------------------------------------------
static bool ErrorOccurred = false;


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
        "    sdir - Service Directory tool.\n"
        "\n"
        "SYNOPSIS:\n"
        "    sdir list\n"
        "    sdir load\n"
        "\n"
        "DESCRIPTION:\n"
        "    sdir list\n"
        "            Lists bindings, services, and waiting clients.\n"
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
    SessionRef = le_msg_CreateSession(protocolRef, LE_SDTP_SERVICE_NAME);

    le_msg_SetSessionCloseHandler(SessionRef, SessionCloseHandler, NULL);

    le_result_t result = msgSession_TryOpenSessionSync(SessionRef);
    if (result != LE_OK)
    {
        fprintf(stderr, "***ERROR: Can't communicate with the Sevice Directory.\n");
        fprintf(stderr,
                "Service Directory is unreachable.\n"
                "Perhaps the Service Directory is not running?\n");
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
    char programName[LIMIT_MAX_PATH_LEN] = "sdir";

    le_arg_GetProgramName(programName, sizeof(programName), NULL);

    fprintf(stderr, "* %s: %s\n", programName, errorMsg);
    fprintf(stderr, "Try '%s --help'.\n", programName);

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
    if (le_arg_NumArgs() != 1)
    {
        ExitWithErrorMsg("Too many arguments to 'list' command.\n");
    }

    le_msg_MessageRef_t msgRef = le_msg_CreateMsg(SessionRef);

    le_msg_SetFd(msgRef, 1);

    le_sdtp_Msg_t* msgPtr = le_msg_GetPayloadPtr(msgRef);

    msgPtr->msgType = LE_SDTP_MSGID_LIST;

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
                                msgPtr->clientServiceName,
                                sizeof(msgPtr->clientServiceName));
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
                              msgPtr->serverServiceName,
                              sizeof(msgPtr->serverServiceName),
                              "");
    if (result != LE_OK)
    {
        char path[LIMIT_MAX_PATH_BYTES];
        le_cfg_GetPath(i, "interface", path, sizeof(path));
        LE_CRIT("Server interface name too big (@ %s)", path);
        return;
    }
    if (msgPtr->serverServiceName[0] == '\0')
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

    if (le_arg_NumArgs() != 1)
    {
        ExitWithErrorMsg("Too many arguments to 'load' command.\n");
    }

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
 * The main function for the log tool.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    char arg[LIMIT_MAX_PATH_LEN];

    // Get the command.
    if (le_arg_NumArgs() == 0)
    {
        ExitWithErrorMsg("Command missing.");
    }
    if (le_arg_GetArg(0, arg, sizeof(arg)) != LE_OK)
    {
        ExitWithErrorMsg("Invalid command.");
    }

    // Check if the user is asking for help.
    if (   (strcmp(arg, "--help") == 0)
        || (strcmp(arg, "help") == 0)
        || (strcmp(arg, "-h") == 0) )
    {
        PrintHelp();

        exit(EXIT_SUCCESS);
    }

    ConnectToServiceDirectory();

    // Act on the command.
    if (strcmp(arg, "list") == 0)
    {
        List();
    }
    else if (strcmp(arg, "load") == 0)
    {
        Load();
    }
    else
    {
        char errorMessage[255];

        snprintf(errorMessage, sizeof(errorMessage), "Unrecognized command '%s'.", arg);

        ExitWithErrorMsg(errorMessage);
    }
}
