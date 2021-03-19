/**
 * @file le_socketLib.c
 *
 * Socket library interface for secure/unsecure connections using TCP or UDP connection types.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

#include "le_socketLib.h"
#include "netSocket.h"
#include "secSocket.h"

#if LE_CONFIG_LINUX
#include <netdb.h>
#endif

#if !LE_CONFIG_RTOS
#include <arpa/inet.h>
#endif

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------
#define ADDR_MAX_LEN    LE_MDC_IPV6_ADDR_MAX_BYTES

//--------------------------------------------------------------------------------------------------
/**
 * Socket context
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_socket_Ref_t    reference;              ///< Safe reference to this object
    int                fd;                     ///< Socket file descriptor
    char               host[HOST_ADDR_LEN];    ///< Host address
    uint16_t           port;                   ///< Host port
    char               srcAddr[ADDR_MAX_LEN];  ///< Source IP address
    SocketType_t       type;                   ///< Socket type (TCP, UDP)
    uint32_t           timeout;                ///< Communication timeout in milliseconds
    bool               isSecure;               ///< True if the socket is secure
    bool               hasCert;                ///< True if the socket has a valid certificate
    bool               isMonitoring;           ///< True if the socket is being monitored
    le_fdMonitor_Ref_t monitorRef;             ///< Reference to the monitor object
    secSocket_Ctx_t*   secureCtxPtr;           ///< Secure socket context pointer
    short              events;                 ///< Bitmap of events that occurred
    void*              userPtr;                ///< User-defined pointer for socket event handler
    le_socket_EventHandler_t eventHandler;     ///< User-defined callback for ocket event handler
}
SocketCtx_t;

//--------------------------------------------------------------------------------------------------
// Internal variables
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for sockets.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(SocketPool, MAX_SOCKET_NB, sizeof(SocketCtx_t));

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool reference for the sockets pool.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SocketPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for the sockets pool.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t SocketRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Retrigger socket event handler in case more data needs to be read from secure socket
 */
//--------------------------------------------------------------------------------------------------
static void ReadMoreAsyncData
(
    void*                   param1Ptr,  ///< [IN] Socket context pointer
    void*                   param2Ptr   ///< [IN] Unused parameter
);

//--------------------------------------------------------------------------------------------------
// Internal functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pick an unused socket context from the socket pool and return it.
 *
 * @return Socket file descriptor
 */
