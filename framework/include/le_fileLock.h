/**
 * @page c_flock File Locking API
 *
 * @subpage le_fileLock.h "API Reference"
 *
 * <HR>
 *
 * File locking is a form of IPC used to synchronize multiple processes' access to
 * common files.
 *
 * This API provides a co-operative file locking mechanism that can be used by multiple processes
 * and/or threads to sychronize reads and writes to common files.
 *
 * This API only supports regular files. Attempts to use this API on sockets, devices, etc. results
 * in undefined behaviour.
 *
 * @section c_flock_cooperative Co-operative File Locking
 *
 * Co-operative file locks (also known as advisory file locks) means that the processes and threads
 * must co-operate to synchronize their access to the file.  If a process or thread simply ignores
 * the lock and accesses the file then access synchronization errors may occur.
 *
 * @section c_flock_locks Locking Files
 *
 * There are two types of locks that can be applied: read lock and write lock.  A file
 * can have multiple simultaneous read locks, but can only have one write lock. Also, a
 * file can only have one type of lock on it at one time. A file may be locked
 * for reading if the file is unlocked or if there are read locks on the file, but to lock a file
 * for writing the file must be unlocked.
 *
 * Use @c le_flock_Open() to lock a file and open it for access. When attempting to lock a file that
 * already has an incompatible lock on it, @c le_flock_Open() will block until it can obtain the lock.
 * Call @c le_flock_Close() to close the file and remove the lock on the file.
 *
 * This code sample shows four processes attempting to access the same file.  Assume
 * that all the calls to le_flock_Open() in the example occur in chronological order as they appear:
 *
 * @code
 *      // Code in Process 1.
 *
 *      // Lock the file for reading.
 *      int fd = le_flock_Open("foo", LE_FLOCK_READ);  // This call will not block.
 *
 *      // Read from the file.
 *      ...
 *
 *      // Close the file and release the lock.
 *      le_flock_Close(fd);
 * -------------------------------------------------------------------------------------------------
 *
 *      // Code in Process 2.
 *
 *      // Lock the file for reading.
 *      int fd = le_flock_Open("foo", LE_FLOCK_READ);  // This call will not block.
 *
 *      // Read from the file.
 *      ...
 *
 *      // Close the file and release the lock.
 *      le_flock_Close(fd);
 * -------------------------------------------------------------------------------------------------
 *
 *      // Code in Process 3.
 *
 *      // Lock the file for writing.
 *      int fd = le_flock_Open("foo", LE_FLOCK_WRITE);  // This call will block until both Process 1
 *                                                      // and Process 2 removes their locks.
 *
 *      // Write to the file.
 *      ...
 *
 *      // Close the file and release the lock.
 *      le_flock_Close(fd);
 * @endcode
 *
 * This sample shows that Process 2 obtains the read lock even though Process 1 already
 * has a read lock on the file. Process 3 is blocked because it's attempting a
 * write lock on the file.  Process 3 is blocked until both Process 1 and 2 remove their locks.
 *
 * When multiple processes are blocked waiting to obtain a lock on the file, it's unspecified which
 * process will obtain the lock when the file becomes available.
 *
 * The le_flock_Create() function can be used to create, lock and open a file in one function call.
 *
 * @section c_flock_streams Streams
 *
 * The functions @c le_flock_OpenStream() and @c le_flock_CreateStream() can be used to obtain a file
 * stream to a locked file.  @c le_flock_CloseStream() is used to close the stream and remove the lock.
 * These functions are analogous to le_flock_Open(), le_flock_Create() and le_flock_Close() except
 * that they return file streams rather than file descriptors.
 *
 * @section c_flock_nonblock Non-blocking
 *
 * Functions le_flock_Open(), le_flock_Create(), le_flock_OpenStream() and
 * le_flock_CreateStream() always block if there is an incompatible lock on the file.  Functions
 * le_flock_TryOpen(), le_flock_TryCreate(), le_flock_TryOpenStream() and
 * le_flock_TryCreateStream() are their non-blocking counterparts.
 *
 * @section c_flock_threads Multiple Threads
 *
 * All functions in this API are thread-safe; processes and threads can use this API to
 * synchronize their access to files.
 *
 * @section c_flock_replicateFd Replicating File Descriptors
 *
 * File locks are contained in the file descriptors that are returned by le_flock_Open() and
 * le_flock_Create() and in the underlying file descriptors of the file streams returned by
 * le_flock_OpenStream() and le_flock_CreateStream().
 *
 * File descriptors are closed the locks are automatically removed.  Functions
 * le_flock_Close() and le_flock_CloseStream() are provided as a convenience.  When a process dies,
 * all of its file descriptors are closed and any file locks they may contain are removed.
 *
 * If a file descriptor is replicated either through dup() or fork(), the file lock will also be
 * replicated in the new file descriptor:
 *
 * @code
 *      int oldfd = le_flock_Open("foo", LE_FLOCK_READ);  // Place a read lock on the file "foo".
 *      int newfd = dup(oldfd);
 *
 *      le_flock_Close(oldfd); // Closes the fd and removes the lock.
 * @endcode
 *
 * There must still be a read lock on the file "foo" because newfd
 * has not been closed.
 *
 * This behaviour can be used to pass file locks from a parent to a child through a fork() call.
 * The parent can obtain the file lock, fork() and close its file descriptor.  Now the child has
 * exclusive possession of the file lock.
 *
 * @section c_flock_limitations Limitations
 *
 * Here are some limitations to the file locking mechanisms in this API:
 *
 * The file locks in this API are advisory only, meaning that a process may
 * simply ignore the lock and access the file anyways.
 *
 * This API does not detect deadlocks and a process may deadlock itself.  For example:
 *
 * @code
 *      int fd1 = le_flock_Open("foo", LE_FLOCK_READ);   // Obtains a read lock on the file.
 *      int fd2 = le_flock_Open("foo", LE_FLOCK_WRITE);  // This call will block forever.
 * @endcode
 *
 * This API only permits whole files to be locked, not portions of a
 * file.
 *
 * Many NFS implementations don't recognize locks used by this API.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

 //--------------------------------------------------------------------------------------------------
/** @file le_fileLock.h
 *
 * Legato @ref c_flock include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_FLOCK_INCLUDE_GUARD
#define LEGATO_FLOCK_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * File access modes.
 *
 * @note When writing to a file, the writes are always appended to the end of the file by default.
 *       When reading from a file, the reads always starts at the beginning of the file by default.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_FLOCK_READ,              ///< Opens the file for reading.
    LE_FLOCK_WRITE,             ///< Opens the file for writing.
    LE_FLOCK_APPEND,            ///< Opens the file for writing.  Writes will be appended to the end
                                ///  of the file.
    LE_FLOCK_READ_AND_WRITE,    ///< Opens the file for reading and writing.
    LE_FLOCK_READ_AND_APPEND    ///< Opens the file for reading and writing.  Writes will be
                                ///  appended to the end of the file.
}
le_flock_AccessMode_t;


//--------------------------------------------------------------------------------------------------
/**
 * File creation modes specify the action to take when creating a file that already exists.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_FLOCK_OPEN_IF_EXIST,     ///< Opens the file if it already exists.
    LE_FLOCK_REPLACE_IF_EXIST,  ///< Replaces the file if it already exists.
    LE_FLOCK_FAIL_IF_EXIST      ///< Fails if the file already exists.
}
le_flock_CreateMode_t;


//--------------------------------------------------------------------------------------------------
/**
 * Opens and locks an existing file.
 *
 * The file can be open for reading, writing or both as specified in the accessMode argument. If
 * accessMode is either LE_FLOCK_WRITE or LE_FLOCK_READ_AND_WRITE then a write lock will be placed
 * on the file, otherwise a read lock will be placed on the file.
 *
 * If attempting to lock a file that already has an incompatible lock on it this function will block
 * until the lock can be obtained.
 *
 * @return
 *      File descriptor to the file specified in pathNamePtr.
 *      LE_NOT_FOUND if the file does not exist.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
int le_flock_Open
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode    ///< [IN] Access mode to open the file.
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates, opens and locks file.
 *
 * If the file does not exist, it will be created with the file permissions specified in the arugment
 * permissions (modified by the process's umask).  Refer to the POSIX function open(2) for details
 * of mode_t:
 *
 * http://man7.org/linux/man-pages/man2/open.2.html
 *
 * If the file already exists, then this function will either replace the existing file, open the
 * existing file or fail depending on the createMode argument.  The permissions argument is ignored
 * if the file already exists.
 *
 * The file can be opened for reading, writing or both as specified in the accessMode argument.  If
 * accessMode is either LE_FLOCK_WRITE or LE_FLOCK_READ_AND_WRITE, a write lock will be placed
 * on the file, otherwise a read lock will be placed on the file.
 *
 * If attempting to lock a file that already has an incompatible lock on it, this function will block
 * until the lock can be obtained. This function may block even if it creates the file because
 * creating the file and locking it is not atomic.
 *
 * @return
 *      File descriptor to the file specified in pathNamePtr.
 *      LE_DUPLICATE if the file already exists and LE_FLOCK_FAIL_IF_EXIST is specified in createMode
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
int le_flock_Create
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode,   ///< [IN] Access mode to open the file.
    le_flock_CreateMode_t createMode,   ///< [IN] Action to take if the file already exists.
    mode_t permissions                  ///< [IN] File permissions used when creating the file.
                                        ///       See the function header comments for more details.
);


//--------------------------------------------------------------------------------------------------
/**
 * Opens and locks an existing file.
 *
 * The file can be open for reading, writing or both as specified in the accessMode argument.  If
 * accessMode is either LE_FLOCK_WRITE or LE_FLOCK_READ_AND_WRITE, a write lock will be placed
 * on the file, otherwise a read lock will be placed on the file.
 *
 * If attempting to lock a file that already has an incompatible lock on it, this function will fail
 * and return LE_WOULD_BLOCK immediately.
 *
 * @return
 *      File descriptor to the file specified in pathNamePtr.
 *      LE_NOT_FOUND if the file does not exist.
 *      LE_WOULD_BLOCK if there is already an incompatible lock on the file.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
int le_flock_TryOpen
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode    ///< [IN] Access mode to open the file.
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates, opens and locks file.
 *
* If the file does not exist, it will be created with the file permissions specified in the argument
 * permissions (modified by the process's umask).  Refer to the POSIX function open(2) for details
 * of mode_t:
 *
 * http://man7.org/linux/man-pages/man2/open.2.html
 *
 * If the file already exists, this function will either replace the existing file, open the
 * existing file or fail depending on the createMode argument. The permissions argument is ignored
 * if the file already exists.
 *
 * The file can be opened for reading, writing or both as specified in the accessMode argument.  f
 * accessMode is either LE_FLOCK_WRITE or LE_FLOCK_READ_AND_WRITE, a write lock will be placed
 * on the file, otherwise a read lock will be placed on the file.
 *
 * If attempting to lock a file that already has an incompatible lock on it, this function will fail
 * and return LE_WOULD_BLOCK immediately.  This function may fail with LE_WOULD_BLOCK even if it
 * creates the file because creating the file and locking it is not atomic.
 *
 * @return
 *      File descriptor to the file specified in pathNamePtr.
 *      LE_DUPLICATE if the file already exists and LE_FLOCK_FAIL_IF_EXIST is specified in createMode
 *      LE_WOULD_BLOCK if there is already an incompatible lock on the file.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
int le_flock_TryCreate
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode,   ///< [IN] Access mode to open the file.
    le_flock_CreateMode_t createMode,   ///< [IN] Action to take if the file already exists.
    mode_t permissions                  ///< [IN] File permissions used when creating the file.
                                        ///       See the function header comments for more details.
);


//--------------------------------------------------------------------------------------------------
/**
 * Closes the file and releases the lock.
 */
