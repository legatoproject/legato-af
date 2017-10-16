//--------------------------------------------------------------------------------------------------
/** @file rebootTest.c
 *
 * Unit test for start daemon
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
/* IPC APIs */
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Max power manager boot count
 */
//--------------------------------------------------------------------------------------------------
#define PM_BOOT_COUNT 8

//--------------------------------------------------------------------------------------------------
/**
 * Max boot count
 */
//--------------------------------------------------------------------------------------------------
#define MAX_BOOT_COUNT PM_BOOT_COUNT*2

//--------------------------------------------------------------------------------------------------
/**
 * Boot count config path
 */
//--------------------------------------------------------------------------------------------------
#define  BOOT_COUNT_CFG "/apps/rebootTest"

//--------------------------------------------------------------------------------------------------
/**
 * Boot count config variable
 */
//--------------------------------------------------------------------------------------------------
#define  BOOT_COUNT_CFG_VAR "bootCount"

//--------------------------------------------------------------------------------------------------
/**
 * Timer interval(in seconds) to exit from shutdown/ultralow-power state.
 *
 * @note Please change this interval as needed.
 */
//--------------------------------------------------------------------------------------------------
#define ULPM_EXIT_INTERVAL 10

//--------------------------------------------------------------------------------------------------
/**
 * Timer interval(in milliseconds) to trigger reboot by ULPM
 */
//--------------------------------------------------------------------------------------------------
#define PM_TIMEOUT_INTERVAL 20000

//--------------------------------------------------------------------------------------------------
/**
 * Timer interval(in milliseconds) to trigger reboot
 */
//--------------------------------------------------------------------------------------------------
#define SUP_TIMEOUT_INTERVAL 62000

//--------------------------------------------------------------------------------------------------
/**
 * Get number of low power and exit(i.e. reboot)
 **/
//--------------------------------------------------------------------------------------------------
static int GetRebootCount
(
    void
)
{
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(BOOT_COUNT_CFG);
    int bootCount = le_cfg_GetInt(iterRef, BOOT_COUNT_CFG_VAR, -1);
    le_cfg_CancelTxn(iterRef);

    return bootCount;
}

//--------------------------------------------------------------------------------------------------
/**
 * Increment number of low power and exit(i.e. reboot) by one if it less than maximum allowed boot
 * count. Otherwise delete the bootCount config var.
 **/
//--------------------------------------------------------------------------------------------------
static int UpdateRebootCount
(
    void
)
{
    int bootCount = GetRebootCount();

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(BOOT_COUNT_CFG);
    bootCount++;
    le_cfg_SetInt(iterRef,  BOOT_COUNT_CFG_VAR, bootCount);
    le_cfg_CommitTxn(iterRef);

    return bootCount;
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the boot source and shutdown MDM.
 */
//--------------------------------------------------------------------------------------------------
static void PmConfigShutDown
(
    le_timer_Ref_t timerRef    ///< Timer that expired
)
{
    LE_WARN("Entering ulpm mode");
    // Boot after specified interval.
    if (le_ulpm_BootOnTimer(ULPM_EXIT_INTERVAL) != LE_OK)
    {
        LE_FATAL("Can't set timer as boot source");
    }

    UpdateRebootCount();

    // Initiate shutdown.
    if (le_ulpm_ShutDown() != LE_OK)
    {
        LE_FATAL("Can't initiate shutdown.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Do a regular shutdown.
 */
//--------------------------------------------------------------------------------------------------
static void RegularShutDown
(
    le_timer_Ref_t timerRef    ///< Timer that expired
)
{
    LE_WARN("Doing a regular reboot");

    UpdateRebootCount();

    // Reboot the system.
    if (reboot(RB_AUTOBOOT) == -1)
    {
        LE_FATAL("Failed to reboot. Errno = %s.", strerror(errno));
    }
}

COMPONENT_INIT
{
    LE_INFO("Reboot test started");

    int bootCount = GetRebootCount();

    if (bootCount > MAX_BOOT_COUNT)
    {
        LE_INFO("Successfully reboot %d times", bootCount);
        LE_INFO("Test passed");
        le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(BOOT_COUNT_CFG);
        le_cfg_DeleteNode(iterRef, BOOT_COUNT_CFG_VAR);
        le_cfg_CommitTxn(iterRef);
        exit(EXIT_SUCCESS);
    }

    le_timer_Ref_t bootTimer = le_timer_Create("BootTimer");

    if (bootCount < PM_BOOT_COUNT)
    {
        LE_INFO("Shutdown will be triggered by PM after ~%d seconds", PM_TIMEOUT_INTERVAL/1000);
        le_timer_SetHandler(bootTimer, PmConfigShutDown);
        le_timer_SetMsInterval(bootTimer, PM_TIMEOUT_INTERVAL);
    }
    else
    {
        LE_INFO("Regular shutdown will be triggered ~%d seconds", SUP_TIMEOUT_INTERVAL/1000);
        le_timer_SetHandler(bootTimer, RegularShutDown);
        le_timer_SetMsInterval(bootTimer, SUP_TIMEOUT_INTERVAL);
    }

    le_timer_Start(bootTimer);
    LE_WARN("For testing purpose, app will mark the current system as good");
    le_updateCtrl_MarkGood(true);
}
