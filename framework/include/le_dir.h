/**
 * @page c_dir Directory API
 *
 * @subpage le_dir.h "API Reference"
 *
 * @section c_dir_create Creating Directories
 *
 * To create a directory at a specific location, call @c le_dir_Make() passing in the path name of the
 * directory to create.  All directories in the path name except the last directory (the directory
 * to be created) must exist prior to calling le_dir_Make().
 *
 * To create all directories in a specified path use @c le_dir_MakePath().
 *
 * With both le_dir_Make() and le_dir_MakePath() the calling process must have write and search
 * permissions on all directories in the path.
 *
 * @section c_dir_delete Removing Directories
 *
 * To remove a directory and everything in the directory (including all files and sub-directories)
 * use @c le_dir_RemoveRecursive().
 *
 * @section c_dir_read Reading Directories
 *
 * To read the contents of a directory use the POSIX function @c openDir().
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

//--------------------------------------------------------------------------------------------------
/** @file le_dir.h
 *
 * Legato @ref c_dir include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_DIR_INCLUDE_GUARD
#define LEGATO_DIR_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Creates a directory with permissions specified in mode.
 *
 * @note Permissions for the created directory will depend on the calling process' umask.
 *
 * @return
 *      LE_OK if successful.
 *      LE_DUPLICATE if the directory already exists.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dir_Make
(
    const char* pathNamePtr,    ///< [IN] Path name to the directory to create.
    mode_t mode                 ///< [IN] Permissions for the directory.
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates all directories in the path.  If some (or all) directories in the path already exist,
 * those directories are left as is.  All created directories have the same permissions
 * (specified in mode).
 *
 * @note Permissions for the created directories will depend on the calling process'
 *       umask.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dir_MakePath
(
    const char* pathNamePtr,    ///< [IN] Path containing all the directories to create.
    mode_t mode                 ///< [IN] Permissions for all created directories.
);


//--------------------------------------------------------------------------------------------------
/**
 * Removes a directory by first recursively removing sub-directories, files, symlinks, hardlinks,
 * devices, etc.  Symlinks are not followed; only the links themselves are deleted.
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
    const char* pathNamePtr           ///< [IN] Path to the directory to remove.
);


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
);


#endif // LEGATO_DIR_INCLUDE_GUARD
