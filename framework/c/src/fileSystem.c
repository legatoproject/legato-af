 /** @file fileSystem.c
 *
 * Implementation of the generic file system API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "fileSystem.h"
#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * Checks if a file system is mounted at the specified location.
 */
//--------------------------------------------------------------------------------------------------
bool fs_IsMounted
(
    const char* fileSysName,        ///< [IN] Name of the mounted filesystem.
    const char* path                ///< [IN] Path of the mounted filesystem.
)
{
    // Open /proc/mounts file to check where all the mounts are.  This sets the entry to the top
    // of the file.
    FILE* mntFilePtr = setmntent("/proc/mounts", "r");
    LE_FATAL_IF(mntFilePtr == NULL, "Could not read '/proc/mounts'.");

    char buf[LIMIT_MAX_MNT_ENTRY_BYTES];
    struct mntent mntEntry;
    bool result = false;

    while (getmntent_r(mntFilePtr, &mntEntry, buf, LIMIT_MAX_MNT_ENTRY_BYTES) != NULL)
    {
        if ( (strcmp(fileSysName, mntEntry.mnt_fsname) == 0) &&
             (strcmp(path, mntEntry.mnt_dir) == 0) )
        {
            result = true;
            break;
        }
    }

    // Close the file descriptor.
    endmntent(mntFilePtr);

    return result;
}
