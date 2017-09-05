/** @file messagingInterface.c
 *
 * Implements the "Interface" objects and the "Interface List" in the Legato @ref c_messaging.
 *
 * See @ref messaging.c for the overall subsystem design.
 *
 * @warning The code in this file @b must be thread safe and re-entrant.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "unixSocket.h"
#include "serviceDirectory/serviceDirectoryProtocol.h"
#include "messagingInterface.h"
#include "messagingSession.h"
#include "fileDescriptor.h"


// =======================================
//  PRIVATE DATA
// =======================================

/// Maximum size of a service instance name, in bytes, including the null-terminator.
#define MAX_SERVICE_NAME_BYTES  64

/// Highest number of Services that are expected to be referred to (served up or used) in a
/// single process.
#define MAX_EXPECTED_SERVICES   32

/// Highest number of Client Interfaces that are expected to be referred to in a single process.
#define MAX_EXPECTED_CLIENT_INTERFACES    32

//--------------------------------------------------------------------------------------------------
/**
 * Hashmap in which Service objects are kept.
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t ServiceMapRef;

//--------------------------------------------------------------------------------------------------
/**
 * Hashmap in which Client Interface objects are kept.
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t ClientInterfaceMapRef;

//--------------------------------------------------------------------------------------------------
/**
 * A counter that increments every time a change is made to ServiceMapRef.
 */
//--------------------------------------------------------------------------------------------------
static size_t ServiceObjMapChangeCount = 0;
static size_t* ServiceObjMapChangeCountRef = &ServiceObjMapChangeCount;

//--------------------------------------------------------------------------------------------------
/**
 * A counter that increments every time a change is made to ClientInterfaceMapRef.
 */
//--------------------------------------------------------------------------------------------------
static size_t ClientInterfaceMapChangeCount = 0;
static size_t* ClientInterfaceMapChangeCountRef = &ClientInterfaceMapChangeCount;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for the handlers reference
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t HandlersRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Pool from which Service objects are allocated.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ServicePoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Pool from which Client Interface objects are allocated.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ClientInterfacePoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Pool from which session event handler object are allocated.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  HandlerEventPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Mutex used to protect data structures in this module from multi-threaded race conditions.
 *
 * @note This is a pthreads FAST mutex, chosen to minimize overhead.  It is non-recursive.
 */
//--------------------------------------------------------------------------------------------------
static pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK    LE_ASSERT(pthread_mutex_lock(&Mutex) == 0);
#define UNLOCK  LE_ASSERT(pthread_mutex_unlock(&Mutex) == 0);


//--------------------------------------------------------------------------------------------------
/**
 * Key used to identify thread-local data record containing the Message object reference for the
 * message currently being processed by a Service's message receive handler; or NULL if the thread
 * is not currently running a Service's message receive handler.
 **/
//--------------------------------------------------------------------------------------------------
static pthread_key_t ThreadLocalRxMsgKey;


//--------------------------------------------------------------------------------------------------
/**
 * session event handler object
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_msg_SessionEventHandler
{
    le_msg_SessionEventHandler_t    handler;    ///< Handler function for when sessions close.
    void*                           contextPtr; ///< ContextPtr parameter for closeHandler.
    le_dls_List_t*                  listPtr;    ///< List containing the current node
    le_msg_SessionEventHandlerRef_t ref;        ///< Handler safe reference.
    le_dls_Link_t                   link;       ///< Node link
} SessionEventHandler_t;


// =======================================
//  PRIVATE FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Key hash function for the hashmaps of interface instances.
 *
 * @return  The hash value for a unique session ID (the key).
 */
//--------------------------------------------------------------------------------------------------
static size_t ComputeInterfaceIdHash
(
    const void* keyPtr
)
//--------------------------------------------------------------------------------------------------
{
    const msgInterface_Id_t* idPtr = keyPtr;

    // NOTE: The protocol IDs are likely to be much longer than the interface instance names,
    //       and we don't expect there to actually be very many interfaces referenced in the
    //       same process, so a collision here and there isn't a big deal.  So, we just use
    //       the interface instance name to compute the hash of the key to save some cycles.

    return le_hashmap_HashString(idPtr->name);
}


//--------------------------------------------------------------------------------------------------
/**
 * Key equality comparison function for the hashmaps of interface instances.
 */
