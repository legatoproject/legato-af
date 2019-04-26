/**
 * @file secSocket_openssl.c
 *
 * This file implements a set of networking functions to manage secure TCP/UDP sockets using
 * OpenSSL library.
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
#include "le_socketLib.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Magic number used in OpenSSL structure to check the structure validity
 */
//--------------------------------------------------------------------------------------------------
#define OPENSSL_MAGIC_NUMBER        0x4F50454E

//--------------------------------------------------------------------------------------------------
/**
 * Port maximum length
 */
//--------------------------------------------------------------------------------------------------
#define PORT_STR_LEN                6

//--------------------------------------------------------------------------------------------------
/**
 * OpenSSL global context
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t                 magicNb;   ///< Magic number to check structure validity
    BIO*                     bioPtr;    ///< I/O stream abstraction pointer
    SSL_CTX*                 sslCtxPtr; ///< SSL internal context pointer
    bool                     isInit;    ///< TRUE if the secure socket context is initialized
}
OpensslCtx_t;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for OpenSSL sockets context.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(SocketCtxPool, MAX_SOCKET_NB, sizeof(OpensslCtx_t));

//--------------------------------------------------------------------------------------------------
// Internal variables
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool reference for the OpenSSL context pool
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SocketCtxPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
// Static functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Cast secure socket context into OpenSSL socket context and check its validity
 *
 * @return
 *  - OpenSSL socket context pointer
 */
