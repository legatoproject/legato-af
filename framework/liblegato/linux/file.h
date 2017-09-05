//--------------------------------------------------------------------------------------------------
/**
 * @file file.h
 *
 * Routines for dealing with files.  Checking for files, deleting files, doing simple reads, writes
 * and copies are all handled here.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_FILE_H_INCLUDE_GUARD
#define LEGATO_FILE_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether or not a file exists at a given file system path.
 *
 * @return true if the file exists and is a normal file.  false otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool file_Exists
(
    const char* filePath  ///< [IN] Path to the file in question.
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a file at a given path.
 */
//--------------------------------------------------------------------------------------------------
void file_Delete
(
    const char* filePath  ///< [IN] Path to the file to delete.
);


//--------------------------------------------------------------------------------------------------
/**
 * Read a string from a file given by filePath into a buffer provided by the caller.
 *
 * Will read up to size - 1 bytes from the file.
 *
 * The result will always be null-terminated.
 *
 * @return Number of bytes read (not including the null terminator) or -1 on failure.
 */
//--------------------------------------------------------------------------------------------------
ssize_t file_ReadStr
(
    const char* filePath,  ///< [IN]  Path to the file to read from.
    char* buffer,          ///< [OUT] The string is written to this buffer.
    size_t size            ///< [IN]  How big is this buffer (in bytes)?
);


//--------------------------------------------------------------------------------------------------
/**
 * Write a null-terminated string to a file given by filePath.
 * The null terminator will not be written.
 * The file will be opened, the string will be written and the file will be closed.
 * If the file does not exist, it will be created.
 * If the file did previously exist, its previous contents will be discarded, but its previous
 * DAC permissions will be kept.  To replace the existing file completely, use
 * file_WriteStrAtomic().
 */
//--------------------------------------------------------------------------------------------------
void file_WriteStr
(
    const char* filePath,   ///< [IN] Path to the file to write.
    const char* string,     ///< [IN] A string to write to this file.
    mode_t mode             ///< [IN] DAC permission bits (see man 2 open). E.g., 0644 = rw-r--r--
);


//--------------------------------------------------------------------------------------------------
/**
 * Atomically replace a file with another containing a string.
 *
 * The string's null terminator will not be written to the file.
 *
 * filePath.new will be created with the contents of the string, then renamed to filePath.
 */
//--------------------------------------------------------------------------------------------------
void file_WriteStrAtomic
(
    const char* filePath,      ///< [IN] Path to the file to write.
    const char* string,        ///< [IN] A string to write to this file.
    mode_t mode                ///< [IN] DAC permission bits (see man 2 open). E.g., 0644 = rw-r--r--
);


//--------------------------------------------------------------------------------------------------
/**
 * Copy a file.  This function copies the source file's owner, permissions and extended attributes
 * to the destination file as well.
 *
 * @return - LE_OK if the copy was successful.
 *         - LE_NOT_PERMITTED if either the source or destination paths are not files or could not
 *           be opened.
 *         - LE_IO_ERROR if an IO error occurs during the copy operation.
 *         - LE_NOT_FOUND if source file or the destination directory does not exist.
 */
//--------------------------------------------------------------------------------------------------
le_result_t file_Copy
(
    const char* sourcePathPtr,  ///< [IN] Copy from this path...
    const char* destPathPtr,    ///< [IN] To this path.
    const char* smackLabelPtr   ///< [IN] If not NULL, the file will have this smack label set.
);


//--------------------------------------------------------------------------------------------------
/**
 * Copy a batch of files recursively from one directory into another.  This function copies the
 * source files' owner, permissions and extended attributes to the destination files as well.
 *
 * @note Does not copy mounted files or any files under mounted directories.  Does not copy anything
 *       if the source path directory is empty.
 *
 * @return - LE_OK if the copy was successful.
 *         - LE_NOT_PERMITTED if either the source or destination paths are not files or could not
 *           be opened.
 *         - LE_IO_ERROR if an IO error occurs during the copy operation.
 *         - LE_NOT_FOUND if source file or the destination directory does not exist.
 */
//--------------------------------------------------------------------------------------------------
le_result_t file_CopyRecursive
(
    const char* sourcePathPtr,  ///< [IN] Copy recursively from this path...
    const char* destPathPtr,    ///< [IN] To this path.
    const char* smackLabelPtr   ///< [IN] If not NULL, the file will have this smack label set.
);


//--------------------------------------------------------------------------------------------------
/**
 * Rename a file or directory.
 **/
//--------------------------------------------------------------------------------------------------
void file_Rename
(
    const char* srcPath,
    const char* destPath
);


#endif // LEGATO_FILE_H_INCLUDE_GUARD
