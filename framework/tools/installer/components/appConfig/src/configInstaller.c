//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Configuration Installer functionality of the appConfig component.
 *
 * Copyright 2013-2014, Sierra Wireless Inc., Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "../inc/configInstaller.h"
#include "interfaces.h"


COMPONENT_INIT
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Add an application's configuration to the root configuration tree.
 **/
//--------------------------------------------------------------------------------------------------
void cfgInstall_Add
(
    const char* appName
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;

    char filePath[256] = "/opt/legato/apps/";

    result = le_utf8_Append(filePath, appName, sizeof(filePath), NULL);
    LE_FATAL_IF(result != LE_OK, "App name '%s' is too long.", appName);

    result = le_utf8_Append(filePath, "/root.cfg", sizeof(filePath), NULL);
    LE_FATAL_IF(result != LE_OK, "App name '%s' is too long.", appName);

    LE_INFO("Importing configuration for application '%s' from '%s'.", appName, filePath);

    le_cfg_IteratorRef_t i = le_cfg_CreateWriteTxn("/apps");

    result = le_cfgAdmin_ImportTree(i, filePath, appName);

    LE_FATAL_IF(result != LE_OK,
                "Failed to import configuration from '%s' to 'root:/apps/%s' (%s)",
                filePath,
                appName,
                LE_RESULT_TXT(result));

    le_cfg_CommitTxn(i);
}


//--------------------------------------------------------------------------------------------------
/**
 * Removes an application's configuration from the root configuration tree.
 **/
//--------------------------------------------------------------------------------------------------
void cfgInstall_Remove
(
    const char* appName
)
//--------------------------------------------------------------------------------------------------
{
    LE_INFO("Removing configuration for application '%s'.", appName);

    // Remove the app configuration from the system tree.
    le_cfg_IteratorRef_t i = le_cfg_CreateWriteTxn("/apps");

    le_cfg_DeleteNode(i, appName);
    le_cfg_CommitTxn(i);

    // Now delete the app specific tree.
    le_cfgAdmin_DeleteTree(appName);
}
