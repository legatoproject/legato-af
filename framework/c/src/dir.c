//--------------------------------------------------------------------------------------------------
/** @file dir.c
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "limit.h"
#include "smack.h"


//--------------------------------------------------------------------------------------------------
/**
 * Creates a directory with the specified permissions and SMACK label.
 *
 * @note Permissions for the created directory will depend on the calling process' umask.
 *
 * @return
 *      LE_OK if successful.
 *      LE_DUPLICATE if the directory already exists.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t dir_MakeSmack
(
    const char* pathNamePtr,    ///< [IN] Path name to the directory to create.
    mode_t mode,                ///< [IN] Permissions for the directory.
    const char* labelPtr        ///< [IN] SMACK label for the directory.  If NULL, no label is given.
)
{
    LE_ASSERT(pathNamePtr != NULL);
    LE_ASSERT(pathNamePtr[0] != '\0');

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

    if (labelPtr != NULL)
    {
        return smack_SetLabel(pathNamePtr, labelPtr);
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates all directories in the path.  If some (or all) directories in the path already exists
 * those directories are left as they are.  All created directories are given the specified
 * permissions and SMACK label.
 *
 * @note The actual permissions for the created directories will depend on the calling process'
 *       umask.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t dir_MakePathSmack
(
    const char* pathNamePtr,    ///< [IN] A path containing all the directories to create.
    mode_t mode,                ///< [IN] The permissions for all created directories.
    const char* labelPtr        ///< [IN] SMACK label for all created directories.  If NULL, no
                                ///       label is given.
)
{
    // Make a copy of the path string.
    size_t len;
    char dirStr[LIMIT_MAX_PATH_BYTES];

    if (le_utf8_Copy(dirStr, pathNamePtr, LIMIT_MAX_PATH_BYTES, &len) != LE_OK)
    {
        LE_DEBUG("Directory path overflowed. Path size exceeds %s.",
                  STRINGIZE(LIMIT_MAX_PATH_BYTES));
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
            if (dir_MakeSmack(dirStr, mode, labelPtr) == LE_FAULT)
            {
                LE_DEBUG("Make directory %s failed.", dirStr);
                return LE_FAULT;
            }

            // Re-insert the separator.
            dirStr[i] = '/';
        }
    }

    // Make the last directory.
    if (dir_MakeSmack(dirStr, mode, labelPtr) == LE_FAULT)
    {
        LE_DEBUG("Make directory %s failed.", dirStr);
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}


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
    return dir_MakeSmack(pathNamePtr, mode, NULL);
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
    return dir_MakePathSmack(pathNamePtr, mode, NULL);
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
    // Check to see if we're dealing with a single file or a symlink to a directory.  In either case
    // we simply have to delete the link or file.
    struct stat sourceStat;

    if (lstat(pathNamePtr, &sourceStat) == -1)
    {
        if (errno == ENOENT)
        {
            return LE_OK;
        }
        else
        {
            LE_CRIT("Error could not stat '%s'.  (%m)", pathNamePtr);
            return LE_FAULT;
        }
    }
    else if (   (S_ISLNK(sourceStat.st_mode))
             || (S_ISREG(sourceStat.st_mode)))
    {
        if (unlink(pathNamePtr) == -1)
        {
            LE_CRIT("Error could not unlink '%s'. (%m)", pathNamePtr);
            return LE_FAULT;
        }

        return LE_OK;
    }

    // Open the directory tree to search.
    char* pathArrayPtr[] = {(char*)pathNamePtr, NULL};

    errno = 0;
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
                    fts_close(ftsPtr);
                    return LE_FAULT;
                }
                break;

            case FTS_F:
            case FTS_NSOK:
                // These are files.
                if (remove(entPtr->fts_accpath) != 0)
                {
                    LE_ERROR("Could not remove file '%s'.  %m", entPtr->fts_accpath);
                    fts_close(ftsPtr);
                    return LE_FAULT;
                }
        }
    }

    int lastErrno = errno;
    fts_close(ftsPtr);

    if (lastErrno != 0)
    {
        LE_ERROR("Could not find directory '%s'.  %m", pathNamePtr);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the path refers to a directory.
 *
 * @return
 *      true if the path refers to a directory.  false otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool le_dir_IsDir
(
    const char* pathNamePtr     ///< [IN] The path to the directory.
)
{
    struct stat stats;
    if (stat(pathNamePtr, &stats) == -1)
    {
        if ( (errno == ENOENT) || (errno == ENOTDIR) )
        {
            return false;
        }

        LE_FATAL("Could not stat path '%s'.  %m", pathNamePtr);
    }

    return S_ISDIR(stats.st_mode);
}
