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
 * Maximum output string length.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_TOOL_STRING_BUFFER_MAX    60


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
 * Ptr to the Parameters command-line argument, or NULL if there wasn't one provided.
 */
//--------------------------------------------------------------------------------------------------
static const char* ParametersArg = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Function to handle exiting from the RPC Tool.
 */
//--------------------------------------------------------------------------------------------------
static void ExitTool
(
    int exitCode  ///< Exit code
)
{
#ifndef LE_CONFIG_RTOS
        exit(exitCode);
#else
        LE_UNUSED(exitCode);
        le_thread_Exit(NULL);
#endif
}

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
        "    rpctool set link <systemName> <linkName> <parameters>\n"
        "    rpctool get link <systemName>\n"
        "    rpctool reset link <systemName>\n"
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
        "            Retrieves the system-name, remote service-name, and status\n"
        "            for the specified service.\n"
        "\n"
        "    rpctool reset binding <serviceName>\n"
        "            Resets the RPC binding for a given service-name.\n"
        "\n"
        "    rpctool list bindings\n"
        "            Lists all RPC bindings configured in the system.\n"
        "\n"
        "    rpctool set link <systemName> <linkName> <parameters>\n"
        "            Sets the RPC link-name and link-parameters (argument string)\n"
        "            for the specified system.\n"
        "\n"
        "    rpctool get link <systemName>\n"
        "            Retrieves the link-name, link-parameters, and status\n"
        "            for the specified system-name.\n"
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

        ExitTool(EXIT_SUCCESS);
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

    ExitTool(EXIT_FAILURE);
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
 * Command-line argument handler callback for the Link-Name argument.
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
 * Command-line argument handler callback for the Parameters argument.
 */
