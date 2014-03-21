//--------------------------------------------------------------------------------------------------
/** @file serviceDirectory.c
 *
 * @section sd_intro        Introduction
 *
 * Implementation of the Service Directory daemon.  This daemon keeps track of what messaging
 * services exist in the system, and what clients are currently waiting for services that don't
 * yet exist.
 *
 * The Service Directory implements @ref serviceDirectoryProtocol.h.
 */
//--------------------------------------------------------------------------------------------------
 /**
 * @section sd_data         Data Structures
 *
 * The data structures look like this:
 *
@verbatim

Service List -----------> Service <---------------------------------+
                           | ^ |                                    |
                           | | +---> Waiting Clients List --+--> Client Connection
                           | |
                           V |
                        Server Connection

@endverbatim
 *
 * Connection objects are used to keep track of the details of socket connections (e.g., the
 * file descriptor, File Descriptor Monitor object, etc.).  Server Connections keep track of
 * connections to servers.  Client Connections keep track of connections to clients.
 *
 * A Service object can be in the Service List if:
 * - A server is currently connected to the Service Directory and offering that service.
 *      In this case, a Server Connection is attached to the Service object.
 * - One or more clients are connected to the Service Directory and waiting for that service to
 *      be offered.  In this case, a list of Client Connections is attached to the Service object.
 *
 * The Service List is implemented using a Hashmap for performance reasons.  The key is the
 * combination of the protocol identifier and the service instance identifier (which are both
 * UTF-8 strings) found in the svcdir_ServiceId_t structure in the Service object.
 */
//--------------------------------------------------------------------------------------------------
/**
 * @section sd_deathDetection       Detection of Client or Server Death
 *
 * When a client or server process dies while it is connected to the Service Directory, the OS
 * will automatically close the the connection to that process.  The Service Directory will detect
 * this using an FD Monitor object (see @ref c_eventLoop) and update the data structures
 * accordingly.
 */
//--------------------------------------------------------------------------------------------------
 /**
 * @section sd_threading            Threading
 *
 * There is only one thread running in this process.
 */
//--------------------------------------------------------------------------------------------------
/**
 * @section sd_startUpSync          Start-Up Synchronization:
 *
 * The Service Directory is a very special process in the Legato framework.  It must be started
 * before every other process, except for the Supervisor itself.  Furthermore, other processes
 * must not start before the Service Directory has opened its named IPC sockets, so that those
 * other processes don't fail because they can't find the Service Directory.  So, after the
 * Supervisor starts the Service Directory, it waits for the Service Directory to signal that
 * it is ready.  This is done as follows:
 *
 * -# Before the Supervisor starts the Service Directory, it creates a pipe and moves one end
 *    of that pipe to fd 0 (stdin).
 * -# After forking, the Supervisor's child process closes the Supervisor's end of that pipe
 *    and leaves the fd 0 end open before execing the Service Directory.
 * -# The Supervisor (parent) process closes its copy of the child's end of the pipe and waits
 *    for the child to close its copy of its end of the pipe.
 * -# After the Service Directory has initialized itself and opened its IPC sockets (when it is
 *    ready to talk to service clients and servers), the Service Directory closes fd 0.
 */
//--------------------------------------------------------------------------------------------------
/**
 * @section sd_future               Future Enhancements
 *
 * @todo Add Access Control capabilities when we have the Configuration Data API and a well-defined
 *       configuration data schema for service access control lists.
 *
 * <hr/>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "serviceDirectoryProtocol.h"
#include "../unixSocket.h"
#include "../fileDescriptor.h"

// =======================================
//  PRIVATE DATA
// =======================================

//--------------------------------------------------------------------------------------------------
/// The number of sessions we expect.  This is used to size the Session List hashmap.
/// If too low, Service Directory performance may suffer.  If too high, some memory will be wasted.
//--------------------------------------------------------------------------------------------------
#define NUM_EXPECTED_SESSIONS 200


//--------------------------------------------------------------------------------------------------
/// The maximum number of backlogged connection requests that will be queued up for either the
/// Client Socket or the Server Socket.  If the Service Directory gets this far behind in accepting
/// connections, then the next client or server that attempts to connect will get a failure
/// indication from the OS.
//--------------------------------------------------------------------------------------------------
#define MAX_CONNECT_REQUEST_BACKLOG 100


/// Forward declaration of the ServicePtr_t.  Needed because of bi-directional linkage between
/// Service objects and Connection objects (they point to each other).
typedef struct Service* ServicePtr_t;


//--------------------------------------------------------------------------------------------------
/**
 * The Service List, in which all Service objects are kept.
 *
 * @note For performance reasons, this is actually a hashmap.  The Key is the combination of
 *       the service's protocol identity and service instance name.
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t ServiceListRef;


//--------------------------------------------------------------------------------------------------
/**
 * Represents a connection to a client process.  Objects of this type are allocated from the
 * Client Connection Pool.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t           link;           ///< Used to link onto a Service's waiting clients list.
    int                     fd;             ///< Fd of the connection socket.
    le_event_FdMonitorRef_t fdMonitorRef;   ///< FD Monitor object monitoring this connection.
    ServicePtr_t            servicePtr;     ///< Pointer to the service
    struct ucred            credentials;    ///< Credentials of client process.
}
ClientConnection_t;


//--------------------------------------------------------------------------------------------------
/**
 * Represents a connection to a server process.  Objects of this type are allocated from the
 * Server Connection Pool.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int                     fd;             ///< Fd of the connection socket.
    le_event_FdMonitorRef_t fdMonitorRef;   ///< FD Monitor object monitoring this connection.
    ServicePtr_t            servicePtr;     ///< Pointer to the service
    struct ucred            credentials;    ///< Credentials of server process.
}
ServerConnection_t;


//--------------------------------------------------------------------------------------------------
/**
 * Represents a service.  Objects of this type are allocated from the Service Pool and are kept
 * on the Service List.
 *
 * @note These objects are reference counted.  Each client in the list of waiting clients will hold
 *       a reference, and the server will hold a reference.
 */
