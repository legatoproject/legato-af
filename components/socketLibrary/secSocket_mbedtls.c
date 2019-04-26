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

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "secSocket.h"
#include "mbedtls/debug.h"
#include "le_socketLib.h"

#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/platform.h"
#include "mbedtls/ssl_internal.h"

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
 * Number of large memory buffers required by MbedTLS
 */
//--------------------------------------------------------------------------------------------------
#define MBEDTLS_LARGE_BUFFER_COUNT  2

//--------------------------------------------------------------------------------------------------
/**
 * Magic number used in MbedTLS structure to check the structure validity
 */
//--------------------------------------------------------------------------------------------------
#define MBEDTLS_MAGIC_NUMBER        0x6D626564

//--------------------------------------------------------------------------------------------------
/**
 * MbedTLS global context
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t                 magicNb;   ///< Magic number to check structure validity
    mbedtls_net_context      serverFd;  ///< MbedTLS wrapper for socket
    mbedtls_ctr_drbg_context ctrDrbg;   ///< CTR_DRBG context structure
    mbedtls_ssl_context      sslCtx;    ///< SSL/TLS context
    mbedtls_ssl_config       sslConf;   ///< SSL/TLS configuration
    mbedtls_entropy_context  entropy;   ///< Entropy context structure
    mbedtls_x509_crt         caCert;    ///< X.509 certificate
    bool                     isInit;    ///< TRUE if the secure socket context is initialized
}
MbedtlsCtx_t;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for MbedTLS.  Largest blocks are in/out buffers which are MBEDTLS_SSL_BUFFER_LEN
 * sized.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(MbedTLS, MBEDTLS_LARGE_BUFFER_COUNT, MBEDTLS_SSL_BUFFER_LEN);

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for MbedTLS sockets context.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(SocketCtxPool, MAX_SOCKET_NB, sizeof(MbedtlsCtx_t));

//--------------------------------------------------------------------------------------------------
// Internal variables
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool reference for MbedTLS pool
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t MbedTLSPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool reference for the MbedtLS context pool
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SocketCtxPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
// Static functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Calloc replacement for MbedTLS. Pull from a memory pool, as with LWM2M on RTOS.
 */
