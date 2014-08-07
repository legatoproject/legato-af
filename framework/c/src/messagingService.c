/** @file messagingService.c
 *
 * Implements the "Service" objects and the "Service List" in the Legato @ref c_messaging.
 *
 * See @ref messaging.c for the overall subsystem design.
 *
 * @warning The code in this file @b must be thread safe and re-entrant.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "unixSocket.h"
#include "serviceDirectory/serviceDirectoryProtocol.h"
#include "messagingService.h"
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

//--------------------------------------------------------------------------------------------------
/**
 * Hashmap in which Service objects are kept.
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t ServiceMapRef;


//--------------------------------------------------------------------------------------------------
/**
 * Pool from which Service objects are allocated.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ServicePoolRef;


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


// =======================================
//  PRIVATE FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Key hash function for the Service Map.
 *
 * @return  The hash value for a unique session ID (the key).
 */
//--------------------------------------------------------------------------------------------------
static size_t ComputeServiceIdHash
(
    const void* keyPtr
)
//--------------------------------------------------------------------------------------------------
{
    const ServiceId_t* idPtr = keyPtr;

    // NOTE: The protocol IDs are likely to be much longer than the service instance names,
    //       and we don't expect there to actually be very many services referenced in the
    //       same process, so a collision here and there isn't a big deal.  So, we just use
    //       the service instance name to compute the hash of the key to save some cycles.

    return le_hashmap_HashString(idPtr->name);
}


//--------------------------------------------------------------------------------------------------
/**
 * Key equality comparison function for the Service Map.
 */
