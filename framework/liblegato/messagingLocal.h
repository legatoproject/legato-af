/**
 * @file messagingLocal.h
 *
 * Interface of local messaging API, which is used as the primary "IPC" mechanism on
 * RTOS.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_LIBLEGATO_MESSAGING_LOCAL_INCLUDE_GUARD
#define LEGATO_LIBLEGATO_MESSAGING_LOCAL_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Initialize global data required by low-level messaging API.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Start a local messaging service.
 *
 * @note This is intended to be used in the implementation of le_msg_AdvertiseService.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_AdvertiseService
(
    le_msg_LocalService_t* servicePtr       ///< [in] service
);

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
    le_msg_SessionRef_t     sessionRef, ///< [in] Reference to the session.
    le_msg_ReceiveHandler_t handlerFunc,///< [in] Handler function.
    void*                   contextPtr  ///< [in] Opaque pointer value to pass to the handler.
);


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
    le_msg_SessionRef_t             sessionRef,    ///< [in] Reference to the session.
    le_msg_SessionEventHandler_t*   handlerFuncPtr,///< [out] Handler function.
    void**                          contextPtrPtr  ///< [out] Opaque pointer value to pass
                                                   ///<       to the handler.
);


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
);

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
);


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
    le_msg_SessionRef_t             sessionRef      ///< [in] Reference to the session.
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * Terminates a session.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_CloseSession
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
);

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
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
);

//--------------------------------------------------------------------------------------------------
/**
 * Adds to the reference count on a message object.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_AddRef
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
);


//--------------------------------------------------------------------------------------------------
/**
 * Releases a message object, decrementing its reference count.  If the reference count has reached
 * zero, the message object is deleted.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_ReleaseMsg
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
);

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
);

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
);

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
);

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
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * Sends a message.  No response expected.
 */
//--------------------------------------------------------------------------------------------------
void msgLocal_Send
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
);

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
);

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
);

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
);

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
);


//--------------------------------------------------------------------------------------------------
/**
 * Check if the calling thread is currently running a Service's message receive handler;
 *  if so, return a reference to the message object being handled.
 *
 * @return  Reference to the message being handled, or NULL if no Service message receive handler
 *          is currently running.
 **/
//--------------------------------------------------------------------------------------------------
le_msg_MessageRef_t msgLocal_GetServiceRxMsg
(
    void
);


/**
 * Get client thread reference
 */
//--------------------------------------------------------------------------------------------------
le_thread_Ref_t msgLocal_GetClientThreadRef
(
    le_msg_SessionRef_t sessionRef     ///< [IN] Session reference
);


#endif /* LEGATO_LIBLEGATO_MESSAGING_LOCAL_INCLUDE_GUARD */
