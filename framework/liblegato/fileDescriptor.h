/** @file fileDescriptor.h
 *
 * Declaration of the framework's internal handy file descriptor manipulation functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_FILE_DESCRIPTOR_H_INCLUDE_GUARD
#define LE_FILE_DESCRIPTOR_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the file descriptor service.
 * This function must be called before any other le_fd functions are called.
 *
 * @note
 *      On failure, the process exits.
 */
//--------------------------------------------------------------------------------------------------
void fd_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Sets a file descriptor non-blocking.
 *
 * @note    This function is used for both clients and servers.
 */
//--------------------------------------------------------------------------------------------------
void fd_SetNonBlocking
(
    int fd
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets a file descriptor blocking.
 */
//--------------------------------------------------------------------------------------------------
void fd_SetBlocking
(
    int fd
);


//--------------------------------------------------------------------------------------------------
/**
 * Closes a file descriptor.
 *
 * @note This is a wrapper around close() that takes care of retrying if interrupted by a signal,
 *       and logging a critical error if close() fails.
 */
//--------------------------------------------------------------------------------------------------
void fd_Close
(
    int fd
);


//--------------------------------------------------------------------------------------------------
/**
 * Closes all file descriptors in the calling process except for the file descriptors 0, 1 and 2
 * which are usually the standard file descriptors, stdin, stdout, stderr.
 */
//--------------------------------------------------------------------------------------------------
void fd_CloseAllNonStd
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Reads a specified number of bytes from the provided file descriptor into the provided buffer.
 * This function will block until the specified number of bytes is read or an EOF is reached.
 *
 * @return
 *      Number of bytes read.
 *      LE_FAULT if there is an error.
 */
//--------------------------------------------------------------------------------------------------
ssize_t fd_ReadSize
(
    int fd,                               ///<[IN] File to read.
    void* bufPtr,                         ///<[OUT] Buffer to store the read bytes in.
    size_t bufSize                        ///<[IN] Size of the buffer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Writes a specified number of bytes from the provided buffer to the provided file descriptor.
 * This function will block until the specified number of bytes is written.
 *
 * @return
 *      Number of bytes written.
 *      LE_FAULT if there is an error.
 */
//--------------------------------------------------------------------------------------------------
ssize_t fd_WriteSize
(
    int fd,                               ///<[IN] File to write.
    void* bufPtr,                         ///<[IN] Buffer which will be written to file.
    size_t bufSize                        ///<[IN] Size of the buffer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Reads a line of text from the opened file descriptor specified up to the first newline or eof
 * character.  The output buffer will always be NULL-terminated and will not include the newline or
 * eof character.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer is too small.  As much of the line as possible will be copied to
 *                  buf.
 *      LE_OUT_OF_RANGE if there is nothing else to read from the file.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t fd_ReadLine
(
    int fd,             ///< [IN] File to read from.
    char* buf,          ///< [OUT] Buffer to store the line.
    size_t bufSize      ///< [IN] Buffer size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Reads bufSize bytes from the open file descriptor specified by fd, starting at offset, and stores
 * the bytes in the provided buffer.  This function will fail and return LE_FAULT if fewer than
 * bufSize bytes are available from the file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t fd_ReadFromOffset
(
    int fd,                 ///< [IN] File to read.
    off_t offset,           ///< [IN] Offset from the beginning of the file to start reading from.
    void* bufPtr,           ///< [OUT] Buffer to store the read bytes in.
    size_t bufSize          ///< [IN] Size of the buffer.
);


#endif // LE_FILE_DESCRIPTOR_H_INCLUDE_GUARD