//--------------------------------------------------------------------------------------------------
static SocketCtx_t* NewSocketContext
(
    void
)
{
    SocketCtx_t* contextPtr = NULL;

    // Alloc memory from pool
    contextPtr = le_mem_TryAlloc(SocketPoolRef);

    if (NULL == contextPtr)
    {
        LE_ERROR("Unable to allocate a socket context from pool");
        return NULL;
    }

    memset(contextPtr, 0x00, sizeof(SocketCtx_t));

    // Create a safe reference for this object
    contextPtr->reference = le_ref_CreateRef(SocketRefMap, contextPtr);

    // Ensure socket context pointer is NULL
    contextPtr->secureCtxPtr = NULL;

    return contextPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Free a socket context and make it available for future use.
 *
 * @return Socket file descriptor
 */
//--------------------------------------------------------------------------------------------------
static void FreeSocketContext
(
    SocketCtx_t*    contextPtr    ///< [IN] Socket context pointer
)
{
    le_ref_DeleteRef(SocketRefMap, contextPtr->reference);
    memset(contextPtr, 0x00, sizeof(SocketCtx_t));
    le_mem_Release(contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Find socket context given its file descriptor.
 *
 * @return Socket file descriptor
 */
//--------------------------------------------------------------------------------------------------
static SocketCtx_t* FindSocketContext
(
    int fd      ///< [IN] Socket file descriptor
)
{
    // Check input parameter
    if (fd == -1)
    {
        LE_WARN("Uninitialized socket file descriptor");
        return NULL;
    }

    // Initialize the socket pool and the socket reference map if not yet done
    if (!SocketPoolRef)
    {
        SocketPoolRef = le_mem_InitStaticPool(SocketPool, MAX_SOCKET_NB, sizeof(SocketCtx_t));
        SocketRefMap = le_ref_CreateMap("le_socketLibMap", MAX_SOCKET_NB);
    }

    SocketCtx_t* contextPtr = NULL;
    le_ref_IterRef_t iterator  = le_ref_GetIterator(SocketRefMap);

    // Find and return socket context that uses the input file descriptor
    while (le_ref_NextNode(iterator) == LE_OK)
    {
        contextPtr = (SocketCtx_t *)le_ref_GetValue(iterator);
        if (!contextPtr)
        {
            continue;
        }

        if (contextPtr->fd == fd)
        {
            return contextPtr;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sockets events handler
 */
//--------------------------------------------------------------------------------------------------
static void SocketEventsHandler
(
    int fd,           ///< [IN] Socket file descriptor
    short events      ///< [IN] Bitmap of events that occurred
)
{
    SocketCtx_t* contextPtr = FindSocketContext(fd);
    if (!contextPtr)
    {
        return;
    }

    if (events & POLLOUT)
    {
        // In le_fdMonitor component, POLLOUT event is raised continuously when writing to the FD
        // is possible. The event is repeated indefinitely. Here, when POLLOUT event is received,
        // we disable it right after. This way, the notification is sent only once.
        if (contextPtr->monitorRef)
        {
            le_fdMonitor_Disable(contextPtr->monitorRef, POLLOUT);
        }
    }

    if (events & POLLRDHUP)
    {
        le_fdMonitor_Delete(contextPtr->monitorRef);
        contextPtr->monitorRef = NULL;
    }

    if (contextPtr->eventHandler)
    {
        contextPtr->eventHandler(contextPtr->reference, events, contextPtr->userPtr);

        // In secure context, low-level layers may read more data from socket than the data size
        // requested by client application. In this case, we need to notify again until all
        // the data is consumed
        if (contextPtr->isSecure)
        {
            if (secSocket_IsDataAvailable(contextPtr->secureCtxPtr))
            {
                contextPtr->events = events;
                le_event_QueueFunction(ReadMoreAsyncData, (void*)contextPtr->reference, NULL);

                // Disable POLLIN monitoring to prevent race condition between FD event and queued
                // function. POLLIN is renabled when the runloop calls ReadMoreAsyncData()
                if (contextPtr->monitorRef)
                {
                    le_fdMonitor_Disable(contextPtr->monitorRef, POLLIN);
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Re-trigger socket event handler in case more data needs to be read from secure socket
 */
//--------------------------------------------------------------------------------------------------
static void ReadMoreAsyncData
(
    void*   param1Ptr,      ///< [IN] Socket context pointer
    void*   param2Ptr       ///< [IN] Unused parameter
)
{
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, param1Ptr);
    if (!contextPtr)
    {
        LE_WARN("Referenece not found");
        return;
    }

    if (!contextPtr->monitorRef)
    {
        LE_INFO("Monitoring disabled");
        return;
    }

    le_fdMonitor_Enable(contextPtr->monitorRef, POLLIN);
    SocketEventsHandler(contextPtr->fd, contextPtr->events);
}

//--------------------------------------------------------------------------------------------------
// Public functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Create a a socket reference and stores the user configuration in a dedicated context.
 *
 * @note
 *  - PDP source address (srcAddr) can be set to Null. In this case, the default PDP profile will
 *    be used and the address family will be selected in the following order: Try IPv4 first, then
 *    try IPv6
 *
 * @return
 *  - Reference to the created context
 */
//--------------------------------------------------------------------------------------------------
le_socket_Ref_t le_socket_Create
(
    char*           hostPtr,         ///< [IN] Host address pointer
    uint16_t        port,            ///< [IN] Port number
    char*           srcAddr,         ///< [IN] Source IP address
    SocketType_t    type             ///< [IN] Socket type (TCP, UDP)
)
{
    SocketCtx_t* contextPtr = NULL;

    // Check input parameters
    if (NULL == hostPtr)
    {
        LE_ERROR("Unspecified host address");
        return NULL;
    }

    // Allocate a socket context and save server parameters
    contextPtr = NewSocketContext();
    if (NULL == contextPtr)
    {
        LE_ERROR("Unable to allocate a socket context from pool");
        return NULL;
    }

    if (srcAddr)
    {
        if (strlen(srcAddr) >= sizeof(contextPtr->srcAddr))
        {
            LE_ERROR("Source address too long");
            FreeSocketContext(contextPtr);
            return NULL;
        }
        else
        {
            strncpy(contextPtr->srcAddr, srcAddr, sizeof(contextPtr->srcAddr));
        }
    }
    else
    {
        contextPtr->srcAddr[0] = '\0';
    }

    strncpy(contextPtr->host, hostPtr, sizeof(contextPtr->host)-1);
    contextPtr->port      = port;
    contextPtr->type      = type;
    contextPtr->fd        = -1;
    contextPtr->timeout   = COMM_TIMEOUT_DEFAULT_MS;
    contextPtr->isMonitoring = false;

    return contextPtr->reference;
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete a previously created socket and free allocated resources.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_Delete
(
    le_socket_Ref_t    ref   ///< [IN] Socket context reference
)
{
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);

    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->monitorRef)
    {
        le_fdMonitor_Delete(contextPtr->monitorRef);
        contextPtr->monitorRef = NULL;
    }

    if (contextPtr->isSecure)
    {
        secSocket_Disconnect(contextPtr->secureCtxPtr);
        secSocket_Delete(contextPtr->secureCtxPtr);
    }
    else
    {
        netSocket_Disconnect(contextPtr->fd);
    }

    FreeSocketContext(contextPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add root CA certificates to the socket in order to make the connection secure.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FORMAT_ERROR  Invalid certificate
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_AddCertificate
(
    le_socket_Ref_t   ref,             ///< [IN] Socket context reference
    const uint8_t*    certificatePtr,  ///< [IN] Certificate Pointer
    size_t            certificateLen   ///< [IN] Certificate Length
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);

    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if ((!certificatePtr) || (certificateLen == 0))
    {
        LE_ERROR("Wrong parameter: %p, %"PRIuS, certificatePtr, certificateLen);
        return LE_BAD_PARAMETER;
    }
    if (contextPtr->secureCtxPtr == NULL)
    {
        // Need to initialize the secure socket before adding the certificate
        status = secSocket_Init(&(contextPtr->secureCtxPtr));
        if (status != LE_OK)
        {
            LE_ERROR("Unable to initialize the secure socket");
            return status;
        }
    }

    status = secSocket_AddCertificate(contextPtr->secureCtxPtr, certificatePtr, certificateLen);
    if (status == LE_OK)
    {
        LE_DEBUG("Added a certificate");
        contextPtr->hasCert = true;
    }
    else
    {
        LE_ERROR("Unable to add certificate");
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add the module's own certificates to the socket context for mutual authentication.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FORMAT_ERROR  Invalid certificate
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_AddOwnCertificate
(
    le_socket_Ref_t   ref,             ///< [IN] Socket context reference
    const uint8_t*    certificatePtr,  ///< [IN] Certificate pointer
    size_t            certificateLen   ///< [IN] Certificate length
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);

    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if ((!certificatePtr) || (certificateLen == 0))
    {
        LE_ERROR("Wrong parameter: %p, %zu", certificatePtr, certificateLen);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->secureCtxPtr == NULL)
    {
        // Need to initialize the secure socket before adding the certificate
        status = secSocket_Init(&(contextPtr->secureCtxPtr));
        if (status != LE_OK)
        {
            LE_ERROR("Unable to initialize the secure socket");
            return status;
        }
    }

    status = secSocket_AddOwnCertificate(contextPtr->secureCtxPtr, certificatePtr, certificateLen);
    if (status == LE_OK)
    {
        LE_DEBUG("Added a certificate");
        contextPtr->hasCert = true;
    }
    else
    {
        LE_ERROR("Unable to add certificate");
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add the module's own private key to the socket context for mutual authentication.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_AddOwnPrivateKey
(
    le_socket_Ref_t   ref,             ///< [IN] Socket context reference
    const uint8_t*    pkeyPtr,         ///< [IN] Private key pointer
    size_t            pkeyLen          ///< [IN] Private key length
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);

    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if ((!pkeyPtr) || (pkeyLen == 0))
    {
        LE_ERROR("Wrong parameter: %p, %zu", pkeyPtr, pkeyLen);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->secureCtxPtr == NULL)
    {
        // Need to initialize the secure socket before adding the certificate
        status = secSocket_Init(&(contextPtr->secureCtxPtr));
        if (status != LE_OK)
        {
            LE_ERROR("Unable to initialize the secure socket");
            return status;
        }
    }

    status = secSocket_AddOwnPrivateKey(contextPtr->secureCtxPtr, pkeyPtr, pkeyLen);
    if (status == LE_OK)
    {
        LE_DEBUG("Added a certificate");
        contextPtr->hasCert = true;
    }
    else
    {
        LE_ERROR("Unable to add certificate");
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set cipher suites to the socket in order to make the connection secure.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_SetCipherSuites
(
    le_socket_Ref_t   ref,             ///< [IN] Socket context reference
    uint8_t           cipherIdx        ///< [IN] Cipher suites index
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->secureCtxPtr == NULL)
    {
        // Need to initialize the secure socket before adding the certificate
        status = secSocket_Init(&(contextPtr->secureCtxPtr));
        if (status != LE_OK)
        {
            LE_ERROR("Unable to initialize the secure socket");
            return status;
        }
    }

    secSocket_SetCipherSuites(contextPtr->secureCtxPtr, cipherIdx);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set authentication type to the socket in order to make the connection secure.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_SetAuthType
(
    le_socket_Ref_t   ref,             ///< [IN] Socket context reference
    uint8_t           auth             ///< [IN] Authentication type
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->secureCtxPtr == NULL)
    {
        // Need to initialize the secure socket before adding the certificate
        status = secSocket_Init(&(contextPtr->secureCtxPtr));
        if (status != LE_OK)
        {
            LE_ERROR("Unable to initialize the secure socket");
            return status;
        }
    }

    secSocket_SetAuthType(contextPtr->secureCtxPtr, auth);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initiate a connection with the server using the defined configuration.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_TIMEOUT       Timeout during execution
 *  - LE_UNAVAILABLE   Unable to reach the server or DNS issue
 *  - LE_FAULT         Internal error
 *  - LE_NO_MEMORY     Memory allocation issue
 *  - LE_CLOSED        In case of end of file error
 *  - LE_COMM_ERROR    Connection failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_Connect
(
    le_socket_Ref_t    ref   ///< [IN] Socket context reference
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);

    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->hasCert)
    {
        status = secSocket_Connect(contextPtr->secureCtxPtr, contextPtr->host,
                                   contextPtr->port, contextPtr->srcAddr,
                                   contextPtr->type, &(contextPtr->fd));
        contextPtr->isSecure = (status == LE_OK);
    }
    else
    {
        status = netSocket_Connect(contextPtr->host, contextPtr->port, contextPtr->srcAddr,
                                   contextPtr->type, &(contextPtr->fd));
    }

    if ((contextPtr->isMonitoring) && (!contextPtr->monitorRef))
    {
        contextPtr->monitorRef = le_fdMonitor_Create("SocketLibrary", contextPtr->fd,
                                                     SocketEventsHandler,
                                                     POLLIN | POLLRDHUP | POLLOUT);
        if (!contextPtr->monitorRef)
        {
            LE_ERROR("Unable to create an FD monitor object");
            return LE_FAULT;
        }
    }

    if (status != LE_OK)
    {
        LE_ERROR("Unable to connect");
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Secures an existing connection by performing TLS negotiation.
 *
 * @note
 *   - Certificate must be added beforehand via @c le_socket_AddCertificate() to succeed
 *   - Only supported on RTOS based systems
 *
 * @return
 *  - LE_OK                 Function success
 *  - LE_BAD_PARAMETER      Invalid parameter
 *  - LE_NOT_FOUND          Certificate not found
 *  - LE_CLOSED             Socket is not connected
 *  - LE_NOT_IMPLEMENTED    Not implemented for device
 *  - LE_TIMEOUT            Timeout during execution
 *  - LE_FAULT              Internal error
 *  - LE_NO_MEMORY          Memory allocation issue
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_SecureConnection
(
    le_socket_Ref_t   ref              ///< [IN] Socket context reference
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);

    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if (!contextPtr->hasCert)
    {
        LE_ERROR("No certificate associated to socket");
        return LE_NOT_FOUND;
    }

    if (contextPtr->fd == -1)
    {
        LE_ERROR("Socket not connected");
        return LE_CLOSED;
    }

    status = secSocket_PerformHandshake(contextPtr->secureCtxPtr,
                                        contextPtr->host,
                                        contextPtr->fd);
    if (status != LE_OK)
    {
        LE_ERROR("Socket not connected");
        return status;
    }

    contextPtr->isSecure = true;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close the socket connection.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_Disconnect
(
    le_socket_Ref_t    ref   ///< [IN] Socket context reference
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->isSecure)
    {
        status = secSocket_Disconnect(contextPtr->secureCtxPtr);
    }
    else
    {
        status = netSocket_Disconnect(contextPtr->fd);
    }

    if (contextPtr->monitorRef)
    {
        le_fdMonitor_Delete(contextPtr->monitorRef);
        contextPtr->monitorRef = NULL;
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send data through the socket.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_TIMEOUT       Timeout during execution
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_Send
(
    le_socket_Ref_t  ref,        ///< [IN] Socket context reference
    char*            dataPtr,    ///< [IN] Data pointer
    size_t           dataLen     ///< [IN] Data length
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if (!dataPtr)
    {
        LE_ERROR("Wrong parameter: %p", dataPtr);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->fd == -1)
    {
        LE_ERROR("Socket not connected");
        return LE_FAULT;
    }

    if (contextPtr->isMonitoring)
    {
        // Enable POLLOUT event just before sending data. Thus, when writing is possible again,
        // an event is raised.
        le_fdMonitor_Enable(contextPtr->monitorRef, POLLOUT);
    }

    if (contextPtr->isSecure)
    {
        status = secSocket_Write(contextPtr->secureCtxPtr, dataPtr, dataLen);
    }
    else
    {
        status = netSocket_Write(contextPtr->fd, dataPtr, dataLen);
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read up to 'dataLenPtr' characters from the socket in a blocking way until data is received or
 * defined timeout value is reached.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_TIMEOUT       Timeout during execution
 *  - LE_FAULT         Internal error
 *  - LE_WOULD_BLOCK   Would have blocked if non-blocking behaviour was not requested
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_Read
(
    le_socket_Ref_t  ref,        ///< [IN] Socket context reference
    char*            dataPtr,    ///< [IN] Read buffer pointer
    size_t*          dataLenPtr  ///< [INOUT] Input: size of the buffer. Output: data size read
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if ((!dataPtr) || (!dataLenPtr))
    {
        LE_ERROR("Wrong parameter: %p, %p", dataPtr, dataLenPtr);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->fd == -1)
    {
        LE_ERROR("Socket not connected");
        return LE_FAULT;
    }

    // Disable FD Monitor if it exists to avoid two different threads selecting the
    // same file descriptor
    if (contextPtr->monitorRef)
    {
        le_fdMonitor_Disable(contextPtr->monitorRef, POLLIN);
    }

    if (contextPtr->isSecure)
    {
        status = secSocket_Read(contextPtr->secureCtxPtr, dataPtr, dataLenPtr, contextPtr->timeout);
    }
    else
    {
        status = netSocket_Read(contextPtr->fd, dataPtr, dataLenPtr, contextPtr->timeout);
    }

    if ((status != LE_OK) && (status != LE_WOULD_BLOCK))
    {
        LE_ERROR("Read failed. Status: %d", status);
    }

    // Re-enable fdMonitor
    if (contextPtr->monitorRef)
    {
        le_fdMonitor_Enable(contextPtr->monitorRef, POLLIN);
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initiate a server connection by listening on the specified port.
 *
 * @return
 *  - LE_OK               Function success
 *  - LE_BAD_PARAMETER    Invalid parameter
 *  - LE_FAULT            Internal error
 *  - LE_UNAVAILABLE      Unable to reach the server or DNS issue
 *  - LE_COMM_ERROR       Connection failure
 *  - LE_NOT_IMPLEMENTED  Function not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_Listen
(
    le_socket_Ref_t    ref       ///< [IN] Socket context reference
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->isSecure)
    {
        LE_ERROR("Function not supported");
        return LE_NOT_IMPLEMENTED;
    }
    else
    {
        status = netSocket_Listen(contextPtr->port, contextPtr->srcAddr,
                                   contextPtr->type, &(contextPtr->fd));
    }

    if ((contextPtr->isMonitoring) && (!contextPtr->monitorRef))
    {
        contextPtr->monitorRef = le_fdMonitor_Create("SocketLibrary", contextPtr->fd,
                                                     SocketEventsHandler,
                                                     POLLIN | POLLRDHUP | POLLOUT);
        if (!contextPtr->monitorRef)
        {
            LE_ERROR("Unable to create an FD monitor object");
            return LE_FAULT;
        }
    }

    if (status != LE_OK)
    {
        LE_ERROR("Listen failed. Status: %d", status);
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Accept a remote client connection and store the spawned socket info
 *
 * @return
 *  - LE_OK               Function success
 *  - LE_BAD_PARAMETER    Invalid parameter
 *  - LE_UNAVAILABLE      Unable to accept a client socket
 *  - LE_NOT_IMPLEMENTED  Function not supported
 *  - LE_FAULT            Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_Accept
(
    le_socket_Ref_t   ref,          ///< [IN]  Socket context reference
    char*             childAddr,    ///< [OUT] Accepted IP address
    int*              childPort,    ///< [OUT] Accepted port number
    le_socket_Ref_t*  childSockRef  ///< [OUT] Accepted socket context reference
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);
    SocketCtx_t* childContextPtr = NULL;
    struct sockaddr_storage clientAddress;
    int acceptedFd = -1;
    char clientIp[ADDR_MAX_LEN]={0};

    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->isSecure)
    {
        LE_ERROR("Function not supported");
        return LE_NOT_IMPLEMENTED;
    }
    else
    {
        status = netSocket_Accept(contextPtr->fd,
                                  (struct sockaddr*)&clientAddress,
                                  &acceptedFd);
    }

    if (status != LE_OK)
    {
        LE_ERROR("Accept failed. Status: %d", status);
        return status;
    }

    childContextPtr = NewSocketContext();
    if (NULL == childContextPtr)
    {
        LE_ERROR("Unable to allocate a socket context from pool");
        netSocket_Disconnect(acceptedFd);
        return LE_FAULT;
    }

    if (AF_INET == clientAddress.ss_family)
    {
        struct sockaddr_in *temp = (struct sockaddr_in*)&clientAddress;
        inet_ntop(AF_INET, &(temp->sin_addr), clientIp, ADDR_MAX_LEN);
        childContextPtr->port = (int)ntohs(temp->sin_port);
    }
    else
    {
        struct sockaddr_in6 *temp = (struct sockaddr_in6*)&clientAddress;
        inet_ntop(AF_INET6, &(temp->sin6_addr), clientIp, ADDR_MAX_LEN);
        childContextPtr->port = (int)ntohs(temp->sin6_port);
    }
    childContextPtr->type    = contextPtr->type;
    childContextPtr->fd      = acceptedFd;
    childContextPtr->timeout = COMM_TIMEOUT_DEFAULT_MS;
    childContextPtr->isMonitoring = false;
    strncpy(childContextPtr->host, clientIp, ADDR_MAX_LEN);
    strncpy(childAddr, clientIp, ADDR_MAX_LEN);
    *childPort = childContextPtr->port;
    *childSockRef = childContextPtr->reference;
    LE_INFO("Accepted connection on FD:%d, address:%s, port:%d", acceptedFd,
                                                                 childContextPtr->host,
                                                                 childContextPtr->port);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the socket communication timeout. This timeout specifies the interval that the read API
 * should block waiting for data reception.
 *
 * @note If this interval is set to zero, then the read API returns immediatly.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_SetTimeout
(
    le_socket_Ref_t  ref,       ///< [IN] Socket context reference
    uint32_t         timeout    ///< [IN] Timeout in milliseconds
)
{
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    contextPtr->timeout = timeout;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable or disable monitoring on the socket file descriptor. By default, monitoring is disabled.
 *
 * @note When monitoring is activated, socket events (e.g: POLLIN, POLLOUT, POLLRDHUP, ..)
 *       can be retrieved by using le_socket_AddEventHandler() API.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_DUPLICATE     Request already executed
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_SetMonitoring
(
    le_socket_Ref_t  socketRef,      ///< [IN] Socket context reference
    bool             enable          ///< [IN] True to activate monitoring, false otherwise
)
{
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, socketRef);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", socketRef);
        return LE_BAD_PARAMETER;
    }

    if ((contextPtr->monitorRef != NULL) == enable)
    {
        LE_INFO("Request was already executed");
        return LE_DUPLICATE;
    }

    if (enable)
    {
        // Check if the FD has already been created and connection started. In this case, FD
        // monitoring needs to be started immediatly. Otherwise, monitoring will be activated
        // after socket creation.
        if (contextPtr->fd != -1)
        {
            contextPtr->monitorRef = le_fdMonitor_Create("SocketLibrary", contextPtr->fd,
                                                         SocketEventsHandler,
                                                         POLLIN | POLLRDHUP | POLLOUT);
            if (!contextPtr->monitorRef)
            {
                LE_ERROR("Unable to create an FD monitor object");
                return LE_FAULT;
            }
        }
    }
    else
    {
        le_fdMonitor_Delete(contextPtr->monitorRef);
        contextPtr->monitorRef = NULL;
    }

    contextPtr->isMonitoring = enable;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check whether the socket monitoring is enabled or not
 *
 * @return
 *  - True if monitoring is enabled, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool le_socket_IsMonitoring
(
    le_socket_Ref_t  socketRef      ///< [IN] Socket context reference
)
{
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, socketRef);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", socketRef);
        return LE_BAD_PARAMETER;
    }

    return contextPtr->isMonitoring;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add a handler to monitor socket events.
 *
 * @note Monitoring is performed by event loop. Thus, any thread that calls this API should provide
 *       an event loop to catch socket events.
 *
 * @note Check @c le_socket_EventHandler_t prototype definition to get the list of monitored events.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_AddEventHandler
(
    le_socket_Ref_t          socketRef,       ///< [IN] Socket context reference
    le_socket_EventHandler_t handlerFunc,     ///< [IN] Handler function
    void*                    userPtr          ///< [IN] User-defined data pointer
)
{
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, socketRef);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", socketRef);
        return LE_BAD_PARAMETER;
    }

    contextPtr->userPtr = userPtr;
    contextPtr->eventHandler = handlerFunc;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Trigger a call to the monitoring event handler when POLLOUT is ready again
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_TrigMonitoring
(
    le_socket_Ref_t          socketRef       ///< [IN] Socket context reference
)
{
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, socketRef);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", socketRef);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->monitorRef == NULL)
    {
        LE_ERROR("Monitoring is not enabled");
        return LE_FAULT;
    }

    // Since POLLOUT event is sent continuously when writing to the FD is possible, enabling it
    // here ensures that SocketEventsHandler will be called right after.
    le_fdMonitor_Enable(contextPtr->monitorRef, POLLOUT);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Component once initializer.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT_ONCE
{
    // Initialize the socket pool and the socket reference map
    SocketPoolRef = le_mem_InitStaticPool(SocketPool, MAX_SOCKET_NB, sizeof(SocketCtx_t));
    SocketRefMap = le_ref_CreateMap("le_socketLibMap", MAX_SOCKET_NB);

    // Initialize the secure socket memory pools
    secSocket_InitializeOnce();
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization function
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_DEBUG("socketLibrary initializing");
}
