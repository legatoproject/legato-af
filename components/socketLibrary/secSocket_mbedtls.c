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

#ifdef MK_CONFIG_THIN_MODEM
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#endif

#if !LE_CONFIG_RTOS
#include <arpa/inet.h>
#endif

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
 * Port maximum length
 */
//--------------------------------------------------------------------------------------------------
#define PORT_STR_LEN      6

//--------------------------------------------------------------------------------------------------
/**
 * MbedTLS global context
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    mbedtls_net_context   sock;                             ///< MbedTLS wrapper for socket.
    mbedtls_ssl_context   sslCtx;                           ///< SSL/TLS context.
    mbedtls_ssl_config    sslConf;                          ///< SSL/TLS configuration.
    mbedtls_x509_crt      caCert;                           ///< Root CA X.509 certificate.
    mbedtls_x509_crt      ownCert;                          ///< Module's X.509 certificate.
    mbedtls_pk_context    ownPkey;                          ///< Module's private key container.

#ifdef MK_CONFIG_THIN_MODEM
    mbedtls_entropy_context entropy;                        ///< Entropy for random number generator.
    mbedtls_ctr_drbg_context ctrDrbg;                       ///< Random number generator context
    /* There are the high and low bytes of ProtocolVersion as defined by:
     * - RFC 5246: ProtocolVersion version = { 3, 3 };     // TLS v1.2
     * - RFC 5246: ProtocolVersion version = { 3, 4 };     // TLS v1.4
     * - RFC 8446: see section 4.2.1
     * As Major version number is same for both TLS v1.2 and v1.3, here we will only keep track of
     * minor version.
     */
    uint8_t               tlsVersion;                       ///< TLS version (minor version)
