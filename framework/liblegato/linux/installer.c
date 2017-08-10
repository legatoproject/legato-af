//--------------------------------------------------------------------------------------------------
/**
 * @file installer.c
 *
 * Utility functions needed by the installers (Update Daemon and the start program's "golden"
 * system installer).
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "limit.h"
#include "installer.h"
#include "file.h"
#include "smack.h"
#include "dir.h"
#include "user.h"



//--------------------------------------------------------------------------------------------------
/**
 * Get app directory smack label based on permission bits.
 */
//--------------------------------------------------------------------------------------------------
static void GetDirSmackLabel
(
    const char* appLabel,   ///< [IN] The smack label of the application.
    mode_t dirMode,         ///< [IN] Permission bits for directory
    char* dirLabel,         ///< [OUT] Ptr to the buffer the directory label will be written into.
    size_t labelSize        ///< [IN] Size (in bytes) of the directory label buffer.
)
{
    char appendMode[10] = "";
    int index = 0;

    if (dirMode & S_IROTH)
    {
        appendMode[index++] = 'r';
    }

    if (dirMode & S_IWOTH)
    {
        appendMode[index++] = 'w';
    }

    if (dirMode & S_IXOTH)
    {
        appendMode[index++] = 'x';
    }

    LE_ASSERT(snprintf(dirLabel, labelSize, "%s%s", appLabel, appendMode) < labelSize);
}


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
)
//--------------------------------------------------------------------------------------------------
{
    char linkContent[100];

    ssize_t bytesRead = readlink(linkPath, linkContent, sizeof(linkContent));
    if (bytesRead < 0)
    {
        LE_FATAL("Failed to read symlink '%s' (%m).", linkPath);
    }
    else if (bytesRead >= sizeof(linkContent))
    {
        LE_FATAL("Contents of symlink '%s' too long (> %zu).", linkPath, sizeof(linkContent) - 1);
    }

    // Null-terminate.
    linkContent[bytesRead] = '\0';

    LE_ASSERT(LE_OK == le_utf8_Copy(hashBuffer,
                                    le_path_GetBasenamePtr(linkContent, "/"),
                                    LIMIT_MD5_STR_BYTES,
                                    NULL));
}


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
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result = LE_OK;

    char freshWriteablesDir[PATH_MAX];

    int baseDirPathLen = snprintf(freshWriteablesDir,
                     sizeof(freshWriteablesDir),
                     "/legato/apps/%s/writeable",
                     appMd5Ptr);
    LE_ASSERT(baseDirPathLen < sizeof(freshWriteablesDir));

    char* pathArrayPtr[] = { freshWriteablesDir, NULL };
    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL, NULL);

    if (ftsPtr == NULL)
    {
        LE_CRIT("Failed to open '%s' for traversal (%m).", freshWriteablesDir);
        return LE_FAULT;
    }

    char appLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
    smack_GetAppLabel(appNamePtr, appLabel, sizeof(appLabel));

    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        // Compute the destination path in the unpack area.
        char destPath[PATH_MAX];
        int n = snprintf(destPath,
                         sizeof(destPath),
                         "/legato/systems/unpack/appsWriteable/%s%s",
                         appNamePtr,
                         entPtr->fts_path + baseDirPathLen);
        if (n >= sizeof(destPath))
        {
            LE_CRIT("Path to writeable file in app '%s' <%s> is too long.",
                    appNamePtr,
                    appMd5Ptr);

            result = LE_FAULT;
            goto cleanup;
        }

        // Act differently depending on whether we are looking at a file or a directory.
        switch (entPtr->fts_info)
        {
            case FTS_D: // Directory visited in pre-order.

                // If this is not the top level directory, create it.
                if (entPtr->fts_level > 0)
                {
                    char dirLabel[LIMIT_MAX_SMACK_LABEL_BYTES] = "";
                    mode_t dirMode = (entPtr->fts_statp->st_mode) & (S_IRWXU | S_IRWXG | S_IRWXO);

                    // Get the directory label based on mode.
                    GetDirSmackLabel(appLabel, dirMode, dirLabel, sizeof(dirLabel));

                    LE_DEBUG("Creating directory: '%s' with smack label: '%s'", destPath, dirLabel);

                    if (dir_MakePathSmack(destPath, dirMode, dirLabel ) != LE_OK)
                    {
                        LE_CRIT("Failed to create directory '%s'.", destPath);
                        result = LE_FAULT;
                        goto cleanup;
                    }
                }
                else  // This is a top level directory
                {
                    char dirLabel[LIMIT_MAX_SMACK_LABEL_BYTES] = "";

                    LE_ASSERT(snprintf(dirLabel, sizeof(dirLabel), "%srwx", appLabel)
                              < sizeof(dirLabel));

                    LE_DEBUG("Creating directory: '%s' with smack label: '%s'", destPath, dirLabel);

                    if (le_dir_MakePath(destPath, S_IRWXU | S_IRWXG | S_IRWXO) == LE_OK)
                    {
                        smack_SetLabel(destPath, dirLabel);
                    }
                    else
                    {
                        LE_CRIT("Could not create directory '%s'.  %m.", destPath);
                        result = LE_FAULT;
                        goto cleanup;
                    }
                }
                break;

            case FTS_DP: // Directory visited in post-order, ignore it.
                break;

            case FTS_SL:

                LE_CRIT("Ignoring symlink in writeable files for app '%s' <%s>.",
                        appNamePtr,
                        appMd5Ptr);
                result = LE_FAULT;

                goto cleanup;

                break;

            case FTS_F:
            {
                // Compute the path the file would appear at in the old system.
                char oldVersionPath[PATH_MAX];
                int n = snprintf(oldVersionPath,
                                 sizeof(oldVersionPath),
                                 "/legato/systems/%s/appsWriteable/%s%s",
                                 oldSystemName,
                                 appNamePtr,
                                 entPtr->fts_path + baseDirPathLen);
                if (n >= sizeof(oldVersionPath))
                {
                    LE_CRIT("Path to writeable file in app '%s' <%s> is too long.",
                            appNamePtr,
                            appMd5Ptr);
                    result = LE_FAULT;
                    goto cleanup;
                }

                // If the file exists in the old system, copy that.
                if (file_Exists(oldVersionPath))
                {
                    if (file_Copy(oldVersionPath, destPath, appLabel) != LE_OK)
                    {
                        result = LE_FAULT;
                        goto cleanup;
                    }
                }
                // If the file does not exist in the old system, install the fresh one.
                else
                {
                    if (file_Copy(entPtr->fts_path, destPath, appLabel) != LE_OK)
                    {
                        result = LE_FAULT;
                        goto cleanup;
                    }
                }

                break;
            }

            case FTS_NS:    // stat() failed

                // We expect stat() to fail at level 0 if there are no writeables in the app.
                if (entPtr->fts_level != 0)
                {
                    LE_CRIT("Stat failed for '%s' (app '%s' <%s>).",
                            entPtr->fts_path,
                            appNamePtr,
                            appMd5Ptr);
                }
                break;

            default:

                LE_CRIT("Ignoring unexpected file type %d at '%s' (app '%s' <%s>).",
                        entPtr->fts_info,
                        entPtr->fts_path,
                        appNamePtr,
                        appMd5Ptr);
                break;
        }
    }