//--------------------------------------------------------------------------------------------------
typedef struct Service
{
    svcdir_ServiceId_t  id;                 ///< Details that uniquely identify this service.
    ServerConnection_t* serverConnectionPtr;///< Ptr to server connection (NULL if no server yet).
    le_dls_List_t       waitingClientsList; ///< List of Client Connectns waiting for this service.
}
Service_t;


//--------------------------------------------------------------------------------------------------
/// Pool from which Service objects are allocated.
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ServicePoolRef;


//--------------------------------------------------------------------------------------------------
/// Pool from which Client Connection objects are allocated.
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ClientConnectionPoolRef;


//--------------------------------------------------------------------------------------------------
/// Pool from which Server Connection objects are allocated.
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ServerConnectionPoolRef;


//--------------------------------------------------------------------------------------------------
/// File descriptor for the Client Socket (which IPC clients connect to).
//--------------------------------------------------------------------------------------------------
static int ClientSocketFd;


//--------------------------------------------------------------------------------------------------
/// File descriptor for the Server Socket (which IPC servers connect to).
//--------------------------------------------------------------------------------------------------
static int ServerSocketFd;


//--------------------------------------------------------------------------------------------------
/// FD Monitor for the Client Socket.  Used to detect when clients connect to the Client Socket.
//--------------------------------------------------------------------------------------------------
static le_event_FdMonitorRef_t ClientSocketMonitorRef;


//--------------------------------------------------------------------------------------------------
/// FD Monitor for the Server Socket.  Used to detect when servers connect to the Server Socket.
//--------------------------------------------------------------------------------------------------
static le_event_FdMonitorRef_t ServerSocketMonitorRef;


// =======================================
//  FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Generates a hash value for a given Session object.
 *
 * This is needed by the Session List hashmap.
 *
 * @return The hash value used to generate an index into the hash map.
 */
