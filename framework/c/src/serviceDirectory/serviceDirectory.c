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
 * exist, and what bindings exist between clients and services.
 *
 * The Service Directory implements @ref serviceDirectoryProtocol.h and @ref sdirToolProtocol.h.
 *
 *
 * @section sd_accessControl        Binding and Access Control
 *
 * The Service Directory is a key component in the implementation of security within the Legato
 * framework.  No two sandboxed applications can access each others files, sockets, shared memory,
 * etc. directly until they have connected to each other through the Service Directory.
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
 * Note that bindings can also be used to connect (bind) a client interface to a server interface
 * that has a different service instance name, so bindings can be useful even when the client and
 * server are running with the same UID.
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
                    +-----------------------------+--------+--------+
                    |                             |        |        |
                    v                             |        |        |
User List ------> User --+---> Name               |        |        |
            [0..n]       |                        |        |        |
                         |                        |        |        |
                         +---> Service --*---> Server      |        |
                         |     List            Connection  |        |
                         |                        ^        |        |
                         |                        |        |        |
                         +---> Binding --*---> Binding ----+        |
                         |     List               |                 |
                         |                        v                 |
                         |                     Waiting              |
                         |                     Clients              |
                         |                     List                 |
                         |                        |                 |
                         |                        *                 |
                         |                        |                 |
                         |                        v                 |
                         +---> Unbound ---*--> Client --------------+
                               Clients         Connection
                               List
@endverbatim
 *
 * The User object represents a single user account.  It has a unique ID which is used as the key
 * to find it in the User List.  Each User also has
 *  - list of bindings between a client-side service name and a server's user name and service name.
 *  - list of services that it offers, and
 *  - list of client connections that are waiting for a binding to be created for them.
 *
 * Binding objects are created for bindings that appear in the configuration data.  The 'sdir' tool
 * is in charge of reading the configuration data and pushing updates to the Service Directory.
 * The Service Directory creates and deletes Binding objects in response to messages received from
 * the 'sdir' tool.  Each Binding object has a list of client connections that match that binding
 * but are waiting for the server to advertise the service.
 *
 * Connection objects are used to keep track of the details of socket connections (e.g., the
 * file descriptor, File Descriptor Monitor object, etc.) and the service name, protocol ID, and
 * maximum message size advertised or requested.  Server Connections keep track of
 * connections to servers.  Client Connections keep track of connections to clients.
 *
 * Client Connection objects and Server Connection objects are created when clients and servers
 * connect to the Service Directory.
 *
 * Client Connection objects are deleted when the client disconnects or its connection is passed
 * to a server.
 *
 * Server Connection objects are deleted when the server disconnects.
 *
 * Each Binding object and Connection object holds a reference count on a User object.  A User
 * object will be deleted when all associated Binding objects and Connection objects are deleted.
 *
 *
 * @section sd_theoryOfOperation Theory of Operation
 *
 * When a client connects and makes a request to open a service, the client's UID is looked up in
 * the User List.  The client User's Binding List is searched for the service name requested by
 * the client.  If a matching Binding object is not found, the Client Connection object is added
 * to the User object's Unbound Clients List.  If a matching Binding object is found, it will
 * specify the server User object and service name.  The server's User's Service List will be
 * searched for a matching Server Connection object.  If no matching Server Connection can be
 * found, the Client Connection is added to the Binding object's Waiting Clients List.
 *
 * When a server connects and advertises a service, the server UID is looked-up in the User List.
 * The service name is then searched for in the Service List for that User.  If a Server Connection
 * object is not found for that service name on that User, the new one is is added to the list.
 * Otherwise, the new server connection is dropped.
 *
 * When a new Server Connection is added to a Service List, all users' Binding Lists are
 * searched for matching bindings, and if any that match have non-empty Waiting Clients Lists,
 * all those Client Connections are removed from those lists and dispatched to the new Server
 * Connection.
 *
 * When a Binding is added, it is added to the client's User object's Binding List.  That user's
 * Unbound Clients List will then be checked for matches to the new binding, and if any are found,
 * they will be removed from the Unbound Clients List and processed as though they are
 * new client connections (see above).
 *
 * Likewise, if a Binding is deleted while it has Client Connections on its Waiting Clients List,
 * those Client Connections will be removed from that list and processed as though they are new
 * client connections (see above).
 *
 * NOTE: It is outside the Service Directory's scope to terminate client IPC connections that
 * were established through bindings that have been changed.  The Service Directory does not
 * keep track of client-server connections after they have been established.  (However, this could
 * be changed.)
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
 * @subsection sd_DesignNotesConfig     Binding Configuration
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
 * @subsection sd_DesignNotesLateBind   Late Binding Updates
 *
 * Note that bindings can be updated after the client and/or server have already been started.
 * Therefore, we must check the waiting clients list of a user whenever a binding of one of that
 * user's client-side interfaces is added or removed, to see if the waiting client can now be
 * connected to a server.
 *
 * <hr/>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
    le_dls_Link_t   link;               ///< Used to link into the User List.
    uid_t           uid;                ///< Unique Unix user ID.
    char            name[LIMIT_MAX_USER_NAME_BYTES]; ///< Name of the user.
    le_dls_List_t   bindingList;        ///< List of bindings of user's client i/fs to services.
    le_dls_List_t   serviceList;        ///< List of services served up by this user.
    le_dls_List_t   unboundClientsList; ///< List of Client Connections waiting to be bound.
}
User_t;


