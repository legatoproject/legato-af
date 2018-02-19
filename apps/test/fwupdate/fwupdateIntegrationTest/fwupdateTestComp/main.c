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
 * Valid value for the system: 1 or 2
 *
 */
//--------------------------------------------------------------------------------------------------
#define SYSTEM_1 1
#define SYSTEM_2 2

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
            "fwupdateTest -- is_good: get the sub system synchronization state",
            "fwupdateTest -- file <path>: read a CWE file from path",
            "fwupdateTest -- do_install: make a swap and reboot the device",
            "fwupdateTest -- do_markgood: synchronize the sub systems",
            "fwupdateTest -- do_install_markgood: make a Swap & Sync operation",
            "fwupdateTest -- do_initdwnld: make an init download operation",
            "fwupdateTest -- do_disablesync: disable the sync before update",
            "fwupdateTest -- do_enablesync: enable the sync before update",
            "fwupdateTest -- do_getsystem: get the current sub system as: <modem>,<lk>,<linux>",
            "fwupdateTest -- do_setsystem <modem>,<lk>,<linux>: define the new sub system",
            "",
    };

    for(idx = 0; idx < NUM_ARRAY_MEMBERS(usagePtr); idx++)
    {
        Print((char*) usagePtr[idx]);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test if the given system ID is valid. In the test, we accept 1 or 2 as valid values.
 *
 */
//--------------------------------------------------------------------------------------------------
static bool IsSubSystemValid
(
    int ssid
)
{
    return ((SYSTEM_1 == ssid) || (SYSTEM_2 == ssid));
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
    memset (string, 0, sizeof(string));

    /* Get the test identifier */
    if (le_arg_NumArgs() >= 1)
    {
        testString = le_arg_GetArg(0);
        if (NULL == testString)
        {
            LE_ERROR("testString is NULL");
            exit(EXIT_FAILURE);
        }

        if (le_arg_NumArgs() >= 2)
        {
            secondString = le_arg_GetArg(1);
            if (NULL == secondString)
            {
                LE_ERROR("secondString is NULL");
                exit(EXIT_FAILURE);
            }
        }
    }

    if (0 == strcmp(testString, "help"))
    {
        PrintUsage();
        exit(0);
    }
    else if (0 == strcmp(testString, "is_good"))
    {
        bool isSystemGood;
        result = le_fwupdate_IsSystemMarkedGood (&isSystemGood);
        snprintf (string, sizeof(string), "fwupdateTest: sync_state -> result %d isSystemGood %d",
                  result, isSystemGood);
        Print (string);
        exit(0);
    }
    else if (0 == strcmp(testString, "do_markgood"))
    {
        bool isSystemGood;
        result = le_fwupdate_MarkGood ();
        snprintf (string, sizeof(string),  "fwupdateTest: MarkGood -> result %d", result);
        Print (string);
        result = le_fwupdate_IsSystemMarkedGood (&isSystemGood);
        snprintf (string, sizeof(string), "fwupdateTest: system_state -> result %d isSystemGood %d",
                  result, isSystemGood);
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
            snprintf (string, sizeof(string), "le_fwupdate_Download %d", result);
            Print (string);
            // Closing fd is unnecessary since the messaging infrastructure underneath
            // le_fwupdate_Download API that use it would close it.
        }
        exit(0);
    }
    else if (0 == strcmp(testString, "do_install_markgood"))
    {
        result = le_fwupdate_InstallAndMarkGood();
        snprintf (string, sizeof(string), "le_fwupdate_InstallAndMarkGood %d", result);
        Print (string);
        exit(0);
    }
    else if (0 == strcmp(testString, "do_install"))
    {
        result = le_fwupdate_Install();
        snprintf (string, sizeof(string), "le_fwupdate_Install %d", result);
        Print (string);
        exit(0);
    }
    else if (0 == strcmp(testString, "do_initdwnld"))
    {
        result = le_fwupdate_InitDownload();
        snprintf (string, sizeof(string), "le_fwupdate_InitDownload %d", result);
        Print (string);
        exit(0);
    }
    else if (0 == strcmp(testString, "do_disablesync"))
    {
        result = le_dualsys_DisableSyncBeforeUpdate(true);
        snprintf (string, sizeof(string), "le_fwupdate_DisableSyncBeforeUpdate(true) %d", result);
        Print (string);
        exit(0);
    }
    else if (0 == strcmp(testString, "do_enablesync"))
    {
        result = le_dualsys_DisableSyncBeforeUpdate(false);
        snprintf (string, sizeof(string), "le_fwupdate_DisableSyncBeforeUpdate(false) %d", result);
        Print (string);
        exit(0);
    }
    else if (0 == strcmp(testString, "do_getsystem"))
    {
        le_dualsys_System_t currentSystem;
        result = le_dualsys_GetCurrentSystem(&currentSystem);
        snprintf (string, sizeof(string), "le_dualsys_GetSystem %d", result);
        Print (string);
        if (LE_OK == result)
        {
            snprintf (string, sizeof(string), "modem %d lk %d linux %d",
                      (currentSystem & LE_DUALSYS_MODEM_GROUP) ? SYSTEM_2
                                                               : SYSTEM_1,
                      (currentSystem & LE_DUALSYS_LK_GROUP) ? SYSTEM_2
                                                            : SYSTEM_1,
                      (currentSystem & LE_DUALSYS_LINUX_GROUP) ? SYSTEM_2
                                                               : SYSTEM_1);
            Print (string);
        }
        exit(0);
    }
    else if (0 == strcmp(testString, "do_setsystem"))
    {
        int modemGroup, lkGroup, linuxGroup;
        le_dualsys_System_t newSystem;
        if (3 != sscanf(secondString, "%d,%d,%d", &modemGroup, &lkGroup, &linuxGroup))
        {
            Print ("Invalid ssid given");
            exit(EXIT_FAILURE);
        }
        if (!IsSubSystemValid( modemGroup ))
        {
            snprintf (string, sizeof(string), "Sub-system modem is not valid: %d", modemGroup);
            Print (string);
            exit(EXIT_FAILURE);
        }
        if (!IsSubSystemValid( lkGroup ))
        {
            snprintf (string, sizeof(string), "Sub-system lk is not valid: %d", lkGroup);
            Print (string);
            exit(EXIT_FAILURE);
        }
        if (!IsSubSystemValid( linuxGroup ))
        {
            snprintf (string, sizeof(string), "Sub-system linux is not valid: %d", linuxGroup);
            Print (string);
            exit(EXIT_FAILURE);
        }
        newSystem = (SYSTEM_2 == modemGroup) * LE_DUALSYS_MODEM_GROUP;
        newSystem |= (SYSTEM_2 == lkGroup) * LE_DUALSYS_LK_GROUP;
        newSystem |= (SYSTEM_2 == linuxGroup) * LE_DUALSYS_LINUX_GROUP;
        snprintf (string, sizeof(string), "newSystem 0x%x", newSystem);
        Print (string);
        result = le_dualsys_SetSystem(newSystem);
        snprintf (string, sizeof(string), "le_dualsys_SetSystem %d", result);
        Print (string);
        if (LE_OK == result)
        {
            snprintf (string, sizeof(string), "modem %d lk %d linux %d",
                      modemGroup, lkGroup, linuxGroup);
            Print (string);
        }
        exit(0);
    }
    else
    {
        LE_DEBUG ("fwupdateTest: not supported arg");
        PrintUsage();
        exit(EXIT_FAILURE);
    }
}
