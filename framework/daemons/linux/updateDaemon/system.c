//--------------------------------------------------------------------------------------------------
/**
 * @file system.c
 *
 * /legato/systems/
 *                 current/
 *                 <index>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "limit.h"
#include "fileDescriptor.h"
#include "fileSystem.h"
#include "properties.h"
#include "supCtrl.h"
#include "file.h"
#include "system.h"
#include "installer.h"
#include "sysPaths.h"
#include "sysStatus.h"
#include "smack.h"

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
 * Absolute file system path to where the current running system's "modified" file is.
 */
//--------------------------------------------------------------------------------------------------
static const char* CurrentModifiedFilePath = CURRENT_SYSTEM_PATH "/modified";


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to where the current running system's "info.properties" file is.
 */
//--------------------------------------------------------------------------------------------------
static const char* CurrentPropertiesFilePath = CURRENT_SYSTEM_PATH "/info.properties";


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
static const char* CurrentVersionFilePath = CURRENT_SYSTEM_PATH "/version";


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to directory containing the current running system's config tree files.
 */
//--------------------------------------------------------------------------------------------------
static const char* UnpackConfigDirPath = UNPACK_BASE_PATH "/config";


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to directory containing the current running system's app files.
 */
//--------------------------------------------------------------------------------------------------
static const char* UnpackAppsDirPath = UNPACK_BASE_PATH "/apps";


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to directory containing the current running system's lib files.
 */
//--------------------------------------------------------------------------------------------------
static const char* UnpackLibDirPath = UNPACK_BASE_PATH "/lib";


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to directory containing the current running system's bin files.
 */
//--------------------------------------------------------------------------------------------------
static const char* UnpackBinDirPath = UNPACK_BASE_PATH "/bin";


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to directory containing the current running system's module files.
 */
//--------------------------------------------------------------------------------------------------
static const char* UnpackModuleDirPath = UNPACK_BASE_PATH "/modules";


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
const char* system_CurrentAppsDir = CURRENT_SYSTEM_PATH "/apps";


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to directory containing writeable app files in the current system.
 **/
//--------------------------------------------------------------------------------------------------
static const char* CurrentAppsWriteableDir = CURRENT_SYSTEM_PATH "/appsWriteable";


// People should really use the const variables, so undefine the macros.
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

    file_WriteStrAtomic(pathBuffer, numBuffer, 0644 /* rw-r--r-- */);

    LE_DEBUG("System index set to %d", newIndex);
}


//--------------------------------------------------------------------------------------------------
/**
 *  Check whether a directory entry is a directory or not.
 *
 *  @return
 *       True if specified entry is a directory
 *       False otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool IsDir
(
    struct dirent* dirEntryPtr              ///< [IN] Directory entry in question.
)
{
    if (dirEntryPtr->d_type == DT_DIR)
    {
        return true;
    }
    else if (dirEntryPtr->d_type == DT_UNKNOWN)
    {
        // As per man page (http://man7.org/linux/man-pages/man3/readdir.3.html), DT_UNKNOWN
        // should be handled properly for portability purpose. Use lstat(2) to check file info.
        struct stat stbuf;

        if (lstat(dirEntryPtr->d_name, &stbuf) != 0)
        {
            LE_ERROR("Error when trying to lstat '%s'. (%m)", dirEntryPtr->d_name);
            return false;
        }

        return S_ISDIR(stbuf.st_mode);
    }

    return false;
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
    file_WriteStrAtomic(CurrentVersionFilePath, newVersion, 0644 /* rw-r--r-- */);

    LE_DEBUG("System version set to '%s'", newVersion);
}

//--------------------------------------------------------------------------------------------------
/**
 * Recursively sets the permissions for all files in new installed system 'lib' directory. All files
 * are set to SMACK label '_'.
 */
