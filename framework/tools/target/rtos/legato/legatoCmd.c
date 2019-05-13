//--------------------------------------------------------------------------------------------------
/** @file legatoCmd.c
 *
 * Legato tool for RTOS.  Currently this just provides verison information.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Display usage information.
 **/
//--------------------------------------------------------------------------------------------------
static void DisplayHelp(void)
{
    printf(
        "NAME\n"
        "  legato - Use the legato tool to control the Legato Application Framework.\n\n"
        "SYNOPSIS\n"
        "  legato [version|help]\n\n"
        "DESCRIPTION\n"
        "    legato version\n"
        "       Displays the current installed version.\n"
        "    legato help\n"
        "       Displays usage help.\n"
    );
}

//--------------------------------------------------------------------------------------------------
/**
 * Display Legato version.
 **/
//--------------------------------------------------------------------------------------------------
static void DisplayVersion(void)
{
    printf("%s\n", LE_VERSION);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function called by command line argument scanner when the command argument is found.
 **/
//--------------------------------------------------------------------------------------------------
static void CommandArgHandler
(
    const char *command
)
{
    if (strcmp(command, "version") == 0)
    {
        DisplayVersion();
    }
    else if (strcmp(command, "help") == 0)
    {
        DisplayHelp();
    }
    else
    {
        fprintf(stderr, "Invalid command '%s'.\n", command);
        DisplayHelp();
    }
}

//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // The command-line has a command string.
    le_arg_AddPositionalCallback(&CommandArgHandler);
    le_arg_Scan();
}