cleanup:

    fts_close(ftsPtr);

    return result;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result = LE_OK;
    char freshWriteablesDir[PATH_MAX];

    int baseDirPathLen = snprintf(freshWriteablesDir,
                     sizeof(freshWriteablesDir),
                     "/legato/apps/%s/writeable",
                     appMd5Ptr);
    LE_ASSERT(baseDirPathLen < sizeof(freshWriteablesDir));

    char* pathArrayPtr[] = { freshWriteablesDir, NULL };
    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_LOGICAL, NULL);

    if (ftsPtr == NULL)
    {
        LE_CRIT("Failed to open '%s' for traversal (%m).", freshWriteablesDir);
        return LE_FAULT;
    }

    char appLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
    smack_GetAppLabel(appNamePtr, appLabel, sizeof(appLabel));

    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        // Compute the destination path in the system.
        char destPath[PATH_MAX];
        int n = snprintf(destPath,
                         sizeof(destPath),
                         "/legato/systems/%s/appsWriteable/%s%s",
                         systemNamePtr,
                         appNamePtr,
                         entPtr->fts_path + baseDirPathLen);
        if (n >= sizeof(destPath))
        {
            LE_CRIT("Path to writeable file in app '%s' <%s> in '%s' system is too long.",
                    appNamePtr,
                    appMd5Ptr,
                    systemNamePtr);
            result = LE_FAULT;
            continue;
        }

        // Act differently depending on whether we are looking at a file or a directory.
        switch (entPtr->fts_info)
        {
            case FTS_D: // Directory visited in pre-order.
                // If this is not the top level directory, create it.
                if (entPtr->fts_level > 0)
                {
                    char dirLabel[LIMIT_MAX_SMACK_LABEL_BYTES] = "";
                    mode_t dirMode = (entPtr->fts_statp->st_mode) & (S_IRWXU | S_IRWXG | S_IRWXO);

                    // Get the directory label based on mode.
                    GetDirSmackLabel(appLabel, dirMode, dirLabel, sizeof(dirLabel));

                    LE_DEBUG("Creating directory: '%s' with smack label: '%s'", destPath, dirLabel);

                    if (dir_MakePathSmack(destPath, dirMode, dirLabel ) != LE_OK)
                    {
                        LE_CRIT("Failed to create directory '%s'.", destPath);
                    }
                }
                else  // This is a top level directory
                {
                    char dirLabel[LIMIT_MAX_SMACK_LABEL_BYTES] = "";

                    LE_ASSERT(snprintf(dirLabel, sizeof(dirLabel), "%srwx", appLabel)
                              < sizeof(dirLabel));

                    LE_DEBUG("Creating directory: '%s' with smack label: '%s'", destPath, dirLabel);

                    if (le_dir_MakePath(destPath, S_IRWXU | S_IRWXG | S_IRWXO) == LE_OK)
                    {
                        smack_SetLabel(destPath, dirLabel);
                    }
                    else
                    {
                        LE_CRIT("Could not create directory '%s'.  %m.", destPath);
                    }
                }
                break;

            case FTS_SL:

                LE_CRIT("Symlink in writeable files for app '%s' <%s> (%s).",
                        appNamePtr,
                        appMd5Ptr,
                        entPtr->fts_path);
                result = LE_FAULT;
                break;

            case FTS_F:
            {
                // If the file does not exist in the system, add it.
                if (!file_Exists(destPath))
                {
                    if (file_Copy(entPtr->fts_path, destPath, appLabel) != LE_OK)
                    {
                        result = LE_FAULT;
                    }
                }

                break;
            }

            case FTS_DP: // Directory visited in post-order (ignore).
            case FTS_NS: // Path doesn't exist (ignore).

                break;

            default:
            {
                LE_CRIT("Unexpected file type %d in app '%s' <%s>.",
                        entPtr->fts_info,
                        appNamePtr,
                        appMd5Ptr);
                LE_CRIT("Offending path: '%s'.", entPtr->fts_path);
                result = LE_FAULT;
                break;
            }
        }
    }

    fts_close(ftsPtr);

    // Delete files from system that are not in the app's install dir's writeable files.
    char appWriteableDirPath[PATH_MAX];
    baseDirPathLen = snprintf(appWriteableDirPath,
                              sizeof(appWriteableDirPath),
                              "/legato/systems/%s/appsWriteable/%s",
                              systemNamePtr,
                              appNamePtr);
    LE_ASSERT(baseDirPathLen < sizeof(appWriteableDirPath));
    pathArrayPtr[0] = appWriteableDirPath;
    ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL, NULL);

    if (ftsPtr == NULL)
    {
        LE_CRIT("Failed to open '%s' for traversal (%m).", appWriteableDirPath);
        return LE_FAULT;
    }

    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        // Compute the equivalent path in the app install directory.
        char appInstallPath[PATH_MAX];
        int n = snprintf(appInstallPath,
                         sizeof(appInstallPath),
                         "/legato/apps/%s/writeable/%s",
                         appMd5Ptr,
                         entPtr->fts_path + baseDirPathLen);
        if (n >= sizeof(appInstallPath))
        {
            LE_FATAL("Path to writeable file in app '%s' <%s> in app install dir is too long.",
                     appNamePtr,
                     appMd5Ptr);
            result = LE_FAULT;
            continue;

        }

        // Act differently depending on whether we are looking at a file or a directory.
        switch (entPtr->fts_info)
        {
            case FTS_DP: // Directory visited in post-order.

                // If this does not exist in the new app, delete it.
                if ((entPtr->fts_level > 0) && (!le_dir_IsDir(appInstallPath)))
                {
                    if (le_dir_RemoveRecursive(entPtr->fts_path) != LE_OK)
                    {
                        LE_CRIT("Failed to delete directory '%s'.", entPtr->fts_path);
                        result = LE_FAULT;
                    }
                }
                break;

            case FTS_D: // Directory visited in pre-order.
            case FTS_NS: // appsWriteable dir doesn't even exist for this app.

                // Ignore.
                break;

            case FTS_SL:
                if (unlink(entPtr->fts_path) != 0)
                {
                    LE_CRIT("Failed to delete symlink '%s'. (%m)", entPtr->fts_path);
                    result = LE_FAULT;
                }

                break;

            case FTS_F:
            {
                // If the file does not exist in the new app, delete the file from the system.
                if (!file_Exists(appInstallPath))
                {
                    if (unlink(entPtr->fts_path) != 0)
                    {
                        LE_CRIT("Failed to delete file '%s'. (%m)", entPtr->fts_path);
                        result = LE_FAULT;
                    }
                }

                break;
            }

            case FTS_DEFAULT:
                if (S_ISCHR(entPtr->fts_statp->st_mode) || S_ISBLK(entPtr->fts_statp->st_mode))
                {
                    if (unlink(entPtr->fts_path) != 0)
                    {
                        LE_ERROR("Failed to delete file '%s'. (%m)", entPtr->fts_path);
                        if (ftsPtr)
                        {
                           fts_close(ftsPtr);
                        }

                        return LE_FAULT;
                    }
                }

                break;

            default:
            {
                LE_CRIT("Unexpected file type %d in app '%s' <%s> in current system.",
                        entPtr->fts_info,
                        appNamePtr,
                        appMd5Ptr);
                LE_CRIT("Offending path: '%s'.", entPtr->fts_path);
                result = LE_FAULT;
                break;
            }
        }
    }

    fts_close(ftsPtr);

    return result;
}


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
)
{
    le_result_t result = LE_OK;

    // Delete files from system that are not in the app's install dir's writeable files.
    char appWriteableDirPath[PATH_MAX];
    int baseDirPathLen = snprintf(appWriteableDirPath,
                                 sizeof(appWriteableDirPath),
                                 "/legato/systems/%s/appsWriteable/%s",
                                 systemNamePtr,
                                 appNamePtr);
    LE_ASSERT(baseDirPathLen < sizeof(appWriteableDirPath));
    char* pathArrayPtr[] = { appWriteableDirPath, NULL };
    FTS*  ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL, NULL);

    if (ftsPtr == NULL)
    {
        LE_CRIT("Failed to open '%s' for traversal (%m).", appWriteableDirPath);
        return LE_FAULT;
    }

    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        // Compute the equivalent path in the app install directory.
        char appInstallPath[PATH_MAX];
        int n = snprintf(appInstallPath,
                         sizeof(appInstallPath),
                         "/legato/apps/%s/writeable/%s",
                         appMd5Ptr,
                         entPtr->fts_path + baseDirPathLen);
        if (n >= sizeof(appInstallPath))
        {
            LE_FATAL("Path to writeable file in app '%s' <%s> in app install dir is too long.",
                     appNamePtr,
                     appMd5Ptr);
            result = LE_FAULT;
            continue;

        }

        // Act differently depending on whether we are looking at a file or a directory.
        switch (entPtr->fts_info)
        {
            case FTS_DP: // Directory visited in post-order.
            case FTS_D: // Directory visited in pre-order.
            case FTS_NS: // appsWriteable dir doesn't even exist for this app.
            case FTS_SL:
            case FTS_F:
                break;

            // remove device
            case FTS_DEFAULT:
                if (S_ISCHR(entPtr->fts_statp->st_mode) || S_ISBLK(entPtr->fts_statp->st_mode))
                {
                    if (unlink(entPtr->fts_path) != 0)
                    {
                        LE_ERROR("Failed to delete file '%s'. (%m)", entPtr->fts_path);
                        if (ftsPtr)
                        {
                           fts_close(ftsPtr);
                        }

                        return LE_FAULT;
                    }
                }

                break;

            default:
            {
                LE_CRIT("Unexpected file type %d in app '%s' <%s> in current system.",
                        entPtr->fts_info,
                        appNamePtr,
                        appMd5Ptr);
                LE_CRIT("Offending path: '%s'.", entPtr->fts_path);
                result = LE_FAULT;
                break;
            }
        }
    }

    fts_close(ftsPtr);

    return result;
}
