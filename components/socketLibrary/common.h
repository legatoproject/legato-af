/**
 * @file common.h
 *

 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_SOCKET_COMMON_H
#define LE_SOCKET_COMMON_H

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of supported socket types
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TCP_TYPE,
    UDP_TYPE
}
SocketType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of Address Family types
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    SOCK_AF_IPV4,   ///< IPV4
    SOCK_AF_IPV6,   ///< IPV6
    SOCK_AF_ANY     ///< Unspecified
}
SocketAfType_t;

#endif /* LE_SOCKET_COMMON_H */
