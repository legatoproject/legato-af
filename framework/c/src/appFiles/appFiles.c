//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the appFiles component's functionality.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "appFiles.h"
#include "interfaces.h"
#include "limit.h"
#include "smack.h"
#include <ftw.h>
#include "sysPaths.h"


//--------------------------------------------------------------------------------------------------
/**
 * Get the path to the application's install directory.
 */
//--------------------------------------------------------------------------------------------------
static void GetAppInstallPath
(
    char* appPathBuffPtr,   ///< [out] Ptr to where the path will be put.
    size_t buffSize,        ///< Size of the buffer pointed to by appPathBuffPtr (bytes).
    const char* appNamePtr  ///< The app's name.
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(buffSize > 0);

    appPathBuffPtr[0] = '\0';

    LE_ASSERT_OK(le_path_Concat("/", appPathBuffPtr, buffSize, APPS_INSTALL_DIR, appNamePtr, NULL));
}


//--------------------------------------------------------------------------------------------------
/**
 * Recursively sets the permissions for all files and directories in an application's install
 * directory.  All files are set to read only with SMACK label 'AppLabel' and all directories are
 * set to read-execute with SMACK label 'AppLabelrx'.
 *
 * @note Kills the calling process on error.
 */
//--------------------------------------------------------------------------------------------------
static void SetInstalledFilesPermissions
(
    const char* appNamePtr
)
{
    // Get the smack labels to use.
    char dirLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
    appSmack_GetAccessLabel(appNamePtr,
                            APPSMACK_ACCESS_FLAG_READ | APPSMACK_ACCESS_FLAG_EXECUTE,
                            dirLabel,
                            sizeof(dirLabel));

    char fileLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
    appSmack_GetLabel(appNamePtr, fileLabel, sizeof(fileLabel));

    // Get the path to the application's install directory.
    char installPath[LIMIT_MAX_PATH_BYTES] = "";
    GetAppInstallPath(installPath, sizeof(installPath), appNamePtr);

    // Get the path to the application's installed bin and lib directories.
    char binPath[LIMIT_MAX_PATH_BYTES] = "";
    LE_ASSERT(le_path_Concat("/", binPath, sizeof(binPath), installPath, "bin/", NULL) == LE_OK);

    char libPath[LIMIT_MAX_PATH_BYTES] = "";
    LE_ASSERT(le_path_Concat("/", libPath, sizeof(libPath), installPath, "lib/", NULL) == LE_OK);

    // Open the directory tree to search.
    char* pathArrayPtr[] = {(char*)installPath, NULL};

    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL | FTS_NOSTAT, NULL);

    LE_FATAL_IF(ftsPtr == NULL, "Could not access dir '%s'.  %m.", installPath);

    // Step through the directory tree.
    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        switch (entPtr->fts_info)
        {
            case FTS_DP:
                // These are directories.

                // Set the owner root.
                LE_FATAL_IF(chown(entPtr->fts_accpath, 0, 0) == -1,
                            "Could not set ownership of file '%s'.  %m.", entPtr->fts_accpath);

                // Set the permissions.
                LE_FATAL_IF(chmod(entPtr->fts_accpath, S_IROTH | S_IXOTH) == -1,
                            "Could not set permissions for file '%s'.  %m.", entPtr->fts_accpath);

                // Set the SMACK label.
                smack_SetLabel(entPtr->fts_accpath, dirLabel);

                break;

            case FTS_F:
            case FTS_NSOK:
            {
                // These are files.

                // TODO: This is a work around for now because built files in /lib and /bin are not
                //       included in the bundles section and we don't know what the permissions
                //       should be.
                char dir[LIMIT_MAX_PATH_BYTES];
                LE_ASSERT(le_path_GetDir(entPtr->fts_path, "/", dir, sizeof(dir)) == LE_OK);

                if ( (strcmp(dir, binPath) != 0) && (strcmp(dir, libPath) != 0) )
                {
                    // Set the owner to root.
                    LE_FATAL_IF(chown(entPtr->fts_accpath, 0, 0) == -1,
                                "Could not set ownership of file '%s'.  %m.", entPtr->fts_accpath);

                    // Set the permissions.
                    LE_FATAL_IF(chmod(entPtr->fts_accpath, S_IROTH) == -1,
                                "Could not set permissions for file '%s'.  %m.", entPtr->fts_accpath);
                }

                // Set the SMACK label.
                smack_SetLabel(entPtr->fts_accpath, fileLabel);

                break;
            }
        }
    }

    LE_FATAL_IF(errno != 0, "Could not traverse directory '%s'.  %m", installPath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the configured permissions for a file or directory.
 **/
//--------------------------------------------------------------------------------------------------
static mode_t GetCfgPermissions
(
    le_cfg_IteratorRef_t cfgIter        ///< [IN] Config iterator pointing to the bundled file/dir.
)
{
    mode_t mode = 0;

    if (le_cfg_GetBool(cfgIter, "isReadable", false))
    {
        mode |= S_IROTH;
    }

    if (le_cfg_GetBool(cfgIter, "isWritable", false))
    {
        mode |= S_IWOTH;
    }

    if (le_cfg_GetBool(cfgIter, "isExecutable", false))
    {
        mode |= S_IXOTH;
    }

    return mode;
}


//--------------------------------------------------------------------------------------------------
/**
 * Convert mode to SMACK access flags.
 **/
//--------------------------------------------------------------------------------------------------
static appSmack_AccessFlags_t ConverToSmackAccessFlags
(
    mode_t mode             ///< [IN] Access mode.
)
{
    appSmack_AccessFlags_t flags = 0;

    if ((mode & S_IROTH) > 0)
    {
        flags |= APPSMACK_ACCESS_FLAG_READ;
    }

    if ((mode & S_IWOTH) > 0)
    {
        flags |= APPSMACK_ACCESS_FLAG_WRITE;
    }

    if ((mode & S_IXOTH) > 0)
    {
        flags |= APPSMACK_ACCESS_FLAG_EXECUTE;
    }

    return flags;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the permissions for the bundled directories.
 *
 * @note Kills the calling process on error.
 **/
//--------------------------------------------------------------------------------------------------
static void SetBundledDirPermissions
(
    const char* appNamePtr,             ///< [IN] Name of app being installed.
    le_cfg_IteratorRef_t cfgIter        ///< [IN] Iterator pointing to the bundles section.
)
{
    le_cfg_GoToNode(cfgIter, "dirs");

    if (le_cfg_GoToFirstChild(cfgIter) == LE_OK)
    {
        do
        {
            // Get the source path from the config.
            char src[LIMIT_MAX_PATH_BYTES];
            LE_ASSERT(le_cfg_GetString(cfgIter, "src", src, sizeof(src), "") == LE_OK);

            // Get the full path to the src.
            char fullPath[LIMIT_MAX_PATH_BYTES] = APPS_INSTALL_DIR;
            LE_ASSERT(le_path_Concat("/", fullPath, sizeof(fullPath), appNamePtr, src, NULL) == LE_OK);

            // Check that the source exists.
            struct stat statBuf;

            LE_FATAL_IF(stat(fullPath, &statBuf) == -1, "Could not stat file '%s'.  %m.", fullPath);

            // Check that the source is the right type.
            LE_FATAL_IF(!S_ISDIR(statBuf.st_mode),
                        "Expected '%s' to be a directory but it was not.", fullPath);

            mode_t mode = GetCfgPermissions(cfgIter);

            // Ensure that write permission is not allowed for directories.  This is enforced
            // because we do not yet have a way to set disk quotas.
            LE_FATAL_IF((mode & S_IWOTH) > 0,
                        "Write access cannot be granted to bundled directory '%s'.", fullPath);

            // Set DAC permissions.
            LE_FATAL_IF(chmod(fullPath, mode) == -1,
                        "Could not set permissions for file '%s'.  %m.", fullPath);

            // Set the SMACK label.
            char smackLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
            appSmack_GetAccessLabel(appNamePtr,
                                    ConverToSmackAccessFlags(mode),
                                    smackLabel,
                                    sizeof(smackLabel));

            smack_SetLabel(fullPath, smackLabel);
        }
        while (le_cfg_GoToNextSibling(cfgIter) == LE_OK);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the permissions for the bundled files.
 *
 * @note Kills the calling process on error.
 **/
//--------------------------------------------------------------------------------------------------
static void SetBundledFilePermissions
(
    const char* appNamePtr,             ///< [IN] Name of app being installed.
    le_cfg_IteratorRef_t cfgIter        ///< [IN] Iterator pointing to the bundles section.
)
{
    le_cfg_GoToNode(cfgIter, "files");

    if (le_cfg_GoToFirstChild(cfgIter) == LE_OK)
    {
        do
        {
            // Get the source path from the config.
            char src[LIMIT_MAX_PATH_BYTES];
            LE_ASSERT(le_cfg_GetString(cfgIter, "src", src, sizeof(src), "") == LE_OK);

            // Get the full path to the src.
            char fullPath[LIMIT_MAX_PATH_BYTES] = APPS_INSTALL_DIR;
            LE_ASSERT(le_path_Concat("/", fullPath, sizeof(fullPath), appNamePtr, src, NULL) == LE_OK);

            // Check that the source exists.
            struct stat statBuf;

            LE_FATAL_IF(stat(fullPath, &statBuf) == -1, "Could not stat file '%s'.  %m.", fullPath);

            // Check that the source is the right type.
            LE_FATAL_IF(S_ISDIR(statBuf.st_mode),
                        "Expected '%s' to be a file but it was not.", fullPath);

            mode_t mode = GetCfgPermissions(cfgIter);

            // Set DAC permissions.
            LE_FATAL_IF(chmod(fullPath, mode) == -1,
                        "Could not set permissions for file '%s'.  %m.", fullPath);
        }
        while (le_cfg_GoToNextSibling(cfgIter) == LE_OK);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets permissions and SMACK labels on all files and directories in the application's bundled
 * files/dirs.  This is done by:
 *
 *      1)  Go through all files and directories in the application's install directory and set the
 *          permisssions and SMACK labels.  All files are set to read-only with SMACK label
 *          'AppLabel' and all directories are set to read-execute with SMACK label 'AppLabelrx'.
 *          Set the owner and group to root.
 *
 *          This is done to cover all files and directories that may not be in the config's bundles
 *          section.
 *
 *      2)  Set permissions for all directories in the config's 'bundles' section to the
 *          configured permissions.  Set the SMACK label according to the configured permissions:
 *          'AppLabel-', 'AppLabelr', 'AppLabelx', 'AppLabelrx', etc.
 *
 *          Disallow setting 'write' permission to directories because we currently do not support
 *          disk quotas.
 *
 *      3)  Set permissions for all files in the config's 'bundles' section to the configured
 *          permissions.  SMACK labels are not set and retain the 'AppLabel' label set in step 1.
 *
 *          The SMACK label for all files are set to match the app's label to support passing of
 *          file descriptors from one application to another.
 *
 * All other files to be imported into the sandbox exist in the system already and already has
 * permissions and SMACK labels set properly.
 *
 * The Supervisor will setup SMACK rules so 'AppLabel' has the proper access to 'AppLabelr',
 * 'AppLabelx', etc.
 *
 * This program must be run as root with the 'admin' SMACK label to be able to set the permissions
 * and SMACK labels as required.
 *
 **/
//--------------------------------------------------------------------------------------------------
void appFiles_SetPermissions
(
    const char* appNamePtr
)
{
    // Set permissions for everything in the app's install directory.
    SetInstalledFilesPermissions(appNamePtr);

    // Create the path to the application's bundles section in the config.
    char bundlesPath[LIMIT_MAX_PATH_BYTES] = "/apps";
    LE_ASSERT(le_path_Concat("/", bundlesPath, sizeof(bundlesPath), appNamePtr, "bundles", NULL) == LE_OK);

    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn(bundlesPath);

    // Set permissions for all directories in bundles.
    SetBundledDirPermissions(appNamePtr, cfgIter);

    // Set permissions for all files in bundles.
    le_cfg_GoToNode(cfgIter, bundlesPath);
    SetBundledFilePermissions(appNamePtr, cfgIter);

    le_cfg_CancelTxn(cfgIter);
}



//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called once for each object in the directory tree being deleted.
 *
 * @return 0 = continue, non-zero = stop.
 */
//--------------------------------------------------------------------------------------------------
static int DeleteObject
(
    const char *path,       ///< Path to the file system object to be deleted.
    const struct stat *statBuffPtr,  ///< Pointer to structure filled in by stat().
    int type,               ///< File type: FTW_D = dir, FTW_F = other, FTW_DNR or FTW_NS = error.
    struct FTW *unused      ///< Unused pos of object relative to root of dir tree being deleted.
)
{
    switch (type)
    {
        case FTW_D:  // Directory.
        case FTW_DP: // Directory for which all subdirectories have been visited.

            if (rmdir(path) != 0)
            {
                LE_CRIT("Failed to delete directory '%s' (%m)", path);
            }
            break;

        case FTW_F:   // Non-directory (file, symlink, named socket, named pipe, etc.)
        case FTW_SL:  // Symlink.
        case FTW_SLN: // Symlink naming a non-existent file.

            if (unlink(path) != 0)
            {
                LE_CRIT("Failed to unlink file system object '%s' (%m)", path);
            }
            break;

        case FTW_DNR:   // Unreadable directory.

            LE_CRIT("Can't read directory '%s'", path);
            break;

        case FTW_NS:    // Not a symlink, but couldn't "stat" it.

            LE_CRIT("Can't stat object '%s'", path);
            break;

        default:    // Should never get anything else.

            LE_FATAL("Unexpected type %d for path '%s'", type, path);
            break;
    }

    return 0;   // Always try to keep going and delete as much as possible.
}


//--------------------------------------------------------------------------------------------------
/**
 * Removes an application's files from the file system.
 *
 * @return LE_OK on success.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t appFiles_Remove
(
    const char* appNamePtr
)
{
    // Get the file system path to the directory that the app's files are installed in.
    char installPath[LIMIT_MAX_PATH_BYTES];
    GetAppInstallPath(installPath, sizeof(installPath), appNamePtr);

    // Walk the directory tree, deleting objects in it.

    #define MAX_FDS_TO_HAVE_OPEN 30

    int flags = FTW_DEPTH // Depth-first: Call DeleteObject for dir contents before calling for dir
              | FTW_MOUNT // Stay in the same file system (don't follow mounts)
              | FTW_PHYS; // Don't follow symlinks

    int result = nftw(installPath, DeleteObject, MAX_FDS_TO_HAVE_OPEN, flags);

    if (result != 0)
    {
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}


COMPONENT_INIT
{
}
