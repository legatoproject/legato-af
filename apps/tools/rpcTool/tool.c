//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the RPC Configuration command-line tool for administering the RPC from the
 * command line.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "interfaces.h"
#include "limit.h"

//--------------------------------------------------------------------------------------------------
/**
 * What type of action are we being asked to do?
 */
//--------------------------------------------------------------------------------------------------
static enum
{
    ACTION_UNSPECIFIED,
    ACTION_HELP,
    ACTION_GET,
    ACTION_SET,
    ACTION_RESET,
    ACTION_LIST,
}
Action = ACTION_UNSPECIFIED;


//--------------------------------------------------------------------------------------------------
/**
 * What type of object are we being asked to act on?
 */
//--------------------------------------------------------------------------------------------------
static enum
{
    OBJECT_BINDING,
    OBJECT_LINK,
}
Object;

//--------------------------------------------------------------------------------------------------
/**
 * Ptr to the Service-Name command-line argument, or NULL if there wasn't one provided.
 */
//--------------------------------------------------------------------------------------------------
static const char* ServiceNameArg = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Ptr to the System-Name command-line argument, or NULL if there wasn't one provided.
 */
//--------------------------------------------------------------------------------------------------
static const char* SystemNameArg = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Ptr to the Remote Service-Name command-line argument, or NULL if there wasn't one provided.
 */
//--------------------------------------------------------------------------------------------------
static const char* RemoteServiceNameArg = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Ptr to the Link-Name command-line argument, or NULL if there wasn't one provided.
 */
//--------------------------------------------------------------------------------------------------
static const char* LinkNameArg = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Ptr to the Node-Name command-line argument, or NULL if there wasn't one provided.
 */
//--------------------------------------------------------------------------------------------------
static const char* NodeNameArg = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Ptr to the Node-Value command-line argument, or NULL if there wasn't one provided.
 */
//--------------------------------------------------------------------------------------------------
static const char* NodeValueArg = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Print help text to stdout and exit with EXIT_SUCCESS.
 */
//--------------------------------------------------------------------------------------------------
static void HandleHelpRequest
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    puts(
        "NAME:\n"
        "    rpctool - RPC Configuration command-line tool.\n"
        "\n"
        "SYNOPSIS:\n"
        "    rpctool set binding <serviceName> <systemName> <remoteServiceName>\n"
        "    rpctool get binding <serviceName>\n"
        "    rpctool reset binding <serviceName>\n"
        "    rpctool list bindings\n"
        "    rpctool set link <systemName> <linkName> <nodeName> <nodeValue>\n"
        "    rpctool get link\n"
        "    rpctool reset link <systemName> <linkName>\n"
        "    rpctool list links\n"
        "    rpctool help\n"
        "    rpctool -h\n"
        "    rpctool --help\n"
        "\n"
        "DESCRIPTION:\n"
        "    rpctool set binding <serviceName> <systemName> <remoteServiceName>\n"
        "            Sets the RPC binding for the specified service-name with the\n"
        "            system-name and remote service-name.\n"
        "\n"
        "    rpctool get binding <serviceName>\n"
        "            Retrieves the system-name and service-name for the specified\n"
        "            service-name.\n"
        "\n"
        "    rpctool reset binding <serviceName>\n"
        "            Resets the RPC binding for a given service-name.\n"
        "\n"
        "    rpctool list bindings\n"
        "            Lists all RPC bindings configured in the system.\n"
        "\n"
        "    rpctool set link <systemName> <linkName> <nodeName> <nodeValue>\n"
        "            Sets the RPC link for the specified system-name and link-name\n"
        "            with the node-name and node-value.\n"
        "\n"
        "    rpctool get link <systemName> <linkName> <nodeName>\n"
        "            Retrieves the node-value for the specified system-name,\n"
        "            link-name, and node-name.\n"
        "\n"
        "    rpctool reset link <systemName>\n"
        "            Resets the RPC link for the specifed system-name.\n"
        "\n"
        "    rpctool list links\n"
        "            Lists all RPC links configured in the system.\n"
        "\n"
        "    rpctool help\n"
        "    rpctool -h\n"
        "    rpctool --help\n"
        "           Print this help text and exit.\n"
        "\n"
        "    All output is always sent to stdout and error messages to stderr.\n"
        "\n"
        );

        exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handles a failure to connect an IPC session with the Data Hub by reporting an error to stderr
 * and exiting with EXIT_FAILURE.
 */
