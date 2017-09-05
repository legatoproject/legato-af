 /** @file fileSystem.c
 *
 * Implementation of the generic file system API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "limit.h"
#include "fileSystem.h"


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


//--------------------------------------------------------------------------------------------------
/**
 * Checks if a path location is a mount point (has file system mounted at that location).
 */
//--------------------------------------------------------------------------------------------------
bool fs_IsMountPoint
(
    const char* path                ///< [IN] Path to check.
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
        if (strcmp(path, mntEntry.mnt_dir) == 0)
        {
            result = true;
            break;
        }
    }

    // Close the file descriptor.
    endmntent(mntFilePtr);

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Lazy umount any file systems that may be mounted at the specified location.
 */
//--------------------------------------------------------------------------------------------------
void fs_TryLazyUmount
(
    const char* pathPtr             ///< [IN] Mount point.
)
{
    if ( (umount2(pathPtr, MNT_DETACH) == -1) && (errno != EINVAL) )
    {
        LE_CRIT("Could not lazy unmount '%s'.  %m.", pathPtr);
    }
}
