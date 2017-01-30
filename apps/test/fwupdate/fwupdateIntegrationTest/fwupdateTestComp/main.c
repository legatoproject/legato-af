 /**
  * This module is for integration testing of the fwupdate component (dual system case)
  *
  * * You must issue the following commands:
  * @verbatim
  * $ app start fwupdateTest
  * $ app runProc fwupdateTest --exe=fwupdateTest -- <arg1> [<arg2>]
  *
  * Example:
  * $ app runProc fwupdateTest --exe=fwupdateTest -- help
  * @endverbatim
  *
  * Copyright (C) Sierra Wireless Inc.
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
    char string[100] = {0};

    LE_INFO("Start fwupdate app.");

    /* Get the test identifier */
    if (le_arg_NumArgs() >= 1)
    {
        testString = le_arg_GetArg(0);

        if (le_arg_NumArgs() >= 2)
        {
            secondString = le_arg_GetArg(1);
        }
    }

    if (0 == strcmp(testString, "help"))
    {
        PrintUsage();
        exit(0);
    }
    else if (0 == strcmp(testString, "sync_state"))
    {
        bool isSync;
        result = le_fwupdate_DualSysSyncState (&isSync);
        snprintf(string, sizeof(string), "fwupdateTest: sync_state -> result %d isSync %d", result, isSync);
        Print (string);
        exit(0);
    }
    else if (0 == strcmp(testString, "do_sync"))
    {
        bool isSync;
        result = le_fwupdate_DualSysSync ();
        snprintf(string, sizeof(string), "fwupdateTest: Sync -> result %d", result);
        Print (string);
        result = le_fwupdate_DualSysSyncState (&isSync);
        snprintf(string, sizeof(string), "fwupdateTest: sync_state -> result %d isSync %d", result, isSync);
        Print (string);
        exit(0);
    }
    else if (0 == strcmp(testString, "file"))
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
            snprintf(string, sizeof(string), "le_fwupdate_Download %d", result);
            Print (string);
            close (fd);
        }
        exit(0);
    }
    else if (0 == strcmp(testString, "do_swapsync"))
    {
        result = le_fwupdate_DualSysSwapAndSync();
        snprintf(string, sizeof(string), "le_fwupdate_DualSysSwapAndSync %d", result);
        Print (string);
        exit(0);
    }
    else if (0 == strcmp(testString, "do_swap"))
    {
        result = le_fwupdate_DualSysSwap();
        snprintf(string, sizeof(string), "le_fwupdate_DualSysSwap %d", result);
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
