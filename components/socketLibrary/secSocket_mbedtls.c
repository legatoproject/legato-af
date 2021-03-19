/**
 * @file secSocket_mbedtls.c
 *
 * This file implements a set of networking functions to manage secure TCP/UDP sockets using
 * mbedTLS library.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"

#include "le_socketLib.h"
#include "secSocket.h"
#include "netSocket.h"

#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"

#if defined(MBEDTLS_DEBUG_C)
#include "mbedtls/debug.h"
#define DEBUG_LEVEL                 (0)
#define SSL_DEBUG_LEVEL             (16)
#endif

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Max MbedTLS read retry
 */
//--------------------------------------------------------------------------------------------------
#define MAX_MBEDTLS_SSL_READ_RETRY  20

//--------------------------------------------------------------------------------------------------
/**
 *  MbedTLS connection timeout (in ms)
 *
 *  Read timeout during connection handshaking.  This is needed to accommodate networks that have
 *  longer connection setup times.
 */
//--------------------------------------------------------------------------------------------------
#define MBEDTLS_SSL_CONNECT_TIMEOUT (3 * 10000)

//--------------------------------------------------------------------------------------------------
/**
 * SSL/TLS cipher suites configuration
 * These constants are in line with the ones in ksslcrypto.h.
 */
//--------------------------------------------------------------------------------------------------
#define SSL_MIN_PROFILE_ID          (0)
#define SSL_MAX_PROFILE_ID          (7)

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
 * MbedTLS global context
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    mbedtls_net_context sock;                                   ///< MbedTLS wrapper for socket.
    mbedtls_ssl_context sslCtx;                                 ///< SSL/TLS context.
    mbedtls_ssl_config  sslConf;                                ///< SSL/TLS configuration.
    mbedtls_x509_crt    caCert;                                 ///< Root CA X.509 certificate.
    mbedtls_x509_crt    ownCert;                                ///< Module's X.509 certificate.
    mbedtls_pk_context  ownPkey;                                ///< Module's private key container.
    uint8_t             auth;                                   ///< Authentication type.
    int                 ciphersuite[2];                         ///< Cipher suite(s) to use.
}
MbedtlsCtx_t;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for MbedTLS sockets context.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SocketCtxPoolRef = NULL;
LE_MEM_DEFINE_STATIC_POOL(SocketCtxPool, MAX_SOCKET_NB, sizeof(MbedtlsCtx_t));

//--------------------------------------------------------------------------------------------------
/**
 * Cipher suite mapping based on https://testssl.sh/openssl-iana.mapping.html
 * List of approved cipher suites --- for full list see: Core/include/mbedtls/ssl_ciphersuites.h
 * The list is in line with blocks available in sslCipherSuiteOpts (see ksslcrypto.h)
 *
 * Note: When +ksslcrypto profile index 0 is selected, all approved ciphers below must be included.
 * For the rest of +ksslcrypto profiles, i.e., 1-7, single cipher suite shall be selected.
 */
//--------------------------------------------------------------------------------------------------
const int ciphersuites[SSL_MAX_PROFILE_ID + 1] = {
    MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,   ///< 0xC02F
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM,        ///< 0xC0AC
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CCM,        ///< 0xC0AD
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8,      ///< 0xC0AE
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CCM_8,      ///< 0xC0AF
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256, ///< 0xC02B
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384, ///< 0xC02C
    0
};

//--------------------------------------------------------------------------------------------------
// Static functions
//--------------------------------------------------------------------------------------------------

#if defined(MBEDTLS_DEBUG_C)
//--------------------------------------------------------------------------------------------------
/**
 * Debug callback function of "mbedtls_ssl_conf_dbg()" API
 *
 * To enable mbedtls log messages, you need to uncomment "#define MBEDTLS_DEBUG_C" in
 * frameworkAdaptor/include/mbedtls_port.h
 *
 * @return
 *  - None
 */