//--------------------------------------------------------------------------------------------------
static void HandleConnectionError
(
    const char* serviceName,    ///< The name of the service to which we failed to connect.
    le_result_t errorCode   ///< Error result code returned by the TryConnectService() function.
)
//--------------------------------------------------------------------------------------------------
{
    fprintf(stderr, "***ERROR: Can't connect to the RPC Proxy.\n");

    switch (errorCode)
    {
        case LE_UNAVAILABLE:
            fprintf(stderr, "%s service not currently available.\n", serviceName);
            break;

        case LE_NOT_PERMITTED:
            fprintf(stderr,
                    "Missing binding to %s service.\n"
                    "System misconfiguration detected.\n",
                    serviceName);
            break;

        case LE_COMM_ERROR:
            fprintf(stderr,
                    "Service Directory is unreachable.\n"
                    "Perhaps the Service Directory is not running?\n");
            break;

        default:
            printf("Unexpected result code %d (%s)\n", errorCode, LE_RESULT_TXT(errorCode));
            break;
    }

    exit(EXIT_FAILURE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Opens IPC sessions with the RPC Proxy.
 */
//--------------------------------------------------------------------------------------------------
static void ConnectToRpcProxy
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result = le_rpc_TryConnectService();
    if (result != LE_OK)
    {
        HandleConnectionError("RPC Tool", result);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Command-line argument handler callback for the Service-Name argument.
 */
//--------------------------------------------------------------------------------------------------
static void ServiceNameArgHandler
(
    const char* arg
)
//--------------------------------------------------------------------------------------------------
{
    ServiceNameArg = arg;
}


//--------------------------------------------------------------------------------------------------
/**
 * Command-line argument handler callback for the System-Name argument.
 */
//--------------------------------------------------------------------------------------------------
static void SystemNameArgHandler
(
    const char* arg
)
//--------------------------------------------------------------------------------------------------
{
    SystemNameArg = arg;
}


//--------------------------------------------------------------------------------------------------
/**
 * Command-line argument handler callback for the Remote Service-Name argument.
 */
//--------------------------------------------------------------------------------------------------
static void RemoteServiceNameArgHandler
(
    const char* arg
)
//--------------------------------------------------------------------------------------------------
{
    RemoteServiceNameArg = arg;
}

//--------------------------------------------------------------------------------------------------
/**
 * Command-line argument handler callback for the Remote Service-Name argument.
 */
//--------------------------------------------------------------------------------------------------
static void LinkNameArgHandler
(
    const char* arg
)
//--------------------------------------------------------------------------------------------------
{
    LinkNameArg = arg;
}

//--------------------------------------------------------------------------------------------------
/**
 * Command-line argument handler callback for the Remote Service-Name argument.
 */
//--------------------------------------------------------------------------------------------------
static void NodeNameArgHandler
(
    const char* arg
)
//--------------------------------------------------------------------------------------------------
{
    NodeNameArg = arg;
}

//--------------------------------------------------------------------------------------------------
/**
 * Command-line argument handler callback for the Remote Service-Name argument.
 */
//--------------------------------------------------------------------------------------------------
static void NodeValueArgHandler
(
    const char* arg
)
//--------------------------------------------------------------------------------------------------
{
    NodeValueArg = arg;
}

//--------------------------------------------------------------------------------------------------
/**
 * Command-line argument handler callback for the object type argument (e.g., "binding", "link").
 */
//--------------------------------------------------------------------------------------------------
static void ObjectTypeArgHandler
(
    const char* arg
)
//--------------------------------------------------------------------------------------------------
{
    if (strcmp(arg, "binding") == 0)
    {
        Object = OBJECT_BINDING;

        switch (Action)
        {
            case ACTION_SET:
            {
                le_arg_AddPositionalCallback(ServiceNameArgHandler);
                le_arg_AddPositionalCallback(SystemNameArgHandler);
                le_arg_AddPositionalCallback(RemoteServiceNameArgHandler);
                break;
            }
            case ACTION_RESET:
            {
                le_arg_AddPositionalCallback(ServiceNameArgHandler);
                break;
            }
            case ACTION_GET:
            {
                le_arg_AddPositionalCallback(ServiceNameArgHandler);
                break;
            }
            default:
                fprintf(stderr, "Unknown object type '%s'.\n", arg);
                exit(EXIT_FAILURE);
                break;
        }
    }
    else if (strcmp(arg, "bindings") == 0)
    {
        Object = OBJECT_BINDING;

        switch (Action)
        {
            case ACTION_LIST:
                break;

            default:
                fprintf(stderr, "Unknown object type '%s'.\n", arg);
                exit(EXIT_FAILURE);
                break;
        }
    }
    else if (strcmp(arg, "link") == 0)
    {
        Object = OBJECT_LINK;

        switch (Action)
        {
            case ACTION_SET:
            {
                le_arg_AddPositionalCallback(SystemNameArgHandler);
                le_arg_AddPositionalCallback(LinkNameArgHandler);
                le_arg_AddPositionalCallback(NodeNameArgHandler);
                le_arg_AddPositionalCallback(NodeValueArgHandler);
                break;
            }
            case ACTION_RESET:
            {
                le_arg_AddPositionalCallback(SystemNameArgHandler);
                break;
            }
            case ACTION_GET:
            {
                le_arg_AddPositionalCallback(SystemNameArgHandler);
                le_arg_AddPositionalCallback(LinkNameArgHandler);
                le_arg_AddPositionalCallback(NodeNameArgHandler);
                break;
            }
            default:
                fprintf(stderr, "Unknown action type '%s'.\n", arg);
                exit(EXIT_FAILURE);
                break;
        }
    }
    else if (strcmp(arg, "links") == 0)
    {
        Object = OBJECT_LINK;

        switch (Action)
        {
            case ACTION_LIST:
                break;

            default:
                fprintf(stderr, "Unknown object type '%s'.\n", arg);
                exit(EXIT_FAILURE);
                break;
        }
    }
    else
    {
        fprintf(stderr, "Unknown object type '%s'.\n", arg);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Command-line argument handler call-back for the first positional argument, which is the command.
 */
//--------------------------------------------------------------------------------------------------
static void CommandArgHandler
(
    const char* arg
)
//--------------------------------------------------------------------------------------------------
{
    if (strcmp(arg, "help") == 0)
    {
        Action = ACTION_HELP;
    }
    else if (strcmp(arg, "get") == 0)
    {
        Action = ACTION_GET;

        // Expect an object type argument ("binding" or "link").
        le_arg_AddPositionalCallback(ObjectTypeArgHandler);
    }
    else if (strcmp(arg, "list") == 0)
    {
        Action = ACTION_LIST;

        // Expect an object type argument ("binding" or "link").
        le_arg_AddPositionalCallback(ObjectTypeArgHandler);
    }
    else if (strcmp(arg, "set") == 0)
    {
        Action = ACTION_SET;

        // Expect an object type argument ("binding" or "link").
        le_arg_AddPositionalCallback(ObjectTypeArgHandler);
    }
    else if (strcmp(arg, "reset") == 0)
    {
        Action = ACTION_RESET;

        // Expect an object type argument ("binding" or "link").
        le_arg_AddPositionalCallback(ObjectTypeArgHandler);
    }
    else
    {
        fprintf(stderr, "Unrecognized command '%s'.  Try 'rpcTool help' for assistance.\n", arg);
        exit(EXIT_FAILURE);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to display all "bindings".
 */
//--------------------------------------------------------------------------------------------------
static void PrintAllBindings
(
    void
)
{
    char serviceName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];
    char systemName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];
    char remoteServiceName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];
    le_result_t result;

    result = le_rpc_GetFirstBinding(serviceName, sizeof(serviceName));
    if (result != LE_OK)
    {
        return;
    }

    printf("================================================"
           "================================================\n");

    do
    {
        // Get the binding, using the service-name
        result = le_rpc_GetBinding(serviceName,
                                   systemName,
                                   sizeof(systemName),
                                   remoteServiceName,
                                   sizeof(remoteServiceName));

        if (result == LE_OK)
        {
            printf("Service-Name: %s, System-Name: %s, "
                   "Remote Service-Name: %s\n",
                   serviceName,
                   systemName,
                   remoteServiceName);
        }
        else
        {
            printf("Configuration not found!\n");
        }
    }
    while (le_rpc_GetNextBinding(
               serviceName,
               serviceName,
               sizeof(serviceName)) == LE_OK);

    printf("================================================"
           "================================================\n");
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to display all system "links".
 */
//--------------------------------------------------------------------------------------------------
static void PrintAllLinks
(
    void
)
{
    char systemName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];
    char linkName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];
    char nodeName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];
    char nodeValue[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];
    le_result_t result;

    result = le_rpc_GetFirstSystemLink(systemName,
                                       sizeof(systemName),
                                       linkName,
                                       sizeof(linkName),
                                       nodeName,
                                       sizeof(nodeName));
    if (result != LE_OK)
    {
        return;
    }

    printf("================================================"
           "================================================\n");

    do
    {
        // Get the system link, using the system, link, and node names
        result = le_rpc_GetSystemLink(systemName,
                                      linkName,
                                      nodeName,
                                      nodeValue,
                                      sizeof(nodeValue));

        if (result == LE_OK)
        {
            printf("System-Name: %s, Link-Name: %s, "
                   "Node-Name: %s, Node-Value: %s\n",
                   systemName,
                   linkName,
                   nodeName,
                   nodeValue);
        }
        else
        {
            printf("Configuration not found!\n");
        }
    }
    while (le_rpc_GetNextSystemLink(
               systemName,
               linkName,
               nodeName,
               systemName,
               sizeof(systemName),
               linkName,
               sizeof(linkName),
               nodeName,
               sizeof(nodeName)) == LE_OK);

    printf("================================================"
           "================================================\n");
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initializer.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_result_t result;

    le_arg_SetFlagCallback(HandleHelpRequest, "h", "help");

    le_arg_AddPositionalCallback(CommandArgHandler);

    le_arg_Scan();

    ConnectToRpcProxy();

    switch (Action)
    {
        case ACTION_HELP:
            HandleHelpRequest();
            break;

        case ACTION_GET:
            switch (Object)
            {
                case OBJECT_BINDING:
                {
                    char systemName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];
                    char remoteServiceName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];
                    result = le_rpc_GetBinding(ServiceNameArg,
                                               systemName,
                                               sizeof(systemName),
                                               remoteServiceName,
                                               sizeof(remoteServiceName));

                    if (result == LE_OK)
                    {
                        printf("================================================"
                               "================================================\n");
                        printf("Service-Name: %s, System-Name: %s, "
                                "Remote Service-Name: %s\n",
                               ServiceNameArg,
                               systemName,
                               remoteServiceName);
                        printf("================================================"
                               "================================================\n");
                    }
                    else
                    {
                        printf("Configuration not found!\n");
                    }
                    break;
                }

                case OBJECT_LINK:
                {
                    char nodeValue[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];
                    result = le_rpc_GetSystemLink(SystemNameArg,
                                                  LinkNameArg,
                                                  NodeNameArg,
                                                  nodeValue,
                                                  sizeof(nodeValue));

                    if (result == LE_OK)
                    {
                        printf("================================================"
                               "================================================\n");
                        printf("Systeme-Name: %s, Link-Name: %s, "
                                "Node-Name: %s, Node-Value: %s\n",
                               SystemNameArg,
                               LinkNameArg,
                               NodeNameArg,
                               nodeValue);
                        printf("================================================"
                               "================================================\n");
                    }
                    else
                    {
                        printf("Configuration not found!\n");
                    }
                    break;
                }

                default:
                    break;
            }
            break;

        case ACTION_LIST:
            switch (Object)
            {
                case OBJECT_BINDING:
                {
                    PrintAllBindings();
                    break;
                }

                case OBJECT_LINK:
                {
                    PrintAllLinks();
                    break;
                }

                default:
                    break;
            }
            break;

        case ACTION_SET:
            switch (Object)
            {
                case OBJECT_BINDING:
                    le_rpc_SetBinding(ServiceNameArg, SystemNameArg, RemoteServiceNameArg);
                    break;

                case OBJECT_LINK:
                    le_rpc_SetSystemLink(SystemNameArg, LinkNameArg, NodeNameArg, NodeValueArg);
                    break;

                default:
                    break;
            }
            break;

        case ACTION_RESET:
            switch (Object)
            {
                case OBJECT_BINDING:
                    le_rpc_ResetBinding(ServiceNameArg);
                    break;

                case OBJECT_LINK:
                    le_rpc_ResetSystemLink(SystemNameArg);
                    break;

                default:
                    break;
            }
            break;

        default:

            LE_FATAL("Unimplemented action.");
            break;
    }

    exit(EXIT_SUCCESS);
}
