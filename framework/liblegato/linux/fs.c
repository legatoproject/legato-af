/**
 * @file fs.c
 *
 * This file contains the data structures and the source code of the File System (FS) service.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "file.h"
#include "dir.h"


//--------------------------------------------------------------------------------------------------
/**
 * Default prefix path for RW if nothing is defined in the config tree
 */
//--------------------------------------------------------------------------------------------------
#define FS_PREFIX_DATA_PATH      "/data/le_fs/"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of fileRef managed by the service
 */
//--------------------------------------------------------------------------------------------------
#define FS_MAX_FILE_REF          32

//--------------------------------------------------------------------------------------------------
/**
 * File structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_fs_FileRef_t fileRef;  ///< The file reference to exchange with clients
    int fd;                   ///< The file descriptor
}
File_t;

//--------------------------------------------------------------------------------------------------
/**
 * Default prefixes path used by the daemon. If NULL, the daemon will reject all open/rename/delete
 * operations
 */
//--------------------------------------------------------------------------------------------------
static char* FsPrefixPtr = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Pool to store the file structures
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t FsFileRefPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe reference map for the file structure
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t FsFileRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * This function adds the prefix to the filePath to access
 *
 * @return
 *  - NULL if the prefix is not set
 *  - Otherwise a pointer on the full path string related to the le_fs directory
 */
