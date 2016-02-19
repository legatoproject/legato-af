//--------------------------------------------------------------------------------------------------
/**
 * @file system.c
 *
 * /legato/systems/
 *                 current/
 *                 <index>
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "limit.h"
#include "fileDescriptor.h"
#include "properties.h"
#include "supCtrl.h"
#include "file.h"
#include "system.h"


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to where systems are installed.
 */
//--------------------------------------------------------------------------------------------------
#define SYSTEM_PATH "/legato/systems"


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to where the current running system is installed.
 */
//--------------------------------------------------------------------------------------------------
#define CURRENT_BASE_PATH SYSTEM_PATH "/current"


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to where new systems are unpacked.
 */
//--------------------------------------------------------------------------------------------------
#define UNPACK_BASE_PATH SYSTEM_PATH "/unpack"


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to where systems are installed.
 */
//--------------------------------------------------------------------------------------------------
static const char* SystemPath = SYSTEM_PATH;


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to a installed system directory.  The system index must be specified by
 * a snpritnf.
 */
//--------------------------------------------------------------------------------------------------
static const char* NumberedSystemPath = SYSTEM_PATH "/%d";


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to where systems are installed.
 */
//--------------------------------------------------------------------------------------------------
static const char* CurrentSystemPath = CURRENT_BASE_PATH;


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to where the current running system's "modified" file is.
 */
//--------------------------------------------------------------------------------------------------
static const char* CurrentModifiedFilePath = CURRENT_BASE_PATH "/modified";


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to where the current running system's "status" file is.
 */
//--------------------------------------------------------------------------------------------------
static const char* CurrentStatusPath = CURRENT_BASE_PATH "/status";


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to where a freshly unpacked system's "status" file is.
 */
//--------------------------------------------------------------------------------------------------
const char* UnpackStatusPath = UNPACK_BASE_PATH "/status";


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to where the current running system's "info.properties" file is.
 */
//--------------------------------------------------------------------------------------------------
static const char* CurrentPropertiesFilePath = CURRENT_BASE_PATH "/info.properties";


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to where the a numbered system's "info.properties" file.  The system
 * index must be specified by a snpritnf.
 */
//--------------------------------------------------------------------------------------------------
static const char* NumberedSystemPropertiesPath = SYSTEM_PATH "/%d/info.properties";


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to where the current running system's "version" file is.
 */
//--------------------------------------------------------------------------------------------------
static const char* CurrentVersionFilePath = CURRENT_BASE_PATH "/version";


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to directory containing the current running system's config tree files.
 */
//--------------------------------------------------------------------------------------------------
static const char* UnpackConfigDirPath = UNPACK_BASE_PATH "/config";


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to the directory that systems get unpacked into.
 */
//--------------------------------------------------------------------------------------------------
const char* system_UnpackPath = UNPACK_BASE_PATH;


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to directory containing apps in current running system.
 **/
//--------------------------------------------------------------------------------------------------
const char* system_CurrentAppsDir = CURRENT_BASE_PATH "/apps";


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to directory containing writeable app files in the current system.
 **/
//--------------------------------------------------------------------------------------------------
static const char* CurrentAppsWriteableDir = CURRENT_BASE_PATH "/appsWriteable";


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to directory containing writeable app files in system unpack area.
 **/
//--------------------------------------------------------------------------------------------------
static const char* UnpackAppsWriteableDir = UNPACK_BASE_PATH "/appsWriteable";


// People should really use the const variables, so undefine the macros.
#undef SYSTEM_PATH
#undef CURRENT_BASE_PATH
#undef UNPACK_BASE_PATH


//--------------------------------------------------------------------------------------------------
/**
 * Prepare the unpack directory for use (delete any old one and create a fresh empty one).
 */
