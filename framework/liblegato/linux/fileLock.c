 //--------------------------------------------------------------------------------------------------
/** @file fileLock.c
 *
 * @todo Detect deadlocks.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "fileDescriptor.h"
#include <sys/file.h>


//--------------------------------------------------------------------------------------------------
/**
 * Gets the lock type and open flags given the accessMode and nonBlock parameters.
 */
//--------------------------------------------------------------------------------------------------
static void GetFlags
(
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    bool blocking,                      ///< [IN] true if blocking, false if non-blocking.
    int* lockTypePtr,                   ///< [OUT] Pointer to the lock type.
    int* openFlagsPtr                   ///< [OUT] Pointer to the open flags.
)
{
    int lockType;
    int openFlags = 0;

    switch (accessMode)
    {
        case LE_FLOCK_READ:
            openFlags |= O_RDONLY;
            lockType = LOCK_SH;
            break;

        case LE_FLOCK_APPEND:
            openFlags |= O_APPEND;
            // Don't break, add the write flags.

        case LE_FLOCK_WRITE:
            openFlags |= O_WRONLY;
            lockType = LOCK_EX;
            break;

        case LE_FLOCK_READ_AND_APPEND:
            openFlags |= O_APPEND;
            // Don't break, add the write flags.

        case LE_FLOCK_READ_AND_WRITE:
            openFlags |= O_RDWR;
            lockType = LOCK_EX;
            break;

        default:
            LE_FATAL("Unrecognized access mode %d.", accessMode);
    }

    if (!blocking)
    {
        lockType |= LOCK_NB;
    }

    *lockTypePtr = lockType;
    *openFlagsPtr = openFlags;
}