//--------------------------------------------------------------------------------------------------
static size_t SessionHashFunc
(
    const void *keyToHashPtr
)
//--------------------------------------------------------------------------------------------------
{
    const svcdir_ServiceId_t* serviceIdPtr = keyToHashPtr;

    return (  le_hashmap_HashString(serviceIdPtr->protocolId)
            + le_hashmap_HashString(serviceIdPtr->serviceName) );
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks for equality of two Session Ids in the Session List.
 */
//--------------------------------------------------------------------------------------------------
static bool SessionEqualityFunc
(
    const void *firstKeyPtr,
    const void *secondKeyPtr
)
//--------------------------------------------------------------------------------------------------
{
    const svcdir_ServiceId_t* firstIdPtr = firstKeyPtr;
    const svcdir_ServiceId_t* secondIdPtr = secondKeyPtr;

    return (   le_hashmap_EqualsString(firstIdPtr->protocolId, secondIdPtr->protocolId)
            && le_hashmap_EqualsString(firstIdPtr->serviceName, secondIdPtr->serviceName) );
}


//--------------------------------------------------------------------------------------------------
/**
 * Destructor function that runs when a Service object's reference count reaches zero and
 * the Service object is about to be released back into the Service Pool.
 */
//--------------------------------------------------------------------------------------------------
static void ServiceDestructor
(
    void* objPtr
)
//--------------------------------------------------------------------------------------------------
{
    Service_t* servicePtr = objPtr;

    // Remove the Service object from the Service List.
    le_hashmap_Remove(ServiceListRef, &servicePtr->id);
}


//--------------------------------------------------------------------------------------------------
/**
 * Closes a connection with a client process.
 */
//--------------------------------------------------------------------------------------------------
static void CloseClientConnection
(
    ClientConnection_t* connectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    // If the connection is associated with a service, then it must be on that service's list
    // of waiting clients.
    if (connectionPtr->servicePtr != NULL)
    {
        // Remove the connection from the service's list of waiting connections.
        le_dls_Remove(&connectionPtr->servicePtr->waitingClientsList, &connectionPtr->link);

        // Release the client's hold on the Service object.
        le_mem_Release(connectionPtr->servicePtr);
        connectionPtr->servicePtr = NULL;
    }

    // Delete the File Descriptor Monitor object.
    le_event_DeleteFdMonitor(connectionPtr->fdMonitorRef);

    // Close the socket.
    fd_Close(connectionPtr->fd);

    // Release the Client Connection object.
    le_mem_Release(connectionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Closes a connection with a server process.
 */
//--------------------------------------------------------------------------------------------------
static void CloseServerConnection
(
    ServerConnection_t* connectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    // If the connection is associated with a service, then it must be the server for that service.
    if (connectionPtr->servicePtr != NULL)
    {
        LE_ASSERT(connectionPtr->servicePtr->serverConnectionPtr == connectionPtr);

        // Detach the server from the session.
        connectionPtr->servicePtr->serverConnectionPtr = NULL;
        LE_DEBUG("Server (pid: %d) disconnected from service (%s:%s).",
                 connectionPtr->credentials.pid,
                 connectionPtr->servicePtr->id.protocolId,
                 connectionPtr->servicePtr->id.serviceName);

        // Release the server's hold on the Service object.
        le_mem_Release(connectionPtr->servicePtr);
        connectionPtr->servicePtr = NULL;
    }

    // Delete the File Descriptor Monitor object.
    le_event_DeleteFdMonitor(connectionPtr->fdMonitorRef);

    // Close the socket.
    fd_Close(connectionPtr->fd);

    // Release the Server Connection object.
    le_mem_Release(connectionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Receive a message containing a Service ID from a connected socket.
 *
 * @return
 * - LE_OK if successful.
 * - LE_WOULD_BLOCK if there's nothing to be received.
 * - LE_CLOSED if the connection closed.
 * - LE_FAULT if failed.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReceiveServiceId
(
    int                 fd,             ///< [in] File descriptor to receive the ID from.
    svcdir_ServiceId_t* serviceIdPtr    ///< [out] Ptr to where the ID will be stored if successful.
)
//--------------------------------------------------------------------------------------------------
{
    size_t byteCount = sizeof(*serviceIdPtr);

    le_result_t result = unixSocket_ReceiveDataMsg(fd, serviceIdPtr, &byteCount);

    if (result == LE_FAULT)
    {
        LE_ERROR("Failed to receive service ID. Errno = %d (%m).", errno);
        return LE_FAULT;
    }
    else if ( (result == LE_OK) && (byteCount != sizeof(*serviceIdPtr)) )
    {
        LE_ERROR("Incorrect number of bytes received (%zd received, %zu expected).",
                 byteCount,
                 sizeof(*serviceIdPtr));
        return LE_FAULT;
    }
    else
    {
        return result;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a Service object.
 */
//--------------------------------------------------------------------------------------------------
static Service_t* CreateService
(
    const svcdir_ServiceId_t* serviceIdPtr  ///< [in] Pointer to the service identity.
)
//--------------------------------------------------------------------------------------------------
{
    Service_t* servicePtr = le_mem_ForceAlloc(ServicePoolRef);

    servicePtr->waitingClientsList = LE_DLS_LIST_INIT;
    servicePtr->serverConnectionPtr = NULL;

    // Store the service identity inside the object
    memcpy(&servicePtr->id, serviceIdPtr, sizeof(servicePtr->id));

    // Add the object to the Service List.
    le_hashmap_Put(ServiceListRef, &servicePtr->id, servicePtr);

    return servicePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a Service object, with one waiting client on its list of waiting clients.
 */
//--------------------------------------------------------------------------------------------------
static Service_t* CreateServiceWithWaitingClient
(
    const svcdir_ServiceId_t* serviceIdPtr,       ///< [in] Pointer to the service identity.
    ClientConnection_t* clientConnectionPtr ///< [in] Pointer to client's connection.
)
//--------------------------------------------------------------------------------------------------
{
    Service_t* servicePtr = CreateService(serviceIdPtr);

    LE_INFO("Client (pid %d) is waiting for service (%s:%s).",
            clientConnectionPtr->credentials.pid,
            serviceIdPtr->protocolId,
            serviceIdPtr->serviceName);

    // Add the client connection to the list of waiting clients.
    le_dls_Queue(&servicePtr->waitingClientsList, &clientConnectionPtr->link);

    // Note:    The Client Connection object takes over ownership of the first reference to the
    //          Service object.
    clientConnectionPtr->servicePtr = servicePtr;

    return servicePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a Service object with a Server Connection.
 */
//--------------------------------------------------------------------------------------------------
static Service_t* CreateServiceWithServer
(
    const svcdir_ServiceId_t* serviceIdPtr, ///< [in] Pointer to the service identity.
    ServerConnection_t* serverConnectionPtr ///< [in] Pointer to server's connection.
)
//--------------------------------------------------------------------------------------------------
{
    Service_t* servicePtr = CreateService(serviceIdPtr);

    // Store the pointer to the server connection inside the Service object.
    // Note:    The Server Connection object takes over ownership of the first reference to the
    //          Service object.
    servicePtr->serverConnectionPtr = serverConnectionPtr;
    serverConnectionPtr->servicePtr = servicePtr;

    LE_INFO("Server (pid: %d) now serving service (%s:%s).",
            serverConnectionPtr->credentials.pid,
            servicePtr->id.protocolId,
            servicePtr->id.serviceName);

    return servicePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a client connection to the list of clients waiting for a given service to become available.
 *
 * @note This client will hold a reference to the Service object.
 */
//--------------------------------------------------------------------------------------------------
static void AddClientToWaitList
(
    Service_t*          servicePtr,         ///< [in] Pointer to the Service object.
    ClientConnection_t* clientConnectionPtr ///< [in] Pointer to the client connection.
)
//--------------------------------------------------------------------------------------------------
{
    // Add the client to the list of waiting clients.
    le_dls_Queue(&servicePtr->waitingClientsList, &clientConnectionPtr->link);

    clientConnectionPtr->servicePtr = servicePtr;
    le_mem_AddRef(servicePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Dispatch a client connection to a server connection.
 *
 * @return
 * - LE_OK if successful.
 *
 * @note If successful, this will delete the Client Connection object.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DispatchClientToServer
(
    ServerConnection_t* serverConnectionPtr, ///< [in] Server connection to dispatch client to.
    ClientConnection_t* clientConnectionPtr  ///< [in] Client connection to be dispatched.
)
//--------------------------------------------------------------------------------------------------
{
    // Send the one client connection fd to the server.
    le_result_t result = unixSocket_SendMsg(serverConnectionPtr->fd,
                                            NULL,   // dataPtr
                                            0,      // dataSize
                                            clientConnectionPtr->fd, // fdToSend
                                            false); // sendCredentials

    if (result == LE_OK)
    {
        LE_INFO("Client (pid %d) connected to server %d for service (%s:%s).",
                clientConnectionPtr->credentials.pid,
                serverConnectionPtr->credentials.pid,
                serverConnectionPtr->servicePtr->id.protocolId,
                serverConnectionPtr->servicePtr->id.serviceName);

        // Close the client connection (it has been handed off to the server now).
        CloseClientConnection(clientConnectionPtr);

        return LE_OK;
    }

    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Dispatch all clients waiting for a given service to that service's server.
 */
//--------------------------------------------------------------------------------------------------
static void DispatchWaitingClients
(
    Service_t* servicePtr   ///< [in] Pointer to a Service object.
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* linkPtr;

    // While there are still clients on the list of waiting clients for this service,
    // get a pointer to the first one and dispatch it to the service's server.
    while ((linkPtr = le_dls_Peek(&servicePtr->waitingClientsList)) != NULL)
    {
        ClientConnection_t* clientConnectionPtr = CONTAINER_OF(linkPtr, ClientConnection_t, link);

        // Note: if successful, DispatchClientToServer() will close the client connection,
        //       which will remove the client from the list of waiting clients.
        //       Otherwise, the client will be left on the waiting list.
        if (DispatchClientToServer(servicePtr->serverConnectionPtr, clientConnectionPtr) != LE_OK)
        {
            // The server seems to have failed.
            // Leave the client on the waiting list, close the server connection, and
            // abort the dispatching process.  The rest of the clients will have to wait
            // for another server.
            CloseServerConnection(servicePtr->serverConnectionPtr);
            break;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a client connection to a service.
 */
//--------------------------------------------------------------------------------------------------
static void AddClientToService
(
    Service_t*          servicePtr,         ///< [in] Pointer to the Service object.
    ClientConnection_t* clientConnectionPtr ///< [in] Pointer to the client connection.
)
//--------------------------------------------------------------------------------------------------
{
    // Add the client to the list of waiting clients.
    AddClientToWaitList(servicePtr, clientConnectionPtr);

    // If the service has a server,
    if (servicePtr->serverConnectionPtr != NULL)
    {
        DispatchWaitingClients(servicePtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Attach a server connection to a Service object.
 *
 * @note This will dispatch waiting clients to the service's new server.
 */
//--------------------------------------------------------------------------------------------------
static void AttachServerToService
(
    Service_t*          servicePtr,         ///< [in] Pointer to the Service object.
    ServerConnection_t* serverConnectionPtr ///< [in] Pointer to the Server Connection object.
)
//--------------------------------------------------------------------------------------------------
{
    servicePtr->serverConnectionPtr = serverConnectionPtr;

    serverConnectionPtr->servicePtr = servicePtr;
    le_mem_AddRef(servicePtr);  // Server holds a reference to the Service.

    LE_INFO("Server (pid: %d) now serving service (%s:%s).",
             serverConnectionPtr->credentials.pid,
             serverConnectionPtr->servicePtr->id.protocolId,
             serverConnectionPtr->servicePtr->id.serviceName);

    // Dispatch all the waiting clients to this server.
    DispatchWaitingClients(servicePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function that gets called when a connection to a client experiences an error.
 *
 * @note The Context Pointer is a pointer to a Client Connection object.
 */
//--------------------------------------------------------------------------------------------------
static void ClientErrorHandler
(
    int fd  ///< [in] File descriptor for the connection.
)
//--------------------------------------------------------------------------------------------------
{
    ClientConnection_t* connectionPtr = le_event_GetContextPtr();

    LE_ASSERT(connectionPtr != NULL);
    LE_INFO("Client (pid %u) experienced error. Closing.", connectionPtr->credentials.pid);

    CloseClientConnection(connectionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function that gets called when the client closes its end of a connection.
 *
 * @note The Context Pointer is a pointer to a Client Connection object.
 */
//--------------------------------------------------------------------------------------------------
static void ClientReadHangUpHandler
(
    int fd  ///< [in] File descriptor for the connection.
)
//--------------------------------------------------------------------------------------------------
{
    ClientConnection_t* connectionPtr = le_event_GetContextPtr();

    LE_ASSERT(connectionPtr != NULL);
    LE_INFO("Client (pid %u) closed their connection.", connectionPtr->credentials.pid);

    CloseClientConnection(connectionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Processes an "Open" request received from a client.
 **/
//--------------------------------------------------------------------------------------------------
static void ProcessOpenRequestFromClient
(
    ClientConnection_t* connectionPtr,      ///< [IN] Pointer to the Client Connection object.
    const svcdir_ServiceId_t* serviceIdPtr  ///< [IN] Ptr to the service ID the client is opening.
)
//--------------------------------------------------------------------------------------------------
{
    // Look up the Service in the Service List.
    Service_t* servicePtr = le_hashmap_Get(ServiceListRef, serviceIdPtr);

    if (servicePtr == NULL)
    {
        // Create Service and add client connection to list of waiting clients.
        servicePtr = CreateServiceWithWaitingClient(serviceIdPtr, connectionPtr);
    }
    else
    {
        // Service object already exists.  Add this client to it.
        AddClientToService(servicePtr, connectionPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function that gets called when the client sends us data.
 *
 * @note The Context Pointer is a pointer to a Client Connection object.
 */
//--------------------------------------------------------------------------------------------------
static void ClientReadHandler
(
    int fd  ///< [in] File descriptor for the connection.
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;
    ClientConnection_t* connectionPtr = le_event_GetContextPtr();

    LE_ASSERT(connectionPtr != NULL);

    // Receive the service identity from the client.
    svcdir_ServiceId_t serviceId;
    result = ReceiveServiceId(fd, &serviceId);

    if (result == LE_OK)
    {
        // The client should only send us the service identification details.  After we get that,
        // we add the connection to the Service's list of waiting clients, so if this connection
        // is already on a list of waiting clients, we shouldn't be receiving data from the client.
        if (connectionPtr->servicePtr != NULL)
        {
            LE_ERROR("Client (pid %d) waiting for service '%s/%s' sent us something extra.",
                     connectionPtr->credentials.pid,
                     connectionPtr->servicePtr->id.protocolId,
                     connectionPtr->servicePtr->id.serviceName);

            // Kill misbehaving client.
            CloseClientConnection(connectionPtr);
        }
        else
        {
            ProcessOpenRequestFromClient(connectionPtr, &serviceId);
        }
    }
    // If the connection has closed or there is simply nothing left to be received
    // from the socket,
    else if ((result == LE_CLOSED) || (result == LE_WOULD_BLOCK))
    {
        // We are done.
        // NOTE: If the connection closed, our hang-up handler will be called.
    }
    // If an error occurred on the receive,
    else
    {
        LE_ERROR("Failed to receive service ID from client (pid %u).",
                 connectionPtr->credentials.pid);

        // Drop the Client connection to trigger a recovery action by the client (or the Supervisor,
        // if the client dies).
        CloseClientConnection(connectionPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function that gets called when a client connects to the Client socket.
 */
//--------------------------------------------------------------------------------------------------
static void ClientConnectHandler
(
    int fd  ///< [in] File descriptor of the socket that has received a connection request.
)
//--------------------------------------------------------------------------------------------------
{
    // Accept the connection, setting the connection to be non-blocking.
    fd = accept4(fd, NULL, NULL, SOCK_NONBLOCK);

    if (fd < 0)
    {
        LE_CRIT("Failed to accept client connection. Errno %d (%m).", errno);
    }
    else
    {
        socklen_t credentialsSize = sizeof(struct ucred);

        // Allocate a Connection object for this connection.
        ClientConnection_t* connectionPtr = le_mem_ForceAlloc(ClientConnectionPoolRef);
        connectionPtr->link = LE_DLS_LINK_INIT;
        connectionPtr->fd = fd;
        connectionPtr->servicePtr = NULL;   // No service for this connection, yet.

        // Get the remote process's credentials.
        if (0 != getsockopt(fd,
                            SOL_SOCKET,
                            SO_PEERCRED,
                            &connectionPtr->credentials,
                            &credentialsSize) )
        {
            LE_ERROR("Failed to obtain credentials from client.  Errno = %d (%m)", errno);
            le_mem_Release(connectionPtr);
            fd_Close(fd);
        }
        else
        {
            char fdMonName[32];  // Buffer for holding the FD Monitor's name string.

            LE_DEBUG("Client connected:  pid = %d;  uid = %d;  gid = %d.",
                     connectionPtr->credentials.pid,
                     connectionPtr->credentials.uid,
                     connectionPtr->credentials.gid);

            // Set up a File Descriptor Monitor for this new connection, and monitor for hang-up,
            // error, and data arriving.  Normally, the next step would be to receive the
            // session details for this fd, but its possible that the client process may die before
            // that happens.
            snprintf(fdMonName, sizeof(fdMonName), "Client:%u", connectionPtr->credentials.pid);
            connectionPtr->fdMonitorRef = le_event_CreateFdMonitor(fdMonName, fd);
            le_event_FdHandlerRef_t handlerRef;
            handlerRef = le_event_SetFdHandler(connectionPtr->fdMonitorRef,
                                               LE_EVENT_FD_ERROR,
                                               ClientErrorHandler);
            le_event_SetFdHandlerContextPtr(handlerRef, connectionPtr);
            handlerRef = le_event_SetFdHandler(connectionPtr->fdMonitorRef,
                                               LE_EVENT_FD_READABLE,
                                               ClientReadHandler);
            le_event_SetFdHandlerContextPtr(handlerRef, connectionPtr);
            handlerRef = le_event_SetFdHandler(connectionPtr->fdMonitorRef,
                                               LE_EVENT_FD_READ_HANG_UP,
                                               ClientReadHangUpHandler);
            le_event_SetFdHandlerContextPtr(handlerRef, connectionPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function that gets called when a connection to a server experiences an error.
 *
 * @note The Context Pointer is a pointer to a Server Connection object.
 */
//--------------------------------------------------------------------------------------------------
static void ServerErrorHandler
(
    int fd  ///< [in] File descriptor for the connection.
)
//--------------------------------------------------------------------------------------------------
{
    ServerConnection_t* connectionPtr = le_event_GetContextPtr();

    LE_ASSERT(connectionPtr != NULL);
    LE_INFO("Server (pid %u) experienced error. Closing.", connectionPtr->credentials.pid);

    CloseServerConnection(connectionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function that gets called when the server closes its end of a connection.
 *
 * @note The Context Pointer is a pointer to a Server Connection object.
 */
//--------------------------------------------------------------------------------------------------
static void ServerReadHangUpHandler
(
    int fd  ///< [in] File descriptor for the connection.
)
//--------------------------------------------------------------------------------------------------
{
    ServerConnection_t* connectionPtr = le_event_GetContextPtr();

    LE_ASSERT(connectionPtr != NULL);
    LE_INFO("Server (pid %u) closed their connection.", connectionPtr->credentials.pid);

    CloseServerConnection(connectionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function that gets called when the server sends us data.
 *
 * @note The Context Pointer is a pointer to a Server Connection object.
 */
//--------------------------------------------------------------------------------------------------
static void ServerReadHandler
(
    int fd  ///< [in] File descriptor for the connection.
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;
    ServerConnection_t* connectionPtr = le_event_GetContextPtr();

    LE_ASSERT(connectionPtr != NULL);

    // Receive the service identity from the server.
    svcdir_ServiceId_t serviceId;
    result = ReceiveServiceId(fd, &serviceId);

    if (result == LE_CLOSED)
    {
        // Connection has closed.  Our Hang-Up handler will get called, so we don't have to
        // do anything here.
    }
    // The server should only send us the service identification details.  After we get that,
    // we add the connection as the Service's server connection, so if this connection
    // is already associated with a Service object, we shouldn't be receiving data from this server.
    else if (connectionPtr->servicePtr != NULL)
    {
        LE_ERROR("Server of service '%s/%s' sent us something extra.",
                 connectionPtr->servicePtr->id.protocolId,
                 connectionPtr->servicePtr->id.serviceName);

        // Kill misbehaving server.
        CloseServerConnection(connectionPtr);
    }
    else if (result != LE_OK)
    {
        LE_ERROR("Failed to receive service ID from server (pid %u).",
                 connectionPtr->credentials.pid);
        CloseServerConnection(connectionPtr);
    }
    else
    {
        // Look up the Service in the Service List.
        Service_t* servicePtr = le_hashmap_Get(ServiceListRef, &serviceId);

        if (servicePtr == NULL)
        {
            // Create Service and add server connection to it.
            servicePtr = CreateServiceWithServer(&serviceId, connectionPtr);
        }
        else if (servicePtr->serverConnectionPtr != NULL)
        {
            // There is already a server for this service, so reject this one.
            LE_ERROR("Service '%s/%s' already has a server (pid %u). Closing connection to pid %u.",
                     servicePtr->id.protocolId,
                     servicePtr->id.serviceName,
                     servicePtr->serverConnectionPtr->credentials.pid,
                     connectionPtr->credentials.pid);
            CloseServerConnection(connectionPtr);
        }
        else
        {
            // There are clients waiting for this service to become available...
            // Attach this server connection to the Service object.
            AttachServerToService(servicePtr, connectionPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function that gets called when a server connects to the Server socket.
 */
//--------------------------------------------------------------------------------------------------
static void ServerConnectHandler
(
    int fd  ///< [in] File descriptor of the socket that has received a connection request.
)
//--------------------------------------------------------------------------------------------------
{
    // Accept the connection, setting the connection to be non-blocking.
    fd = accept4(fd, NULL, NULL, SOCK_NONBLOCK);

    if (fd < 0)
    {
        LE_CRIT("Failed to accept server connection. Errno %d (%m).", errno);
    }
    else
    {
        socklen_t credentialsSize = sizeof(struct ucred);

        // Allocate a Connection object for this connection.
        ServerConnection_t* connectionPtr = le_mem_ForceAlloc(ServerConnectionPoolRef);
        connectionPtr->fd = fd;
        connectionPtr->servicePtr = NULL;   // No service for this connection, yet.

        // Get the remote process's credentials.
        if (0 != getsockopt(fd,
                            SOL_SOCKET,
                            SO_PEERCRED,
                            &connectionPtr->credentials,
                            &credentialsSize) )
        {
            LE_ERROR("Failed to obtain credentials from server.  Errno = %d (%m)", errno);
            le_mem_Release(connectionPtr);
            fd_Close(fd);
        }
        else
        {
            char fdMonName[32];  // Buffer for holding the FD Monitor's name string.

            LE_DEBUG("Server connected:  pid = %d;  uid = %d;  gid = %d.",
                     connectionPtr->credentials.pid,
                     connectionPtr->credentials.uid,
                     connectionPtr->credentials.gid);

            // Set up a File Descriptor Monitor for this new connection, and monitor for hang-up,
            // error, and data arriving.  Normally, the next step would be to receive the
            // session details for this fd, but its possible that the server process may die before
            // that happens.
            snprintf(fdMonName, sizeof(fdMonName), "Server:%u", connectionPtr->credentials.pid);
            connectionPtr->fdMonitorRef = le_event_CreateFdMonitor(fdMonName, fd);
            le_event_FdHandlerRef_t handlerRef;
            handlerRef = le_event_SetFdHandler(connectionPtr->fdMonitorRef,
                                               LE_EVENT_FD_ERROR,
                                               ServerErrorHandler);
            le_event_SetFdHandlerContextPtr(handlerRef, connectionPtr);
            handlerRef = le_event_SetFdHandler(connectionPtr->fdMonitorRef,
                                               LE_EVENT_FD_READABLE,
                                               ServerReadHandler);
            le_event_SetFdHandlerContextPtr(handlerRef, connectionPtr);
            handlerRef = le_event_SetFdHandler(connectionPtr->fdMonitorRef,
                                               LE_EVENT_FD_READ_HANG_UP,
                                               ServerReadHangUpHandler);
            le_event_SetFdHandlerContextPtr(handlerRef, connectionPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Opens a named SOCK_SEQPACKET unix domain socket, using a given file system path as the address.
 *
 * If something already exists in the file system at the path given, this function will try to
 * unlink it to make way for the socket it is trying to create.
 *
 * @return File descriptor of the socket.
 *
 * @note Logs a message and terminates the process on failure.
 */
//--------------------------------------------------------------------------------------------------
static int OpenSocket
(
    const char* socketPathStr   ///< [in] File system path.
)
//--------------------------------------------------------------------------------------------------
{
    int fd = unixSocket_CreateSeqPacketNamed(socketPathStr);

    if (fd == LE_DUPLICATE)
    {
        if (unlink(socketPathStr) != 0)
        {
            LE_FATAL("Couldn't unlink '%s' to make way for new socket. Errno = %d (%m).",
                     socketPathStr,
                     errno);
        }
        fd = unixSocket_CreateSeqPacketNamed(socketPathStr);
    }

    if (fd < 0)
    {
        LE_FATAL("Failed to open socket '%s'. Result = %d (%s).",
                 socketPathStr,
                 fd,
                 LE_RESULT_TXT(fd));
    }

    return fd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialization function.  This is called at start-up.  When it returns, the process's main
 * event loop will run.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Get references to the Service Pool, Client Connection Pool and Server Connection Pool.
    ServicePoolRef = le_mem_CreatePool("Service", sizeof(Service_t));
    ClientConnectionPoolRef = le_mem_CreatePool("Client Connection", sizeof(ClientConnection_t));
    ServerConnectionPoolRef = le_mem_CreatePool("Server Connection", sizeof(ServerConnection_t));

    /// Expand the pools to their expected maximum sizes.
    /// @todo Make this configurable.
    le_mem_ExpandPool(ServicePoolRef, 30);
    le_mem_ExpandPool(ClientConnectionPoolRef, 100);
    le_mem_ExpandPool(ServerConnectionPoolRef, 30);

    // Register the Service objects' destructor function.
    le_mem_SetDestructor(ServicePoolRef, ServiceDestructor);

    // Create the Service List hashmap.
    ServiceListRef = le_hashmap_Create("ServiceList",
                                       NUM_EXPECTED_SESSIONS,
                                       SessionHashFunc,
                                       SessionEqualityFunc);

    // Create the Legato runtime directory if it doesn't already exists.
    LE_ASSERT(le_dir_Make(LE_RUNTIME_DIR, S_IRWXU | S_IXOTH) != LE_FAULT);

    /// @todo Check permissions of directory containing client and server socket addresses.
    ///       Only the current user or root should be allowed write access.
    ///       Warn if it is found to be otherwise.

    // Open the sockets.
    ClientSocketFd = OpenSocket(LE_SVCDIR_CLIENT_SOCKET_NAME);
    ServerSocketFd = OpenSocket(LE_SVCDIR_SERVER_SOCKET_NAME);

    // Start listening for connection attempts.
    ClientSocketMonitorRef = le_event_CreateFdMonitor("Client Socket", ClientSocketFd);
    le_event_SetFdHandler(ClientSocketMonitorRef, LE_EVENT_FD_READABLE, ClientConnectHandler);
    ServerSocketMonitorRef = le_event_CreateFdMonitor("Server Socket", ServerSocketFd);
    le_event_SetFdHandler(ServerSocketMonitorRef, LE_EVENT_FD_READABLE, ServerConnectHandler);
    if (listen(ClientSocketFd, MAX_CONNECT_REQUEST_BACKLOG) != 0)
    {
        LE_FATAL("Client socket listen() call failed with errno %d (%m).", errno);
    }
    if (listen(ServerSocketFd, MAX_CONNECT_REQUEST_BACKLOG) != 0)
    {
        LE_FATAL("Server socket listen() call failed with errno %d (%m).", errno);
    }

    // Close the fd that we inherited from the Supervisor.  This will let the Supervisor know that
    // we are initialized.  Then re-open it to /dev/null so that it cannot be reused later.
    FILE* filePtr;
    do
    {
        filePtr = freopen("/dev/null", "r", stdin);
    }
    while ( (filePtr == NULL) && (errno == EINTR) );

    LE_FATAL_IF(filePtr == NULL, "Failed to redirect standard in to /dev/null.  %m.");


    LE_INFO("Service Directory is ready.");
}
