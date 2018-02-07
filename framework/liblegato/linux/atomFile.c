 //--------------------------------------------------------------------------------------------------
/** @file atomic.c
 *
 * Implementation of APIs for atomic file access operations.
 *
 * Atomic change of file means changing its contents in a way that power-cut or any unclean activity
 * could not lead to any corruption or inconsistency in the file. Following steps shows a reliable
 * way to do this in most of the file-systems (UBIFS, JFFS2, EXT3 etc). For details, please visit:
 * http://www.linux-mtd.infradead.org/faq/ubifs.html#L_atomic_change
 *
 *      1. Make a copy of the file;
 *      2. Change the copy;
 *      3. Synchronize the copy;
 *      4. Synchronize the directory;
 *      5. Rename the copy to the file;
 *
 * As per POSIX requirement, rename operation should be atomic. So any dirty activity (e.g.power-cut)
 * during the re-naming should keep the original file intact. All the aforementioned steps are
 * followed in this API implementation,
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "fileDescriptor.h"
#include "file.h"
#include <sys/file.h>


//--------------------------------------------------------------------------------------------------
/**
 * Extension used for temporary file
 */
//--------------------------------------------------------------------------------------------------
#define TEMP_FILE_EXTENSION       ".bak~~XXXXXX"


//--------------------------------------------------------------------------------------------------
/**
 * Extension used for lock file
 */
//--------------------------------------------------------------------------------------------------
#define LOCK_FILE_EXTENSION       ".lock~~XXXXXX"


//--------------------------------------------------------------------------------------------------
/**
 * Temp directory to use for lock file when directory is not writable
 */
//--------------------------------------------------------------------------------------------------
#define LOCK_FILE_TEMP_DIR        "/tmp/"


//--------------------------------------------------------------------------------------------------
/**
 * Mutex used to protect shared data structures in this module.
 */
//--------------------------------------------------------------------------------------------------
static pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;   // POSIX "Fast" mutex.

/// Locks the mutex.
#define LOCK    LE_ASSERT(pthread_mutex_lock(&Mutex) == 0);

/// Unlocks the mutex.
#define UNLOCK  LE_ASSERT(pthread_mutex_unlock(&Mutex) == 0);


//--------------------------------------------------------------------------------------------------
/**
 * Structure that can store the details of file opened for atomic access. Used to store original
 * file information that is required during file closing.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t   link;                 ///< Used to link into the FileAccessList.
    int tempFd;                           ///< File descriptor of temp file.
    int originFd;                         ///< File descriptor of original file.
    int lockFd;                           ///< File descriptor for lock file.
    char filePath[PATH_MAX];              ///< Original file path
}
FileAccess_t;


//--------------------------------------------------------------------------------------------------
/**
 * Pool to allocate FileAccess_t objects.
 **/
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t FileAccessPool;


//--------------------------------------------------------------------------------------------------
/**
 * Linked list of FileAccess_t objects.
 **/
//--------------------------------------------------------------------------------------------------
static le_dls_List_t FileAccessList = LE_DLS_LIST_INIT;



//--------------------------------------------------------------------------------------------------
/**
 * Search and return FileAccess_t object using file descriptor of temporary file.
 *
 * @return
 *      FileAccess_t object if exists any.
 *      NULL if doesn't exist
 **/
