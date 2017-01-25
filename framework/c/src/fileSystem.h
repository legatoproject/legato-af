/** @file fileSystem.h
 *
 * Generic File System API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_FILE_SYSTEM_H_INCLUDE_GUARD
#define LE_FILE_SYSTEM_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Checks if a file system is mounted at the specified location.
 */
//--------------------------------------------------------------------------------------------------
bool fs_IsMounted
(
    const char* fileSysName,        ///< [IN] Name of the mounted filesystem.
    const char* path                ///< [IN] Path of the mounted filesystem.
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks if a path location is a mount point (has file system mounted at that location).
 */
//--------------------------------------------------------------------------------------------------
bool fs_IsMountPoint
(
    const char* path                ///< [IN] Path to check.
);


//--------------------------------------------------------------------------------------------------
/**
 * Lazy umount any file systems that may be mounted at the specified location.
 */
//--------------------------------------------------------------------------------------------------
void fs_TryLazyUmount
(
    const char* pathPtr             ///< [IN] Mount point.
);


#endif // LE_FILE_SYSTEM_H_INCLUDE_GUARD
