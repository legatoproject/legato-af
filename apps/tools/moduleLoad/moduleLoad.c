
//--------------------------------------------------------------------------------------------------
/*
 * Simple command line tool that allows the command line to load and unload Legato bundled modules.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"



// -------------------------------------------------------------------------------------------------
/**
 *  Simply write the usage text to the console.
 */
// -------------------------------------------------------------------------------------------------
static void PrintHelp
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    const char* programName = le_arg_GetProgramName();

    printf("Load/Unload a Legato bundled kernel module.\n"
           "\n"
           "  Usage:\n"
           "\n"
           "  To load a module:\n"
           "\n"
           "      %1$s load <moduleName>\n"
           "\n"
           "  To unload a module:\n"
           "\n"
           "      %1$s unload <moduleName>\n"
           "\n",
           programName);
}



// -------------------------------------------------------------------------------------------------
/**
 *  Call the API function and print a success or failure message.
 *
 *  @return
 *      - EXIT_SUCCESS if all went to plan.
 *      - EXIT_FAILURE if we encounter errors during loading/unloading.
 */
// -------------------------------------------------------------------------------------------------
static int CallApi
(
    const char* command,                             ///< [IN] The name of the command to run.
    const char* kmodName,                            ///< [IN] Name of the module to work with.
    le_result_t (*functionPtr)(const char* namePtr)  ///< [IN] The function to work with that
                                                     ///<      module.
)
// -------------------------------------------------------------------------------------------------
{
    // Call the function, check the result.  Print the right message.  Return the error code.
    le_result_t result = (*functionPtr)(kmodName);

    if (result != LE_OK)
    {
        fprintf(stderr,
                "Could not %s the required module, %s. (%s)\n"
                "See the device log for details.\n",
                command,
                kmodName,
                LE_RESULT_TXT(result));

        return EXIT_FAILURE;
    }

    // Because our commands are all lower case, and we're starting the sentence with it, let's make
    // the first character uppercase.
    char* modifiedCmdStr = strdup(command);

    modifiedCmdStr[0] = toupper(modifiedCmdStr[0]);
    printf("%s of module %s has been successful.\n", modifiedCmdStr, kmodName);
    free(modifiedCmdStr);

    return EXIT_SUCCESS;
}



// -------------------------------------------------------------------------------------------------
/**
 *  Process the arguments from the command line.  We currently handle loading/unloading and
 *  displaying help.
 *
 *  @return
 *      - EXIT_SUCCESS if all went to plan.
 *      - EXIT_FAILURE if we encounter errors during loading/unloading.
 */
// -------------------------------------------------------------------------------------------------
static int HandleCommand
(
    int numArgs
)
// -------------------------------------------------------------------------------------------------
{
    const char* command = le_arg_GetArg(0);

    if (command == NULL)
    {
        fprintf(stderr, "Internal Error: Argument processor failed to return argument.\n");
        return EXIT_FAILURE;
    }

    // If the user is asking for help, give it to them now.
    if (   (strcmp(command, "help") == 0)
        || (strcmp(command, "--help") == 0)
        || (strcmp(command, "-h") == 0))
    {
        PrintHelp();
        return EXIT_SUCCESS;
    }

    // From here on in, we only have commands that need two args, so if we don't have that, it's a
    // problem.
    if (numArgs != 2)
    {
        fprintf(stderr, "Wrong number of arguments.\n");
        PrintHelp();
        return EXIT_FAILURE;
    }

    // Get the name of the kernel module we're working with.
    const char* kmodName = le_arg_GetArg(1);

    if (kmodName == NULL)
    {
        fprintf(stderr, "Internal Error: Argument processor failed to return argument.\n");
        return EXIT_FAILURE;
    }

    // Are we handling a load?
    if (strcmp(command, "load") == 0)
    {
        return CallApi(command, kmodName, &le_kernelModule_Load);
    }

    // How about an unload?
    if (strcmp(command, "unload") == 0)
    {
        return CallApi(command, kmodName, &le_kernelModule_Unload);
    }

    // The command wasn't handled, so report this, print the help and exit.
    fprintf(stderr, "Unrecognized command, '%s'.\n", command);
    PrintHelp();

    return EXIT_FAILURE;
}


COMPONENT_INIT
{
    int numArgs = le_arg_NumArgs();
    int exitCode = EXIT_SUCCESS;

    switch (numArgs)
    {
        // Either asking for help, or to deal with a module.
        case 1:
        case 2:
            exitCode = HandleCommand(numArgs);
            break;

        // Totally the wrong number of args.
        default:
            exitCode = EXIT_FAILURE;
            if (numArgs > 0)
            {
                fprintf(stderr, "Wrong number of arguments.\n");
            }
            PrintHelp();
            break;
    }

    // Let the calling process know how things went.
    exit(exitCode);
}
