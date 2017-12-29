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

    /* Start the tcf-agent daemon */
    // tcf-agent.conf contains target device info necessary for the tcf-agent to send to Dev Studio.
    // If the file exists, or is defined by the user, then it's not generated.
    struct stat buf;
    if (stat("/etc/tcf-agent.conf", &buf) != 0)
    {
        // On SWI platforms, use ud_getusbinfo to provide name & IMEI of the target.
        if (stat("/usr/bin/ud_getusbinfo", &buf) == 0)
        {
            if (0 != system("echo `/usr/bin/ud_getusbinfo NAME` `/usr/bin/ud_getusbinfo IMEI` > "
                   "/etc/tcf-agent.conf"))
            {
                LE_ERROR("Unable to populate '/etc/tcf-agent.conf'");
            }
        }
    }

    if (0 == system("/legato/systems/current/apps/devMode/read-only/sbin/tcf-agent -d -L- -l0"))
    {
        // Do nothing
    }
    else if (0 == system("/usr/sbin/tcf-agent -d -L- -l0"))
    {
        // Do nothing
    }
    else
    {
        LE_ERROR("Unable to launch tcf-agent");
    }

    if (stat("/usr/lib/openssh", &buf) != 0)
    {
        if (0 != system("mkdir -p /tmp/ulib /tmp/ulib_wk"))
        {
            LE_ERROR("Unable to create directories /tmp/ulib");
        }
        if (0 == system("mount -t overlay"
                        " -o upperdir=/tmp/ulib,lowerdir=/usr/lib,workdir=/tmp/ulib_wk"
                        " overlay /usr/lib"))
        {
            // Do nothing
        }
        else if (0 == system("mount -t aufs"
                             " -o dirs=/tmp/ulib=rw:/usr/lib=ro"
                             " aufs /usr/lib"))
        {
            // Do nothing
        }
        else
        {
            LE_ERROR("Unable to mount overlay over /usr/lib");
        }
        if (0 != system("mkdir -p /usr/lib/openssh &&"
                        " ln -s /legato/systems/current/apps/devMode/read-only/bin/sftp-server"
                        " /usr/lib/openssh/"))
        {
            LE_ERROR("Unable to link sftp-server");
        }
    }

    if (stat("/usr/libexec/sftp-server", &buf) != 0)
    {
        if (0 != system("mkdir -p /tmp/libexec /tmp/libexec_wk"))
        {
            LE_ERROR("Unable to create directories /tmp/libexec");
        }
        if (0 == system("mount -t overlay"
                        " -o upperdir=/tmp/libexec,lowerdir=/usr/libexec,workdir=/tmp/libexec_wk"
                        " overlay /usr/libexec"))
        {
            // Do nothing
        }
        else if (0 == system("mount -t aufs"
                             " -o dirs=/tmp/libexec=rw:/usr/libexec=ro"
                             " aufs /usr/libexec"))
        {
            // Do nothing
        }
        else
        {
            LE_ERROR("Unable to mount overlay over /usr/libexec");
        }
        if (0 != system("ln -s /legato/systems/current/apps/devMode/read-only/bin/sftp-server"
                        " /usr/libexec/"))
        {
            LE_ERROR("Unable to link sftp-server");
        }
    }

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
