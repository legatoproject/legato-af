//--------------------------------------------------------------------------------------------------
/**
 *
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"


static void AppInstallHandler
(
    const char* appName,
    void* contextPtr
)
{
    LE_INFO("========== App %s installed.", appName);
}


static void AppUninstallHandler
(
    const char* appName,
    void* contextPtr
)
{
    LE_INFO("========== App %s uninstalled.", appName);
}


COMPONENT_INIT
{
    // Test file descriptor passing.
    LE_INFO("********** Start Install Notify Test ***********");

    // Register for notifications of install/uninstall of apps.
    le_instStat_AddAppInstallEventHandler(AppInstallHandler, NULL);
    le_instStat_AddAppUninstallEventHandler(AppUninstallHandler, NULL);
}