//--------------------------------------------------------------------------------------------------
static OpensslCtx_t* GetContext
(
    secSocket_Ctx_t*  ctxPtr   ///< [IN] Secure socket context pointer
)
{
    if (!ctxPtr)
    {
        return NULL;
    }

    OpensslCtx_t* sslSocketCtx = (secSocket_Ctx_t*)ctxPtr;

    if (sslSocketCtx->magicNb == OPENSSL_MAGIC_NUMBER)
    {
        return sslSocketCtx;
    }
    else
    {
        LE_ERROR("Unrecognized context provided");
        return NULL;
    }
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
    // Check input parameter
    if (!ctxPtr)
    {
        LE_ERROR("Null pointer provided");
        return LE_BAD_PARAMETER;
    }

    // Initialize the socket context pool
    if (!SocketCtxPoolRef)
    {
        SocketCtxPoolRef = le_mem_InitStaticPool(SocketCtxPool,
                                                 MAX_SOCKET_NB,
                                                 sizeof(OpensslCtx_t));
    }

    // Check if the socket is already initialized
    OpensslCtx_t* contextPtr = GetContext(*ctxPtr);
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
    contextPtr->magicNb = OPENSSL_MAGIC_NUMBER;

    // Initialize OpenSSL library and setup SSL pointers
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    SSL_library_init();
    contextPtr->sslCtxPtr = SSL_CTX_new(TLSv1_client_method());
#else
    OPENSSL_init_ssl(0, NULL);
    contextPtr->sslCtxPtr = SSL_CTX_new(TLS_client_method());
#endif

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
    X509_STORE *store = NULL;
    X509 *cert = NULL;
    BIO *bio = NULL;
    le_result_t status = LE_FAULT;
    le_clk_Time_t currentTime;

    // Check input parameters
    if ((!ctxPtr) || (!certificatePtr) || (!certificateLen))
    {
        return LE_BAD_PARAMETER;
    }

    OpensslCtx_t* contextPtr = GetContext(ctxPtr);
    if (!contextPtr)
    {
        return LE_BAD_PARAMETER;
    }

    LE_INFO("Certificate: %p Len:%zu", certificatePtr, certificateLen);

    // Get a BIO abstraction pointer
    bio = BIO_new_mem_buf((void*)certificatePtr, certificateLen);
    if (!bio)
    {
        LE_ERROR("Unable to allocate BIO pointer");
        goto end;
    }

    // Read the DER formatted certificate from memory into an X509 structure
    cert = d2i_X509(NULL, &certificatePtr, certificateLen);
    if (!cert)
    {
        LE_ERROR("Unable to read certificate");
        goto end;
    }

    // Check certificate validity
    currentTime = le_clk_GetAbsoluteTime();

    if ((X509_cmp_time(X509_get_notBefore(cert), &currentTime.sec) >= 0)  ||
        (X509_cmp_time(X509_get_notAfter(cert), &currentTime.sec) <= 0))
    {
        LE_ERROR("Current certificate expired, please add a valid certificate");
        status = LE_FORMAT_ERROR;
        goto end;
    }

    // Get a pointer to the current certificate verification pool
    store = SSL_CTX_get_cert_store(contextPtr->sslCtxPtr);
    if (!store)
    {
        LE_ERROR("Unable to get a pointer to the X509 certificate");
        goto end;
    }

    // Add certificate to the verification pool
    if (!X509_STORE_add_cert(store, cert))
    {
        LE_ERROR("Unable to add certificate to pool");
        goto end;
    }

    status = LE_OK;

end:
    if (cert)
    {
        X509_free(cert);
    }

    if (bio)
    {
        BIO_free(bio);
    }

    return status;
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
    SSL* sslPtr = NULL;
    BIO* bioPtr = NULL;
    char hostAndPort[HOST_ADDR_LEN + PORT_STR_LEN + 1];
    le_result_t status = LE_FAULT;
    unsigned long code;

    if ((!ctxPtr) || (!hostPtr) || (!fdPtr))
    {
        LE_ERROR("Invalid argument: ctxPtr %p, hostPtr %p fdPtr %p", ctxPtr, hostPtr, fdPtr);
        return LE_BAD_PARAMETER;
    }

    OpensslCtx_t* contextPtr = GetContext(ctxPtr);
    if (!contextPtr)
    {
        return LE_BAD_PARAMETER;
    }

    // Start the connection
    snprintf(hostAndPort, sizeof(hostAndPort), "%s:%d", hostPtr, port);
    LE_INFO("Connecting to %d/%s:%d - %s...", type, hostPtr, port, hostAndPort);

    // Clear the current thread's OpenSSL error queue
    ERR_clear_error();

    // Setting up the BIO abstraction layer
    bioPtr = BIO_new_ssl_connect(contextPtr->sslCtxPtr);
    if (!bioPtr)
    {
        LE_ERROR("Unable to allocate and connect BIO");
        goto err;
    }

    BIO_get_ssl(bioPtr, &sslPtr);
    if (!sslPtr)
    {
        LE_ERROR("Unable to locate SSL pointer");
        goto err;
    }

    // Set the SSL_MODE_AUTO_RETRY flag: it will cause read/write operations to only return after
    // the handshake and successful completion
    SSL_set_mode(sslPtr, SSL_MODE_AUTO_RETRY);

    BIO_set_conn_hostname(bioPtr, hostAndPort);

    // Attempt to connect the supplied BIO and perform the handshake.
    // This function returns 1 if the connection was successfully established and 0 or -1 if the
    // connection failed.
    if (BIO_do_connect(bioPtr) != 1)
    {
        LE_ERROR("Unable to connect BIO to %s", hostAndPort);
        goto err;
    }

    // Get the FD linked to the BIO
    BIO_get_fd(bioPtr, fdPtr);
    BIO_socket_nbio(*fdPtr, 1);

    contextPtr->bioPtr = bioPtr;
    return LE_OK;

err:
    code = ERR_peek_last_error();

    if ((ERR_GET_LIB(code) == ERR_LIB_BIO) || (ERR_GET_LIB(code) == ERR_LIB_SSL))
    {
        switch (ERR_GET_REASON(code))
        {
            case ERR_R_MALLOC_FAILURE:
                status = LE_NO_MEMORY;
                break;

            case BIO_R_NULL_PARAMETER:
                status = LE_BAD_PARAMETER;
                break;

#if defined(BIO_R_BAD_HOSTNAME_LOOKUP)
            case BIO_R_BAD_HOSTNAME_LOOKUP:
                status = LE_UNAVAILABLE;
                break;
#endif

            case BIO_R_CONNECT_ERROR:
                status = LE_COMM_ERROR;
                break;

#if defined(BIO_R_EOF_ON_MEMORY_BIO)
            case BIO_R_EOF_ON_MEMORY_BIO:
                status = LE_CLOSED;
                break;
#endif

            default:
                status = LE_FAULT;
                break;
        }
     }

    return status;
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

    OpensslCtx_t* contextPtr = GetContext(ctxPtr);
    if (!contextPtr)
    {
        return LE_BAD_PARAMETER;
    }

    BIO_ssl_shutdown(contextPtr->bioPtr);
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

    OpensslCtx_t* contextPtr = GetContext(ctxPtr);
    if (!contextPtr)
    {
        return LE_BAD_PARAMETER;
    }

    BIO_free_all(contextPtr->bioPtr);
    contextPtr->bioPtr = NULL;
    SSL_CTX_free(contextPtr->sslCtxPtr);
    contextPtr->sslCtxPtr = NULL;

#if OPENSSL_API_COMPAT < 0x10100000L
#if OPENSSL_API_COMPAT > 0x10002000L
    // In versions of OpenSSL prior to 1.1.0 SSL_COMP_free_compression_methods() freed the
    // internal table of compression methods that were built internally, and possibly augmented
    // by adding SSL_COMP_add_compression_method().
    // However this is now unnecessary from version 1.1.0. No explicit initialisation or
    // de-initialisation is necessary.
    SSL_COMP_free_compression_methods();
#endif
#endif
    EVP_cleanup();
    ERR_free_strings();

    CRYPTO_cleanup_all_ex_data();
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    ERR_remove_state(0);
#endif
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

    OpensslCtx_t* contextPtr = GetContext(ctxPtr);
    if (!contextPtr)
    {
        return LE_BAD_PARAMETER;
    }

    int r = BIO_write(contextPtr->bioPtr, dataPtr, dataLen);
    if (0 >= r)
    {
        LE_ERROR("Write failed. Error code: %d", r);
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
 *  - LE_WOULD_BLOCK   Would have blocked if non-blocking behaviour was not requested
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
    fd_set set;
    int rv, fd;

    if ((!ctxPtr) || (!dataPtr) || (!dataLenPtr))
    {
        return LE_BAD_PARAMETER;
    }

    OpensslCtx_t* contextPtr = GetContext(ctxPtr);
    if (!contextPtr)
    {
        return LE_BAD_PARAMETER;
    }

    if (!contextPtr->bioPtr)
    {
        LE_ERROR("Socket not connected");
        return LE_FAULT;
    }

    if (!BIO_pending(contextPtr->bioPtr))
    {
        struct timeval time = {.tv_sec = timeout / 1000, .tv_usec = (timeout % 1000) * 1000};

        // Get file descriptor of the current BIO
        BIO_get_fd(contextPtr->bioPtr, &fd);

        do
        {
           FD_ZERO(&set);
           FD_SET(fd, &set);
           rv = select(fd + 1, &set, NULL, NULL, &time);
        }
        while (rv == -1 && errno == EINTR);

        if (rv > 0)
        {
            if (!FD_ISSET(fd, &set))
            {
                LE_ERROR("Nothing to read");
                return LE_FAULT;
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
    }

    // At this point, there is something available for reading from BIO
    rv = BIO_read(contextPtr->bioPtr, dataPtr, *dataLenPtr);
    if (rv <= 0)
    {
        if (BIO_should_retry(contextPtr->bioPtr))
        {
             return LE_WOULD_BLOCK;
        }
        else
        {
            LE_ERROR("Read failed. Error code: %d", rv);
            return LE_FAULT;
        }
    }

    *dataLenPtr = rv;
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

    OpensslCtx_t* contextPtr = GetContext(ctxPtr);
    if (!contextPtr)
    {
        return false;
    }

    return (BIO_pending(contextPtr->bioPtr) ? true: false);
}
