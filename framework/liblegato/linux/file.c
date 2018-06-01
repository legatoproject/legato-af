//--------------------------------------------------------------------------------------------------
/**
 * @file file.c
 *
 * Routines for dealing with files.  Checking for files, deleting files, doing simple reads, writes
 * and copies are all handled here.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include <sys/sendfile.h>
#include "legato.h"
#include "smack.h"
#include "fileDescriptor.h"
#include "file.h"
#include "fileSystem.h"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum size of extended attribute list. This is generally limited to size of flash page
 */
//--------------------------------------------------------------------------------------------------
#define MAX_XATTR_LIST_SIZE             4096


//--------------------------------------------------------------------------------------------------
/**
 * Maximum size of extended attribute value. This is generally limited to size of flash page
 */
//--------------------------------------------------------------------------------------------------
#define MAX_XATTR_VALUE_SIZE            4096


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
)
//--------------------------------------------------------------------------------------------------
{
    struct stat fileStatus;

    // Use stat(2) to check for existence of the file.
    if (stat(filePath, &fileStatus) != 0)
    {
        // ENOENT = the file doesn't exist.  Anything else warrants and error report.
        if (errno != ENOENT)
        {
            LE_CRIT("Error when trying to stat '%s'. (%m)", filePath);
        }

        return false;
    }
    else
    {
        // Something exists. Make sure it's a file.
        // NOTE: stat() follows symlinks.
        if (S_ISREG(fileStatus.st_mode))
        {
            return true;
        }
        else
        {
            LE_CRIT("Unexpected file system object type (%#o) at path '%s'.",
                    fileStatus.st_mode & S_IFMT,
                    filePath);

            return false;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a file at a given path.
 */
//--------------------------------------------------------------------------------------------------
void file_Delete
(
    const char* filePath  ///< [IN] Path to the file to delete.
)
//--------------------------------------------------------------------------------------------------
{
    if ((unlink(filePath) != 0) && (errno != ENOENT))
    {
        LE_CRIT("Failed to delete file '%s' (%m).", filePath);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Read a string from a file given by filePath into a buffer provided by the caller.
 *
 * Will read up to size - 1 bytes from the file.
 *
 * On success the output string will always be null-terminated.
 *
 * @return Number of bytes read (not including the null terminator) or -1 on failure.
 */
//--------------------------------------------------------------------------------------------------
ssize_t file_ReadStr
(
    const char* filePath,  ///< [IN]  Path to the file to read from.
    char* buffer,          ///< [OUT] The string is written to this buffer.
    size_t size            ///< [IN]  How big is this buffer (in bytes)?
)
//--------------------------------------------------------------------------------------------------
{
    int fd;
    ssize_t bytesRead = 0;

    // Null terminate buffer (we won't ever read that many bytes).
    buffer[size - 1] = '\0';

    fd = open(filePath, O_RDONLY);
    if (fd == -1)
    {
        LE_CRIT("Unable to open file '%s' for reading (%m).", filePath);
        bytesRead = -1;
    }
    else
    {
        ssize_t readResult = 0;

        while (bytesRead < (size - 1))
        {
            do
            {
                readResult = read(fd, buffer + bytesRead, (size - 1) - bytesRead);
            }
            while ((readResult == -1) && (errno == EINTR));

            if (readResult == -1)
            {
                LE_CRIT("Error reading from file '%s' (%m).", filePath);
                bytesRead = -1;
                break;
            }
            else if (readResult == 0)
            {
                // finished file

                // Null terminate string that is shorter than the buffer could have held.
                buffer[bytesRead] = '\0';

                break;
            }

            bytesRead += readResult;
        }

        fd_Close(fd);
    }

    return bytesRead;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    int fd;

    fd = open(filePath, O_WRONLY | O_TRUNC | O_CREAT, mode);
    if (fd == -1)
    {
        LE_FATAL("Unable to open file '%s' for writing (%m).", filePath);
    }
    else
    {
        size_t writeBytes = 0;

        if (   (string != NULL)
            && ((writeBytes = strlen(string)) > 0))
        {
            ssize_t result;

            do
            {
                result = write(fd, string, writeBytes);
            }
            while ((result == -1) && (errno == EINTR));

            if (result == -1)
            {
                LE_FATAL("Error writing to file '%s' (%m).", filePath);
            }
            else if (result != writeBytes)
            {
                LE_FATAL("Unable to write all bytes of '%s' to file '%s'.", string, filePath);
            }
        }

        fd_Close(fd);
    }
}


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
)
//--------------------------------------------------------------------------------------------------
{
    char tempFilePath[PATH_MAX];

    if (snprintf(tempFilePath, sizeof(tempFilePath), "%s.new", filePath) >= sizeof(tempFilePath))
    {
        LE_FATAL("File path '%s' is too long (>= PATH_MAX - 4).", filePath);
    }

    file_WriteStr(tempFilePath, string, mode);

    file_Rename(tempFilePath, filePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Open an existing file for reading.
 *
 * @return LE_OK is successful, LE_NOT_PERMITTED is returned otherwise.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t OpenRead
(
    const char* sourcePathPtr,  ///< [IN]  The file to open.
    int* fdPtr                  ///< [OUT] The file's FD to use for future reading on this file.
)
//--------------------------------------------------------------------------------------------------
{
    *fdPtr = -1;

    int newFd;

    do
    {
        newFd = open(sourcePathPtr, O_RDONLY);
    }
    while (   (newFd == -1)
           && (errno == EINTR));

    if (newFd == -1)
    {
        LE_CRIT("Error when opening file for reading, '%s'. (%m)", sourcePathPtr);
        return LE_NOT_PERMITTED;
    }

    *fdPtr = newFd;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create and open new file for writing.
 *
 * @return LE_OK is successful, LE_NOT_PERMITTED is returned otherwise.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateWrite
(
    const char* destPathPtr,    ///< [IN]  Path to the file to create.
    mode_t mode,                ///< [IN]  The mode bits to set for the file.
    int* fdPtr                  ///< [OUT] The FD of the newly created and opened file.
)
//--------------------------------------------------------------------------------------------------
{
    *fdPtr = -1;

    int newFd;

    do
    {
        newFd = creat(destPathPtr, mode);
    }
    while (   (newFd == -1)
           && (errno == EINTR));

    if (newFd == -1)
    {
        LE_CRIT("Error when opening file for writing, '%s'. (%m)", destPathPtr);
        return LE_NOT_PERMITTED;
    }

    *fdPtr = newFd;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check to see if the parent of the filesystem object in question actually exists.
 *
 * @return True if the parent directory exists, false if not.
 */
//--------------------------------------------------------------------------------------------------
static bool BasePathExists
(
    const char* pathPtr  ///< Check to see if the parent of this filesystem object actually exists.
)
//--------------------------------------------------------------------------------------------------
{
    char basePath[PATH_MAX];

    LE_ASSERT(le_path_GetDir(pathPtr, "/", basePath, sizeof(basePath)) == LE_OK);
    return le_dir_IsDir(basePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stat the given path.
 *
 * @return - LE_OK if all goes to plan.
 *         - LE_NOT_FOUND if the specified file system object does not exist.
 -         - LE_IO_ERROR if the stat fails for any other reason.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StatPath
(
    const char* pathPtr,    ///< [IN]  Stat this file system object.
    struct stat* statusPtr  ///< [OUT] If successful, populate this stat structure.
)
//--------------------------------------------------------------------------------------------------
{
    if (stat(pathPtr, statusPtr) != 0)
    {
        if (errno == ENOENT)
        {
            return LE_NOT_FOUND;
        }
        else
        {
            LE_CRIT("Error when trying to stat '%s'. (%m)", pathPtr);
            return LE_IO_ERROR;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Copies the extended attributes from the source to the destination file.
 *
 * @return - LE_OK if all goes to plan.
 -         - LE_IO_ERROR if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CopyXattrs
(
    const char* srcPathPtr,                 ///< [IN] Path to source file.
    const char* destPathPtr                 ///< [IN] Path to destination file.
)
//--------------------------------------------------------------------------------------------------
{

    // Get a list of extended attributes.
    char xattrList[MAX_XATTR_LIST_SIZE] = "";

    ssize_t listSize = listxattr(srcPathPtr, xattrList, sizeof(xattrList));

    if (listSize == -1)
    {
        LE_ERROR("Could not get list of extended attributes for %s.  %m.", srcPathPtr);
        return LE_IO_ERROR;
    }

    // Copy each extended attribute.
    char* namePtr = xattrList;

    while (listSize > 0)
    {
        // Get the extended attribute value.
        char value[MAX_XATTR_VALUE_SIZE] = "";

        ssize_t valueSize = getxattr(srcPathPtr, namePtr, value, sizeof(value));

        if (valueSize == -1)
        {
            LE_ERROR("Could not get value for extended attribute %s for file %s.  %m.",
                     namePtr, srcPathPtr);
            return LE_IO_ERROR;
        }

        // Set the extended attribute in the destination file.
        if (setxattr(destPathPtr, namePtr, value, valueSize, 0) == -1)
        {
            if (errno == EPERM)
            {
                // TODO: This workaround is for sandboxed app. Change it later.
                LE_DEBUG("Permission denied to set smack label");
            }
            else
            {
                LE_ERROR("Could not set extended attribute %s for file %s.  %m.",
                          namePtr, srcPathPtr);
                return LE_IO_ERROR;
            }
        }

        ssize_t nameLen = strlen(namePtr) + 1;
        listSize -= nameLen;
        namePtr += nameLen;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Copies the owner, group and the extended attributes from the source to the destination file.
 *
 * @return - LE_OK if all goes to plan.
 -         - LE_IO_ERROR if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CopyAttrs
(
    const char* srcPathPtr,     ///< [IN] Path to source file.
    const char* destPathPtr,    ///< [IN] Path to destination file.
    const char* smackLabelPtr   ///< [IN] If not NULL, set the dest file to this smack label.
)
//--------------------------------------------------------------------------------------------------
{
    struct stat sourceStatus;

    le_result_t result = StatPath(srcPathPtr, &sourceStatus);

    if (result != LE_OK)
    {
        return LE_IO_ERROR;
    }

    // Set the owner of the dest file.
    if (chown(destPathPtr, sourceStatus.st_uid, sourceStatus.st_gid) == -1)
    {
        LE_ERROR("Could not set owner and group of %s.  %m.", destPathPtr);
        return LE_IO_ERROR;
    }

    // Copy all extended attributes.
    result = CopyXattrs(srcPathPtr, destPathPtr);

    if (result != LE_OK)
    {
        return result;
    }

    // Set the SMACK label.
    if (smackLabelPtr != NULL)
    {
        if (smack_SetLabel(destPathPtr, smackLabelPtr) != LE_OK)
        {
            return LE_IO_ERROR;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a destination directory with the same owner, permissions and extended attributes as the
 * source directory.
 *
 * @return - LE_OK if the copy was successful.
 *         - LE_IO_ERROR if an IO error occurs during the copy operation.
 *         - LE_NOT_FOUND if source file or the destination directory does not exist.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateDir
(
    const char* sourcePathPtr,  ///< [IN] Source directory.
    const char* destPathPtr,    ///< [IN] Destination directory.
    const char* smackLabelPtr   ///< [IN] If not NULL, the directory will have this smack label set.
)
//--------------------------------------------------------------------------------------------------
{
    struct stat sourceStatus;

    le_result_t result = StatPath(sourcePathPtr, &sourceStatus);

    if (result != LE_OK)
    {
        return result;
    }

    if (BasePathExists(destPathPtr) == false)
    {
        return LE_NOT_FOUND;
    }

    // Create the directory with the proper permissions.
    if (le_dir_MakePath(destPathPtr, sourceStatus.st_mode) == LE_FAULT)
    {
        return LE_IO_ERROR;
    }

    // Set the owner and extended attributes.
    return CopyAttrs(sourcePathPtr, destPathPtr, smackLabelPtr);
}


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
)
//--------------------------------------------------------------------------------------------------
{
    // Make sure that the source file exists.
    struct stat sourceStatus;

    le_result_t result = StatPath(sourcePathPtr, &sourceStatus);

    if (result != LE_OK)
    {
        return result;
    }

    if (!S_ISREG(sourceStatus.st_mode))
    {
        // There's something there, but it's not a file or a symlink to a file.
        return LE_NOT_PERMITTED;
    }

    // Make sure that the output directory exists.
    if (BasePathExists(destPathPtr) == false)
    {
        return LE_NOT_FOUND;
    }

    // If the output file exists, make sure that it's actually a file, and not a directory or a
    // device or something.
    struct stat destStatus;
    result = StatPath(destPathPtr, &destStatus);

    if (result == LE_IO_ERROR)
    {
        return result;
    }

    if (   (result != LE_NOT_FOUND)
        && (!S_ISREG(destStatus.st_mode)))
    {
        return LE_NOT_PERMITTED;
    }

    // Open our files for reading and writing.
    int readFd;
    int writeFd;

    result = OpenRead(sourcePathPtr, &readFd);

    if (result != LE_OK)
    {
        return result;
    }

    result = CreateWrite(destPathPtr, sourceStatus.st_mode, &writeFd);

    if (result != LE_OK)
    {
        fd_Close(readFd);
        return result;
    }

    // Copy owner and extended attributes.
    result = CopyAttrs(sourcePathPtr, destPathPtr, smackLabelPtr);

    if (result != LE_OK)
    {
        fd_Close(readFd);
        fd_Close(writeFd);
        return result;
    }

    // Get the kernel to copy the data over.  It may or may not happen in one go, so keep trying
    // until the whole file has been written or we error out.
    ssize_t sizeWritten = 0;
    result = LE_OK;
    off_t fileOffset = 0;

    while (sizeWritten < sourceStatus.st_size)
    {
        ssize_t nextWritten = sendfile(writeFd,
                                       readFd,
                                       &fileOffset,
                                       sourceStatus.st_size - sizeWritten);

        if (nextWritten == -1)
        {
            LE_CRIT("Error when copying file '%s' to '%s'. (%m)", sourcePathPtr, destPathPtr);
            result = LE_IO_ERROR;
            break;
        }

        sizeWritten += nextWritten;
    }

    fd_Close(readFd);
    fd_Close(writeFd);

    return result;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    // Make sure that the source file exists.
    struct stat sourceStatus;
    struct stat destStatus;

    le_result_t result = StatPath(sourcePathPtr, &sourceStatus);

    if (result != LE_OK)
    {
        return result;
    }

    // If the source is a file, then just copy it.
    if (S_ISREG(sourceStatus.st_mode))
    {
        return file_Copy(sourcePathPtr, destPathPtr, smackLabelPtr);
    }

    // Now check the destination.
    result = StatPath(destPathPtr, &destStatus);

    if (result == LE_IO_ERROR)
    {
        return result;
    }

    // If the destination doesn't exist, make sure it's base path exists.
    if (result == LE_NOT_FOUND)
    {
        result = CreateDir(sourcePathPtr, destPathPtr, smackLabelPtr);

        if (result != LE_OK)
        {
            return result;
        }
    }
    else if (!S_ISDIR(destStatus.st_mode))
    {
        // Looks like we're trying to copy a dir to a file or device or something.
        LE_CRIT("Attempting to copy a directory, '%s', into a file, '%s'.",
                sourcePathPtr,
                destPathPtr);

        return LE_NOT_PERMITTED;
    }

    // Iterate through the directory and copy the files to the destination.
    size_t sourcePathLen = strlen(sourcePathPtr);

    char* pathArrayPtr[] = { (char*)sourcePathPtr, NULL };
    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL, NULL);

    result = LE_OK;

    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        char newPath[PATH_MAX] = "";

        if (le_path_Concat("/", newPath, sizeof(newPath),
                           destPathPtr, entPtr->fts_path + sourcePathLen, NULL) != LE_OK)
        {
            LE_CRIT("Destination path to file '%s' too long.",
                    le_path_GetBasenamePtr(entPtr->fts_path, "/"));
            result = LE_IO_ERROR;
            goto cleanup;
        }

        switch (entPtr->fts_info)
        {
            // A directory, visited in pre-order.
            case FTS_D:
                if (entPtr->fts_level > 0)
                {
                    if (fs_IsMountPoint(entPtr->fts_path))
                    {
                        if (fts_set(ftsPtr, entPtr, FTS_SKIP) != 0)
                        {
                            result = LE_IO_ERROR;
                            goto cleanup;
                        }
                    }
                    else
                    {
                        result = CreateDir(entPtr->fts_path, newPath, smackLabelPtr);

                        if (result != LE_OK)
                        {
                            goto cleanup;
                        }
                    }
                }
                break;

            // Directory visited in reverse order.
            case FTS_DP:
                // Nothing to do here.
                break;

            // Unspecified type.
            case FTS_DEFAULT:
                // Nothing to do here.
                break;

            // Regular file.
            case FTS_F:
                if (!fs_IsMountPoint(entPtr->fts_path))
                {
                    result = file_Copy(entPtr->fts_path, newPath, smackLabelPtr);
                    if (result != LE_OK)
                    {
                        goto cleanup;
                    }
                }
                break;

            // A symbolic link or a symbolic link that doesn't point to a file.
            case FTS_SL:
            case FTS_SLNONE:
                if (!fs_IsMountPoint(entPtr->fts_path))
                {
                    char linkBuffer[PATH_MAX] = "";
                    ssize_t bytesRead = readlink(entPtr->fts_path, linkBuffer, sizeof(linkBuffer));
                    if (bytesRead < 0)
                    {
                        LE_CRIT("Failed to read symlink '%s'.", entPtr->fts_path);
                        result = LE_IO_ERROR;
                        goto cleanup;
                    }

                    int linkResult;
                    char resolvedLinkPath[PATH_MAX] = "";
                    if (realpath(linkBuffer, resolvedLinkPath) == NULL)
                    {
                        LE_CRIT("No such path : '%s'",linkBuffer);
                        result = LE_IO_ERROR;
                        goto cleanup;
                    }
                    else
                    {
                        linkResult = symlink(linkBuffer, newPath);
                    }

                    if (linkResult == -1)
                    {
                        LE_CRIT("Failed to create symlink '%s' to '%s'.  (%m)",
                                newPath,
                                linkBuffer);
                        result = LE_IO_ERROR;
                        goto cleanup;
                    }
                }
                break;

            // Cyclic directory.
            case FTS_DC:
                LE_CRIT("Cyclic directory structure detected, '%s'.", entPtr->fts_path);
                result = LE_NOT_PERMITTED;
                goto cleanup;

            // A directory which cannot be read.
            case FTS_DNR:
                LE_CRIT("Could not read directory information, '%s'.", entPtr->fts_path);
                result = LE_IO_ERROR;
                goto cleanup;

            // A file for which no stat information was available.
            // Or error occurred while calling stat.
            case FTS_NS:
            case FTS_ERR:
                LE_CRIT("Error reading file/directory information, '%s'. (%s)",
                        entPtr->fts_path,
                        strerror(entPtr->fts_errno));
                result = LE_IO_ERROR;
                goto cleanup;

            default:
                LE_CRIT("Unexpected file type, %d, on file %s.",
                        entPtr->fts_info,
                        entPtr->fts_path);
                result = LE_IO_ERROR;
                goto cleanup;
        }
    }

 cleanup:

    fts_close(ftsPtr);

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Rename a file or directory.
 **/
//--------------------------------------------------------------------------------------------------
void file_Rename
(
    const char* srcPath,
    const char* destPath
)
//--------------------------------------------------------------------------------------------------
{
    // Move the new system in place.
    if (rename(srcPath, destPath) != 0)
    {
        LE_FATAL("Failed rename '%s' to '%s' (%m).", srcPath, destPath);
    }
}