//--------------------------------------------------------------------------------------------------
static void OutputMbedtlsDebugInfo(
    void*       ctx,    ///< [IN] Output file handle
    int         level,  ///< [IN] Debug level
    const char* file,   ///< [IN] Source file path
    int         line,   ///< [IN] Source file line number
    const char* str     ///< [IN] Debug message
)
{
    // Extract basename from the file path
    const char *p;
    const char *basename;
    for (p = basename = file; *p != '\0'; p++)
    {
        if (*p == '/' || *p == '\\')
        {
            basename = p + 1;
        }
    }

    fprintf((FILE *)ctx, "%s:%04d: |%d| %s", basename, line, level, str);
    fflush((FILE *)ctx);
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Write to a stream and handle restart if necessary
 *
 * @return
 *  -  0 when all data are written
 *  - < on failure
 */
//--------------------------------------------------------------------------------------------------
static int WriteToStream
(
    mbedtls_ssl_context *sslCtxPtr, ///< [IN] SSL context
    char                *bufferPtr, ///< [IN] Data to be sent
    int                  length     ///< [IN] Data length
)
{
    int r;

    LE_ASSERT(sslCtxPtr != NULL);
    LE_ASSERT(bufferPtr != NULL);

    r = mbedtls_ssl_write(sslCtxPtr, (const unsigned char*)bufferPtr, (size_t)length);
    if (0 >= r)
    {
        char str[256];
        mbedtls_strerror(r, str, sizeof(str));
        LE_ERROR("Error %d on write: %s", r, str);
        return r;
    }

    if (r > 0)
    {
        size_t newLength = length;
        while (r > 0)
        {
            newLength -= r;
            bufferPtr = bufferPtr + r;
            r = mbedtls_ssl_write(sslCtxPtr, (const unsigned char*)bufferPtr, newLength);
        }
    }

    if (0 > r)
    {
        char str[256];
        mbedtls_strerror(r, str, sizeof(str));
        LE_ERROR("Error %d on write: %s", r, str);
    }
    return r;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a from a stream and handle restart if necessary
 *
 * @return
 *  - Data length read
 *  - 0 or negative value on failure
 *  - LWM2MCORE_ERR_TIMEOUT if no data are read
 */
//--------------------------------------------------------------------------------------------------
static int ReadFromStream
(
    mbedtls_ssl_context     *sslCtxPtr, ///< [IN] SSL context
    char                    *bufferPtr, ///< [IN] Buffer
    int                      length     ///< [IN] Buffer length
)
{
    int             r = -1;
    static size_t   count = 0;

    mbedtls_ssl_context* ctxPtr = (mbedtls_ssl_context*)sslCtxPtr;

    while (r < 0)
    {
        r = mbedtls_ssl_read(ctxPtr, (unsigned char*)bufferPtr, (size_t)length);
        if ((r == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) || (r == MBEDTLS_ERR_NET_RECV_FAILED))
        {
            LE_ERROR("Peer close notification or socket unreadable");
            r = 0;
        }
        else if (r == MBEDTLS_ERR_SSL_TIMEOUT)
        {
            LE_ERROR("Timeout on read");
            r = LE_TIMEOUT;
            break;
        }
        else if ((r < 0) && (r != MBEDTLS_ERR_SSL_WANT_READ) && (r != MBEDTLS_ERR_SSL_WANT_WRITE))
        {
            LE_ERROR("Error on MbedTLS ssl read: %d", r);
            break;
        }
        else
        {
            // Count the number of MBEDTLS_ERR_SSL_WANT_READ or MBEDTLS_ERR_SSL_WANT_WRITE
            count++;
        }

        if (count > MAX_MBEDTLS_SSL_READ_RETRY)
        {
            break;
        }

        LE_INFO_IF(r == 0, "Reached the end of the data stream");
    }
    count = 0;
    return r;
}

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
    secSocket_Ctx_t **ctxPtr ///< [OUT] Secure socket context pointer
)
{
    MbedtlsCtx_t *contextPtr;

    LE_ASSERT(ctxPtr != NULL);

    // Alloc memory from pool
    contextPtr = le_mem_Alloc(SocketCtxPoolRef);

    // Initialize the session data
    mbedtls_net_init(&(contextPtr->sock));
    mbedtls_ssl_init(&(contextPtr->sslCtx));
    mbedtls_ssl_config_init(&(contextPtr->sslConf));
    mbedtls_x509_crt_init(&(contextPtr->caCert));
    mbedtls_x509_crt_init(&(contextPtr->ownCert));
    mbedtls_pk_init(&(contextPtr->ownPkey));
    contextPtr->auth = AUTH_SERVER;

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_ssl_conf_dbg(&(contextPtr->sslConf), OutputMbedtlsDebugInfo, stdout);
    mbedtls_debug_set_threshold(SSL_DEBUG_LEVEL);
#endif

    *ctxPtr = (secSocket_Ctx_t *) contextPtr;

    return LE_OK;
}

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
)
{
    int              ret;
    MbedtlsCtx_t    *contextPtr = (MbedtlsCtx_t *) ctxPtr;

    LE_ASSERT(contextPtr != NULL);
    LE_ASSERT(certificatePtr != NULL);
    LE_ASSERT(certificateLen > 0);

    LE_DEBUG("Add root CA certificates: %p Len:%"PRIuS, certificatePtr, certificateLen);
    ret = mbedtls_x509_crt_parse(&(contextPtr->caCert), certificatePtr, certificateLen);
    if (ret < 0)
    {
        LE_ERROR("Failed! mbedtls_x509_crt_parse returned -0x%x", -ret);
        return LE_FAULT;
    }

    // Check certificate validity
    if ((mbedtls_x509_time_is_past(&contextPtr->caCert.valid_to)) ||
        (mbedtls_x509_time_is_future(&contextPtr->caCert.valid_from)))
    {
        LE_ERROR("Current root CA certificates expired, please add valid certificates");
        return LE_FORMAT_ERROR;
    }

    return LE_OK;
}

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
)
{
    int              ret;
    MbedtlsCtx_t    *contextPtr = (MbedtlsCtx_t *) ctxPtr;

    LE_ASSERT(contextPtr != NULL);
    LE_ASSERT(certificatePtr != NULL);
    LE_ASSERT(certificateLen > 0);

    LE_DEBUG("Add client certificates: %p Len:%"PRIuS, certificatePtr, certificateLen);
    ret = mbedtls_x509_crt_parse(&(contextPtr->ownCert), certificatePtr, certificateLen);
    if (ret < 0)
    {
        LE_ERROR("Failed! mbedtls_x509_crt_parse returned -0x%x", -ret);
        return LE_FAULT;
    }

    // Check certificate validity
    if ((mbedtls_x509_time_is_past(&contextPtr->ownCert.valid_to)) ||
        (mbedtls_x509_time_is_future(&contextPtr->ownCert.valid_from)))
    {
        LE_ERROR("Current client certificates expired, please add valid certificates");
        return LE_FORMAT_ERROR;
    }

    return LE_OK;
}

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
)
{
    int              ret;
    MbedtlsCtx_t    *contextPtr = (MbedtlsCtx_t *) ctxPtr;

    LE_ASSERT(contextPtr != NULL);
    LE_ASSERT(pkeyPtr != NULL);
    LE_ASSERT(pkeyLen > 0);

    LE_DEBUG("Add client private key: %p Len:%"PRIuS, pkeyPtr, pkeyLen);
    ret = mbedtls_pk_parse_key(&(contextPtr->ownPkey), pkeyPtr, pkeyLen, NULL, 0);
    if (ret < 0)
    {
        LE_ERROR("Failed! mbedtls_pk_parse_key returned -0x%x", -ret);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the cipher suites to the secure socket context.
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_FAULT         Failure
 */
//--------------------------------------------------------------------------------------------------
void secSocket_SetCipherSuites
(
    secSocket_Ctx_t*  ctxPtr,           ///< [INOUT] Secure socket context pointer
    uint8_t           cipherIdx         ///< [IN] Cipher suites index
)
{
    MbedtlsCtx_t    *contextPtr = (MbedtlsCtx_t *) ctxPtr;

    LE_ASSERT(contextPtr != NULL);
    LE_ASSERT(cipherIdx <= SSL_MAX_PROFILE_ID);

    // If +ksslcrypto profile index 0 is selected, the module would send all the approved cipher
    // suites to the server, so the server can check its own supported cipher suites and pick one
    // that is supported by both parties. If other profile index is selected, the module would send
    // the specified cipher suite to the server.
    contextPtr->ciphersuite[0] = cipherIdx == 0 ? 0 : ciphersuites[cipherIdx - 1];
    contextPtr->ciphersuite[1] = 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set authentication type to the secure socket context.
 */
//--------------------------------------------------------------------------------------------------
void secSocket_SetAuthType
(
    secSocket_Ctx_t*  ctxPtr,           ///< [INOUT] Secure socket context pointer
    uint8_t           auth              ///< [IN] Authentication type
)
{
    MbedtlsCtx_t    *contextPtr = (MbedtlsCtx_t *) ctxPtr;

    LE_ASSERT(contextPtr != NULL);
    LE_ASSERT(auth == AUTH_SERVER || auth == AUTH_MUTUAL);

    contextPtr->auth = auth;
}

//--------------------------------------------------------------------------------------------------
/**
 * Performs TLS Handshake
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
)
{
    int              ret;
    MbedtlsCtx_t    *contextPtr = (MbedtlsCtx_t *) ctxPtr;

    LE_ASSERT(contextPtr != NULL);
    LE_ASSERT(hostPtr != NULL);
    LE_ASSERT(fd != -1);

    // Set the secure socket fd to the original netsocket fd
    contextPtr->sock.fd = fd;

    // Setup
    LE_INFO("Set up the default SSL/TLS configuration");
    if ((ret = mbedtls_ssl_config_defaults(&(contextPtr->sslConf),
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        LE_ERROR("Failed! mbedtls_ssl_config_defaults returned %d", ret);
        // Only possible error is linked to memory allocation issue
        return LE_NO_MEMORY;
    }

    if (contextPtr->ciphersuite[0] == 0)
    {
        LE_INFO("Add all approved cipher suites to SSL/TLS configuration");
        mbedtls_ssl_conf_ciphersuites(&contextPtr->sslConf, ciphersuites);
    }
    else
    {
        LE_INFO("Add cipher suite '%d' to SSL/TLS configuration", contextPtr->ciphersuite[0]);
        mbedtls_ssl_conf_ciphersuites(&contextPtr->sslConf, contextPtr->ciphersuite);
    }

    mbedtls_ssl_conf_authmode(&(contextPtr->sslConf), MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&(contextPtr->sslConf), &(contextPtr->caCert), NULL);

    // Apply local certificate and key bindings if the authentication type is mutual
    if (contextPtr->auth == AUTH_MUTUAL)
    {
        if ((ret = mbedtls_ssl_conf_own_cert(&(contextPtr->sslConf),
                                             &(contextPtr->ownCert),
                                             &(contextPtr->ownPkey))) != 0)
        {
            LE_ERROR("Failed! mbedtls_ssl_conf_own_cert returned %d", ret);
            return LE_FAULT;
        }
    }

    mbedtls_port_SSLSetRNG(&contextPtr->sslConf);

    if ((ret = mbedtls_ssl_setup(&(contextPtr->sslCtx), &(contextPtr->sslConf))) != 0)
    {
        LE_ERROR("Failed! mbedtls_ssl_setup returned %d", ret);
        if (MBEDTLS_ERR_SSL_ALLOC_FAILED == ret)
        {
            return LE_NO_MEMORY;
        }
        return LE_FAULT;
    }

    if ((ret = mbedtls_ssl_set_hostname(&(contextPtr->sslCtx), hostPtr)) != 0)
    {
        LE_ERROR("Failed! mbedtls_ssl_set_hostname returned %d", ret);
        if (MBEDTLS_ERR_SSL_ALLOC_FAILED == ret)
        {
            return LE_NO_MEMORY;
        }
        return LE_FAULT;
    }

    mbedtls_ssl_set_bio(&(contextPtr->sslCtx), &(contextPtr->sock),
                        mbedtls_net_send, NULL, mbedtls_net_recv_timeout);

    // Set the timeout for the initial handshake.
    mbedtls_ssl_conf_read_timeout(&(contextPtr->sslConf), MBEDTLS_SSL_CONNECT_TIMEOUT);

    // Handshake
    LE_INFO("Performing the SSL/TLS handshake...");
    while ((ret = mbedtls_ssl_handshake(&(contextPtr->sslCtx))) != 0)
    {
        LE_ERROR("Failed! mbedtls_ssl_handshake returned -0x%x", -ret);
        if ((ret != MBEDTLS_ERR_SSL_WANT_READ) && (ret != MBEDTLS_ERR_SSL_WANT_WRITE))
        {
            if (ret == MBEDTLS_ERR_NET_RECV_FAILED)
            {
                return LE_TIMEOUT;
            }
            else if (ret == MBEDTLS_ERR_NET_SEND_FAILED)
            {
                return LE_FAULT;
            }
            else if (ret == MBEDTLS_ERR_SSL_ALLOC_FAILED)
            {
                return LE_NO_MEMORY;
            }
            else if (ret == MBEDTLS_ERR_SSL_CONN_EOF)
            {
                return LE_CLOSED;
            }
            else
            {
                return LE_FAULT;
            }
        }
    }

    return LE_OK;
}

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
)
{
    char             portBuffer[6] = {0};
    MbedtlsCtx_t    *contextPtr = (MbedtlsCtx_t *) ctxPtr;

    LE_ASSERT(contextPtr != NULL);
    LE_ASSERT(hostPtr != NULL);
    LE_ASSERT(fdPtr != NULL);

    // Start the connection
    snprintf(portBuffer, sizeof(portBuffer), "%d", port);
    LE_INFO("Connecting to %d/%s:%d - %s:%s...", type, hostPtr, port, hostPtr, portBuffer);

    le_result_t result = netSocket_Connect(hostPtr, port, srcAddrPtr, TCP_TYPE,
                                           (int *)&(contextPtr->sock));
    if ( result != LE_OK)
    {
        LE_ERROR("Failed! mbedtls_net_connect returned %d", result);
        return result;
    }

    //Get the file descriptor
    *fdPtr = contextPtr->sock.fd;
    LE_DEBUG("File descriptor: %d", *fdPtr);

    return secSocket_PerformHandshake(ctxPtr, hostPtr, contextPtr->sock.fd);
}

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
    secSocket_Ctx_t *ctxPtr ///< [INOUT] Secure socket context pointer
)
{
    MbedtlsCtx_t *contextPtr = (MbedtlsCtx_t *) ctxPtr;
    int status;
    LE_ASSERT(contextPtr != NULL);

    status = mbedtls_ssl_close_notify(&(contextPtr->sslCtx));
    if (status != 0)
    {
        LE_ERROR("Failed to close SSL connection. Error: %d", status);
    }
    mbedtls_net_free(&(contextPtr->sock));

    if (status == 0)
    {
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
}

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
    secSocket_Ctx_t *ctxPtr ///< [IN] Secure socket context pointer
)
{
    MbedtlsCtx_t *contextPtr = (MbedtlsCtx_t *) ctxPtr;

    LE_ASSERT(contextPtr != NULL);

    mbedtls_net_free(&(contextPtr->sock));
    mbedtls_ssl_free(&(contextPtr->sslCtx));
    mbedtls_ssl_config_free(&(contextPtr->sslConf));
    mbedtls_x509_crt_free(&(contextPtr->caCert));
    mbedtls_x509_crt_free(&(contextPtr->ownCert));
    mbedtls_pk_free(&(contextPtr->ownPkey));

    le_mem_Release(contextPtr);
    return LE_OK;
}

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
    secSocket_Ctx_t *ctxPtr,    ///< [INOUT] Secure socket context pointer
    char            *dataPtr,   ///< [IN] Data pointer
    size_t           dataLen    ///< [IN] Data length
)
{
    MbedtlsCtx_t *contextPtr = (MbedtlsCtx_t *) ctxPtr;

    LE_ASSERT(contextPtr != NULL);
    LE_ASSERT(dataPtr != NULL);

    if (0 > WriteToStream(&(contextPtr->sslCtx), dataPtr, (int)dataLen))
    {
        return LE_FAULT;
    }

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
le_result_t secSocket_Read
(
    secSocket_Ctx_t *ctxPtr,        ///< [INOUT] Secure socket context pointer
    char            *dataPtr,       ///< [INOUT] Data pointer
    size_t          *dataLenPtr,    ///< [IN] Data length pointer
    uint32_t         timeout        ///< [IN] Read timeout in milliseconds.
)
{
    int           count;
    MbedtlsCtx_t *contextPtr = (MbedtlsCtx_t *) ctxPtr;

    LE_ASSERT(contextPtr != NULL);
    LE_ASSERT(dataPtr != NULL);
    LE_ASSERT(dataLenPtr != NULL);

    mbedtls_ssl_conf_read_timeout(&(contextPtr->sslConf), timeout);
    count = ReadFromStream(&(contextPtr->sslCtx), dataPtr, (int)*dataLenPtr);
    if (count == LE_TIMEOUT)
    {
        return LE_TIMEOUT;
    }
    else if (count <= 0)
    {
        LE_INFO("ERROR on reading data from stream");
        return LE_FAULT;
    }

    *dataLenPtr = count;
    return LE_OK;
}

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
    secSocket_Ctx_t *ctxPtr ///< [IN] Secure socket context pointer
)
{
    MbedtlsCtx_t *contextPtr = (MbedtlsCtx_t *) ctxPtr;

    LE_ASSERT(contextPtr != NULL);
    return (mbedtls_ssl_get_bytes_avail(&(contextPtr->sslCtx)) != 0);
}


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
)
{
    // Initialize the socket context pool
    SocketCtxPoolRef = le_mem_InitStaticPool(SocketCtxPool,
                                             MAX_SOCKET_NB,
                                             sizeof(MbedtlsCtx_t));
}
