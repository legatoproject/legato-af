/**
 * @file secSocket.h
 *
 * Networking functions definition for secure TCP/UDP sockets using secure library.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_SEC_SOCKET_LIB_H
#define LE_SEC_SOCKET_LIB_H

#include "legato.h"
#include "interfaces.h"
#include "common.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Secure socket handler context definition
 */
//--------------------------------------------------------------------------------------------------
typedef void secSocket_Ctx_t;

//--------------------------------------------------------------------------------------------------
// Public functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Initialize a secure socket using the input certificate.
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t secSocket_Init
(
    secSocket_Ctx_t**  ctxPtr       ///< [INOUT] Secure socket context pointer
);

//--------------------------------------------------------------------------------------------------
/**
 * Add root CA certificates to the secure socket context.
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_FORMAT_ERROR  Invalid certificate
 *  - LE_FAULT         Failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t secSocket_AddCertificate
(
    secSocket_Ctx_t*  ctxPtr,           ///< [INOUT] Secure socket context pointer
    const uint8_t*    certificatePtr,   ///< [IN] Certificate pointer
    size_t            certificateLen    ///< [IN] Certificate length
);

//--------------------------------------------------------------------------------------------------
/**
 * Add the module's own certificates to the secure socket context for mutual authentication.
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_FORMAT_ERROR  Invalid certificate
 *  - LE_FAULT         Failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t secSocket_AddOwnCertificate
(
    secSocket_Ctx_t*  ctxPtr,           ///< [INOUT] Secure socket context pointer
    const uint8_t*    certificatePtr,   ///< [IN] Certificate pointer
    size_t            certificateLen    ///< [IN] Certificate length
);

//--------------------------------------------------------------------------------------------------
/**
 * Add the module's own private key to the secure socket context for mutual authentication.
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_FAULT         Failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t secSocket_AddOwnPrivateKey
(
    secSocket_Ctx_t*  ctxPtr,           ///< [INOUT] Secure socket context pointer
    const uint8_t*    pkeyPtr,          ///< [IN] Private key pointer
    size_t            pkeyLen           ///< [IN] Private key length
);

//--------------------------------------------------------------------------------------------------
/**
 * Set cipher suites to the secure socket context.
 */
//--------------------------------------------------------------------------------------------------
void secSocket_SetCipherSuites
(
    secSocket_Ctx_t*  ctxPtr,           ///< [INOUT] Secure socket context pointer
    uint8_t           cipherIdx         ///< [IN] Cipher suites index
);

//--------------------------------------------------------------------------------------------------
/**
 * Set authentication type to the secure socket context.
 */