//--------------------------------------------------------------------------------------------------
void le_flock_Close
(
    int fd                              ///< [IN] File descriptor of the file to close.
);


//--------------------------------------------------------------------------------------------------
/**
 * Locks an existing file and opens a C standard library buffered file stream to it.
 *
 * The file can be open for reading, writing or both read and write as specified in the accessMode
 * argument.  If accessMode is either LE_FLOCK_WRITE or LE_FLOCK_READ_AND_WRITE then a write lock
 * will be placed on the file, otherwise a read lock will be placed on the file.
 *
 * If attempting to lock a file that already has an incompatible lock on it, this function will block
 * until the lock can be obtained.
 *
 * If there was an error NULL is returned and resultPtr is set to:
 *   - LE_NOT_FOUND if the file does not exist.
 *   - LE_FAULT if there was an error.
 *
 * @return
 *      Buffered file stream handle to the file if successful.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
FILE* le_flock_OpenStream
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode,   ///< [IN] Access mode to open the file.
    le_result_t* resultPtr              ///< [OUT] Pointer to result code.  This can be NULL if
                                        ///        the result code is not wanted.
);


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
 *      Buffered file stream handle to the file if successful.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
FILE* le_flock_CreateStream
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode,   ///< [IN] Access mode to open the file.
    le_flock_CreateMode_t createMode,   ///< [IN] Action to take if the file already exists.
    mode_t permissions,                 ///< [IN] File permissions used when creating the file.
                                        ///       See the function header comments for more details.
    le_result_t* resultPtr              ///< [OUT] Pointer to result code.  This can be NULL if
                                        ///        the result code is not wanted.
);


//--------------------------------------------------------------------------------------------------
/**
 * Locks an existing file and opens a C standard library buffered file stream to it.
 *
 * The file can be open for reading, writing or both read and write as specified in the accessMode
 * argument. If accessMode is either LE_FLOCK_WRITE or LE_FLOCK_READ_AND_WRITE then a write lock
 * will be placed on the file, otherwise a read lock will be placed on the file.
 *
 * If attempting to lock a file that already has an incompatible lock on it, this function will
 * return NULL immediately and set resultPtr to LE_WOULD_BLOCK.
 *
 * If there was an error NULL is returned and resultPtr is set to:
 *   - LE_NOT_FOUND if the file does not exist.
 *   - LE_WOULD_BLOCK if there is already an incompatible lock on the file.
 *   - LE_FAULT if there was an error.
 *
 * @return
 *      Buffered file stream handle to the file if successful.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
FILE* le_flock_TryOpenStream
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode,   ///< [IN] Access mode to open the file.
    le_result_t* resultPtr              ///< [OUT] Pointer to result code.
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a file, locks it and opens a C standard library buffered file stream to it.
 *
 * If the file does not exist, it will be created with the file permissions specified in the arugment
 * permissions (modified by the process's umask).  Refer to the POSIX function open(2) for details
 * of mode_t:
 *
 * http://man7.org/linux/man-pages/man2/open.2.html
 *
 * If the file already exists, this function will either replace the existing file, open the
 * existing file or fail depending on the createMode argument.
 *
 * The file can be opened for reading, writing or both as specified in the accessMode argument. If
 * accessMode is either LE_FLOCK_WRITE or LE_FLOCK_READ_AND_WRITE then a write lock will be placed
 * on the file, otherwise a read lock will be placed on the file.
 *
 * If attempting to lock a file that already has an incompatible lock on it, this function will
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
 *      Buffered file stream handle to the file if successful.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
FILE* le_flock_TryCreateStream
(
    const char* pathNamePtr,            ///< [IN] Pointer to the path name of the file to open.
    le_flock_AccessMode_t accessMode,   ///< [IN] Access mode to open the file.
    le_flock_CreateMode_t createMode,   ///< [IN] Action to take if the file already exists.
    mode_t permissions,                 ///< [IN] File permissions used when creating the file.
                                        ///       See the function header comments for more details.
    le_result_t* resultPtr              ///< [OUT] Pointer to result code.
);


//--------------------------------------------------------------------------------------------------
/**
 * Closes the file stream and releases the lock.
 */
//--------------------------------------------------------------------------------------------------
void le_flock_CloseStream
(
    FILE* fileStreamPtr
);


#endif //LEGATO_FLOCK_INCLUDE_GUARD
