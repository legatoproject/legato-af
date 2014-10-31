/** @file fileDescriptor.c
 *
 * Implementation of the framework's internal handy file descriptor manipulation functions.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "fileDescriptor.h"
#include "limit.h"

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
)
//--------------------------------------------------------------------------------------------------
{
    int fdFlags = fcntl(fd, F_GETFL);
    if (fdFlags < 0)
    {
        LE_FATAL("Failed to get flags for fd %d. Errno = %d (%m).", fd, errno);
    }
    if (fcntl(fd, F_SETFL, fdFlags | O_NONBLOCK) != 0)
    {
        LE_FATAL("Failed to set O_NONBLOCK on fd %d. Errno = %d (%m).", fd, errno);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets a file descriptor blocking.
 */
//--------------------------------------------------------------------------------------------------
void fd_SetBlocking
(
    int fd
)
//--------------------------------------------------------------------------------------------------
{
    int fdFlags = fcntl(fd, F_GETFL);
    if (fdFlags < 0)
    {
        LE_FATAL("Failed to get flags for fd %d. Errno = %d (%m).", fd, errno);
    }
    if (fcntl(fd, F_SETFL, fdFlags & (~O_NONBLOCK)) != 0)
    {
        LE_FATAL("Failed to clear O_NONBLOCK on fd %d. Errno = %d (%m).", fd, errno);
    }
}


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
)
//--------------------------------------------------------------------------------------------------
{
    int result;

    // Keep trying to close the fd as long as it keeps getting interrupted by signals.
    do
    {
        result = close(fd);
    }
    while ((result == -1) && (errno == EINTR));

    if (result != 0)
    {
        LE_CRIT("Failed to close file descriptor %d. Errno = %d (%m).", fd, errno);
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Closes all file descriptors in the calling process except for the file descriptors 0, 1 and 2
 * which are usually the standard file descriptors, stdin, stdout, stderr.
 */
//--------------------------------------------------------------------------------------------------
void fd_CloseAllNonStd
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    int maxNumFds = sysconf(_SC_OPEN_MAX);
    if (maxNumFds == -1)
    {
        maxNumFds = LIMIT_MAX_NUM_PROCESS_FD;
    }

    int fd;
    for (fd = 3; fd < maxNumFds; fd++)
    {
        int result;

        do
        {
            result = close(fd);
        } while ( (result == -1) && (errno == EINTR) );

        if ( (result == -1) && (errno != EBADF) )
        {
            LE_CRIT("Could not close file descriptor.  %m.");
        }
    }
}



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
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(bufPtr == NULL, "Supplied NULL string pointer");
    LE_FATAL_IF(fd < 0, "Supplied invalid file descriptor");

    int bytesRd = 0, tempBufSize = 0, rdReq = bufSize;
    char *tempStr;

    // Requested zero bytes to read, return immediately
    if (bufSize == 0)
    {
        return tempBufSize;
    }

    // TODO: Add timeout, this is needed to detect whether network link died
    do
    {
        tempStr = (char *)(bufPtr);
        tempStr = tempStr + tempBufSize;

        bytesRd = read(fd, tempStr, rdReq);

        if ((bytesRd == -1) && (errno != EINTR))
        {
            LE_ERROR("Error while reading file, errno: %d (%m)", errno);
            return LE_FAULT;
        }

        //Reached End of file, so return what it reads upto EOF
        if (bytesRd == 0)
        {
            return tempBufSize;
        }

        tempBufSize += bytesRd;
        LE_DEBUG("Iterating read, bufsize: %zd , Requested: %d Read: %d", bufSize, rdReq, bytesRd);

        if (tempBufSize < bufSize)
        {
            rdReq = bufSize - tempBufSize;
        }
    }
    while (tempBufSize < bufSize);

    return tempBufSize;
}



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
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(bufPtr == NULL, "Supplied NULL String Pointer");
    LE_FATAL_IF(fd < 0, "Supplied invalid file descriptor");

    int bytesWr = 0, tempBufSize = 0, wrReq = bufSize;
    char *tempStr;

    // Requested zero bytes to write, returns immediately
    if (bufSize == 0)
    {
        return tempBufSize;
    }

    do
    {
        tempStr = (char *)(bufPtr);
        tempStr = tempStr + tempBufSize;

        bytesWr= write(fd, tempStr, wrReq);

        if ((bytesWr == -1) && (errno != EINTR))
        {
            LE_ERROR("Error while writing file, errno: %d (%m)", errno);
            return LE_FAULT;
        }

        tempBufSize += bytesWr;

        LE_DEBUG("Iterating write, bufsize: %zd , Requested: %d Write: %d", bufSize, wrReq, bytesWr);

        if(tempBufSize < bufSize)
        {
            wrReq = bufSize - tempBufSize;
        }
    }
    while (tempBufSize < bufSize);

    return tempBufSize;
}

