//--------------------------------------------------------------------------------------------------
/** @file dir.c
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "limit.h"
#include <fts.h>


//--------------------------------------------------------------------------------------------------
/**
 * Creates a directory with permissions specified in mode.
 *
 * @note The actual permissions for the created directory will depend on the calling process' umask.
 *
 * @return
 *      LE_OK if successful.
 *      LE_DUPLICATE if the directory already exists.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dir_Make
(
    const char* pathNamePtr,    ///< [IN] The path name to the directory to create.
    mode_t mode                 ///< [IN] The permissions for the directory.
)
{
    if (mkdir(pathNamePtr, mode) == -1)
    {
        if (errno == EEXIST)
        {
            return LE_DUPLICATE;
        }
        else
        {
            LE_ERROR("Could not create directory '%s'.  %m", pathNamePtr);
            return LE_FAULT;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates all directories in the path.  If some (or all) directories in the path already exists
 * those directories are left as they are.  All created directories have the same permissions
 * (specified in mode).
 *
 * @note The actual permissions for the created directories will depend on the calling process'
 *       umask.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dir_MakePath
(
    const char* pathNamePtr,    ///< [IN] A path containing all the directories to create.
    mode_t mode                 ///< [IN] The permissions for all created directories.
)
{
    // Make a copy of the path string.
    size_t len;
    char dirStr[LIMIT_MAX_PATH_BYTES];

    if (le_utf8_Copy(dirStr, pathNamePtr, LIMIT_MAX_PATH_BYTES, &len) != LE_OK)
    {
        return LE_FAULT;
    }

    // Ignore final separator
    if (dirStr[len-1] == '/')
    {
        dirStr[len-1] = '\0';
    }

    // Start from the second char in case it is a separator.
    size_t i;
    for (i = 1; dirStr[i] != '\0'; i++)
    {
        if (dirStr[i] == '/')
        {
            // Temporarily terminate the string here.
            dirStr[i] = '\0';

            // Make the directory.  If the directory already exists just move on to the next dir.
            if (le_dir_Make(dirStr, mode) == LE_FAULT)
            {
                return LE_FAULT;
            }

            // Re-insert the separator.
            dirStr[i] = '/';
        }
    }

    // Make the last directory.
    if (le_dir_Make(dirStr, mode) == LE_FAULT)
    {
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Removes a directory by first recursively removing sub-directories, files, symlinks, hardlinks,
 * devices, etc.  Symlinks are not followed, only the links themselves are deleted.
 *
 * A file or device may not be able to be removed if it is busy, in which case an error message
 * is logged and LE_FAULT is returned.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dir_RemoveRecursive
(
    const char* pathNamePtr           ///< [IN] The path to the directory to remove.
)
{
    // Attempt first to just delete the directory.
    if ( (rmdir(pathNamePtr) == 0) || (errno == ENOENT) )
    {
        return LE_OK;
    }

    // Open the directory tree to search.
    char* pathArrayPtr[] = {(char*)pathNamePtr, NULL};

    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL | FTS_NOSTAT, NULL);

    if (ftsPtr == NULL)
    {
        return LE_FAULT;
    }

    // Step through the directory tree.
    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        switch (entPtr->fts_info)
        {
            case FTS_DP:
            case FTS_DNR:
                // These are directories.
                if (rmdir(entPtr->fts_accpath) != 0)
                {
                    LE_ERROR("Could not remove directory '%s'.  %m", entPtr->fts_accpath);
                    return LE_FAULT;
                }
                break;

            case FTS_F:
            case FTS_NSOK:
                // These are files.
                if (remove(entPtr->fts_accpath) != 0)
                {
                    LE_ERROR("Could not remove file '%s'.  %m", entPtr->fts_accpath);
                    return LE_FAULT;
                }
        }
    }

    if (errno != 0)
    {
        LE_ERROR("Could not find directory '%s'.  %m", pathNamePtr);
        return LE_FAULT;
    }

    return LE_OK;
}
