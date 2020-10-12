/**
 * @file networkSocket.c
 *
 * This file provides a Network Socket implementation of the RPC Communication API (le_comm.h).
 * It allows for testing two RPC Proxies, using a TCP Socket session.
 *
 * NOTE:  Temporary interim solution for testing the RPC Proxy communication framework
 *        while under development.
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#include "legato.h"
#include "interfaces.h"
#include "le_comm.h"
#include <sys/socket.h>

#ifdef LE_CONFIG_LINUX
#include <arpa/inet.h>
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of outstanding (pending) socket client connections
 */
//--------------------------------------------------------------------------------------------------
#define NETWORK_SOCKET_MAX_CONNECT_REQUEST_BACKLOG   100

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Socket Handle records
 */
//--------------------------------------------------------------------------------------------------
#define NETWORK_SOCKET_HANDLE_RECORD_MAX             10

//--------------------------------------------------------------------------------------------------
/**
 * Maximum IPv6 Address string length
 */
//--------------------------------------------------------------------------------------------------
#define NETWORK_SOCKET_IP6ADDR_STRLEN_MAX            49

//--------------------------------------------------------------------------------------------------
/**
 * Reference to File descriptor monitor object.
 */
//--------------------------------------------------------------------------------------------------
static le_fdMonitor_Ref_t FdMonitorRef = NULL;
static short PollingEvents = 0x00;


//--------------------------------------------------------------------------------------------------
/**
 * Socket Handle-Record Structure - Defines the data of a Socket connection.
 */
//--------------------------------------------------------------------------------------------------
typedef struct HandleRecord
{
    int fd; ///< File-descriptor of the socket connection
    bool isListeningFd; ///< Identifies if this is a listening server socket
    void* parentRecordPtr; ///< Pointer to parent (listening) socket record [client sockets only]
}
HandleRecord_t;

//--------------------------------------------------------------------------------------------------
/**
 * This pool is used to allocate memory for the Handle record.
 * Initialized in COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(HandleRecordPool,
                          NETWORK_SOCKET_HANDLE_RECORD_MAX,
                          sizeof(HandleRecord_t));

static le_mem_PoolRef_t HandleRecordPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Hash Map to store the Handle record (value), using the File Descriptor (key).
 * Initialized in COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(HandleRecordByFileDescriptorHashMap, NETWORK_SOCKET_HANDLE_RECORD_MAX);
static le_hashmap_Ref_t HandleRecordByFileDescriptor = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Registered Asynchronous Receive Callback Handler function
 */
//--------------------------------------------------------------------------------------------------
static le_comm_CallbackHandlerFunc_t AsyncReceiveHandlerFuncPtr = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Registered Asycnhronous Connection Callback Handler function
 */
//--------------------------------------------------------------------------------------------------
static le_comm_CallbackHandlerFunc_t AsyncConnectionHandlerFuncPtr = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Connection Socket File descriptor and monitor object
 */
//--------------------------------------------------------------------------------------------------
static le_fdMonitor_Ref_t ConnectionFdMonitorRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Network Socket IP Address string
 */
//--------------------------------------------------------------------------------------------------
static char NetworkSocketIpAddress[NETWORK_SOCKET_IP6ADDR_STRLEN_MAX];

//--------------------------------------------------------------------------------------------------
/**
 * Network Socket TCP Listening Port Number string
 */
