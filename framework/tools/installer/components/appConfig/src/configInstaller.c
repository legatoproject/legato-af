//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Configuration Installer component.
 *
 * Copyright 2013, Sierra Wireless Inc., all rights reserved.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "../inc/configInstaller.h"
#include "le_cfg_interface.h"
#include "le_cfgAdmin_interface.h"


COMPONENT_INIT
{
    le_cfg_Initialize();
    le_cfgAdmin_Initialize();
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

    le_cfg_IteratorRef_t i = le_cfg_CreateWriteTxn("/apps");

    le_cfg_DeleteNode(i, appName);
    le_cfg_CommitTxn(i);
}
