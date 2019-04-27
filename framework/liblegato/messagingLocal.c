/**
 * @file messagingLocal.c
 *
 * Implementation of local messaging API, which is used as the primary "IPC" mechanism on
 * RTOS.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

#include "messagingCommon.h"


//--------------------------------------------------------------------------------------------------
/**
 * Types of session (client or server)
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_MSG_SESSION_SERVER,
    LE_MSG_SESSION_CLIENT
} msg_Type_t;


//--------------------------------------------------------------------------------------------------
/**
 * Session definition for local client sessions.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_msg_LocalSession
{
    struct le_msg_Session session;           ///< Generic session object.
    le_msg_LocalService_t* servicePtr;       ///< Service endpoint for this session
    le_msg_LocalReceiver_t receiver;         ///< Destination of messages to this session
} msg_LocalSession_t;


//--------------------------------------------------------------------------------------------------
/**
 * Static pool for client sessions
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ClientSession, LE_CONFIG_MAX_MSG_LOCAL_CLIENT_SESSION_POOL_SIZE,
    sizeof(msg_LocalSession_t));


//--------------------------------------------------------------------------------------------------
/**
 * Pool for client sessions.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SessionPool;

#if LE_CONFIG_CUSTOM_OS && defined(LE_MSG_SERVICE_READY_FLAG)
    // Include definitions to check service ready flag
#   include "custom_os/fa_messagingLocal.h"
#else

static pthread_mutex_t ServiceReadyMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ServiceReadyCond = PTHREAD_COND_INITIALIZER;

//--------------------------------------------------------------------------------------------------
/**
 * Initialize service ready flag
 */
