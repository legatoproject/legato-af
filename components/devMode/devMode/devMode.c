/**
 * @file devMode.c
 *
 * This component puts the device into the "Developer Mode", which aids the development of new apps
 * or modifications to existing apps or systems.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Timer reference for the "MarkGood" timer.
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t MarkGoodTimer;


//--------------------------------------------------------------------------------------------------
/**
 * Timer expiry handler for the "MarkGood" timer. Mark the system as "Good".
 */
//--------------------------------------------------------------------------------------------------
static void MarkGood
(
    le_timer_Ref_t timer    ///< [IN] Timer ref. Not used.
)
{
    // Attempt to mark the system "good", so return status isn't checked. If there's a probation
    // lock, it might be from the apps are being developed, so we shouldn't override that by forcing
    // "Mark Good".
    le_updateCtrl_MarkGood(false);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function called when the system is changed (app installed or removed). Upon system
 * change, the "MarkGood" timer starts ticking.
 */
//--------------------------------------------------------------------------------------------------
static void SysChangeHandler
(
    const char* appName,  ///< [IN] App being installed or removed. Not used.
    void* contextPtr      ///< [IN] Context for this function. Not used.
)
{
    le_timer_Start(MarkGoodTimer);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialization function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Mark next reboot as expected to avoid false-positive detection of boot loops that can occur
    // if the developer is repeatedly testing system behaviour after reboots.
    le_framework_NotifyExpectedReboot();

    /* Obtain a wake lock */
    // Note that the wake lock is released if the app is stopped.
    le_pm_WakeupSourceRef_t wakeUpSource = le_pm_NewWakeupSource(0, "devModeApp");
    le_pm_StayAwake(wakeUpSource);

    /* Setup a timer to attempt to mark the system as "Good" within 10 seconds of system changes. */
    MarkGoodTimer = le_timer_Create("MarkGood");
    le_timer_SetHandler(MarkGoodTimer, MarkGood);
    le_timer_SetMsInterval(MarkGoodTimer, 10000);

    // Start the "MarkGood" timer upon the system changes of installing or uninstalling an app.
    le_instStat_AddAppUninstallEventHandler(SysChangeHandler, NULL);
    le_instStat_AddAppInstallEventHandler(SysChangeHandler, NULL);

    // In case a new system has been installed, handle this system change. Otherwise calling this
    // handler does no harm.
    SysChangeHandler(NULL, NULL);
}