//--------------------------------------------------------------------------------------------------
/// Pool from which User objects are allocated.
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t UserPoolRef;


//--------------------------------------------------------------------------------------------------
/// The User List, in which all User objects are kept.
//--------------------------------------------------------------------------------------------------
static le_dls_List_t UserList = LE_DLS_LIST_INIT;



//--------------------------------------------------------------------------------------------------
/**
 * Represents a connection to a server process.  Objects of this type are allocated from
 * the Server Connection Pool.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t           link;           ///< Used to link onto user's Service List.
    int                     fd;             ///< Fd of the connection socket.
    le_event_FdMonitorRef_t fdMonitorRef;   ///< FD Monitor object monitoring this connection.
    User_t*                 userPtr;        ///< Pointer to the User object for the client uid.
    pid_t                   pid;            ///< Process ID of client process.
    svcdir_ServiceId_t      serviceId;      ///< Service identifier.
}
ServerConnection_t;


//--------------------------------------------------------------------------------------------------
/// Pool from which Server Connection objects are allocated.
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ServerConnectionPoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * Represents a binding from a user's client interface to a service.  Objects of this type are
 * allocated from the Binding Pool and are kept on a User object's Binding List.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t       link;               ///< Used to link into the User's Binding List.
    User_t*             clientUserPtr;      ///< Ptr to the client User whose Binding List I'm in.
    User_t*             serverUserPtr;      ///< Ptr to the User who serves the service.
    char                clientServiceName[LIMIT_MAX_SERVICE_NAME_BYTES]; ///< Name client uses.
    char                serverServiceName[LIMIT_MAX_SERVICE_NAME_BYTES]; ///< Name server uses.
    ServerConnection_t* serverConnectionPtr;///< Ptr to Server Connection (NULL if service unavail.)
    le_dls_List_t       waitingClientsList; ///< List of Client Connections waiting for the service.
}
Binding_t;


//--------------------------------------------------------------------------------------------------
/// Pool from which Binding objects are allocated.
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t BindingPoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of the different states that a client connection can be in.
 **/
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CLIENT_STATE_ID_UNKNOWN,    ///< Service ID not yet received from client. (START STATE)
    CLIENT_STATE_UNBOUND,       ///< On user's Unbound Clients List.
    CLIENT_STATE_WAITING,       ///< On a binding's Waiting Clients List.
}
ClientConnectionState_t;


//--------------------------------------------------------------------------------------------------
/**
 * Represents a connection to a client process.  Objects of this type are allocated from
 * the Client Connection Pool.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t           link;           ///< Used to link onto unbound or waiting clients lists.
    ClientConnectionState_t state;          ///< State of the client connection.
    int                     fd;             ///< Fd of the connection socket.
    le_event_FdMonitorRef_t fdMonitorRef;   ///< FD Monitor object monitoring this connection.
    User_t*                 userPtr;        ///< Pointer to the User object for the client uid.
    pid_t                   pid;            ///< Process ID of client process.
    svcdir_ServiceId_t      serviceId;      ///< Service identifier.
    Binding_t*              bindingPtr;     ///< Ptr to Binding whose Waiting Clients List we are on
}
ClientConnection_t;


//--------------------------------------------------------------------------------------------------
/// Pool from which Client Connection objects are allocated.
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ClientConnectionPoolRef;


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
    userPtr->unboundClientsList = LE_DLS_LIST_INIT;

    // Add it to the User List.
    le_dls_Queue(&UserList, &userPtr->link);

    return userPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Searches the User List for a particular Unix user ID.  If found, increments the reference count
 * on that object.  If not found, creates a new User object.
 *
 * @return Pointer to the User object.
 **/
//--------------------------------------------------------------------------------------------------
static User_t* GetUser
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
            le_mem_AddRef(userPtr);
            return userPtr;
        }

        linkPtr = le_dls_PeekNext(&UserList, linkPtr);
    }

    return CreateUser(uid);
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

        if (strcmp(bindingPtr->clientServiceName, serviceName) == 0)
        {
            return bindingPtr;
        }

        linkPtr = le_dls_PeekNext(&userPtr->bindingList, linkPtr);
    }

    return NULL;
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
 * Searches a User's Service List for a particular service name.
 *
 * @return Pointer to the Server Connection object for the matching service.
 **/
