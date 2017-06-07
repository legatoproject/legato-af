/*
 * External watchdog bridge for standard Linux watchdog.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * File descriptor of the external watchdog.
 */
//--------------------------------------------------------------------------------------------------
static int wdogFd = -1;

//--------------------------------------------------------------------------------------------------
/**
 * Timeout on hardware watchdog (in ms).
 */
//--------------------------------------------------------------------------------------------------
#define WDOG_TIMEOUT          60000

//--------------------------------------------------------------------------------------------------
/**
 * Kick the external watchdog
 */
//--------------------------------------------------------------------------------------------------
static void ExternalWdogKick
(
    void* contextPtr
)
{
    // Write a single byte to kick the watchdog.
    if (write(wdogFd, "k", 1) != 1)
    {
        LE_FATAL("Failed to kick watchdog.");
    }
}

COMPONENT_INIT
{
    // Remove this line if using a hardware watchdog.
    system("/sbin/modprobe softdog");

    wdogFd = open("/dev/watchdog", O_WRONLY);
    if (wdogFd < 0)
    {
        LE_FATAL("Could not open watchdog device");
    }

    // And kick watchdog immediately.  This could be a restart, in which case the watchdog
    // will be running already.
    ExternalWdogKick(NULL);

    // Kick watchdog at twice expiry rate so there's no risk of failing to kick due to timing
    // issues.
    le_wdog_AddExternalWatchdogHandler(WDOG_TIMEOUT/2, ExternalWdogKick, NULL);

    // Do not close or cleanup external watchdog on exit.  This is on purpose -- if this program
    // is killed or exits unexpectedly and is not restarted, the board should reboot.
}