//--------------------------------------------------------------------------------------------------
static inline void InitServiceReady
(
    le_msg_LocalService_t* servicePtr
)
{
    servicePtr->serviceReady = false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if a service is ready.
 *
 * @note Standard method requires pthread_cond which is not available on all platforms.  To
 *       override define LE_MSG_SERVICE_READY_FLAG
 */
//--------------------------------------------------------------------------------------------------
static inline void WaitServiceReady
(
    le_msg_LocalService_t* servicePtr
)
{
    pthread_mutex_lock(&ServiceReadyMutex);
    while (!servicePtr->serviceReady)
    {
        pthread_cond_wait(&ServiceReadyCond, &ServiceReadyMutex);
    }
    pthread_mutex_unlock(&ServiceReadyMutex);
}

//--------------------------------------------------------------------------------------------------
/**
 * Signal a service is ready.
 *
 * @note Standard method requires pthread_cond which is not available on all platforms.  To
 *       override define LE_MSG_SERVICE_READY_FLAG
 */
//--------------------------------------------------------------------------------------------------
static inline void SignalServiceReady
(
    le_msg_LocalService_t* servicePtr
)
{
    pthread_mutex_lock(&ServiceReadyMutex);
    if (!servicePtr->serviceReady)
    {
        servicePtr->serviceReady = true;
        pthread_cond_broadcast(&ServiceReadyCond);
    }
    pthread_mutex_unlock(&ServiceReadyMutex);
}
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Initialize global data required by low-level messaging API.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_Init
(
    void
)
{
    SessionPool = le_mem_InitStaticPool(ClientSession,
                                        LE_CONFIG_MAX_MSG_LOCAL_CLIENT_SESSION_POOL_SIZE,
                                        sizeof(msg_LocalSession_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Destructor for messages.
 */
//--------------------------------------------------------------------------------------------------
static void MessageDestructor
(
    void* msgVoidPtr               ///< [in] message
)
{
    le_msg_LocalMessage_t* msgPtr = msgVoidPtr;

    le_sem_Delete(msgPtr->responseReady);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize a local messaging service.
 *
 * This must be called before any client can connect to the servce, for example in COMPONENT_INIT
 * before any other threads are created.
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_msg_InitLocalService
(
    le_msg_LocalService_t* servicePtr,     ///< [in] service
    const char* serviceNameStr,            ///< [in] service name
    le_mem_PoolRef_t messagePoolRef        ///< [in] memory pool for messages
)
{
    LE_UNUSED(serviceNameStr);

    // If the service is bound, initialize it.
    if (servicePtr)
    {
        servicePtr->service.type = LE_MSG_SERVICE_LOCAL;
        InitServiceReady(servicePtr);
        servicePtr->receiver.thread = NULL;
        servicePtr->receiver.handler = NULL;
        servicePtr->receiver.contextPtr = NULL;
        servicePtr->messagePool = messagePoolRef;
        le_mem_SetDestructor(servicePtr->messagePool, MessageDestructor);

        return &servicePtr->service;
    }
    else
    {
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a local messaging service.
 *
 * @note This is intended to be used in the implementation of le_msg_AdvertiseService.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_AdvertiseService
(
    le_msg_LocalService_t* servicePtr            ///< [in] service
)
{
    LE_FATAL_IF(!servicePtr, "No such service");

    servicePtr->receiver.thread = le_thread_GetCurrent();

    SignalServiceReady(servicePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the receive handler callback function to be called when a non-response message arrives
 * on this session.
 *
 * The handler function will be called by the Legato event loop of the thread that created
 * the session.
 *
 * @note    This is a client-only function.  Servers are expected to use
 *          le_msg_SetServiceRecvHandler() instead.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_SetSessionRecvHandler
(
    msg_LocalSession_t*     sessionRef, ///< [in] Reference to the session.
    le_msg_ReceiveHandler_t handlerFunc,///< [in] Handler function.
    void*                   contextPtr  ///< [in] Opaque pointer value to pass to the handler.
)
{
    // Set session receive handle if the session exists.  If it doesn't exist, it means the
    // session isn't bound.  Since session isn't bound, can just throw away the handler
    // as it will never be called.
    if (sessionRef)
    {
        sessionRef->receiver.handler = handlerFunc;
        sessionRef->receiver.contextPtr = contextPtr;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the handler callback function to be called when the session is closed from the other
 * end.
 *
 * @note: This is a stub for local messaging
 */
//--------------------------------------------------------------------------------------------------
void messagingLocal_GetSessionCloseHandler
(
    msg_LocalSession_t*             sessionRef,    ///< [in] Reference to the session.
    le_msg_SessionEventHandler_t*   handlerFuncPtr,///< [out] Handler function.
    void**                          contextPtrPtr  ///< [out] Opaque pointer value to pass
                                                   ///<       to the handler.
)
{
    LE_UNUSED(sessionRef);

    *handlerFuncPtr = NULL;
    *contextPtrPtr  = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called when messages are received from clients via sessions
 * that they have open with this service.
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_SetServiceRecvHandler
(
    le_msg_LocalService_t* servicePtr,  ///< [in] service
    le_msg_ReceiveHandler_t handlerFunc,///< [in] Handler function.
    void*                   contextPtr  ///< [in] Opaque pointer value to pass to the handler.
)
{
    LE_FATAL_IF(!servicePtr, "No such service");

    // Store server handler & context in message box structure.
    servicePtr->receiver.handler = handlerFunc;
    servicePtr->receiver.contextPtr = contextPtr;
}



//--------------------------------------------------------------------------------------------------
/**
 * Create a session that will always use message boxes to talk to a service in the same process
 * space.
 *
 * @return   Session reference.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_msg_CreateLocalSession
(
    le_msg_LocalService_t*             servicePtr  ///< [in] Reference to the service.
)
{
    if (!servicePtr)
    {
        // No such service, so do not create a session.
        return NULL;
    }

    msg_LocalSession_t* clientSessionPtr = le_mem_ForceAlloc(SessionPool);
    memset(clientSessionPtr, 0, sizeof(msg_LocalSession_t));
    clientSessionPtr->session.type = LE_MSG_SESSION_LOCAL;
    clientSessionPtr->receiver.thread = le_thread_GetCurrent();
    clientSessionPtr->servicePtr = servicePtr;

    return &clientSessionPtr->session;
}



//--------------------------------------------------------------------------------------------------
/**
 * Deletes a session.  This will end the session and free up any resources associated
 * with it.  Any pending request-response transactions in this session will be terminated.
 * If the far end has registered a session close handler callback, it will be called.
 *
 * @note    Function is only used by clients.  On the server side, sessions are automatically
 *          deleted when they close.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_DeleteSession
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
)
{
    // If no such session exists, fatal.
    if (!sessionRef)
    {
        return;
    }

    le_mem_Release(sessionRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Synchronously open a session with a service.  Blocks until the session is open.
 *
 * This function logs a fatal error and terminates the calling process if unsuccessful.
 *
 * @note    Only clients open sessions.  Servers must patiently wait for clients to open sessions
 *          with them.
 *
 * @warning If the client and server do not agree on the maximum message size for the protocol,
 *          a fatal error will be logged and the client process will be killed.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_OpenSessionSync
(
    msg_LocalSession_t*            sessionRef      ///< [in] Reference to the session.
)
{
    // If no such session exists, fatal.
    LE_FATAL_IF(!sessionRef, "No such session");
    le_msg_LocalService_t* servicePtr = sessionRef->servicePtr;

    // Wait for service to start
    WaitServiceReady(servicePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Synchronously open a session with a service.  Does not wait for the session to become available
 * if not available..
 *
 * le_msg_TryOpenSessionSync() differs from le_msg_OpenSessionSync() in that
 * le_msg_TryOpenSessionSync() will not wait for a server session to become available if it's
 * not already available at the time of the call.  That is, if the client's interface is not
 * bound to any service, or if the service that it's bound to is not currently advertised
 * by the server, then le_msg_TryOpenSessionSync() will return an error code, while
 * le_msg_OpenSessionSync() will wait until the binding is created or the server advertises
 * the service (or both).
 *
 * @return
 *  - LE_OK if the session was successfully opened.
 *  - LE_NOT_FOUND if the server is not currently offering the service to which the client is bound.
 *  - LE_NOT_PERMITTED if the client interface is not bound to any service (doesn't have a binding).
 *  - LE_COMM_ERROR if the Service Directory cannot be reached.
 *
 * @note    Only clients open sessions.  Servers' must patiently wait for clients to open sessions
 *          with them.
 *
 * @warning If the client and server do not agree on the maximum message size for the protocol,
 *          a fatal error will be logged and the client process will be killed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t msgLocal_TryOpenSessionSync
(
    le_msg_SessionRef_t             sessionRef      ///< [in] Reference to the session.
)
{
    // Check if session exists and is started.
    if (!sessionRef)
    {
        // Bindings are static.  If the session ref is NULL, it means this service is not bound.
        return LE_NOT_PERMITTED;
    }

    le_msg_LocalService_t* servicePtr = (le_msg_LocalService_t*)sessionRef;

    // Do not lock; we want to fail if service is not yet started rather than blocking
    // until service starts.
    if (servicePtr->receiver.thread != NULL)
    {
        return LE_OK;
    }
    else
    {
        return LE_NOT_FOUND;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Terminates a session.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_CloseSession
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
)
{
    le_mem_Release(sessionRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a message to be sent over a given session.
 *
 * @return  Message reference.
 *
 * @note
 * - Function never returns on failure, there's no need to check the return code.
 * - If you see warnings on message pools expanding, then you may be forgetting to
 *   release the messages you have received.
 */
//--------------------------------------------------------------------------------------------------
le_msg_MessageRef_t msgLocal_CreateMsg
(
    msg_LocalSession_t* sessionRef  ///< [in] Reference to the session.
)
{
    LE_FATAL_IF(!sessionRef, "No such session");
    LE_FATAL_IF(!sessionRef->servicePtr, "No such service");
    le_msg_LocalService_t* servicePtr = sessionRef->servicePtr;
    le_msg_LocalMessage_t* msgPtr = le_mem_ForceAlloc(servicePtr->messagePool);
    msgPtr->message.sessionRef = &sessionRef->session;
    msgPtr->responseReady = le_sem_Create("msgResponseReady", 0);
    msgPtr->fd = -1;
    msgPtr->completionCallback = NULL;
    msgPtr->contextPtr = NULL;

    return &msgPtr->message;
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds to the reference count on a message object.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_AddRef
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
{
    le_mem_AddRef(CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message));
}

//--------------------------------------------------------------------------------------------------
/**
 * Releases a message object, decrementing its reference count.  If the reference count has reached
 * zero, the message object is deleted.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_ReleaseMsg
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
{
    le_mem_Release(CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message));
}

//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a message requires a response or not.
 *
 * @note    This is intended for use on the server side only.
 *
 * @return
 *  - TRUE if the message needs to be responded to using le_msg_Respond().
 *  - FALSE if the message doesn't need to be responded to, and should be disposed of using
 *    le_msg_ReleaseMsg() when it's no longer needed.
 */
//--------------------------------------------------------------------------------------------------
bool msgLocal_NeedsResponse
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
{
    LE_FATAL_IF(!msgRef, "No such message");

    return CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message)->needsResponse;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets a pointer to the message payload memory buffer.
 *
 * @return Pointer to the payload buffer.
 *
 * @warning Be careful not to overflow this buffer.
 */
//--------------------------------------------------------------------------------------------------
void* msgLocal_GetPayloadPtr
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
{
    LE_FATAL_IF(!msgRef, "No such message");

    return CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message)->data;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the size, in bytes, of the message payload memory buffer.
 *
 * @return The size, in bytes.
 */
//--------------------------------------------------------------------------------------------------
size_t msgLocal_GetMaxPayloadSize
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
{
    LE_FATAL_IF(!msgRef, "No such message");
    LE_FATAL_IF(!msgRef->sessionRef,
                "No such session");
    LE_FATAL_IF(!CONTAINER_OF(msgRef->sessionRef,
                              msg_LocalSession_t,
                              session)->servicePtr, "No such service");

    return le_mem_GetObjectSize(CONTAINER_OF(msgRef->sessionRef,
                                             msg_LocalSession_t,
                                             session)->servicePtr->messagePool) -
        LE_MSG_LOCAL_HEADER_SIZE;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the file descriptor to be sent with this message.
 *
 * This file descriptor will be closed when the message is sent (or when it's deleted without
 * being sent).
 *
 * At most one file descriptor is allowed to be sent per message.
 **/
//--------------------------------------------------------------------------------------------------
void msgLocal_SetFd
(
    le_msg_MessageRef_t msgRef,     ///< [in] Reference to the message.
    int                 fd          ///< [in] File descriptor.
)
{
    LE_FATAL_IF(!msgRef, "No such message");
    le_msg_LocalMessage_t* localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    LE_FATAL_IF(localMsgPtr->fd != -1, "Cannot set fd twice");

    localMsgPtr->fd = fd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches a received file descriptor from the message.
 *
 * @return The file descriptor, or -1 if no file descriptor was sent with this message or if the
 *         fd was already fetched from the message.
 **/
//--------------------------------------------------------------------------------------------------
int msgLocal_GetFd
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
{
    LE_FATAL_IF(!msgRef, "No such message");
    le_msg_LocalMessage_t* localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);

    int fd = localMsgPtr->fd;
    localMsgPtr->fd = -1;

    return fd;
}

//--------------------------------------------------------------------------------------------------
/**
 * Call the completion callback function for a given message, if it has one.
 */
//--------------------------------------------------------------------------------------------------
static void msgLocal_CallCompletionCallback
(
    void* msgVoidPtr,              ///< [in] message
    void* receiverVoidPtr          ///< [in] message receiver (client or server)
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t msgRef = msgVoidPtr;

    le_msg_LocalMessage_t* localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    if (localMsgPtr->completionCallback != NULL)
    {
        if (localMsgPtr->needsResponse != true)
        {
            LE_FATAL("Message is invalid");
        }

        // Call the completion handler callback
        localMsgPtr->completionCallback(msgRef, receiverVoidPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Dispatch a message onto handler function on this thread.
 *
 * Called to pass on message via QueueFunctionToThread
 */
//--------------------------------------------------------------------------------------------------
static void msgLocal_Recv
(
    void* msgVoidPtr,              ///< [in] message
    void* receiverVoidPtr          ///< [in] message receiver (client or server)
)
{
    le_msg_LocalReceiver_t* receiverPtr = receiverVoidPtr;
    le_msg_MessageRef_t msgRef = msgVoidPtr;

    // Pass the message to the server's registered receive handler, if there is one.
    if (receiverPtr->handler)
    {
        // Call the receive handler
        msgCommon_CallRecvHandler(receiverPtr->handler, msgRef, receiverPtr->contextPtr);
    }
    else
    {
        LE_FATAL("No service receive handler.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Find thread to send message to, and queue to be received by that thread.
 */
//--------------------------------------------------------------------------------------------------
static void msgLocal_SendRaw
(
    le_msg_LocalMessage_t* localMessagePtr      ///< [in] Reference to the message.
)
{
    LE_FATAL_IF(!localMessagePtr, "No such message");
    LE_FATAL_IF(!localMessagePtr->message.sessionRef, "No such session");
    msg_LocalSession_t* localSessionPtr = CONTAINER_OF(localMessagePtr->message.sessionRef,
                                                       msg_LocalSession_t,
                                                       session);

    le_thread_Ref_t currentThread = le_thread_GetCurrent();
    le_thread_Ref_t clientThread = localSessionPtr->receiver.thread;
    le_msg_LocalReceiver_t* receiverPtr = NULL;

    le_msg_LocalService_t* servicePtr = localSessionPtr->servicePtr;
    le_thread_Ref_t serverThread = servicePtr->receiver.thread;

    if (clientThread == currentThread)
    {
        // Set the receivePtr to the Server's receiver
        receiverPtr = &servicePtr->receiver;
    }
    else if (serverThread == currentThread)
    {
        // Set the receivePtr to the Client's receiver
        receiverPtr = &localSessionPtr->receiver;
    }
    else
    {
        LE_FATAL("Message sent by invalid thread");
    }

    // Enqueue the message for sending
    le_event_QueueFunctionToThread(receiverPtr->thread, msgLocal_Recv,
                                   &localMessagePtr->message, receiverPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a message.  No response is expected.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_Send
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
{
    LE_FATAL_IF(!msgRef, "No such message");
    le_msg_LocalMessage_t* localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);

    localMsgPtr->needsResponse = false;
    msgLocal_SendRaw(localMsgPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to the session to which a given message belongs.
 *
 * @return Session reference.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t msgLocal_GetSession
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
{
    LE_FATAL_IF(!msgRef, "No such message");

    return msgRef->sessionRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start an asynchronous request-response transaction.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_RequestResponse
(
    le_msg_MessageRef_t         msgRef,     ///< [in] Reference to the request message.
    le_msg_ResponseCallback_t   handlerFunc,///< [in] Function to be called when transaction done.
    void*                       contextPtr  ///< [in] Opaque value to be passed to handler function.
)
{
    LE_FATAL_IF(!msgRef, "No such message");
    le_msg_LocalMessage_t* localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);

    localMsgPtr->completionCallback = handlerFunc;
    localMsgPtr->contextPtr = contextPtr;
    localMsgPtr->needsResponse = true;

    msgLocal_SendRaw(localMsgPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Requests a response from a server by sending it a request.  Blocks until the response arrives
 * or until the transaction terminates without a response (i.e., if the session terminates or
 * the server deletes the request without responding).
 *
 * @return  Reference to the response message, or NULL if the transaction terminated without a
 *          response.
 *
 * @note
 *        - To prevent deadlocks, this function can only be used on the client side of a session.
 *          Servers can't use this function.
 *        - To prevent race conditions, only the client thread attached to the session
 *          (the thread that created the session) is allowed to perform a synchronous
 *          request-response transaction.
 *
 * @warning
 *        - The calling (client) thread will be blocked until the server responds, so no other
 *          event handling will happen in that client thread until the response is received (or the
 *          server dies).  This function should only be used when the server is certain
 *          to respond quickly enough to ensure that it will not cause any event response time
 *          deadlines to be missed by the client.  Consider using le_msg_RequestResponse()
 *          instead.
 *        - If this function is used when the client and server are in the same thread, then the
 *          message will be discarded and NULL will be returned.  This is a deadlock prevention
 *          measure.
 */
//--------------------------------------------------------------------------------------------------
le_msg_MessageRef_t msgLocal_RequestSyncResponse
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the request message.
)
{
    LE_FATAL_IF(!msgRef, "No such message");
    le_msg_LocalMessage_t* localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);

    localMsgPtr->needsResponse = true;

    msgLocal_SendRaw(localMsgPtr);

    // Wait for handover of message back to client
    le_sem_Wait(localMsgPtr->responseReady);

    // One message is shared for both send & receive, so return same message that came in.
    return msgRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a response back to the client that send the request message.
 *
 * Takes a reference to the request message.  Copy the response payload (if any) into the
 * same payload buffer that held the request payload, then call le_msg_Respond().
 *
 * The messaging system will delete the message automatically when it's finished sending
 * the response.
 *
 * @note    Function can only be used on the server side of a session.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_Respond
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the request message.
)
{
    LE_FATAL_IF(!msgRef, "No such message");
    le_msg_LocalMessage_t* localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);

    if (localMsgPtr->completionCallback != NULL)
    {
        msg_LocalSession_t* localSessionPtr =
            CONTAINER_OF(msgRef->sessionRef, msg_LocalSession_t, session);

        // Enqueue the message back to the Client's Completion Callback handler
        le_event_QueueFunctionToThread(localSessionPtr->receiver.thread,
                                       msgLocal_CallCompletionCallback,
                                       msgRef,
                                       localMsgPtr->contextPtr);
    }
    else
    {
        le_sem_Post(localMsgPtr->responseReady);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get client thread reference
 */
//--------------------------------------------------------------------------------------------------
le_thread_Ref_t msgLocal_GetClientThreadRef
(
    le_msg_SessionRef_t sessionRef     ///< [IN] Session reference
)
{
    msg_LocalSession_t* localSessionPtr = CONTAINER_OF(sessionRef, msg_LocalSession_t, session);

    return localSessionPtr->receiver.thread;
}