//--------------------------------------------------------------------------------------------------
static char* BuildPathName
(
    char* pathPtr,            ///< [OUT] Full file path with prefix
    size_t pathSize,          ///< [IN]  Length of the full path array
    const char* filePathPtr   ///< [IN]  File path
)
{
    if (NULL == FsPrefixPtr)
    {
        return NULL;
    }
    snprintf(pathPtr, pathSize, "%s%s", FsPrefixPtr, filePathPtr);
    return pathPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to create the directories of a file path if some do not exist
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_UNSUPPORTED    The prefix cannot be added and the function is unusable
 *  - LE_NOT_POSSIBLE   A directory in the tree belongs to a Read-Only space and cannot be created
 *  - LE_NOT_PERMITTED  A directory in the tree belongs has not access right and cannot be created
 *  - LE_FAULT          The function fails while creating or accessing to a directory.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t MkDirTree
(
    const char* filePathPtr   ///< [IN]  Directory path
)
{
    char* slashPtr = (char *)filePathPtr;
    char dirPath[PATH_MAX];

    if (NULL == FsPrefixPtr)
    {
        return LE_UNSUPPORTED;
    }

    while ((slashPtr = strchr (slashPtr + 1, '/')))
    {
        memset(dirPath, 0, PATH_MAX);
        strncat(strncpy(dirPath, FsPrefixPtr, (PATH_MAX-1)), filePathPtr, slashPtr - filePathPtr);
        if ((-1 == mkdir(dirPath, S_IRWXU)) && (EEXIST != errno))
        {
            if (EROFS == errno)
            {
                return LE_NOT_POSSIBLE;
            }
            else if ((EPERM == errno) || (EACCES == errno))
            {
                return LE_NOT_PERMITTED;
            }
            else
            {
                return LE_FAULT;
            }
        }
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Destructor function that runs when a file ref is deallocated
 */
//--------------------------------------------------------------------------------------------------
static void FsFileRefDestructor
(
    void* objPtr
)
{
    File_t* filePtr = (File_t*) objPtr;

    if (NULL != filePtr)
    {
        // Release the reference
        le_ref_DeleteRef(FsFileRefMap, filePtr->fileRef);
    }
}

//--------------------------------------------------------------------------------------------------
// APIs
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to create or open an existing file.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The file path is too long.
 *  - LE_NOT_FOUND      The file does not exists or a directory in the path does not exist
 *  - LE_NOT_PERMITTED  Access denied to the file or to a directory in the path
 *  - LE_UNSUPPORTED    The prefix cannot be added and the function is unusable
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fs_Open
(
    const char* filePathPtr,        ///< [IN]  File path
    le_fs_AccessMode_t accessMode,  ///< [IN]  Access mode for the file
    le_fs_FileRef_t* fileRefPtr     ///< [OUT] File reference (if successful)
)
{
    mode_t mode = 0;
    int fd;
    char path[PATH_MAX];

    // Check whether input is null. filePathPtr can be null as it is a pointer (i.e. pointer to
    // const char).
    if (filePathPtr == NULL)
    {
        LE_ERROR("File path can't be null");
        return LE_BAD_PARAMETER;
    }

    // Check if the pointers are set
    *fileRefPtr = (le_fs_FileRef_t)NULL;

    // Check if the file path starts with '/'
    if ('/' != *filePathPtr)
    {
        LE_ERROR("File path should start with '/'");
        return LE_BAD_PARAMETER;
    }

    // Check if the access mode is correct
    if ((accessMode & (uint16_t)(~(uint16_t)LE_FS_ACCESS_MODE_MAX)) || (0 == accessMode))
    {
        LE_ERROR("Unable to open file, wrong access mode 0x%04X", accessMode);
        return LE_BAD_PARAMETER;
    }

    if (LE_FS_RDONLY & accessMode)
    {
        mode |= O_RDONLY;
    }
    if (LE_FS_WRONLY & accessMode)
    {
        mode |= O_WRONLY;
    }
    if (LE_FS_RDWR & accessMode)
    {
        mode |= O_RDWR;
    }
    if (LE_FS_CREAT & accessMode)
    {
        mode |= O_CREAT;
    }
    if (LE_FS_TRUNC & accessMode)
    {
        mode |= O_TRUNC;
    }
    if (LE_FS_APPEND & accessMode)
    {
        mode |= O_APPEND;
    }
    if (LE_FS_SYNC & accessMode)
    {
        mode |= O_SYNC;
    }

    if ((mode & O_CREAT) && (LE_OK != MkDirTree(filePathPtr)))
    {
        return LE_FAULT;
    }

    if (NULL == BuildPathName(path, PATH_MAX, filePathPtr))
    {
        return LE_UNSUPPORTED;
    }

    fd = open(path, mode, S_IRUSR | S_IWUSR);
    if (-1 < fd)
    {
        File_t* tmpFilePtr = le_mem_ForceAlloc(FsFileRefPool);
        tmpFilePtr->fd = fd;
        tmpFilePtr->fileRef = le_ref_CreateRef(FsFileRefMap, tmpFilePtr);
        *fileRefPtr = (le_fs_FileRef_t)(tmpFilePtr->fileRef);
        return LE_OK;
    }
    else if (ENOENT == errno)
    {
        return LE_NOT_FOUND;
    }
    else if (EACCES == errno)
    {
        return LE_NOT_PERMITTED;
    }
    else
    {
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to close an opened file.
 *
 * @return
 *  - LE_OK         The function succeeded.
 *  - LE_FAULT      The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fs_Close
(
    le_fs_FileRef_t fileRef     ///< [IN] File reference
)
{
    File_t* filePtr;
    int rc;

    filePtr = le_ref_Lookup(FsFileRefMap, fileRef);
    if (NULL == filePtr)
    {
        return LE_BAD_PARAMETER;
    }
    rc = close(filePtr->fd);
    if (!rc)
    {
        le_mem_Release(filePtr);
    }
    else
    {
        LE_ERROR("Failed to close descriptor %d: %m", filePtr->fd);
    }
    return (!rc) ? LE_OK : LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to read the requested data length from an opened file.
 * The data is read at the current file position.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fs_Read
(
    le_fs_FileRef_t fileRef,     ///< [IN]  File reference
    uint8_t* bufPtr,             ///< [OUT] Buffer to store the data read in the file
    size_t* bufNumElementsPtr    ///< [IN]  Number of bytes to read when this function is called
                                 ///< [OUT] Number of bytes read when this function returns
)
{
    File_t* filePtr;
    int rc;

    // Check if the pointers are set
    if (NULL == bufPtr)
    {
        LE_ERROR("NULL buffer pointer!");
        return LE_BAD_PARAMETER;
    }
    if (NULL == bufNumElementsPtr)
    {
        LE_ERROR("NULL bytes number pointer!");
        return LE_BAD_PARAMETER;
    }

    // Check the number of bytes to read
    if (0 == *bufNumElementsPtr)
    {
        // No need to read 0 bytes
        return LE_OK;
    }

    filePtr = le_ref_Lookup(FsFileRefMap, fileRef);
    if (NULL == filePtr)
    {
        return LE_BAD_PARAMETER;
    }
    do
    {
        rc = read(filePtr->fd, bufPtr, *bufNumElementsPtr);
    }
    while ((-1 == rc) && (EINTR == errno));

    if (rc < 0)
    {
        return LE_FAULT;
    }

    *bufNumElementsPtr = rc;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to write the requested data length to an opened file.
 * The data is written at the current file position.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNDERFLOW      The write succeed but was not able to write all bytes
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fs_Write
(
    le_fs_FileRef_t fileRef,  ///< [IN] File reference
    const uint8_t* bufPtr,    ///< [IN] Buffer to write in the file
    size_t bufNumElements     ///< [IN] Number of bytes to write
)
{
    File_t* filePtr;
    int rc;
    filePtr = le_ref_Lookup(FsFileRefMap, fileRef);

    if (NULL == filePtr)
    {
        LE_ERROR("fileRef is invalid");
        return LE_BAD_PARAMETER;
    }

    // Check if the pointer is set
    if (NULL == bufPtr)
    {
        LE_ERROR("NULL buffer pointer!");
        return LE_BAD_PARAMETER;
    }

    // Check the number of bytes to write
    if (0 == bufNumElements)
    {
        // No need to write 0 bytes
        return LE_OK;
    }

    do
    {
        rc = write(filePtr->fd, bufPtr, bufNumElements);
    }
    while ((-1 == rc) && (EINTR == errno));

    if (-1 == rc)
    {
        return LE_FAULT;
    }

    if (rc != bufNumElements)
    {
        return LE_UNDERFLOW;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to change the file position of an opened file.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fs_Seek
(
    le_fs_FileRef_t fileRef,    ///< [IN]  File reference
    int32_t offset,             ///< [IN]  Offset to apply when this function is called
    le_fs_Position_t position,  ///< [IN]  Offset is relative to this position
    int32_t* currentOffsetPtr   ///< [OUT] Offset from the beginning after the seek operation
)
{
    File_t* filePtr;
    int whence = 0;
    off_t rc;

    // Check if the pointer is set
    if (NULL == currentOffsetPtr)
    {
        LE_ERROR("NULL current offset pointer!");
        return LE_BAD_PARAMETER;
    }
    if (LE_FS_SEEK_SET == position)
    {
        whence = SEEK_SET;
    }
    else if (LE_FS_SEEK_CUR == position)
    {
        whence = SEEK_CUR;
    }
    else if (LE_FS_SEEK_END == position)
    {
        whence = SEEK_END;
    }
    else
    {
        LE_ERROR("Wrong seek position!");
        return LE_BAD_PARAMETER;
    }

    filePtr = le_ref_Lookup(FsFileRefMap, fileRef);
    if (NULL == filePtr)
    {
        return LE_BAD_PARAMETER;
    }
    rc = lseek(filePtr->fd, (off_t)offset, whence);

    if (-1 == rc)
    {
        return LE_FAULT;
    }
    *currentOffsetPtr = (int32_t)rc;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to get the size of a file.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The file path is too long.
 *  - LE_UNSUPPORTED    The prefix cannot be added and the function is unusable
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fs_GetSize
(
    const char* filePathPtr,    ///< [IN]  File path
    size_t* sizePtr             ///< [OUT] File size (if successful)
)
{
    int rc;
    struct stat st;
    char path[PATH_MAX];

    // Check whether input is null. filePathPtr can be null as it is a pointer (i.e. pointer to
    // const char).
    if (filePathPtr == NULL)
    {
        LE_ERROR("File path can't be null");
        return LE_BAD_PARAMETER;
    }

    // Check if the pointers are set
    if (NULL == sizePtr)
    {
        LE_ERROR("NULL size pointer!");
        return LE_BAD_PARAMETER;
    }

    // Check if the file path starts with '/'
    if ('/' != *filePathPtr)
    {
        LE_ERROR("File path should start with '/'");
        return LE_BAD_PARAMETER;
    }

    if (NULL == BuildPathName(path, PATH_MAX, filePathPtr))
    {
        return LE_UNSUPPORTED;
    }

    rc = stat(path, &st);

    if (-1 == rc)
    {
        return LE_FAULT;
    }
    *sizePtr = st.st_size;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to delete a file.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The file path is too long.
 *  - LE_NOT_FOUND      The file does not exist or a directory in the path does not exist
 *  - LE_NOT_PERMITTED  The access right fails to delete the file or access is not granted to a
 *                      a directory in the path
 *  - LE_UNSUPPORTED    The prefix cannot be added and the function is unusable
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fs_Delete
(
    const char* filePathPtr     ///< [IN] File path
)
{
    int rc;
    char path[PATH_MAX];

    // Check whether input is null. filePathPtr can be null as it is a pointer (i.e. pointer to
    // const char).
    if (filePathPtr == NULL)
    {
        LE_ERROR("File path can't be null");
        return LE_BAD_PARAMETER;
    }

    // Check if the pointer is set
    // Check if the file path starts with '/'
    if ('/' != *filePathPtr)
    {
        LE_ERROR("File path should start with '/'");
        return LE_BAD_PARAMETER;
    }

    if (NULL == BuildPathName(path, PATH_MAX, filePathPtr))
    {
        return LE_UNSUPPORTED;
    }

    rc = unlink(path);
    if ((-1 == rc) && (ENOENT == errno))
    {
        return LE_NOT_FOUND;
    }
    else if ((-1 == rc) && ((EACCES == errno) || (EPERM == errno)))
    {
        return LE_NOT_PERMITTED;
    }
    else
    {
        return (!rc) ? LE_OK : LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function checks whether a regular file exists at the provided path under file system service
 * storage.
 *
 * @return
 *  - true              If file exists and it is a regular file.
 *  - false             Otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool le_fs_Exists
(
    const char* filePathPtr     ///< [IN] File path
)
{
    char path[PATH_MAX] = "";

    // Check whether input is null. filePathPtr can be null as it is a pointer (i.e. pointer to
    // const char).
    if (filePathPtr == NULL)
    {
        LE_ERROR("File path can't be null");
        return LE_BAD_PARAMETER;
    }

    // Check if the file path starts with '/'
    if ('/' != *filePathPtr)
    {
        LE_ERROR("File path should start with '/'");
        return false;
    }

    if (NULL == BuildPathName(path, PATH_MAX, filePathPtr))
    {
        return false;
    }

    return file_Exists(path);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes a directory located at storage managed by file system service by first recursively
 * removing sub-directories, files, symlinks, hardlinks, devices, etc.  Symlinks are not followed,
 * only the links themselves are deleted.
 *
 * A file or device may not be able to be removed if it is busy, in which case an error message
 * is logged and LE_FAULT is returned.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNSUPPORTED    The prefix cannot be added and the function is unusable
 *  - LE_FAULT          There is an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fs_RemoveDirRecursive
(
    const char* dirPathPtr     ///< [IN] Directory path
)
{
    char path[PATH_MAX] = "";

    // Check whether input is null. dirPathPtr can be null as it is a pointer (i.e. pointer to
    // const char).
    if (dirPathPtr == NULL)
    {
        LE_ERROR("Directory path can't be null");
        return LE_BAD_PARAMETER;
    }

    // Check if the pointer is set
    // Check if the file path starts with '/'
    if ('/' != *dirPathPtr)
    {
        LE_ERROR("File path should start with '/'");
        return LE_BAD_PARAMETER;
    }

    if (NULL == BuildPathName(path, PATH_MAX, dirPathPtr))
    {
        return LE_UNSUPPORTED;
    }

    return le_dir_RemoveRecursive(path);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to rename an existing file.
 * If rename fails, the file will keep its original name.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       A file path is too long.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fs_Move
(
    const char* srcPathPtr,     ///< [IN] Path to file to rename
    const char* destPathPtr     ///< [IN] New path to file
)
{
    int rc;
    char srcPath[PATH_MAX];
    char destPath[PATH_MAX];

    // Check whether input is null. srcPathPtr can be null as it is a pointer (i.e. pointer to
    // const char).
    if (srcPathPtr == NULL)
    {
        LE_ERROR("Source file path can't be null");
        return LE_BAD_PARAMETER;
    }

    // destPathPtr can be null as it is a pointer (i.e. pointer to const char).
    if (destPathPtr == NULL)
    {
        LE_ERROR("Destination file path can't be null");
        return LE_BAD_PARAMETER;
    }

    // Check if the file paths start with '/'
    if ('/' != *srcPathPtr)
    {
        LE_ERROR("Source file path should start with '/'");
        return LE_BAD_PARAMETER;
    }
    if ('/' != *destPathPtr)
    {
        LE_ERROR("Destination file path should start with '/'");
        return LE_BAD_PARAMETER;
    }

    // Check if the paths are different
    if (0 == strcmp(srcPathPtr, destPathPtr))
    {
        LE_ERROR("Same path for source and destination!");
        return LE_BAD_PARAMETER;
    }

    if (NULL == BuildPathName(srcPath, PATH_MAX, srcPathPtr))
    {
        return LE_UNSUPPORTED;
    }

    if (NULL == BuildPathName(destPath, PATH_MAX, destPathPtr))
    {
        return LE_UNSUPPORTED;
    }

    rc = rename(srcPath, destPath);
    if ((-1 == rc) && (ENOENT == errno))
    {
        return LE_NOT_FOUND;
    }
    else if ((-1 == rc) && ((EACCES == errno) || (EPERM == errno)))
    {
        return LE_NOT_PERMITTED;
    }
    else
    {
        return (!rc) ? LE_OK : LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize this component
 */
//--------------------------------------------------------------------------------------------------
void fs_Init
(
    void
)
{
    le_result_t res;
    char* fsPrefixArray[] =
    {
        FS_PREFIX_DATA_PATH,
        "/tmp" FS_PREFIX_DATA_PATH,
        NULL,
    };
    char** tempFsPrefixPtr = fsPrefixArray;

    do
    {
        if (-1 == access(*tempFsPrefixPtr, R_OK | W_OK | X_OK))
        {
            if (ENOENT == errno)
            {
                FsPrefixPtr = "";
                res = MkDirTree(*tempFsPrefixPtr);
                if (LE_OK == res)
                {
                    FsPrefixPtr = *tempFsPrefixPtr;
                    break;
                }
                else if ((LE_NOT_POSSIBLE == res) || (LE_NOT_PERMITTED == res))
                {
                    FsPrefixPtr = NULL;
                    tempFsPrefixPtr++;
                }
                else
                {
                    FsPrefixPtr = NULL;
                    LE_CRIT("Unable to create directory '%s'", *tempFsPrefixPtr);
                    goto end;
                }
            }
            else
            {
                LE_ERROR("Failed to access \"%s\": %m", *tempFsPrefixPtr);
                tempFsPrefixPtr++;
            }
        }
        else
        {
            FsPrefixPtr = *tempFsPrefixPtr;
            break;
        }
    }
    while (*tempFsPrefixPtr);

end:
    if (NULL == FsPrefixPtr)
    {
        LE_CRIT("le_fs module is unusable because no valid prefix path");
    }
    else
    {
        LE_DEBUG("FS prefix path \"%s\"", FsPrefixPtr);
    }

    FsFileRefPool = le_mem_CreatePool("FsFileRefPool", sizeof(File_t));
    le_mem_ExpandPool (FsFileRefPool, FS_MAX_FILE_REF);
    le_mem_SetDestructor(FsFileRefPool, FsFileRefDestructor);

    // Create the Safe Reference Map to use for data profile object Safe References.
    FsFileRefMap = le_ref_CreateMap("FsFileRefMap", FS_MAX_FILE_REF);
}
