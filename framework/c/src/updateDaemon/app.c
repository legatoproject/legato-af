//--------------------------------------------------------------------------------------------------
/**
 * @file app.c
 *
 * Functions used by updateDaemon to install and remove apps.
 *
 * Structure:
 *
 * legato/
 *   apps/
 *     unpack/
 *     <hash>/
 *       read-only/
 *       info.properties
 *       root.cfg
 *   systems/
 *     current/
 *       appsWriteable/
 *         <appName>/
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include <sys/types.h>
#include <sys/stat.h>
#include "legato.h"
#include "interfaces.h"
#include "limit.h"
#include "appUser.h"
#include "file.h"
#include "dir.h"
#include "app.h"
#include "sysStatus.h"
#include "system.h"
#include "supCtrl.h"
#include "instStat.h"
#include "installer.h"
#include "smack.h"
#include "sysPaths.h"
#include "fileSystem.h"


static const char* InstallHookScriptPath = "/legato/systems/current/bin/install-hook";


const char* app_UnpackPath = "/legato/apps/unpack";


static const char* PreInstallPath = "/legato/apps/%s/read-only/script/pre-install";


static const char* PostInstallPath = "/legato/apps/%s/read-only/script/post-install";


//--------------------------------------------------------------------------------------------------
/**
 * Import an applications configuration into the system config tree, allowing the supervisor to be
 * able to launch this application.
 */
