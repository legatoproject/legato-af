//--------------------------------------------------------------------------------------------------
/**
 * @file mcuWdog.c: mcu watchdog control sample app via sys node
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------
#include <legato.h>
#include <interfaces.h>
#include <stdio.h>

static void WatchdogSetStatus(bool enable)
{
    /* 1 to start watchdog; 0 to stop watchdog */
    if (enable)
    {
        FILE* fd = popen("echo 1 > /sys/module/swimcu_pm/watchdog/enable", "r");
        pclose(fd);

    }
    else
    {
        FILE* fd = popen("echo 0 > /sys/module/swimcu_pm/watchdog/enable", "r");
        pclose(fd);
    }
}

static void WatchdogSetTimeout(int timeout)
{
    /* timeout value for the watchdog */
    char cmd[100];
    sprintf(cmd, "echo %d > /sys/module/swimcu_pm/watchdog/timeout", timeout);
    FILE* fd = popen(cmd, "r");
    pclose(fd);
}

static void WatchdogSetDebugmask()
{
    FILE* fd = popen("echo 255 > /sys/module/swimcu_pm/watchdog/parameters/debug_mask", "r");
    pclose(fd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize watchdog config app
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    WatchdogSetDebugmask();
    WatchdogSetTimeout(10);
    WatchdogSetStatus(true);
}