#endif

    uint8_t               auth;                             ///< Authentication type.
    const char          **alpn_list;                        ///< ALPN Protocol Name list
    int                   ciphersuite[2];                   ///< Cipher suite(s) to use.
    int                   mbedtls_errcode;                  ///< MbedTLS error codes.
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
    const char          *bufferPtr, ///< [IN] Data to be sent
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
    LE_DEBUG("Requested read length: %d", length);

    while (r < 0)
    {
        r = mbedtls_ssl_read(ctxPtr, (unsigned char*)bufferPtr, (size_t)length);

        LE_INFO_IF(r == 0, "Reached the end of the data stream");
        if ((r == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) || (r == MBEDTLS_ERR_NET_RECV_FAILED))
        {
            LE_ERROR("Peer close notification or socket unreadable");
            break;
        }
        else if (r == MBEDTLS_ERR_SSL_TIMEOUT)
        {
            LE_ERROR("Timeout on read");
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
    }
    count = 0;
    return r;
}
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
static le_result_t GetSocketInfo
(
    char*                    ipAddress,        ///< [IN] IP address in string format
    int*                     addrFamilyPtr,    ///< [OUT] Address family
    struct sockaddr_storage* socketPtr         ///< [OUT] Socket pointer for binding info
)
{
    char tmpIpAddress[LE_MDC_IPV6_ADDR_MAX_BYTES] = {0};
    char* tmpIpAddressPtr = ipAddress;
    le_result_t result = LE_FAULT;

    if (!strlen(ipAddress))
    {
        // No source IP address given - use default profile source address
        le_mdc_ProfileRef_t profileRef = le_mdc_GetProfile((uint32_t)LE_MDC_DEFAULT_PROFILE);
        if(!profileRef)
        {
             LE_ERROR("le_mdc_GetProfile cannot get default profile");
             return result;
        }
        // Try IPv4, then Ipv6
        if (LE_OK == le_mdc_GetIPv4Address(profileRef, tmpIpAddress, sizeof(tmpIpAddress)))
        {
            LE_INFO("GetSocketInfo using default IPv4");
        }
        else if (LE_OK == le_mdc_GetIPv6Address(profileRef, tmpIpAddress,
                                                            sizeof(tmpIpAddress)))
        {
            LE_INFO("GetSocketInfo using default IPv6");
        }
        else
        {
            LE_ERROR("GetSocketInfo No IPv4 or IPv6 address");
            return result;
        }
        tmpIpAddressPtr = tmpIpAddress;
        memcpy(ipAddress, tmpIpAddress, LE_MDC_IPV6_ADDR_MAX_BYTES);
    }

    // Get socket address and family from source IP string
    if (inet_pton(AF_INET, tmpIpAddressPtr,
                  &(((struct sockaddr_in *)socketPtr)->sin_addr)) == 1)
    {
        LE_INFO("GetSocketInfo address is IPv4 %s", tmpIpAddressPtr);
        ((struct sockaddr_in *)socketPtr)->sin_family = AF_INET;
        *addrFamilyPtr = AF_INET;
        result = LE_OK;
    }
    else if (inet_pton(AF_INET6, tmpIpAddressPtr,
                       &(((struct sockaddr_in6 *)socketPtr)->sin6_addr)) == 1)
    {
        LE_INFO("GetSocketInfo address is IPv6 %s", tmpIpAddressPtr);
        ((struct sockaddr_in *)socketPtr)->sin_family = AF_INET6;
        *addrFamilyPtr = AF_INET6;
        result = LE_OK;
    }
    else
    {
        LE_ERROR("GetSocketInfo cannot convert address %s", tmpIpAddressPtr);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Setup TLS parameters
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
le_result_t SetupTLSParams
(
    secSocket_Ctx_t*    ctxPtr,    ///< [INOUT] Secure socket context pointer
    char*               hostPtr    ///< [IN] Host to connect on
)
{
    int              ret;
    MbedtlsCtx_t    *contextPtr = (MbedtlsCtx_t *) ctxPtr;
    uint8_t isTls13Higher = 0;     ///< Whether TLS v1.3 or higher is used

    LE_ASSERT(contextPtr != NULL);
    LE_ASSERT(hostPtr != NULL);

#ifdef MK_CONFIG_THIN_MODEM
    isTls13Higher = contextPtr->tlsVersion > MBEDTLS_SSL_MINOR_VERSION_3 ? 1 : 0;
#endif

    LE_INFO("Setting up TLS parameters");
    // Setup
    LE_INFO("Set up the default SSL/TLS configuration");
    if ((ret = mbedtls_ssl_config_defaults(&(contextPtr->sslConf),
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        contextPtr->mbedtls_errcode = ret;
        LE_ERROR("Failed! mbedtls_ssl_config_defaults returned %d", ret);
        // Only possible error is linked to memory allocation issue
        return LE_NO_MEMORY;
    }

    // Not ciphersuite should be set when tls version is 1.3 or higher
    if (!isTls13Higher)
    {
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
    }

    if (contextPtr->auth == AUTH_MUTUAL)
    {
        mbedtls_ssl_conf_authmode(&(contextPtr->sslConf), MBEDTLS_SSL_VERIFY_REQUIRED);
    }
    else if (contextPtr->auth == AUTH_SERVER)
    {
        mbedtls_ssl_conf_authmode(&(contextPtr->sslConf), MBEDTLS_SSL_VERIFY_OPTIONAL);
    }
    else
    {
        LE_ERROR("Bad authentication mode: %d, Allowed auth mode: %d or %d",
                 contextPtr->auth, AUTH_MUTUAL, AUTH_SERVER);
        return LE_FAULT;
    }

    mbedtls_ssl_conf_ca_chain(&(contextPtr->sslConf), &(contextPtr->caCert), NULL);

    // Apply local certificate and key bindings if the authentication type is mutual
    if (contextPtr->auth == AUTH_MUTUAL)
    {
        LE_INFO("Configuring Mutual Authentication");
        if ((ret = mbedtls_ssl_conf_own_cert(&(contextPtr->sslConf),
                                             &(contextPtr->ownCert),
                                             &(contextPtr->ownPkey))) != 0)
        {
            contextPtr->mbedtls_errcode = ret;
            LE_ERROR("Failed! mbedtls_ssl_conf_own_cert returned %d", ret);
            return LE_FAULT;
        }
    }

    // Apply ALPN protocol list if configured
    if ((contextPtr->alpn_list != NULL) &&
        (contextPtr->alpn_list[0] != NULL) &&
        (contextPtr->alpn_list[0][0] != '\0'))
    {
        LE_INFO("Configuring ALPN list %s", *contextPtr->alpn_list);

        if ((ret = mbedtls_ssl_conf_alpn_protocols(&(contextPtr->sslConf), contextPtr->alpn_list)) != 0)
        {
            LE_ERROR("Failed! mbedtls_ssl_conf_alpn_protocols returned -0x%0x", -ret);
            return LE_FAULT;
        }
    }

    if ((ret = mbedtls_ssl_setup(&(contextPtr->sslCtx), &(contextPtr->sslConf))) != 0)
    {
        contextPtr->mbedtls_errcode = ret;
        LE_ERROR("Failed! mbedtls_ssl_setup returned %d", ret);
        if (MBEDTLS_ERR_SSL_ALLOC_FAILED == ret)
        {
            return LE_NO_MEMORY;
        }
        return LE_FAULT;
    }

    if ((ret = mbedtls_ssl_set_hostname(&(contextPtr->sslCtx), hostPtr)) != 0)
    {
        contextPtr->mbedtls_errcode = ret;
        LE_ERROR("Failed! mbedtls_ssl_set_hostname returned %d", ret);
        if (MBEDTLS_ERR_SSL_ALLOC_FAILED == ret)
        {
            return LE_NO_MEMORY;
        }
        return LE_FAULT;
    }

#ifdef MK_CONFIG_THIN_MODEM
    if (isTls13Higher)
    {
        LE_DEBUG("Setting TLS version 1.3");
        // Initialize PSA
        psa_status_t psa_init_status = psa_crypto_init();
        if (psa_init_status != PSA_SUCCESS) {
            LE_ERROR("Failed! psa_crypto_init returned %ld", psa_init_status);
            return LE_FAULT;
        }
        mbedtls_ssl_conf_min_tls_version( &(contextPtr->sslConf), MBEDTLS_SSL_VERSION_TLS1_3);
        //Set the maximum supported SSL/TLS version
        mbedtls_ssl_conf_max_tls_version( &(contextPtr->sslConf), MBEDTLS_SSL_VERSION_TLS1_3 );
    }
    else
    {
        LE_DEBUG("Setting TLS version 1.2");
        //Set the minimum accepted SSL/TLS protocol version
        mbedtls_ssl_conf_min_tls_version( &(contextPtr->sslConf), MBEDTLS_SSL_VERSION_TLS1_2);
        //Set the maximum supported SSL/TLS version
        mbedtls_ssl_conf_max_tls_version( &(contextPtr->sslConf), MBEDTLS_SSL_VERSION_TLS1_2 );
    }
    // Set the RNG function
    mbedtls_ssl_conf_rng(&(contextPtr->sslConf), mbedtls_ctr_drbg_random, &(contextPtr->ctrDrbg));
    mbedtls_ssl_set_bio(&(contextPtr->sslCtx), &(contextPtr->sock),
                        mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);
#else
    //Set the minimum accepted SSL/TLS protocol version
    mbedtls_ssl_conf_min_version( &(contextPtr->sslConf), MBEDTLS_SSL_MAJOR_VERSION_3,
                                                       MBEDTLS_SSL_MINOR_VERSION_3);
    //Set the maximum supported SSL/TLS version
    mbedtls_ssl_conf_max_version( &(contextPtr->sslConf), MBEDTLS_SSL_MAJOR_VERSION_3,
                                                       MBEDTLS_SSL_MINOR_VERSION_3);
    // Set the RNG function
    mbedtls_port_SSLSetRNG(&contextPtr->sslConf);
    mbedtls_ssl_set_bio(&(contextPtr->sslCtx), &(contextPtr->sock),
                        mbedtls_net_send, NULL, mbedtls_net_recv_timeout);
#endif

    // Set the timeout for the initial handshake.
    mbedtls_ssl_conf_read_timeout(&(contextPtr->sslConf), MBEDTLS_SSL_CONNECT_TIMEOUT);

    LE_INFO("Setup TLS param done");

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Performs TLS Handshake
 *
 * Note: All the TLS parameters must be set and connection should be made before calling this
 * function
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
static le_result_t PerformHandshake
(
    secSocket_Ctx_t*    ctxPtr    ///< [INOUT] Secure socket context pointer
)
{
    int              ret;
    MbedtlsCtx_t    *contextPtr = (MbedtlsCtx_t *) ctxPtr;

    LE_ASSERT(contextPtr != NULL);

    // Handshake
    LE_INFO("Performing the SSL/TLS handshake...");
    while ((ret = mbedtls_ssl_handshake(&(contextPtr->sslCtx))) != 0)
    {
        LE_ERROR("Failed! mbedtls_ssl_handshake returned -0x%x", -ret);
        if ((ret != MBEDTLS_ERR_SSL_WANT_READ) && (ret != MBEDTLS_ERR_SSL_WANT_WRITE))
        {
            contextPtr->mbedtls_errcode = ret;
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
    LE_INFO("SSL/TLS handshake done...");
    return LE_OK;
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
    LE_DEBUG("start secSocket_Init");
    // Alloc memory from pool
    contextPtr = le_mem_Alloc(SocketCtxPoolRef);

    // Initialize the session data
    mbedtls_net_init(&(contextPtr->sock));
    LE_DEBUG("Socket init");
    mbedtls_ssl_init(&(contextPtr->sslCtx));
    LE_DEBUG("SSL ctx init");
    mbedtls_ssl_config_init(&(contextPtr->sslConf));
    LE_DEBUG("SSL cfg init");
    mbedtls_x509_crt_init(&(contextPtr->caCert));
    LE_DEBUG("CA cert init");
    mbedtls_x509_crt_init(&(contextPtr->ownCert));
    LE_DEBUG("OWN cert init");
    mbedtls_pk_init(&(contextPtr->ownPkey));
    LE_DEBUG("PK init init");
    contextPtr->auth = AUTH_SERVER;
    contextPtr->mbedtls_errcode = 0;

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_ssl_conf_dbg(&(contextPtr->sslConf), OutputMbedtlsDebugInfo, stdout);
    mbedtls_debug_set_threshold(SSL_DEBUG_LEVEL);
#endif

#ifdef MK_CONFIG_THIN_MODEM
    int ret = 0;
    mbedtls_entropy_init(&(contextPtr->entropy));
    mbedtls_ctr_drbg_init(&(contextPtr->ctrDrbg));
    LE_DEBUG("Entropy and drbg init");
    if ((ret = mbedtls_ctr_drbg_seed(&(contextPtr->ctrDrbg), mbedtls_entropy_func,
            &(contextPtr->entropy), NULL, 0)) != 0) {
        LE_ERROR("mbedtls_ctr_drbg_seed returned 0x%4x", ret);
        mbedtls_entropy_free(&(contextPtr->entropy));
        mbedtls_ctr_drbg_free(&(contextPtr->ctrDrbg));
        le_mem_Release(contextPtr);
        return LE_FAULT;
    }
    contextPtr->tlsVersion = MBEDTLS_SSL_MINOR_VERSION_4;
    LE_DEBUG("Initializing psa_crypto");
    // Initialize PSA
    psa_status_t psa_init_status = psa_crypto_init();
    if (psa_init_status != PSA_SUCCESS) {
        LE_ERROR("psa_crypto_init() returned %ld", psa_init_status);
        mbedtls_entropy_free(&(contextPtr->entropy));
        mbedtls_ctr_drbg_free(&(contextPtr->ctrDrbg));
        le_mem_Release(contextPtr);
        return LE_FAULT;
    }
#endif
    LE_DEBUG("secSocket_Init done");

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
        contextPtr->mbedtls_errcode = ret;
        LE_ERROR("Failed! mbedtls_x509_crt_parse returned -0x%x", -ret);
        return LE_FAULT;
    }

    // Check certificate validity
#ifdef MK_CONFIG_THIN_MODEM
    if ((mbedtls_x509_time_is_past(GET_X509_TIME_TO(&contextPtr->caCert))) ||
        (mbedtls_x509_time_is_future(GET_X509_TIME_FROM(&contextPtr->caCert))))
#else
    if ((mbedtls_x509_time_is_past(&contextPtr->caCert.valid_to)) ||
        (mbedtls_x509_time_is_future(&contextPtr->caCert.valid_from)))
#endif
    {
        contextPtr->mbedtls_errcode = MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
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
        contextPtr->mbedtls_errcode = ret;
        LE_ERROR("Failed! mbedtls_x509_crt_parse returned -0x%x", -ret);
        return LE_FAULT;
    }

    // Check certificate validity
#ifdef MK_CONFIG_THIN_MODEM
    if ((mbedtls_x509_time_is_past(GET_X509_TIME_TO(&contextPtr->ownCert))) ||
        (mbedtls_x509_time_is_future(GET_X509_TIME_FROM(&contextPtr->ownCert))))
#else
    if ((mbedtls_x509_time_is_past(&contextPtr->ownCert.valid_to)) ||
        (mbedtls_x509_time_is_future(&contextPtr->ownCert.valid_from)))
#endif
    {
        contextPtr->mbedtls_errcode = MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
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
    int              ret = -1;
    MbedtlsCtx_t    *contextPtr = (MbedtlsCtx_t *) ctxPtr;

    LE_ASSERT(contextPtr != NULL);
    LE_ASSERT(pkeyPtr != NULL);
    LE_ASSERT(pkeyLen > 0);

    LE_DEBUG("Add client private key: %p Len:%"PRIuS, pkeyPtr, pkeyLen);

#ifdef MK_CONFIG_THIN_MODEM
    ret = mbedtls_pk_parse_key(&(contextPtr->ownPkey), pkeyPtr, pkeyLen, NULL, 0,
                                mbedtls_ctr_drbg_random, &(contextPtr->ctrDrbg));
#else
    ret = mbedtls_pk_parse_key(&(contextPtr->ownPkey), pkeyPtr, pkeyLen, NULL, 0);
#endif

    if (ret < 0)
    {
        contextPtr->mbedtls_errcode = ret;
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
 * Set the ALPN protocol list in the secure socket context.
 */
//--------------------------------------------------------------------------------------------------
void secSocket_SetAlpnProtocolList
(
    secSocket_Ctx_t*  ctxPtr,           ///< [INOUT] Secure socket context pointer
    const char**      alpnList          ///< [IN] ALPN protocol list pointer
)
{
    MbedtlsCtx_t *contextPtr = (MbedtlsCtx_t *) ctxPtr;
    LE_ASSERT(contextPtr != NULL);

    contextPtr->alpn_list = alpnList;
}


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
)
{
    le_result_t result = LE_OK;
    MbedtlsCtx_t    *contextPtr = (MbedtlsCtx_t *) ctxPtr;

    LE_ASSERT(contextPtr != NULL);
    LE_ASSERT(hostPtr != NULL);
    LE_ASSERT(fd != -1);

    // Set the secure socket fd to the original netsocket fd
    contextPtr->sock.fd = fd;

    // Setup TLS parameters
    result = SetupTLSParams(ctxPtr, hostPtr);
    if (LE_OK != result)
    {
        LE_ERROR("Failed! setting up TLS parameters: %s", LE_RESULT_TXT(result));
        return LE_FAULT;
    }

    return PerformHandshake(ctxPtr);
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

    char portStr[PORT_STR_LEN] = {0};
    int ret = 0;
    struct addrinfo hints;
    struct sockaddr_storage srcSocket = {0};

    // Setup TLS parameters
    le_result_t result = SetupTLSParams(ctxPtr, hostPtr);
    if (LE_OK != result)
    {
        LE_ERROR("Failed! setting up TLS parameters: %s", LE_RESULT_TXT(result));
        return LE_FAULT;
    }

    // Start the connection
    snprintf(portBuffer, sizeof(portBuffer), "%d", port);
    LE_INFO("Connecting to %d/%s:%d - %s:%s...", type, hostPtr, port, hostPtr, portBuffer);

    // Convert port to string
    snprintf(portStr, PORT_STR_LEN, "%hu", port);

    // Do name resolution with both IPv6 and IPv4
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = (type == UDP_TYPE) ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = (type == UDP_TYPE) ? IPPROTO_UDP : IPPROTO_TCP;

    // Initialize socket structure from source IP string
    if (GetSocketInfo(srcAddrPtr, &(hints.ai_family), &srcSocket) != LE_OK)
    {
        LE_ERROR("Error on function: GetSocketInfo");
        return LE_UNAVAILABLE;
    }

    if ((ret = mbedtls_net_connect_swi(&(contextPtr->sock), hostPtr, portStr, &srcSocket, sizeof(srcSocket), MBEDTLS_NET_PROTO_TCP)) != 0)
    {
        LE_ERROR("mbedtls_net_connect failed to address: %s. error: -0x%04x", hostPtr, -ret);
        return LE_FAULT;
    }

    //Get the file descriptor
    *fdPtr = contextPtr->sock.fd;
    LE_DEBUG("File descriptor: %d", *fdPtr);

    return PerformHandshake(ctxPtr);
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
#ifdef MK_CONFIG_THIN_MODEM
    mbedtls_entropy_free(&(contextPtr->entropy));
    mbedtls_ctr_drbg_free(&(contextPtr->ctrDrbg));
#endif
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
    const char      *dataPtr,   ///< [IN] Data pointer
    size_t           dataLen    ///< [IN] Data length
)
{
    MbedtlsCtx_t *contextPtr = (MbedtlsCtx_t *) ctxPtr;
    int ret;

    LE_ASSERT(contextPtr != NULL);
    LE_ASSERT(dataPtr != NULL);

    ret = WriteToStream(&(contextPtr->sslCtx), dataPtr, (int)dataLen);
    if (0 > ret)
    {
        contextPtr->mbedtls_errcode = ret;
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
 *  - LE_IN_PROGRESS   Secure HandShake still in progress
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
    if (count <= 0)
    {
        if (count < 0)
        {
            contextPtr->mbedtls_errcode = count;
            if (count == MBEDTLS_ERR_SSL_TIMEOUT)
            {
                return LE_TIMEOUT;
            }
#ifdef MK_CONFIG_THIN_MODEM
            else if (count == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET)
            {
                contextPtr->mbedtls_errcode = 0;
                LE_ERROR("Recived NEW session ticket error, will be ignored and tried again");
                return LE_IN_PROGRESS;
            }
#endif
        }
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
 * Set the tls version to the secure socket context.
 */
//--------------------------------------------------------------------------------------------------
void secSocket_SetTlsVersion
(
    secSocket_Ctx_t*  ctxPtr,           ///< [INOUT] Secure socket context pointer
    uint8_t           tlsVersion        ///< [IN] Supported TLS version (Minor version number)
)
{
#ifdef MK_CONFIG_THIN_MODEM
    MbedtlsCtx_t    *contextPtr = (MbedtlsCtx_t *) ctxPtr;

    LE_ASSERT(contextPtr != NULL);
    LE_DEBUG("Setting TLS minor version: %d", tlsVersion);

    // If +ksslcrypto profile index 0 is selected, the module would send all the approved cipher
    // suites to the server, so the server can check its own supported cipher suites and pick one
    // that is supported by both parties. If other profile index is selected, the module would send
    // the specified cipher suite to the server.
    contextPtr->tlsVersion = tlsVersion;
#else
    LE_WARN("Changing TLS version isn't supported for this platform. Ignoring it.");
#endif
}

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
)
{
    MbedtlsCtx_t *contextPtr = (MbedtlsCtx_t *) ctxPtr;
    if (contextPtr == NULL)
    {
        LE_INFO("Non secure case, will just return no error (0)");
        return 0;
    }

    return contextPtr->mbedtls_errcode;
}

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
    secSocket_Ctx_t        *ctxPtr,   ///< [IN] Secure socket context pointer
    int                    err_code   ///< [IN] INT error code
)
{
    MbedtlsCtx_t *contextPtr = (MbedtlsCtx_t *) ctxPtr;
    if (contextPtr == NULL)
    {
        LE_INFO("Non secure case, will just return");
        return;
    }

    contextPtr->mbedtls_errcode = err_code;
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