//--------------------------------------------------------------------------------------------------
static uint16_t NetworkSocketTCPListeningPort;


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the RPC Communication implementation.
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
#ifndef RPC_PROXY_LOCAL_SERVICE
__attribute__((constructor))
#endif
static void networkSocketInitialize(void)
{
    LE_INFO("RPC Network Socket Init start");

    if (HandleRecordPoolRef == NULL)
    {
        // NOTE: Must be performed once.
        HandleRecordPoolRef = le_mem_InitStaticPool(HandleRecordPool,
                                                    NETWORK_SOCKET_HANDLE_RECORD_MAX,
                                                    sizeof(HandleRecord_t));
    }

    if (HandleRecordByFileDescriptor == NULL) {
        // Create hash map for storing the handle record (value) using the FD (key)
        // NOTE: Must be performed once.
        HandleRecordByFileDescriptor = le_hashmap_InitStatic(
                                           HandleRecordByFileDescriptorHashMap,
                                           NETWORK_SOCKET_HANDLE_RECORD_MAX,
                                           le_hashmap_HashVoidPointer,
                                           le_hashmap_EqualsVoidPointer);
    }

    LE_INFO("RPC Network Socket Init done");
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the RPC Communication implementation.
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Callback function to receive events on a connection and pass them onto the RPC Proxy
 */
//--------------------------------------------------------------------------------------------------
static void AsyncRecvHandler(void* handle, short events)
{
    // Retrieve the Handle record, using the FD
    HandleRecord_t* connectionRecordPtr = le_hashmap_Get(HandleRecordByFileDescriptor, handle);
    if (connectionRecordPtr == NULL)
    {
        LE_ERROR("Unable to find matching Handle Record, fd [%" PRIiPTR "]", (intptr_t) handle);
        return;
    }

    // Notify the RPC Proxy
    AsyncReceiveHandlerFuncPtr(connectionRecordPtr, events);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to Parse Command Line Arguments
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseCommandLineArgs
(
    const int argc,
    const char* argv[]
)
{
    le_result_t result = LE_OK;

    LE_INFO("Parsing Command Line Arguments");
    if (argc != 2)
    {
        LE_INFO("Invalid Command Line Argument, argc = [%d]", argc);
        return LE_BAD_PARAMETER;
    }

    // Extract the Server's IP address
    le_utf8_Copy(NetworkSocketIpAddress, argv[0], sizeof(NetworkSocketIpAddress), NULL);
    LE_INFO("Setting Network Socket IP Address [%s]", NetworkSocketIpAddress);

    // Extract the Server's TCP Listening port
    int portNumber = atoi(argv[1]);

    // Verify the TCP Port range
    if (portNumber > 0xffff)
    {
        return LE_BAD_PARAMETER;
    }
    NetworkSocketTCPListeningPort = portNumber;
    LE_INFO("Setting Network Socket TCP Port [%" PRIu16 "]",
            NetworkSocketTCPListeningPort);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Callback function to handle connections and pass them onto the RPC Proxy
 */
//--------------------------------------------------------------------------------------------------
static void ConnectionRecvHandler(void* handle, short events)
{
    HandleRecord_t *connectionRecordPtr = NULL;
    short connectionEvents = events;

#ifdef SOCKET_SERVER
    int clientFd = -1;

    if (!(events & POLLIN))
    {
        LE_ERROR("Unexpected fd event(s): 0x%hX", events);
        return;
    }

    // Retrieve the Handle record, using the FD
    HandleRecord_t* parentRecordPtr =
        le_hashmap_Get(HandleRecordByFileDescriptor, handle);

    if (parentRecordPtr == NULL)
    {
        LE_ERROR("Unable to find matching Handle Record, "
                 "fd [%" PRIiPTR "]", (intptr_t) handle);
        return;
    }

    // Accept the connection, setting the connection to be non-blocking.
    clientFd = accept4((int)(size_t)handle, NULL, NULL, SOCK_NONBLOCK);
    if (clientFd < 0)
    {
        LE_ERROR("Failed to accept client connection. Errno %d", errno);
    }

    LE_INFO("Accepting Client socket connection, fd [%d]", clientFd);
    connectionRecordPtr = le_mem_AssertAlloc(HandleRecordPoolRef);

    // Initialize the connection record
    connectionRecordPtr->fd = clientFd;
    connectionRecordPtr->isListeningFd = false;
    connectionRecordPtr->parentRecordPtr = parentRecordPtr;

    le_hashmap_Put(HandleRecordByFileDescriptor,
                   (void*)(intptr_t) connectionRecordPtr->fd,
                   connectionRecordPtr);

#else
    if (!(events & POLLOUT))
    {
        LE_ERROR("Unexpected fd event(s): 0x%hX", events);
        return;
    }

    int socketErr = 0;
    socklen_t socketLen = sizeof(socketErr);
    if (getsockopt((int)(size_t)handle,
                   SOL_SOCKET,
                   SO_ERROR,
                   &socketErr,
                   &socketLen) < 0)
    {
        LE_ERROR("Unable to retrieve Socket Error option, "
                 "fd [%" PRIiPTR "], ErrorNo [%d]",
                 (intptr_t) handle,
                 errno);
        return;
    }

    if (socketErr != 0)
    {
        connectionEvents = POLLERR;
        LE_INFO("Connect failed with errno %d", socketErr);
    }

    // Retrieve the Handle record, using the FD
    connectionRecordPtr = le_hashmap_Get(HandleRecordByFileDescriptor, handle);
    if (connectionRecordPtr == NULL)
    {
        LE_ERROR("Unable to find matching Connection Handle Record, "
                 "fd [%" PRIiPTR "]", (intptr_t) handle);
        return;
    }

    // Set the parent record to ourself
    connectionRecordPtr->parentRecordPtr = connectionRecordPtr;

#endif

    LE_INFO("Notifying RPC Proxy socket is connected, fd [%d]",
            connectionRecordPtr->fd);

    if (AsyncConnectionHandlerFuncPtr != NULL)
    {
        // Notify the RPC Proxy of the Client connection
        AsyncConnectionHandlerFuncPtr(connectionRecordPtr, connectionEvents);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Creating a RPC Network-Socket Communication Channel
 *
 * Return Code values:
 *      - LE_OK if successfully,
 *      - otherwise failure
 *
 * @return
 *      Opague handle to the Communication Channel.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void* le_comm_Create
(
    const int argc,         ///< [IN] Number of strings pointed to by argv.
    const char *argv[],     ///< [IN] Pointer to an array of character strings.
    le_result_t* resultPtr  ///< [OUT] Return Code
)
{
    //
    // Create Connection Socket
    //

    // Verify result pointer is valid
    if (resultPtr == NULL)
    {
        LE_ERROR("resultPtr is NULL");
        return NULL;
    }

    // Check if Communication Globals need initialization
    networkSocketInitialize();

    if (!le_hashmap_isEmpty(HandleRecordByFileDescriptor))
    {
        LE_ERROR("Sanity Check Failure: Hashmap is not empty");
        *resultPtr = LE_FAULT;
        return NULL;
    }

    // Parse the Command Line arguments to extract the IP Address and TCP Port number
    *resultPtr = ParseCommandLineArgs(argc, argv);
    if (*resultPtr != LE_OK)
    {
        return NULL;
    }

    HandleRecord_t* connectionRecordPtr = le_mem_AssertAlloc(HandleRecordPoolRef);

    // Initialize the connection record
    connectionRecordPtr->fd = -1;
    connectionRecordPtr->isListeningFd = false;
    connectionRecordPtr->parentRecordPtr = NULL;

    // Create the socket
    connectionRecordPtr->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connectionRecordPtr->fd < 0)
    {
        LE_WARN("Failed to create AF_INET socket.  Errno = %d", errno);

        // Set the result pointer with a fault code
        *resultPtr = LE_FAULT;
        connectionRecordPtr->fd = -1;
        le_mem_Release(connectionRecordPtr);
        return NULL;
    }

    //
    // Set the Socket as Non-Blocking
    //

    // Retrieve existing socket options
    int opts = fcntl(connectionRecordPtr->fd, F_GETFL);
    if (opts < 0) {
        LE_ERROR("fcntl(F_GETFL)");

        // Close the Socket handle
        close(connectionRecordPtr->fd);
        connectionRecordPtr->fd = -1;

        *resultPtr = LE_FAULT;
        le_mem_Release(connectionRecordPtr);
        return NULL;
    }

    // Set Non-Blocking socket option
    opts = (opts | O_NONBLOCK);
    if (fcntl(connectionRecordPtr->fd, F_SETFL, opts) < 0) {
        LE_ERROR("fcntl(F_SETFL)");

        // Close the Socket handle
        close(connectionRecordPtr->fd);
        connectionRecordPtr->fd = -1;

        *resultPtr = LE_FAULT;
        le_mem_Release(connectionRecordPtr);
        return NULL;
    }

    // Set SO_REUSEADDR when rebinding the same address.
    int enable = 1;
    int result = setsockopt(connectionRecordPtr->fd,
                            SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (result < 0)
    {
        LE_ERROR("setsockopt(SO_REUSEADDR) failed");
        // Close the Socket handle
        close(connectionRecordPtr->fd);
        connectionRecordPtr->fd = -1;

        *resultPtr = LE_FAULT;
        le_mem_Release(connectionRecordPtr);
        return NULL;
    }

#ifdef SOCKET_SERVER
    struct sockaddr_in sockAddr;

    // Prepare the sockaddr_in structure
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = htons(NetworkSocketTCPListeningPort);

    // Bind
    if (bind(connectionRecordPtr->fd, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) < 0)
    {
        LE_WARN("Failed to bind socket, fd %d, result = %d",
                connectionRecordPtr->fd,
                errno);

        // Close the Socket handle
        close(connectionRecordPtr->fd);
        connectionRecordPtr->fd = -1;

        // Set the result pointer with a fault code
        *resultPtr = LE_FAULT;
        le_mem_Release(connectionRecordPtr);
        return NULL;
    }
#endif

    LE_INFO("Created AF_INET Socket, fd %d", connectionRecordPtr->fd);

    // Store the Handle record
    le_hashmap_Put(HandleRecordByFileDescriptor,
                   (void*)(intptr_t) connectionRecordPtr->fd,
                   connectionRecordPtr);

    return (connectionRecordPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for Registering a Callback Handler function to monitor events on the specific handle
 *
 * @return
 *      - LE_OK if successfully.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_comm_RegisterHandleMonitor
(
    void* handle,
    le_comm_CallbackHandlerFunc_t handlerFunc,
    short events
)
{
    char socketName[80];
    HandleRecord_t* connectionRecordPtr = (HandleRecord_t*) handle;

    if (events & POLLIN)
    {
        // Store the Asynchronous Receive callback function
        AsyncReceiveHandlerFuncPtr = handlerFunc;

        LE_INFO("Registering Asynchronous Receive callback on fd: %d", connectionRecordPtr->fd);
        snprintf(socketName, sizeof(socketName) - 1, "inetSocket-%d", connectionRecordPtr->fd);

        // Set the Polling Events
        PollingEvents = events;

#ifndef SOCKET_SERVER
        if (ConnectionFdMonitorRef != NULL)
        {
            // Delete Connection FD monitor
            le_fdMonitor_Delete(ConnectionFdMonitorRef);
            ConnectionFdMonitorRef = NULL;
        }
#endif

        // Create thread to monitor FD handle for activity, as defined by the events
        FdMonitorRef =
            le_fdMonitor_Create(
                socketName,
                connectionRecordPtr->fd,
                (le_fdMonitor_HandlerFunc_t) AsyncRecvHandler,
                PollingEvents);
    }
    else
    {
        // Store the Asynchronous Connection callback function
        AsyncConnectionHandlerFuncPtr = handlerFunc;
    }

    LE_INFO("Successfully registered handle_monitor callback, events [0x%x]", events);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Deleting RPC Network-Socket Communication Channel
 *
 * @return
 *      - LE_OK if successfully.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_comm_Delete (void* handle)
{
    HandleRecord_t* connectionRecordPtr = (HandleRecord_t*) handle;

    LE_INFO("Deleting AF_INET socket, fd %d .........", connectionRecordPtr->fd);

    if ((ConnectionFdMonitorRef != NULL) &&
        (le_fdMonitor_GetFd(ConnectionFdMonitorRef) == connectionRecordPtr->fd))
    {
        // Delete FD monitor
        le_fdMonitor_Delete(ConnectionFdMonitorRef);
        ConnectionFdMonitorRef = NULL;
    }

    if ((FdMonitorRef != NULL) &&
        (le_fdMonitor_GetFd(FdMonitorRef) == connectionRecordPtr->fd))
    {
        // Delete FD monitor
        le_fdMonitor_Delete(FdMonitorRef);
        FdMonitorRef = NULL;
    }

    // Remove the Handle record
    le_hashmap_Remove(HandleRecordByFileDescriptor, (void*)(intptr_t) connectionRecordPtr->fd);

    // Shut-down the open socket
    shutdown(connectionRecordPtr->fd, SHUT_RDWR);

    // Close Socket Handle
    close(connectionRecordPtr->fd);
    connectionRecordPtr->fd = -1;

    // Free the Handle Record memory
    le_mem_Release(connectionRecordPtr);
    connectionRecordPtr = NULL;

    // Reset the Network IP Address and TCP Port
    memset(NetworkSocketIpAddress, 0, sizeof(NetworkSocketIpAddress));
    NetworkSocketTCPListeningPort = 0;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Connecting RPC Network-Socket Communication Channel
 *
 * @return
 *      - LE_OK if successfully,
 *      - LE_IN_PROGRESS if pending on asynchronous connection,
 *      - otherwise failure
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_comm_Connect (void* handle)
{
    HandleRecord_t* connectionRecordPtr = (HandleRecord_t*) handle;

    LE_INFO("Connecting AF_INET socket, fd %d .........", connectionRecordPtr->fd);

    LE_INFO("Registering Connection Callback handler");

    char socketName[80];
    snprintf(socketName, sizeof(socketName) - 1, "inetSocket-%d", connectionRecordPtr->fd);

    // Create thread to monitor FD handle for activity, as defined by the events
    ConnectionFdMonitorRef =
        le_fdMonitor_Create(
            socketName,
            connectionRecordPtr->fd,
            (le_fdMonitor_HandlerFunc_t) &ConnectionRecvHandler,
#ifdef SOCKET_SERVER
            POLLIN);
#else
            POLLOUT);
#endif

    LE_INFO("Successfully registered listening callback function, events [0x%x]", POLLIN);

#ifndef SOCKET_SERVER
    struct sockaddr_in sockAddr;

    sockAddr.sin_addr.s_addr = inet_addr(NetworkSocketIpAddress);
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(NetworkSocketTCPListeningPort);

    // Connect the client socket
    int result =
        connect(
            connectionRecordPtr->fd,
            (struct sockaddr *) &sockAddr,
             sizeof(sockAddr));

    if (result != 0)
    {
        // Check the error code
        switch (errno)
        {
            case EACCES:
                return LE_NOT_PERMITTED;

           case ECONNREFUSED:
                return LE_NOT_FOUND;

            case EINPROGRESS:
                // Connection is in progress, and will be notified via the connection callback
                return LE_IN_PROGRESS;

            default:
                LE_ERROR("Connect failed with errno %d", errno);
                return LE_FAULT;
        }
    }
#else
    // Flag as a listening socket
    connectionRecordPtr->isListeningFd = true;

    // Listen
    if (listen(connectionRecordPtr->fd, NETWORK_SOCKET_MAX_CONNECT_REQUEST_BACKLOG) != 0)
    {
        LE_WARN("Server socket listen() call failed with errno %d", errno);
    }

#endif

    LE_INFO("Connecting AF_INET socket, fd %d ......... [DONE]", connectionRecordPtr->fd);
    return LE_IN_PROGRESS;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Disconnecting RPC Network-Socket Communication Channel
 *
 * @return
 *      - LE_OK if successfully.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_comm_Disconnect (void* handle)
{
    HandleRecord_t* connectionRecordPtr = (HandleRecord_t*) handle;
    if (FdMonitorRef != NULL)
    {
        // Disable FD Monitoring
        le_fdMonitor_Disable(FdMonitorRef, PollingEvents);
    }

    le_hashmap_Remove(HandleRecordByFileDescriptor, (void*)(intptr_t) connectionRecordPtr->fd);

    close(connectionRecordPtr->fd);
    connectionRecordPtr->fd = -1;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Sending Data over RPC Network-Socket Communication Channel
 *
 * @return
 *      - LE_OK if successfully.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_comm_Send (void* handle, const void* buf, size_t len)
{
    HandleRecord_t* connectionRecordPtr = (HandleRecord_t*) handle;
    ssize_t bytesSent;

    // Now send the message (retry if interrupted by a signal).
    do
    {
        bytesSent = send(connectionRecordPtr->fd, buf, len, 0);
    }
    while ((bytesSent < 0) && (errno == EINTR));

    if (bytesSent < 0)
    {
        switch (errno)
        {
            case EAGAIN:  // Same as EWOULDBLOCK
                return LE_NO_MEMORY;

            case ENOTCONN:
            case ECONNRESET:
                LE_WARN("sendmsg() failed with errno %d", errno);
                return LE_COMM_ERROR;

            default:
                LE_ERROR("sendmsg() failed with errno %d", errno);
                return LE_FAULT;
        }
    }

    if (bytesSent < len)
    {
        LE_ERROR("The last %zu data bytes (of %zu total) were discarded by sendmsg()!",
                 len - bytesSent,
                 len);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Receiving Data over RPC Network-Socket Communication Channel
 *
 * @return
 *      - LE_OK if successfully.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_comm_Receive (void* handle, void* buf, size_t* len)
{
    HandleRecord_t* connectionRecordPtr = (HandleRecord_t*) handle;
    ssize_t bytesReceived;

    // Keep trying to receive until we don't get interrupted by a signal.
    do
    {
        bytesReceived = recv(connectionRecordPtr->fd, buf, *len, 0);

    }
    while ((bytesReceived < 0) && (errno == EINTR));

    // If we failed, process the error and return.
    if (bytesReceived < 0)
    {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
        {
            // Set the length to zero and return
            *len = 0;
            return LE_OK;
        }
        else if (errno == ECONNRESET)
        {
            return LE_CLOSED;
        }
        else
        {
            LE_ERROR("recvmsg() failed with errno %d", errno);
            return LE_FAULT;
        }
    }

    // If we tried to receive data,
    if (buf != NULL)
    {
        // Set the received data count output parameter.
        *len = bytesReceived;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get Support Functions
 */
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * Function to retrieve an ID for the specified handle.
 * NOTE:  For logging or display purposes only.
 *
 * @return
 *      - Non-zero integer, if successful.
 *      - Negative one (-1), otherwise.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int le_comm_GetId
(
    void* handle
)
{
    if (handle == NULL)
    {
        return -1;
    }

    HandleRecord_t* connectionRecordPtr = (HandleRecord_t*) handle;
    if (connectionRecordPtr == NULL)
    {
        return -1;
    }

    return connectionRecordPtr->fd;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to retrieve the Parent Handle.
 * NOTE:  For asynchronous connections only.
 *
 * @return
 *      - Parent (Listening) handle, if successfully.
 *      - NULL, otherwise.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void* le_comm_GetParentHandle
(
    void* handle
)
{
    HandleRecord_t* connectionRecordPtr = (HandleRecord_t*) handle;
    if (connectionRecordPtr == NULL)
    {
        return NULL;
    }

    return (connectionRecordPtr->parentRecordPtr);
}
