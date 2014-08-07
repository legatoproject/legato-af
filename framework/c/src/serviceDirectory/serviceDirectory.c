//--------------------------------------------------------------------------------------------------
/** @file serviceDirectory.c
 *
 * @ref sd_intro <br>
 * @ref sd_accessControl <br>
 * @ref sd_toolService <br>
 * @ref sd_data <br>
 * @ref sd_theoryOfOperation <br>
 * @ref sd_deathDetection <br>
 * @ref sd_threading <br>
 * @ref sd_startUpSync <br>
 * @ref sd_DesignNotes <br>
 *
 * @section sd_intro        Introduction
 *
 * Implementation of the Service Directory daemon.  This daemon keeps track of what messaging
 * services exist in the system, what clients are currently waiting for services that don't yet
 * exist, and what pre-configured bindings exist between clients and services.
 *
 * The Service Directory implements @ref serviceDirectoryProtocol.h.
 *
 *
 * @section sd_accessControl        Binding and Access Control
 *
 * The Service Directory is a key component in the implementation of security within the Legato
 * framework.  No two sandboxed applications can access each others files, sockets, shared memory,
 * etc. directly until they have connected to each other through the Service Directory.
 *
 * The Service Directory grants or prohibits IPC connections between servers and clients based
 * on the configuration of IPC "bindings" (see @ref sd_accessControl).  Bindings can also be
 * used to connect (bind) a client "import" to a server "export" that has a different service
 * instance name.
 *
 * The Service Directory essentially creates namespaces for IPC services based on user IDs.
 * Clients in one namespace cannot see the services offered by services in other namespaces
 * unless there is a binding explicitly configured between them.  Because each application has
 * its own unique user ID, this ensures that each application has its own IPC namespace, and
 * can only access specific IPC services from other applications that they have been explicitly
 * granted access to.
 *
 * It is not necessary to explicitly declare bindings between clients and servers in the same
 * namespace (running under the same effective user ID).  If a client attempts to open a service
 * and the Service Directory doesn't find a binding for that client interface, it will assume that
 * a server within the same namespace will eventually advertise that service (if it hasn't already).
 *
 *
 * @section sd_toolService          "sdir" Tool
 *
 * The @c sdir command-line tool is used to:
 * - configure bindings and
 * - view the internal workings of the Service Directory at run-time for diagnostic purposes.
 *
 * The @c sdir tool interfaces with the Service Directory using the IPC services of the
 * Service Directory.  From the point-of-view of the @c sdir tool, it is a regular Legato IPC
 * client connecting to a regular IPC server.
 *
 *
 * @section sd_data                 Data Structures
 *
 * The Service Directory's internal (RAM) data structures look like this:
 *
@verbatim

                    +----------------------------------+<------------------------------------------+
                    | [1..1]                           ^                                           |
                    v                                  |                                           |
User List ------> User --+---> Name                    |                                           |
            [0..n]  ^    |                    [0..n]   |                                           |
                    |    +---> Binding List ------> Binding -----> Service Name                    |
                    |    |                             |[0..n]                                     |
                    |    |                             |                                           |
                    |    |                             |                                           |
                    |    |                  [0..n]     v[1..1]                                     |
                    |    +---> Service List ------> Service --------+---> Service Name             |
                    |                                |   ^[1..1]    |                              |
                    +--------------------------------+   |          +---> Protocol ID    +-------->+
                                                         |          |                    |         ^
                                                         |          +---> Server Connection ----+  |
                                                         |          |                           |  |
                                                         |          +---> Waiting Clients List  |  |
                                                         |                       |              |  |
                                                         |                       |              |  |
                                                         |                       v [0..n]       |  |
                                                         +<-------------- Client Connection ----|--+
                                                         ^                                      |
                                                         |                                      |
                                                         +--------------------------------------+
@endverbatim
 *
 * The User object represents a single user account.  It has a unique ID which is used as the key
 * to find it in the User List.  Each User also has a list of Binding objects, which represent
 * bindings between a service name in the client and an actual service.  Each User also has a list
 * of services that it offers.
 *
 * Connection objects are used to keep track of the details of socket connections (e.g., the
 * file descriptor, File Descriptor Monitor object, etc.).  Server Connections keep track of
 * connections to servers.  Client Connections keep track of connections to clients.
 *
 * Binding objects are created for bindings that appear in the configuration data.
 *
 * Client Connection objects and Server Connection objects are created when clients and servers
 * connect to the Service Directory.
 *
 * A Service object can exist for any of the following reasons:
 * - A server is currently connected to the Service Directory and offering that service.
 *      In this case, a Server Connection is attached to the Service object.
 * - One or more clients are connected to the Service Directory and waiting for that service to
 *      be offered.  In this case, a list of Client Connections is attached to the Service object.
 * - A binding to this Service exists in the system configuration.  In this case there is a
 *      Binding object somewhere that points to this Service.
 *
 * Each Binding object and Connection object (Server and Client) holds a reference count on
 * a Service object.  Service objects will be deleted when all associated Binding objects
 * and Connection objects are deleted.
 *
 * Each Binding object, Service object, and Connection object holds a reference count on a
 * User object.  A User object will be deleted when all associated Binding objects, Service objects,
 * and Connection objects are deleted.
 *
 * Client Connection objects are deleted when the client disconnects or its connection is passed
 * to a server.
 *
 * Server Connection objects are deleted when the server disconnects.
 *
 * Binding objects are deleted when the binding is removed from the configuration.
 *
 *
 * @section sd_theoryOfOperation Theory of Operation
 *
 * When the Config Tree starts up, the Service Directory loads the binding configuration settings
 * and builds the above structure in RAM (without any Client Connections or Server Connections yet).
 *
 * When a client connects and makes a request to open a service, the client's UID is looked up in
 * the User List.  The client User's Binding List is searched for the service name requested by
 * the client.  If a matching Binding object is found, it will refer to the Service that the client
 * is bound to.  If there's no binding for that service name, then the client's own Service List
 * will be searched for a Service object with the same name as the requested service name.  If not
 * found, a Service object will be created on that User's Service List (in anticipation of that
 * User eventually offering a matching service) and the Client Connection will be queued on that
 * Service's Waiting Clients List.
 *
 * When a server connects and advertises a service, the server UID is looked-up in the User List.
 * The service name is then searched for in the Service List for that User.  If a Service object
 * is not found for that service name on that User, a new one is created and the Server Connection
 * is added to it.  If a Service object is found that already has a server connection, then the
 * new server connection is dropped.  If a Service object is found that doesn't have a Server
 * Connection already, the new server connection is added to that Service object, and if it has
 * waiting Client Connection objects, their connections are all passed to the new server.
 *
 * NOTE: It is outside the Service Directory's scope to terminate client IPC connections that
 * were established through bindings that have been changed.  The Service Directory does not
 * keep track of client-server connections after they have been established.
 *
 *
 * @section sd_deathDetection       Detection of Client or Server Death
 *
 * When a client or server process dies while it is connected to the Service Directory, the OS
 * will automatically close the the connection to that process.  The Service Directory will detect
 * this using an FD Monitor object (see @ref c_eventLoop) and update the data structures
 * accordingly.
 *
 *
 * @section sd_threading            Threading
 *
 * There is only one thread running in this process.
 *
 *
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
 *    ready to talk to service clients and servers), the Service Directory closes fd 0 and reopens
 *    it to "/dev/null".
 *
 * @section sd_DesignNotes          Design Notes
 *
 * We considered making the Service Directory a client of the Config Tree and having the
 * Service Directory register "handler" call-backs to notify it when binding configuration
 * changes.  While this complicates the start-up sequence considerably, the real problem is
 * that it creates a race condition:
 *
 * -# App Installer commits configuration changes to the Config Tree.
 * -# App Installer asks the Supervisor to start apps.
 * -# Supervisor starts apps.
 * -# Apps open IPC services.
 * -# Config Tree notifies Service Directory of configuration changes.
 * -# Service Directory loads new binding configuration.
 *
 * The race occurs because steps 2, 3 and 4 run in parallel with steps 5 and 6.
 *
 * So, instead, we created the "sdir load" tool and made the Supervisor run it before starting
 * any applications and made the installer run it after installing/removing any apps.
 *
 * <hr/>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013 - 2014. Use of this work is subject to license.
 */

