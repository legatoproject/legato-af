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

//--------------------------------------------------------------------------------------------------
/**
 * Authentication types, which is the 7th parameter in the ksslcrypto write command.
 */
//--------------------------------------------------------------------------------------------------
typedef enum AuthType
{
    AUTH_SERVER  = 1,
    AUTH_MUTUAL  = 3,
    AUTH_UNKNOWN = 0xFF
}
AuthType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Max MbedTLS ALPN Protocol List size
 */
//--------------------------------------------------------------------------------------------------
#define ALPN_LIST_SIZE       1

#endif /* LE_SOCKET_COMMON_H */