//--------------------------------------------------------------------------------------------------
static bool AreInterfaceIdsTheSame
(
    const void* firstKeyPtr,
    const void* secondKeyPtr
)
//--------------------------------------------------------------------------------------------------
{
    const msgInterface_Id_t* firstIdPtr = firstKeyPtr;
    const msgInterface_Id_t* secondIdPtr = secondKeyPtr;

    return (   le_hashmap_EqualsString(firstIdPtr->name, secondIdPtr->name)
            && le_hashmap_EqualsString(le_msg_GetProtocolIdStr(firstIdPtr->protocolRef),
                                       le_msg_GetProtocolIdStr(secondIdPtr->protocolRef)) );
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize an Interface object.
 */
//--------------------------------------------------------------------------------------------------
static void InitInterface
(
    le_msg_ProtocolRef_t    protocolRef,   ///< [in] Protocol to initialize the interface obj.
    const char*             interfaceName, ///< [in] Interface name to initialize the interface obj.
    msgInterface_Type_t interfaceType,     ///< [in] Interface type to init the interface obj.
    msgInterface_Interface_t* interfacePtr ///< [out] Ptr to the interface object to be initialized.
)
{
    interfacePtr->interfaceType = interfaceType;
    interfacePtr->id.protocolRef = protocolRef;
    le_result_t result = le_utf8_Copy(interfacePtr->id.name,
                                      interfaceName,
                                      sizeof(interfacePtr->id.name),
                                      NULL);
    LE_FATAL_IF(result != LE_OK,
                "Service ID '%s' too long (should only be %zu bytes total).",
                interfaceName,
                sizeof(interfacePtr->id.name));

    interfacePtr->sessionList = LE_DLS_LIST_INIT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new Service object.
 *
 * @return  A pointer to the object.
 *
 * @warning Assumes that the Mutex is locked.
 */
//--------------------------------------------------------------------------------------------------
static msgInterface_Service_t* CreateService
(
    le_msg_ProtocolRef_t    protocolRef,
    const char*             interfaceName
)
//--------------------------------------------------------------------------------------------------
{
    msgInterface_Service_t* servicePtr = le_mem_ForceAlloc(ServicePoolRef);

    InitInterface(protocolRef, interfaceName, LE_MSG_INTERFACE_SERVER,
                  (msgInterface_Interface_t*)servicePtr);

    servicePtr->state = LE_MSG_INTERFACE_SERVICE_HIDDEN;

    servicePtr->directorySocketFd = -1;
    servicePtr->fdMonitorRef = NULL;
    servicePtr->serverThread = NULL;    // NULL indicates no server in this process.

    servicePtr->recvHandler = NULL;
    servicePtr->recvContextPtr = NULL;

    // Initialize the close handlers dls
    servicePtr->closeListPtr = LE_DLS_LIST_INIT;

    // Initialize the open handlers dls
    servicePtr->openListPtr = LE_DLS_LIST_INIT;

    ServiceObjMapChangeCount++;
    le_hashmap_Put(ServiceMapRef, &servicePtr->interface.id, servicePtr);

    return servicePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new Client Interface object.
 *
 * @return  A pointer to the object.
 *
 * @warning Assumes that the Mutex is locked.
 */
//--------------------------------------------------------------------------------------------------
static msgInterface_ClientInterface_t* CreateClientInterface
(
    le_msg_ProtocolRef_t    protocolRef,
    const char*             interfaceName
)
//--------------------------------------------------------------------------------------------------
{
    msgInterface_ClientInterface_t* clientPtr = le_mem_ForceAlloc(ClientInterfacePoolRef);

    InitInterface(protocolRef,
                  interfaceName,
                  LE_MSG_INTERFACE_CLIENT,
                  (msgInterface_Interface_t*)clientPtr);

    ClientInterfaceMapChangeCount++;
    le_hashmap_Put(ClientInterfaceMapRef, &clientPtr->interface.id, clientPtr);

    return clientPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a Service object matching a given service identification.  Must be released using
 * msgInterface_Release() when you are done with it.
 *
 * @return  Pointer to the service object.
 *
 * @note    Creates an object if one doesn't already exist, so always returns a valid pointer.
 *
 * @warning Assumes that the Mutex is locked.
 */
//--------------------------------------------------------------------------------------------------
static msgInterface_Service_t* GetService
(
    le_msg_ProtocolRef_t    protocolRef,
    const char*             interfaceName
)
//--------------------------------------------------------------------------------------------------
{
    msgInterface_Id_t id;

    id.protocolRef = protocolRef;
    LE_FATAL_IF(le_utf8_Copy(id.name, interfaceName, sizeof(id.name), NULL) == LE_OVERFLOW,
                "Service ID '%s' too long (should only be %zu bytes total).",
                interfaceName,
                sizeof(id.name));

    msgInterface_Service_t* servicePtr = le_hashmap_Get(ServiceMapRef, &id);
    if (servicePtr == NULL)
    {
        servicePtr = CreateService(protocolRef, interfaceName);
    }
    else
    {
        le_mem_AddRef(servicePtr);
    }

    return servicePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a Client Interface object matching a given client interface name.  Must be released using
 * msgInterface_Release() when you are done with it.
 *
 * @return  Pointer to the Client Interface object.
 *
 * @note    Creates an object if one doesn't already exist, so always returns a valid pointer.
 *
 * @warning Assumes that the Mutex is locked.
 */
//--------------------------------------------------------------------------------------------------
static msgInterface_ClientInterface_t* GetClient
(
    le_msg_ProtocolRef_t    protocolRef,
    const char*             interfaceName
)
//--------------------------------------------------------------------------------------------------
{
    // Create an ID structure for this interface.
    msgInterface_Id_t id;

    id.protocolRef = protocolRef;

    LE_FATAL_IF(le_utf8_Copy(id.name, interfaceName, sizeof(id.name), NULL) == LE_OVERFLOW,
                "Service ID '%s' too long (should only be %zu bytes total).",
                interfaceName,
                sizeof(id.name));

    // Look up the ID in the client hash map to see if a client already exists for this interface.
    msgInterface_ClientInterface_t* clientPtr = le_hashmap_Get(ClientInterfaceMapRef, &id);

    if (clientPtr == NULL)
    {
        clientPtr = CreateClientInterface(protocolRef, interfaceName);
    }
    else
    {
        le_mem_AddRef(clientPtr);
    }

    return clientPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Destructor function that runs when a Service object is about to be returned back to the
 * Service Pool.
 *
 * @warning Assumes that the Mutex is locked, therefore the Mutex must be locked during all
 *          calls to le_mem_Release() for Service objects.
 */
//--------------------------------------------------------------------------------------------------
static void ServiceDestructor
(
    void* objPtr
)
//--------------------------------------------------------------------------------------------------
{
    msgInterface_Service_t* servicePtr = objPtr;

    ServiceObjMapChangeCount++;
    le_hashmap_Remove(ServiceMapRef, &servicePtr->interface.id);

    // Release the close handlers
    le_dls_Link_t* linkPtr;
    while ((linkPtr = le_dls_PopTail(&servicePtr->closeListPtr)) != NULL)
    {
        SessionEventHandler_t* closeEventPtr = CONTAINER_OF(linkPtr, SessionEventHandler_t, link);

        le_ref_DeleteRef(HandlersRefMap, closeEventPtr->ref);

        le_mem_Release(closeEventPtr);
    }

    // Release the open handlers
    while ((linkPtr = le_dls_PopTail(&servicePtr->openListPtr)) != NULL)
    {
        SessionEventHandler_t* openEventPtr = CONTAINER_OF(linkPtr, SessionEventHandler_t, link);

        le_ref_DeleteRef(HandlersRefMap, openEventPtr->ref);

        le_mem_Release(openEventPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Destructor function that runs when a Client Interface object is about to be returned back to the
 * Client Interface Pool.
 *
 * @warning Assumes that the Mutex is locked, therefore the Mutex must be locked during all
 *          calls to le_mem_Release() for Client Interface objects.
 */
//--------------------------------------------------------------------------------------------------
static void ClientInterfaceDestructor
(
    void* objPtr
)
//--------------------------------------------------------------------------------------------------
{
    msgInterface_ClientInterface_t* clientPtr = objPtr;

    ClientInterfaceMapChangeCount++;
    le_hashmap_Remove(ClientInterfaceMapRef, &clientPtr->interface.id);
}


//--------------------------------------------------------------------------------------------------
/**
 * Calls a Service's server's "open" handler if there is one registered.
 *
 * @note    This only gets called by the server thread for the service.
 */
//--------------------------------------------------------------------------------------------------
static void CallOpenHandler
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionRef_t sessionRef
)
//--------------------------------------------------------------------------------------------------
 {
    // If there is a Close Handler registered, call it now.
    le_dls_Link_t* openLinkPtr = le_dls_Peek(&serviceRef->openListPtr);

    while (openLinkPtr)
    {
        SessionEventHandler_t* openEventPtr = CONTAINER_OF(openLinkPtr, SessionEventHandler_t, link);

        if (openEventPtr->handler != NULL)
        {
            openEventPtr->handler(sessionRef, openEventPtr->contextPtr);
        }

        openLinkPtr = le_dls_PeekNext(&serviceRef->openListPtr, openLinkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Event handler function called when a Service's directorySocketFd becomes writeable.
 *
 * This only happens when the Service is in the CONNECTING state and the connection to the
 * Service Directory is established or fails to be established.  After that, we disable
 * writeability notification.
 */
//--------------------------------------------------------------------------------------------------
static void DirectorySocketWriteable
(
    msgInterface_Service_t* servicePtr
)
//--------------------------------------------------------------------------------------------------
{
    if (servicePtr->state == LE_MSG_INTERFACE_SERVICE_CONNECTING)
    {
        // Must have connected (or failed to do so).
        int errCode = unixSocket_GetErrorState(servicePtr->directorySocketFd);

        // Disable writeability notification.
        le_fdMonitor_Disable(servicePtr->fdMonitorRef, POLLOUT);

        // If connection successful,
        if (errCode == 0)
        {
            // Send the Interface ID to the Service Directory.
            svcdir_InterfaceDetails_t msg;
            msgInterface_GetInterfaceDetails(&(servicePtr->interface), &msg);
            le_result_t result = unixSocket_SendDataMsg(servicePtr->directorySocketFd,
                                                        &msg,
                                                        sizeof(msg));
            if (result != LE_OK)
            {
                // NOTE: This is only done when the socket is newly opened, so this shouldn't ever
                //       be LE_NO_MEMORY (send buffers full).
                LE_FATAL("Failed to send service advertisement to the Service Directory."
                         " Result = %d (%s)",
                         result,
                         LE_RESULT_TXT(result));
            }

            servicePtr->state = LE_MSG_INTERFACE_SERVICE_ADVERTISED;

            // Wait for the Service Directory to respond by either dropping the connection
            // (meaning that we have been denied permission to offer this service) or by
            // forwarding us file descriptors for authenticated client connections.
        }
        // If connection failed,
        else
        {
            LE_FATAL("Failed to connect to Service Directory. SO_ERROR %d (%s).",
                     errCode,
                     strerror(errCode));
        }
    }
    else
    {
        LE_CRIT("Unexpected writeability notification in state %d.", servicePtr->state);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Event handler function called when a Service's directorySocketFd becomes readable.
 *
 * This means that the Service Directory has sent us the file descriptor of an authenticated
 * client connection socket.
 */
//--------------------------------------------------------------------------------------------------
static void DirectorySocketReadable
(
    msgInterface_Service_t* servicePtr
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;

    int clientSocketFd;

    // Receive the Client connection file descriptor from the Service Directory.
    result = unixSocket_ReceiveMsg(servicePtr->directorySocketFd,
                                   NULL,   // dataBuffPtr
                                   0,      // dataBuffSize
                                   &clientSocketFd,
                                   NULL);  // credPtr
    if (result == LE_CLOSED)
    {
        LE_DEBUG("Connection has closed.");
    }
    else if (result != LE_OK)
    {
        LE_FATAL("Failed to receive client fd from Service Directory (%d: %s).",
                 result,
                 LE_RESULT_TXT(result));
    }
    else if (clientSocketFd < 0)
    {
        LE_ERROR("Received something other than a file descriptor from Service Directory for (%s:%s).",
                 servicePtr->interface.id.name,
                 le_msg_GetProtocolIdStr(servicePtr->interface.id.protocolRef));
    }
    // This should never happen before we have sent our advertisement to the Service Directory.
    else if (servicePtr->state == LE_MSG_INTERFACE_SERVICE_CONNECTING)
    {
        LE_FATAL("Received fd from Service Directory before advertisement sent for (%s:%s).",
                 servicePtr->interface.id.name,
                 le_msg_GetProtocolIdStr(servicePtr->interface.id.protocolRef));
    }
    else
    {
        // Create a server-side Session object for that connection to this Service.
        le_msg_SessionRef_t sessionRef = msgSession_CreateServerSideSession(servicePtr,
                                                                            clientSocketFd);

        // If successful, call the registered "open" handler, if there is one.
        if (sessionRef != NULL)
        {
            CallOpenHandler(servicePtr, sessionRef);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Event handler function called when a Service's directorySocketFd closes.
 *
 * This means that the Service Directory has denied us permission to advertise a service.
 *
 * @note    It could also mean that the Service Directory has crashed or been shut down, but
 *          that is much less likely to be the cause, and the effect is the same anyway.
 */
//--------------------------------------------------------------------------------------------------
static void DirectorySocketClosed
(
    msgInterface_Service_t* servicePtr
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL("Connection to Service Directory lost for service (%s:%s).",
             servicePtr->interface.id.name,
             le_msg_GetProtocolIdStr(servicePtr->interface.id.protocolRef));
}


//--------------------------------------------------------------------------------------------------
/**
 * Event handler function called when a Service's directorySocketFd experiences an error.
 *
 * This means that we have failed to communicate with the Service Directory.
 */
//--------------------------------------------------------------------------------------------------
static void DirectorySocketError
(
    msgInterface_Service_t* servicePtr
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL("Error on Service Directory connection for service (%s:%s).",
             servicePtr->interface.id.name,
             le_msg_GetProtocolIdStr(servicePtr->interface.id.protocolRef));
}


//--------------------------------------------------------------------------------------------------
/**
 * Handles events detected on the file descriptor for the socket connection to the
 * Service Directory.
 **/
//--------------------------------------------------------------------------------------------------
static void DirectorySocketEventHandler
(
    int     fd,
    short   events
)
//--------------------------------------------------------------------------------------------------
{
    msgInterface_Service_t* servicePtr = le_fdMonitor_GetContextPtr();

    LE_ASSERT(fd == servicePtr->directorySocketFd);

    if (events & (POLLHUP | POLLRDHUP))
    {
        DirectorySocketClosed(servicePtr);
    }
    else if (events & POLLERR)
    {
        DirectorySocketError(servicePtr);
    }
    else
    {
        if (events & POLLIN)
        {
            DirectorySocketReadable(servicePtr);
        }
        if (events & POLLOUT)
        {
            DirectorySocketWriteable(servicePtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Start monitoring for readable, hang-up, and error events on a given Service's "Directory Socket"
 * (the socket connected to the Service Directory).
 */
//--------------------------------------------------------------------------------------------------
static void StartMonitoringDirectorySocket
(
    msgInterface_Service_t*  servicePtr
)
//--------------------------------------------------------------------------------------------------
{
    char name[LIMIT_MAX_MEM_POOL_NAME_BYTES];
    char* destPtr = name;
    size_t spaceLeft = sizeof(name);
    size_t bytesCopied;
    le_utf8_Copy(destPtr, servicePtr->interface.id.name, spaceLeft, &bytesCopied);
    destPtr += bytesCopied;
    spaceLeft -= bytesCopied;
    le_utf8_Copy(destPtr, ":", spaceLeft, &bytesCopied);
    destPtr += bytesCopied;
    spaceLeft -= bytesCopied;
    le_utf8_Copy(destPtr, le_msg_GetProtocolIdStr(servicePtr->interface.id.protocolRef), spaceLeft,
                 NULL);

    servicePtr->fdMonitorRef = le_fdMonitor_Create(name,
                                                   servicePtr->directorySocketFd,
                                                   DirectorySocketEventHandler,
                                                   POLLOUT | POLLIN);

    le_fdMonitor_SetContextPtr(servicePtr->fdMonitorRef, servicePtr);

}


//--------------------------------------------------------------------------------------------------
/**
 * Close all sessions on a given Service object's list of open sessions.
 */
//--------------------------------------------------------------------------------------------------
static void CloseAllSessions
(
    msgInterface_Service_t* servicePtr
)
//--------------------------------------------------------------------------------------------------
{
    LOCK

    for (;;)
    {
        le_dls_Link_t* linkPtr = le_dls_Peek(&servicePtr->interface.sessionList);

        if (linkPtr != NULL)
        {
            le_msg_DeleteSession(msgSession_GetSessionContainingLink(linkPtr));
        }
        else
        {
            break;
        }
    }

    UNLOCK
}


// =======================================
//  PROTECTED (INTER-MODULE) FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Exposing the service object map; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
le_hashmap_Ref_t* msgInterface_GetServiceObjMap
(
    void
)
{
    return (&ServiceMapRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Exposing the service object map change counter; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
size_t** msgInterface_GetServiceObjMapChgCntRef
(
    void
)
{
    return (&ServiceObjMapChangeCountRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Exposing the client interface map; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
le_hashmap_Ref_t* msgInterface_GetClientInterfaceMap
(
    void
)
{
    return (&ClientInterfaceMapRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Exposing the client interface map change counter; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
size_t** msgInterface_GetClientInterfaceMapChgCntRef
(
    void
)
{
    return (&ClientInterfaceMapChangeCountRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the module.  This must be called only once at start-up, before
 * any other functions in that module are called.
 */
//--------------------------------------------------------------------------------------------------
void msgInterface_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Create and initialize the pool of Service objects.
    ServicePoolRef = le_mem_CreatePool("MessagingServices", sizeof(msgInterface_Service_t));
    le_mem_ExpandPool(ServicePoolRef, MAX_EXPECTED_SERVICES);
    le_mem_SetDestructor(ServicePoolRef, ServiceDestructor);

    // Create and initialize the pool of Client Interface objects.
    ClientInterfacePoolRef = le_mem_CreatePool("MessagingClientInterfaces",
                                               sizeof(msgInterface_ClientInterface_t));
    le_mem_ExpandPool(ClientInterfacePoolRef, MAX_EXPECTED_CLIENT_INTERFACES );
    le_mem_SetDestructor(ClientInterfacePoolRef, ClientInterfaceDestructor);

    // Create and initialize the pool of event handlers objects.
    HandlerEventPoolRef = le_mem_CreatePool("HandlerEventPool", sizeof(SessionEventHandler_t));
    le_mem_ExpandPool(HandlerEventPoolRef, MAX_EXPECTED_SERVICES*6);

    // Create safe reference map for add references.
    HandlersRefMap = le_ref_CreateMap("HandlersRef", MAX_EXPECTED_SERVICES*6);

    // Create the Service Map.
    ServiceMapRef = le_hashmap_Create("MessagingServices",
                                      MAX_EXPECTED_SERVICES,
                                      ComputeInterfaceIdHash,
                                      AreInterfaceIdsTheSame);

    // Create the Client Map.
    ClientInterfaceMapRef = le_hashmap_Create("MessagingClients",
                                              MAX_EXPECTED_CLIENT_INTERFACES,
                                              ComputeInterfaceIdHash,
                                              AreInterfaceIdsTheSame);

    // Create the key to be used to identify thread-local data records containing the Message
    // Reference when running a Service's message receive handler.
    int result = pthread_key_create(&ThreadLocalRxMsgKey, NULL);
    if (result != 0)
    {
        LE_FATAL("Failed to create thread local key: result = %d (%s).", result, strerror(result));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to a Client Interface object.  Must be released using msgInterface_Release() when
 * you are done with it.
 *
 * @return  Reference to the client interface object.
 */
//--------------------------------------------------------------------------------------------------
le_msg_ClientInterfaceRef_t msgInterface_GetClient
(
    le_msg_ProtocolRef_t    protocolRef,
    const char*             interfaceName
)
//--------------------------------------------------------------------------------------------------
{
    msgInterface_ClientInterface_t* clientPtr;

    LOCK
    clientPtr = GetClient(protocolRef, interfaceName);
    UNLOCK

    return clientPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the interface details for a given interface object.
 */
//--------------------------------------------------------------------------------------------------
void msgInterface_GetInterfaceDetails
(
    le_msg_InterfaceRef_t interfaceRef,     ///< [in] Pointer to the interface.
    svcdir_InterfaceDetails_t* detailsPtr   ///< [out] Ptr to where the details will be stored.
)
//--------------------------------------------------------------------------------------------------
{
    memset(detailsPtr, 0, sizeof(*detailsPtr));

    detailsPtr->maxProtocolMsgSize = le_msg_GetProtocolMaxMsgSize(interfaceRef->id.protocolRef);

    le_utf8_Copy(detailsPtr->protocolId,
                 le_msg_GetProtocolIdStr(interfaceRef->id.protocolRef),
                 sizeof(detailsPtr->protocolId),
                 NULL);

    le_utf8_Copy(detailsPtr->interfaceName,
                 interfaceRef->id.name,
                 sizeof(detailsPtr->interfaceName),
                 NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Release a reference to an Interface. Note that this can also be (and is meant to be) used with
 * specific interface references, such as Services and Client Interfaces. Remember to cast the
 * specific interface refrences to the more generic le_msg_InterfaceRef_t.
 */
//--------------------------------------------------------------------------------------------------
void msgInterface_Release
(
    le_msg_InterfaceRef_t interfaceRef
)
{
    // NOTE: Must lock the mutex before releasing in case the destructor runs, because
    //       the destructor manipulates structures that are shared by all threads in the
    //       process.

    LOCK

    le_mem_Release(interfaceRef);

    UNLOCK
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a Session to an Interface's list of open sessions.
 */
//--------------------------------------------------------------------------------------------------
void msgInterface_AddSession
(
    le_msg_InterfaceRef_t interfaceRef,
    le_msg_SessionRef_t sessionRef
)
//--------------------------------------------------------------------------------------------------
{
    // The Session object holds a reference to the Interface object.
    le_mem_AddRef(interfaceRef);

    LOCK
    le_dls_Queue(&interfaceRef->sessionList, msgSession_GetListLink(sessionRef));
    UNLOCK
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove a Session from an Interface's list of open sessions.
 */
//--------------------------------------------------------------------------------------------------
void msgInterface_RemoveSession
(
    le_msg_InterfaceRef_t interfaceRef,
    le_msg_SessionRef_t sessionRef
)
//--------------------------------------------------------------------------------------------------
{
    LOCK
    le_dls_Remove(&interfaceRef->sessionList, msgSession_GetListLink(sessionRef));
    UNLOCK

    // The Session object no longer holds a reference to the Interface object.
    msgInterface_Release(interfaceRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Call a Service's registered session close handler function, if there is one registered.
 */
//--------------------------------------------------------------------------------------------------
void msgInterface_CallCloseHandler
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionRef_t sessionRef
)
//--------------------------------------------------------------------------------------------------
{
    // If there is a Close Handler registered, call it now.
    le_dls_Link_t* closeLinkPtr = le_dls_Peek(&serviceRef->closeListPtr);

    while (closeLinkPtr)
    {
        SessionEventHandler_t* closeEventPtr = CONTAINER_OF(closeLinkPtr, SessionEventHandler_t, link);

        if (closeEventPtr && (closeEventPtr->handler != NULL))
        {
            closeEventPtr->handler(sessionRef, closeEventPtr->contextPtr);
        }

        closeLinkPtr = le_dls_PeekNext(&serviceRef->closeListPtr, closeLinkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Dispatches a message received from a client to a service's server.
 */
//--------------------------------------------------------------------------------------------------
void msgInterface_ProcessMessageFromClient
(
    le_msg_ServiceRef_t serviceRef, ///< [IN] Reference to the Service object.
    le_msg_MessageRef_t msgRef      ///< [IN] Message reference for the received message.
)
//--------------------------------------------------------------------------------------------------
{
    // Pass the message to the server's registered receive handler, if there is one.
    if (serviceRef->recvHandler != NULL)
    {
        // Set the thread-local received message reference so it can be retrieved by the handler.
        pthread_setspecific(ThreadLocalRxMsgKey, msgRef);

        // Call the handler function.
        serviceRef->recvHandler(msgRef, serviceRef->recvContextPtr);

        // Clear the thread-local reference.
        pthread_setspecific(ThreadLocalRxMsgKey, NULL);
    }
    // Discard the message if no handler is registered.
    else
    {
        LE_WARN("No service receive handler (%s:%s). Discarding message. Closing session.",
                serviceRef->interface.id.name,
                le_msg_GetProtocolIdStr(serviceRef->interface.id.protocolRef));
        le_msg_DeleteSession(le_msg_GetSession(msgRef));
        le_msg_ReleaseMsg(msgRef);
    }
}


// =======================================
//  PUBLIC API FUNCTIONS
// =======================================


//--------------------------------------------------------------------------------------------------
/**
 * Creates a service that is accessible using a given protocol.
 *
 * @return  The service reference.
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_msg_CreateService
(
    le_msg_ProtocolRef_t    protocolRef,    ///< [in] Reference to the protocol to be used.
    const char*             interfaceName   ///< [in] Server-side interface name.
)
//--------------------------------------------------------------------------------------------------
{
    // Must lock the mutex to prevent races between different threads trying to offer the
    // same service at the same time, or one thread trying to delete a service while another
    // tries to create it, or accessing the Service List hashmap while another thread
    // is updating it.

    LOCK

    // Get a Service object.
    msgInterface_Service_t* servicePtr = GetService(protocolRef, interfaceName);

    // If the Service object already has a server thread, then it means that this service
    // is already being offered by someone else in this very process.
    LE_FATAL_IF(servicePtr->serverThread != NULL,
                "Duplicate service (%s:%s) offered in same process.",
                interfaceName,
                le_msg_GetProtocolIdStr(protocolRef));

    servicePtr->serverThread = le_thread_GetCurrent();

    UNLOCK

    return servicePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a service.  Any open sessions will be terminated.
 *
 * @note    This is a server-only function that can only be called by the service's server thread.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_DeleteService
(
    le_msg_ServiceRef_t             serviceRef  ///< [in] Reference to the service.
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(serviceRef->serverThread != le_thread_GetCurrent(),
                "Attempted to delete service (%s:%s) not owned by thread.",
                serviceRef->interface.id.name,
                le_msg_GetProtocolIdStr(serviceRef->interface.id.protocolRef));

    // If the service is still advertised, hide it.
    le_msg_HideService(serviceRef);

    // Close any remaining open sessions.
    CloseAllSessions(serviceRef);

    // NOTE: Lock the mutex here to prevent a race between this thread dropping ownership
    // of the service and another thread trying to offer the same service.  This is very
    // unlikely to ever happen, but just in case, make sure it fails with a sensible
    // ("duplicate") log message, instead of just quietly messing up the hashmap or something.
    LOCK

    // Clear out the server thread reference.
    serviceRef->serverThread = NULL;

    // Release the server's hold on the object.
    le_mem_Release(serviceRef);

    UNLOCK
}


//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called when clients open sessions with this service.
 *
 * @note    This is a server-only function.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t le_msg_AddServiceOpenHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(serviceRef == NULL,
                "Service doesn't exist. Make sure service is started before setting handlers");
    LE_FATAL_IF(serviceRef->serverThread != le_thread_GetCurrent(),
                "Service (%s:%s) not owned by calling thread.",
                serviceRef->interface.id.name,
                le_msg_GetProtocolIdStr(serviceRef->interface.id.protocolRef));

    // Create the node.
    SessionEventHandler_t* openEventPtr = le_mem_ForceAlloc(HandlerEventPoolRef);

    // Initialize the node
    openEventPtr->handler = handlerFunc;
    openEventPtr->contextPtr = contextPtr;
    openEventPtr->link = LE_DLS_LINK_INIT;
    openEventPtr->listPtr = &serviceRef->openListPtr;

    // Add the node to the head of the list by passing in the node's link.
    le_dls_Stack(&serviceRef->openListPtr, &openEventPtr->link);

    // Need to return a unique reference that will be used by the remove function.
    openEventPtr->ref = le_ref_CreateRef(HandlersRefMap, &openEventPtr->link);

    return openEventPtr->ref;

}

//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called whenever one of this service's sessions is closed by
 * the client.
 *
 * @note    This is a server-only function.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t le_msg_AddServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(serviceRef == NULL,
                "Service doesn't exist. Make sure service is started before setting handlers");
    LE_FATAL_IF(serviceRef->serverThread != le_thread_GetCurrent(),
                "Service (%s:%s) not owned by calling thread.",
                serviceRef->interface.id.name,
                le_msg_GetProtocolIdStr(serviceRef->interface.id.protocolRef));

    // Create the node.
    SessionEventHandler_t* closeEventPtr = le_mem_ForceAlloc(HandlerEventPoolRef);

    // Initialize the node.
    closeEventPtr->handler = handlerFunc;
    closeEventPtr->contextPtr = contextPtr;
    closeEventPtr->link = LE_DLS_LINK_INIT;
    closeEventPtr->listPtr = &serviceRef->closeListPtr;

    // Add the node to the head of the list by passing in the node's link.
    le_dls_Stack(&serviceRef->closeListPtr, &closeEventPtr->link);

    // Need to return a unique reference that will be used by the remove function.
    closeEventPtr->ref = le_ref_CreateRef(HandlersRefMap, &closeEventPtr->link);

    return closeEventPtr->ref;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove a function previously registered by le_msg_AddServiceOpenHandler or
 * le_msg_AddServiceCloseHandler.
 *
 * @note    This is a server-only function.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_RemoveServiceHandler
(
    le_msg_SessionEventHandlerRef_t handlerRef   ///< [in] Reference to a previously call of
                                                 ///       le_msg_AddServiceCloseHandler()
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* linkPtr = le_ref_Lookup(HandlersRefMap, handlerRef);

    if ( linkPtr == NULL )
    {
        LE_ERROR("Invalid data request reference");
    }
    else
    {
        SessionEventHandler_t* eventPtr = CONTAINER_OF(linkPtr, SessionEventHandler_t, link);

        // Remove from the close handler list
        le_dls_Remove(eventPtr->listPtr, linkPtr);

        le_mem_Release(eventPtr);

        le_ref_DeleteRef(HandlersRefMap, handlerRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called when messages are received from clients via sessions
 * that they have open with this service.
 *
 * @note    This is a server-only function.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_SetServiceRecvHandler
(
    le_msg_ServiceRef_t     serviceRef, ///< [in] Reference to the service.
    le_msg_ReceiveHandler_t handlerFunc,///< [in] Handler function.
    void*                   contextPtr  ///< [in] Opaque pointer value to pass to the handler.
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(serviceRef->serverThread != le_thread_GetCurrent(),
                "Service (%s:%s) not owned by calling thread.",
                serviceRef->interface.id.name,
                le_msg_GetProtocolIdStr(serviceRef->interface.id.protocolRef));

    serviceRef->recvHandler = handlerFunc;
    serviceRef->recvContextPtr = contextPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Associates an opaque context value (void pointer) with a given service that can be retrieved
 * later using le_msg_GetServiceContextPtr().
 *
 * @note    This is a server-only function.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_SetServiceContextPtr
(
    le_msg_ServiceRef_t serviceRef, ///< [in] Reference to the service.

    void*               contextPtr  ///< [in] Opaque value to be returned by
                                    ///         le_msg_GetServiceContextPtr().
)
//--------------------------------------------------------------------------------------------------
{
    serviceRef->contextPtr = contextPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the opaque context value (void pointer) that was associated with a given service using
 * le_msg_SetServiceContextPtr().
 *
 * @return  The context pointer value, or NULL if le_msg_SetServiceContextPtr() was never called
 *          for this service.
 *
 * @note    This is a server-only function.
 */
//--------------------------------------------------------------------------------------------------
void* le_msg_GetServiceContextPtr
(
    le_msg_ServiceRef_t serviceRef  ///< [in] Reference to the service.
)
//--------------------------------------------------------------------------------------------------
{
    return serviceRef->contextPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Makes a given service available for clients to find.
 *
 * @note    This is a server-only function.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_AdvertiseService
(
    le_msg_ServiceRef_t serviceRef  ///< [in] Reference to the service.
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(serviceRef->state != LE_MSG_INTERFACE_SERVICE_HIDDEN,
                "Re-advertising before hiding service '%s:%s'.",
                serviceRef->interface.id.name,
                le_msg_GetProtocolIdStr(serviceRef->interface.id.protocolRef));

    serviceRef->state = LE_MSG_INTERFACE_SERVICE_CONNECTING;

    // Open a socket.
    int fd = unixSocket_CreateSeqPacketUnnamed();
    serviceRef->directorySocketFd = fd;

    // Check for failure.
    LE_FATAL_IF(fd == LE_NOT_PERMITTED, "Permission to open socket denied.");
    LE_FATAL_IF(fd == LE_FAULT, "Failed to open socket.");

    // Warn if one of the three standard I/O streams have been somehow connected to the
    // Service Directory.
    if (fd < 3)
    {
        const char* streamNameStr = "<unknown>";
        switch (fd)
        {
            case 0:
                streamNameStr = "stdin";
                break;
            case 1:
                streamNameStr = "stdout";
                break;
            case 2:
                streamNameStr = "stderr";
                break;
            default:
                break;
        }

        LE_WARN("Service Directory connection mapped to %s.", streamNameStr);
    }

    // Set the socket non-blocking.
    fd_SetNonBlocking(fd);

    // Start monitoring the socket for events.
    StartMonitoringDirectorySocket(serviceRef);

    // Connect the socket to the Service Directory.
    le_result_t result = unixSocket_Connect(fd, LE_SVCDIR_SERVER_SOCKET_NAME);
    LE_FATAL_IF((result != LE_OK) && (result != LE_WOULD_BLOCK),
                "Failed to connect to Service Directory. Result = %d (%s).",
                result,
                LE_RESULT_TXT(result));

    // Wait for writeability notification on the socket.  See DirectorySocketWriteable().
}



//--------------------------------------------------------------------------------------------------
/**
 * Makes a given service unavailable for clients to find, but without terminating any ongoing
 * sessions.
 *
 * @note    This is a server-only function.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_HideService
(
    le_msg_ServiceRef_t serviceRef  ///< [in] Reference to the service.
)
//--------------------------------------------------------------------------------------------------
{
    // Stop monitoring the directory socket.
    le_fdMonitor_Delete(serviceRef->fdMonitorRef);
    serviceRef->fdMonitorRef = NULL;

    // Close the connection with the Service Directory.
    fd_Close(serviceRef->directorySocketFd);
    serviceRef->directorySocketFd = -1;

    serviceRef->state = LE_MSG_INTERFACE_SERVICE_HIDDEN;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches a pointer to the name of an interface.
 *
 * @return Pointer to a null-terminated string.
 *
 * @warning Pointer returned will remain valid only until the interface is deleted.
 */
//--------------------------------------------------------------------------------------------------
const char* le_msg_GetInterfaceName
(
    le_msg_InterfaceRef_t interfaceRef  ///< [in] Reference to the interface.
)
//--------------------------------------------------------------------------------------------------
{
    return interfaceRef->id.name;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches a reference to the protocol supported by a given interface.
 *
 * @return  The protocol reference.
 */
//--------------------------------------------------------------------------------------------------
le_msg_ProtocolRef_t le_msg_GetInterfaceProtocol
(
    le_msg_InterfaceRef_t interfaceRef  ///< [in] Reference to the interface.
)
//--------------------------------------------------------------------------------------------------
{
    return interfaceRef->id.protocolRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check whether or not the calling thread is currently running a Service's message receive handler,
 * and if so, return a reference to the message object being handled.
 *
 * @return  A reference to the message being handled, or NULL if no Service message receive handler
 *          is currently running.
 **/
//--------------------------------------------------------------------------------------------------
le_msg_MessageRef_t le_msg_GetServiceRxMsg
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    return pthread_getspecific(ThreadLocalRxMsgKey);
}
