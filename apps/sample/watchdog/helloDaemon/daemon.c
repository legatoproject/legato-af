/*
 * Sample showing how to monitor a critical process with a watchdog.
 *
 * In this case logging "Hello World" every 10s or so
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

COMPONENT_INIT
{
    while (1)
    {
        le_wdog_Timeout(30000);
        LE_INFO("Hello World!");
        usleep(10000000);
    }
}
