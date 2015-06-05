//--------------------------------------------------------------------------------------------------
/** @file execInApp/sandbox.c
 *
 * Temporary solution until command apps are available.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "sandbox.h"
#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * Location for all sandboxed apps.
 */
//--------------------------------------------------------------------------------------------------
#define SANDBOXES_DIR                   "/tmp/legato/sandboxes/"


//--------------------------------------------------------------------------------------------------
/**
 * Gets the sandbox location path string.  The sandbox does not have to exist before this function
 * is called.  This function gives the expected location of the sandbox by simply appending the
 * appName to the sandbox root path.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW the provided buffer is too small.
 */
//--------------------------------------------------------------------------------------------------
le_result_t sandbox_GetPath
(
    const char* appName,    ///< [IN] The application name.
    char* pathBuf,          ///< [OUT] The buffer that will contain the path string.
    size_t pathBufSize      ///< [IN] The buffer size.
)
{
    LE_ASSERT( (pathBuf != NULL) && (pathBufSize > 0) );

    pathBuf[0] = '\0';

    return le_path_Concat("/", pathBuf, pathBufSize, SANDBOXES_DIR, appName, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Confines the calling process into the sandbox.
 *
 * @note Kills the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
void sandbox_ConfineProc
(
    const char* sandboxRootPtr, ///< [IN] Path to the sandbox root.
    uid_t uid,                  ///< [IN] The user ID the process should be set to.
    gid_t gid,                  ///< [IN] The group ID the process should be set to.
    const gid_t* groupsPtr,     ///< [IN] List of supplementary groups for this process.
    size_t numGroups,           ///< [IN] The number of groups in the supplementary groups list.
    const char* workingDirPtr   ///< [IN] The directory the process's working directory will be set
                                ///       to relative to the sandbox root.
)
{
    // @Note: The order of the following statements is important and should not be changed carelessly.

    // Change working directory.
    char homeDir[LIMIT_MAX_PATH_BYTES];
    size_t s;

    if (workingDirPtr[0] == '/')
    {
        s = snprintf(homeDir, sizeof(homeDir), "%s%s", sandboxRootPtr, workingDirPtr);
    }
    else
    {
        s = snprintf(homeDir, sizeof(homeDir), "%s/%s", sandboxRootPtr, workingDirPtr);
    }

    if (s >= sizeof(homeDir))
    {
        LE_FATAL("Working directory is too long: '%s'", homeDir);
    }

    LE_FATAL_IF(chdir(homeDir) != 0, "Could not change working directory to '%s'.  %m", homeDir);

    // Chroot to the sandbox.
    LE_FATAL_IF(chroot(sandboxRootPtr) != 0, "Could not chroot to '%s'.  %m", sandboxRootPtr);

    // Clear our supplementary groups list.
    LE_FATAL_IF(setgroups(0, NULL) == -1, "Could not set the supplementary groups list.  %m.");

    // Populate our supplementary groups list with the provided list.
    LE_FATAL_IF(setgroups(numGroups, groupsPtr) == -1,
                "Could not set the supplementary groups list.  %m.");

    // Set our process's primary group ID.
    LE_FATAL_IF(setgid(gid) == -1, "Could not set the group ID.  %m.");

    // Set our process's user ID.  This sets all of our user IDs (real, effective, saved).  This
    // call also clears all cababilities.  This function in particular MUST be called after all
    // the previous system calls because once we make this call we will lose root priviledges.
    LE_FATAL_IF(setuid(uid) == -1, "Could not set the user ID.  %m.");
}
