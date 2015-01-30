//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the filePermissions component's functionality.
 *
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
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "filePermissions.h"
#include "interfaces.h"
#include "limit.h"
#include "smack.h"


//--------------------------------------------------------------------------------------------------
/**
 * The location where all applications are installed.
 */
//--------------------------------------------------------------------------------------------------
#define APPS_INSTALL_DIR                                "/opt/legato/apps"


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
    smack_GetAppAccessLabel(appNamePtr, S_IROTH | S_IXOTH, dirLabel, sizeof(dirLabel));

    char fileLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
    smack_GetAppLabel(appNamePtr, fileLabel, sizeof(fileLabel));

    // Get the path to the application's install directory.
    char installPath[LIMIT_MAX_PATH_BYTES] = APPS_INSTALL_DIR;
    LE_ASSERT(le_path_Concat("/", installPath, sizeof(installPath), appNamePtr, NULL) == LE_OK);

    // Get the path to the application's installed bin and lib directories.
    char binPath[LIMIT_MAX_PATH_BYTES] = APPS_INSTALL_DIR;
    LE_ASSERT(le_path_Concat("/", binPath, sizeof(binPath), appNamePtr, "bin/", NULL) == LE_OK);

    char libPath[LIMIT_MAX_PATH_BYTES] = APPS_INSTALL_DIR;
    LE_ASSERT(le_path_Concat("/", libPath, sizeof(libPath), appNamePtr, "lib/", NULL) == LE_OK);

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
            smack_GetAppAccessLabel(appNamePtr, mode, smackLabel, sizeof(smackLabel));

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
 * Sets file permissions and SMACK labels for an application's files according to the settings in
 * the configuration tree.
 **/
//--------------------------------------------------------------------------------------------------
void filePermissions_Set
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


COMPONENT_INIT
{
}