//--------------------------------------------------------------------------------------------------
static FileAccess_t* GetFileData
(
    int fd                 ///< [IN] File descriptor of atomically accessed file.
)
{
    LOCK
    le_dls_Link_t* linkPtr = le_dls_Peek(&FileAccessList);

    while (linkPtr != NULL)
    {
        FileAccess_t* accessPtr = CONTAINER_OF(linkPtr, FileAccess_t, link);

        if ((accessPtr->tempFd == fd) ||
            ((accessPtr->originFd == fd) && (accessPtr->tempFd == -1)))  // This is for READ_ONLY
                                                                         // file check.
        {
            UNLOCK
            return accessPtr;
        }

        linkPtr = le_dls_PeekNext(&FileAccessList, linkPtr);
    }

    UNLOCK

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Store atomically accessed file info to memory
 **/
//--------------------------------------------------------------------------------------------------
static void SaveFileData
(
    int fd,                   ///< File descriptor of atomically accessed file.
    int lockFd,               ///< File descriptor of lock file.
    int tempFd,               ///< File descriptor of temporary file.
    const char* pathNamePtr   ///< Path to atomically accessed file.
)
{
    LOCK

    FileAccess_t* accessPtr = le_mem_ForceAlloc(FileAccessPool);

    accessPtr->link = LE_DLS_LINK_INIT;
    accessPtr->originFd = fd;
    accessPtr->lockFd = lockFd;
    accessPtr->tempFd = tempFd;
    LE_ASSERT_OK(le_utf8_Copy(accessPtr->filePath, pathNamePtr, sizeof(accessPtr->filePath), NULL));
    le_dls_Queue(&FileAccessList, &accessPtr->link);

    UNLOCK
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete atomically accessed file data from memory
 **/
//--------------------------------------------------------------------------------------------------
static void DeleteFileData
(
    FileAccess_t* accessPtr  ///< File access data to be deleted.
)
{
    LOCK

    le_dls_Remove(&FileAccessList, &accessPtr->link);
    le_mem_Release(accessPtr);

    UNLOCK
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a file at a given path.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if failed,
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DeleteFile
(
    const char* filePath  ///< [IN] Path to the file to delete.
)
//--------------------------------------------------------------------------------------------------
{
    if ((unlink(filePath) != 0) && (errno != ENOENT))
    {
        LE_CRIT("Failed to delete file '%s' (%m).", filePath);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get file path with file extension appended. If the directory is not writable, the lock is created
 * into /tmp instead and in path name '/' are replaced by '.'.
 **/
//--------------------------------------------------------------------------------------------------
static void GetFilePath
(
    const char* originFilePath,     ///< [IN] Path of original file.
    const char* fileExtension,      ///< [IN] Extension that should be appended with file path
    char* outFilePath,              ///< [OUT] Path of appended file extension
    size_t filePathSize             ///< [IN] File path length
)
{
    char basePath[PATH_MAX];
    bool isDirWritable = false;
    bool isLockFile = (0 == strcmp(fileExtension, LOCK_FILE_EXTENSION) ? true : false);

    LE_ASSERT(le_path_GetDir(originFilePath, "/", basePath, sizeof(basePath)) == LE_OK);

    if (le_dir_IsDir(basePath))
    {
        // Directory is not writable and the file is a lock
        if (access(basePath, W_OK) && isLockFile)
        {
            LE_ASSERT(snprintf(outFilePath, filePathSize,
                               LOCK_FILE_TEMP_DIR "%s%s", originFilePath, fileExtension)
                          < filePathSize);
        }
        else
        {
            LE_ASSERT(snprintf(outFilePath, filePathSize,
                               "%s%s", originFilePath, fileExtension)
                          < filePathSize);
            isDirWritable = true;
        }
    }
    else
    {
        // Origin path only file name., i.e. something like "fileName.extension". Consider it is
        // in current directory, so add "./" in path
        if (access(".", W_OK) && isLockFile)
        {
            LE_ASSERT(snprintf(outFilePath, filePathSize,
                               LOCK_FILE_TEMP_DIR "./%s%s", originFilePath, fileExtension)
                          < filePathSize);
        }
        else
        {
            LE_ASSERT(snprintf(outFilePath, filePathSize,
                               "./%s%s", originFilePath, fileExtension)
                          < filePathSize);
            isDirWritable = true;
        }
    }

    if (!isDirWritable && isLockFile)
    {
        // If directory is not writable, opening a file with lock even in R/O will fail due to
        // lock path name. So we replace the lock path with a file into /tmp directory with the
        // '/' replaced by '.'.
        int i;
        // Replace '/' after "/tmp/" by '.' as the directory is not writable.
        for( i = strlen(LOCK_FILE_TEMP_DIR); outFilePath[i]; i++ )
        {
            if( '/' == outFilePath[i] )
            {
                 outFilePath[i] = '.';
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks the file at given file system path.
 *
 * @return
 *      LE_OK if the file exists and is a normal file.
 *      LE_NOT_FOUND if file does not exists.
 *      LE_FAULT for any other error.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CheckIfRegFileExist
(
    const char* filePath    ///< [IN] Path to the file in question.
)
{
    struct stat fileStatus;

    // Use stat(2) to check for existence of the file.
    if (stat(filePath, &fileStatus) != 0)
    {
        // ENOENT = the file doesn't exist.  Anything else warrants and error report.
        if (errno != ENOENT)
        {
            LE_CRIT("Error when trying to stat '%s'. (%m)", filePath);
            return LE_FAULT;
        }

        return LE_NOT_FOUND;
    }
    else
    {
        // Something exists. Make sure it's a file.
        // NOTE: stat() follows symlinks.
        if (S_ISREG(fileStatus.st_mode))
        {
            return LE_OK;
        }
        else
        {
            LE_CRIT("Unexpected file system object type (%#o) at path '%s'.",
                    fileStatus.st_mode & S_IFMT,
                    filePath);

            return LE_FAULT;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Open lock file for the file which will do atomic operation. If there is no lock file, this
 * function will create and open lock file
 *
 * @return
 *      A file descriptor for doing atomic operation.
 *      LE_WOULD_BLOCK if there is already an incompatible lock on the file.
 *      LE_FAULT if there was an error.
 **/
//--------------------------------------------------------------------------------------------------
static int OpenLockFile
(
    const char* pathNamePtr,             ///< [IN] Path of the file for which lockfile should be open
    le_flock_AccessMode_t accessMode,    ///< [IN] The access mode to open the file with.
    bool blocking                        ///< [IN] true if blocking, false if non-blocking.
)
{
    char lockFilePath[PATH_MAX];
    GetFilePath(pathNamePtr, LOCK_FILE_EXTENSION, lockFilePath, sizeof(lockFilePath));

    int lockFd;

    if (blocking)
    {
        lockFd = le_flock_Create(lockFilePath,
                                 accessMode,
                                 LE_FLOCK_OPEN_IF_EXIST,
                                 S_IRUSR | S_IWUSR);
    }
    else
    {
        lockFd = le_flock_TryCreate(lockFilePath,
                                    accessMode,
                                    LE_FLOCK_OPEN_IF_EXIST,
                                    S_IRUSR | S_IWUSR);
    }

    return lockFd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates temporary file for doing all intermediate operations. This function is used to create
 * temporary file when original file exists. File permissions are copied from the original file.
 *
 * @return
 *      A file descriptor for doing atomic operation.
 *      LE_NOT_FOUND if the file does not exist.
 *      LE_FAULT if there was an error.
 **/
//--------------------------------------------------------------------------------------------------
static int CreateTempFromOriginal
(
    const char* origPathPtr,             ///< [IN] Path to original file.
    const char* tempPathPtr,             ///< [IN] Path to temporary file.
    le_flock_AccessMode_t accessMode,    ///< [IN] The access mode to open the file with.
    bool copy                            ///< [IN] Whether content of original file should be copied
                                         ///<      to temporary file.
)
{
    // Delete the temporary file if exists.
    unlink(tempPathPtr);

    // File permission mode in temporary file should be same as original file, so set the
    // processor umask to 0.
    mode_t old_mode = umask((mode_t)0);
    int tempfd;

    if (copy)
    {
        // Copy the contents to temporary file.
        if (file_Copy(origPathPtr, tempPathPtr, NULL) == LE_OK)
        {
            // Temp file already exists. So opening should be fine.
            tempfd = le_flock_Open(tempPathPtr, accessMode);
        }
        else
        {
            tempfd = LE_FAULT;
        }
    }
    else
    {
        // Temp file doesn't exist, so create it with original file permission
        struct stat fileStatus;

        // Get the original file permission mode and create a temp file with same permission mode.
        if (stat(origPathPtr, &fileStatus) == 0)
        {
            tempfd = le_flock_Create(tempPathPtr,
                                     accessMode,
                                     LE_FLOCK_REPLACE_IF_EXIST,
                                     fileStatus.st_mode);
        }
        else
        {
            LE_CRIT("Error when trying to stat '%s'. (%m)", origPathPtr);
            tempfd = LE_FAULT;
        }
    }

    umask(old_mode);

    return tempfd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates temporary file stream for doing all intermediate operations. This function is used to
 * create temporary file when original file exists. File permissions are copied from the original
 * file.
 *
 * If there was an error NULL is returned and resultPtr is set to:
 *      LE_NOT_FOUND if file doesn't exists.
 *      LE_FAULT if there was an error.
 *
 * @return
 *      Buffered file stream handle to the file if successful.
 *      NULL if there was an error.
 **/
//--------------------------------------------------------------------------------------------------
static FILE* CreateTempStreamFromOriginal
(
    const char* origPathPtr,             ///< [IN] Path to original file.
    const char* tempPathPtr,             ///< [IN] Path to temporary file.
    le_flock_AccessMode_t accessMode,    ///< [IN] The access mode to open the file with.
    bool copy,                           ///< [IN] Whether content of original file should be copied
                                         ///<      to temporary file.
    le_result_t* resultPtr               ///< [OUT] A pointer to result code
)
{
    // Delete the temporary file if exists.
    unlink(tempPathPtr);

    // File permission mode in temporary file should be same as original file, so set the
    // processor umask to 0.
    mode_t old_mode = umask((mode_t)0);
    FILE* file;

    if (copy)
    {
        // Copy the contents to temporary file.
        if (file_Copy(origPathPtr, tempPathPtr, NULL) == LE_OK)
        {
            // Temp file already exists. So opening should be fine.
            file = le_flock_OpenStream(tempPathPtr, accessMode, resultPtr);
        }
        else
        {
            if (resultPtr != NULL)
            {
                *resultPtr = LE_FAULT;
            }
            file = NULL;
        }
    }
    else
    {
        // Temp file doesn't exist, so create it with original file permission
        struct stat fileStatus;

        // Get the original file permission mode and create a temp file with same permission mode.
        if (stat(origPathPtr, &fileStatus) == 0)
        {
            file = le_flock_CreateStream(tempPathPtr,
                                         accessMode,
                                         LE_FLOCK_REPLACE_IF_EXIST,
                                         fileStatus.st_mode,
                                         resultPtr);
        }
        else
        {
            LE_CRIT("Error when trying to stat '%s'. (%m)", origPathPtr);

            if (resultPtr != NULL)
            {
                *resultPtr = LE_FAULT;
            }
            file = NULL;
        }
    }

    umask(old_mode);

    return file;
}


//--------------------------------------------------------------------------------------------------
/**
 * Open an existing file for atomic operation.
 *
 * @return
 *      A file descriptor for doing atomic operation.
 *      LE_NOT_FOUND if the file does not exist.
 *      LE_WOULD_BLOCK if there is already an incompatible lock on the file.
 *      LE_FAULT if there was an error.
 **/
//--------------------------------------------------------------------------------------------------
static int Open
(
    const char* pathNamePtr,             ///< [IN] Path of the file to open.
    le_flock_AccessMode_t accessMode,    ///< [IN] The access mode to open the file with.
    bool blocking                        ///< [IN] true if blocking, false if non-blocking.
)
{
    LE_ASSERT(pathNamePtr != NULL);
    LE_ASSERT(pathNamePtr[0] != '\0');

    // High level algorithm:
    // if (Read-Only access requested)
    //     1. Lock the lockfile
    //     2. Lock the file and return the file descriptor
    // else
    //     1. Lock the lockfile.
    //     2. Lock the original file.
    //     3. Create a temp copy of the file and lock that temp copy
    //     4. Open the temp copy and return the file descriptor.

    // Open(or lock) the lockfile.
    int lockFd = OpenLockFile(pathNamePtr, accessMode, blocking);

    if (lockFd < 0)
    {
        return lockFd;
    }

    if (accessMode == LE_FLOCK_READ)
    {
        // Note: Even for read access we need to put lock the lockfile. This should be done to avoid
        // a race condition (e.g. Process A requests write access to an existing file named abc.txt
        // and it's access request is granted, now process B requests read access to abc.txt, so
        // process B opens abc.txt but it is blocked when it tries to acquire read lock. Now when
        // process A commits all its changes, it renames the temporary file to abc.txt, hence
        // process B is pointing to wrong inode)

        // We need to consider blocking here as well, otherwise this api call will be blocked if
        // file is already opened by using le_flock api.
        int fd = blocking ? le_flock_Open(pathNamePtr, accessMode) :
                            le_flock_TryOpen(pathNamePtr, accessMode);

        if (fd < 0)
        {
            le_flock_Close(lockFd);
            return fd;
        }

        // Store info about this file in the File Access List.
        SaveFileData(fd, lockFd, -1, pathNamePtr);

        return fd;
    }
    else
    {
        // We need to consider blocking here as well, otherwise this api call will be blocked if
        // file is already opened by using le_flock api.
        int fd = blocking ? le_flock_Open(pathNamePtr, accessMode):
                            le_flock_TryOpen(pathNamePtr, accessMode);

        if (fd < 0)
        {
            le_flock_Close(lockFd);
            return fd;
        }

        char tempFilePath[PATH_MAX];
        GetFilePath(pathNamePtr, TEMP_FILE_EXTENSION, tempFilePath, sizeof(tempFilePath));

        // Now open the temporary file.
        int tempfd = CreateTempFromOriginal(pathNamePtr,
                                            tempFilePath,
                                            accessMode,
                                            true);

        if (tempfd < 0)
        {
           le_flock_Close(fd);
           le_flock_Close(lockFd);
           return tempfd;
        }

        // Store info about this file in the File Access List.
        SaveFileData(fd, lockFd, tempfd, pathNamePtr);

        return tempfd;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a file for atomic operation.
 *
 * @return
 *      File descriptor for doing atomic operation.
 *      LE_DUPLICATE if the file already exists and LE_FLOCK_FAIL_IF_EXIST is specified in createMode
 *      LE_WOULD_BLOCK if there is already an incompatible lock on the file.
 *      LE_FAULT if there was an error.
 **/
//--------------------------------------------------------------------------------------------------
static int Create
(
    const char* pathNamePtr,            ///< [IN] Path of the file to open.
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    le_flock_CreateMode_t createMode,   ///< [IN] The action to take if the file already exists.
    mode_t permissions,                 ///< [IN] The file permissions used when creating the file.
    bool blocking                       ///< [IN] true if blocking, false if non-blocking.
)
{
    LE_ASSERT(pathNamePtr != NULL);
    LE_ASSERT(pathNamePtr[0] != '\0');

    // High level algorithm:
    //
    // if (file exists)
    //     if (Read-Only access requested)
    //         1. Lock the lockfile
    //         2. Lock the file and return the file descriptor
    //     else
    //         1. Lock the lockfile.
    //         2. Lock the original file.
    //         3. Create a temp copy of the file and lock that temp copy
    //         4. Open the temp copy and return the file descriptor.
    //  else
    //     1. Lock the lockfile.
    //     2. Create a temp copy and lock that temp copy
    //     3. Open the temp copy and return the file descriptor.

    char tempFilePath[PATH_MAX];
    GetFilePath(pathNamePtr, TEMP_FILE_EXTENSION, tempFilePath, sizeof(tempFilePath));

    int lockFd = OpenLockFile(pathNamePtr, accessMode, blocking);

    if (lockFd < 0)
    {
        return lockFd;
    }

    // Check whether pathNamePtr points to an existent regular file.
    le_result_t fileExistResult = CheckIfRegFileExist(pathNamePtr);

    // Something error happened, so return immediately
    if (fileExistResult == LE_FAULT)
    {
        // No need to print any error message as it was done in CheckRegFile() function.
        le_flock_Close(lockFd);
        return LE_FAULT;
    }

    int fd = -1;
    int tempfd = -1;
    bool copy;

    if ((accessMode == LE_FLOCK_READ) && (fileExistResult == LE_OK))
    {
        // We need to consider blocking here as well, otherwise this api call will be
        // blocked if file is already opened by using le_flock api
        fd = blocking ? le_flock_Create(pathNamePtr, accessMode, createMode, permissions) :
                        le_flock_TryCreate(pathNamePtr, accessMode, createMode, permissions);
        if (fd < 0)
        {
            le_flock_Close(lockFd);
            return fd;
        }

        // Store info about this file in the File Access List.
        SaveFileData(fd, lockFd, -1, pathNamePtr);

        return fd;
    }

    if (fileExistResult == LE_OK)
    {
        switch(createMode)
        {
            case LE_FLOCK_OPEN_IF_EXIST:
            case LE_FLOCK_REPLACE_IF_EXIST:
                // We need to consider blocking here as well, otherwise this api call will be
                // blocked if file is already opened by using le_flock api.
                fd = blocking ? le_flock_Open(pathNamePtr, accessMode) :
                                le_flock_TryOpen(pathNamePtr, accessMode);

                if (fd < 0)
                {
                    le_flock_Close(lockFd);
                    return fd;
                }

                copy = (createMode == LE_FLOCK_OPEN_IF_EXIST);   // Copy if LE_FLOCK_OPEN_IF_EXIST
                                                                 // specified.
                // Now open and lock the temporary file.
                tempfd = CreateTempFromOriginal(pathNamePtr,
                                                tempFilePath,
                                                accessMode,
                                                copy);
                break;

            case LE_FLOCK_FAIL_IF_EXIST:
                le_flock_Close(lockFd);
                return LE_DUPLICATE;
        }
    }
    else          // File doesn't exist
    {
        fd = -1;
        // Unlink the temp file, this is needed to avoid a bug (old temp file exists and creation
        // of new file requested with different permission mode. In this case, new permission mode
        // will be discarded)
        unlink(tempFilePath);

        // No need to use TryCreate as we already opened (i.e. locked) the lockfile
        tempfd = le_flock_Create(tempFilePath,
                                 accessMode,
                                 LE_FLOCK_REPLACE_IF_EXIST,
                                 permissions);
    }


    if (tempfd < 0)
    {
        if (fd > -1)
        {
            le_flock_Close(fd);
        }
        le_flock_Close(lockFd);
        return tempfd;
    }

    // Store info about this file in the File Access List.
    SaveFileData(fd, lockFd, tempfd, pathNamePtr);

    return tempfd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sync files to disk
 *
 * @return
 *      LE_OK if successful
 *      LE_FAULT if failed.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SyncFile
(
    FileAccess_t* accessPtr,            ///< [IN] Object containing files to be Sync-ed
    const char* tempFilePath            ///< [IN] Path to temporary file.
)
{
    // Do a fsync to ensure write to temporary file goes to storage device.
     if (fsync(accessPtr->tempFd) == -1)
     {
         LE_CRIT("Failed to do fsync on file '%s' (%m).", tempFilePath);
         return LE_FAULT;
     }

     char dirName[PATH_MAX];

     // Get containing directory
     LE_ASSERT_OK(le_path_GetDir(accessPtr->filePath, "/", dirName, sizeof(dirName)));

     // le_path_GetDir returns file name when no path is specified.
     if (!le_dir_IsDir(dirName))
     {
         dirName[0] = '.';
         dirName[1] = 0;
     }

     int dirFd;
     do
     {
         // Directory can be opened with read-only flag
         dirFd = open(dirName, O_RDONLY);
     }
     while ( (dirFd == -1) && (errno == EINTR) );

     if (dirFd == -1)
     {
         LE_CRIT("Failed to open directory '%s' (%m).", dirName);
         return LE_FAULT;
     }

     // Now do a sync on directory
     if (fsync(dirFd) == -1)
     {
         LE_CRIT("Failed to do fsync on directory: '%s' (%m).", dirName);
         fd_Close(dirFd);
         return LE_FAULT;
     }

     fd_Close(dirFd);

     if (rename(tempFilePath, accessPtr->filePath))
     {
         LE_CRIT("Failed rename '%s' to '%s' (%m).", tempFilePath, accessPtr->filePath);
         return LE_FAULT;
     }

     return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Commit or cancel all changes done on the file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Close
(
    int fd,                   ///< [IN] File descriptor to close.
    bool commit               ///< [IN] true to commit, false to cancel changes.
)
{
    LE_ASSERT(fd > -1);

    // High level algorithm:
    // if (file opened with Read-Only access )
    //     1. Close the file and release resources
    // else
    //     if (Commit requested)
    //        1. Sync the temp copy
    //        2. Sync the containing directory
    //        3. Rename the temp copy to appropriate name
    //        4. Close files and release resources
    //     else   // Cancel requested
    //        1. Delete the temp copy
    //        2. Close files and release resources

    // Find out info about this file descriptor.
    FileAccess_t* accessPtr = GetFileData(fd);

    // Coding bug. So terminate immediately.
    LE_FATAL_IF(accessPtr == NULL, "Bad file descriptor: %d", fd);

    le_result_t result = LE_OK;

    if ((accessPtr->originFd == fd) &&
        (accessPtr->tempFd < 0))
    {
        le_flock_Close(fd);
        le_flock_Close(accessPtr->lockFd);
    }
    else
    {
        char tempFilePath[PATH_MAX];
        GetFilePath(accessPtr->filePath, TEMP_FILE_EXTENSION, tempFilePath, sizeof(tempFilePath));

        if (commit)  // Commit all the necessary changes.
        {
            result = SyncFile(accessPtr, tempFilePath);
        }
        else  // Cancel all changes, i.e. delete the temporary file
        {
            // Following function unlinks the file. Unlink is ok while file descriptor is open. File
            // will be deleted when file descriptor will be closed.
            result = DeleteFile(tempFilePath);
        }

        // Now close temp and original file descriptor.
        le_flock_Close(fd);

        if (accessPtr->originFd > -1)
        {
            le_flock_Close(accessPtr->originFd);
        }

        le_flock_Close(accessPtr->lockFd);
    }

    // Release memory.
    DeleteFileData(accessPtr);

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Atomically and safely deletes a file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if file doesn't exists.
 *      LE_WOULD_BLOCK if file is already locked (i.e. someone is using it).
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Delete
(
    const char* pathNamePtr,            ///< [IN] Path of the file to delete.
    bool blocking                       ///< [IN] True if blocking, false if non-blocking.
)
{
    // High level algorithm:
    //    1. Lock the lockfile.
    //    2. Lock the original file.
    //    3. Rename original file to a temporary file
    //    4. Unlink the temporary file and unlock lockfile.

    int lockFd = OpenLockFile(pathNamePtr, LE_FLOCK_APPEND, blocking);

    if (lockFd < 0)
    {
        return lockFd;
    }

    // This locking is needed to avoid some unexpected race condition (e.g. process A opens a file
    // using le_flock_Open() api and starts writing on this file. Process B calls
    // le_atomFile_Delete() function to delete the same file. If no locking mechanism is used here,
    // process B will delete the file while process A is writing on this file.)
    int fd = blocking ? le_flock_Open(pathNamePtr, LE_FLOCK_WRITE) :
                        le_flock_TryOpen(pathNamePtr, LE_FLOCK_WRITE);

    if (fd < 0)
    {
        le_flock_Close(lockFd);
        return fd;
    }

    char lockFilePath[PATH_MAX];
    char tempFilePath[PATH_MAX];
    GetFilePath(pathNamePtr, LOCK_FILE_EXTENSION, lockFilePath, sizeof(lockFilePath));
    GetFilePath(pathNamePtr, TEMP_FILE_EXTENSION, tempFilePath, sizeof(tempFilePath));

    if (rename(pathNamePtr, tempFilePath) == -1)
    {
        LE_CRIT("Failed rename '%s' to '%s' (%m).", pathNamePtr, tempFilePath);
        le_flock_Close(fd);
        le_flock_Close(lockFd);
        return LE_FAULT;
    }

    DeleteFile(tempFilePath);

    le_flock_Close(fd);

    // Note: Don't unlink the lockfile, it may lead to race condition (e.g. process B opens lockfile
    // but can't lock as process A is on the way to delete lockfile, when process A closes lockfile
    // descriptor, process B can lock the lockfile, now if process C checks lockfile it won't find
    // any lockfile and can create and lock the lockfile).
    // Size of lockfile is zero, so it won't matter much.
    le_flock_Close(lockFd);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Opens an existing file for atomic access operation.
 *
 * The file can be open for reading, writing or both as specified in the accessMode argument.
 * Parameter accessMode specifies the lock to be applied on the file (read lock will be applied for
 * LE_FLOCK_READ and write lock will be placed for all other cases).
 *
 * This is a blocking call. It will block until it can open the target file with specified
 * accessMode.
 *
 * @return
 *      A file descriptor if successful.
 *      LE_NOT_FOUND if the file does not exist.
 *      LE_FAULT if there was an error.
 *
 * @note
 *     File must be closed using le_atomFile_Close() or le_atomFile_Cancel() function.
 */
//--------------------------------------------------------------------------------------------------
int le_atomFile_Open
(
    const char* pathNamePtr,            ///< [IN] Path of the file to open
    le_flock_AccessMode_t accessMode    ///< [IN] The access mode to open the file with.
)
{

    return Open(pathNamePtr, accessMode, true);

}


//--------------------------------------------------------------------------------------------------
/**
 * Creates and opens file for atomic operation.
 *
 * If the file does not exist it will be created with the file permissions specified in the argument
 * permissions (modified by the process's umask).  Refer to the POSIX function open(2) for details
 * of mode_t:
 *
 * http://man7.org/linux/man-pages/man2/open.2.html
 *
 * The file can be opened for reading, writing or both as specified in the accessMode argument.
 * Parameter accessMode specifies the lock to be applied on the file (read lock will be applied for
 * LE_FLOCK_READ and write lock will be placed for all other cases).
 *
 * This is a blocking call. It will block until it can create and open the target file with specified
 * parameters(i.e. accessMode, createMode, permissions).
 *
 * @return
 *      A file descriptor if successful.
 *      LE_DUPLICATE if the file already exists and LE_FLOCK_FAIL_IF_EXIST is specified in createMode
 *      LE_FAULT if there was an error.
 *
 * @note
 *     File must be closed using le_atomFile_Close() or le_atomFile_Cancel() function.
 */
//--------------------------------------------------------------------------------------------------
int le_atomFile_Create
(
    const char* pathNamePtr,            ///< [IN] Path of the file to open
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    le_flock_CreateMode_t createMode,   ///< [IN] The action to take if the file already exists.
    mode_t permissions                  ///< [IN] The file permissions used when creating the file.
                                        ///       See the function header comments for more details.
)
{
    return Create(pathNamePtr, accessMode, createMode, permissions, true);
}


//--------------------------------------------------------------------------------------------------
/**
 * Same as @c le_atomFile_Open() except that it is non-blocking function and it will fail and return
 * LE_WOULD_BLOCK immediately if target file has incompatible lock.
 *
 * @return
 *      A file descriptor if successful.
 *      LE_NOT_FOUND if the file does not exist.
 *      LE_WOULD_BLOCK if there is already an incompatible lock on the file.
 *      LE_FAULT if there was an error.
 *
 * @note
 *     File must be closed using le_atomFile_Close() or le_atomFile_Cancel() function.
 */
//--------------------------------------------------------------------------------------------------
int le_atomFile_TryOpen
(
    const char* pathNamePtr,            ///< [IN] Path of the file to open
    le_flock_AccessMode_t accessMode    ///< [IN] The access mode to open the file with.
)
{
    return Open(pathNamePtr, accessMode, false);
}


//--------------------------------------------------------------------------------------------------
/**
 * Same as @c le_atomFile_Create() except that it is non-blocking function and  it will fail and
 * return LE_WOULD_BLOCK immediately if target file has incompatible lock.
 *
 * @return
 *      A file descriptor if successful.
 *      LE_DUPLICATE if the file already exists and LE_FLOCK_FAIL_IF_EXIST is specified in createMode
 *      LE_WOULD_BLOCK if there is already an incompatible lock on the file.
 *      LE_FAULT if there was an error.
 *
 * @note
 *     File must be closed using le_atomFile_Close() or le_atomFile_Cancel() function.
 */
//--------------------------------------------------------------------------------------------------
int le_atomFile_TryCreate
(
    const char* pathNamePtr,            ///< [IN] Path of the file to open
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    le_flock_CreateMode_t createMode,   ///< [IN] The action to take if the file already exists.
    mode_t permissions                  ///< [IN] The file permissions used when creating the file.
)
{
    return Create(pathNamePtr, accessMode, createMode, permissions, false);
}


//--------------------------------------------------------------------------------------------------
/**
 * Cancels all changes and closes the file descriptor.
 */
//--------------------------------------------------------------------------------------------------
void le_atomFile_Cancel
(
    int fd                              /// [IN] The file descriptor to close.
)
{
    Close(fd, false);
}


//--------------------------------------------------------------------------------------------------
/**
 * Commits all changes and closes the file descriptor. No need to close the file descriptor again if
 * this function returns error (i.e. file descriptor is closed in both success and error scenario).
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atomFile_Close
(
    int fd                              /// [IN] The file descriptor to close.
)
{
    return Close(fd, true);
}


//--------------------------------------------------------------------------------------------------
/**
 * Open a stream for atomic operation.
 *
 * If there was an error NULL is returned and resultPtr is set to:
 *   - LE_NOT_FOUND if the file does not exist.
 *   - LE_WOULD_BLOCK if there is already an incompatible lock on the file.
 *   - LE_FAULT if there was an error.
 *
 * @return
 *      Buffered file stream handle to the file if successful.
 *      NULL if there was an error.
 **/
//--------------------------------------------------------------------------------------------------
static FILE* OpenStream
(
    const char* pathNamePtr,            ///< [IN] Path of the file to open.
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    bool blocking,                      ///< [IN] true if blocking, false if non-blocking.
    le_result_t* resultPtr              ///< [OUT] A pointer to result code.
)
{
    LE_ASSERT(pathNamePtr != NULL);
    LE_ASSERT(pathNamePtr[0] != '\0');

    // High level algorithm:
    // if (Read-Only access requested)
    //     1. Lock the lockfile
    //     2. Lock the file and return the file stream
    // else
    //     1. Lock the lockfile
    //     2. Lock the original file
    //     3. Create a temp copy of the file and lock that temp copy
    //     4. Open the temp copy and return the file stream

    int lockFd = OpenLockFile(pathNamePtr, accessMode, blocking);

    if (lockFd < 0)
    {
        if (resultPtr != NULL)
        {
            *resultPtr = lockFd;
        }
        return NULL;
    }

    if (accessMode == LE_FLOCK_READ)
    {
        // We need to consider blocking here as well, otherwise this api call will be blocked if
        // file is already opened by using le_flock api.
        FILE* file = blocking ? le_flock_OpenStream(pathNamePtr, accessMode, resultPtr) :
                                le_flock_TryOpenStream(pathNamePtr, accessMode, resultPtr);

        if (file == NULL)
        {
            le_flock_Close(lockFd);
            return file;
        }

        // Store info about this file in the File Access List.
        SaveFileData(fileno(file), lockFd, -1, pathNamePtr);

        return file;
    }
    else
    {
        // We need to consider blocking here as well, otherwise this api call will be blocked if
        // file is already opened by using le_flock api.
        int fd = blocking ? le_flock_Open(pathNamePtr, accessMode) :
                            le_flock_TryOpen(pathNamePtr, accessMode);

        if (fd < 0)
        {
            if (resultPtr != NULL)
            {
                *resultPtr = fd;
            }
            le_flock_Close(lockFd);
            return NULL;
        }

        char tempFilePath[PATH_MAX];
        GetFilePath(pathNamePtr, TEMP_FILE_EXTENSION, tempFilePath, sizeof(tempFilePath));

        // Now open and lock the temporary file.
        FILE* file = CreateTempStreamFromOriginal(pathNamePtr,
                                                  tempFilePath,
                                                  accessMode,
                                                  true,        // Copy the content of original file
                                                  resultPtr);

        if (file == NULL)
        {
            le_flock_Close(fd);
            le_flock_Close(lockFd);
            return NULL;
        }

        // Store info about this file in the File Access List.
        SaveFileData(fd, lockFd, fileno(file), pathNamePtr);

        return file;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a file stream for atomic operation.
 *
 * If there was an error NULL is returned and resultPtr is set to:
 *  - LE_DUPLICATE if the file already exists and LE_FLOCK_FAIL_IF_EXIST is specified in createMode.
 *  - LE_WOULD_BLOCK if there is already an incompatible lock on the file.
 *  - LE_FAULT if there was an error.
 *
 * @return
 *      Buffered file stream handle to the file if successful.
 *      NULL if there was an error.
 **/
//--------------------------------------------------------------------------------------------------
static FILE* CreateStream
(
    const char* pathNamePtr,            ///< [IN] Path of the file to open.
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    le_flock_CreateMode_t createMode,   ///< [IN] The action to take if the file already exists.
    mode_t permissions,                 ///< [IN] The file permissions used when creating the file.
    bool blocking,                      ///< [IN] true if blocking, false if non-blocking.
    le_result_t* resultPtr              ///< [OUT] A pointer to result code
)
{
    LE_ASSERT(pathNamePtr != NULL);
    LE_ASSERT(pathNamePtr[0] != '\0');


    // High level algorithm:
    //
    // if (file exists)
    //     if (Read-Only access requested)
    //         1. Lock the lockfile
    //         2. Lock the file and return the file stream.
    //     else
    //         1. Lock the lockfile.
    //         2. Lock the original file.
    //         3. Create a temp copy of the file and lock that temp copy
    //         4. Open the temp copy and return the file stream.
    //  else
    //     1. Lock the lockfile.
    //     2. Create a temp copy and lock that temp copy
    //     3. Open the temp copy and return the file stream.

    char tempFilePath[PATH_MAX];
    GetFilePath(pathNamePtr, TEMP_FILE_EXTENSION, tempFilePath, sizeof(tempFilePath));

    int lockFd = OpenLockFile(pathNamePtr, accessMode, blocking);

    if (lockFd < 0)
    {
        if (resultPtr != NULL)
        {
            *resultPtr = lockFd;
        }
        return NULL;
    }

    FILE* file = NULL;
    int fd = -1;
    bool copy;

    // Check whether pathNamePtr points to an existent regular file. This one has to be done after
    // lock is acquired to avoid race condition (e.g. process A checks existence of file abc.txt
    // then block on locking lockfile, where as in the mean time process B rename/delete abc.txt.
    // In this case process A has old stale information)
    le_result_t fileExistResult = CheckIfRegFileExist(pathNamePtr);

    // Something error happened, so return immediately
    if (fileExistResult == LE_FAULT)
    {
        // No need to print any error message as it was done in CheckIfRegFileExist() function.
        if (resultPtr != NULL)
        {
            *resultPtr = LE_FAULT;
        }

        le_flock_Close(lockFd);
        return NULL;
    }

    if ((accessMode == LE_FLOCK_READ) && (fileExistResult == LE_OK))
    {
        // We need to consider blocking here as well, otherwise this api call will be
        // blocked if file is already opened by using le_flock api
        file = blocking ? le_flock_CreateStream(pathNamePtr,
                                                accessMode,
                                                createMode,
                                                permissions,
                                                resultPtr) :
                          le_flock_TryCreateStream(pathNamePtr,
                                                   accessMode,
                                                   createMode,
                                                   permissions,
                                                   resultPtr);
        if (file == NULL)
        {
            le_flock_Close(lockFd);
            return file;
        }

        // Store info about this file in the File Access List.
        SaveFileData(fileno(file), lockFd, -1, pathNamePtr);

        return file;
    }

    if (fileExistResult == LE_OK)
    {
        switch(createMode)
        {
            case LE_FLOCK_OPEN_IF_EXIST:
            case LE_FLOCK_REPLACE_IF_EXIST:
                // We need to consider blocking here as well, otherwise this api call will be blocked if
                // file is already opened by using le_flock api.
                fd = blocking ? le_flock_Open(pathNamePtr, accessMode) :
                                le_flock_TryOpen(pathNamePtr, accessMode);

                if (fd < 0)
                {
                    if (resultPtr != NULL)
                    {
                        *resultPtr = fd;
                    }

                    le_flock_Close(lockFd);
                    return NULL;
                }

                copy = (createMode == LE_FLOCK_OPEN_IF_EXIST);   // Copy if LE_FLOCK_OPEN_IF_EXIST
                                                                 // specified.
                // Now open and lock the temporary file.
                file = CreateTempStreamFromOriginal(pathNamePtr,
                                                    tempFilePath,
                                                    accessMode,
                                                    copy,
                                                    resultPtr);
                break;

            case LE_FLOCK_FAIL_IF_EXIST:

                if (resultPtr != NULL)
                {
                    *resultPtr = LE_DUPLICATE;
                }
                le_flock_Close(lockFd);

                return NULL;
        }
    }
    else          // File doesn't exist
    {
        fd = -1;
        // Unlink the temp file, this is needed to avoid a bug (old temp file exists and creation
        // of new file requested with different permission mode. In this case, new permission mode
        // will be discarded)
        unlink(tempFilePath);

        // No need to use TryCreate as it is a temp file and we already locked the lockfile.
        file =  le_flock_CreateStream(tempFilePath,
                                      accessMode,
                                      LE_FLOCK_REPLACE_IF_EXIST,
                                      permissions,
                                      resultPtr);
    }

    if (file == NULL)
    {
        if (fd > -1)
        {
            le_flock_Close(fd);
        }
        le_flock_Close(lockFd);
        return NULL;
    }

    // Store info about this file in the File Access List.
    SaveFileData(fd, lockFd, fileno(file), pathNamePtr);

    return file;
}


//--------------------------------------------------------------------------------------------------
/**
 * Commit or cancel all changes done on the stream.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CloseStream
(
    FILE* file,               ///< [IN] File Stream to close
    bool commit               ///< [IN] true to commit, false to cancel changes.
)
{
    LE_ASSERT(file != NULL);

    // High level algorithm:
    // if (file opened with Read-Only access )
    //     1. Close the file and release resources
    // else
    //     if (Commit requested)
    //        1. Sync the temp copy
    //        2. Sync the containing directory
    //        3. Rename the temp copy to appropriate name
    //        4. Close files and release resources
    //     else   // Cancel requested
    //        1. Delete the temp copy
    //        2. Close files and release resources

    int fd = fileno(file);

    LE_ASSERT(fd > -1);

    FileAccess_t* accessPtr = GetFileData(fd);

    // Coding bug. Terminate immediately.
    LE_FATAL_IF(accessPtr == NULL, "Bad file stream: %p", file);

    le_result_t result = LE_OK;

    if ((accessPtr->tempFd < 0) &&  // Negative tempfd implies READ_ONLY access requested.
        (accessPtr->originFd == fd))
    {
        le_flock_CloseStream(file);
        le_flock_Close(accessPtr->lockFd);
    }
    else
    {
        char tempFilePath[PATH_MAX];
        GetFilePath(accessPtr->filePath, TEMP_FILE_EXTENSION, tempFilePath, sizeof(tempFilePath));

        if (commit)  // Commit all the necessary changes.
        {
            // Now flush data to OS.
            int flushResult;
            do
            {
                flushResult = fflush(file);
            }
            while ( (flushResult != 0) && (errno == EINTR) );

            if (flushResult != 0)
            {
                LE_CRIT("Failed to flush file '%s' (%m).", tempFilePath);
                result = LE_FAULT;
            }

            if (result == LE_OK)
            {
                //Now flush data to disk
                result = SyncFile(accessPtr, tempFilePath);
            }
        }
        else   // Discard all the changes
        {
            // Following function unlinks the file. Unlink is ok while file descriptor is open. File
            // will be deleted when file descriptor will be closed.
            result = DeleteFile(tempFilePath);
        }

        // Now close temporary and original file.

        // It is ok to close stream after renaming it as it is in same filesystem.
        le_flock_CloseStream(file);

        if (accessPtr->originFd > -1)
        {
            le_flock_Close(accessPtr->originFd);
        }

        le_flock_Close(accessPtr->lockFd);
    }

    // Release allocated memory
    DeleteFileData(accessPtr);

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Opens an existing file via C standard library buffered file stream for atomic operation.
 *
 * The file can be open for reading, writing or both as specified in the accessMode argument.
 * Parameter accessMode specifies the lock to be applied on the file (read lock will be applied for
 * LE_FLOCK_READ and write lock will be placed for all other cases).
 *
 * This is a blocking call. It will block until it can open the target file with specified
 * accessMode.
 *
 * If there was an error NULL is returned and resultPtr is set to:
 *   - LE_NOT_FOUND if the file does not exist.
 *   - LE_FAULT if there was an error.
 *
 * @return
 *      Buffered file stream handle to the file if successful.
 *      NULL if there was an error.
 *
 * @note
 *     Stream must be closed using le_atomFile_CloseStream() or le_atomFile_CancelStream()
 *     function.
 */
//--------------------------------------------------------------------------------------------------
FILE* le_atomFile_OpenStream
(
    const char* pathNamePtr,            ///< [IN] Path of the file to open
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    le_result_t* resultPtr              ///< [OUT] A pointer to result code.
)
{
    return OpenStream(pathNamePtr, accessMode, true, resultPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates and open a file via C standard library buffered file stream for atomic operation.
 *
 * If the file does not exist it will be created with the file permissions specified in the argument
 * permissions (modified by the process's umask).  Refer to the POSIX function open(2) for details
 * of mode_t:
 *
 * http://man7.org/linux/man-pages/man2/open.2.html
 *
 * The file can be opened for reading, writing or both as specified in the accessMode argument.
 * Parameter accessMode specifies the lock to be applied on the file (read lock will be applied for
 * LE_FLOCK_READ and write lock will be placed for all other cases).
 *
 * This is a blocking call. It will block until it can create and open the target file with
 * specified parameters(i.e. accessMode, createMode, permissions).
 *
 * If there was an error NULL is returned and resultPtr is set to:
 *  - LE_DUPLICATE if the file already exists and LE_FLOCK_FAIL_IF_EXIST is specified in createMode.
 *  - LE_FAULT if there was an error.
 *
 * @return
 *      Buffered file stream handle to the file if successful.
 *      NULL if there was an error.
 *
 * @note
 *     Stream must be closed using le_atomFile_CloseStream() or le_atomFile_CancelStream()
 *     function.
 */
//--------------------------------------------------------------------------------------------------
FILE* le_atomFile_CreateStream
(
    const char* pathNamePtr,            ///< [IN] Path of the file to open
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    le_flock_CreateMode_t createMode,   ///< [IN] The action to take if the file already exists.
    mode_t permissions,                 ///< [IN] The file permissions used when creating the file.
                                        ///       See the function header comments for more details.
    le_result_t* resultPtr              ///< [OUT] A pointer to result code.
)
{
    return CreateStream(pathNamePtr, accessMode, createMode, permissions, true, resultPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Same as @c le_atomFile_OpenStream() except that it is non-blocking function and it will fail and
 * return LE_WOULD_BLOCK immediately if target file has incompatible lock.
 *
 * If there was an error NULL is returned and resultPtr is set to:
 *   - LE_NOT_FOUND if the file does not exist.
 *   - LE_WOULD_BLOCK if there is already an incompatible lock on the file.
 *   - LE_FAULT if there was an error.
 *
 * @return
 *      Buffered file stream handle to the file if successful.
 *      NULL if there was an error.
 *
 * @note
 *     Stream must be closed using le_atomFile_CloseStream() or le_atomFile_CancelStream()
 *     function.
 */
//--------------------------------------------------------------------------------------------------
FILE* le_atomFile_TryOpenStream
(
    const char* pathNamePtr,            ///< [IN] Path of the file to open
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    le_result_t* resultPtr              ///< [OUT] A pointer to result code.
)
{
    return OpenStream(pathNamePtr, accessMode, false, resultPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Same as @c le_atomFile_CreateStream() except that it is non-blocking function and  it will fail
 * and return LE_WOULD_BLOCK immediately if target file has incompatible lock.
 *
 * If there was an error NULL is returned and resultPtr is set to:
 *  - LE_DUPLICATE if the file already exists and LE_FLOCK_FAIL_IF_EXIST is specified in createMode.
 *  - LE_WOULD_BLOCK if there is already an incompatible lock on the file.
 *  - LE_FAULT if there was an error.
 *
 * @return
 *      Buffered file stream handle to the file if successful.
 *      NULL if there was an error.
 *
 * @note
 *     Stream must be closed using le_atomFile_CloseStream() or le_atomFile_CancelStream()
 *     function.
 */
//--------------------------------------------------------------------------------------------------
FILE* le_atomFile_TryCreateStream
(
    const char* pathNamePtr,            ///< [IN] Path of the file to open
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    le_flock_CreateMode_t createMode,   ///< [IN] The action to take if the file already exists.
    mode_t permissions,                 ///< [IN] The file permissions used when creating the file.
    le_result_t* resultPtr              ///< [OUT] A pointer to result code.
)
{
    return CreateStream(pathNamePtr, accessMode, createMode, permissions, false, resultPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Cancels all changes and closes the file stream.
 */
//--------------------------------------------------------------------------------------------------
void le_atomFile_CancelStream
(
    FILE* fileStreamPtr               ///< [IN] File stream pointer to close
)
{
    CloseStream(fileStreamPtr, false);
}


//--------------------------------------------------------------------------------------------------
/**
 * Commits all changes and closes the file stream. No need to close the file stream again if this
 * function returns error (i.e. file stream is closed in both success and error scenario).
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atomFile_CloseStream
(
    FILE* fileStreamPtr             ///< [IN] File stream pointer to close
)
{
    return CloseStream(fileStreamPtr, true);
}


//--------------------------------------------------------------------------------------------------
/**
 * Atomically deletes a file. This function also ensures safe deletion of file (i.e. if any other
 * process/thread is using the file by acquiring file lock, it won't delete the file unless lock is
 * released). This is a blocking call. It will block until lock on file is released.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if file doesn't exists.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atomFile_Delete
(
    const char* pathNamePtr            ///< [IN] Path of the file to delete
)
{
    return Delete(pathNamePtr, true);
}


//--------------------------------------------------------------------------------------------------
/**
 * Same as @c le_atomFile_Delete() except that it is non-blocking function and it will fail and
 * return LE_WOULD_BLOCK immediately if target file is locked.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if file doesn't exists.
 *      LE_WOULD_BLOCK if file is already locked (i.e. someone is using it).
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atomFile_TryDelete
(
    const char* pathNamePtr            ///< [IN] Path of the file to delete
)
{
    return Delete(pathNamePtr, false);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the atomic file access internal memory pools.  This function is meant to be called
 * from Legato's internal init.
 */
//--------------------------------------------------------------------------------------------------
void atomFile_Init
(
    void
)
{
    // Initialize pools
    FileAccessPool = le_mem_CreatePool("AtomicFileAccessPool",
                                        sizeof(FileAccess_t));
}
