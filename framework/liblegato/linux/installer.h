//--------------------------------------------------------------------------------------------------
/**
 * @file installer.h
 *
 * Utility functions needed by the installers (Update Daemon and the start program's "golden"
 * system installer).
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_INSTALLER_H_INCLUDE_GUARD
#define LEGATO_INSTALLER_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Get the app hash ID contained in a symlink to an app.
 *
 * On return the application's hash is copied into the supplied hashBuffer.
 */
//--------------------------------------------------------------------------------------------------
void installer_GetAppHashFromSymlink
(
    const char* linkPath,                 ///< [IN] The path to the symlink.
    char hashBuffer[LIMIT_MD5_STR_BYTES]  ///< [OUT] Buffer to hold the application hash ID.
);


//--------------------------------------------------------------------------------------------------
/**
 * Install a given app's writeable files in the "unpack" system from either the app's install
 * directory (/legato/apps/<hash>) or a specified other system, as appropriate for each file.
 *
 * @return LE_OK if successful.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t installer_InstallAppWriteableFiles
(
    const char* appMd5Ptr,
    const char* appNamePtr,
    const char* oldSystemName   ///< Name of system to inherit files from (e.g., "current" or "0").
);


//--------------------------------------------------------------------------------------------------
/**
 * Update a given app's writeable files in a given system to match what's in the app's install
 * directory (/legato/apps/<hash>).  Deletes from the system any files that are not in the app's
 * install directory.  Adds to the system any files from the apps' install directory that are
 * already in the system.  Leaves alone files that are in both the system and the app's
 * install directory.
 *
 * @return LE_OK if successful.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t installer_UpdateAppWriteableFiles
(
    const char* systemNamePtr,  ///< System dir to write files into ("current" or "unpack").
    const char* appMd5Ptr,
    const char* appNamePtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Remove devices from a given app's writeable files. This is to ensure that device node numbers
 * always match their respective devices outside the sandbox.
 *
 * @return LE_OK if successful.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t installer_RemoveAppWriteableDeviceFiles
(
    const char* systemNamePtr,  ///< System dir to write files into ("current" or "unpack").
    const char* appMd5Ptr,
    const char* appNamePtr
);


#endif // LEGATO_INSTALLER_H_INCLUDE_GUARD
