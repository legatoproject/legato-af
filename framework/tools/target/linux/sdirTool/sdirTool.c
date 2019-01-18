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
/// Format option string.  NULL = default format.
//--------------------------------------------------------------------------------------------------
static const char* FormatPtr = NULL;


//--------------------------------------------------------------------------------------------------
/// Client interface specifier string (used by Bind()).
//--------------------------------------------------------------------------------------------------
static const char* ClientIfPtr = NULL;


//--------------------------------------------------------------------------------------------------
/// Server interface specifier string (used by Bind()).
//--------------------------------------------------------------------------------------------------
static const char* ServerIfPtr = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Temporary file, created in our local tmpfs, used to help with printing sdir list.
 */
//--------------------------------------------------------------------------------------------------
#define TEMP_FILE                   "/tmp/sdOutput"


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
        "    sdir bind CLIENT_IF SERVER_IF\n"
        "    sdir help\n"
        "    sdir -h\n"
        "    sdir --help\n"
        "\n"
        "DESCRIPTION:\n"
        "    sdir list\n"
        "            Lists bindings, services, and waiting clients.\n"
        "\n"
        "    sdir list --format=json\n"
        "            Lists bindings, services, and waiting clients in json format.\n"
        "\n"
        "    sdir load\n"
        "            Updates the Service Directory's bindings with the current state\n"
        "            of the binding configuration settings in the configuration tree.\n"
        "\n"
        "            The tool will not exit until it gets confirmation from\n"
        "            the Service Directory that the changes have been applied.\n"
        "\n"
        "    sdir bind CLIENT_IF SERVER_IF\n"
        "            Creates a temporary binding in the Service Directory from\n"
        "            client-side IPC interface CLIENT_IF to server-side IPC interface\n"
        "            SERVER_IF.  Each of the interfaces is specified in one of the\n"
        "            following forms:\n"
        "                appName.executableName.componentName.interfaceName\n"
        "                appName.externInterfaceName\n"
        "                <userName>.executableName.componentName.interfaceName\n"
        "                <userName>.externInterfaceName\n"
        "\n"
        "    sdir help\n"
        "    sdir -h\n"
        "    sdir --help\n"
        "           Print this help text and exit.\n"
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
    fprintf(stderr, "Try '%s help'.\n", programNamePtr);

    exit(EXIT_FAILURE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Print 'list' command.
 */
//--------------------------------------------------------------------------------------------------
static void PrintList
(
    void
)
{
    int c;
    FILE *file;
    file = fopen(TEMP_FILE, "r");
    if (file) {
        while ((c = getc(file)) != EOF)
        {
            putchar(c);
        }
        fclose(file);
    }
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

    int fd = open(TEMP_FILE, O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR);

    if (fd < 0)
    {
        ExitWithErrorMsg("Setup with Service Directory failed.");
    }

    le_msg_SetFd(msgRef, fd);

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
        close(fd);
        ExitWithErrorMsg("Communication with Service Directory failed.");
    }

    close(fd);
    le_msg_ReleaseMsg(msgRef);

    // Print list output
    PrintList();

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
static void SendCfgBindRequest
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
                SendCfgBindRequest(uid, i);

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
                SendCfgBindRequest(uid, i);

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
 * Parse an interface specifier and extract the user ID and interface name.
 */
//--------------------------------------------------------------------------------------------------
static void ParseInterfaceSpec
(
    const char* spec,
    uid_t* uidPtr,      /// [OUT] Ptr to where the UID will be stored.
    char* ifName,       /// [OUT] Ptr to where the interface name will be copied.
    size_t ifNameSize   /// [IN] Number of bytes in the ifName buffer.
)
//--------------------------------------------------------------------------------------------------
{
    size_t i; // Index into the spec string.
    char userName[LIMIT_MAX_USER_NAME_BYTES] = {0};

    // If the interface spec starts with a '<', then it must be a user name.
    if (spec[0] == '<')
    {
        for (i = 1; spec[i] != '>'; i++)
        {
            if (i >= (sizeof(userName) - 1))
            {
                fprintf(stderr, "User name too long in interface specifier '%s'.\n", spec);
                exit(EXIT_FAILURE);
            }

            if (spec[i] == '\0')
            {
                fprintf(stderr,
                        "Missing terminating '>' after user name in interface specifier '%s'.\n",
                        spec);
                exit(EXIT_FAILURE);
            }

            userName[i - 1] = spec[i];
        }

        // Move past the '>' in the spec string.
        i++;
    }
    else
    {
        // Extract the app name and convert it into a user name.
        char appName[LIMIT_MAX_APP_NAME_BYTES] = {0};

        for (i = 0; spec[i] != '.'; i++)
        {
            if (i >= (sizeof(appName) - 1))
            {
                fprintf(stderr, "App name too long in interface specifier '%s'.\n", spec);
                exit(EXIT_FAILURE);
            }

            if (spec[i] == '\0')
            {
                fprintf(stderr, "Missing '.' after app name in interface specifier '%s'.\n", spec);
                exit(EXIT_FAILURE);
            }

            appName[i] = spec[i];
        }

        if (LE_OK != user_AppNameToUserName(appName, userName, sizeof(userName)))
        {
            LE_FATAL("userName buffer overflow.");
        }
    }

    // Convert the user name into a UID.
    if (user_GetUid(userName, uidPtr) != LE_OK)
    {
        fprintf(stderr, "Failed to get user ID for user '%s'.\n", userName);
        exit(EXIT_FAILURE);
    }

    // Move past the first '.' in the spec string.
    i++;

    if (spec[i] == '\0')
    {
        fprintf(stderr, "Missing interface name in interface specifier '%s'.\n", spec);
        exit(EXIT_FAILURE);
    }

    // The rest of the spec string is the interface name.
    if (LE_OK != le_utf8_Copy(ifName, spec + i, ifNameSize, NULL))
    {
        fprintf(stderr, "Interface name too long in interface specifier '%s'.\n", spec);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Execute the 'bind' command.
 */
//--------------------------------------------------------------------------------------------------
static void Bind
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Initialize the "User API".
    user_Init();

    // Construct the request message.
    le_msg_MessageRef_t msgRef = le_msg_CreateMsg(SessionRef);
    le_sdtp_Msg_t* msgPtr = le_msg_GetPayloadPtr(msgRef);

    msgPtr->msgType = LE_SDTP_MSGID_BIND;

    // Parse the client interface specifier.
    ParseInterfaceSpec(ClientIfPtr,
                       &msgPtr->client,
                       msgPtr->clientInterfaceName,
                       sizeof(msgPtr->clientInterfaceName));

    // Parse the server interface specifier.
    ParseInterfaceSpec(ServerIfPtr,
                       &msgPtr->server,
                       msgPtr->serverInterfaceName,
                       sizeof(msgPtr->serverInterfaceName));

    // Send the message and wait for a response.
    msgRef = le_msg_RequestSyncResponse(msgRef);

    // If a response message was not received, then the operation failed.
    if (msgRef == NULL)
    {
        ExitWithErrorMsg("Communication with Service Directory failed.");
    }

    le_msg_ReleaseMsg(msgRef);

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Positional argument callback function that gets called with the CLIENT_IF argument from the
 * command line.
 **/
//--------------------------------------------------------------------------------------------------
static void ClientIfArgHandler
(
    const char* argPtr  ///< Pointer to the argument string.
)
//--------------------------------------------------------------------------------------------------
{
    // Just save it for now.
    ClientIfPtr = argPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Positional argument callback function that gets called with the SERVER_IF argument from the
 * command line.
 **/
//--------------------------------------------------------------------------------------------------
static void ServerIfArgHandler
(
    const char* argPtr  ///< Pointer to the argument string.
)
//--------------------------------------------------------------------------------------------------
{
    // Just save it for now.
    ServerIfPtr = argPtr;
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

    // The bind command expects two positional arguments: the client and the server interfaces.
    if (strcmp(CommandPtr, "bind") == 0)
    {
        le_arg_AddPositionalCallback(ClientIfArgHandler);
        le_arg_AddPositionalCallback(ServerIfArgHandler);
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
    // The first positional argument is the command.
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

    // Processing of all other commands requires a connection to the Service Directory.
    ConnectToServiceDirectory();

    if (strcmp(CommandPtr, "list") == 0)
    {
        List();
    }
    // --format= option is not valid for any commands other than 'list'.
    else if (FormatPtr != NULL)
    {
        char errorMsg[255];
        snprintf(errorMsg,
                 sizeof(errorMsg),
                 "Format specifier (--format=%s) not valid for command '%s'.",
                 FormatPtr,
                 CommandPtr);
        ExitWithErrorMsg(errorMsg);
    }
    else if (strcmp(CommandPtr, "load") == 0)
    {
        Load();
    }
    else if (strcmp(CommandPtr, "bind") == 0)
    {
        Bind();
    }
    else
    {
       char errorMsg[255];
       snprintf(errorMsg, sizeof(errorMsg), "Unrecognized command '%s'.", CommandPtr);
       ExitWithErrorMsg(errorMsg);
    }
}
