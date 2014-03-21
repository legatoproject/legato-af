//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Configuration Installer component.
 *
 * Copyright 2013, Sierra Wireless Inc., all rights reserved.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef CONFIG_INSTALLER_H_INCLUDE_GUARD
#define CONFIG_INSTALLER_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Add an application's configuration to the root configuration tree.
 **/
//--------------------------------------------------------------------------------------------------
void cfgInstall_Add
(
    const char* appName
);


//--------------------------------------------------------------------------------------------------
/**
 * Removes an application's configuration from the root configuration tree.
 **/
//--------------------------------------------------------------------------------------------------
void cfgInstall_Remove
(
    const char* appName
);



#endif // CONFIG_INSTALLER_H_INCLUDE_GUARD
