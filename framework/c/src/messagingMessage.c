/** @file messagingMessage.c
 *
 * @ref c_messaging implementation's "Message" module implementation.
 *
 * See @ref messaging.c for an overview of the @ref c_messaging implementation.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "messagingMessage.h"
#include "messagingProtocol.h"
#include "messagingSession.h"
#include "messagingService.h"

// =======================================
//  PRIVATE FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Destructor function for Message objects.
 */
//--------------------------------------------------------------------------------------------------
static void MessageDestructor
(
    void* objPtr
)
//--------------------------------------------------------------------------------------------------
{
    Message_t* msgPtr = objPtr;

    // If the session is still open and we are releasing a message that the client expects a
    // response to, the client could get stuck waiting for the response forever.  So, we close
    // the session to wake up the client (and probably kill it).
    if (msgSession_IsOpen(msgPtr->sessionRef) && le_msg_NeedsResponse(msgPtr))
    {
        LE_ERROR("Released a message without sending response expected by client.");

        le_msg_CloseSession(msgPtr->sessionRef);
        // NOTE: Because the message object holds a reference to the session object, even though
        // we have closed the session and it has been "deleted", it actually still exists until
        // we release it (later in this function).

        // Because the session is closing without the server asking for it to be closed,
        // notify the server of the closure (if the server has a close handler registered).
        msgService_CallCloseHandler(msgSession_GetServiceRef(msgPtr->sessionRef),
                                    msgPtr->sessionRef);
    }

    // Release the Message object's hold on the Session object.
    le_mem_Release(msgPtr->sessionRef);
}


// =======================================
//  PROTECTED (INTER-MODULE) FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Initializes this module.  This must be called only once at start-up, before any other functions
 * in this module are called.
 */
//--------------------------------------------------------------------------------------------------
void msgMessage_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Don't actually need to do anything here.
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a Message Pool.
 *
 * @return  A reference to the pool.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t msgMessage_CreatePool