//--------------------------------------------------------------------------------------------------
/**
 * Locks the open file descriptor.
 *
 * @return
 *      The fd file descriptor if successful.
 *      LE_WOULD_BLOCK if blocking is false and there is an incompatible lock on the file.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static int LockFd
(
    int fd,                     ///< [IN] The open file descriptor to lock.
    const char* pathNamePtr,     ///< [IN] Pointer to the path name of the file to open.
    int lockType,               ///< [IN] The lock type to use.
    bool blocking               ///< [IN] true if blocking, false if non-blocking.
)
{
    int r;
    do
    {
        r = flock(fd, lockType);
    }
    while ( (r == -1) && (errno == EINTR) );

    if (r == -1)
    {
        le_flock_Close(fd);

        if ( !blocking && (errno == EWOULDBLOCK) )
        {
            return LE_WOULD_BLOCK;
        }
        else
        {
            LE_ERROR("Could not obtain lock on file '%s'.  %m.", pathNamePtr);
            return LE_FAULT;
        }
    }

    return fd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Opens and locks an existing file.
 *
 * @return
 *      A file descriptor to the file specified in pathNamePtr.
 *      LE_NOT_FOUND if the file does not exist.
 *      LE_WOULD_BLOCK if noblock is true and there is an incompatible lock on the file.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static int Open
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    bool blocking                       ///< [IN] true if blocking, false if non-blocking.
)
{
    // Get the lock type and the open flags based on the access mode.
    int lockType;
    int openFlags;
    GetFlags(accessMode, blocking, &lockType, &openFlags);

    // Open the file.
    int fd;
    do
    {
        fd = open(pathNamePtr, openFlags);
    }
    while ( (fd == -1) && (errno == EINTR) );

    if (fd == -1)
    {
        if (errno == ENOENT)
        {
            return LE_NOT_FOUND;
        }
        else
        {
            LE_WARN("Could not open file '%s'.  %m.", pathNamePtr);
            return LE_FAULT;
        }
    }

    // Lock the file.
    return LockFd(fd, pathNamePtr, lockType, blocking);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates, opens and locks file.
 *
 * @return
 *      A file descriptor to the file specified in pathNamePtr.
 *      LE_DUPLICATE if the file already exists and LE_FLOCK_FAIL_IF_EXIST is specified in createMode.
 *      LE_WOULD_BLOCK if noblock is true and there is an incompatible lock on the file.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static int Create
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    le_flock_CreateMode_t createMode,   ///< [IN] The action to take if the file already exists.
    mode_t permissions,                 ///< [IN] The file permissions used when creating the file.
                                        ///       See the function header comments for more details.
    bool blocking                       ///< [IN] true if blocking, false if non-blocking.
)
{
    // Get the lock type and the open flags based on the access mode.
    int lockType;
    int openFlags;
    GetFlags(accessMode, blocking, &lockType, &openFlags);

    // Always add the create flag.
    openFlags |= O_CREAT;

    // Add additional open flags based on the createMode.
    switch (createMode)
    {
        case LE_FLOCK_OPEN_IF_EXIST:
            // No other flags are needed.
            break;

        case LE_FLOCK_REPLACE_IF_EXIST:
            openFlags |= O_TRUNC;
            break;

        case LE_FLOCK_FAIL_IF_EXIST:
            openFlags |= O_EXCL;
            break;

        default:
            LE_FATAL("Unrecognized create mode %d.", createMode);
    }

    // Open the file.
    int fd;
    do
    {
        fd = open(pathNamePtr, openFlags, permissions);
    }
    while ( (fd == -1) && (errno == EINTR) );

    if (fd == -1)
    {
        if (errno == EEXIST)
        {
            return LE_DUPLICATE;
        }
        else
        {
            LE_WARN("Could not open file '%s'.  %m.", pathNamePtr);
            return LE_FAULT;
        }
    }

    // Lock the file.
    return LockFd(fd, pathNamePtr, lockType, blocking);
}


//--------------------------------------------------------------------------------------------------
/**
 * Opens and locks an existing file.
 *
 * The file can be open for reading, writing or both as specified in the accessMode argument.  If
 * accessMode is either LE_FLOCK_WRITE or LE_FLOCK_READ_AND_WRITE then a write lock will be placed
 * on the file, otherwise a read lock will be placed on the file.
 *
 * If attempting to lock a file that already has an incompatible lock on it this function will block
 * until the lock can be obtained.
 *
 * @return
 *      A file descriptor to the file specified in pathNamePtr.
 *      LE_NOT_FOUND if the file does not exist.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
int le_flock_Open
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode    ///< [IN] The access mode to open the file with.
)
{
    return Open(pathNamePtr, accessMode, true);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates, opens and locks file.
 *
 * If the file does not exist it will be created with the file permissions specified in the arugment
 * permissions (modified by the process's umask).  Refer to the POSIX function open(2) for details
 * of mode_t:
 *
 * http://man7.org/linux/man-pages/man2/open.2.html
 *
 * If the file already exists then this function will either replace the existing file, open the
 * existing file or fail depending on the createMode argument.  The permissions argument is ignored
 * if the file already exists.
 *
 * The file can be opened for reading, writing or both as specified in the accessMode argument.  If
 * accessMode is either LE_FLOCK_WRITE or LE_FLOCK_READ_AND_WRITE then a write lock will be placed
 * on the file, otherwise a read lock will be placed on the file.
 *
 * If attempting to lock a file that already has an incompatible lock on it this function will block
 * until the lock can be obtained.  This function may block even if it creates the file because
 * creating the file and locking it is not atomic.
 *
 * @return
 *      A file descriptor to the file specified in pathNamePtr.
 *      LE_DUPLICATE if the file already exists and LE_FLOCK_FAIL_IF_EXIST is specified in createMode
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
int le_flock_Create
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
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
 * Opens and locks an existing file.
 *
 * The file can be open for reading, writing or both as specified in the accessMode argument.  If
 * accessMode is either LE_FLOCK_WRITE or LE_FLOCK_READ_AND_WRITE then a write lock will be placed
 * on the file, otherwise a read lock will be placed on the file.
 *
 * If attempting to lock a file that already has an incompatible lock on it this function will fail
 * and return LE_WOULD_BLOCK immediately.
 *
 * @return
 *      A file descriptor to the file specified in pathNamePtr.
 *      LE_NOT_FOUND if the file does not exist.
 *      LE_WOULD_BLOCK if there is already an incompatible lock on the file.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
int le_flock_TryOpen
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode    ///< [IN] The access mode to open the file with.
)
{
    return Open(pathNamePtr, accessMode, false);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates, opens and locks file.
 *
 * If the file does not exist it will be created with the file permissions specified in the arugment
 * permissions (modified by the process's umask).  Refer to the POSIX function open(2) for details
 * of mode_t:
 *
 * http://man7.org/linux/man-pages/man2/open.2.html
 *
 * If the file already exists then this function will either replace the existing file, open the
 * existing file or fail depending on the createMode argument.  The permissions argument is ignored
 * if the file already exists.
 *
 * The file can be opened for reading, writing or both as specified in the accessMode argument.  If
 * accessMode is either LE_FLOCK_WRITE or LE_FLOCK_READ_AND_WRITE then a write lock will be placed
 * on the file, otherwise a read lock will be placed on the file.
 *
 * If attempting to lock a file that already has an incompatible lock on it this function will fail
 * and return LE_WOULD_BLOCK immediately.  This function may fail with LE_WOULD_BLOCK even if it
 * creates the file because creating the file and locking it is not atomic.
 *
 * @return
 *      A file descriptor to the file specified in pathNamePtr.
 *      LE_DUPLICATE if the file already exists and LE_FLOCK_FAIL_IF_EXIST is specified in createMode
 *      LE_WOULD_BLOCK if there is already an incompatible lock on the file.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
int le_flock_TryCreate
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    le_flock_CreateMode_t createMode,   ///< [IN] The action to take if the file already exists.
    mode_t permissions                  ///< [IN] The file permissions used when creating the file.
                                        ///       See the function header comments for more details.
)
{
    return Create(pathNamePtr, accessMode, createMode, permissions, false);
}


//--------------------------------------------------------------------------------------------------
/**
 * Closes the file and releases the lock.
 */
