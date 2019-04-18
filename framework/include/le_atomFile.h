/**
 * @page c_atomFile Atomic File Operation API
 *
 * @subpage le_atomFile.h "API Reference"
 *
 * <HR>
 *
 * This API provides an atomic file access mechanism that can be used to perform file operation
 * (specially file write) in atomic fashion.
 *
 * This API only supports regular files. Attempts to use this API on sockets, devices, etc. results
 * in undefined behavior.
 *
 * @section c_atomFile_operation Atomic File Operations
 *
 * An atomic file operation is an operation that cannot be partially performed. Either the entire
 * operation is performed or the operation fails. Any unclean reboot or power-cut should not lead to
 * corruption or inconsistency of the file. Also when a process is performing an atomic write on a
 * file, other processes should not be able modify that file, i.e. some file locking mechanism
 * should be there.
 *
 * Use @c le_atomFile_Open() to open a file for atomic access. This API uses @ref c_flock_cooperative
 * mechanism while opening a file, i.e. if file already has an incompatible lock on it,
 * @c le_atomFile_Open() will block until it can obtain the lock. File must be closed using
 * @c le_atomFile_Close() or @c le_atomFile_Cancel() api. Both @c le_atomFile_Close() and
 * @c le_atomFile_Close() closes the file and releases the acquired resources. However,
 * @c le_atomFile_Close() transfers all changes to disk, @c le_atomFile_Cancel() no change is
 * reflected on file. A file can be deleted atomically using @c le_atomFile_Delete() api.
 *
 * For opening C standard file stream, please see @ref c_atomFile_streams section.
 *
 * Writing on the file descriptors obtained by this API same as writing to regular file descriptor.
 * That means, any write to a file descriptor using this API doesn't ensure that data is transferred
 * to disk. Data is only transferred to disk when le_atomFile_Close() returns successfully. This
 * behavior is same for file stream as well.
 *
 * Code fragment illustrating atomic write using file descriptor.
 *
 * @code
 *
 *      // Atomic write example, File Descriptor case.
 *
 *      int fd = le_atomFile_Open("./myfile.txt", LE_FLOCK_READ_AND_APPEND);
 *
 *      if (fd < 0)
 *      {
 *          // Print error message and exit.
 *      }
 *
 *      // Write something in fd
 *      char myString[] = "This string for atomic writing";
 *
 *      // Now write this string to fd
 *      write(fd, myString, sizeof(myString));    // This string write doesn't go disk
 *
 *
 *      le_result_t result = le_atomFile_Close(fd); // Transfers all changes to disk
 *
 *      if (result == LE_OK)
 *      {
 *          // Print success message
 *      }
 *
 * @endcode
 *
 * Code fragment illustrating atomic write using file stream.
 *
 * @code
 *      // Atomic write example, File Stream case.
 *
 *      FILE* file = le_atomFile_OpenStream("./myfile.txt", LE_FLOCK_READ_AND_APPEND, NULL);
 *
 *      if (file == NULL)
 *      {
 *          // Print error message and exit.
 *      }
 *
 *      // Write something in file stream
 *      char myString[] = "This string for atomic writing";
 *
 *      // Now write this string to file stream
 *      fwrite(myString, 1, sizeof(myString), file);   // This string write doesn't go disk
 *
 *
 *      le_result_t result = le_atomFile_CloseStream(fd); // Transfers all changes to disk
 *
 *      if (result == LE_OK)
 *      {
 *          // Print success message
 *      }
 *
 * @endcode
 *
 * An example illustrating usage of le_atomFile_Close(), le_atomFile_Cancel() and
 * le_atomFile_Delete() function.
 *
 * @code
 *
 *      int fd = le_atomFile_Open("./myfile.txt", LE_FLOCK_READ_AND_APPEND);
 *
 *      if (fd < 0)
 *      {
 *          // Print error message and exit.
 *      }
 *
 *      // Write something in fd
 *      char myString[] = "This string for atomic writing";
 *
 *      // Now write this string to fd
 *      write(fd, myString, sizeof(myString));    // This string write doesn't go disk
 *
 *      bool doCommit = NeedToCommit();           // A fictitious function that returns whether
 *                                                // write on fd should be sent to disk or not.
 *
 *      if (doCommit)
 *      {
 *          le_result_t result = le_atomFile_Close(fd); // Transfer all changes to disk and close
 *                                                      // the file descriptor.
 *          if (result != LE_OK)
 *          {
 *              // Print error message.
 *          }
 *
 *      }
 *      else
 *      {
 *          le_atomFile_Cancel(fd); // Discard all changes and close the file descriptor.
 *      }
 *
 *
 *      // Now do some additional stuff with file myfile.txt
 *      // .........Code.........
 *      // .........Code.........
 *
 *
 *      // Now delete file myfile.txt
 *      le_result_t result = le_atomFile_Delete("./myfile.txt");
 *      if (result != LE_OK)
 *      {
 *          // Print error message.
 *      }
 *
 * @endcode
 *
 * The le_atomFile_Create() function can be used to create, lock and open a file in one function
 * call.
 *
 * @section c_atomFile_streams Streams
 *
 * The functions @c le_atomFile_OpenStream() and @c le_atomFile_CreateStream() can be used to obtain
 * a file stream for atomic operation. @c le_atomFile_CloseStream() is used to commit all changes
 * to disk and close the stream. @c le_atomFile_CancelStream() is used to discard all changes and
 * close the stream. These functions are analogous to le_atomFile_Open(), le_atomFile_Create()
 * le_atomFile_Close() and le_atomFile_Cancel() except that works on file streams rather than file
 * descriptors.
 *
 * @section c_atomFile_nonblock Non-blocking
 *
 * Functions le_atomFile_Open(), le_atomFile_Create(), le_atomFile_OpenStream(),
 * le_atomFile_CreateStream() and le_atomFile_Delete() always block if there is an incompatible lock
 * on the file. Functions le_atomFile_TryOpen(), le_atomFile_TryCreate(),
 * le_atomFile_TryOpenStream(), le_atomFile_TryCreateStream() and le_atomFile_TryDelete() are their
 * non-blocking counterparts.
 *
 * @section c_atomFile_threading Multiple Threads
 *
 * All the functions in this API are thread-safe and reentrant.
 *
 * @section c_atomFile_limitations Limitations
 *
 * These APIs have inherent limitations of @ref c_flock (i.e. advisory lock, inability to detect
 * deadlock etc.), as they use @ref c_flock.
 *
 * File descriptors obtained via calling these APIs can't be replicated via fork or dup
 *
 * @code
 *
 *      int oldfd = le_atomFile_Open("./myfile.txt", LE_FLOCK_READ_AND_APPEND);
  *
 *      if (oldfd < 0)
 *      {
 *          // Print error message and exit.
 *      }
 *
 *      int newfd = dup(oldfd);   // newfd is created via dup.
 *      ..
 *      // Write something in newfd
 *      ..
 *      le_result_t result = le_atomFile_Close(newfd); // Wrong. This newfd is not recognized by API
 *
 * @endcode
 *
 * File descriptors/streams obtained via using these APIs must be closed using the corresponding
 * closing APIs (i.e. le_atomFile_Close(), le_atomFile_CloseStream() etc.). This is illustrated in
 * following code fragments.
 *
 * Code fragment showing proper closing procedure of file descriptor obtained via this API.
 *
 * @code
 *
 *      int fd = le_atomFile_Open("./myfile.txt", LE_FLOCK_READ_AND_APPEND);
 *
 *      if (fd < 0)
 *      {
 *          // Print error message and exit.
 *      }
 *
 *      ..
 *      // Write something in fd
 *      ..
 *      le_result_t result = le_atomFile_Close(fd); // This is right.
 *
 *      if (result != LE_OK)
 *      {
 *         // Print some error message
 *      }
 *
 * @endcode
 *
 * Code fragment showing wrong closing procedure of file descriptor obtained via this API.
 *
 * @code
 *
 *      int fd = le_atomFile_Open("./myfile.txt", LE_FLOCK_READ_AND_APPEND);
 *
 *      if (fd < 0)
 *      {
 *          // Print error message and exit.
 *      }
 *
 *      ..
 *      // Write something in fd
 *      ..
 *      int result = close(fd); // Wrong as it doesn't use closing API (i.e. le_atomFile_Close()
 *                              // or le_atomFile_Cancel())
 *
 *      if (result < 0)
 *      {
 *         // Print some error message
 *      }
 *
 * @endcode
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

//--------------------------------------------------------------------------------------------------
/** @file le_atomFile.h
 *
 * Legato @ref c_atomFile include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_ATOMFILE_INCLUDE_GUARD
#define LEGATO_ATOMFILE_INCLUDE_GUARD


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
    le_flock_AccessMode_t accessMode    ///< [IN] Access mode to open the file.
);


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
    le_flock_AccessMode_t accessMode,   ///< [IN] Access mode to open the file.
    le_flock_CreateMode_t createMode,   ///< [IN] Action to take if the file already exists.
    mode_t permissions                  ///< [IN] File permissions used when creating the file.
                                        ///       See the function header comments for more details.
);


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
    le_flock_AccessMode_t accessMode    ///< [IN] Access mode to open the file.
);


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
    le_flock_AccessMode_t accessMode,   ///< [IN] Access mode to open the file.
    le_flock_CreateMode_t createMode,   ///< [IN] Action to take if the file already exists.
    mode_t permissions                  ///< [IN] File permissions used when creating the file.
);


//--------------------------------------------------------------------------------------------------
/**
 * Cancels all changes and closes the file descriptor.
 */