#include "legato.h"
#include "serviceDirectoryProtocol.h"
#include "sdirToolProtocol.h"
#include "../unixSocket.h"
#include "../fileDescriptor.h"
#include "../limit.h"
#include "../user.h"

// =======================================
//  PRIVATE DATA
// =======================================

//--------------------------------------------------------------------------------------------------
/// The number of services we expect.  This is used to size the Session List hashmap.
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


//--------------------------------------------------------------------------------------------------
/**
 * Represents a user.  Objects of this type are allocated from the User Pool and are kept on the
 * User List.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t       link;               ///< Used to link into the User List.
    uid_t               uid;                ///< Unique Unix user ID.
    char                name[LIMIT_MAX_USER_NAME_BYTES]; ///< Name of the user.
    le_dls_List_t       bindingList;        ///< List of bindings of user's client i/fs to services.
    le_dls_List_t       serviceList;        ///< List of services served up by this user.
}
User_t;


//--------------------------------------------------------------------------------------------------
/// Pool from which User objects are allocated.
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t UserPoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * The User List, in which all User objects are kept.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t UserList = LE_DLS_LIST_INIT;



/// Forward declaration of the ServicePtr_t.  Needed because of bi-directional linkage between
/// Service objects and Connection objects (they point to each other).
typedef struct Service* ServicePtr_t;


//--------------------------------------------------------------------------------------------------
/**
 * Represents a binding from a user's client interface to a service.  Objects of this type are
 * allocated from the Binding Pool and are kept on a User object's Binding List.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t       link;               ///< Used to link into the User's Binding List.
    User_t*             userPtr;            ///< Ptr to the client User whose Binding List I'm in.
    char                serviceName[LIMIT_MAX_SERVICE_NAME_BYTES]; ///< Service name client uses.
    ServicePtr_t        servicePtr;         ///< Ptr to Service object the client is bound to.
}
Binding_t;


//--------------------------------------------------------------------------------------------------
/// Pool from which Binding objects are allocated.
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t BindingPoolRef;


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
    User_t*                 userPtr;        ///< Pointer to the User object for the client uid.
    pid_t                   pid;            ///< Process ID of client process.
    ServicePtr_t            servicePtr;     ///< Pointer to the service
    ssize_t             maxProtocolMsgSize; ///< Max service protocol msg size (in bytes) or -1.
}
ClientConnection_t;


//--------------------------------------------------------------------------------------------------
/// Pool from which Client Connection objects are allocated.
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ClientConnectionPoolRef;


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
    User_t*                 userPtr;        ///< Pointer to the User object for the server uid.
    pid_t                   pid;            ///< Process ID of server process.
    ServicePtr_t            servicePtr;     ///< Pointer to the service
    ssize_t             maxProtocolMsgSize; ///< Max service protocol msg size (in bytes) or -1.
}
ServerConnection_t;


//--------------------------------------------------------------------------------------------------
/// Pool from which Server Connection objects are allocated.
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ServerConnectionPoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * Represents a service.  Objects of this type are allocated from the Service Pool and are kept
 * on User objects' Service Lists.
 */
//--------------------------------------------------------------------------------------------------
typedef struct Service
{
    le_dls_Link_t       link;               ///< Used to link into a User's Service List.
    User_t*             userPtr;            ///< Ptr to the client User whose Service List I'm in.
    char serviceName[LIMIT_MAX_SERVICE_NAME_BYTES];  ///< Service name advertised by server.
    char protocolId[LIMIT_MAX_PROTOCOL_ID_BYTES];    ///< Protocol ID string (usually a hash).
    ServerConnection_t* serverConnectionPtr;///< Ptr to server connection (NULL if no server yet).
    le_dls_List_t       waitingClientsList; ///< List of Client Connectns waiting for this service.
}
Service_t;


//--------------------------------------------------------------------------------------------------
/// Pool from which Service objects are allocated.
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ServicePoolRef;


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
 * Creates a User object for a given Unix user ID.
 *
 * @return Pointer to the new User object.
 **/