//--------------------------------------------------------------------------------------------------
static void* MemoryCalloc
(
    size_t n,
    size_t size
)
{
    void* memPtr = NULL;
    size_t totalSize = n*size;

    // Try to allocate SSL buffers off the memory pool
    if (totalSize == MBEDTLS_SSL_BUFFER_LEN)
    {
        memPtr = le_mem_TryAlloc(MbedTLSPoolRef);
    }

#ifndef LE_MEM_VALGRIND
    // Otherwise use malloc.  This includes if buffer would overflow
    if (!memPtr)
    {
        memPtr = malloc(totalSize);
    }
#endif

    // And zero memory if allocated via either method above, because calloc zeros but
    // le_mem_TryAlloc() and malloc() do not.
    if (memPtr)
    {
        memset(memPtr, 0, totalSize);
    }

    return memPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Free replacement for mbedtls.  Place blocks back on the memory pool.
 */
//--------------------------------------------------------------------------------------------------
static void MemoryFree
(
    void* memPtr
)
{
    // Free NULL does nothing
    if (!memPtr)
    {
        LE_WARN("Null pointer provided");
        return;
    }

    // Check if this is in the memory pool.

    // There's no good general purpose way of doing this,
    // but in the case of static memory pools allocated only via le_mem_TryAlloc() we can check
    // if it's in the pool data area.  Do this here directly as it's not something we should
    // be encouraging by providing an API.
#ifndef LE_MEM_VALGRIND
    if ((memPtr >= ((void*)(&_mem_MbedTLSData))) &&
        (memPtr < (void*)(&_mem_MbedTLSData[sizeof(_mem_MbedTLSData)/sizeof(_mem_MbedTLSData[0])])))
    {
#endif
        // This is in the memory pool
        le_mem_Release(memPtr);
#ifndef LE_MEM_VALGRIND
    }
    else
    {
        // This is malloc'ed memory
        free(memPtr);
    }
#endif
}

//--------------------------------------------------------------------------------------------------
/**
 * Write to a stream and handle restart if necessary
 *
 * @return
 *  -  0 when all data are written
 *  - -1 on failure
 *
 */
//--------------------------------------------------------------------------------------------------
static int WriteToStream
(
    mbedtls_ssl_context*    sslCtxPtr,      ///< [IN] SSL context
    char*                   bufferPtr,      ///< [IN] Data to be sent
    int                     length          ///< [IN] Data length
)
{
    int r;

    if ((!bufferPtr) || (!sslCtxPtr))
    {
        return -1;
    }

    r = mbedtls_ssl_write(sslCtxPtr, (const unsigned char*)bufferPtr, (size_t)length);
    if (0 >= r)
    {
        LE_ERROR("Error on write %d", r);
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
        LE_ERROR("Error on write %d", r);
    }
    return r;
}

//--------------------------------------------------------------------------------------------------
/**
 * Cast secure socket context into MbedTLS socket context and check its validity
 *
 * @return
 *  - MbedTLS socket context pointer
 */
//--------------------------------------------------------------------------------------------------
static MbedtlsCtx_t* GetContext
(
    secSocket_Ctx_t*  ctxPtr   ///< [IN] Secure socket context pointer
)
{
    if (!ctxPtr)
    {
        return NULL;
    }

    MbedtlsCtx_t* mbedSocketCtx = (secSocket_Ctx_t*)ctxPtr;

    if (mbedSocketCtx->magicNb == MBEDTLS_MAGIC_NUMBER)
    {
        return mbedSocketCtx;
    }
    else
    {
        LE_ERROR("Unrecognized context provided");
        return NULL;
    }
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
static le_result_t ReadFromStream
(
    mbedtls_ssl_context*    sslCtxPtr,      ///< [IN] SSL context
    char*                   bufferPtr,      ///< [IN] Buffer
    int                     length          ///< [IN] Buffer length
)
{
    int r = -1;
    static int count = 0;

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
    secSocket_Ctx_t**  ctxPtr       ///< [INOUT] Secure socket context pointer
)
{
    int ret;

    // Check input parameter
    if (!ctxPtr)
    {
        LE_ERROR("Null pointer provided");
        return LE_BAD_PARAMETER;
    }

    // Initialize MbedTLS internal pool and attach callbacks
    if (!MbedTLSPoolRef)
    {
        MbedTLSPoolRef = le_mem_InitStaticPool(MbedTLS,
                                               MBEDTLS_LARGE_BUFFER_COUNT,
                                               MBEDTLS_SSL_BUFFER_LEN);

        mbedtls_platform_set_calloc_free(MemoryCalloc, MemoryFree);
    }

    // Initialize the socket context pool
    if (!SocketCtxPoolRef)
    {
        SocketCtxPoolRef = le_mem_InitStaticPool(SocketCtxPool,
                                                 MAX_SOCKET_NB,
                                                 sizeof(MbedtlsCtx_t));
    }

    // Check if the socket is already initialized
    MbedtlsCtx_t* contextPtr = GetContext(*ctxPtr);
    if ((contextPtr) && (contextPtr->isInit))
    {
        LE_ERROR("Socket context already initialized");
        return LE_FAULT;
    }

    // Alloc memory from pool
    contextPtr = le_mem_TryAlloc(SocketCtxPoolRef);
    if (NULL == contextPtr)
    {
        LE_ERROR("Unable to allocate a socket context from pool");
        return LE_FAULT;
    }

    // Set the magic number
    contextPtr->magicNb = MBEDTLS_MAGIC_NUMBER;

    // Initialize the RNG and the session data
    mbedtls_net_init(&(contextPtr->serverFd));
    mbedtls_ssl_init(&(contextPtr->sslCtx));
    mbedtls_ssl_config_init(&(contextPtr->sslConf));
    mbedtls_ctr_drbg_init(&(contextPtr->ctrDrbg));

    LE_DEBUG("Seeding the random number generator...");
    mbedtls_entropy_init(&(contextPtr->entropy));
    ret = mbedtls_ctr_drbg_seed(&(contextPtr->ctrDrbg),
                                mbedtls_entropy_func,
                                &(contextPtr->entropy),
                                NULL,
                                0);
    if (ret)
    {
        LE_ERROR("Failed! mbedtls_ctr_drbg_seed returned %d", ret);
        return LE_FAULT;
    }

    contextPtr->isInit = true;
    *ctxPtr = (secSocket_Ctx_t*)contextPtr;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add one or more certificates to the secure socket context.
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FORMAT_ERROR  Invalid certificate
 *  - LE_FAULT         Failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t secSocket_AddCertificate
(
    secSocket_Ctx_t*  ctxPtr,           ///< [INOUT] Secure socket context pointer
    const uint8_t*    certificatePtr,   ///< [IN] Certificate Pointer
    size_t            certificateLen    ///< [IN] Certificate Length
)
{
    // Check input parameters
    if ((!ctxPtr) || (!certificatePtr) || (!certificateLen))
    {
        return LE_BAD_PARAMETER;
    }

    MbedtlsCtx_t* contextPtr = GetContext(ctxPtr);
    if (!contextPtr)
    {
        return LE_BAD_PARAMETER;
    }

    LE_DEBUG("Certificate: %p Len:%zu", certificatePtr, certificateLen);
    int ret = mbedtls_x509_crt_parse(&(contextPtr->caCert), certificatePtr, certificateLen);
    if (ret < 0)
    {
        LE_ERROR("Failed!  mbedtls_x509_crt_parse returned -0x%x", -ret);
        return LE_FAULT;
    }

    // Check certificate validity
    if ((mbedtls_x509_time_is_past(&contextPtr->caCert.valid_to)) ||
        (mbedtls_x509_time_is_future(&contextPtr->caCert.valid_from)))
    {
        LE_ERROR("Current certificate expired, please add a valid certificate");
        return LE_FORMAT_ERROR;
    }

    return LE_OK;
}

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
    SocketType_t     type,       ///< [IN] Socket type (TCP, UDP)
    int*             fdPtr       ///< [OUT] Socket file descriptor
)
{
    int ret;
    char portBuffer[6] = {0};

    if ((!ctxPtr) || (!hostPtr) || (!fdPtr))
    {
        LE_ERROR("Invalid argument: ctxPtr %p, hostPtr %p fdPtr %p", ctxPtr, hostPtr, fdPtr);
        return LE_BAD_PARAMETER;
    }

    MbedtlsCtx_t* contextPtr = GetContext(ctxPtr);
    if (!contextPtr)
    {
        return LE_BAD_PARAMETER;
    }

    // Start the connection
    snprintf(portBuffer, sizeof(portBuffer), "%d", port);
    LE_INFO("Connecting to %d/%s:%d - %s:%s...", type, hostPtr, port, hostPtr, portBuffer);

    if ((ret = mbedtls_net_connect(&(contextPtr->serverFd),
                                   hostPtr,
                                   portBuffer,
                                   (type == TCP_TYPE) ?
                                   MBEDTLS_NET_PROTO_TCP : MBEDTLS_NET_PROTO_UDP)) != 0)
    {
        LE_INFO("Failed! mbedtls_net_connect returned %d", ret);
        if (ret == MBEDTLS_ERR_NET_UNKNOWN_HOST)
        {
            return LE_UNAVAILABLE;
        }
        else if (ret == MBEDTLS_ERR_NET_CONNECT_FAILED)
        {
            return LE_COMM_ERROR;
        }
        return LE_FAULT;
    }

    //Get the file descriptor
    *fdPtr = contextPtr->serverFd.fd;
    LE_DEBUG("File descriptor: %d", *fdPtr);

    // Setup
    LE_INFO("Setting up the SSL/TLS structure...");

    if ((ret = mbedtls_ssl_config_defaults(&(contextPtr->sslConf),
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        LE_ERROR("Failed! mbedtls_ssl_config_defaults returned %d", ret);
        // Only possible error is linked to memory allocation issue
        return LE_NO_MEMORY;
    }

    mbedtls_ssl_conf_authmode(&(contextPtr->sslConf), MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&(contextPtr->sslConf), &(contextPtr->caCert), NULL);
    mbedtls_ssl_conf_rng(&(contextPtr->sslConf), mbedtls_ctr_drbg_random, &(contextPtr->ctrDrbg));

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

    mbedtls_ssl_set_bio(&(contextPtr->sslCtx), &(contextPtr->serverFd),
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
)
{
    if (!ctxPtr)
    {
        return LE_BAD_PARAMETER;
    }

    MbedtlsCtx_t* contextPtr = GetContext(ctxPtr);
    if (!contextPtr)
    {
        return LE_BAD_PARAMETER;
    }

    mbedtls_net_free(&(contextPtr->serverFd));
    return LE_OK;
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
    secSocket_Ctx_t* ctxPtr      ///< [INOUT] Secure socket context pointer
)
{
    if (!ctxPtr)
    {
        return LE_BAD_PARAMETER;
    }

    MbedtlsCtx_t* contextPtr = GetContext(ctxPtr);
    if (!contextPtr)
    {
        return LE_BAD_PARAMETER;
    }

    mbedtls_net_free(&(contextPtr->serverFd));
    mbedtls_x509_crt_free(&(contextPtr->caCert));
    mbedtls_ssl_free(&(contextPtr->sslCtx));
    mbedtls_ssl_config_free(&(contextPtr->sslConf));
    mbedtls_ctr_drbg_free(&(contextPtr->ctrDrbg));
    mbedtls_entropy_free(&(contextPtr->entropy));

    contextPtr->isInit = false;
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
    secSocket_Ctx_t* ctxPtr,      ///< [INOUT] Secure socket context pointer
    char*            dataPtr,     ///< [IN] Data pointer
    size_t           dataLen      ///< [IN] Data length
)
{
    if ((!ctxPtr) || (!dataPtr))
    {
        return LE_BAD_PARAMETER;
    }

    MbedtlsCtx_t* contextPtr = GetContext(ctxPtr);
    if (!contextPtr)
    {
        return LE_BAD_PARAMETER;
    }

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
    secSocket_Ctx_t* ctxPtr,       ///< [INOUT] Secure socket context pointer
    char*            dataPtr,      ///< [INOUT] Data pointer
    size_t*          dataLenPtr,   ///< [IN] Data length pointer
    uint32_t         timeout       ///< [IN] Read timeout in milliseconds.
)
{
    if ((!ctxPtr) || (!dataPtr) || (!dataLenPtr))
    {
        return LE_BAD_PARAMETER;
    }

    MbedtlsCtx_t* contextPtr = GetContext(ctxPtr);
    if (!contextPtr)
    {
        return LE_BAD_PARAMETER;
    }

    mbedtls_ssl_conf_read_timeout(&(contextPtr->sslConf), timeout);

    int data = ReadFromStream(&(contextPtr->sslCtx), dataPtr, (int)*dataLenPtr);

    if (data == LE_TIMEOUT)
    {
        return LE_TIMEOUT;
    }
    else if (data < 0)
    {
        LE_INFO("ERROR on reading data from stream");
        return LE_FAULT;
    }

    *dataLenPtr = data;
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
    secSocket_Ctx_t* ctxPtr       ///< [INOUT] Secure socket context pointer
)
{
    if (!ctxPtr)
    {
        return false;
    }

    MbedtlsCtx_t* contextPtr = GetContext(ctxPtr);
    if (!contextPtr)
    {
        return false;
    }

    return (mbedtls_ssl_get_bytes_avail(&(contextPtr->sslCtx)) != 0);
}
