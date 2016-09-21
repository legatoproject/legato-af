 /**
  * This module is for integration testing of the fwupdate component (dual system case)
  *
  * * You must issue the following commands:
  * @verbatim
  * $ app start fwupdateTest
  * $ execInApp fwupdateTest fwupdateTest <arg1> [<arg2>]
  *
  * Example:
  * $ execInApp fwupdateTest fwupdateTest help
  * @endverbatim
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */

#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Print function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Print
(
    char* string
)
{
    bool sandboxed = (getuid() != 0);

    if(sandboxed)
    {
        LE_INFO("%s", string);
    }
    else
    {
        fprintf(stderr, "%s\n", string);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Help.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage
(
    void
)
{
    int idx;
    const char * usagePtr[] = {
            "Usage of the 'fwupdateTest' application is:",
            "fwupdateTest -- sync_state: get the sub system synchronization state",
            "fwupdateTest -- file <path>: read a CWE file from path",
            "fwupdateTest -- do_swap: make a swap and reboot the device",
            "fwupdateTest -- do_sync: synchronize the sub systems",
            "fwupdateTest -- do_swapsync: make a Swap & Sync operation",
            "",
    };

    for(idx = 0; idx < NUM_ARRAY_MEMBERS(usagePtr); idx++)
    {
        Print((char*) usagePtr[idx]);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Main thread.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    const char* testString = "";
    const char* secondString = "";
    le_result_t result;
    char string[100];

    LE_INFO("Start fwupdate app.");
    memset (string, 0, 100);

    /* Get the test identifier */
    if (le_arg_NumArgs() >= 1)
    {
        testString = le_arg_GetArg(0);

        if (le_arg_NumArgs() >= 2)
        {
            secondString = le_arg_GetArg(1);
        }
    }

    if (strcmp(testString, "help") == 0)
    {
        PrintUsage();
        exit(0);
    }
    else if (strcmp(testString, "sync_state") == 0)
    {
        bool isSync;
        result = le_fwupdate_DualSysSyncState (&isSync);
        sprintf (string,"fwupdateTest: sync_state -> result %d isSync %d", result, isSync);
        Print (string);
        exit(0);
    }
    else if (strcmp(testString, "do_sync") == 0)
    {
        bool isSync;
        result = le_fwupdate_DualSysSync ();
        sprintf (string, "fwupdateTest: Sync -> result %d", result);
        Print (string);
        result = le_fwupdate_DualSysSyncState (&isSync);
        sprintf (string, "fwupdateTest: sync_state -> result %d isSync %d", result, isSync);
        Print (string);
        exit(0);
    }
    else if (strcmp(testString, "file") == 0)
    {
        int fd = 0;
        LE_INFO ("file to read: secondString %s", secondString);
        fd = open (secondString, O_RDONLY);
        if( fd < 0 )
        {
            LE_ERROR ("Bad fd %d", fd);
        }
        else
        {
            result = le_fwupdate_Download (fd);
            sprintf (string, "le_fwupdate_Download %d", result);
            Print (string);
            close (fd);
        }
        exit(0);
    }
    else if (strcmp(testString, "do_swapsync") == 0)
    {
        result = le_fwupdate_DualSysSwapAndSync();
        sprintf (string, "le_fwupdate_DualSysSwapAndSync %d", result);
        Print (string);
        exit(0);
    }
    else if (strcmp(testString, "do_swap") == 0)
    {
        result = le_fwupdate_DualSysSwap();
        sprintf (string, "le_fwupdate_DualSysSwap %d", result);
        Print (string);
        exit(0);
    }
    else
    {
        LE_DEBUG ("fwupdateTest: not supported arg");
        PrintUsage();
        exit(EXIT_FAILURE);
    }
}
