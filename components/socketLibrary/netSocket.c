/**
 * @file netSocket.c
 *
 * This file implements a set of networking functions to manage unsecure TCP/UDP sockets.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "netSocket.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Port maximum length
 */
//--------------------------------------------------------------------------------------------------
#define PORT_STR_LEN      6


//--------------------------------------------------------------------------------------------------
// Public functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Initiate a connection with host:port and the given protocol
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_UNAVAILABLE   Unable to reach the server or DNS issue
 *  - LE_FAULT         Internal error
 *  - LE_COMM_ERROR    Connection failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_Connect
(
    char*         hostPtr,    ///< [IN] Host address pointer
    uint16_t      port,       ///< [IN] Port number
    SocketType_t  type,       ///< [IN] Socket type (TCP, UDP)
    int*          fdPtr       ///< [OUT] Socket file descriptor
)
{
    char portStr[PORT_STR_LEN] = {0};
    struct addrinfo hints, *addrList, *cur;
    int fd = -1;

    if ((!hostPtr) || (!fdPtr))
    {
        LE_ERROR("Wrong parameter provided: %p, %p", hostPtr, fdPtr);
        return LE_BAD_PARAMETER;
    }

    // Convert port to string
    snprintf(portStr, PORT_STR_LEN, "%hu", port);

    // Do name resolution with both IPv6 and IPv4
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = (type == UDP_TYPE) ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = (type == UDP_TYPE) ? IPPROTO_UDP : IPPROTO_TCP;

    if(getaddrinfo(hostPtr, portStr, &hints, &addrList) != 0)
    {
        LE_ERROR("Error on function: getaddrinfo");
        return LE_UNAVAILABLE;
    }

    // Try the sockaddrs until a connection succeeds
    for(cur = addrList; cur != NULL; cur = cur->ai_next)
    {
        fd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);

        if( fd < 0 )
        {
            continue;
        }

        if( connect(fd, cur->ai_addr, cur->ai_addrlen) == 0 )
        {
            break;
        }

        LE_ERROR("Error on function: connect");
        close(fd);
        freeaddrinfo(addrList);
        return LE_COMM_ERROR;
    }

    freeaddrinfo( addrList );

    // Check file descriptor
    if (fd == -1)
    {
        LE_ERROR("Unable to create a socket");
        return LE_FAULT;
    }
    else
    {
        *fdPtr = fd;
        return LE_OK;

    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Gracefully close the socket connection
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_Disconnect
(
    int fd    ///< [IN] Socket file descriptor
)
{
    if (fd == -1)
    {
        return LE_FAULT;
    }

    close(fd);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write to the socket file descriptor an amount of data in a blocking way
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_Write
(
    int     fd,        ///< [IN] Socket file descriptor
    char*   bufPtr,    ///< [IN] Data pointer to be sent
    size_t  bufLen     ///< [IN] Size of data to be sent
)
{
    ssize_t count;

    if ((!bufPtr) || (fd < 0))
    {
        return LE_BAD_PARAMETER;
    }

    while (bufLen)
    {
        count = write(fd, bufPtr, bufLen);
        if (count >= 0)
        {
            bufLen -= count;
            bufPtr += count;
        }
        else if (-1 == count && (EINTR == errno))
        {
            continue;
        }
        else
        {
            LE_ERROR("Write failed: %d, %s", errno, strerror(errno));
            return LE_FAULT;
        }
    }

    LE_INFO("Write done successfully on fd: %d", fd);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read data from the socket file descriptor in a blocking way. If the timeout is zero, then the
 * API returns immediatly.
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 *  - LE_TIMEOUT       Timeout during execution
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_Read
(
    int     fd,           ///< [IN] Socket file descriptor
    char*   bufPtr,       ///< [IN] Read buffer pointer
    size_t*  bufLenPtr,   ///< [INOUT] Input: size of the buffer. Output: data size read
    uint32_t timeout      ///< [IN] Read timeout in milliseconds.
)
{
    fd_set set;
    int rv, count;
    struct timeval time = {.tv_sec = timeout / 1000, .tv_usec = (timeout % 1000) * 1000};

    if ((!bufPtr) || (!bufLenPtr) || (fd < 0))
    {
        return LE_BAD_PARAMETER;
    }

    do
    {
       FD_ZERO(&set);
       FD_SET(fd, &set);
       rv = select(fd + 1, &set, NULL, NULL, &time);
    }
    while (rv == -1 && errno == EINTR);

    if (rv > 0)
    {
        if (FD_ISSET(fd, &set))
        {
            do
            {
               count = recv(fd, bufPtr, *bufLenPtr, 0);
            }
            while (count == -1 && errno == EINTR);

            if (count < 0)
            {
                LE_ERROR("Read failed: %d, %s", errno, strerror(errno));
                return LE_FAULT;
            }

            *bufLenPtr = count;
        }
    }
    else if (rv == 0)
    {
        return LE_TIMEOUT;
    }
    else
    {
        return LE_FAULT;
    }

    LE_INFO("Read size: %zu", *bufLenPtr);
    return LE_OK;
}