//--------------------------------------------------------------------------------------------------
static void ParametersArgHandler
(
    const char* arg
)
//--------------------------------------------------------------------------------------------------
{
    ParametersArg = arg;
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
                ExitTool(EXIT_FAILURE);
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
                ExitTool(EXIT_FAILURE);
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
                le_arg_AddPositionalCallback(ParametersArgHandler);
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
                break;
            }
            default:
                fprintf(stderr, "Unknown action type '%s'.\n", arg);
                ExitTool(EXIT_FAILURE);
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
                ExitTool(EXIT_FAILURE);
                break;
        }
    }
    else
    {
        fprintf(stderr, "Unknown object type '%s'.\n", arg);
        ExitTool(EXIT_FAILURE);
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

        // Expect an object type argument ("bindings" or "links").
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
        fprintf(stderr, "Unrecognized command '%s'.  Try 'rpctool help' for assistance.\n", arg);
        ExitTool(EXIT_FAILURE);
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
    char systemName[LIMIT_MAX_SYSTEM_NAME_BYTES];
    char remoteServiceName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];
    uint32_t serviceId;
    le_result_t result;

    result = le_rpc_GetFirstSystemBinding(serviceName, sizeof(serviceName));
    if (result != LE_OK)
    {
        return;
    }

    printf("\n========================================= RPC Bindings "
           "=========================================\n");

    do
    {
        // Get the binding, using the service-name
        result = le_rpc_GetSystemBinding(
                    serviceName,
                    systemName,
                    sizeof(systemName),
                    remoteServiceName,
                    sizeof(remoteServiceName),
                    &serviceId);

        if (result == LE_OK)
        {
            char strBuffer[RPC_TOOL_STRING_BUFFER_MAX];
            snprintf(strBuffer,
                     sizeof(strBuffer),
                     "CONNECTED, Service-ID: %-10" PRIu32,
                     serviceId);

            printf("\nService-Name: %-40s Status: %s\n"
                   "    System-Name: %s\n"
                   "    Remote Service-Name: %s\n",
                   serviceName,
                   (serviceId == 0) ? "NOT CONNECTED" : strBuffer,
                   systemName,
                   remoteServiceName);
        }
        else
        {
            printf("Configuration not found!\n");
        }
    }
    while (le_rpc_GetNextSystemBinding(
               serviceName,
               serviceName,
               sizeof(serviceName)) == LE_OK);

    printf("\n================================================"
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
    char systemName[LIMIT_MAX_SYSTEM_NAME_BYTES];
    char linkName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];
    char parameters[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];
    le_rpc_NetworkState_t state;
    le_result_t result;

    result = le_rpc_GetFirstSystemLink(systemName, sizeof(systemName));
    if (result != LE_OK)
    {
        return;
    }

    printf("\n========================================== RPC Links "
           "===========================================\n");

    do
    {
        // Get the system link, using the system, link, and node names
        result = le_rpc_GetSystemLink(systemName,
                                      linkName,
                                      sizeof(linkName),
                                      parameters,
                                      sizeof(parameters),
                                      &state);

        if (result == LE_OK)
        {
           char strBuffer[RPC_TOOL_STRING_BUFFER_MAX];
           switch (state)
            {
                case LE_RPC_NETWORK_UP:
                    snprintf(strBuffer,
                             sizeof(strBuffer),
                             "UP");
                    break;
                case LE_RPC_NETWORK_DOWN:
                    snprintf(strBuffer,
                             sizeof(strBuffer),
                             "DOWN");
                    break;

                case LE_RPC_NETWORK_UNKNOWN:
                default:
                    snprintf(strBuffer,
                             sizeof(strBuffer),
                             "NOT STARTED");
                    break;
            }

            printf("\nSystem-Name: %-40s  Status: %s\n"
                   "    Link-Name: %s\n"
                   "    Parameters: %s\n",
                   systemName,
                   strBuffer,
                   linkName,
                   parameters);
        }
        else
        {
            printf("Configuration not found!\n");
        }
    }
    while (le_rpc_GetNextSystemLink(
               systemName,
               systemName,
               sizeof(systemName)) == LE_OK);

    printf("\n================================================"
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
                    char systemName[LIMIT_MAX_SYSTEM_NAME_BYTES];
                    char remoteServiceName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];
                    uint32_t serviceId;
                    result = le_rpc_GetSystemBinding(
                                ServiceNameArg,
                                systemName,
                                sizeof(systemName),
                                remoteServiceName,
                                sizeof(remoteServiceName),
                                &serviceId);

                    if (result == LE_OK)
                    {
                        char strBuffer[RPC_TOOL_STRING_BUFFER_MAX];
                        printf("\n========================================= RPC Binding "
                               "==========================================\n");

                        snprintf(strBuffer,
                                 sizeof(strBuffer),
                                 "CONNECTED, Service-ID: %-10" PRIu32,
                                 serviceId);

                        printf("\nService-Name: %-40s Status: %s\n"
                               "    System-Name: %s\n"
                               "    Remote Service-Name: %s\n",
                               ServiceNameArg,
                               (serviceId == 0) ? "NOT CONNECTED" : strBuffer,
                               systemName,
                               remoteServiceName);
                        printf("\n================================================"
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
                    char linkName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];
                    char parameters[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];
                    le_rpc_NetworkState_t state;
                    result = le_rpc_GetSystemLink(SystemNameArg,
                                                  linkName,
                                                  sizeof(linkName),
                                                  parameters,
                                                  sizeof(parameters),
                                                  &state);

                    if (result == LE_OK)
                    {
                        char strBuffer[RPC_TOOL_STRING_BUFFER_MAX];
                        printf("\n========================================== RPC Link "
                               "============================================\n");

                        switch (state)
                        {
                            case LE_RPC_NETWORK_UP:
                                snprintf(strBuffer,
                                         sizeof(strBuffer),
                                         "UP");
                                break;
                            case LE_RPC_NETWORK_DOWN:
                                snprintf(strBuffer,
                                         sizeof(strBuffer),
                                         "DOWN");
                                break;

                            case LE_RPC_NETWORK_UNKNOWN:
                            default:
                                snprintf(strBuffer,
                                         sizeof(strBuffer),
                                         "NOT STARTED");
                                break;
                        }

                        printf("\nSystem-Name: %-40s  Status: %s\n"
                               "    Link-Name: %s\n"
                               "    Parameters: %s\n",
                               SystemNameArg,
                               strBuffer,
                               linkName,
                               parameters);
                        printf("\n================================================"
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
                    le_rpc_SetSystemBinding(ServiceNameArg,
                                            SystemNameArg,
                                            RemoteServiceNameArg);
                    break;

                case OBJECT_LINK:
                    le_rpc_SetSystemLink(SystemNameArg,
                                         LinkNameArg,
                                         ParametersArg);
                    break;

                default:
                    break;
            }
            break;

        case ACTION_RESET:
            switch (Object)
            {
                case OBJECT_BINDING:
                    le_rpc_ResetSystemBinding(ServiceNameArg);
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

    ExitTool(EXIT_SUCCESS);
}