//--------------------------------------------------------------------------------------------------
static void SetSystemFilesPermissions
(
    const char* newSystemPath         ///< [IN] Path to new system.
)
{

    // Get the smack labels to use.
    char* fileLabel = "_";

    char* pathArrayPtr[] = {(char *)newSystemPath, NULL};

    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_LOGICAL | FTS_NOSTAT, NULL);

    LE_FATAL_IF(ftsPtr == NULL, "Could not access dir '%s'.  %m.", pathArrayPtr[0]);

    // Clear the errno
    errno = 0;

    // Step through the directory tree.
    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        switch (entPtr->fts_info)
        {
            case FTS_F:
            case FTS_NSOK:
                // These are files. Set the SMACK label.
                LE_DEBUG("Setting smack label: '%s' for file: '%s'", fileLabel,
                         entPtr->fts_accpath);
                smack_SetLabel(entPtr->fts_accpath, fileLabel);
                break;

        }
    }

    LE_CRIT_IF(errno != 0, "Could not traverse directory '%s'.  %m", pathArrayPtr[0]);

    fts_close(ftsPtr);
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
        if (sysStatus_Status() != SYS_GOOD)
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
 * Remove a symlink to a given app's non-writeable files in a given system.
 **/
//--------------------------------------------------------------------------------------------------
void system_UnlinkApp
(
    const char* systemNamePtr,  ///< E.g., "current" or "unpack".
    const char* appNamePtr
)
//--------------------------------------------------------------------------------------------------
{
    char linkPath[PATH_MAX];

    int n;
    n = snprintf(linkPath,
                 sizeof(linkPath),
                 "%s/%s/apps/%s",
                 SystemPath,
                 systemNamePtr,
                 appNamePtr);
    LE_ASSERT(n < sizeof(linkPath));

    // Remove the symlink
    if (unlink(linkPath) == -1)
    {
        LE_WARN("Failed to delete symlink '%s': %m.", linkPath);
    }
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
    LE_FATAL_IF(installer_UpdateAppWriteableFiles("current", appMd5Ptr, appNamePtr) != LE_OK,
                "Failed to update app writeable files in current system.");
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

    // Attempt to umount appsWritable/<appName> because it may have been mounted as a sandbox.
    fs_TryLazyUmount(path);

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

    // Change label of the config file so that it can be read by configTree
    smack_SetLabel("/legato/systems/unpack/config/system.paper", "framework");

    // Set the smackfs permission of unpacked system. This has to be done before renaming unpack
    // path to some index. Modify all files under lib and bin with "_" label.
    SetSystemFilesPermissions("/legato/systems/unpack/lib");
    SetSystemFilesPermissions("/legato/systems/unpack/bin");

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
    sysStatus_Status_t status = sysStatus_Status();

    // Copy the current system to the work dir.
    int currentIndex = system_Index();

    if (status != SYS_GOOD)
    {
        LE_WARN("System has not yet passed probation, no snapshot taken.");

        // Increment the index of the current system.
        SetIndex("current", currentIndex + 1);

        return LE_OK;
    }

    system_PrepUnpackDir();

    if (file_CopyRecursive(CURRENT_SYSTEM_PATH, system_UnpackPath, NULL) != LE_OK)
    {
        return LE_FAULT;
    }

    // Make sure everything under appsWriteable is copied too.  This is necessary because sandboxed
    // apps under appsWriteable may have been bind mounted unto itself.
    DIR* appsWriteableDir = opendir(APPS_WRITEABLE_DIR);

    if (appsWriteableDir == NULL)
    {
        LE_ERROR("Error opening directory %s.  %m.", APPS_WRITEABLE_DIR);
        return LE_FAULT;
    }

    le_result_t result = LE_OK;

    while (1)
    {
        errno = 0;

        struct dirent* dirPtr = readdir(appsWriteableDir);

        if (dirPtr == NULL)
        {
            if (errno != 0)
            {
                LE_ERROR("Error reading directory %s.  %m.", APPS_WRITEABLE_DIR);
                result = LE_FAULT;
            }

            break;
        }

        if ( (IsDir(dirPtr)) &&
             (strcmp(dirPtr->d_name, ".") != 0) &&
             (strcmp(dirPtr->d_name, "..") != 0) )
        {
            // Get a path name to the source directory.
            char sourceDir[LIMIT_MAX_PATH_BYTES] = APPS_WRITEABLE_DIR;

            if (le_path_Concat("/", sourceDir, sizeof(sourceDir), dirPtr->d_name, NULL) != LE_OK)
            {
                LE_ERROR("Directory name '%s...' is too long.", sourceDir);
                result = LE_FAULT;
                break;
            }

            if (fs_IsMountPoint(sourceDir))
            {
                // Get a path name to the destination directory.
                char destDir[LIMIT_MAX_PATH_BYTES] = "";

                if (le_path_Concat("/", destDir, sizeof(destDir), system_UnpackPath,
                                   "appsWriteable", dirPtr->d_name, NULL) != LE_OK)
                {
                    LE_ERROR("Directory name '%s...' is too long.", destDir);
                    result = LE_FAULT;
                    break;
                }

                // Copy directories.
                if (file_CopyRecursive(sourceDir, destDir, NULL) != LE_OK)
                {
                    result = LE_FAULT;
                    break;
                }
            }
        }
    }

    // Now close the directory pointer
    if (closedir(appsWriteableDir) != 0)
    {
        LE_ERROR("Failed to close dir '%s'. %m", APPS_WRITEABLE_DIR);
        result = LE_FAULT;
    }

    if (result == LE_FAULT)
    {
        return LE_FAULT;
    }

    // Atomically rename the work dir to the proper index
    char newSystemPath[100] = "";
    snprintf(newSystemPath, sizeof(newSystemPath), "%s/%d", SystemPath, currentIndex);

    LE_DEBUG("Creating system snapshot '%s'", newSystemPath);

    file_Rename(system_UnpackPath, newSystemPath);

    // Ensure that snapshotted system retains the framework label. Otherwise a rollback will
    // not work since it does not have permission to access it.
    smack_SetLabel(newSystemPath, "framework");

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
    if (system_IsModified())
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
                  0644 /* rw-r--r-- */);
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

    sysStatus_SetUntried();

    // Update the version string.
    static const char modifiedStr[] = "_modified";
    char versionBuffer[200] = {0};
    char newVersionBuffer[sizeof(versionBuffer) + sizeof(modifiedStr) + 1] = {0}; // +1 for newline

    // Get the old version.
    size_t versionLen = system_Version(versionBuffer, sizeof(versionBuffer));

    if (versionLen > 0)
    {
        // Remove newline if there is one.
        if (versionBuffer[versionLen-1] == '\n')
        {
            versionBuffer[versionLen-1] = '\0';
        }
    }
    else  // versionLen <= 0
    {
        LE_ERROR("Can not retrieve system version. Setting version to 'Unknown'");
        snprintf(versionBuffer, sizeof(versionBuffer), "Unknown");
    }

    // Append "_modified".
    LE_ASSERT(snprintf(newVersionBuffer,
                       sizeof(newVersionBuffer),
                       "%s%s\n",
                       versionBuffer,
                       modifiedStr)
                       < sizeof(newVersionBuffer));

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
bool system_IsModified
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    return file_Exists(CurrentModifiedFilePath);
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
    sysStatus_Status_t status = sysStatus_Status();

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
                    if ((status == SYS_GOOD) || (sysStatus_GetStatus(foundSystemPtr) != SYS_GOOD))
                    {
                        // Attempt to umount the system because it may have been mounted when
                        // sandboxed apps were created.
                        fs_TryLazyUmount(entPtr->fts_path);

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


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the labels of the unpacked system.
 **/
//--------------------------------------------------------------------------------------------------
void system_InitSmackLabels
(
    void
)
{
    // Ensure that most of the system content is labeled with the framework label.
    // The updateDaemon needs admin priveledges to set smack labels for now which causes all
    // untar'ed content to have the admin label. This causes issues since most subjects cannot
    // access admin e.g. app exec'ing a process and accessing library etc...
    // TODO: Labels will be set at build time in the future.
    smack_SetLabel(system_UnpackPath, "framework");
    smack_SetLabel(UnpackConfigDirPath, "framework");
    smack_SetLabel(UnpackAppsDirPath, "framework");
    smack_SetLabel(UnpackLibDirPath, "framework");
    smack_SetLabel(UnpackBinDirPath, "framework");
    smack_SetLabel(UnpackModuleDirPath, "framework");
}