//--------------------------------------------------------------------------------------------------
void le_atomFile_Cancel
(
    int fd                              /// [IN] The file descriptor to close.
);


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
);


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
    le_flock_AccessMode_t accessMode,   ///< [IN] Access mode to open the file.
    le_result_t* resultPtr              ///< [OUT] Pointer to result code.  This can be NULL if
                                        ///        the result code is not wanted.
);


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
    le_flock_AccessMode_t accessMode,   ///< [IN] Access mode to open the file.
    le_flock_CreateMode_t createMode,   ///< [IN] Action to take if the file already exists.
    mode_t permissions,                 ///< [IN] File permissions used when creating the file.
                                        ///       See the function header comments for more details.
    le_result_t* resultPtr              ///< [OUT] Pointer to result code.  This can be NULL if
                                        ///        the result code is not wanted.
);


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
    le_flock_AccessMode_t accessMode,   ///< [IN] Access mode to open the file.
    le_result_t* resultPtr              ///< [OUT] Pointer to result code.
);


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
    le_flock_AccessMode_t accessMode,   ///< [IN] Access mode to open the file.
    le_flock_CreateMode_t createMode,   ///< [IN] Action to take if the file already exists.
    mode_t permissions,                 ///< [IN] File permissions used when creating the file.
    le_result_t* resultPtr              ///< [OUT] Pointer to result code.
);


//--------------------------------------------------------------------------------------------------
/**
 * Cancels all changes and closes the file stream.
 */
//--------------------------------------------------------------------------------------------------
void le_atomFile_CancelStream
(
    FILE* fileStreamPtr               ///< [IN] File stream pointer to close
);


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
);


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
);


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
);


#endif //LEGATO_ATOMIC_INCLUDE_GUARD
