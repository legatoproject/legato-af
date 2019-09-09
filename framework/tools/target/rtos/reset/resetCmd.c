//--------------------------------------------------------------------------------------------------
/** @file resetCmd.c
 *
 * Reboot the machine.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * true = exit command ASAP.
 **/
//--------------------------------------------------------------------------------------------------
static bool IsExiting;

//--------------------------------------------------------------------------------------------------
/**
 * Display usage information.
 **/
//--------------------------------------------------------------------------------------------------
static void PrintHelp
(
    void
)
{
    printf(
        "NAME:\n"
        "    reset - Reboot the machine.\n"
        "\n"
        "SYNOPSIS:\n"
        "    reset [OPTIONS]\n"
        "\n"
        "DESCRIPTION:\n"
        "    reset   Reboot the machine.\n"
        "\n"
        "OPTIONS:\n"
        "    -h, --help\n"
        "        Display this help and exit.\n"
    );

    IsExiting = true;
}

//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    IsExiting = false;

    le_arg_SetFlagCallback(&PrintHelp, "h", "help");
    le_arg_Scan();

    if (IsExiting)
    {
        return;
    }

    reboot(RB_AUTOBOOT);
}
