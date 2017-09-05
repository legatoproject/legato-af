//--------------------------------------------------------------------------------------------------
/** @file dir.h
 *
 * Declaration of the framework's internal directory manipulation functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#ifndef LEGATO_SRC_DIR_INCLUDE_GUARD
#define LEGATO_SRC_DIR_INCLUDE_GUARD


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
);


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
);


#endif // LEGATO_SRC_DIR_INCLUDE_GUARD