//--------------------------------------------------------------------------------------------------
void le_flock_Close
(
    int fd                              /// [IN] The file descriptor of the file to close.
)
{
    // Closing the file descriptor releases the lock.
    fd_Close(fd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Opens a stream to the given file descriptor.
 *
 * @return
 *      A buffered file stream handle to the file if successful.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static FILE* OpenStreamToFd
(
    int fd,                             ///< [IN] The file descriptor to open the stream for.
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    le_result_t* resultPtr              ///< [OUT] A pointer to result code.
)
{
    // Build the fopen mode string from the accessMode.
    char modeStr[3];

    switch (accessMode)
    {
        case LE_FLOCK_READ:
            LE_ASSERT(le_utf8_Copy(modeStr, "r", sizeof(modeStr), NULL) == LE_OK);
            break;

        case LE_FLOCK_WRITE:
            // The 'w' option does not cause truncation when used with fdopen().
            LE_ASSERT(le_utf8_Copy(modeStr, "w", sizeof(modeStr), NULL) == LE_OK);
            break;

        case LE_FLOCK_APPEND:
            // The 'a' option cannot create the file when used with fdopen().
            LE_ASSERT(le_utf8_Copy(modeStr, "a", sizeof(modeStr), NULL) == LE_OK);
            break;

        case LE_FLOCK_READ_AND_WRITE:
            // The 'w+' option does not cause truncation when used with fdopen().
            LE_ASSERT(le_utf8_Copy(modeStr, "w+", sizeof(modeStr), NULL) == LE_OK);
            break;

        case LE_FLOCK_READ_AND_APPEND:
            // The 'a+' option cannot create the file when used with fdopen().
            LE_ASSERT(le_utf8_Copy(modeStr, "a+", sizeof(modeStr), NULL) == LE_OK);
            break;

        default:
            LE_FATAL("Unrecognized access mode %d.", accessMode);
    }

    // Open the stream to the fd.
    FILE* filePtr = fdopen(fd, modeStr);

    if (filePtr == NULL)
    {
        LE_WARN("Could not open stream to file '%s'.  %m.", pathNamePtr);

        if (resultPtr != NULL)
        {
            *resultPtr = LE_FAULT;
        }
    }
    else if (resultPtr != NULL)
    {
        *resultPtr = LE_OK;
    }

    return filePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Locks an existing file and opens a C standard library buffered file stream to it.
 *
 * The file can be open for reading, writing or both read and write as specified in the accessMode
 * argument.  If accessMode is either LE_FLOCK_WRITE or LE_FLOCK_READ_AND_WRITE then a write lock
 * will be placed on the file, otherwise a read lock will be placed on the file.
 *
 * If attempting to lock a file that already has an incompatible lock on it this function will block
 * until the lock can be obtained.
 *
 * If there was an error NULL is returned and resultPtr is set to:
 *   - LE_NOT_FOUND if the file does not exist.
 *   - LE_FAULT if there was an error.
 *
 * @return
 *      A buffered file stream handle to the file if successful.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
FILE* le_flock_OpenStream
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    le_result_t* resultPtr              ///< [OUT] A pointer to result code.
)
{
    // Open the file and get the fd to it.
    int fd = Open(pathNamePtr, accessMode, false);

    if (fd < 0)
    {
        // There was an error so set the result value and return NULL.
        if (resultPtr != NULL)
        {
            *resultPtr = fd;
        }

        return NULL;
    }

    return OpenStreamToFd(fd, pathNamePtr, accessMode, resultPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a file, locks it and opens a C standard library buffered file stream to it.
 *
 * If the file does not exist it will be created with the file permissions specified in the arugment
 * permissions (modified by the process's umask).  Refer to the POSIX function open(2) for details
 * of mode_t:
 *
 * http://man7.org/linux/man-pages/man2/open.2.html
 *
 * If the file already exists then this function will either replace the existing file, open the
 * existing file or fail depending on the createMode argument.
 *
 * The file can be opened for reading, writing or both as specified in the accessMode argument.  If
 * accessMode is either LE_FLOCK_WRITE or LE_FLOCK_READ_AND_WRITE then a write lock will be placed
 * on the file, otherwise a read lock will be placed on the file.
 *
 * If attempting to lock a file that already has an incompatible lock on it this function will block
 * until the lock can be obtained.  This function may block even if it creates the file because
 * creating the file and locking it is not atomic.
 *
 * If there was an error NULL is returned and resultPtr is set to:
 *  - LE_DUPLICATE if the file already exists and LE_FLOCK_FAIL_IF_EXIST is specified in createMode.
 *  - LE_FAULT if there was an error.
 *
 * @return
 *      A buffered file stream handle to the file if successful.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
FILE* le_flock_CreateStream
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    le_flock_CreateMode_t createMode,   ///< [IN] The action to take if the file already exists.
    mode_t permissions,                 ///< [IN] The file permissions used when creating the file.
                                        ///       See the function header comments for more details.
    le_result_t* resultPtr              ///< [OUT] A pointer to result code.
)
{
    // Create and open the file and get the fd to it.
    int fd = Create(pathNamePtr, accessMode, createMode, permissions, false);

    if (fd < 0)
    {
        // There was an error so set the result value and return NULL.
        if (resultPtr != NULL)
        {
            *resultPtr = fd;
        }
        return NULL;
    }

    return OpenStreamToFd(fd, pathNamePtr, accessMode, resultPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Locks an existing file and opens a C standard library buffered file stream to it.
 *
 * The file can be open for reading, writing or both read and write as specified in the accessMode
 * argument.  If accessMode is either LE_FLOCK_WRITE or LE_FLOCK_READ_AND_WRITE then a write lock
 * will be placed on the file, otherwise a read lock will be placed on the file.
 *
 * If attempting to lock a file that already has an incompatible lock on it this function will
 * return NULL immediately and set resultPtr to LE_WOULD_BLOCK.
 *
 * If there was an error NULL is returned and resultPtr is set to:
 *   - LE_NOT_FOUND if the file does not exist.
 *   - LE_WOULD_BLOCK if there is already an incompatible lock on the file.
 *   - LE_FAULT if there was an error.
 *
 * @return
 *      A buffered file stream handle to the file if successful.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
FILE* le_flock_TryOpenStream
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    le_result_t* resultPtr              ///< [OUT] A pointer to result code.
)
{
    // Open the file and get the fd to it.
    int fd = Open(pathNamePtr, accessMode, true);

    if (fd < 0)
    {
        // There was an error so set the result value and return NULL.
        if (resultPtr != NULL)
        {
            *resultPtr = fd;
        }
        return NULL;
    }

    return OpenStreamToFd(fd, pathNamePtr, accessMode, resultPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a file, locks it and opens a C standard library buffered file stream to it.
 *
 * If the file does not exist it will be created with the file permissions specified in the argument
 * permissions (modified by the process's umask).  Refer to the POSIX function open(2) for details
 * of mode_t:
 *
 * http://man7.org/linux/man-pages/man2/open.2.html
 *
 * If the file already exists then this function will either replace the existing file, open the
 * existing file or fail depending on the createMode argument.
 *
 * The file can be opened for reading, writing or both as specified in the accessMode argument.  If
 * accessMode is either LE_FLOCK_WRITE or LE_FLOCK_READ_AND_WRITE then a write lock will be placed
 * on the file, otherwise a read lock will be placed on the file.
 *
 * If attempting to lock a file that already has an incompatible lock on it this function will
 * return NULL immediately and set resultPtr to LE_WOULD_BLOCK.  This function may fail with
 * LE_WOULD_BLOCK even if it creates the file because creating the file and locking it is not
 * atomic.
 *
 * If there was an error NULL is returned and resultPtr is set to:
 *  - LE_DUPLICATE if the file already exists and LE_FLOCK_FAIL_IF_EXIST is specified in createMode.
 *  - LE_WOULD_BLOCK if there is already an incompatible lock on the file.
 *  - LE_FAULT if there was an error.
 *
 * @return
 *      A buffered file stream handle to the file if successful.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
FILE* le_flock_TryCreateStream
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode,   ///< [IN] The access mode to open the file with.
    le_flock_CreateMode_t createMode,   ///< [IN] The action to take if the file already exists.
    mode_t permissions,                 ///< [IN] The file permissions used when creating the file.
                                        ///       See the function header comments for more details.
    le_result_t* resultPtr              ///< [OUT] A pointer to result code.
)
{
    // Create and open the file and get the fd to it.
    int fd = Create(pathNamePtr, accessMode, createMode, permissions, true);

    if (fd < 0)
    {
        // There was an error so set the result value and return NULL.
        if (resultPtr != NULL)
        {
            *resultPtr = fd;
        }
        return NULL;
    }

    return OpenStreamToFd(fd, pathNamePtr, accessMode, resultPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Closes the file stream and releases the lock.
 */
//--------------------------------------------------------------------------------------------------
void le_flock_CloseStream
(
    FILE* fileStreamPtr
)
{
    // Closing the file stream also closes the underlying file descriptor which releases the lock.
    LE_CRIT_IF(fclose(fileStreamPtr) != 0, "Failed to close file stream. %m.");

}