(
    const char* name,       ///< [in] Name of the pool.
    size_t largestMsgSize   ///< [in] Size of the largest message payload, in bytes.
)
//--------------------------------------------------------------------------------------------------
{
    char poolName[128];
    size_t bytesCopied;
    le_result_t result;

    le_utf8_Copy(poolName, name, sizeof(poolName), &bytesCopied);
    result = le_utf8_Copy(poolName + bytesCopied, "-Msgs", sizeof(poolName) - bytesCopied, NULL);
    LE_ERROR_IF(result != LE_OK, "Pool name truncated to '%s'.", poolName);

    le_mem_PoolRef_t poolRef = le_mem_CreatePool(poolName, sizeof(Message_t) + largestMsgSize);

    le_mem_SetDestructor(poolRef, MessageDestructor);

    le_mem_ExpandPool(poolRef, 10); /// @todo Make this configurable.

    return poolRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Send a single message over a connected socket.
 *
 * @return
 * - LE_OK if successful.
 * - LE_NO_MEMORY if the socket doesn't have enough send buffer space available right now.
 * - LE_COMM_ERROR if the socket reported an error on the send operation.
 *
 * @note    Won't return LE_NO_MEMORY if the socket is in blocking mode.
 */
//--------------------------------------------------------------------------------------------------
le_result_t msgMessage_Send
(
    int         socketFd,   ///< [IN] Connected socket's file descriptor.
    Message_t*  msgPtr      ///< The Message to be sent.
)
//--------------------------------------------------------------------------------------------------
{
    // The first bytes come from our transaction ID and the rest (if any)
    // from our Message object's payload section.
    struct iovec ioVector[] =   {
                                    {   iov_base: &msgPtr->txnId,
                                        iov_len: sizeof(msgPtr->txnId)
                                    },
                                    {   iov_base: msgPtr->payload,
                                        iov_len: le_msg_GetMaxPayloadSize(msgPtr)
                                    }
                                };
    struct msghdr msgHeader =   {
                                    msg_name: NULL,
                                    msg_namelen: 0,
                                    msg_iov: ioVector,
                                    msg_iovlen: NUM_ARRAY_MEMBERS(ioVector),
                                    msg_control: NULL,
                                    msg_controllen: 0,
                                };

    ssize_t bytesSent;
    do
    {
        bytesSent = sendmsg(socketFd, &msgHeader, MSG_EOR);
    }
    while ((bytesSent == -1) && (errno == EINTR));

    if (bytesSent < 0)
    {
        // Failed to send!

        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
        {
            // There's no send buffer memory available in the kernel.
            LE_DEBUG("Out of send buffer memory.");
            return LE_NO_MEMORY;
        }
        else
        {
            LE_ERROR("sendmsg() failed. Errno = %d (%m).", errno);
            return LE_COMM_ERROR;
        }
    }
    else
    {
        LE_ASSERT(bytesSent == (sizeof(msgPtr->txnId) + le_msg_GetMaxPayloadSize(msgPtr)));
        return LE_OK;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Receive a single message from a connected socket.
 *
 * @return
 * - LE_OK if successful.
 * - LE_NOT_FOUNT if there's nothing there to receive.
 * - LE_CLOSED if the connection has closed.
 * - LE_COMM_ERROR if an error was encountered.
 */
//--------------------------------------------------------------------------------------------------
le_result_t msgMessage_Receive
(
    int                 socketFd,   ///< [IN] The socket's file descriptor.
    le_msg_MessageRef_t msgRef      ///< [IN] Message object to store the received message in.
)
//--------------------------------------------------------------------------------------------------
{
    // Receive the first bytes into our transaction ID and the rest (if any)
    // into our Message object's payload section.
    struct iovec ioVector[] =   {
                                    {   iov_base: &msgRef->txnId,
                                        iov_len: sizeof(msgRef->txnId)
                                    },
                                    {   iov_base: msgRef->payload,
                                        iov_len: le_msg_GetMaxPayloadSize(msgRef)
                                    }
                                };
    struct msghdr msgHeader =   {
                                    msg_name: NULL,
                                    msg_namelen: 0,
                                    msg_iov: ioVector,
                                    msg_iovlen: NUM_ARRAY_MEMBERS(ioVector),
                                    msg_control: NULL,
                                    msg_controllen: 0,
                                };
    ssize_t bytesReceived;
    do
    {
        bytesReceived = recvmsg(socketFd, &msgHeader, 0);
    }
    while ((bytesReceived == -1) && (errno == EINTR));

    if (bytesReceived < 0)
    {
        if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
        {
            return LE_NOT_FOUND;
        }
        else if (errno == ECONNRESET)
        {
            return LE_CLOSED;
        }
        else
        {
            // Failed to receive!  This is an error on the connection.
            LE_ERROR("recvmsg() failed. Errno = %d (%m).", errno);
            return LE_COMM_ERROR;
        }
    }
    else if (bytesReceived == 0)
    {
        // The socket closed down.  This can trigger a "readable" event, so this is normal.
        return LE_CLOSED;
    }
    else
    {
        return LE_OK;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Call the completion callback function for a given message, if it has one.
 */
//--------------------------------------------------------------------------------------------------
void msgMessage_CallCompletionCallback
(
    le_msg_MessageRef_t requestMsgRef,
    le_msg_MessageRef_t responseMsgRef
)
//--------------------------------------------------------------------------------------------------
{
    if (requestMsgRef->completionCallback != NULL)
    {
        requestMsgRef->completionCallback(responseMsgRef, requestMsgRef->contextPtr);
    }
}


// =======================================
//  PUBLIC API FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Creates a message to be sent over a given session.
 *
 * @return  The message reference.
 *
 * @note
 * - This function never returns on failure, so no need to check the return code.
 * - If you see warnings about message pools expanding, then you may be forgetting to
 *   release the messages you have received.
 */
//--------------------------------------------------------------------------------------------------
le_msg_MessageRef_t le_msg_CreateMsg
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    // Get a reference to the Session's Protocol and ask the Protocol to allocate a Message
    // object from its Message Pool.
    le_msg_ProtocolRef_t protocolRef = le_msg_GetSessionProtocol(sessionRef);
    Message_t* msgPtr = msgProto_AllocMessage(protocolRef);

    // Initialize the Message object's data members.
    msgPtr->link = LE_DLS_LINK_INIT;
    msgPtr->sessionRef = sessionRef;
    le_mem_AddRef(sessionRef);  // Message object holds a reference to the Session object.
    msgPtr->completionCallback = NULL;
    msgPtr->contextPtr = NULL;
    msgPtr->txnId = 0;
    memset(msgPtr->payload, 0, le_msg_GetProtocolMaxMsgSize(protocolRef));

    return msgPtr;
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
//--------------------------------------------------------------------------------------------------
{
    le_mem_AddRef(msgRef);
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
//--------------------------------------------------------------------------------------------------
{
    le_mem_Release(msgRef);
}



//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a message requires a response or not.
 *
 * @note    This is intended for use on the server side only.
 *
 * @return
 *  - TRUE if the message needs to be responded to using le_msg_Respond().
 *  - FALSE if the message does not need to be responded to, and should be disposed of using
 *    le_msg_ReleaseMsg() when it is no longer needed.
 */
//--------------------------------------------------------------------------------------------------
bool le_msg_NeedsResponse
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
//--------------------------------------------------------------------------------------------------
{
    return ((msgRef->txnId != 0) && (!msgSession_IsClient(msgRef->sessionRef)));
}



//--------------------------------------------------------------------------------------------------
/**
 * Gets a pointer to the message payload memory buffer.
 *
 * @return A pointer to the payload buffer.
 *
 * @warning Be careful not to overflow this buffer.
 */
//--------------------------------------------------------------------------------------------------
void* le_msg_GetPayloadPtr
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
//--------------------------------------------------------------------------------------------------
{
    return msgRef->payload;
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
//--------------------------------------------------------------------------------------------------
{
    return le_msg_GetProtocolMaxMsgSize(le_msg_GetSessionProtocol(msgRef->sessionRef));
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
//--------------------------------------------------------------------------------------------------
{
    // Tell the Session to send the message.
    msgSession_SendMessage(msgRef->sessionRef, msgRef);
}



//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to the session to which a given message belongs.
 *
 * @return The session reference.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_msg_GetSession
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
)
//--------------------------------------------------------------------------------------------------
{
    return msgRef->sessionRef;
}



//--------------------------------------------------------------------------------------------------
/**
 * Requests a response from a server by sending it a request.  Does not block.  Instead,
 * provides a callback function to be called when the response arrives or the transaction
 * terminates without a response (due to the session terminating or the server deleting the
 * request without responding).
 *
 * @note
 *     - The thread that is attached to the session (that is, the thread that created the session)
 *       will call the callback from its main event loop.  Of course, this means that if
 *       that thread doesn't run its main event loop then it will not call the callback.
 *     - This function can only be used on the client side of a session.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_RequestResponse
(
    le_msg_MessageRef_t         msgRef,     ///< [in] Reference to the request message.
    le_msg_ResponseCallback_t   handlerFunc,///< [in] Function to be called when transaction done.
    void*                       contextPtr  ///< [in] Opaque value to be passed to handler function.
)
//--------------------------------------------------------------------------------------------------
{
    // Save the completion callback function.
    msgRef->completionCallback = handlerFunc;
    msgRef->contextPtr = contextPtr;

    // Tell the Session to do an asynchronous request-response transaction.
    msgSession_RequestResponse(msgRef->sessionRef, msgRef);
}



//--------------------------------------------------------------------------------------------------
/**
 * Requests a response from a server by sending it a request.  Blocks until the response arrives
 * or until the transaction terminates without a response (i.e., if the session terminates or
 * the server deletes the request without responding).
 *
 * @return  A reference to the response message, or NULL if the transaction terminated without a
 *          response.
 *
 * @note
 *        - To prevent deadlocks, this function can only be used on the client side of a session.
 *          Servers cannot use this function.
 *        - To prevent race conditions, only the client thread that is attached to the session
 *          (the thread that created the session) is allowed to perform a synchronous
 *          request-response transaction.
 *
 * @warning
 *        - The calling (client) thread will be blocked until the server responds, so no other
 *          event handling will happen in that client thread until the response is received (or the
 *          server dies).  Therefore, this function should only be used when the server is certain
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
//--------------------------------------------------------------------------------------------------
{
    // Tell the Session to do a synchronous request-response transaction.
    return msgSession_DoSyncRequestResponse(msgRef->sessionRef, msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends a response back to the client that send the request message.
 *
 * Takes a reference to the request message.  Copy the response payload (if any) into the
 * same payload buffer that held the request payload, then call le_msg_Respond().
 *
 * The messaging system will delete the message automatically when it has finished sending
 * the response.
 *
 * @note    This function can only be used on the server side of a session.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_Respond
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the request message.
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(!le_msg_NeedsResponse(msgRef),
                "Attempt to respond to a message that doesn't need a response.");

    // Send the response message.
    msgSession_SendMessage(msgRef->sessionRef, msgRef);
}