//--------------------------------------------------------------------------------------------------
static void ImportConfig
(
    const char* appMd5Ptr,  ///< [IN] The MD5 hash ID for this application.
    const char* appNamePtr  ///< [IN] The user visible name for this application.
)
//--------------------------------------------------------------------------------------------------
{
    char configPath[100] = "";
    snprintf(configPath, sizeof(configPath), "/legato/apps/%s/root.cfg", appMd5Ptr);

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn("/apps");
    le_cfg_DeleteNode(iterRef, appNamePtr);
    le_cfg_CommitTxn(iterRef);

    iterRef = le_cfg_CreateWriteTxn("/apps");
    le_result_t result = le_cfgAdmin_ImportTree(iterRef, configPath, appNamePtr);

    if (result != LE_OK)
    {
        LE_EMERG("Failed to import application, '%s', configuration, %s.",
                 appNamePtr,
                 LE_RESULT_TXT(result));

        le_cfg_CancelTxn(iterRef);
    }
    else
    {
        le_cfg_CommitTxn(iterRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Run the specified install script through the system's hook script.
 *
 * @return LE_OK if the script ran successfully, LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ExecInstallHook
(
    const char* userScriptPath  ///< [IN] Which install hook to execute?
)
//--------------------------------------------------------------------------------------------------
{
    if (file_Exists(userScriptPath) == false)
    {
        return LE_OK;
    }

    char commandLineBuffer[400] = "";
    snprintf(commandLineBuffer,
             sizeof(commandLineBuffer),
             "%s %s",
             InstallHookScriptPath,
             userScriptPath);

    LE_DEBUG("*** Executing application install hook. ***");
    LE_DEBUG("*** %s", userScriptPath);

    int result = system(commandLineBuffer);
    LE_FATAL_IF(result == -1, "Could not exec install hook.");

    // Check to see if the script exited, and what the return code was.
    if (WIFEXITED(result))
    {
        int exitCode = WEXITSTATUS(result);

        if (exitCode != 0)
        {
            LE_CRIT("Install hook, '%s', failed to execute, return code: '%d'.",
                    le_path_GetBasenamePtr(userScriptPath, "/"),
                    exitCode);

            return LE_FAULT;
        }
    }
    else if (WIFSIGNALED(result))
    {
        LE_CRIT("Install hook, '%s', failed to execute, terminated by signal: '%d'.",
                le_path_GetBasenamePtr(userScriptPath, "/"),
                WTERMSIG(result));

        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Execute the application's preinstall hook, but only if one is supplied.  But even then, there's
 * no guarantee that anything will run as the user has to modify their system file script
 * 'install-hook' so that it will run the executable file passed in.
 */
//--------------------------------------------------------------------------------------------------
static void ExecPreinstallHook
(
    const char* appMd5Ptr,
    const char* appNamePtr
)
//--------------------------------------------------------------------------------------------------
{
    // Compute the proper path to the app pre-install script, then run it.
    char scriptPathBuffer[200] = "";
    int count = snprintf(scriptPathBuffer, sizeof(scriptPathBuffer), PreInstallPath, appMd5Ptr);

    LE_ASSERT(count < sizeof(scriptPathBuffer));

    if (ExecInstallHook(scriptPathBuffer) != LE_OK)
    {
        LE_FATAL("Pre-install program failed for app '%s' <%s>.", appNamePtr, appMd5Ptr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Execute the application's postinstall hook.  Like the preinstall hook, it is the install-hook
 * script that handles the actual execution.
 */
//--------------------------------------------------------------------------------------------------
static void ExecPostinstallHook
(
    const char* appMd5Ptr
)
//--------------------------------------------------------------------------------------------------
{
    // Compute the proper path to the app post-install script, then run it.
    char scriptPathBuffer[200] = "";
    int count = snprintf(scriptPathBuffer, sizeof(scriptPathBuffer), PostInstallPath, appMd5Ptr);

    LE_ASSERT(count < sizeof(scriptPathBuffer));

    if (ExecInstallHook(scriptPathBuffer) != LE_OK)
    {
        LE_FATAL("Postinstall hook for the application '%s' failed.", appMd5Ptr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Recursively sets the permissions for all files and directories in application read-only directory.
 *
 * returns LE_OK if successful, LE_FAULT if fails.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetSmackPermReadOnlyDir
(
    const char* appMd5Ptr,  ///< [IN] Hash ID of the application to install.
    const char* appNamePtr  ///< [IN] Name of the application to install.
)
{
    char fileLabel[LIMIT_MAX_SMACK_LABEL_BYTES];

    // Get file label
    smack_GetAppLabel(appNamePtr, fileLabel, sizeof(fileLabel));

    char readOnlyPath[LIMIT_MAX_PATH_BYTES] = "";
    int n = snprintf(readOnlyPath,
                 sizeof(readOnlyPath),
                 "/legato/apps/%s/read-only",
                 appMd5Ptr);
    LE_ASSERT(n < sizeof(readOnlyPath));

    // Open the directory tree to search.
    char* pathArrayPtr[] = {(char *)readOnlyPath, NULL};

    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_LOGICAL, NULL);

    LE_FATAL_IF(ftsPtr == NULL, "Could not access dir '%s'.  %m.",
                pathArrayPtr[0]);

    le_result_t result = LE_OK;

    // Step through the directory tree.
    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        switch (entPtr->fts_info)
        {
            case FTS_D:
                {
                    //Get dir label
                    char dirLabel[LIMIT_MAX_SMACK_LABEL_BYTES] = "";
                    char dirPerm[10] = "";
                    int index = 0;
                    mode_t dirMode = entPtr->fts_statp->st_mode;

                    // No need to check write permission, check only read and execute permission.
                    if (dirMode & S_IROTH)
                    {
                         dirPerm[index++] = 'r';
                    }
                    if (dirMode & S_IXOTH)
                    {
                        dirPerm[index++] = 'x';
                    }

                    n = snprintf(dirLabel, sizeof(dirLabel), "%s%s", fileLabel, dirPerm);
                    LE_ASSERT(n < sizeof(dirLabel));
                    // These are directories, visited in pre-order. Set the SMACK label.
                    LE_DEBUG("Setting smack label: '%s' for directory: '%s'", dirLabel,
                             entPtr->fts_accpath);
                    result = smack_SetLabel(entPtr->fts_accpath, dirLabel);
                }
                break;

            case FTS_DP:
                // Same directory traversed in post-order. So ignore it.
                break;

            case FTS_F:
                // These are files. Set the SMACK label.
                LE_DEBUG("Setting smack label: '%s' for file: '%s'", fileLabel,
                           entPtr->fts_accpath);
                result = smack_SetLabel(entPtr->fts_accpath, fileLabel);
                break;

            case FTS_NS:
            case FTS_ERR:
            case FTS_DNR:
            case FTS_NSOK:
                LE_CRIT("Unexpected file type %d in app '%s' <%s>. %s",
                        entPtr->fts_info,
                        appNamePtr,
                        appMd5Ptr,
                        strerror(entPtr->fts_errno));
                LE_CRIT("Offending path: '%s'.", entPtr->fts_path);
                result = LE_FAULT;
                break;
        }

        if (result != LE_OK)
        {
            break;
        }
    }

    fts_close(ftsPtr);
    return (result == LE_OK) ? LE_OK:LE_FAULT;
}



//--------------------------------------------------------------------------------------------------
/**
 * Recursively sets the smack permissions for directories under apps writeable directory.
 *
 * returns LE_OK if successful, LE_FAULT if fails.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetPermAppWritableDir
(
    const char* appWritableDir,  ///< [IN] Path to apps writable directory.
    const char* appLabel        ///< [IN] App smack label.
)
{

    // Open the directory tree to search.
    char* pathArrayPtr[] = {(char *)appWritableDir, NULL};

    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_LOGICAL, NULL);

    LE_FATAL_IF(ftsPtr == NULL, "Could not access dir '%s'.  %m.", pathArrayPtr[0]);

    le_result_t result = LE_OK;

    // Step through the directory tree.
    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        switch (entPtr->fts_info)
        {
            case FTS_D:
                {
                    //Get dir label
                    char dirLabel[LIMIT_MAX_SMACK_LABEL_BYTES] = "";
                    char dirPerm[10] = "";
                    int index = 0;
                    mode_t dirMode = entPtr->fts_statp->st_mode;

                    if (dirMode & S_IROTH)
                    {
                         dirPerm[index++] = 'r';
                    }
                    if (dirMode & S_IWOTH)
                    {
                         dirPerm[index++] = 'w';
                    }
                    if (dirMode & S_IXOTH)
                    {
                        dirPerm[index++] = 'x';
                    }

                    LE_ASSERT(snprintf(dirLabel, sizeof(dirLabel), "%s%s", appLabel, dirPerm)
                              < sizeof(dirLabel));
                    // These are directories, visited in pre-order. Set the SMACK label.
                    LE_DEBUG("Setting smack label: '%s' for directory: '%s'", dirLabel,
                             entPtr->fts_accpath);
                    result = smack_SetLabel(entPtr->fts_accpath, dirLabel);
                }
                break;

        }

        if (result != LE_OK)
        {
            break;
        }
    }

    fts_close(ftsPtr);
    return (result == LE_OK) ? LE_OK:LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Setup smack permission for contents in app's read-only directory.
 *
 * @return LE_OK if successful, LE_FAULT if fails.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_SetSmackPermReadOnly
(
    const char* appMd5Ptr,  ///< [IN] Hash ID of the application to install.
    const char* appNamePtr  ///< [IN] Name of the application to install.
)
{
    return SetSmackPermReadOnlyDir(appMd5Ptr, appNamePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Check to see if the given application exists.
 */
//--------------------------------------------------------------------------------------------------
bool app_Exists
(
    const char* md5StrPtr  ///< [IN] The hash of the app to find.
)
//--------------------------------------------------------------------------------------------------
{
    char pathBuffer[100] = "";

    LE_FATAL_IF(snprintf(pathBuffer, sizeof(pathBuffer), "/legato/apps/%s", md5StrPtr)
                >= sizeof(pathBuffer),
                "MD5 sum string way too long");

    return le_dir_IsDir(pathBuffer);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the hash ID for the named application.
 *
 * On return the application's hash is copied into the supplied hashBuffer.
 */
//--------------------------------------------------------------------------------------------------
void app_Hash
(
    const char* appNamePtr,               ///< [IN] The name of the application to read.
    char hashBuffer[LIMIT_MD5_STR_BYTES]  ///< [OUT] Buffer to hold the application hash ID.
)
//--------------------------------------------------------------------------------------------------
{
    char appLinkPath[100];

    LE_ASSERT(snprintf(appLinkPath, sizeof(appLinkPath), "%s/%s", system_CurrentAppsDir, appNamePtr)
              < sizeof(appLinkPath));

    installer_GetAppHashFromSymlink(appLinkPath, hashBuffer);
}


//--------------------------------------------------------------------------------------------------
/**
 * Prepare the app unpack directory for use (delete any old one and create a fresh empty one).
 */
//--------------------------------------------------------------------------------------------------
void app_PrepUnpackDir
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Clear out the current unpack dir, if it exists, then make sure it exists.
    LE_FATAL_IF(le_dir_RemoveRecursive(app_UnpackPath) != LE_OK,
                "Failed to recursively delete '%s'.",
                app_UnpackPath);
    LE_FATAL_IF(LE_OK != le_dir_MakePath(app_UnpackPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH),
                "Failed to create directory '%s'.",
                app_UnpackPath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set up a given app's writeable files in the "unpack" system.
 *
 * Files will be copied to the system unpack area based on whether an app with the same name
 * exists in the current system.
 *
 * @warning Assumes the app identified by the hash is installed in /legato/apps/<hash>.
 *
 * @return LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_SetUpAppWriteables
(
    const char* appMd5Ptr,  ///< [IN] Hash ID of the application to install.
    const char* appNamePtr  ///< [IN] Name of the application to install.
)
//--------------------------------------------------------------------------------------------------
{
    // If an app with the same name is installed in the current system,
    if (system_HasApp(appNamePtr))
    {
        // Copy the app's config tree file.
        if (system_CopyAppConfig(appNamePtr) != LE_OK)
        {
            return LE_FAULT;
        }
    }

    // Install appropriate writable app files.
    return installer_InstallAppWriteableFiles(appMd5Ptr, appNamePtr, "current");
}


//--------------------------------------------------------------------------------------------------
/**
 * Install a new individual application update in the current running system.
 *
 * @return LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_InstallIndividual
(
    const char* appMd5Ptr,  ///< [IN] Hash ID of the application to install.
    const char* appNamePtr  ///< [IN] Name of the application to install.
)
//--------------------------------------------------------------------------------------------------
{
    if (system_Snapshot() != LE_OK)
    {
        return LE_FAULT;
    }

    system_MarkModified();

    // If the app is just in the unpack dir, and not yet moved to /legato/apps/<hash>,
    // move it there now.
    if (app_Exists(appMd5Ptr) == false)
    {
        char path[PATH_MAX];

        LE_ASSERT(snprintf(path, sizeof(path), "/legato/apps/%s", appMd5Ptr) < sizeof(path));

        // In case there is a dangling symlink there, unlink it.
        // Ignore failure, because most of the time there won't be anything there.
        (void)unlink(path);

        if (rename(app_UnpackPath, path) != 0)
        {
            LE_EMERG("Failed to rename '%s' to '%s', %m.", app_UnpackPath, path);
            sysStatus_MarkBad();
            LE_FATAL("Rolling-back to snapshot.");
        }
    }

    le_result_t result = LE_OK;

    // If this app is already in the current system,
    if (system_HasApp(appNamePtr))
    {
        // If it has the same hash, we don't have to do anything,
        char currentAppHash[LIMIT_MD5_STR_BYTES];
        app_Hash(appNamePtr, currentAppHash);
        if (strcmp(appMd5Ptr, currentAppHash) == 0)
        {
            result = LE_DUPLICATE;
        }
        else
        {
            sysStatus_MarkBad();   // Mark "bad" for now because it will be in a bad state for a while.

            // Otherwise, stop it before we update it.
            supCtrl_StopApp(appNamePtr);

            // Attempt to umount appsWritable/<appName> because it may have been mounted as a sandbox.
            char path[PATH_MAX] = "";
            LE_ASSERT(le_path_Concat("/", path, sizeof(path),
                                     APPS_WRITEABLE_DIR, appNamePtr, NULL) == LE_OK);

            fs_TryLazyUmount(path);

            // Run the pre-install hook.
            ExecPreinstallHook(appMd5Ptr, appNamePtr);

            // Set smackfs file permission for installed files
            SetSmackPermReadOnlyDir(appMd5Ptr, appNamePtr);

            // Update non-writeable files dir symlink to point to the new version of the app
            system_SymlinkApp("current", appMd5Ptr, appNamePtr);

            // Load the root.cfg from the new version of the app into the system config tree.
            ImportConfig(appMd5Ptr, appNamePtr);

            // Update the writeable files.
            system_UpdateCurrentAppWriteableFiles(appMd5Ptr, appNamePtr);
        }
    }
    // If the app is not in the current system yet, install fresh.
    else
    {
        sysStatus_MarkBad();   // Mark "bad" for now because it will be in a bad state for a while.

        // Run the pre-install hook.
        ExecPreinstallHook(appMd5Ptr, appNamePtr);

        // Set smackfs file permission for installed files
        SetSmackPermReadOnlyDir(appMd5Ptr, appNamePtr);

        // Create a non-writeable files dir symlink pointing to the app's installed files.
        system_SymlinkApp("current", appMd5Ptr, appNamePtr);

        // Compute the path to the app's install directory's writeable files directory.
        char srcDir[PATH_MAX];
        int n = snprintf(srcDir,
                         sizeof(srcDir),
                         "/legato/apps/%s/writeable/.",
                         appMd5Ptr);
        LE_ASSERT(n < sizeof(srcDir));

        // Create a user for this new app.
        appUser_Add(appNamePtr);

        // Import the applications config.
        ImportConfig(appMd5Ptr, appNamePtr);

        // Install the writeable files if there are any.
        if (le_dir_IsDir(srcDir))
        {
            char destDir[PATH_MAX];
            system_GetAppWriteableFilesDirPath(destDir, sizeof(destDir), "current", appNamePtr);

            char appLabel[LIMIT_MAX_SMACK_LABEL_BYTES] = "";
            smack_GetAppLabel(appNamePtr, appLabel, sizeof(appLabel));

            char dirLabel[LIMIT_MAX_SMACK_LABEL_BYTES] = "";
            LE_ASSERT(snprintf(dirLabel, sizeof(dirLabel), "%srwx", appLabel) < sizeof(dirLabel));

            if (dir_MakePathSmack(destDir, (S_IRWXU | S_IRWXG | S_IRWXO), dirLabel) != LE_OK)
            {
                LE_ERROR("Couldn't create dir %s", destDir);
                return LE_FAULT;
            }

            // Directory created, now copy files recursively.
            if (file_CopyRecursive(srcDir, destDir, appLabel) != LE_OK)
            {
                LE_ERROR("Failed to copy files recursively from '%s' to '%s'", srcDir, destDir);
                return LE_FAULT;
            }

            // While copying file, directories smack permission was not properly. Set it now.
            if (SetPermAppWritableDir(destDir, appLabel) != LE_OK)
            {
                LE_ERROR("Failed to set smack permission in directory '%s'", destDir);
                return LE_FAULT;
            }
        }
    }

    if (result == LE_OK)
    {
        // Reload the bindings configuration
        system("/legato/systems/current/bin/sdir load");

        ExecPostinstallHook(appMd5Ptr);

        sysStatus_MarkTried();

        instStat_ReportAppInstall(appNamePtr);

        supCtrl_StartApp(appNamePtr);

        LE_INFO("App %s <%s> installed", appNamePtr, appMd5Ptr);
    }
    else
    {
        LE_ASSERT(result == LE_DUPLICATE);

        LE_INFO("App %s <%s> was already installed", appNamePtr, appMd5Ptr);
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove the named app from the current running system.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_NOT_FOUND if requested to remove non-existent app.
 *      - LE_FAULT for any other failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_RemoveIndividual
(
    const char* appNamePtr  ///< [IN] Name of the app to remove.
)
//--------------------------------------------------------------------------------------------------
{
    le_cfg_IteratorRef_t i = le_cfg_CreateWriteTxn("system:/apps");

    if (!system_HasApp(appNamePtr) && !le_cfg_NodeExists(i, appNamePtr))
    {
        le_cfg_CancelTxn(i);

        LE_INFO("Ignoring request to remove non-existent app '%s'.", appNamePtr);

        return LE_NOT_FOUND;
    }

    if (system_Snapshot() != LE_OK)
    {
        return LE_FAULT;
    }

    system_MarkModified();

    sysStatus_MarkBad();

    // Get the hash for this application.
    char appHash[LIMIT_MD5_STR_BYTES] = "";
    app_Hash(appNamePtr, appHash);

    // Make sure that the application isn't running when we attempt to uninstall it.
    supCtrl_StopApp(appNamePtr);

    // Delete the /apps/<name> branch from the system's config tree.
    le_cfg_DeleteNode(i, appNamePtr);
    le_cfg_CommitTxn(i);

    // Remove the app specific tree, (if it exists.)
    le_cfgAdmin_DeleteTree(appNamePtr);

    // Delete the app's files from the current running system.
    system_RemoveApp(appNamePtr);

    // Delete the user account for this app.
    appUser_Remove(appNamePtr);

    // Now, check to see if any systems have this application installed.
    if (system_AppUsedInAnySystem(appHash) == false)
    {
        // They do not, so uninstall the application now.
        char appPath[PATH_MAX];
        LE_ASSERT(snprintf(appPath, sizeof(appPath), "/legato/apps/%s", appHash) < sizeof(appPath));
        if (le_dir_RemoveRecursive(appPath) != LE_OK)
        {
            LE_ERROR("Was unable to remove old application path, '%s'.", appPath);
        }
    }

    // Reload the bindings configuration
    system("/legato/systems/current/bin/sdir load");

    sysStatus_MarkTried();

    instStat_ReportAppUninstall(appNamePtr);

    LE_INFO("App %s removed.", appNamePtr);

    return LE_OK;
}