//--------------------------------------------------------------------------------------------------
static User_t* CreateUser
(
    uid_t uid   ///< [in] The user ID.
)
//--------------------------------------------------------------------------------------------------
{
    User_t* userPtr = le_mem_ForceAlloc(UserPoolRef);

    userPtr->link = LE_DLS_LINK_INIT;
    userPtr->uid = uid;

    le_result_t result = user_GetName(uid, userPtr->name, sizeof(userPtr->name));
    if (result != LE_OK)
    {
        LE_ERROR("Error (%s) getting user name for uid %u.", LE_RESULT_TXT(result), uid);
        memset(userPtr->name, 0, sizeof(userPtr->name));
    }

    userPtr->bindingList = LE_DLS_LIST_INIT;
    userPtr->serviceList = LE_DLS_LIST_INIT;

    // Add it to the User List.
    le_dls_Queue(&UserList, &userPtr->link);

    return userPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Searches the User List for a particular Unix user ID.
 *
 * @return Pointer to the User object or NULL if not found.
 **/
//--------------------------------------------------------------------------------------------------
static User_t* FindUser
(
    uid_t uid   ///< [in] The user ID.
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&UserList);

    while (linkPtr != NULL)
    {
        User_t* userPtr = CONTAINER_OF(linkPtr, User_t, link);

        if (userPtr->uid == uid)
        {
            return userPtr;
        }

        linkPtr = le_dls_PeekNext(&UserList, linkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Destructor function that runs when a User object's reference count reaches zero and
 * the object is about to be released back into its pool.
 */
//--------------------------------------------------------------------------------------------------
static void UserDestructor
(
    void* objPtr
)
//--------------------------------------------------------------------------------------------------
{
    User_t* userPtr = objPtr;

    // Remove the User object from the User List.
    le_dls_Remove(&UserList, &userPtr->link);
}


//--------------------------------------------------------------------------------------------------
/**
 * Searches a (client) User's Binding List for a particular service name.
 *
 * @return Pointer to the Binding object or NULL if not found.
 **/
//--------------------------------------------------------------------------------------------------
static Binding_t* FindBinding
(
    User_t* userPtr,            ///< [in] Pointer to the client's User object.
    const char* serviceName     ///< [in] Client's service name.
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&userPtr->bindingList);

    while (linkPtr != NULL)
    {
        Binding_t* bindingPtr = CONTAINER_OF(linkPtr, Binding_t, link);

        if (strcmp(bindingPtr->serviceName, serviceName) == 0)
        {
            return bindingPtr;
        }

        linkPtr = le_dls_PeekNext(&userPtr->bindingList, linkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Destructor function that runs when a Binding object's reference count reaches zero and
 * it is about to be released back into its Pool.
 */
//--------------------------------------------------------------------------------------------------
static void BindingDestructor
(
    void* objPtr
)
//--------------------------------------------------------------------------------------------------
{
    Binding_t* bindingPtr = objPtr;

    // Remove the Binding object from the User's Binding List.
    le_dls_Remove(&bindingPtr->userPtr->bindingList, &bindingPtr->link);

    // Release the Binding's reference count on the Service object.
    le_mem_Release(bindingPtr->servicePtr);
    bindingPtr->servicePtr = NULL;

    // Release the Binding's reference count on the User object.
    le_mem_Release(bindingPtr->userPtr);
    bindingPtr->userPtr = NULL;
}



//--------------------------------------------------------------------------------------------------
/**
 * Destructor function that runs when a Service object's reference count reaches zero and
 * it is about to be released back into its Pool.
 */
//--------------------------------------------------------------------------------------------------
static void ServiceDestructor
(
    void* objPtr
)
//--------------------------------------------------------------------------------------------------
{
    Service_t* servicePtr = objPtr;

    // Remove the Service object from the User's Service List.
    le_dls_Remove(&servicePtr->userPtr->serviceList, &servicePtr->link);

    // Release the Service's reference count on the User object.
    le_mem_Release(servicePtr->userPtr);
    servicePtr->userPtr = NULL;
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
    // Release the Client Connection object.  Let ClientConnectionDestructor() do the work.
    le_mem_Release(connectionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Rejects a connection with a client process.
 *
 * This is used when a client is not permitted to access the service it has requested access to.
 *
 * Generally, this only happens when the server and the client disagree on the maximum message
 * size of the protocol they want to use to communicate with each other.  In other instances,
 * the client just ends up waiting for the server to advertise the service.
 */
//--------------------------------------------------------------------------------------------------
static void RejectClient
(
    ClientConnection_t* connectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;

    // Send rejection message to the client.
    le_result_t rejectReason = LE_NOT_PERMITTED;
    result = unixSocket_SendDataMsg(connectionPtr->fd, &rejectReason, sizeof(rejectReason));

    if (result != LE_OK)
    {
        LE_ERROR("Failed to send rejection message to client %u '%s', pid %d. (%s).",
                 connectionPtr->userPtr->uid,
                 connectionPtr->userPtr->name,
                 connectionPtr->pid,
                 LE_RESULT_TXT(result));
    }

    CloseClientConnection(connectionPtr);
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
    // Release the Server Connection object.  Let ServerConnectionDestructor() do the work.
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
 *
 * @return A pointer to the new object.
 */
//--------------------------------------------------------------------------------------------------
static Service_t* CreateService
(
    User_t* userPtr,                    ///< [in] Pointer to the User who will serve this up.
    const char* serviceName,            ///< [in] Service name string.
    const char* protocolId              ///< [in] Protocol ID string.
)
//--------------------------------------------------------------------------------------------------
{
    Service_t* servicePtr = le_mem_ForceAlloc(ServicePoolRef);

    servicePtr->link = LE_DLS_LINK_INIT;
    servicePtr->waitingClientsList = LE_DLS_LIST_INIT;
    servicePtr->serverConnectionPtr = NULL;
    servicePtr->userPtr = userPtr;

    // Store the service identity inside the object
    // NOTE: We know that the serviceName and protocolId strings are the right length already.
    le_utf8_Copy(servicePtr->serviceName, serviceName,  sizeof(servicePtr->serviceName), NULL);
    le_utf8_Copy(servicePtr->protocolId, protocolId, sizeof(servicePtr->protocolId), NULL);

    // Add the object to the User's Service List.
    le_dls_Queue(&userPtr->serviceList, &servicePtr->link);

    // The Service object holds a reference count on the User object.
    le_mem_AddRef(userPtr);

    return servicePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a Service object, with one waiting client on its list of waiting clients.
 */
//--------------------------------------------------------------------------------------------------
static void CreateServiceWithWaitingClient
(
    ClientConnection_t* connectionPtr,      ///< [in] Pointer to client's connection.
    const char* serviceName,            ///< [in] Service name string.
    const char* protocolId              ///< [in] Protocol ID string.
)
//--------------------------------------------------------------------------------------------------
{
    Service_t* servicePtr = CreateService(connectionPtr->userPtr, serviceName, protocolId);

    LE_DEBUG("Client (uid %u '%s', pid %d) is waiting for service '%s:%s'.",
             connectionPtr->userPtr->uid,
             connectionPtr->userPtr->name,
             connectionPtr->pid,
             serviceName,
             protocolId);

    // Add the client connection to the list of waiting clients.
    // Note:    The Client Connection object takes over ownership of the first reference to the
    //          Service object.
    le_dls_Queue(&servicePtr->waitingClientsList, &connectionPtr->link);
    connectionPtr->servicePtr = servicePtr;
}



//--------------------------------------------------------------------------------------------------
/**
 * Attaches a Server Connection to a given Service object for a service that it is advertising.
 *
 * NOTE: Does not increment the use count on the Service object.
 **/
//--------------------------------------------------------------------------------------------------
static void AttachServerToService
(
    ServerConnection_t* connectionPtr,      ///< [in] Pointer to the server's Connection object.
    Service_t* servicePtr                   ///< [in] Pointer to the Service object.
)
//--------------------------------------------------------------------------------------------------
{
    // Attach this server to this service.
    servicePtr->serverConnectionPtr = connectionPtr;
    connectionPtr->servicePtr = servicePtr;

    LE_DEBUG("Server (uid %u '%s', pid %d) now serving service '%s:%s'.",
             connectionPtr->userPtr->uid,
             connectionPtr->userPtr->name,
             connectionPtr->pid,
             servicePtr->serviceName,
             servicePtr->protocolId);
}


//--------------------------------------------------------------------------------------------------
/**
 * Searches a User's Service List for a particular service ID.
 *
 * @return Pointer to the Service object or NULL if not found.
 **/
//--------------------------------------------------------------------------------------------------
static Service_t* FindService
(
    const User_t* userPtr,      ///< [in] Ptr to the User object to search.
    const char* serviceName,    ///< [in] Service name string.
    const char* protocolId      ///< [in] Protocol ID string.
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&userPtr->serviceList);

    while (linkPtr != NULL)
    {
        Service_t* servicePtr = CONTAINER_OF(linkPtr, Service_t, link);

        if (strcmp(servicePtr->serviceName, serviceName) == 0)
        {
            if (strcmp(servicePtr->protocolId, protocolId) == 0)
            {
                return servicePtr;
            }
            else
            {
                LE_DEBUG("Service name '%s' found, but protocol ID doesn't match.",
                         servicePtr->serviceName);
            }
        }

        linkPtr = le_dls_PeekNext(&userPtr->serviceList, linkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a Binding object for a given binding between a client user's service name and a
 * Service.
 **/
//--------------------------------------------------------------------------------------------------
static void CreateBinding
(
    User_t* clientPtr,              ///< [in] Pointer to the client's User object.
    const char* clientServiceName,  ///< [in] Client's service name.
    User_t* serverPtr,              ///< [in] Pointer to the server's User object.
    const char* serviceName,        ///< [in] Service name string.
    const char* protocolId          ///< [in] Protocol ID string.
)
//--------------------------------------------------------------------------------------------------
{
    Binding_t* bindingPtr = le_mem_ForceAlloc(BindingPoolRef);

    bindingPtr->link = LE_DLS_LINK_INIT;

    // Copy the client's service name into the Binding object.
    // Note: we know the service name and protocol ID strings are valid lengths.
    le_utf8_Copy(bindingPtr->serviceName, clientServiceName, sizeof(bindingPtr->serviceName), NULL);

    // Look for the service that the client service name is being bound to.
    bindingPtr->servicePtr = FindService(serverPtr, serviceName, protocolId);

    // If found,
    if (bindingPtr->servicePtr != NULL)
    {
        // Count the Binding object's reference to the Service object.
        le_mem_AddRef(bindingPtr->servicePtr);
    }
    // If NOT found,
    else
    {
        // Create a new Service object for this service.
        bindingPtr->servicePtr = CreateService(serverPtr, serviceName, protocolId);

        // Note: In this case, the Binding object takes ownership of the initial reference count
        //       on the new Service object.
    }

    // Add the Binding to the client User's Binding List.
    le_dls_Queue(&clientPtr->bindingList, &bindingPtr->link);

    // The Binding object holds a reference to the client User object.
    bindingPtr->userPtr = clientPtr;
    le_mem_AddRef(clientPtr);
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
        LE_DEBUG("Client (uid %u '%s', pid %d) connected to server (uid %u '%s', pid %d) for "
                    "service '%s:%s'.",
                 clientConnectionPtr->userPtr->uid,
                 clientConnectionPtr->userPtr->name,
                 clientConnectionPtr->pid,
                 serverConnectionPtr->userPtr->uid,
                 serverConnectionPtr->userPtr->name,
                 serverConnectionPtr->pid,
                 serverConnectionPtr->servicePtr->serviceName,
                 serverConnectionPtr->servicePtr->protocolId);

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

        // Check that the client agrees with the server on the protocol's maximum message size.
        // If not, drop the client connection without dispatching it to the server.
        if (   clientConnectionPtr->maxProtocolMsgSize
            != servicePtr->serverConnectionPtr->maxProtocolMsgSize )
        {
            LE_ERROR("Client (uid %u '%s', pid %d) disagrees with server (uid %u '%s', pid %d) "
                        "on max message size (%zu vs. %zu) of service '%s:%s'.",
                     clientConnectionPtr->userPtr->uid,
                     clientConnectionPtr->userPtr->name,
                     clientConnectionPtr->pid,
                     servicePtr->serverConnectionPtr->userPtr->uid,
                     servicePtr->serverConnectionPtr->userPtr->name,
                     servicePtr->serverConnectionPtr->pid,
                     clientConnectionPtr->maxProtocolMsgSize,
                     servicePtr->serverConnectionPtr->maxProtocolMsgSize,
                     servicePtr->serviceName,
                     servicePtr->protocolId);

            RejectClient(clientConnectionPtr);
        }
        else
        {
            // Note: if successful, DispatchClientToServer() will close the client connection,
            //       which will remove the client from the list of waiting clients.
            //       Otherwise, the client will be left on the waiting list.
            if (   DispatchClientToServer(servicePtr->serverConnectionPtr, clientConnectionPtr)
                != LE_OK )
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
 * Connects the Service Directory to the Config Tree service as a client.
 **/
//--------------------------------------------------------------------------------------------------
/*
static void ConnectToConfigTree
(
    void
)
//--------------------------------------------------------------------------------------------------
{

}
*/

//--------------------------------------------------------------------------------------------------
/**
 * Process an advertisement by a server of a service.
 *
 * @note This will dispatch waiting clients to the service's new server, if there are any.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessAdvertisementFromServer
(
    ServerConnection_t* connectionPtr,      ///< [in] Ptr to Connection to advertising server.
    const svcdir_ServiceId_t* serviceIdPtr  ///< [in] Ptr to the service identity.
)
//--------------------------------------------------------------------------------------------------
{
    // Store the server's maximum message size limit for later.
    connectionPtr->maxProtocolMsgSize = serviceIdPtr->maxProtocolMsgSize;

    Service_t* servicePtr = FindService(connectionPtr->userPtr,
                                        serviceIdPtr->serviceName,
                                        serviceIdPtr->protocolId);

    // If the service didn't already exist in the user's service list, add it.
    if (servicePtr == NULL)
    {
        servicePtr = CreateService(connectionPtr->userPtr,
                                   serviceIdPtr->serviceName,
                                   serviceIdPtr->protocolId);

        // Note:    The Server Connection object takes over ownership of the first reference to the
        //          Service object.

        AttachServerToService(connectionPtr, servicePtr);
    }
    // If the service was found on this user's service list,
    else
    {
        // Check that it isn't a duplicate advertisement of the same service.
        if (servicePtr->serverConnectionPtr != NULL)
        {
            LE_ERROR("Server (uid %u '%s', pid %d) already offers service '%s:%s'.",
                     servicePtr->userPtr->uid,
                     servicePtr->userPtr->name,
                     servicePtr->serverConnectionPtr->pid,
                     servicePtr->serviceName,
                     servicePtr->protocolId);
            LE_ERROR("Dropping connection to server (pid %d).", connectionPtr->pid);

            CloseServerConnection(connectionPtr);
        }
        else
        {
            le_mem_AddRef(servicePtr);  // Server will hold a reference to the Service.

            AttachServerToService(connectionPtr, servicePtr);

            DispatchWaitingClients(servicePtr);
        }
    }
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

    LE_DEBUG("Client (uid %u '%s', pid %d) experienced error. Closing.",
             connectionPtr->userPtr->uid,
             connectionPtr->userPtr->name,
             connectionPtr->pid);

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

    LE_DEBUG("Client (uid %u '%s', pid %d) closed their connection.",
             connectionPtr->userPtr->uid,
             connectionPtr->userPtr->name,
             connectionPtr->pid);

    CloseClientConnection(connectionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * If the client is root, attempt to find a match to the requested service, offered by any user.
 *
 * @todo    Remove auto-binding when support for Command-Line Apps is added to the mk tools.
 *          This doesn't fit with our Mandatory Access Control philosophy.
 *
 * @return  true if the connection was successfully auto-bound.
 **/
//--------------------------------------------------------------------------------------------------
static bool AutoBind
(
    ClientConnection_t* connectionPtr,      ///< [IN] Pointer to the Client Connection object.
    const svcdir_ServiceId_t* serviceIdPtr  ///< [IN] Ptr to the service ID the client is opening.
)
//--------------------------------------------------------------------------------------------------
{
    // Only root is allowed to auto-bind.  No one can be allowed to bypass access control
    // in this way.
    if (connectionPtr->userPtr->uid == 0)
    {
        le_dls_Link_t* linkPtr = le_dls_Peek(&UserList);
        while (linkPtr != NULL)
        {
            User_t* userPtr = CONTAINER_OF(linkPtr, User_t, link);

            // Look up the Service in this user's list of services that it exports.
            Service_t* servicePtr = FindService(userPtr,
                                                serviceIdPtr->serviceName,
                                                serviceIdPtr->protocolId);
            // If a matching Service is found,
            if (servicePtr != NULL)
            {
                AddClientToService(servicePtr, connectionPtr);
                return true;
            }

            linkPtr = le_dls_PeekNext(&UserList, linkPtr);
        }
    }

    return false;
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
    // Store the number of bytes that the client said it thinks is the maximum protocol message size
    // for the service for later use.
    connectionPtr->maxProtocolMsgSize = serviceIdPtr->maxProtocolMsgSize;

    // Look up the service ID in the User's Binding List.
    Binding_t* bindingPtr = FindBinding(connectionPtr->userPtr, serviceIdPtr->serviceName);

    // If a matching binding was found, add the client to the service it is bound to.
    if (bindingPtr != NULL)
    {
        LE_DEBUG("Following binding %s.%s=%s.%s:%s",
                 bindingPtr->userPtr->name,
                 bindingPtr->serviceName,
                 bindingPtr->servicePtr->userPtr->name,
                 bindingPtr->servicePtr->serviceName,
                 bindingPtr->servicePtr->protocolId);

        AddClientToService(bindingPtr->servicePtr, connectionPtr);
    }
    // If not found,
    else
    {
        // Look up the Service in the client user's own list of services that it exports.
        Service_t* servicePtr = FindService(connectionPtr->userPtr,
                                            serviceIdPtr->serviceName,
                                            serviceIdPtr->protocolId);

        // If a matching Service is found,
        if (servicePtr != NULL)
        {
            AddClientToService(servicePtr, connectionPtr);
        }
        // If the Service object doesn't exist yet,
        else
        {
            // Try to auto-bind.
            if (!AutoBind(connectionPtr, serviceIdPtr))
            {
                // Auto-binding failed.
                // Assume that this user will eventually serve up this service to itself.
                CreateServiceWithWaitingClient(connectionPtr,
                                               serviceIdPtr->serviceName,
                                               serviceIdPtr->protocolId);
            }
        }
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

    // If the connection has closed or there is simply nothing left to be received
    // from the socket,
    if ((result == LE_CLOSED) || (result == LE_WOULD_BLOCK))
    {
        // We are done.
        // NOTE: If the connection closed, our hang-up handler will be called.
    }
    else
    {
        // The client should only send us the service identification details once, at which time
        // we add the connection to the Service's list of waiting clients.  So, if this connection
        // is already on a list of waiting clients, it means we shouldn't be receiving data from it.
        if (connectionPtr->servicePtr != NULL)
        {
            LE_ERROR("Client (uid %u '%s', pid %d) sent data while waiting for service '%s:%s'.",
                     connectionPtr->userPtr->uid,
                     connectionPtr->userPtr->name,
                     connectionPtr->pid,
                     connectionPtr->servicePtr->serviceName,
                     connectionPtr->servicePtr->protocolId);

            // Drop connection to misbehaving client.
            RejectClient(connectionPtr);
        }
        else if (result == LE_OK)
        {
            ProcessOpenRequestFromClient(connectionPtr, &serviceId);
        }
        // If an error occurred on the receive,
        else
        {
            LE_ERROR("Failed to receive service ID from client (uid %u '%s', pid %d).",
                     connectionPtr->userPtr->uid,
                     connectionPtr->userPtr->name,
                     connectionPtr->pid);

            // Drop the Client connection to trigger a recovery action by the client (or the
            // Supervisor, if the client dies).
            RejectClient(connectionPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a Client Connection object to track a given connection to a given client process.
 **/
//--------------------------------------------------------------------------------------------------
static void CreateClientConnection
(
    int     fd,     ///< [in] File descriptor for the connection.
    uid_t   uid,    ///< [in] Unix user ID of the connected process.
    pid_t   pid     ///< [in] Unix process ID of the connected process.
)
//--------------------------------------------------------------------------------------------------
{
    User_t* userPtr = FindUser(uid);

    // If a User object already exists for this user, then increment its reference count because
    // this Connection object will hold a reference to it.  Otherwise, create a new User object
    // and this Connection object will take over that first reference count.
    if (userPtr != NULL)
    {
        le_mem_AddRef(userPtr);
    }
    else
    {
        userPtr = CreateUser(uid);
    }

    // Allocate a new Client Connection object.
    ClientConnection_t* connectionPtr = le_mem_ForceAlloc(ClientConnectionPoolRef);

    connectionPtr->link = LE_DLS_LINK_INIT;
    connectionPtr->fd = fd;
    connectionPtr->userPtr = userPtr;
    connectionPtr->pid = pid;
    connectionPtr->servicePtr = NULL;   // No service for this connection, yet.
    connectionPtr->maxProtocolMsgSize = -1; // Haven't received the "Open" request yet.

    // Set up a File Descriptor Monitor for this new connection, and monitor for hang-up,
    // error, and data arriving.

    char fdMonName[64];  // Buffer for holding the FD Monitor's name string.

    snprintf(fdMonName, sizeof(fdMonName), "Client:fd%duid%upid%d", fd, uid, pid);
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

    // Set a pointer to the Connection object as the FD Handler context.
    le_event_SetFdHandlerContextPtr(handlerRef, connectionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Destructor function that runs when a Client Connection object's reference count reaches zero and
 * it is about to be released back into its Pool.
 */
//--------------------------------------------------------------------------------------------------
static void ClientConnectionDestructor
(
    void* objPtr
)
//--------------------------------------------------------------------------------------------------
{
    ClientConnection_t* connectionPtr = objPtr;

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
    if (connectionPtr->fdMonitorRef != NULL)
    {
        le_event_DeleteFdMonitor(connectionPtr->fdMonitorRef);
        connectionPtr->fdMonitorRef = NULL;
    }

    // Release the Connection object's reference to the User object.
    le_mem_Release(connectionPtr->userPtr);
    connectionPtr->userPtr = NULL;

    // Close the socket.
    fd_Close(connectionPtr->fd);
    connectionPtr->fd = -1;
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
        struct ucred credentials;

        socklen_t credentialsSize = sizeof(credentials);

        // Get the remote process's credentials.
        if (0 != getsockopt(fd,
                            SOL_SOCKET,
                            SO_PEERCRED,
                            &credentials,
                            &credentialsSize) )
        {
            LE_ERROR("Failed to obtain credentials from client.  Errno = %d (%m)", errno);
            fd_Close(fd);
        }
        else
        {
            LE_DEBUG("Client connected:  pid = %d;  uid = %d;  gid = %d.",
                     credentials.pid,
                     credentials.uid,
                     credentials.gid);

            // Create a Connection object to use to track this connection.
            CreateClientConnection(fd, credentials.uid, credentials.pid);

            // Now we wait for the client to send us the session details (or disconnect).
            // When that happens, our client fd event handler functions will be called.
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

    LE_DEBUG("Server (uid %u '%s', pid %d) experienced error. Closing.",
             connectionPtr->userPtr->uid,
             connectionPtr->userPtr->name,
             connectionPtr->pid);

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

    LE_DEBUG("Server connection closed (uid %u '%s', pid %d).",
             connectionPtr->userPtr->uid,
             connectionPtr->userPtr->name,
             connectionPtr->pid);

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
    else if (connectionPtr->servicePtr != NULL)
    {
        // The Server Connection is already associated with a Service object.
        // The server should only send us the service identification details, and only once.
        // After we get that, the Connection object becomes associated with a Service object.
        // So, if this connection is already associated with a Service object, we shouldn't be
        // receiving data from it.
        LE_ERROR("Server sent extra data (uid %u '%s', pid %d, service %s[%s]).",
                 connectionPtr->userPtr->uid,
                 connectionPtr->userPtr->name,
                 connectionPtr->pid,
                 connectionPtr->servicePtr->serviceName,
                 connectionPtr->servicePtr->protocolId);

        CloseServerConnection(connectionPtr);
    }
    else if (result != LE_OK)
    {
        LE_ERROR("Failed to receive service ID from server (uid %u '%s', pid %d).",
                 connectionPtr->userPtr->uid,
                 connectionPtr->userPtr->name,
                 connectionPtr->pid);

        CloseServerConnection(connectionPtr);
    }
    else
    {
        // Got the service advertisement.  Now process it.
        ProcessAdvertisementFromServer(connectionPtr, &serviceId);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a Server Connection object to track a given connection to a given server process.
 **/
//--------------------------------------------------------------------------------------------------
static void CreateServerConnection
(
    int     fd,     ///< [in] File descriptor for the connection.
    uid_t   uid,    ///< [in] Unix user ID of the connected process.
    pid_t   pid     ///< [in] Unix process ID of the connected process.
)
//--------------------------------------------------------------------------------------------------
{
    User_t* userPtr = FindUser(uid);

    // If a User object already exists for this user, then increment its reference count because
    // this Connection object will hold a reference to it.  Otherwise, create a new User object
    // and this Connection object will take over that first reference count.
    if (userPtr != NULL)
    {
        le_mem_AddRef(userPtr);
    }
    else
    {
        userPtr = CreateUser(uid);
    }

    // Allocate a new Server Connection object.
    ServerConnection_t* connectionPtr = le_mem_ForceAlloc(ServerConnectionPoolRef);

    connectionPtr->fd = fd;
    connectionPtr->userPtr = userPtr;
    connectionPtr->pid = pid;
    connectionPtr->servicePtr = NULL;   // No service for this connection, yet.
    connectionPtr->maxProtocolMsgSize = -1; // Haven't received the advertisement yet.

    // Set up a File Descriptor Monitor for this new connection, and monitor for hang-up,
    // error, and data arriving.

    char fdMonName[64];  // Buffer for holding the FD Monitor's name string.

    snprintf(fdMonName, sizeof(fdMonName), "Server:fd%duid%upid%d", fd, uid, pid);
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

    // Set a pointer to the Connection object as the FD Handler context.
    le_event_SetFdHandlerContextPtr(handlerRef, connectionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Destructor function that runs when a Server Connection object's reference count reaches zero and
 * it is about to be released back into its Pool.
 */
//--------------------------------------------------------------------------------------------------
static void ServerConnectionDestructor
(
    void* objPtr
)
//--------------------------------------------------------------------------------------------------
{
    ServerConnection_t* connectionPtr = objPtr;

    // If the connection is associated with a service, then it must be the server for that service.
    if (connectionPtr->servicePtr != NULL)
    {
        LE_ASSERT(connectionPtr->servicePtr->serverConnectionPtr == connectionPtr);

        // Detach the server from the session.
        connectionPtr->servicePtr->serverConnectionPtr = NULL;
        LE_DEBUG("Server (uid %u '%s', pid %d) disconnected from service (%s:%s).",
                 connectionPtr->userPtr->uid,
                 connectionPtr->userPtr->name,
                 connectionPtr->pid,
                 connectionPtr->servicePtr->serviceName,
                 connectionPtr->servicePtr->protocolId);

        // Release the server's hold on the Service object.
        le_mem_Release(connectionPtr->servicePtr);
        connectionPtr->servicePtr = NULL;
    }
    else
    {
        LE_DEBUG("Server (uid %u '%s', pid %d) disconnected without ever advertising a service.",
                 connectionPtr->userPtr->uid,
                 connectionPtr->userPtr->name,
                 connectionPtr->pid);
    }

    // Delete the File Descriptor Monitor object.
    if (connectionPtr->fdMonitorRef != NULL)
    {
        le_event_DeleteFdMonitor(connectionPtr->fdMonitorRef);
        connectionPtr->fdMonitorRef = NULL;
    }

    // Release the Connection object's reference to the User object.
    le_mem_Release(connectionPtr->userPtr);
    connectionPtr->userPtr = NULL;

    // Close the socket.
    fd_Close(connectionPtr->fd);
    connectionPtr->fd = -1;
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
        struct ucred credentials;
        socklen_t credentialsSize = sizeof(credentials);

        // Get the remote process's credentials.
        if (0 != getsockopt(fd,
                            SOL_SOCKET,
                            SO_PEERCRED,
                            &credentials,
                            &credentialsSize) )
        {
            LE_ERROR("Failed to obtain credentials from server.  Errno = %d (%m)", errno);
            fd_Close(fd);
        }
        else
        {
            LE_DEBUG("Server connected:  pid = %d;  uid = %u;  gid = %u.",
                     credentials.pid,
                     credentials.uid,
                     credentials.gid);

            // Create a Connection object to use to track this connection.
            CreateServerConnection(fd, credentials.uid, credentials.pid);

            // Now we wait for the server to send us the session details (or disconnect).
            // When that happens, our server fd event handler functions will be called.
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
 * Handles the "List Services" request from the 'sdir' tool.
 */
//--------------------------------------------------------------------------------------------------
static void SdirToolListServices
(
    int fd      ///< [in] The file descriptor to write the output to.
)
//--------------------------------------------------------------------------------------------------
{
    // Iterate over the User List, and for each user, iterate over their Service List.
    le_dls_Link_t* userLinkPtr = le_dls_Peek(&UserList);

    while (userLinkPtr != NULL)
    {
        User_t* userPtr = CONTAINER_OF(userLinkPtr, User_t, link);

        le_dls_Link_t* serviceLinkPtr = le_dls_Peek(&userPtr->serviceList);

        if (serviceLinkPtr != NULL)
        {
            dprintf(fd, "User %u (%s) is serving:\n", userPtr->uid, userPtr->name);
        }

        while (serviceLinkPtr != NULL)
        {
            Service_t* servicePtr = CONTAINER_OF(serviceLinkPtr, Service_t, link);

            if (servicePtr->serverConnectionPtr != NULL)
            {
                dprintf(fd,
                        "        %s:%s (max msg size: %zu bytes)\n",
                        servicePtr->serviceName,
                        servicePtr->protocolId,
                        servicePtr->serverConnectionPtr->maxProtocolMsgSize);
            }

            serviceLinkPtr = le_dls_PeekNext(&userPtr->serviceList, serviceLinkPtr);
        }

        userLinkPtr = le_dls_PeekNext(&UserList, userLinkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handles the "List Waiting Clients" request from the 'sdir' tool.
 */
//--------------------------------------------------------------------------------------------------
static void SdirToolListWaitingClients
(
    int fd      ///< [in] The file descriptor to write the output to.
)
//--------------------------------------------------------------------------------------------------
{
    // Iterate over the User List, and for each user, iterate over their Service List,
    // and for each Service, iterate over their Waiting Clients List.
    le_dls_Link_t* userLinkPtr = le_dls_Peek(&UserList);

    while (userLinkPtr != NULL)
    {
        User_t* userPtr = CONTAINER_OF(userLinkPtr, User_t, link);

        le_dls_Link_t* serviceLinkPtr = le_dls_Peek(&userPtr->serviceList);

        while (serviceLinkPtr != NULL)
        {
            Service_t* servicePtr = CONTAINER_OF(serviceLinkPtr, Service_t, link);

            le_dls_Link_t* connectionLinkPtr = le_dls_Peek(&servicePtr->waitingClientsList);

            if (connectionLinkPtr != NULL)
            {
                dprintf(fd,
                        "Waiting for user %u (%s) to offer service '%s:%s':\n",
                        userPtr->uid,
                        userPtr->name,
                        servicePtr->serviceName,
                        servicePtr->protocolId);
            }

            while (connectionLinkPtr != NULL)
            {
                ClientConnection_t* connectionPtr = CONTAINER_OF(connectionLinkPtr,
                                                                 ClientConnection_t,
                                                                 link);
                dprintf(fd,
                        "        User %d (%s) pid %d\n",
                        connectionPtr->userPtr->uid,
                        connectionPtr->userPtr->name,
                        connectionPtr->pid);

                connectionLinkPtr = le_dls_PeekNext(&servicePtr->waitingClientsList,
                                                    connectionLinkPtr);
            }

            serviceLinkPtr = le_dls_PeekNext(&userPtr->serviceList, serviceLinkPtr);
        }

        userLinkPtr = le_dls_PeekNext(&UserList, userLinkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handles the "List Bindings" request from the 'sdir' tool.
 */
//--------------------------------------------------------------------------------------------------
static void SdirToolListBindings
(
    int fd      ///< [in] The file descriptor to write the output to.
)
//--------------------------------------------------------------------------------------------------
{
    // Iterate over the User List, and for each user, iterate over their Bindings List.
    le_dls_Link_t* userLinkPtr = le_dls_Peek(&UserList);

    while (userLinkPtr != NULL)
    {
        User_t* userPtr = CONTAINER_OF(userLinkPtr, User_t, link);

        le_dls_Link_t* bindingLinkPtr = le_dls_Peek(&userPtr->bindingList);

        if (bindingLinkPtr != NULL)
        {
            dprintf(fd, "User %u (%s):\n", userPtr->uid, userPtr->name);
        }

        while (bindingLinkPtr != NULL)
        {
            Binding_t* bindingPtr = CONTAINER_OF(bindingLinkPtr, Binding_t, link);

            dprintf(fd,
                    "        %s.%s=%s.%s:%s\n",
                    userPtr->name,
                    bindingPtr->serviceName,
                    bindingPtr->servicePtr->userPtr->name,
                    bindingPtr->servicePtr->serviceName,
                    bindingPtr->servicePtr->protocolId);

            bindingLinkPtr = le_dls_PeekNext(&userPtr->bindingList, bindingLinkPtr);
        }

        userLinkPtr = le_dls_PeekNext(&UserList, userLinkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handles the "List" request from the 'sdir' tool.
 */
//--------------------------------------------------------------------------------------------------
static void SdirToolList
(
    int fd      ///< [in] The file descriptor to write the output to.
)
//--------------------------------------------------------------------------------------------------
{
    if (fd == -1)
    {
        LE_KILL_CLIENT("No output fd provided.");
    }
    else
    {
        dprintf(fd, "SERVICES\n\n");

        SdirToolListServices(fd);

        dprintf(fd, "\nWAITING CLIENTS\n\n");

        SdirToolListWaitingClients(fd);

        dprintf(fd, "\nBINDINGS\n\n");

        SdirToolListBindings(fd);

        fd_Close(fd);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handles an "Unbind All" request from the 'sdir' tool.
 */
//--------------------------------------------------------------------------------------------------
static void SdirToolUnbindAll
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* userLinkPtr = le_dls_Peek(&UserList);

    while (userLinkPtr != NULL)
    {
        User_t* userPtr = CONTAINER_OF(userLinkPtr, User_t, link);

        // Increment the reference count on the User object to ensure that it doesn't go away
        // when we delete all its bindings.
        le_mem_AddRef(userPtr);

        le_dls_Link_t* bindingLinkPtr;

        while ((bindingLinkPtr = le_dls_Peek(&userPtr->bindingList)) != NULL)
        {
            // The destructor will remove it from the User's Binding List, etc.
            le_mem_Release(CONTAINER_OF(bindingLinkPtr, Binding_t, link));
        }

        userLinkPtr = le_dls_PeekNext(&UserList, userLinkPtr);

        // It's okay for the User object to go away now, because we don't need to access it
        // anymore, so we can safely release our reference count now.
        le_mem_Release(userPtr);
    }

}


//--------------------------------------------------------------------------------------------------
/**
 * Handles a "Bind" request from the 'sdir' tool.
 */
//--------------------------------------------------------------------------------------------------
static void SdirToolBind
(
    const le_sdtp_Msg_t* msgPtr   ///< [in] Pointer to the request message payload.
)
//--------------------------------------------------------------------------------------------------
{
    size_t len = strnlen(msgPtr->clientServiceName, LIMIT_MAX_SERVICE_NAME_BYTES);
    if (len == 0)
    {
        LE_KILL_CLIENT("Client service name empty.");
    }
    else if (len == LIMIT_MAX_SERVICE_NAME_BYTES)
    {
        LE_KILL_CLIENT("Client service name not null terminated!");
    }
    else
    {
        len = strnlen(msgPtr->serverServiceName, LIMIT_MAX_SERVICE_NAME_BYTES);
        if (len == 0)
        {
            LE_KILL_CLIENT("Server service name empty.");
        }
        else if (len == LIMIT_MAX_SERVICE_NAME_BYTES)
        {
            LE_KILL_CLIENT("Server service name not null terminated!");
        }
        else
        {
            len = strnlen(msgPtr->protocolId, LIMIT_MAX_PROTOCOL_ID_BYTES);
            if (len == 0)
            {
                LE_KILL_CLIENT("Protocol ID empty.");
            }
            else if (len == LIMIT_MAX_PROTOCOL_ID_BYTES)
            {
                LE_KILL_CLIENT("Protocol ID not null terminated!");
            }
            else
            {
                User_t* clientPtr = FindUser(msgPtr->client);
                if (clientPtr == NULL)
                {
                    clientPtr = CreateUser(msgPtr->client);
                }

                // See if the client already has a bind for this service name.
                Binding_t* bindingPtr = FindBinding(clientPtr, msgPtr->clientServiceName);
                if (bindingPtr != NULL)
                {
                    LE_WARN("Replacing duplicate binding of '%s' for client user %u '%s'.",
                            msgPtr->clientServiceName,
                            clientPtr->uid,
                            clientPtr->name);
                    // NOTE: we wait to delete the old binding until after the new one
                    //       has been created.  This ensures that the reference count on
                    //       the client User object doesn't drop to zero.
                }

                User_t* serverPtr = FindUser(msgPtr->server);
                if (serverPtr == NULL)
                {
                    serverPtr = CreateUser(msgPtr->server);
                }

                CreateBinding(clientPtr,
                              msgPtr->clientServiceName,
                              serverPtr,
                              msgPtr->serverServiceName,
                              msgPtr->protocolId);

                // If we are replacing a binding, delete the old binding now.
                if (bindingPtr != NULL)
                {
                    le_mem_Release(bindingPtr);
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Process a message received from the "sdir" tool.
 */
//--------------------------------------------------------------------------------------------------
static void SdirToolRecv
(
    le_msg_MessageRef_t msgRef, ///< [in] Reference to the received message.
    void* contextPtr            ///< [in] Not used.
)
//--------------------------------------------------------------------------------------------------
{
    le_sdtp_Msg_t* msgPtr = le_msg_GetPayloadPtr(msgRef);

    switch (msgPtr->msgType)
    {
        case LE_SDTP_MSGID_LIST:

            SdirToolList(le_msg_GetFd(msgRef));
            break;

        case LE_SDTP_MSGID_UNBIND_ALL:

            SdirToolUnbindAll();
            break;

        case LE_SDTP_MSGID_BIND:

            SdirToolBind(msgPtr);
            break;

        default:
            LE_KILL_CLIENT("Invalid message ID %d.", msgPtr->msgType);
            break;
    }

    le_msg_Respond(msgRef);
}



//--------------------------------------------------------------------------------------------------
/**
 * Start the "sdir" tool service.
 */
//--------------------------------------------------------------------------------------------------
static void StartSdirToolService
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_ProtocolRef_t protocol = le_msg_GetProtocolRef(LE_SDTP_PROTOCOL_ID,
                                                          sizeof(le_sdtp_Msg_t));
    le_msg_ServiceRef_t service = le_msg_CreateService(protocol, LE_SDTP_SERVICE_NAME);

    le_msg_SetServiceRecvHandler(service, SdirToolRecv, NULL);

    le_msg_AdvertiseService(service);
}



//--------------------------------------------------------------------------------------------------
/**
 * Initialization function.  This is called at start-up.  When it returns, the process's main
 * event loop will run.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    user_Init();    // Initialize the User module.

    // Get references to the pools.
    ServicePoolRef = le_mem_CreatePool("Service", sizeof(Service_t));
    ClientConnectionPoolRef = le_mem_CreatePool("Client Connection", sizeof(ClientConnection_t));
    ServerConnectionPoolRef = le_mem_CreatePool("Server Connection", sizeof(ServerConnection_t));
    UserPoolRef = le_mem_CreatePool("User", sizeof(User_t));
    BindingPoolRef = le_mem_CreatePool("Binding", sizeof(Binding_t));

    /// Expand the pools to their expected maximum sizes.
    /// @todo Make this configurable.
    le_mem_ExpandPool(ServicePoolRef, 30);
    le_mem_ExpandPool(ClientConnectionPoolRef, 100);
    le_mem_ExpandPool(ServerConnectionPoolRef, 30);
    le_mem_ExpandPool(UserPoolRef, 30);
    le_mem_ExpandPool(BindingPoolRef, 30);

    // Register destructor functions.
    le_mem_SetDestructor(ServicePoolRef, ServiceDestructor);
    le_mem_SetDestructor(ClientConnectionPoolRef, ClientConnectionDestructor);
    le_mem_SetDestructor(ServerConnectionPoolRef, ServerConnectionDestructor);
    le_mem_SetDestructor(UserPoolRef, UserDestructor);
    le_mem_SetDestructor(BindingPoolRef, BindingDestructor);

    // Create the Legato runtime directory if it doesn't already exists.
    LE_ASSERT(le_dir_Make(STRINGIZE(LE_RUNTIME_DIR), S_IRWXU | S_IXOTH) != LE_FAULT);

    /// @todo Check permissions of directory containing client and server socket addresses.
    ///       Only the current user or root should be allowed write access.
    ///       Warn if it is found to be otherwise.

    // Open the sockets.
    ClientSocketFd = OpenSocket(STRINGIZE(LE_SVCDIR_CLIENT_SOCKET_NAME));
    ServerSocketFd = OpenSocket(STRINGIZE(LE_SVCDIR_SERVER_SOCKET_NAME));

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

    // Start our own service that we provide to the "sdir" tool.
    StartSdirToolService();

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
