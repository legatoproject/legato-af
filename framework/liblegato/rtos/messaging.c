/**
 * @file messaging.c
 *
 * Implementation of messaging API on RTOS.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

#include "messaging.h"
#include "messagingCommon.h"
#include "messagingLocal.h"
#include "thread.h"

// =======================================
//  PROTECTED (INTER-MODULE) FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Initialize global data required by low-level messaging API.
 */
//--------------------------------------------------------------------------------------------------
void msg_Init
(
    void
)
{
    msgCommon_Init();
    msgLocal_Init();
}

// =======================================
//  PUBLIC API FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Makes a given service available for clients to find.
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_AdvertiseService
(
    le_msg_ServiceRef_t serviceRef  ///< [in] Reference to the service.
)
{
    LE_ASSERT(serviceRef->type == LE_MSG_SERVICE_LOCAL);
    msgLocal_AdvertiseService(CONTAINER_OF(serviceRef,
                                          le_msg_LocalService_t,
                                          service));
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
void le_msg_SetSessionRecvHandler
(
    le_msg_SessionRef_t     sessionRef, ///< [in] Reference to the session.
    le_msg_ReceiveHandler_t handlerFunc,///< [in] Handler function.
    void*                   contextPtr  ///< [in] Opaque pointer value to pass to the handler.
)
{
    msgLocal_SetSessionRecvHandler(sessionRef,
                                  handlerFunc,
                                  contextPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the handler callback function to be called when the session is closed from the other
 * end.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_GetSessionCloseHandler
(
    le_msg_SessionRef_t             sessionRef, ///< [in] Reference to the session.
    le_msg_SessionEventHandler_t*   handlerFuncPtr,///< [out] Handler function.
    void**                          contextPtrPtr  ///< [out] Opaque pointer value to pass
                                                   ///<       to the handler.
)
{
    messagingLocal_GetSessionCloseHandler(sessionRef,
                                          handlerFuncPtr,
                                          contextPtrPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called when messages are received from clients via sessions
 * that they have open with this service.
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_SetServiceRecvHandler
(
    le_msg_ServiceRef_t     serviceRef, ///< [in] Reference to the service.
    le_msg_ReceiveHandler_t handlerFunc,///< [in] Handler function.
    void*                   contextPtr  ///< [in] Opaque pointer value to pass to the handler.
)
{
    LE_ASSERT(serviceRef->type == LE_MSG_SERVICE_LOCAL);
    msgLocal_SetServiceRecvHandler(CONTAINER_OF(serviceRef,
                                               le_msg_LocalService_t,
                                               service),
                                  handlerFunc,
                                  contextPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called whenever one of this service's sessions is closed by
 * the client.
 *
 * @note    Empty stub on RTOS as session close cannot be detected
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t le_msg_AddServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
)
{
    LE_UNUSED(serviceRef);
    LE_UNUSED(handlerFunc);
    LE_UNUSED(contextPtr);
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Synchronously open a session with a service.
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
void le_msg_OpenSession
(
    le_msg_SessionRef_t             sessionRef,      ///< [in] Reference to the session.
    le_msg_SessionEventHandler_t    callbackFunc,   ///< [in] Function to be called when open.
                                                    ///       NULL if no notification is needed.
    void*                           contextPtr      ///< [in] Opaque value to pass to the callback.

)
{
    // Note -- with local messaging, opening a session doesn't block, so
    // call sync open, then call callback immediately.
    msgLocal_OpenSessionSync(sessionRef);
    if (callbackFunc)
    {
        callbackFunc(sessionRef, contextPtr);
    }
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
void le_msg_OpenSessionSync
(
    le_msg_SessionRef_t             sessionRef      ///< [in] Reference to the session.
)
{
    msgLocal_OpenSessionSync(sessionRef);
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
le_result_t le_msg_TryOpenSessionSync
(
    le_msg_SessionRef_t             sessionRef      ///< [in] Reference to the session.
)
{
    return msgLocal_TryOpenSessionSync(sessionRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Terminates a session.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_CloseSession
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
)
{
    msgLocal_CloseSession(sessionRef);
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
void le_msg_DeleteSession
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
)
{
    msgLocal_DeleteSession(sessionRef);
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
 * - In full API this can be called by either client or server, otherwise it can only
 *   be called by the client.
 */
//--------------------------------------------------------------------------------------------------
le_msg_MessageRef_t le_msg_CreateMsg
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
)
{
    return msgLocal_CreateMsg(sessionRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds to the reference count on a message object.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_AddRef
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
{
    msgLocal_AddRef(msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Releases a message object, decrementing its reference count.  If the reference count has reached
 * zero, the message object is deleted.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_ReleaseMsg
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
{
    msgLocal_ReleaseMsg(msgRef);
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
bool le_msg_NeedsResponse
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
{
    return msgLocal_NeedsResponse(msgRef);
}


/**
 * Gets a pointer to the message payload memory buffer.
 *
 * @return Pointer to the payload buffer.
 *
 * @warning Be careful not to overflow this buffer.
 */
//--------------------------------------------------------------------------------------------------
void* le_msg_GetPayloadPtr
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
{
    return msgLocal_GetPayloadPtr(msgRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the size, in bytes, of the message payload memory buffer.
 *
 * @return The size, in bytes.
 */
//--------------------------------------------------------------------------------------------------
size_t le_msg_GetMaxPayloadSize
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
{
    return msgLocal_GetMaxPayloadSize(msgRef);
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
void le_msg_SetFd
(
    le_msg_MessageRef_t msgRef,     ///< [in] Reference to the message.
    int                 fd          ///< [in] File descriptor.
)
{
    msgLocal_SetFd(msgRef, fd);
}

//--------------------------------------------------------------------------------------------------
/**
 * Fetches a received file descriptor from the message.
 *
 * @return The file descriptor, or -1 if no file descriptor was sent with this message or if the
 *         fd was already fetched from the message.
 **/
//--------------------------------------------------------------------------------------------------
int le_msg_GetFd
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
{
    return msgLocal_GetFd(msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends a message.  No response expected.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_Send
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
{
    msgLocal_Send(msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to the session to which a given message belongs.
 *
 * @return Session reference.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_msg_GetSession
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
{
    return msgLocal_GetSession(msgRef);
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
le_msg_MessageRef_t le_msg_RequestSyncResponse
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the request message.
)
{
    return msgLocal_RequestSyncResponse(msgRef);
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
void le_msg_Respond
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the request message.
)
{
    return msgLocal_Respond(msgRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Fetches the user credentials of the client at the far end of a given IPC session.
 *
 * @warning This function can only be called for the server-side of a session.
 *
 * @return LE_OK if successful.
 *         LE_CLOSED if the session has closed.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t le_msg_GetClientUserCreds
(
    le_msg_SessionRef_t sessionRef,   ///< [in] Reference to the session.
    uid_t*              userIdPtr,    ///< [out] Ptr to where the uid is to be stored on success.
    pid_t*              processIdPtr  ///< [out] Ptr to where the pid is to be stored on success.
)
{
    le_result_t     result = LE_OK;
    le_thread_Ref_t threadRef;

    if (userIdPtr)
    {
        // On RTOS every task is owned by root.
        *userIdPtr = 0;
    }

    if (processIdPtr)
    {
        // On RTOS the "process" is the current pthread id.  Not quite the same as on Linux, but
        // as close as we can get.

        threadRef = msgLocal_GetClientThreadRef(sessionRef);
        result = thread_GetOSThread(threadRef, (pthread_t *) processIdPtr);
    }

    return result;
}
