/** @file fileDescriptor.c
 *
 * Implementation of the framework's internal handy file descriptor manipulation functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

#include "fa/fd.h"

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
        LE_FATAL("Failed to get flags for fd %d. Errno = %d.", fd, errno);
    }
    if (fcntl(fd, F_SETFL, fdFlags | O_NONBLOCK) != 0)
    {
        LE_FATAL("Failed to set O_NONBLOCK on fd %d. Errno = %d.", fd, errno);
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
        LE_FATAL("Failed to get flags for fd %d. Errno = %d.", fd, errno);
    }
    if (fcntl(fd, F_SETFL, fdFlags & (~O_NONBLOCK)) != 0)
    {
        LE_FATAL("Failed to clear O_NONBLOCK on fd %d. Errno = %d.", fd, errno);
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

    result = close(fd);

    if (result != 0)
    {
        if (errno == EINTR)
        {
            LE_WARN("Closing file descriptor '%d' caused EINTR. Proceeding anyway.", fd);
        }
        else
        {
            LE_CRIT("Failed to close file descriptor %d. Errno = %d.", fd, errno);
        }
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

        result = close(fd);

        if ( (result == -1) && (errno != EBADF) )
        {
            LE_CRIT("Could not close file descriptor.  Errno = %d.", errno);
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

    char    *tempStr;
    int      bytesRd = 0;
    size_t   rdReq = bufSize;
    size_t   tempBufSize = 0;

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

        // If resource is temporarily unavailable, try again to read.
        if ((bytesRd == -1) && (errno != EINTR) && (errno != EAGAIN))
        {
            LE_ERROR("Error while reading file, errno: %d", errno);
            return LE_FAULT;
        }

        //Reached End of file, so return what it reads upto EOF
        if (bytesRd == 0)
        {
            return tempBufSize;
        }
        else if (bytesRd > 0)
        {
            tempBufSize += bytesRd;
            LE_DEBUG("Iterating read, bufsize: %" PRIuS " , Requested: %" PRIuS " Read: %d",
                bufSize, rdReq, bytesRd);

            if (tempBufSize < bufSize)
            {
                rdReq = bufSize - tempBufSize;
            }
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

    char    *tempStr;
    int      bytesWr = 0;
    size_t   tempBufSize = 0;
    size_t   wrReq = bufSize;

    // Requested zero bytes to write, returns immediately
    if (bufSize == 0)
    {
        return tempBufSize;
    }

    do
    {
        tempStr = (char *)(bufPtr);
        tempStr = tempStr + tempBufSize;

        bytesWr = write(fd, tempStr, wrReq);

        if ((bytesWr == -1) && (errno != EINTR))
        {
            LE_ERROR("Error while writing file, errno: %d", errno);
            return LE_FAULT;
        }

        tempBufSize += bytesWr;

        LE_DEBUG("Iterating write, bufsize: %" PRIuS " , Requested: %" PRIuS " Write: %d",
            bufSize, wrReq, bytesWr);

        if(tempBufSize < bufSize)
        {
            wrReq = bufSize - tempBufSize;
        }
    }
    while (tempBufSize < bufSize);

    return tempBufSize;
}


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
)
{
    if ( (bufSize == 0) || (buf == NULL) )
    {
        return LE_FAULT;
    }

    size_t index = 0;

    for (; index < bufSize; index++)
    {
        // read one byte at a time.
        char c;
        int result;

        do
        {
            result = read(fd, &c, 1);
        }
        while ( (result == -1) && (errno == EINTR) );

        if (result == 1)
        {
            if (c == '\n')
            {
                // This is the end of the line.  Terminate the string and return.
                buf[index] = '\0';
                return LE_OK;
            }
            else if (index == bufSize - 1)
            {
                // There is still data but we've run out of buffer space.
                buf[index] = '\0';
                return LE_OVERFLOW;
            }

            // Store char.
            buf[index] = c;
        }
        else if (result == 0)
        {
            // End of file nothing else to read.  Terminate the string and return.
            buf[index] = '\0';

            if (index == 0)
            {
                return LE_OUT_OF_RANGE;
            }
            else
            {
                return LE_OK;
            }
        }
        else
        {
            LE_ERROR("Could not read file.  Errno = %d.", errno);
            return LE_FAULT;
        }
    }

    LE_FATAL("Should never get here.");
}


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
)
{
    if (lseek(fd, offset, SEEK_SET) == -1)
    {
        LE_ERROR("Could not seek to address %" PRIxS ".  Errno = %d.", (size_t)offset, errno);
        return LE_FAULT;
    }

    int result;

    do
    {
        result = read(fd, bufPtr, bufSize);
    }
    while ( (result == -1) && (errno == EINTR) );

    if (result == -1)
    {
        LE_ERROR("Could not read file.  Errno = %d.", errno);
        return LE_FAULT;
    }
    else if (result != (int) bufSize)
    {
        LE_ERROR("Unexpected end of file.");
        return LE_FAULT;
    }

    return LE_OK;
}

