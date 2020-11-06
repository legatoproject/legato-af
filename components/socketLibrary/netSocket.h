/**
 * @file netSocket.h
 *
 * Networking functions definition for unsecure TCP/UDP sockets.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_NET_SOCKET_LIB_H
#define LE_NET_SOCKET_LIB_H

#include "legato.h"
#include "interfaces.h"
#include "common.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

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
 *  - LE_COMM_ERROR    If connection failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_Connect
(
    char*           hostPtr,    ///< [IN] Host address pointer
    uint16_t        port,       ///< [IN] Port number
    char*           srcAddrPtr, ///< [IN] Source address pointer
    SocketType_t    type,       ///< [IN] Socket type (TCP, UDP)
    int*            fdPtr       ///< [OUT] Socket file descriptor
);

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
);

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
);

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
);

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
);

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
);

#endif /* LE_NET_SOCKET_LIB_H */
