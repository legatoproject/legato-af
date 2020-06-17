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

#include "limit.h"
#include "legato.h"
#include "microSupervisor.h"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum allowed number of command line arguments.
 */
//--------------------------------------------------------------------------------------------------
#define RPCTOOL_MAX_ARGC               7

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of command line arguments.
 */
//--------------------------------------------------------------------------------------------------
#define RPCTOOL_MAX_ARGS_STR_BYTES     48

//--------------------------------------------------------------------------------------------------
/**
 * Command-line Argument Vector.
 */
//--------------------------------------------------------------------------------------------------
static char argv[RPCTOOL_MAX_ARGC + 1][RPCTOOL_MAX_ARGS_STR_BYTES];


//--------------------------------------------------------------------------------------------------
/**
 * Component initializer.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    int argc = le_arg_NumArgs();
    const char *argvPtr[RPCTOOL_MAX_ARGC + 1] = {0};

    // Verify command-line argument count is within range
    if (argc > MAX_ARGC)
    {
        LE_ERROR("Number of support arguments exceed");
        return;
    }

    for (int i = 0; i < argc; i++)
    {
        // Copy the argument string in global buffer
        le_utf8_Copy(argv[i], le_arg_GetArg(i), sizeof(argv[i]), NULL);

        // Set char* pointer to argument string
        argvPtr[i] = argv[i];
    }

    // Set last argument to NULL
    argvPtr[argc] = NULL;

    // Call the microSupervisor to execute command-line
    // arguments on the rpcTool daemon
    le_microSupervisor_RunProc("tools", "rpcTool", argc, argvPtr);
}
