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
    char*         hostPtr,    ///< [IN] Host address pointer
    uint16_t      port,       ///< [IN] Port number
    SocketType_t  type,       ///< [IN] Socket type (TCP, UDP)
    int*          fdPtr       ///< [OUT] Socket file descriptor
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