//--------------------------------------------------------------------------------------------------
static ServerConnection_t* FindService
(
    const User_t* userPtr,      ///< [in] Ptr to the User object to search.
    const char* serviceName     ///< [in] Service name string.
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&userPtr->serviceList);

    while (linkPtr != NULL)
    {
        ServerConnection_t* serverConnectionPtr = CONTAINER_OF(linkPtr, ServerConnection_t, link);

        if (strcmp(serverConnectionPtr->serviceId.serviceName, serviceName) == 0)
        {
            return serverConnectionPtr;
        }

        linkPtr = le_dls_PeekNext(&userPtr->serviceList, linkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a given server connection is offering a service that is already being
 * offered by an older server connection.
 *
 * @return true if the service already exists.
 **/
//--------------------------------------------------------------------------------------------------
static bool IsDuplicateService
(
    ServerConnection_t* newConnectionPtr  ///< [in] Ptr to the newer server connection object.
)
//--------------------------------------------------------------------------------------------------
{
    ServerConnection_t* oldConnectionPtr = FindService(newConnectionPtr->userPtr,
                                                       newConnectionPtr->serviceId.serviceName);

    if (oldConnectionPtr == NULL)
    {
        return false;
    }

    // Duplicate detected.  Report diagnostic info.
    if (strcmp(newConnectionPtr->serviceId.protocolId, oldConnectionPtr->serviceId.protocolId) == 0)
    {
        LE_ERROR("Server (uid %u '%s', pid %d) already offers service '%s'.",
                 oldConnectionPtr->userPtr->uid,
                 oldConnectionPtr->userPtr->name,
                 oldConnectionPtr->pid,
                 oldConnectionPtr->serviceId.serviceName);
    }
    else
    {
        LE_ERROR("Server (uid %u '%s', pid %d) already offers service '%s', but with "
                    "different protocol ID (%s).",
                 oldConnectionPtr->userPtr->uid,
                 oldConnectionPtr->userPtr->name,
                 oldConnectionPtr->pid,
                 oldConnectionPtr->serviceId.serviceName,
                 oldConnectionPtr->serviceId.protocolId);
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Dispatch a client connection to a server connection.
 *
 * @warning In some error cases, the client or server connection may be closed by this function.
 *          To prevent possible loss of the Client Connection object, it should be on a Binding
 *          object's Waiting Clients List when it is dispatched.  Then, if the dispatch fails,
 *          due to server failure, the Client Connection will remain on the Waiting Clients List.
 *          On the other hand, if the Client Connection is deleted, its destructor will remove it
 *          from the Binding object's Waiting Clients List.
 *
 * @return  LE_CLOSED if the server connection went down and the Server Connection was deleted.
 *          LE_OK otherwise.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DispatchToServer
(
    ClientConnection_t* clientConnectionPtr, ///< [in] Client connection to be dispatched.
    ServerConnection_t* serverConnectionPtr  ///< [in] Server connection to dispatch client to.
)
//--------------------------------------------------------------------------------------------------
{
    // Check that the client agrees with the server on the protocol ID.
    // If not, drop the client connection without dispatching it to the server.
    if ( 0 != strcmp(clientConnectionPtr->serviceId.protocolId,
                     serverConnectionPtr->serviceId.protocolId ) )
    {
        LE_ERROR("Client (uid %u '%s', pid %d) disagrees with server (uid %u '%s', pid %d) "
                    "on protocol ID of service '%s' ('%s' vs. '%s').",
                 clientConnectionPtr->userPtr->uid,
                 clientConnectionPtr->userPtr->name,
                 clientConnectionPtr->pid,
                 serverConnectionPtr->userPtr->uid,
                 serverConnectionPtr->userPtr->name,
                 serverConnectionPtr->pid,
                 clientConnectionPtr->serviceId.serviceName,
                 clientConnectionPtr->serviceId.protocolId,
                 serverConnectionPtr->serviceId.protocolId);

        RejectClient(clientConnectionPtr);
    }

    // Check that the client agrees with the server on the protocol's maximum message size.
    // If not, drop the client connection without dispatching it to the server.
    else if (   clientConnectionPtr->serviceId.maxProtocolMsgSize
        != serverConnectionPtr->serviceId.maxProtocolMsgSize )
    {
        LE_ERROR("Client (uid %u '%s', pid %d) disagrees with server (uid %u '%s', pid %d) "
                    "on max message size (%zu vs. %zu) of service '%s:%s'.",
                 clientConnectionPtr->userPtr->uid,
                 clientConnectionPtr->userPtr->name,
                 clientConnectionPtr->pid,
                 serverConnectionPtr->userPtr->uid,
                 serverConnectionPtr->userPtr->name,
                 serverConnectionPtr->pid,
                 clientConnectionPtr->serviceId.maxProtocolMsgSize,
                 serverConnectionPtr->serviceId.maxProtocolMsgSize,
                 clientConnectionPtr->serviceId.serviceName,
                 clientConnectionPtr->serviceId.protocolId);

        RejectClient(clientConnectionPtr);
    }

    else
    {
        // Send the client connection fd to the server.
        le_result_t result = unixSocket_SendMsg(serverConnectionPtr->fd,
                                                NULL,   // dataPtr
                                                0,      // dataSize
                                                clientConnectionPtr->fd, // fdToSend
                                                false); // sendCredentials

        if (result == LE_OK)
        {
            LE_DEBUG("Client (uid %u '%s', pid %d) connected to server (uid %u '%s', pid %d) for "
                        "service '%s' (protocol ID = '%s').",
                     clientConnectionPtr->userPtr->uid,
                     clientConnectionPtr->userPtr->name,
                     clientConnectionPtr->pid,
                     serverConnectionPtr->userPtr->uid,
                     serverConnectionPtr->userPtr->name,
                     serverConnectionPtr->pid,
                     serverConnectionPtr->serviceId.serviceName,
                     serverConnectionPtr->serviceId.protocolId);

            // Close the client connection (it has been handed off to the server now).
            CloseClientConnection(clientConnectionPtr);
        }
        else
        {
            // The server seems to have failed.
            // Leave the client on the waiting list, close the server connection.
            CloseServerConnection(serverConnectionPtr);

            return LE_CLOSED;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Processes a client connection by following a binding that matches that client connection.
 *
 * Either dispatches to a server or queues to the binding's list of waiting clients.
 **/
//--------------------------------------------------------------------------------------------------
static void FollowBinding
(
    Binding_t* bindingPtr,
    ClientConnection_t* clientConnectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    clientConnectionPtr->state = CLIENT_STATE_WAITING;

    LE_DEBUG("FOLLOWING BINDING <%s>.%s -> <%s>.%s",
            bindingPtr->clientUserPtr->name,
            bindingPtr->clientServiceName,
            bindingPtr->serverUserPtr->name,
            bindingPtr->serverServiceName);

    clientConnectionPtr->bindingPtr = bindingPtr;

    le_dls_Queue(&bindingPtr->waitingClientsList, &clientConnectionPtr->link);

    if (bindingPtr->serverConnectionPtr != NULL)
    {
        DispatchToServer(clientConnectionPtr, bindingPtr->serverConnectionPtr);
    }
    else
    {
        LE_DEBUG("Client user %s (uid %u) pid %d interface '%s' is waiting for"
                    " server user %s (%u) to advertise service '%s'.",
                 clientConnectionPtr->userPtr->name,
                 clientConnectionPtr->userPtr->uid,
                 clientConnectionPtr->pid,
                 clientConnectionPtr->serviceId.serviceName,
                 bindingPtr->serverUserPtr->name,
                 bindingPtr->serverUserPtr->uid,
                 bindingPtr->serverServiceName);
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Creates a Binding object for a given binding between a client user's service name and a
 * Service.
 **/
//--------------------------------------------------------------------------------------------------
static void CreateBinding
(
    uid_t   clientUserId,           ///< [in] Client's user ID.
    const char* clientServiceName,  ///< [in] Client's service name.
    uid_t   serverUserId,           ///< [in] Server's user ID.
    const char* serverServiceName   ///< [in] Server's service name.
)
//--------------------------------------------------------------------------------------------------
{
    Binding_t* bindingPtr = le_mem_ForceAlloc(BindingPoolRef);

    bindingPtr->link = LE_DLS_LINK_INIT;

    // Copy the service names into the Binding object.
    // Note: we know the service names are valid lengths.
    le_utf8_Copy(bindingPtr->clientServiceName,
                 clientServiceName,
                 sizeof(bindingPtr->clientServiceName),
                 NULL);
    le_utf8_Copy(bindingPtr->serverServiceName,
                 serverServiceName,
                 sizeof(bindingPtr->serverServiceName),
                 NULL);

    // The Binding object holds a reference to the client and server User objects.
    bindingPtr->clientUserPtr = GetUser(clientUserId);
    bindingPtr->serverUserPtr = GetUser(serverUserId);

    bindingPtr->serverConnectionPtr = NULL;
    bindingPtr->waitingClientsList = LE_DLS_LIST_INIT;

    // See if the client already has a bind for this service name.
    Binding_t* oldBindingPtr = FindBinding(bindingPtr->clientUserPtr, clientServiceName);
    if (oldBindingPtr != NULL)
    {
        LE_WARN("Replacing binding of <%s>.%s -> <%s>.%s with -> <%s>.%s.",
                bindingPtr->clientUserPtr->name,
                clientServiceName,
                oldBindingPtr->serverUserPtr->name,
                oldBindingPtr->serverServiceName,
                bindingPtr->serverUserPtr->name,
                bindingPtr->serverServiceName);

        // Delete the old binding.
        // NOTE: This should happen after the new binding gets a reference to the user to
        //       avoid wasting time deleting the User object and then recreating it again.
        le_mem_Release(oldBindingPtr);
    }
    else
    {
        LE_DEBUG("Creating binding: <%s>.%s -> <%s>.%s",
                 bindingPtr->clientUserPtr->name,
                 bindingPtr->clientServiceName,
                 bindingPtr->serverUserPtr->name,
                 bindingPtr->serverServiceName);
    }

    // Add the Binding to the client User's Binding List.
    le_dls_Queue(&bindingPtr->clientUserPtr->bindingList, &bindingPtr->link);

    // Look for a server serving the binding's destination service.
    bindingPtr->serverConnectionPtr = FindService(bindingPtr->serverUserPtr, serverServiceName);

    // Check for unbound client connections that match the new binding.
    le_dls_List_t* unboundClientsListPtr = &(bindingPtr->clientUserPtr->unboundClientsList);
    le_dls_Link_t* linkPtr = le_dls_Peek(unboundClientsListPtr);
    while (linkPtr != NULL)
    {
        ClientConnection_t* clientConnectionPtr = CONTAINER_OF(linkPtr, ClientConnection_t, link);

        // Move the linkPtr to the next node in the list now, in case we have to remove
        // the node it currently points to.
        linkPtr = le_dls_PeekNext(unboundClientsListPtr, linkPtr);

        // If this is the binding this client has been waiting for,
        if (strcmp(clientConnectionPtr->serviceId.serviceName, clientServiceName) == 0)
        {
            // Remove this client connection from the list of unbound clients and
            // dispatch it via the binding.
            // WARNING: Don't use linkPtr here, because it has been moved to the next node already.
            le_dls_Remove(unboundClientsListPtr, &clientConnectionPtr->link);
            FollowBinding(bindingPtr, clientConnectionPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Create built-in, hard-coded bindings.
 **/
//--------------------------------------------------------------------------------------------------
static void CreateHardCodedBindings
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    uid_t uid = getuid();

    CreateBinding(uid, "sdirTool", uid, "sdirTool");
    CreateBinding(uid, "LogClient", uid, "LogClient");
    CreateBinding(uid, "LogControl", uid, "LogControl");
    CreateBinding(uid, "le_sup_ctrl", uid, "le_sup_ctrl");
    CreateBinding(uid, "le_sup_state", uid, "le_sup_state");
    CreateBinding(uid, "le_sup_wdog", uid, "le_sup_wdog");
    CreateBinding(uid, "le_cfg", uid, "le_cfg");
    CreateBinding(uid, "le_cfgAdmin", uid, "le_cfgAdmin");
}


//--------------------------------------------------------------------------------------------------
/**
 * Search for and associate bindings that refer to this service and dispatch any
 * waiting clients to the new server.
 */
//--------------------------------------------------------------------------------------------------
static void ResolveBindingsToServer
(
    ServerConnection_t* connectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    // For each user,
    le_dls_Link_t* userLinkPtr = le_dls_Peek(&UserList);

    while (userLinkPtr != NULL)
    {
        User_t* userPtr = CONTAINER_OF(userLinkPtr, User_t, link);

        // For each of the user's bindings,
        le_dls_Link_t* bindingLinkPtr = le_dls_Peek(&userPtr->bindingList);
        while (bindingLinkPtr != NULL)
        {
            Binding_t* bindingPtr = CONTAINER_OF(bindingLinkPtr, Binding_t, link);

            // If the binding is pointing at the new server's service,
            if (   (connectionPtr->userPtr == bindingPtr->serverUserPtr)
                && (0 == strcmp(connectionPtr->serviceId.serviceName,
                                bindingPtr->serverServiceName))  )
            {
                bindingPtr->serverConnectionPtr = connectionPtr;

                // While there's still a client connection on the Waiting Clients List, get
                // a pointer to the first one, without removing it from the list, then try
                // to dispatch that client to the server.
                le_dls_Link_t* clientLinkPtr;
                while (NULL != (clientLinkPtr = le_dls_Peek(&bindingPtr->waitingClientsList)))
                {
                    ClientConnection_t* clientConnectionPtr = CONTAINER_OF(clientLinkPtr,
                                                                           ClientConnection_t,
                                                                           link);
                    if (DispatchToServer(clientConnectionPtr, connectionPtr) == LE_CLOSED)
                    {
                        // Server went down.  Client was left on the Waiting Clients List.
                        // Server Connection destructor was run and it disconnected itself
                        // from the Binding object.
                        return;
                    }
                    // NOTE: If the server didn't go down, then the Client Connection has been
                    // deleted and its destructor removed it from the Waiting Clients List.
                }
            }

            bindingLinkPtr = le_dls_PeekNext(&userPtr->bindingList, bindingLinkPtr);
        }

        userLinkPtr = le_dls_PeekNext(&UserList, userLinkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Process an advertisement by a server of a service.
 *
 * @note This will dispatch waiting clients to the service's new server, if there are any.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessAdvertisementFromServer
(
    ServerConnection_t* connectionPtr       ///< [in] Ptr to Connection to advertising server.
)
//--------------------------------------------------------------------------------------------------
{
    // Check for a server already serving this same service.
    if (IsDuplicateService(connectionPtr))
    {
        LE_ERROR("Dropping connection to server (uid %u '%s', pid %d) of service '%s' (%s).",
                 connectionPtr->userPtr->uid,
                 connectionPtr->userPtr->name,
                 connectionPtr->pid,
                 connectionPtr->serviceId.serviceName,
                 connectionPtr->serviceId.protocolId);

        CloseServerConnection(connectionPtr);
    }
    // If there wasn't already a server for this service on the user's service list, add this
    // connection to the service list.
    else
    {
        // Add the object to the User's Service List.
        le_dls_Queue(&connectionPtr->userPtr->serviceList, &connectionPtr->link);

        LE_DEBUG("Server (uid %u '%s', pid %d) now serving service '%s' (%s).",
                 connectionPtr->userPtr->uid,
                 connectionPtr->userPtr->name,
                 connectionPtr->pid,
                 connectionPtr->serviceId.serviceName,
                 connectionPtr->serviceId.protocolId);

        // Search for and associate bindings that refer to this service and dispatch any
        // waiting clients to the new server.
        ResolveBindingsToServer(connectionPtr);
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
 * Processes an "Open" request received from a client.
 **/
//--------------------------------------------------------------------------------------------------
static void ProcessOpenRequestFromClient
(
    ClientConnection_t* connectionPtr       ///< [IN] Pointer to the Client Connection object.
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("Processing OPEN request from client pid %u <%s> for service '%s' (%s).",
             connectionPtr->pid,
             connectionPtr->userPtr->name,
             connectionPtr->serviceId.serviceName,
             connectionPtr->serviceId.protocolId);

    // Look up the client's service name in the client User's Binding List.
    Binding_t* bindingPtr = FindBinding(connectionPtr->userPtr,
                                        connectionPtr->serviceId.serviceName);

    // If a matching binding was found, follow it.
    if (bindingPtr != NULL)
    {
        FollowBinding(bindingPtr, connectionPtr);
    }
    // If not found, add the client connection to the user's list of unbound clients.
    else
    {
        connectionPtr->state = CLIENT_STATE_UNBOUND;

        le_dls_Queue(&(connectionPtr->userPtr->unboundClientsList), &(connectionPtr->link));

        LE_DEBUG("Client interface <%s>.%s is unbound.",
                 connectionPtr->userPtr->name,
                 connectionPtr->serviceId.serviceName);
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
    ClientConnection_t* clientConnectionPtr = le_event_GetContextPtr();

    LE_ASSERT(clientConnectionPtr != NULL);

    // Receive the service identity from the client.
    result = ReceiveServiceId(fd, &clientConnectionPtr->serviceId);

    // If the connection has closed or there is simply nothing left to be received
    // from the socket,
    if ((result == LE_CLOSED) || (result == LE_WOULD_BLOCK))
    {
        // We are done.
        // NOTE: If the connection closed, our hang-up handler will be called.
    }
    // The client should only send us the service identification details once.  So, if we
    // already have the service identification details, it means we shouldn't be receiving
    // data from it.
    else if (clientConnectionPtr->state != CLIENT_STATE_ID_UNKNOWN)
    {
        LE_ERROR("Client (uid %u '%s', pid %d) sent data while waiting for service "
                    "'%s:%s'.",
                 clientConnectionPtr->userPtr->uid,
                 clientConnectionPtr->userPtr->name,
                 clientConnectionPtr->pid,
                 clientConnectionPtr->serviceId.serviceName,
                 clientConnectionPtr->serviceId.protocolId);

        // Drop connection to misbehaving client.
        RejectClient(clientConnectionPtr);
    }
    else if (result == LE_OK)
    {
        ProcessOpenRequestFromClient(clientConnectionPtr);
    }
    // If an error occurred on the receive,
    else
    {
        LE_ERROR("Failed to receive service ID from client (uid %u '%s', pid %d).",
                 clientConnectionPtr->userPtr->uid,
                 clientConnectionPtr->userPtr->name,
                 clientConnectionPtr->pid);

        // Drop the Client connection to trigger a recovery action by the client (or the
        // Supervisor, if the client dies).
        RejectClient(clientConnectionPtr);
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
    // Allocate a new Client Connection object.
    ClientConnection_t* connectionPtr = le_mem_ForceAlloc(ClientConnectionPoolRef);

    connectionPtr->link = LE_DLS_LINK_INIT;
    connectionPtr->state = CLIENT_STATE_ID_UNKNOWN;
    connectionPtr->fd = fd;
    connectionPtr->userPtr = GetUser(uid);
    connectionPtr->pid = pid;
    connectionPtr->bindingPtr = NULL;

    // Haven't received ID yet, so clear it out.
    memset(&connectionPtr->serviceId, 0, sizeof(connectionPtr->serviceId));

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

    switch (connectionPtr->state)
    {
        case CLIENT_STATE_ID_UNKNOWN:

            break;

        case CLIENT_STATE_UNBOUND:

            // Remove the connection from the user's list of unbound client connections.
            le_dls_Remove(&connectionPtr->userPtr->unboundClientsList, &connectionPtr->link);

            break;

        case CLIENT_STATE_WAITING:

            // Remove the connection from the Binding object's list of waiting clients.
            le_dls_Remove(&connectionPtr->bindingPtr->waitingClientsList, &connectionPtr->link);
            connectionPtr->bindingPtr = NULL;
            break;
    }

    // Delete the File Descriptor Monitor object.
    if (connectionPtr->fdMonitorRef != NULL)
    {
        le_event_DeleteFdMonitor(connectionPtr->fdMonitorRef);
        connectionPtr->fdMonitorRef = NULL;
    }

    // Close the socket.
    fd_Close(connectionPtr->fd);
    connectionPtr->fd = -1;

    // Release the Connection object's reference to the User object.
    le_mem_Release(connectionPtr->userPtr);
    connectionPtr->userPtr = NULL;
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

    bool alreadyReceivedServiceId = (connectionPtr->serviceId.serviceName[0] != '\0');

    // Receive the service identity from the server.
    result = ReceiveServiceId(fd, &connectionPtr->serviceId);

    // If the connection has closed or there is simply nothing left to be received
    // from the socket,
    if ((result == LE_CLOSED) || (result == LE_WOULD_BLOCK))
    {
        // We are done.
        // NOTE: If the connection closed, our hang-up handler will be called.
    }
    else if (alreadyReceivedServiceId)
    {
        // The server should only send us the service identification details once.  So, if we
        // already have the service identification details, it means we shouldn't be receiving
        // data from it.
        LE_ERROR("Server sent extra data (uid %u '%s', pid %d, service '%s').",
                 connectionPtr->userPtr->uid,
                 connectionPtr->userPtr->name,
                 connectionPtr->pid,
                 connectionPtr->serviceId.serviceName);

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
        ProcessAdvertisementFromServer(connectionPtr);
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
    // Allocate a new Server Connection object.
    ServerConnection_t* connectionPtr = le_mem_ForceAlloc(ServerConnectionPoolRef);

    connectionPtr->link = LE_DLS_LINK_INIT;
    connectionPtr->fd = fd;
    connectionPtr->userPtr = GetUser(uid);
    connectionPtr->pid = pid;

    // Haven't received ID yet, so clear it out.
    memset(&connectionPtr->serviceId, 0, sizeof(connectionPtr->serviceId));

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

    // Disassociate the Server Connection object from all Binding objects that refer to it...

    // For each user,
    le_dls_Link_t* userLinkPtr = le_dls_Peek(&UserList);

    while (userLinkPtr != NULL)
    {
        User_t* userPtr = CONTAINER_OF(userLinkPtr, User_t, link);

        // For each of the user's bindings,
        le_dls_Link_t* bindingLinkPtr = le_dls_Peek(&userPtr->bindingList);
        while (bindingLinkPtr != NULL)
        {
            Binding_t* bindingPtr = CONTAINER_OF(bindingLinkPtr, Binding_t, link);

            // If the binding is associated with the deleted server connection,
            if (connectionPtr == bindingPtr->serverConnectionPtr)
            {
                bindingPtr->serverConnectionPtr = NULL;
            }

            bindingLinkPtr = le_dls_PeekNext(&userPtr->bindingList, bindingLinkPtr);
        }

        userLinkPtr = le_dls_PeekNext(&UserList, userLinkPtr);
    }

    if (connectionPtr->serviceId.serviceName[0] == '\0')
    {
        LE_DEBUG("Server (uid %u '%s', pid %d) disconnected without ever advertising a service.",
                 connectionPtr->userPtr->uid,
                 connectionPtr->userPtr->name,
                 connectionPtr->pid);
    }
    else
    {
        LE_DEBUG("Server (uid %u '%s', pid %d) withdrew service (%s:%s).",
                 connectionPtr->userPtr->uid,
                 connectionPtr->userPtr->name,
                 connectionPtr->pid,
                 connectionPtr->serviceId.serviceName,
                 connectionPtr->serviceId.protocolId);

        // Remove the Server Connection from the User's Service List, if it has been added.
        // NOTE: If the connection is rejected because of a bad or duplicate advertisement,
        //       then the connection will not have made it into the user's list of services.
        if (le_dls_IsInList(&connectionPtr->userPtr->serviceList, &connectionPtr->link))
        {
            le_dls_Remove(&connectionPtr->userPtr->serviceList, &connectionPtr->link);
        }
    }

    // Delete the File Descriptor Monitor object.
    if (connectionPtr->fdMonitorRef != NULL)
    {
        le_event_DeleteFdMonitor(connectionPtr->fdMonitorRef);
        connectionPtr->fdMonitorRef = NULL;
    }

    // Close the socket.
    fd_Close(connectionPtr->fd);
    connectionPtr->fd = -1;

    // Release the Connection object's reference to the User object.
    le_mem_Release(connectionPtr->userPtr);
    connectionPtr->userPtr = NULL;
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
    le_dls_Remove(&bindingPtr->clientUserPtr->bindingList, &bindingPtr->link);

    // While the list of waiting clients is not empty, pop one off and process it.
    le_dls_Link_t* linkPtr;
    while (NULL != (linkPtr = le_dls_Pop(&(bindingPtr->waitingClientsList))))
    {
        ClientConnection_t* clientConnectionPtr = CONTAINER_OF(linkPtr, ClientConnection_t, link);

        clientConnectionPtr->bindingPtr = NULL;

        ProcessOpenRequestFromClient(clientConnectionPtr);
    }

    // Release the Binding's reference count on the client's User object.
    le_mem_Release(bindingPtr->clientUserPtr);
    bindingPtr->clientUserPtr = NULL;

    // Release the Binding's reference count on the server's User object.
    le_mem_Release(bindingPtr->serverUserPtr);
    bindingPtr->serverUserPtr = NULL;
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

        while (serviceLinkPtr != NULL)
        {
            ServerConnection_t* connectionPtr = CONTAINER_OF(serviceLinkPtr,
                                                             ServerConnection_t,
                                                             link);

            // Print a description of the service.
            if (strncmp(userPtr->name, "app", 3) == 0)
            {
                dprintf(fd, "        %s", userPtr->name + 3);
            }
            else
            {
                dprintf(fd, "        <%s>", userPtr->name);
            }

            dprintf(fd,
                    ".%s  (protocol ID = '%s', max message size = %zu bytes)\n",
                    connectionPtr->serviceId.serviceName,
                    connectionPtr->serviceId.protocolId,
                    connectionPtr->serviceId.maxProtocolMsgSize);

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
    // Iterate over the User List, and for each user,
    le_dls_Link_t* userLinkPtr = le_dls_Peek(&UserList);

    while (userLinkPtr != NULL)
    {
        User_t* userPtr = CONTAINER_OF(userLinkPtr, User_t, link);

        // List all the unbound client connections:
        le_dls_Link_t* clientLinkPtr = le_dls_Peek(&userPtr->unboundClientsList);
        while (clientLinkPtr != NULL)
        {
            ClientConnection_t* connectionPtr = CONTAINER_OF(clientLinkPtr,
                                                             ClientConnection_t,
                                                             link);

            if (strncmp(userPtr->name, "app", 3) == 0)
            {
                dprintf(fd,
                        "        [pid %5d] %s.%s UNBOUND  (protocol ID = '%s')\n",
                        connectionPtr->pid,
                        userPtr->name + 3,
                        connectionPtr->serviceId.serviceName,
                        connectionPtr->serviceId.protocolId);
            }
            else
            {
                dprintf(fd,
                        "        [pid %5d] <%s>.%s UNBOUND  (protocol ID = '%s')\n",
                        connectionPtr->pid,
                        userPtr->name,
                        connectionPtr->serviceId.serviceName,
                        connectionPtr->serviceId.protocolId);
            }

            clientLinkPtr = le_dls_PeekNext(&userPtr->unboundClientsList, clientLinkPtr);
        }

        // For each binding in the user's Binding List,
        le_dls_Link_t* bindingLinkPtr = le_dls_Peek(&userPtr->bindingList);

        while (bindingLinkPtr != NULL)
        {
            Binding_t* bindingPtr = CONTAINER_OF(bindingLinkPtr, Binding_t, link);

            // For each client connection on the binding's Waiting Clients List,
            clientLinkPtr = le_dls_Peek(&bindingPtr->waitingClientsList);
            while (clientLinkPtr != NULL)
            {
                ClientConnection_t* connectionPtr = CONTAINER_OF(clientLinkPtr,
                                                                 ClientConnection_t,
                                                                 link);

                // Print a description of the waiting connection and what it is waiting for.
                if (strncmp(userPtr->name, "app", 3) == 0)
                {
                    dprintf(fd, "        [pid %5d] %s", connectionPtr->pid, userPtr->name + 3);
                }
                else
                {
                    dprintf(fd, "        [pid %5d] <%s>", connectionPtr->pid, userPtr->name);
                }

                dprintf(fd, ".%s WAITING for ", connectionPtr->serviceId.serviceName);

                if (strncmp(bindingPtr->serverUserPtr->name, "app", 3) == 0)
                {
                    dprintf(fd, "%s", bindingPtr->serverUserPtr->name + 3);
                }
                else
                {
                    dprintf(fd, "<%s>", bindingPtr->serverUserPtr->name);
                }

                dprintf(fd,
                        ".%s  (protocol ID = '%s')\n",
                        bindingPtr->serverServiceName,
                        connectionPtr->serviceId.protocolId);

                clientLinkPtr = le_dls_PeekNext(&bindingPtr->waitingClientsList, clientLinkPtr);
            }

            bindingLinkPtr = le_dls_PeekNext(&userPtr->bindingList, bindingLinkPtr);
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

        while (bindingLinkPtr != NULL)
        {
            Binding_t* bindingPtr = CONTAINER_OF(bindingLinkPtr, Binding_t, link);

            // Print the binding to the provided file descriptor.
            // Use the same format as would be seen in a .sdef file.
            if (strncmp(userPtr->name, "app", 3) == 0)
            {
                dprintf(fd, "        %s", userPtr->name + 3);
            }
            else
            {
                dprintf(fd, "        <%s>", userPtr->name);
            }

            dprintf(fd, ".%s -> ", bindingPtr->clientServiceName);

            if (strncmp(bindingPtr->serverUserPtr->name, "app", 3) == 0)
            {
                dprintf(fd, "%s", bindingPtr->serverUserPtr->name + 3);
            }
            else
            {
                dprintf(fd, "<%s>", bindingPtr->serverUserPtr->name);
            }

            dprintf(fd, ".%s\n", bindingPtr->serverServiceName);

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
        dprintf(fd, "\nBINDINGS\n\n");

        SdirToolListBindings(fd);

        dprintf(fd, "\nSERVICES\n\n");

        SdirToolListServices(fd);

        dprintf(fd, "\nWAITING CLIENTS\n\n");

        SdirToolListWaitingClients(fd);

        dprintf(fd, "\n");

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

    // Re-create built-in, hard-coded bindings.
    CreateHardCodedBindings();
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
            CreateBinding(msgPtr->client,
                          msgPtr->clientServiceName,
                          msgPtr->server,
                          msgPtr->serverServiceName);
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
    ClientConnectionPoolRef = le_mem_CreatePool("Client Connection", sizeof(ClientConnection_t));
    ServerConnectionPoolRef = le_mem_CreatePool("Server Connection", sizeof(ServerConnection_t));
    UserPoolRef = le_mem_CreatePool("User", sizeof(User_t));
    BindingPoolRef = le_mem_CreatePool("Binding", sizeof(Binding_t));

    /// Expand the pools to their expected maximum sizes.
    /// @todo Make this configurable.
    le_mem_ExpandPool(ClientConnectionPoolRef, 100);
    le_mem_ExpandPool(ServerConnectionPoolRef, 30);
    le_mem_ExpandPool(UserPoolRef, 30);
    le_mem_ExpandPool(BindingPoolRef, 30);

    // Register destructor functions.
    le_mem_SetDestructor(ClientConnectionPoolRef, ClientConnectionDestructor);
    le_mem_SetDestructor(ServerConnectionPoolRef, ServerConnectionDestructor);
    le_mem_SetDestructor(UserPoolRef, UserDestructor);
    le_mem_SetDestructor(BindingPoolRef, BindingDestructor);

    // Create built-in, hard-coded bindings.
    CreateHardCodedBindings();

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
