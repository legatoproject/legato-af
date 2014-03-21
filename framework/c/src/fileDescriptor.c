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
 * Closes all file descriptors in the calling process.
 */
//--------------------------------------------------------------------------------------------------
void fd_CloseAll
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

    // @todo Leaves the standard fds open for testing for now.  Need to remove this later.
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

