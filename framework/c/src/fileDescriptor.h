/** @file fileDescriptor.h
 *
 * Declaration of the framework's internal handy file descriptor manipulation functions.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LE_FILE_DESCRIPTOR_H_INCLUDE_GUARD
#define LE_FILE_DESCRIPTOR_H_INCLUDE_GUARD

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


#endif // LE_FILE_DESCRIPTOR_H_INCLUDE_GUARD