//--------------------------------------------------------------------------------------------------
void secSocket_SetAuthType
(
    secSocket_Ctx_t*  ctxPtr,           ///< [INOUT] Secure socket context pointer
    uint8_t           auth              ///< [IN] Authentication type
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the ALPN protocol list in the secure socket context.
 */
//--------------------------------------------------------------------------------------------------
void secSocket_SetAlpnProtocolList
(
    secSocket_Ctx_t*  ctxPtr,           ///< [INOUT] Secure socket context pointer
    const char**      alpnList          ///< [IN] ALPN protocol list pointer
);

//--------------------------------------------------------------------------------------------------
/**
 * Performs TLS Handshake
 *
 * Warning: Deprecated function. Use secSocket_Connect() to connect remote host and doing handshake.
 *
 * @return
 *  - LE_OK                 The function succeeded
 *  - LE_NOT_IMPLEMENTED    Not implemented for device
 *  - LE_TIMEOUT            Timeout during execution
 *  - LE_FAULT              Internal error
 *  - LE_NO_MEMORY          Memory allocation issue
 *  - LE_CLOSED             In case of end of file error
 */
//--------------------------------------------------------------------------------------------------
le_result_t secSocket_PerformHandshake
(
    secSocket_Ctx_t*    ctxPtr,    ///< [INOUT] Secure socket context pointer
    char*               hostPtr,   ///< [IN] Host to connect on
    int                 fd         ///< [IN] File descriptor
);

//--------------------------------------------------------------------------------------------------
/**
 * Initiate a connection with host:port and the given protocol
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_TIMEOUT       Timeout during execution
 *  - LE_UNAVAILABLE   Unable to reach the server or DNS issue
 *  - LE_FAULT         Internal error
 *  - LE_NO_MEMORY     Memory allocation issue
 *  - LE_CLOSED        In case of end of file error
 *  - LE_COMM_ERROR    Connection failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t secSocket_Connect
(
    secSocket_Ctx_t* ctxPtr,     ///< [INOUT] Secure socket context pointer
    char*            hostPtr,    ///< [IN] Host to connect on
    uint16_t         port,       ///< [IN] Port to connect on
    char*            srcAddrPtr, ///< [IN] Source address pointer
    SocketType_t     type,       ///< [IN] Socket type (TCP, UDP)
    int*             fdPtr       ///< [OUT] Socket file descriptor
);

//--------------------------------------------------------------------------------------------------
/**
 * Gracefully close the socket connection while keeping the SSL configuration.
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_BAD_PARAMETER Invalid parameter
 */
//--------------------------------------------------------------------------------------------------
le_result_t secSocket_Disconnect
(
    secSocket_Ctx_t* ctxPtr      ///< [INOUT] Secure socket context pointer
);

//--------------------------------------------------------------------------------------------------
/**
 * Free the last connection resources including the certificate and SSL configuration.
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_BAD_PARAMETER Invalid parameter
 */
//--------------------------------------------------------------------------------------------------
le_result_t secSocket_Delete
(
    secSocket_Ctx_t* ctxPtr      ///< [INOUT] Secure socket context pointer
);

//--------------------------------------------------------------------------------------------------
/**
 * Write an amount of data to the secure socket.
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t secSocket_Write
(
    secSocket_Ctx_t* ctxPtr,      ///< [INOUT] Secure socket context pointer
    const char*      dataPtr,     ///< [IN] Data pointer
    size_t           dataLen      ///< [IN] Data length
);

//--------------------------------------------------------------------------------------------------
/**
 * Read data from the socket file descriptor in a blocking way. If the timeout is zero, then the
 * API returns immediatly.
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_IN_PROGRESS   Secure HandShake still in progress
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 *  - LE_TIMEOUT       Timeout during execution
 */
//--------------------------------------------------------------------------------------------------
le_result_t secSocket_Read
(
    secSocket_Ctx_t* ctxPtr,       ///< [INOUT] Secure socket context pointer
    char*            dataPtr,      ///< [INOUT] Data pointer
    size_t*          dataLenPtr,   ///< [IN] Data length pointer
    uint32_t         timeout       ///< [IN] Read timeout in milliseconds.
);

//--------------------------------------------------------------------------------------------------
/**
 * Check if data is available to be read
 *
 * @return
 *  - True if data is available to be read, false otherwise
 */
//--------------------------------------------------------------------------------------------------
bool secSocket_IsDataAvailable
(
    secSocket_Ctx_t* ctxPtr       ///< [INOUT] Secure socket context pointer
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the cipher suites to the secure socket context.
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_FAULT         Failure
 */
//--------------------------------------------------------------------------------------------------
void secSocket_SetTlsVersion
(
    secSocket_Ctx_t*  ctxPtr,           ///< [INOUT] Secure socket context pointer
    uint8_t           tlsVersion        ///< [IN] Supported TLS version (Minor version number)
);

//--------------------------------------------------------------------------------------------------
/**
 * Get tls error code
 *
 * @note Get tls error code
 *
 * @return
 *  - INT tls error code
 */
//--------------------------------------------------------------------------------------------------
int secSocket_GetTlsErrorCode
(
    secSocket_Ctx_t *ctxPtr     ///< [IN] Secure socket context pointer
);

//--------------------------------------------------------------------------------------------------
/**
 * Set tls error code
 *
 * @note Set tls error code
 *
 */
//--------------------------------------------------------------------------------------------------
void secSocket_SetTlsErrorCode
(
    secSocket_Ctx_t *ctxPtr,     ///< [IN] Secure socket context pointer
    int             err_code     ///< [IN] INT error code
);

//--------------------------------------------------------------------------------------------------
/**
 * One-time init for Secure Socket component
 *
 * This pre-initializes the secSocket memory pools.
 *
 */
//--------------------------------------------------------------------------------------------------
void secSocket_InitializeOnce
(
    void
);

#endif /* LE_SEC_SOCKET_LIB_H */
