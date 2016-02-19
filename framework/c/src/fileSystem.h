/** @file fileSystem.h
 *
 * Generic File System API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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


#endif // LE_FILE_SYSTEM_H_INCLUDE_GUARD
