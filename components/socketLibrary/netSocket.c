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

#if LE_CONFIG_LINUX
#include <netdb.h>
#endif

#if !LE_CONFIG_RTOS
#include <arpa/inet.h>
#endif

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Port maximum length
 */
//--------------------------------------------------------------------------------------------------
#define PORT_STR_LEN      6
#define TCP_PENDING_CONNECTION 5

//-----------------------------------------------------------------------------------------------
/**
 * Function to populate a socket structure from IP address in string format
 *
 * @note
 *  - srcIpAddress can be a Null string. In this case, the default PDP profile will be used
 *    and the address family will be selected in the following order: Try IPv4 first, then
 *    try IPv6
 *
 * @return
 *        - LE_OK if success
 *        - LE_FAULT in case of failure
 */
//-----------------------------------------------------------------------------------------------
static le_result_t GetSrcSocketInfo
(
    char*                    srcIpAddress,     ///< [IN] Source IP address in string format
    int*                     addrFamilyPtr,    ///< [OUT] Address family
    struct sockaddr_storage* srcSocketPtr      ///< [OUT] Socket pointer for binding info
)
{
    char tmpSrcIpAddress[LE_MDC_IPV6_ADDR_MAX_BYTES] = {0};
    char* tmpSrcIpAddressPtr = srcIpAddress;
    le_result_t result = LE_FAULT;

    if (!strlen(srcIpAddress))
    {
        // No source IP address given - use default profile source address
        le_mdc_ProfileRef_t profileRef = le_mdc_GetProfile((uint32_t)LE_MDC_DEFAULT_PROFILE);
        if(!profileRef)
        {
             LE_ERROR("le_mdc_GetProfile cannot get default profile");
             return result;
        }
        // Try IPv4, then Ipv6
        if (LE_OK == le_mdc_GetIPv4Address(profileRef, tmpSrcIpAddress, sizeof(tmpSrcIpAddress)))
        {
            LE_INFO("GetSrcSocketInfo using default IPv4");
        }
        else if (LE_OK == le_mdc_GetIPv6Address(profileRef, tmpSrcIpAddress,
                                                            sizeof(tmpSrcIpAddress)))
        {
            LE_INFO("GetSrcSocketInfo using default IPv6");
        }
        else
        {
            LE_ERROR("GetSrcSocketInfo No IPv4 or IPv6 address");
            return result;
        }
        tmpSrcIpAddressPtr = tmpSrcIpAddress;
        memcpy(srcIpAddress, tmpSrcIpAddress, LE_MDC_IPV6_ADDR_MAX_BYTES);
    }

    // Get socket address and family from source IP string
    if (inet_pton(AF_INET, tmpSrcIpAddressPtr,
                  &(((struct sockaddr_in *)srcSocketPtr)->sin_addr)) == 1)
    {
        LE_INFO("GetSrcSocketInfo address is IPv4 %s", tmpSrcIpAddressPtr);
        ((struct sockaddr_in *)srcSocketPtr)->sin_family = AF_INET;
        *addrFamilyPtr = AF_INET;
        result = LE_OK;
    }
    else if (inet_pton(AF_INET6, tmpSrcIpAddressPtr,
                       &(((struct sockaddr_in6 *)srcSocketPtr)->sin6_addr)) == 1)
    {
        LE_INFO("GetSrcSocketInfo address is IPv6 %s", tmpSrcIpAddressPtr);
        ((struct sockaddr_in *)srcSocketPtr)->sin_family = AF_INET6;
        *addrFamilyPtr = AF_INET6;
        result = LE_OK;
    }
    else
    {
        LE_ERROR("GetSrcSocketInfo cannot convert address %s", tmpSrcIpAddressPtr);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
// Public functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Initiate a connection with host:port and the given protocol
 *
 * @note
 *  - srcAddrPtr can be a Null string. In this case, the default PDP profile will be used
 *    and the address family will be selected in the following order: Try IPv4 first, then
 *    try IPv6
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
    char*           hostPtr,    ///< [IN] Host address pointer
    uint16_t        port,       ///< [IN] Port number
    char*           srcAddrPtr, ///< [IN] Source address pointer
    SocketType_t    type,       ///< [IN] Socket type (TCP, UDP)
    int*            fdPtr       ///< [OUT] Socket file descriptor
)
{
    char portStr[PORT_STR_LEN] = {0};
    struct addrinfo hints, *addrList, *cur;
    struct sockaddr_storage srcSocket = {0};
    int fd = -1;
    le_result_t result = LE_FAULT;

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

    // Initialize socket structure from source IP string
    if (GetSrcSocketInfo(srcAddrPtr, &(hints.ai_family), &srcSocket) != LE_OK)
    {
        LE_ERROR("Error on function: GetSrcSocketInfo");
        return LE_UNAVAILABLE;
    }

    // Resolve name
    if (getaddrinfo(hostPtr, portStr, &hints, &addrList) != 0)
    {
        LE_ERROR("Error on function: getaddrinfo");
        return LE_UNAVAILABLE;
    }

    // Try the sockaddrs until a connection succeeds
    for (cur = addrList; cur != NULL; cur = cur->ai_next)
    {
        LE_INFO(" netSocket_Connect: family %d, socktype %d", cur->ai_family,
                                                              cur->ai_socktype);

        fd = socket(cur->ai_family, cur->ai_socktype, 0);
        if( fd < 0 )
        {
            LE_ERROR("Cannot create socket");
            continue;
        }

        // Bind socket to source address
        if (bind(fd, (struct sockaddr *)&srcSocket, sizeof(srcSocket)) != 0)
        {
            LE_ERROR("Cannot bind socket");
            close(fd);
            continue;
        }
        if (connect(fd, cur->ai_addr, cur->ai_addrlen) == 0)
        {
            *fdPtr = fd;
            result = LE_OK;
            break;
        }
        LE_ERROR("Cannot connect socket");
        result = LE_COMM_ERROR;
        close(fd);
    }

    freeaddrinfo(addrList);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initiate a server connection by listening on the specified port.
 *
 * @return
 *  - LE_OK             Function success
 *  - LE_BAD_PARAMETER  Invalid parameter
 *  - LE_UNAVAILABLE    Unable to reach the server or DNS issue
 *  - LE_COMM_ERROR     Connection failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_Listen
(
    uint16_t        port,       ///< [IN] Port number
    char*           srcAddrPtr, ///< [IN] Source address pointer
    SocketType_t    type,       ///< [IN] Socket type (TCP, UDP)
    int*            fdPtr       ///< [OUT] Socket file descriptor
)
{
    struct addrinfo serverInfo;
    struct sockaddr_storage srcSocket = {0};
    int fd = -1;

    if ((!srcAddrPtr) || (!fdPtr))
    {
        LE_ERROR("Wrong parameter provided: %p, %p", srcAddrPtr, fdPtr);
        return LE_BAD_PARAMETER;
    }

    // Do name resolution with both IPv6 and IPv4
    memset(&serverInfo, 0, sizeof(serverInfo));
    serverInfo.ai_family = AF_UNSPEC;
    serverInfo.ai_socktype = (type == UDP_TYPE) ? SOCK_DGRAM : SOCK_STREAM;
    serverInfo.ai_protocol = (type == UDP_TYPE) ? IPPROTO_UDP : IPPROTO_TCP;

    // Initialize socket structure from source IP string
    if (GetSrcSocketInfo(srcAddrPtr, &(serverInfo.ai_family), &srcSocket) != LE_OK)
    {
        LE_ERROR("Error on function: GetSrcSocketInfo");
        return LE_UNAVAILABLE;
    }

    if(AF_INET == serverInfo.ai_family)
    {
        ((struct sockaddr_in*)&srcSocket)->sin_port = htons(port);
    }
    else
    {
        ((struct sockaddr_in6*)&srcSocket)->sin6_port = htons(port);
    }

    fd = socket(serverInfo.ai_family, serverInfo.ai_socktype, 0);
    if (fd < 0)
    {
        LE_ERROR("Unable to create a socket");
        return LE_COMM_ERROR;
    }

    // Bind socket to source address
    if(-1 == bind(fd, (struct sockaddr*)&srcSocket, sizeof(struct sockaddr_storage)))
    {
        LE_ERROR("ERROR binding to the socket (%d)", errno);
        close(fd);
        return LE_COMM_ERROR;
    }

    if (-1 == listen(fd, TCP_PENDING_CONNECTION))
    {
        LE_ERROR("ERROR listening to the socket (%d)", errno);
        close(fd);
        return LE_COMM_ERROR;
    }
    *fdPtr = fd;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Accept a client connection from remote host and get the address info
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_UNAVAILABLE   Unable to accept a client socket
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_Accept
(
    int               fd,         ///< [IN] Socket file descriptor
    struct sockaddr*  hostAddr,   ///< [OUT] Accepted host info
    int*              hostFd      ///< [OUT] Accepted socket file descriptor
)
{
    int acceptedFd = -1;
    socklen_t addrLen = sizeof(struct sockaddr_storage);

    acceptedFd = accept(fd, hostAddr, &addrLen);

    if(-1 == acceptedFd)
    {
        LE_ERROR("ERROR accepting the socket (%d)", errno);
        return LE_UNAVAILABLE;
    }

    *hostFd = acceptedFd;
    return LE_OK;
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

#if LE_CONFIG_TARGET_GILL
    struct timeval timeout = {1, 0};
    if(0 != setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)))
    {
        return LE_FAULT;
    }
#endif

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
            LE_ERROR("Write failed: %d, %s", errno, LE_ERRNO_TXT(errno));
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
                LE_ERROR("Read failed: %d, %s", errno, LE_ERRNO_TXT(errno));
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

    LE_INFO("Read size: %"PRIuS, *bufLenPtr);
    return LE_OK;
}
