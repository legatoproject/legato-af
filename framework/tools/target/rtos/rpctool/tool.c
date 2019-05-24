//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the RPC Configuration 'shim' tool.  It provides support to run the
 * RPC Configuration Tool process using the the Micro-Supervisor's
 * "app runProc appName procName [-- <args> ]" feature.
 *
 * NOTE:  This is seen as a temporary work-around for the HL78, which does not currently allow
 * command-line tools to invoke Legato API function calls from within a tool.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "interfaces.h"
#include "legato.h"
#include "microSupervisor.h"

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
    OBJECT_UNSPECIFIED,
    OBJECT_BINDING,
    OBJECT_LINK,
}
Object = OBJECT_UNSPECIFIED;

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
 * Print help text to stdout.
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
        "            for the specified system.\n"
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
                fprintf(stderr,
                        "Unknown object type '%s'.  Try 'rpcTool help' for assistance.\n",
                        arg);

                // Reset the Action and Object types
                Action = ACTION_UNSPECIFIED;
                Object = OBJECT_UNSPECIFIED;
                return;
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
                fprintf(stderr,
                        "Unknown object type '%s'.  Try 'rpcTool help' for assistance.\n",
                        arg);

                // Reset the Action and Object types
                Action = ACTION_UNSPECIFIED;
                Object = OBJECT_UNSPECIFIED;
                return;
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
                fprintf(stderr,
                        "Unknown object type '%s'.  Try 'rpcTool help' for assistance.\n",
                        arg);

                // Reset the Action and Object types
                Action = ACTION_UNSPECIFIED;
                Object = OBJECT_UNSPECIFIED;
                return;
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
                fprintf(stderr,
                        "Unknown object type '%s'.  Try 'rpcTool help' for assistance.\n",
                        arg);

                // Reset the Action and Object types
                Action = ACTION_UNSPECIFIED;
                Object = OBJECT_UNSPECIFIED;
                return;
                break;
        }
    }
    else
    {
        fprintf(stderr,
                "Unknown object type '%s'.  Try 'rpcTool help' for assistance.\n",
                arg);

        // Reset the Action type
        Action = ACTION_UNSPECIFIED;
        return;
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
        fprintf(stderr, "Unrecognized command '%s'.  Try 'rpcTool help' for assistance.\n", arg);

        // Reset the Action type
        Action = ACTION_UNSPECIFIED;
        return;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Component initializer.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Initialize Globals
    Action = ACTION_UNSPECIFIED;
    Object = OBJECT_UNSPECIFIED;
    ServiceNameArg = NULL;
    SystemNameArg = NULL;
    LinkNameArg = NULL;
    ParametersArg = NULL;
    RemoteServiceNameArg = NULL;

    le_arg_SetFlagCallback(HandleHelpRequest, "h", "help");
    le_arg_AddPositionalCallback(CommandArgHandler);
    le_arg_Scan();

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
                    // Check the arguments are valid
                    if (ServiceNameArg == NULL)
                    {
                        fprintf(stderr,
                                "Too few arguments.  Try 'rpcTool help' for assistance.\n");
                        return;
                    }

                    const char *argv[] = {
                        "get",
                        "binding",
                        ServiceNameArg,
                        NULL,
                    };
                    // Call the microSupervisor to execute the
                    // 'get binding' command on the rpcTool daemon
                    le_microSupervisor_RunProc("tools", "rpcTool", 3, argv);
                    break;
                }

                case OBJECT_LINK:
                {
                    // Check the arguments are valid
                    if (SystemNameArg == NULL)
                    {
                        fprintf(stderr,
                                "Too few arguments.  Try 'rpcTool help' for assistance.\n");
                        return;
                    }

                    const char *argv[] = {
                        "get",
                        "link",
                        SystemNameArg,
                        NULL,
                    };
                    // Call the microSupervisor to execute the
                    // 'get link' command on the rpcTool daemon
                    le_microSupervisor_RunProc("tools", "rpcTool", 3, argv);
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
                    const char *argv[] = {
                        "list",
                        "bindings",
                        NULL,
                    };
                    // Call the microSupervisor to execute the
                    // 'list bindings' command on the rpcTool daemon
                    le_microSupervisor_RunProc("tools", "rpcTool", 2, argv);
                    break;
                }

                case OBJECT_LINK:
                {
                    const char *argv[] = {
                        "list",
                        "links",
                        NULL,
                    };
                    // Call the microSupervisor to execute the
                    // 'list links' command on the rpcTool daemon
                    le_microSupervisor_RunProc("tools", "rpcTool", 2, argv);
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
                {
                    // Check the arguments are valid
                    if ((ServiceNameArg == NULL) ||
                        (SystemNameArg == NULL) ||
                        (RemoteServiceNameArg == NULL))
                    {
                        fprintf(stderr,
                                "Too few arguments.  Try 'rpcTool help' for assistance.\n");
                        return;
                    }

                    const char *argv[] = {
                        "set",
                        "binding",
                        ServiceNameArg,
                        SystemNameArg,
                        RemoteServiceNameArg,
                        NULL,
                    };
                    // Call the microSupervisor to execute the
                    // 'set binding' command on the rpcTool daemon
                    le_microSupervisor_RunProc("tools", "rpcTool", 5, argv);
                    break;
                }

                case OBJECT_LINK:
                {
                    // Check the arguments are valid
                    if ((SystemNameArg == NULL) ||
                        (LinkNameArg == NULL) ||
                        (ParametersArg == NULL))
                    {
                        fprintf(stderr,
                                "Too few arguments.  Try 'rpcTool help' for assistance.\n");
                        return;
                    }

                    const char *argv[] = {
                        "set",
                        "link",
                        SystemNameArg,
                        LinkNameArg,
                        ParametersArg,
                        NULL,
                    };
                    // Call the microSupervisor to execute the
                    // 'set link' command on the rpcTool daemon
                    le_microSupervisor_RunProc("tools", "rpcTool", 5, argv);
                    break;
                }

                default:
                    break;
            }
            break;

        case ACTION_RESET:
            switch (Object)
            {
                case OBJECT_BINDING:
                {
                    // Check the arguments are valid
                    if (ServiceNameArg == NULL)
                    {
                        fprintf(stderr,
                                "Too few arguments.  Try 'rpcTool help' for assistance.\n");
                        return;
                    }

                    const char *argv[] = {
                        "reset",
                        "binding",
                        ServiceNameArg,
                        NULL,
                    };
                    // Call the microSupervisor to execute the
                    // 'reset binding' command on the rpcTool daemon
                    le_microSupervisor_RunProc("tools", "rpcTool", 3, argv);
                    break;
                }

                case OBJECT_LINK:
                {
                    // Check the arguments are valid
                    if (SystemNameArg == NULL)
                    {
                        fprintf(stderr,
                                "Too few arguments.  Try 'rpcTool help' for assistance.\n");
                        return;
                    }

                    const char *argv[] = {
                        "reset",
                        "link",
                        SystemNameArg,
                        NULL,
                    };
                    // Call the microSupervisor to execute the
                    // 'reset link' command on the rpcTool daemon
                    le_microSupervisor_RunProc("tools", "rpcTool", 3, argv);
                    break;
                }

                default:
                    break;
            }
            break;

        default:
            break;
    }
}