//--------------------------------------------------------------------------------------------------
static bool AreServiceIdsTheSame
(
    const void* firstKeyPtr,
    const void* secondKeyPtr
)
//--------------------------------------------------------------------------------------------------
{
    const ServiceId_t* firstIdPtr = firstKeyPtr;
    const ServiceId_t* secondIdPtr = secondKeyPtr;

    return (   le_hashmap_EqualsString(firstIdPtr->name, secondIdPtr->name)
            && le_hashmap_EqualsString(le_msg_GetProtocolIdStr(firstIdPtr->protocolRef),
                                       le_msg_GetProtocolIdStr(secondIdPtr->protocolRef)) );
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
static Service_t* CreateService
(
    le_msg_ProtocolRef_t    protocolRef,
    const char*             serviceName
)
//--------------------------------------------------------------------------------------------------
{
    Service_t* servicePtr = le_mem_ForceAlloc(ServicePoolRef);
    servicePtr->id.protocolRef = protocolRef;
    le_result_t result = le_utf8_Copy(servicePtr->id.name,
                                      serviceName,
                                      sizeof(servicePtr->id.name),
                                      NULL);
    LE_FATAL_IF(result != LE_OK,
                "Service ID '%s' too long (should only be %zu bytes total).",
                serviceName,
                sizeof(servicePtr->id.name));

    servicePtr->state = LE_MSG_SERVICE_HIDDEN;

    servicePtr->directorySocketFd = -1;
    servicePtr->fdMonitorRef = NULL;
    servicePtr->serverThread = NULL;    // NULL indicates no server in this process.

    servicePtr->sessionList = LE_DLS_LIST_INIT;

    servicePtr->openHandler = NULL;
    servicePtr->openContextPtr = NULL;

    servicePtr->closeHandler = NULL;
    servicePtr->closeContextPtr = NULL;

    servicePtr->recvHandler = NULL;
    servicePtr->recvContextPtr = NULL;

    le_hashmap_Put(ServiceMapRef, &servicePtr->id, servicePtr);

    return servicePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a Service object matching a given service identification.  Must be released using
 * msgService_Release() when you are done with it.
 *
 * @return  Pointer to the service object.
 *
 * @note    Creates an object if one doesn't already exist, so always returns a valid pointer.
 *
 * @warning Assumes that the Mutex is locked.
 */
//--------------------------------------------------------------------------------------------------
static Service_t* GetService
(
    le_msg_ProtocolRef_t    protocolRef,
    const char*             serviceName
)
//--------------------------------------------------------------------------------------------------
{
    ServiceId_t id;

    id.protocolRef = protocolRef;
    LE_FATAL_IF(le_utf8_Copy(id.name, serviceName, sizeof(id.name), NULL) == LE_OVERFLOW,
                "Service ID '%s' too long (should only be %zu bytes total).",
                serviceName,
                sizeof(id.name));

    Service_t* servicePtr = le_hashmap_Get(ServiceMapRef, &id);
    if (servicePtr == NULL)
    {
        servicePtr = CreateService(protocolRef, serviceName);
    }
    else
    {
        le_mem_AddRef(servicePtr);
    }

    return servicePtr;
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
    Service_t* servicePtr = objPtr;

    le_hashmap_Remove(ServiceMapRef, &servicePtr->id);
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
    // If there is an open handler function registered, call it now.
    if (serviceRef->openHandler != NULL)
    {
        serviceRef->openHandler(sessionRef, serviceRef->openContextPtr);
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
    int fd
)
//--------------------------------------------------------------------------------------------------
{
    Service_t* servicePtr = le_event_GetContextPtr();

    if (servicePtr->state == LE_MSG_SERVICE_CONNECTING)
    {
        // Must have connected (or failed to do so).
        int errCode = unixSocket_GetErrorState(servicePtr->directorySocketFd);

        // Disable writeability notification.
        le_event_ClearFdHandlerByEventType(servicePtr->fdMonitorRef, LE_EVENT_FD_WRITEABLE);

        // If connection successful,
        if (errCode == 0)
        {
            // Send the Service ID to the Service Directory.
            msgService_SendServiceId(servicePtr, fd);

            servicePtr->state = LE_MSG_SERVICE_ADVERTISED;

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
    int fd
)
//--------------------------------------------------------------------------------------------------
{
    Service_t* servicePtr = le_event_GetContextPtr();
    le_result_t result;

    int clientSocketFd;

    // Receive the Client connection file descriptor from the Service Directory.
    result = unixSocket_ReceiveMsg(fd,
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
                 servicePtr->id.name,
                 le_msg_GetProtocolIdStr(servicePtr->id.protocolRef));
    }
    else
    {
        // Create a server-side Session object for that connection to this Service.
        le_msg_SessionRef_t sessionRef = msgSession_CreateServerSideSession(servicePtr, clientSocketFd);

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
    int fd
)
//--------------------------------------------------------------------------------------------------
{
    Service_t* servicePtr = le_event_GetContextPtr();

    LE_FATAL("Permission to offer service (%s:%s) has been denied.",
             servicePtr->id.name,
             le_msg_GetProtocolIdStr(servicePtr->id.protocolRef));
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
    int fd
)
//--------------------------------------------------------------------------------------------------
{
    Service_t* servicePtr = le_event_GetContextPtr();

    LE_FATAL("Error on Service Directory connection for service (%s:%s).",
             servicePtr->id.name,
             le_msg_GetProtocolIdStr(servicePtr->id.protocolRef));
}


//--------------------------------------------------------------------------------------------------
/**
 * Start monitoring for readable, hang-up, and error events on a given Service's "Directory Socket"
 * (the socket connected to the Service Directory).
 */
//--------------------------------------------------------------------------------------------------
static void StartMonitoringDirectorySocket
(
    Service_t*  servicePtr
)
//--------------------------------------------------------------------------------------------------
{
    le_event_FdHandlerRef_t handlerRef;

    char name[LIMIT_MAX_MEM_POOL_NAME_BYTES];
    char* destPtr = name;
    size_t spaceLeft = sizeof(name);
    size_t bytesCopied;
    le_utf8_Copy(destPtr, servicePtr->id.name, spaceLeft, &bytesCopied);
    destPtr += bytesCopied;
    spaceLeft -= bytesCopied;
    le_utf8_Copy(destPtr, ":", spaceLeft, &bytesCopied);
    destPtr += bytesCopied;
    spaceLeft -= bytesCopied;
    le_utf8_Copy(destPtr, le_msg_GetProtocolIdStr(servicePtr->id.protocolRef), spaceLeft, NULL);

    servicePtr->fdMonitorRef = le_event_CreateFdMonitor(name, servicePtr->directorySocketFd);

    handlerRef = le_event_SetFdHandler(servicePtr->fdMonitorRef,
                                       LE_EVENT_FD_WRITEABLE,
                                       DirectorySocketWriteable);
    le_event_SetFdHandlerContextPtr(handlerRef, servicePtr);

    handlerRef = le_event_SetFdHandler(servicePtr->fdMonitorRef,
                                       LE_EVENT_FD_READABLE,
                                       DirectorySocketReadable);
    le_event_SetFdHandlerContextPtr(handlerRef, servicePtr);

    handlerRef = le_event_SetFdHandler(servicePtr->fdMonitorRef,
                                       LE_EVENT_FD_READ_HANG_UP,
                                       DirectorySocketClosed);
    le_event_SetFdHandlerContextPtr(handlerRef, servicePtr);

    handlerRef = le_event_SetFdHandler(servicePtr->fdMonitorRef,
                                       LE_EVENT_FD_WRITE_HANG_UP,
                                       DirectorySocketClosed);
    le_event_SetFdHandlerContextPtr(handlerRef, servicePtr);

    handlerRef = le_event_SetFdHandler(servicePtr->fdMonitorRef,
                                       LE_EVENT_FD_ERROR,
                                       DirectorySocketError);
    le_event_SetFdHandlerContextPtr(handlerRef, servicePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Close all sessions on a given Service object's list of open sessions.
 */
//--------------------------------------------------------------------------------------------------
static void CloseAllSessions
(
    Service_t* servicePtr
)
//--------------------------------------------------------------------------------------------------
{
    for (;;)
    {
        le_dls_Link_t* linkPtr = le_dls_Peek(&servicePtr->sessionList);

        if (linkPtr != NULL)
        {
            le_msg_DeleteSession(msgSession_GetSessionContainingLink(linkPtr));
        }
        else
        {
            break;
        }
    }
}


// =======================================
//  PROTECTED (INTER-MODULE) FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the module.  This must be called only once at start-up, before
 * any other functions in that module are called.
 */
//--------------------------------------------------------------------------------------------------
void msgService_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Create and initialize the pool of Service objects.
    ServicePoolRef = le_mem_CreatePool("MessagingServices", sizeof(Service_t));
    le_mem_ExpandPool(ServicePoolRef, MAX_EXPECTED_SERVICES);
    le_mem_SetDestructor(ServicePoolRef, ServiceDestructor);

    // Create the Service Map.
    ServiceMapRef = le_hashmap_Create("MessagingServices",
                                      MAX_EXPECTED_SERVICES,
                                      ComputeServiceIdHash,
                                      AreServiceIdsTheSame);

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
 * Gets a reference to a Service object.  Must be released using msgService_Release() when
 * you are done with it.
 *
 * @return  Reference to the service object.
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t msgService_GetService
(
    le_msg_ProtocolRef_t    protocolRef,
    const char*             serviceName
)
//--------------------------------------------------------------------------------------------------
{
    Service_t* servicePtr;

    LOCK
    servicePtr = GetService(protocolRef, serviceName);
    UNLOCK

    return servicePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Send service identification information via a connected socket.
 *
 * @note    This function is used for both clients and servers.
 */
//--------------------------------------------------------------------------------------------------
void msgService_SendServiceId
(
    le_msg_ServiceRef_t serviceRef, ///< [in] Pointer to the service whose ID is to be sent.
    int                 socketFd    ///< [in] File descriptor of the connected socket to send on.
)
//--------------------------------------------------------------------------------------------------
{
    svcdir_ServiceId_t serviceId;

    memset(&serviceId, 0, sizeof(serviceId));

    serviceId.maxProtocolMsgSize = le_msg_GetProtocolMaxMsgSize(serviceRef->id.protocolRef);

    le_utf8_Copy(serviceId.protocolId,
                 le_msg_GetProtocolIdStr(serviceRef->id.protocolRef),
                 sizeof(serviceId.protocolId),
                 NULL);

    le_utf8_Copy(serviceId.serviceName,
                 serviceRef->id.name,
                 sizeof(serviceId.serviceName),
                 NULL);

    le_result_t result = unixSocket_SendDataMsg(socketFd, &serviceId, sizeof(serviceId));
    if (result != LE_OK)
    {
        LE_FATAL("Failed to send. Result = %d (%s)", result, LE_RESULT_TXT(result));
    }
    // NOTE: This is only done when the socket is newly opened, so this shouldn't ever
    //       return LE_NO_MEMORY to indicate send buffers being full.
}


//--------------------------------------------------------------------------------------------------
/**
 * Release a reference to a Service.
 */
//--------------------------------------------------------------------------------------------------
void msgService_Release
(
    le_msg_ServiceRef_t serviceRef
)
{
    // NOTE: Must lock the mutex before releasing in case the destructor runs, because
    //       the destructor manipulates the service map, which is shared by all threads in the
    //       process.

    LOCK

    le_mem_Release(serviceRef);

    UNLOCK
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a Session to a Service's list of open sessions.
 *
 * @note    This only gets called by the server thread for the service.
 */
//--------------------------------------------------------------------------------------------------
void msgService_AddSession
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionRef_t sessionRef
)
//--------------------------------------------------------------------------------------------------
{
    // The Session object holds a reference to the Service object.
    le_mem_AddRef(serviceRef);

    le_dls_Queue(&serviceRef->sessionList, msgSession_GetListLink(sessionRef));
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove a Session from a Service's list of open sessions.
 *
 * @note    This only gets called by the server thread for the service.
 */
//--------------------------------------------------------------------------------------------------
void msgService_RemoveSession
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionRef_t sessionRef
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Remove(&serviceRef->sessionList, msgSession_GetListLink(sessionRef));

    // The Session object no longer holds a reference to the Service object.
    msgService_Release(serviceRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Call a Service's registered session close handler function, if there is one registered.
 */
//--------------------------------------------------------------------------------------------------
void msgService_CallCloseHandler
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionRef_t sessionRef
)
//--------------------------------------------------------------------------------------------------
{
    // If there is a Close Handler registered, call it now.
    if (serviceRef->closeHandler != NULL)
    {
        serviceRef->closeHandler(sessionRef, serviceRef->closeContextPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Dispatches a message received from a client to a service's server.
 */
//--------------------------------------------------------------------------------------------------
void msgService_ProcessMessageFromClient
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
                serviceRef->id.name,
                le_msg_GetProtocolIdStr(serviceRef->id.protocolRef));
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
    const char*             serviceName     ///< [in] The service instance name.
)
//--------------------------------------------------------------------------------------------------
{
    // Must lock the mutex to prevent races between different threads trying to offer the
    // same service at the same time, or one thread trying to delete a service while another
    // tries to create it, or accessing the Service List hashmap while another thread
    // is updating it.

    LOCK

    // Get a Service object.
    Service_t* servicePtr = GetService(protocolRef, serviceName);

    // If the Service object already has a server thread, then it means that this service
    // is already being offered by someone else in this very process.
    LE_FATAL_IF(servicePtr->serverThread != NULL,
                "Duplicate service (%s:%s) offered in same process.",
                serviceName,
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
                serviceRef->id.name,
                le_msg_GetProtocolIdStr(serviceRef->id.protocolRef));

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
void le_msg_SetServiceOpenHandler
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
                serviceRef->id.name,
                le_msg_GetProtocolIdStr(serviceRef->id.protocolRef));

    serviceRef->openHandler = handlerFunc;
    serviceRef->openContextPtr = contextPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the currently registered service open handler and it's context pointer, or NULL if none are
 * currently registered.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_GetServiceOpenHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in]  Reference to the service.
    le_msg_SessionEventHandler_t*   handlerFunc,///< [out] Handler function.
    void**                          contextPtr  ///< [out] Opaque pointer value to pass to handler.
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(serviceRef == NULL,
                "Service doesn't exist. Make sure service is started before setting handlers");
    LE_FATAL_IF(serviceRef->serverThread != le_thread_GetCurrent(),
                "Service (%s:%s) not owned by calling thread.",
                serviceRef->id.name,
                le_msg_GetProtocolIdStr(serviceRef->id.protocolRef));

    *handlerFunc = serviceRef->openHandler;
    *contextPtr = serviceRef->openContextPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called whenever one of this service's sessions is closed by
 * the client.
 *
 * @note    This is a server-only function.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_SetServiceCloseHandler
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
                serviceRef->id.name,
                le_msg_GetProtocolIdStr(serviceRef->id.protocolRef));

    serviceRef->closeHandler = handlerFunc;
    serviceRef->closeContextPtr = contextPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the currently registered service close handler and it's context pointer, or NULL if none are
 * currently registered.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_GetServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in]  Reference to the service.
    le_msg_SessionEventHandler_t*   handlerFunc,///< [out] Handler function.
    void**                          contextPtr  ///< [out] Opaque pointer value to pass to handler.
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(serviceRef == NULL,
                "Service doesn't exist. Make sure service is started before setting handlers");
    LE_FATAL_IF(serviceRef->serverThread != le_thread_GetCurrent(),
                "Service (%s:%s) not owned by calling thread.",
                serviceRef->id.name,
                le_msg_GetProtocolIdStr(serviceRef->id.protocolRef));

    *handlerFunc = serviceRef->closeHandler;
    *contextPtr = serviceRef->closeContextPtr;
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
                serviceRef->id.name,
                le_msg_GetProtocolIdStr(serviceRef->id.protocolRef));

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
    LE_FATAL_IF(serviceRef->state != LE_MSG_SERVICE_HIDDEN,
                "Re-advertising before hiding service '%s:%s'.",
                serviceRef->id.name,
                le_msg_GetProtocolIdStr(serviceRef->id.protocolRef));

    serviceRef->state = LE_MSG_SERVICE_CONNECTING;

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
        const char* streamNameStr;
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
    le_event_DeleteFdMonitor(serviceRef->fdMonitorRef);
    serviceRef->fdMonitorRef = NULL;

    // Close the connection with the Service Directory.
    fd_Close(serviceRef->directorySocketFd);
    serviceRef->directorySocketFd = -1;

    serviceRef->state = LE_MSG_SERVICE_HIDDEN;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches a pointer to the name of a service.
 *
 * @return A pointer to a null-terminated string.
 *
 * @warning The pointer returned will only remain valid until the service is deleted.
 */
//--------------------------------------------------------------------------------------------------
const char* le_msg_GetServiceName
(
    le_msg_ServiceRef_t serviceRef  ///< [in] Reference to the service.
)
//--------------------------------------------------------------------------------------------------
{
    return serviceRef->id.name;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches a reference to the protocol supported by a given Service.
 *
 * @return  The protocol reference.
 */
//--------------------------------------------------------------------------------------------------
le_msg_ProtocolRef_t le_msg_GetServiceProtocol
(
    le_msg_ServiceRef_t serviceRef  ///< [in] Reference to the service.
)
//--------------------------------------------------------------------------------------------------
{
    return serviceRef->id.protocolRef;
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