//--------------------------------------------------------------------------------------------------
void system_PrepUnpackDir
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Clear out the current unpack dir, if it exists, then make sure it exists.
    system_KillUnpackDir();
    le_dir_MakePath(system_UnpackPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove the systems unpack directory.
 */
//--------------------------------------------------------------------------------------------------
void system_KillUnpackDir
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Clear out the current unpack dir, if it exists.
    LE_FATAL_IF(le_dir_RemoveRecursive(system_UnpackPath) != LE_OK,
                "Failed to recursively delete '%s'.",
                system_UnpackPath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the index of the current system.
 */
//--------------------------------------------------------------------------------------------------
static void SetIndex
(
    const char* systemNamePtr,  ///< Which system are we setting the index on?
    int newIndex                ///< [IN] The new index to set.
)
//--------------------------------------------------------------------------------------------------
{
    char pathBuffer[100] = "";
    snprintf(pathBuffer, sizeof(pathBuffer), "/legato/systems/%s/index", systemNamePtr);

    char numBuffer[12] = "";
    LE_ASSERT(snprintf(numBuffer, sizeof(numBuffer), "%d", newIndex) < sizeof(numBuffer));

    file_WriteStrAtomic(pathBuffer, numBuffer);

    LE_DEBUG("System index set to %d", newIndex);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the version string for the current system.
 */
//--------------------------------------------------------------------------------------------------
static void SetVersion
(
    const char* newVersion  ///< [IN] Set the current systems version to this string.
)
//--------------------------------------------------------------------------------------------------
{
    file_WriteStrAtomic(CurrentVersionFilePath, newVersion);

    LE_DEBUG("System version set to '%s'", newVersion);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a given system's index.
 *
 * @return System index or -1 on error.
 */
//--------------------------------------------------------------------------------------------------
static int GetIndex
(
    const char* systemNamePtr
)
//--------------------------------------------------------------------------------------------------
{
    char path[PATH_MAX];
    LE_ASSERT(snprintf(path, sizeof(path), "%s/%s/index", SystemPath, systemNamePtr)
              < sizeof(path));

    if (file_Exists(path))
    {
        char numBuffer[12] = "";

        ssize_t bytesRead = file_ReadStr(path, numBuffer, sizeof(numBuffer));

        if (bytesRead == -1)
        {
            LE_CRIT("Failed to read system index file '%s'.", path);
        }
        else if (bytesRead == 0)
        {
            LE_CRIT("System index file '%s' is empty.", path);
        }
        else
        {
            int index;
            le_result_t result = le_utf8_ParseInt(&index, numBuffer);
            if (result != LE_OK)
            {
                LE_CRIT("System index file '%s' contained invalid integer value '%s'. (%s)",
                        path,
                        numBuffer,
                        LE_RESULT_TXT(result));
            }
            else
            {
                return index;
            }
        }
    }

    return -1;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the current system's index.
 *
 * @return System index.
 */
//--------------------------------------------------------------------------------------------------
int system_Index
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    int index = GetIndex("current");

    if (index < 0)
    {
        if (system_Status() != SYS_GOOD)
        {
            LE_FATAL("Going down because of problems with system index file.");
        }

        // Limp along.
        LE_CRIT("Assuming system index is 0.");
        index = 0;
    }

    return index;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the index for the previous system in the chain, using the current system as a starting point.
 *
 * @return The index to the system that's previous to the given system.  -1 is returned if the
 *         previous system was not found.
 */
//--------------------------------------------------------------------------------------------------
int system_GetPreviousSystemIndex
(
    int systemIndex  ///< [IN] Get the system that's earlier to this system.
)
//--------------------------------------------------------------------------------------------------
{
    // Iterate through the system directories, ignoring a system being unpacked...  Look for the
    // highest system index that is less than the requested index.

    int highestFound = -1;

    char* pathArrayPtr[] = { (char*)SystemPath, NULL };
    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL, NULL);
    FTSENT* entPtr;

    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        if (   (entPtr->fts_info == FTS_D)
            && (entPtr->fts_level > 0))
        {
            char* namePtr = le_path_GetBasenamePtr(entPtr->fts_path, "/");

            // No need to recurse into sub-directories.
            fts_set(ftsPtr, entPtr, FTS_SKIP);

            if (strcmp(namePtr, "unpack") != 0)
            {
                int index = GetIndex(namePtr);

                if (   (index < systemIndex)
                    && (index > highestFound))
                {
                    highestFound = index;
                }
            }
        }
    }

    fts_close(ftsPtr);

    return highestFound;
}


//--------------------------------------------------------------------------------------------------
/**
 * Read the current system's version string (null-terminated).
 *
 * @return Length of the version string, in bytes, excluding the null terminator.
 *         0 if version string could not be read.
 */
//--------------------------------------------------------------------------------------------------
size_t system_Version
(
    char* versionPtr,   ///< [OUT] Buffer to write the version string too.
    size_t versionSize  ///< [IN]  Size of the buffer passed in.
)
//--------------------------------------------------------------------------------------------------
{
    ssize_t len = file_ReadStr(CurrentVersionFilePath, versionPtr, versionSize);

    if (len == -1)
    {
        LE_WARN("Could not read the current legato version from %s", CurrentVersionFilePath);
        return 0;
    }

    LE_DEBUG("Current Legato system version: %s", versionPtr);

    return len;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check to see if the given system exists.
 *
 * @return True if the system in question exists, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool system_Exists
(
    int systemIndex  ///< [IN] The system index to check.
)
//--------------------------------------------------------------------------------------------------
{
    char path[PATH_MAX];
    int n = snprintf(path, sizeof(path), NumberedSystemPath, systemIndex);

    LE_ASSERT(n < sizeof(path));

    if (le_dir_IsDir(path))
    {
        return true;
    }

    return system_Index() == systemIndex;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the status of the current system.
 *
 * @return The system status (SYS_BAD on error).
 */
//--------------------------------------------------------------------------------------------------
static system_Status_t GetStatus
(
    const char* systemNamePtr
)
//--------------------------------------------------------------------------------------------------
{
    char path[PATH_MAX];
    LE_ASSERT(snprintf(path, sizeof(path), "%s/%s/status", SystemPath, systemNamePtr)
              < sizeof(path));

    char statusBuffer[100] = "";

    if (file_Exists(path) == false)
    {
        LE_DEBUG("System status file '%s' does not exist, assuming untried system.", path);
        return SYS_PROBATION;
    }

    if (file_ReadStr(path, statusBuffer, sizeof(statusBuffer)) == -1)
    {
        LE_DEBUG("The system status file could not be read, '%s', assuming a bad system.", path);

        return SYS_BAD;
    }

    if (strcmp("good", statusBuffer) == 0)
    {
        return SYS_GOOD;
    }

    if (strcmp("bad", statusBuffer) == 0)
    {
        return SYS_BAD;
    }

    char* triedStr = "tried ";
    size_t triedSize = strlen(triedStr);

    if (strncmp(triedStr, statusBuffer, triedSize) == 0)
    {
        return SYS_PROBATION;
    }

    LE_ERROR("Unknown system status '%s' found in file '%s'.", statusBuffer, path);

    return SYS_BAD;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the status of the current system.
 *
 * @return the system status.
 */
//--------------------------------------------------------------------------------------------------
system_Status_t system_Status
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    system_Status_t status = GetStatus("current");

    if (status == SYS_BAD)
    {
        LE_FATAL("Currently running a 'bad' system!");
    }

    return status;
}


//--------------------------------------------------------------------------------------------------
/**
 * If the current system status is in Probation, then this returns the number of times the system
 * has been tried while in probation.
 *
 * Do not call this if you are not in probation!
 *
 * @return A count of the number of times the system has been "tried."
 */
//--------------------------------------------------------------------------------------------------
int system_TryCount
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    char statusBuffer[100] = "";

    if (file_ReadStr(CurrentStatusPath, statusBuffer, sizeof(statusBuffer)) == -1)
    {
        LE_WARN("The system status file could not be found, '%s', assuming a bad system.",
                CurrentStatusPath);
        return 10;
    }

    char* triedStr = "tried ";
    size_t triedSize = strlen(triedStr);

    if (strncmp(triedStr, statusBuffer, triedSize) == 0)
    {
        int tryCount;

        le_result_t result = le_utf8_ParseInt(&tryCount, &statusBuffer[triedSize]);

        if (result != LE_OK)
        {
            LE_FATAL("System try count '%s' is not a valid integer. (%s)",
                     &statusBuffer[triedSize],
                     LE_RESULT_TXT(result));
        }
        else
        {
            return tryCount;
        }
    }

    LE_FATAL("Current system not in probation, so try count is invalid.");
    return 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * Increment the try count.
 */
//--------------------------------------------------------------------------------------------------
void system_IncrementTryCount
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    int tryCount = system_TryCount();
    char statusBuffer[100] = "";

    ++tryCount;

    snprintf(statusBuffer, sizeof(statusBuffer), "tried %d", tryCount);
    file_WriteStrAtomic(CurrentStatusPath, statusBuffer);
}


//--------------------------------------------------------------------------------------------------
/**
 * Read the hash ID from a given system.
 *
 * @return
 *      - LE_OK if no problems are encountered.
 *      - LE_NOT_FOUND if the given index does not correspond to an available system.
 *      - LE_FORMAT_ERROR if there are problems reading the hash from the system.
 */
//--------------------------------------------------------------------------------------------------
le_result_t system_GetSystemHash
(
    int systemIndex,                        ///< [IN]  The system to read the hash for.
    char hashBuffer[LIMIT_MD5_STR_BYTES]    ///< [OUT] Buffer to hold the system's hash ID.
)
//--------------------------------------------------------------------------------------------------
{
    if (system_Exists(systemIndex) == false)
    {
        return LE_NOT_FOUND;
    }

    const char* propertyPathPtr = NULL;
    char propPathBuffer[PATH_MAX];

    // If we are getting a hash for a system that is not current, build a path to it's property
    // file.  Otherwise just use the current system path.
    if (system_Index() != systemIndex)
    {
        int n = snprintf(propPathBuffer,
                         sizeof(propPathBuffer),
                         NumberedSystemPropertiesPath,
                         systemIndex);

        LE_ASSERT(n < sizeof(propPathBuffer));

        propertyPathPtr = propPathBuffer;
    }
    else
    {
        propertyPathPtr = CurrentPropertiesFilePath;
    }

    // Make sure that the property file exists and then attempt to read from it.
    if (file_Exists(propertyPathPtr) == false)
    {
        LE_ERROR("The system property file, '%s', is missing.", propertyPathPtr);
        return LE_NOT_FOUND;
    }

    le_result_t result = properties_GetValueForKey(propertyPathPtr,
                                                   "system.md5",
                                                   hashBuffer,
                                                   LIMIT_MD5_STR_BYTES);

    // If the md5 is missing, or if for some reason the string is too big, then we have a faulty
    // properties file.
    if (result != LE_OK)
    {
        LE_ERROR("Error, '%s', while reading system property file, '%s'.",
                 LE_RESULT_TXT(result),
                 propertyPathPtr);

        return LE_FORMAT_ERROR;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a symlink to a given app's non-writeable files in a given system.
 **/
//--------------------------------------------------------------------------------------------------
void system_SymlinkApp
(
    const char* systemNamePtr,  ///< E.g., "current" or "unpack".
    const char* appMd5Ptr,
    const char* appNamePtr
)
//--------------------------------------------------------------------------------------------------
{
    char linkPath[PATH_MAX];
    char linkTargetPath[PATH_MAX];

    int n;
    n = snprintf(linkPath,
                 sizeof(linkPath),
                 "%s/%s/apps/%s",
                 SystemPath,
                 systemNamePtr,
                 appNamePtr);
    LE_ASSERT(n < sizeof(linkPath));

    n = snprintf(linkTargetPath,
                 sizeof(linkTargetPath),
                 "/legato/apps/%s",
                 appMd5Ptr);
    LE_ASSERT(n < sizeof(linkTargetPath));

    // If the symlink already exists, delete it.
    if ((unlink(linkPath) == -1) && (errno != ENOENT))
    {
        LE_FATAL("Failed to delete old symlink '%s': %m.", linkPath);
    }

    LE_INFO("Creating symlink %s -> %s", linkPath, linkTargetPath);

    // Create the symlink.
    if (symlink(linkTargetPath, linkPath) != 0)
    {
        LE_FATAL("Failed to create symlink '%s' pointing to '%s': %m.", linkPath, linkTargetPath);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Install a given app's writeable files in the "unpack" system from either the app's install
 * directory (/legato/apps/<hash>) or the current running system, as appropriate for each file.
 *
 * @warning Assumes the app identified by the hash is installed in /legato/apps/<hash>.
 *
 * @return LE_OK if successful.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t system_InstallAppWriteableFiles
(
    const char* appMd5Ptr,
    const char* appNamePtr
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result = LE_OK;

    char freshWriteablesDir[PATH_MAX];

    char appLabel[APPSMACK_MAX_LABEL_LEN + 1];
    supCtrl_GetLabel(appNamePtr, appLabel, sizeof(appLabel));

    int baseDirPathLen = snprintf(freshWriteablesDir,
                     sizeof(freshWriteablesDir),
                     "/legato/apps/%s/writeable",
                     appMd5Ptr);
    LE_ASSERT(baseDirPathLen < sizeof(freshWriteablesDir));

    char* pathArrayPtr[] = { freshWriteablesDir, NULL };
    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL, NULL);

    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        // Compute the destination path in the unpack area.
        char destPath[PATH_MAX];
        int n = snprintf(destPath,
                         sizeof(destPath),
                         "%s/%s%s",
                         UnpackAppsWriteableDir,
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
                    if (le_dir_MakePath(destPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH ) != LE_OK)
                    {
                        LE_CRIT("Failed to create directory '%s'.", destPath);
                        result = LE_FAULT;
                        goto cleanup;
                    }
                }
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
                // Compute the path the file would appear at in the current running system.
                char oldVersionPath[PATH_MAX];
                int n = snprintf(oldVersionPath,
                                 sizeof(oldVersionPath),
                                 "%s/%s%s",
                                 CurrentAppsWriteableDir,
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

                // If the file exists in the current running system, copy that.
                if (file_Exists(oldVersionPath))
                {
                    if (file_Copy(oldVersionPath, destPath, appLabel) != LE_OK)
                    {
                        result = LE_FAULT;
                        goto cleanup;
                    }
                }
                // If the file does not exist in the current running system, install the fresh one.
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
 * Update a given app's writeable files in the "current" system to match what's in the app's install
 * directory (/legato/apps/<hash>).  Deletes from the current system files that are not in the app's
 * install directory.  Adds to the current system files from the apps' install directory that are
 * not in the current system.  Leaves alone files that are in both the current system and the app's
 * install directory.
 *
 * @warning Assumes the app identified by the hash is installed in /legato/apps/<hash>.
 **/
//--------------------------------------------------------------------------------------------------
void system_UpdateCurrentAppWriteableFiles
(
    const char* appMd5Ptr,
    const char* appNamePtr
)
//--------------------------------------------------------------------------------------------------
{
    char freshWriteablesDir[PATH_MAX];

    int baseDirPathLen = snprintf(freshWriteablesDir,
                     sizeof(freshWriteablesDir),
                     "/legato/apps/%s/writeable",
                     appMd5Ptr);
    LE_ASSERT(baseDirPathLen < sizeof(freshWriteablesDir));

    char* pathArrayPtr[] = { freshWriteablesDir, NULL };
    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_LOGICAL, NULL);

    char appLabel[APPSMACK_MAX_LABEL_LEN + 1];
    supCtrl_GetLabel(appNamePtr, appLabel, sizeof(appLabel));

    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        // Compute the destination path in the current system.
        char destPath[PATH_MAX];
        int n = snprintf(destPath,
                         sizeof(destPath),
                         "%s/%s%s",
                         CurrentAppsWriteableDir,
                         appNamePtr,
                         entPtr->fts_path + baseDirPathLen);
        if (n >= sizeof(destPath))
        {
            LE_FATAL("Path to writeable file in app '%s' <%s> in current system is too long.",
                    appNamePtr,
                    appMd5Ptr);
        }

        // Act differently depending on whether we are looking at a file or a directory.
        switch (entPtr->fts_info)
        {
            case FTS_D: // Directory visited in pre-order.

                // If this is not the top level directory, create it.
                if (entPtr->fts_level > 0)
                {
                    if (le_dir_MakePath(destPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH ) != LE_OK)
                    {
                        LE_FATAL("Failed to create directory '%s'.", destPath);
                    }
                }
                break;

            case FTS_SL:

                LE_FATAL("Symlink in writeable files for app '%s' <%s> (%s).",
                         appNamePtr,
                         appMd5Ptr,
                         entPtr->fts_path);
                break;

            case FTS_F:
            {
                // If the file does not exist in the current running system, add it.
                if (!file_Exists(destPath))
                {
                    if (file_Copy(entPtr->fts_path, destPath, appLabel) != LE_OK)
                    {
                        LE_FATAL("Failed to copy");
                    }
                }

                break;
            }

            case FTS_DP: // Directory visited in post-order (ignore).
            case FTS_NS: // Path doesn't exist (ignore).

                break;

            default:

                LE_EMERG("Unexpected file type %d in app '%s' <%s>.",
                         entPtr->fts_info,
                         appNamePtr,
                         appMd5Ptr);
                LE_FATAL("Offending path: '%s'.", entPtr->fts_path);
                break;
        }
    }

    fts_close(ftsPtr);

    // Delete files from current app that are not in the app's install dir's writeable files.
    char appWriteableDirPath[PATH_MAX];
    baseDirPathLen = snprintf(appWriteableDirPath,
                              sizeof(appWriteableDirPath),
                              "%s/%s",
                              CurrentAppsWriteableDir,
                              appNamePtr);
    LE_ASSERT(baseDirPathLen < sizeof(appWriteableDirPath));
    pathArrayPtr[0] = appWriteableDirPath;
    ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL, NULL);

    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        // Compute the equivalent path in the app install directory.
        char appInstallPath[PATH_MAX];
        int n = snprintf(appInstallPath,
                         sizeof(appInstallPath),
                         "/legato/apps/%s%s",
                         appMd5Ptr,
                         entPtr->fts_path + baseDirPathLen);
        if (n >= sizeof(appInstallPath))
        {
            LE_FATAL("Path to writeable file in app '%s' <%s> in app install dir is too long.",
                     appNamePtr,
                     appMd5Ptr);
        }

        // Act differently depending on whether we are looking at a file or a directory.
        switch (entPtr->fts_info)
        {
            case FTS_D: // Directory visited in pre-order.

                // If this does not exist in the new app, delete it.
                if ((entPtr->fts_level > 0) && (!le_dir_IsDir(appInstallPath)))
                {
                    if (le_dir_RemoveRecursive(entPtr->fts_path) != LE_OK)
                    {
                        LE_FATAL("Failed to delete directory '%s'.", entPtr->fts_path);
                    }
                }
                break;

            case FTS_DP: // Directory visited in post-order.
            case FTS_NS: // appsWriteable dir doesn't even exist for this app.

                // Ignore.
                break;

            case FTS_F:
            {
                // If the file does not exist in the new app, delete the file from the current.
                if (!file_Exists(appInstallPath))
                {
                    if (unlink(entPtr->fts_path) != 0)
                    {
                        LE_FATAL("Failed to delete file '%s'. (%m)", entPtr->fts_path);
                    }
                }

                break;
            }

            default:

                LE_EMERG("Unexpected file type %d in app '%s' <%s> in current system.",
                         entPtr->fts_info,
                         appNamePtr,
                         appMd5Ptr);
                LE_FATAL("Offending path: '%s'.", entPtr->fts_path);
                break;
        }
    }

    fts_close(ftsPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove a given app's files from the current running system.
 **/
//--------------------------------------------------------------------------------------------------
void system_RemoveApp
(
    const char* appNamePtr
)
//--------------------------------------------------------------------------------------------------
{
    char path[PATH_MAX];

    int n = snprintf(path, sizeof(path), "%s/%s", CurrentAppsWriteableDir, appNamePtr);

    LE_ASSERT(n < sizeof(path));

    LE_FATAL_IF(le_dir_RemoveRecursive(path) != LE_OK,
                "Failed to recursively delete '%s'.",
                path);

    // Delete The symlink.
    snprintf(path, sizeof(path), "%s/%s", system_CurrentAppsDir, appNamePtr);
    LE_ERROR_IF(unlink(path) != 0, "Failed to unlink '%s' (%m)", path);
}


//--------------------------------------------------------------------------------------------------
/**
 * Copy a given app's (app-specific) config file from the current running system to the system
 * unpack area.
 *
 * @return LE_OK if successful.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t system_CopyAppConfig
(
    const char* appNamePtr
)
//--------------------------------------------------------------------------------------------------
{
    char filePath[PATH_MAX];
    int n = snprintf(filePath, sizeof(filePath), "%s/%s.scissors", UnpackConfigDirPath, appNamePtr);
    LE_ASSERT(n < sizeof(filePath));

    char configTreePath[100];
    n = snprintf(configTreePath, sizeof(configTreePath), "%s:/", appNamePtr);
    LE_FATAL_IF(n >= sizeof(configTreePath), "App name too long.");

    le_cfg_IteratorRef_t i = le_cfg_CreateReadTxn(configTreePath);

    le_result_t result = le_cfgAdmin_ExportTree(i, filePath, "");

    le_cfg_CancelTxn(i);

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Complete a system update and move the system from unpack into current.
 *
 * @return LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t system_FinishUpdate
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Get the next system index.
    int currentIndex = system_Index()  + 1;

    // Set the new system index, and update the new systems status so that its in probation.
    SetIndex("unpack", currentIndex);


    // Copy the old config, and do it as an atomic transaction so that we do not catch anything in
    // the middle of writing to the system config.
    le_cfg_IteratorRef_t iter = le_cfg_CreateReadTxn("system:/");
    le_result_t result;

    result = le_cfgAdmin_ExportTree(iter, "/legato/systems/unpack/config/system.paper", "/");
    le_cfg_CancelTxn(iter);

    if (result != LE_OK)
    {
        return LE_FAULT;
    }

    // Now, move the unpacked system into its index.
    char newSystemPath[100] = "";
    snprintf(newSystemPath, sizeof(newSystemPath), "%s/%d", SystemPath, currentIndex);
    file_Rename(system_UnpackPath, newSystemPath);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Take a snapshot of the current system.
 *
 * @return LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t system_Snapshot
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    system_Status_t status = system_Status();

    if (status != SYS_GOOD)
    {
        LE_WARN("System has not yet passed probation, no snapshot taken.");

        return LE_OK;
    }

    // Copy the current system to the work dir.
    int currentIndex = system_Index();

    system_PrepUnpackDir();
    if (file_CopyRecursive(CurrentSystemPath, system_UnpackPath, NULL) != LE_OK)
    {
        return LE_FAULT;
    }

    // Atomically rename the work dir to the proper index
    char newSystemPath[100] = "";
    snprintf(newSystemPath, sizeof(newSystemPath), "%s/%d", SystemPath, currentIndex);

    LE_DEBUG("Creating system snapshot '%s'", newSystemPath);

    file_Rename(system_UnpackPath, newSystemPath);

    // Increment the index of the current system.
    SetIndex("current", currentIndex + 1);

    LE_INFO("Snapshot taken of system index %d.  Current system index is now %d.",
            currentIndex,
            currentIndex + 1);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark the system as being modified.
 */
//--------------------------------------------------------------------------------------------------
void system_MarkModified
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // If the system is already modified, there's not much left to do.
    if (system_IsModifed())
    {
        return;
    }

    // Attempt to open the modified file.  Its mere existence marks the system as modified, so once
    // the file is created, we simply leave it empty and close it again.
    int fd = -1;
    do
    {
        fd = open(CurrentModifiedFilePath,
                  O_CREAT | O_WRONLY | O_TRUNC,
                  S_IRUSR | S_IWUSR);
    }
    while (   (fd == -1)
           && (errno == EINTR));

    if (fd != -1)
    {
        fd_Close(fd);
    }
    else
    {
        LE_FATAL("Could not mark the current system as modified because of a file error: %m.");
    }

    // Now, clear the system hash from its properties files, so that other tools don't think they
    // can compute a proper diff on this system.
    LE_FATAL_IF(properties_SetValueForKey(CurrentPropertiesFilePath,
                                          "system.md5",
                                          "modified") != LE_OK,
                "Failed to update the system properties file.");

    LE_FATAL_IF((unlink(CurrentStatusPath) == -1) && (errno != ENOENT),
                "Unable to delete '%s' (%m).",
                CurrentStatusPath);

    // Update the version string.
    static const char modifiedStr[] = "_modified";
    char versionBuffer[200] = "";
    char newVersionBuffer[sizeof(versionBuffer) + sizeof(modifiedStr) + 1] = ""; // +1 for newline

    // Get the old version.
    size_t versionLen = system_Version(versionBuffer, sizeof(versionBuffer));

    // Remove newline if there is one.
    if (versionBuffer[versionLen-1] == '\n')
    {
        versionBuffer[versionLen-1] = '\0';
    }

    // Append "_modified".
    snprintf(newVersionBuffer, sizeof(newVersionBuffer), "%s%s\n", versionBuffer, modifiedStr);

    // Write the new version string.
    SetVersion(newVersionBuffer);

    LE_INFO("Current system is now \"modified\".");
}


//--------------------------------------------------------------------------------------------------
/**
 * Check to see if the current system has been marked as modified.
 *
 * @return true if the system has been modified, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool system_IsModifed
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    return file_Exists(CurrentModifiedFilePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark the system "bad".
 */
//--------------------------------------------------------------------------------------------------
void system_MarkBad
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    file_WriteStrAtomic(CurrentStatusPath, "bad");
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark the system "tried 1".
 */
//--------------------------------------------------------------------------------------------------
void system_MarkTried
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    file_WriteStrAtomic(CurrentStatusPath, "tried 1");

    LE_INFO("Current system has been marked \"tried 1\".");
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark the system "good".
 */
//--------------------------------------------------------------------------------------------------
void system_MarkGood
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    file_WriteStrAtomic(CurrentStatusPath, "good");
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks to see if an application with a given name is used in the current running system.
 *
 * @return true if the app is in the system.
 */
//--------------------------------------------------------------------------------------------------
bool system_HasApp
(
    const char* appNamePtr  ///< [IN] Look for this app name.
)
//--------------------------------------------------------------------------------------------------
{
    char path[PATH_MAX] = "";
    int count = snprintf(path, sizeof(path), "%s/%s", system_CurrentAppsDir, appNamePtr);
    LE_ASSERT(count < sizeof(path));

    return le_dir_IsDir(path);
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete any apps that are not used by any systems (including the "unpack" system, if there is one).
 */
//--------------------------------------------------------------------------------------------------
void system_RemoveUnusedApps
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Walk the list of directories in /legato/apps/, and for each one, see if the app is
    // used in any system.
    char* pathArrayPtr[] = { "/legato/apps", NULL };
    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL, NULL);

    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        if (entPtr->fts_level == 1)
        {
            if ((entPtr->fts_info == FTS_D) || (entPtr->fts_info == FTS_SL))
            {
                char* foundHashPtr = le_path_GetBasenamePtr(entPtr->fts_path, "/");

                if (!system_AppUsedInAnySystem(foundHashPtr))
                {
                    LE_INFO("Removing unused app with MD5 sum %s.", foundHashPtr);

                    if (le_dir_RemoveRecursive(entPtr->fts_path) != LE_OK)
                    {
                        LE_ERROR("Unable to remove '%s'.", entPtr->fts_path);
                    }
                }
                else
                {
                    LE_INFO("App with MD5 sum %s is still needed.", foundHashPtr);
                }

                // We don't need to go into this directory.
                fts_set(ftsPtr, entPtr, FTS_SKIP);
            }
            else if (entPtr->fts_info != FTS_DP)
            {
                LE_ERROR("Unexpected file type %d at '%s'", entPtr->fts_info, entPtr->fts_path);
            }
        }
    }

    fts_close(ftsPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Check all installed systems and see if the given application is installed in any of them.  The
 * search is done by application MD5 hash.
 *
 * @return true if there is any system with the given application installed, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool system_AppUsedInAnySystem
(
    const char* appHashPtr  ///< [IN] Look for this app hash.
)
//--------------------------------------------------------------------------------------------------
{
    char* pathArrayPtr[] = { "/legato/systems", NULL };
    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL, NULL);

    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        switch (entPtr->fts_info)
        {
            case FTS_D:
                if (entPtr->fts_level > 3)
                {
                    // We don't need to go past the apps directory.
                    fts_set(ftsPtr, entPtr, FTS_SKIP);
                }
                break;

            case FTS_SL:
                // We're looking for symlinks 3 levels deep, where <appName> is a symlink to the
                // application hash directory, under /legato/apps/<hashId>

                // /legato/systems/<index>/apps/<appName>
                //               0       1    2         3

                // So, for each symlink, read its target, get the last node of it's path and see if
                // the directory name is our hash ID.  If it is, we have a system with the
                // application installed.
                if (entPtr->fts_level == 3)
                {
                    char buffer[1024] = "";
                    ssize_t bytesRead = readlink(entPtr->fts_path, buffer, sizeof(buffer));
                    if (bytesRead < 0)
                    {
                        LE_FATAL("Failed to read symlink '%s'.", entPtr->fts_path);
                    }
                    LE_ASSERT(bytesRead < sizeof(buffer));
                    buffer[bytesRead] = '\0'; // Null-terminate.

                    char* foundHashPtr = le_path_GetBasenamePtr(buffer, "/");

                    if (strcmp(appHashPtr, foundHashPtr) == 0)
                    {
                        fts_close(ftsPtr);
                        return true;
                    }
                }
                break;
        }
    }

    fts_close(ftsPtr);
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete any systems that are "bad" or older than the newest "good".
 */
//--------------------------------------------------------------------------------------------------
void system_RemoveUnneeded
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    system_Status_t status = system_Status();

    // Walk the list of directories in /legato/systems/ to find out which is the newest "good"
    // system, and delete any "bad" systems while we are at it.
    char * pathArrayPtr[] = { (char*)SystemPath, NULL };
    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL, NULL);

    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        if (entPtr->fts_level == 1)
        {
            if (entPtr->fts_info == FTS_D)
            {
                char* foundSystemPtr = le_path_GetBasenamePtr(entPtr->fts_path, "/");

                // Never delete the current system.
                if (strcmp(foundSystemPtr, "current") != 0)
                {
                    // If the current system is good, delete all other systems.
                    // Otherwise, we should only have one "good" system, so delete everything
                    // but the "good" system.
                    if ((status == SYS_GOOD) || (GetStatus(foundSystemPtr) != SYS_GOOD))
                    {
                        if (le_dir_RemoveRecursive(entPtr->fts_path) != LE_OK)
                        {
                            LE_ERROR("Unable to remove '%s'.", entPtr->fts_path);
                        }
                    }
                }

                // We don't need to go into this directory.
                fts_set(ftsPtr, entPtr, FTS_SKIP);
            }
            else if (entPtr->fts_info != FTS_DP)
            {
                LE_ERROR("Unexpected file type %d at '%s'", entPtr->fts_info, entPtr->fts_path);
            }
        }
    }

    fts_close(ftsPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the file system path to the directory into which a given app's writeable files are
 * found inside a given system.
 **/
//--------------------------------------------------------------------------------------------------
void system_GetAppWriteableFilesDirPath
(
    char* pathBuffPtr,          ///< [out] Ptr to where the path will be put.
    size_t buffSize,            ///< Size of the buffer.
    const char* systemNamePtr,
    const char* appNamePtr
)
//--------------------------------------------------------------------------------------------------
{
    int n = snprintf(pathBuffPtr,
                     buffSize,
                     "%s/%s/appsWriteable/%s",
                     SystemPath,
                     systemNamePtr,
                     appNamePtr);
    if (n >= buffSize)
    {
        LE_FATAL("Path buffer too small (< %zu).", buffSize);
    }
}

