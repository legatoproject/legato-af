/** @file messagingSession.c
 *
 * The Session module of the @ref c_messaging implementation.
 *
 * See @ref messaging.c for an overview of the @ref c_messaging implementation.
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
#include "messagingProtocol.h"
#include "messagingMessage.h"
#include "messagingLocal.h"
#include "fileDescriptor.h"


// =======================================
//  PRIVATE DATA
// =======================================

//--------------------------------------------------------------------------------------------------
/// The peak number of outstanding request-response transactions that we expect to have
/// ongoing at the same time in the same process.
//--------------------------------------------------------------------------------------------------
#define MAX_EXPECTED_TXNS 32


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
 * Trace reference used for controlling tracing in this module.
 **/
//--------------------------------------------------------------------------------------------------
static le_log_TraceRef_t TraceRef;

/// Macro used to generate trace output in this module.
/// Takes the same parameters as LE_DEBUG() et. al.
#define TRACE(...) LE_TRACE(TraceRef, ##__VA_ARGS__)


//--------------------------------------------------------------------------------------------------
/**
 * Pool from which Session objects are allocated.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SessionPoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * Transaction Map.  This is a Safe Reference Map used to generate and match up
 * transaction IDs for request-response transactions.
 *
 * @note    Because this is shared by multiple threads, it must be protected using the Mutex.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t TxnMapRef;


//--------------------------------------------------------------------------------------------------
/**
 * A counter that increments every time a change is made to a session list in ANY interface obj.
 */
//--------------------------------------------------------------------------------------------------
static size_t SessionObjListChangeCount = 0;
static size_t* SessionObjListChangeCountRef = &SessionObjListChangeCount;


// =======================================
//  PRIVATE FUNCTIONS
// =======================================

static msgSession_UnixSession_t *msgSession_GetUnixSessionPtr(le_msg_SessionRef_t sessionRef);
static le_msg_SessionRef_t msgSession_GetSessionRef(msgSession_UnixSession_t *unixSessionPtr);

static void AttemptOpen(msgSession_UnixSession_t* sessionPtr);


//--------------------------------------------------------------------------------------------------
/**
 * Pushes a message onto the tail of the Transmit Queue.
 *
 * @note    This is used on both the client side and the server side.
 */
//--------------------------------------------------------------------------------------------------
static void PushTransmitQueue
(
    msgSession_UnixSession_t* sessionPtr,
    le_msg_MessageRef_t     msgRef
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* linkPtr = msgMessage_GetQueueLinkPtr(msgRef);

    LOCK
    le_dls_Queue(&sessionPtr->transmitQueue, linkPtr);
    UNLOCK
}


//--------------------------------------------------------------------------------------------------
/**
 * Pops a message off of the Transmit Queue.
 *
 * @return A reference to the Message object that was popped from the queue, or NULL if the queue
 *         is empty.
 *
 * @note    This is used on both the client side and the server side.
 */
//--------------------------------------------------------------------------------------------------
static le_msg_MessageRef_t PopTransmitQueue
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* linkPtr;

    LOCK
    linkPtr = le_dls_Pop(&sessionPtr->transmitQueue);
    UNLOCK

    if (linkPtr != NULL)
    {
        return msgMessage_GetMessageContainingLink(linkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Puts a message back onto the head of the Transmit Queue.
 *
 * @note    This is used on both the client side and the server side.
 */
//--------------------------------------------------------------------------------------------------
static void UnPopTransmitQueue
(
    msgSession_UnixSession_t* sessionPtr,
    le_msg_MessageRef_t msgRef
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* linkPtr = msgMessage_GetQueueLinkPtr(msgRef);

    LOCK
    le_dls_Stack(&sessionPtr->transmitQueue, linkPtr);
    UNLOCK
}


//--------------------------------------------------------------------------------------------------
/**
 * Pushes a message onto the tail of the Receive Queue.
 */
//--------------------------------------------------------------------------------------------------
static inline void PushReceiveQueue
(
    msgSession_UnixSession_t*   sessionPtr,
    le_msg_MessageRef_t     msgRef
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Queue(&sessionPtr->receiveQueue, msgMessage_GetQueueLinkPtr(msgRef));
}


//--------------------------------------------------------------------------------------------------
/**
 * Pops a message off of the Receive Queue.
 *
 * @return A reference to the Message object that was popped from the queue, or NULL if the queue
 *         is empty.
 */
//--------------------------------------------------------------------------------------------------
static le_msg_MessageRef_t PopReceiveQueue
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* linkPtr;

    linkPtr = le_dls_Pop(&sessionPtr->receiveQueue);

    if (linkPtr != NULL)
    {
        return msgMessage_GetMessageContainingLink(linkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a transaction ID for a given message and stores it inside the Message object.
 */
//--------------------------------------------------------------------------------------------------
static void CreateTxnId
(
    le_msg_MessageRef_t msgRef
)
//--------------------------------------------------------------------------------------------------
{
    LOCK

    msgMessage_SetTxnId(msgRef, le_ref_CreateRef(TxnMapRef, msgRef));

    UNLOCK
}


//--------------------------------------------------------------------------------------------------
/**
 * Looks for a request message that matches a received message's transaction ID.
 *
 * @return  A reference to the matching request message, or NULL if not found.
 */
//--------------------------------------------------------------------------------------------------
static le_msg_MessageRef_t LookupTxnId
(
    le_msg_MessageRef_t msgRef
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t requestMsgRef;

    LOCK
    requestMsgRef = le_ref_Lookup(TxnMapRef, msgMessage_GetTxnId(msgRef));
    UNLOCK

    return requestMsgRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Invalidates the transaction ID of a given message.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteTxnId
(
    le_msg_MessageRef_t msgRef
)
//--------------------------------------------------------------------------------------------------
{
    LOCK

    le_ref_DeleteRef(TxnMapRef, msgMessage_GetTxnId(msgRef));

    UNLOCK
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a given message to a given session's transaction list.
 *
 * This is only done on the client side after the message has been written to the socket.
 * The server side doesn't use a transaction list.
 *
 * @warning The Message object must have already had a transaction ID assigned to it using
 *          CreateTxnId().
 */
//--------------------------------------------------------------------------------------------------
static void AddToTxnList
(
    msgSession_UnixSession_t* sessionPtr,
    le_msg_MessageRef_t msgRef
)
//--------------------------------------------------------------------------------------------------
{
    LOCK

    le_dls_Queue(&sessionPtr->txnList, msgMessage_GetQueueLinkPtr(msgRef));

    UNLOCK
}


//--------------------------------------------------------------------------------------------------
/**
 * Removes a given message from a given session's transaction list and from the Transaction Map.
 */
//--------------------------------------------------------------------------------------------------
static void RemoveFromTxnList
(
    msgSession_UnixSession_t* sessionPtr,
    le_msg_MessageRef_t msgRef
)
//--------------------------------------------------------------------------------------------------
{
    LOCK

    le_dls_Remove(&sessionPtr->txnList, msgMessage_GetQueueLinkPtr(msgRef));

    UNLOCK
}


//--------------------------------------------------------------------------------------------------
/**
 * Removes all messages from the Transaction List, calls their completion callbacks (indicating
 * transaction failure for each) and deletes them.
 *
 * @note    This is used only on the server side.
 */
//--------------------------------------------------------------------------------------------------
static void PurgeTxnList
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (;;)
    {
        le_dls_Link_t* linkPtr;

        LOCK
        linkPtr = le_dls_Pop(&sessionPtr->txnList);
        UNLOCK

        if (linkPtr == NULL)
        {
            break;
        }

        le_msg_MessageRef_t msgRef = msgMessage_GetMessageContainingLink(linkPtr);

        DeleteTxnId(msgRef);

        msgMessage_CallCompletionCallback(msgRef, NULL /* no response */);

        le_msg_ReleaseMsg(msgRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Removes all messages from the Transmit Queue and deletes them.  On the client side, for those
 * Request messages that expect a response, their completion callback will be called (indicating
 * transaction failure).
 *
 * @note    This is used on both the client side and the server side.
 */
//--------------------------------------------------------------------------------------------------
static void PurgeTransmitQueue
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t msgRef;

    while (NULL != (msgRef = PopTransmitQueue(sessionPtr)))
    {
        // On the client side,
        if (sessionPtr->interfaceRef->interfaceType == LE_MSG_INTERFACE_CLIENT)
        {
            // If the message is part of a transaction, that transaction is now terminated
            // and its transaction ID needs to be deleted.
            if ( (msgMessage_GetTxnId(msgRef) != NULL) )
            {
                DeleteTxnId(msgRef);
            }

            // Call the message's completion callback function, if it has one.
            msgMessage_CallCompletionCallback(msgRef, NULL /* no response */);
        }

        // NOTE: Messages never have completion call-backs on the server side, and transaction IDs
        //       are only created and deleted on the client-side.

        le_msg_ReleaseMsg(msgRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Removes all messages from the Receive Queue and deletes them.
 */
//--------------------------------------------------------------------------------------------------
static void PurgeReceiveQueue
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t msgRef;

    while (NULL != (msgRef = PopReceiveQueue(sessionPtr)))
    {
        le_msg_ReleaseMsg(msgRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a Session object.
 *
 * @return  Pointer to the Session object.
 *
 * @note    This is used on both the client side and the server side.
 */
//--------------------------------------------------------------------------------------------------
static msgSession_UnixSession_t* CreateSession
(
    le_msg_InterfaceRef_t       interfaceRef ///< [in] Reference to the session's interface.
)
//--------------------------------------------------------------------------------------------------
{
    msgSession_UnixSession_t* sessionPtr = le_mem_ForceAlloc(SessionPoolRef);

    sessionPtr->session.type = LE_MSG_SESSION_UNIX_SOCKET;
    sessionPtr->link = LE_DLS_LINK_INIT;
    sessionPtr->state = LE_MSG_SESSION_STATE_CLOSED;
    sessionPtr->threadRef = le_thread_GetCurrent();
    sessionPtr->socketFd = -1;
    sessionPtr->fdMonitorRef = NULL;

    sessionPtr->txnList = LE_DLS_LIST_INIT;
    sessionPtr->transmitQueue = LE_DLS_LIST_INIT;
    sessionPtr->receiveQueue = LE_DLS_LIST_INIT;

    sessionPtr->contextPtr = NULL;
    sessionPtr->rxHandler = NULL;
    sessionPtr->rxContextPtr = NULL;
    sessionPtr->openHandler = NULL;
    sessionPtr->openContextPtr = NULL;
    sessionPtr->closeHandler = NULL;
    sessionPtr->closeContextPtr = NULL;

    sessionPtr->interfaceRef = interfaceRef;

    SessionObjListChangeCount++;
    msgInterface_AddSession(interfaceRef, msgSession_GetSessionRef(sessionPtr));

    return sessionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Closes a session.
 *
 * @note    This is used on both the client side and the server side.
 */
//--------------------------------------------------------------------------------------------------
static void CloseSession
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    sessionPtr->state = LE_MSG_SESSION_STATE_CLOSED;

    // Always notify the server on close.
    if (sessionPtr->interfaceRef->interfaceType == LE_MSG_INTERFACE_SERVER)
    {
        // Note: This needs to be done before the FD is closed, in case someone wants to check the
        //       credentials in their callback.
        msgInterface_CallCloseHandler(CONTAINER_OF(sessionPtr->interfaceRef,
                                                   msgInterface_UnixService_t,
                                                   interface),
                                      msgSession_GetSessionRef(sessionPtr));
    }

    // Delete the socket and the FD Monitor.
    if (sessionPtr->fdMonitorRef != NULL)
    {
        le_fdMonitor_Delete(sessionPtr->fdMonitorRef);
        sessionPtr->fdMonitorRef = NULL;
    }
    fd_Close(sessionPtr->socketFd);
    sessionPtr->socketFd = -1;

    // If there are any messages stranded on the transmit queue, the pending transaction list,
    // or the receive queue, clean them all up.
    if (sessionPtr->interfaceRef->interfaceType == LE_MSG_INTERFACE_SERVER)
    {
        PurgeTxnList(sessionPtr);
    }
    PurgeTransmitQueue(sessionPtr);
    PurgeReceiveQueue(sessionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a session object.
 *
 * @note This function is common to both the client side and the server side.
 **/
//--------------------------------------------------------------------------------------------------
static void DeleteSession
(
    msgSession_UnixSession_t* sessionPtr,  ///< Pointer to the Unix Session
    const bool mutexLocked  ///< Indicates whether Mutex is already locked
)
//--------------------------------------------------------------------------------------------------
{
    // Close the session, if it isn't already closed.
    if (sessionPtr->state != LE_MSG_SESSION_STATE_CLOSED)
    {
        CloseSession(sessionPtr);
    }

    // Remove the Session from the Interface's Session List.
    SessionObjListChangeCount++;
    msgInterface_RemoveSession(sessionPtr->interfaceRef,
                               msgSession_GetSessionRef(sessionPtr),
                               mutexLocked);

    // Release the Session object itself.
    le_mem_Release(sessionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates an IPC socket
 **/
//--------------------------------------------------------------------------------------------------
static int CreateSocket
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Create a socket for the session.
    int fd = unixSocket_CreateSeqPacketUnnamed();
    if (fd < 0)
    {
        LE_FATAL("Failed to create socket. Result = %d (%s).", fd, LE_RESULT_TXT(fd));
    }
    else if (fd < 3)
    {
        LE_WARN("Socket opened as standard i/o file descriptor %d!", fd);
    }
    return fd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Connect a local socket to the Service Directory's client connection socket.
 *
 * @return LE_COMM_ERROR if failed to connect to the Service Directory.
 **/
//--------------------------------------------------------------------------------------------------
static le_result_t ConnectToServiceDirectory
(
    int fd  ///< [IN] File descriptor of socket to be connected.
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result = unixSocket_Connect(fd, LE_SVCDIR_CLIENT_SOCKET_NAME);

    if (result != LE_OK)
    {
        LE_DEBUG("Failed to connect to Service Directory. Result = %d (%s).",
                 result,
                 LE_RESULT_TXT(result));

        return LE_COMM_ERROR;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Tells a Session object's FD Monitor to start notifying us when the session's socket FD becomes
 * writeable.
 **/
//--------------------------------------------------------------------------------------------------
static void EnableWriteabilityNotification
(
    msgSession_UnixSession_t*  sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_fdMonitor_Enable(sessionPtr->fdMonitorRef, POLLOUT);
}


//--------------------------------------------------------------------------------------------------
/**
 * Tells a Session object's FD Monitor to stop notifying us when the session's socket FD is
 * writeable.
 **/
//--------------------------------------------------------------------------------------------------
static inline void DisableWriteabilityNotification
(
    msgSession_UnixSession_t*  sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_fdMonitor_Disable(sessionPtr->fdMonitorRef, POLLOUT);
}


//--------------------------------------------------------------------------------------------------
/**
 * Performs a retry on a failed attempt to open a session.
 *
 * @note    This is used only on the client side.
 */
//--------------------------------------------------------------------------------------------------
static void RetryOpen
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    CloseSession(sessionPtr);

    le_msg_InterfaceRef_t interfaceRef =
        le_msg_GetSessionInterface(msgSession_GetSessionRef(sessionPtr));
    LE_ERROR("Retrying connection on interface (%s:%s)...",
             le_msg_GetInterfaceName(interfaceRef),
             le_msg_GetProtocolIdStr(le_msg_GetInterfaceProtocol(interfaceRef)));

    AttemptOpen(sessionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Receives an LE_OK session open response from the server.
 *
 * @note    This is used only on the client side.
 *
 * @return
 * - LE_OK if the session was successfully opened.
 * - LE_UNAVAILABLE if "try" option selected and server not currently offering the service.
 * - LE_NOT_PERMITTED if "try" option selected and client interface not bound to any service.
 * - LE_CLOSED if the connection closed.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReceiveSessionOpenResponse
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    // We expect to receive a very small message (one le_result_t).
    le_result_t serverResponse;
    size_t  bytesReceived = sizeof(serverResponse);

    // Receive the message.
    le_result_t result;
    result = unixSocket_ReceiveDataMsg(sessionPtr->socketFd, &serverResponse, &bytesReceived);

    if (result == LE_OK)
    {
        if (serverResponse == LE_OK)
        {
            le_msg_InterfaceRef_t interfaceRef =
                le_msg_GetSessionInterface(msgSession_GetSessionRef(sessionPtr));
            TRACE("Session opened on interface (%s:%s)",
                  le_msg_GetInterfaceName(interfaceRef),
                  le_msg_GetProtocolIdStr(
                      le_msg_GetSessionProtocol(
                          msgSession_GetSessionRef(sessionPtr))));
        }
        else if ((serverResponse == LE_UNAVAILABLE) || (serverResponse == LE_NOT_PERMITTED))
        {
            result = serverResponse;
        }
        else
        {
            LE_FATAL("Unexpected server response: %d (%s).",
                     serverResponse,
                     LE_RESULT_TXT(serverResponse));
        }
    }
    // If the server died just as it was about to send an OK message, then we'll get LE_CLOSED.
    // Otherwise, it's a fatal error because nothing else should be possible.
    else if (result != LE_CLOSED)
    {
        LE_FATAL("Failed to receive session open response (%s)", LE_RESULT_TXT(result));
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends an LE_OK session open response to the client.
 *
 * @return  LE_OK if successful, LE_COMM_ERROR if failed.
 *
 * @note    This is used only on the server side.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SendSessionOpenResponse
(
    int socketFd    ///< [IN] Connected socket to send through.
)
//--------------------------------------------------------------------------------------------------
{
    const le_result_t response = LE_OK;
    ssize_t bytesSent;

    do
    {
        bytesSent = send(socketFd, &response, sizeof(response), MSG_EOR);
    }
    while ((bytesSent == -1) && (errno == EINTR));

    if (bytesSent < 0)
    {
        // Failed to send!
        LE_ERROR("send() failed. Errno = %d (%m).", errno);
        return LE_COMM_ERROR;
    }
    else
    {
        LE_ASSERT(bytesSent == sizeof(response));
        return LE_OK;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Process a message that was received from a server.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessMessageFromServer
(
    msgSession_UnixSession_t*          sessionPtr,
    le_msg_MessageRef_t msgRef
)
//--------------------------------------------------------------------------------------------------
{
    // This is either an asynchronous response message or an indication message from the server.
    // If it is an asynchronous response, this newly received message will have a matching
    // request message on the Transaction List and in the Transaction Map.

    // Use the Transaction Map to look for the request message.
    le_msg_MessageRef_t requestMsgRef = LookupTxnId(msgRef);
    if (requestMsgRef != NULL)
    {
        // The transaction is complete!  Remove it from the Transaction Map.
        DeleteTxnId(requestMsgRef);

        // Remove the request message from the session's Transaction List.
        RemoveFromTxnList(sessionPtr, requestMsgRef);

        // Call the completion callback function from the request message.
        msgMessage_CallCompletionCallback(requestMsgRef, msgRef);

        // Release the request message.
        le_msg_ReleaseMsg(requestMsgRef);
    }
    // If it is an indication message, pass the indication message to the client's registered
    // receive handler, if there is one.
    else if (sessionPtr->rxHandler != NULL)
    {
        sessionPtr->rxHandler(msgRef, sessionPtr->rxContextPtr);
    }
    // Discard the message if no handler is registered.
    else
    {
        LE_WARN("Discarding indication message from server (%s:%s).",
                le_msg_GetInterfaceName(sessionPtr->interfaceRef),
                le_msg_GetProtocolIdStr(le_msg_GetInterfaceProtocol(sessionPtr->interfaceRef)));
        le_msg_ReleaseMsg(msgRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Process all the messages waiting in the Receive Queue.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessReceivedMessages
(
    msgSession_UnixSession_t*  sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* linkPtr;

    while (NULL != (linkPtr = le_dls_Pop(&sessionPtr->receiveQueue)))
    {
        le_msg_MessageRef_t msgRef = msgMessage_GetMessageContainingLink(linkPtr);

        if (sessionPtr->interfaceRef->interfaceType == LE_MSG_INTERFACE_CLIENT)
        {
            ProcessMessageFromServer(sessionPtr, msgRef);
        }
        else if (sessionPtr->interfaceRef->interfaceType == LE_MSG_INTERFACE_SERVER)
        {
            msgInterface_ProcessMessageFromClient(CONTAINER_OF(sessionPtr->interfaceRef,
                                                               msgInterface_UnixService_t,
                                                               interface),
                                                  msgRef);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Client-side handler for when the server closes a session's socket connection.
 */
//--------------------------------------------------------------------------------------------------
static void ClientSocketHangUp
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    TRACE("Socket closed for session with service (%s:%s).",
          le_msg_GetInterfaceName(sessionPtr->interfaceRef),
          le_msg_GetProtocolIdStr(le_msg_GetInterfaceProtocol(sessionPtr->interfaceRef)));

    switch (sessionPtr->state)
    {
        case LE_MSG_SESSION_STATE_OPENING:
            // If the socket closes during the session opening process, just try again.
            LE_WARN("Session closed while connecting, retrying (%s:%s)",
                    le_msg_GetInterfaceName(sessionPtr->interfaceRef),
                    le_msg_GetProtocolIdStr(le_msg_GetInterfaceProtocol(sessionPtr->interfaceRef)));
            RetryOpen(sessionPtr);
            break;

        case LE_MSG_SESSION_STATE_OPEN:
            // If the session has a close handler registered, then close the session and call
            // the handler.
            if (sessionPtr->closeHandler != NULL)
            {
                CloseSession(sessionPtr);
                sessionPtr->closeHandler(msgSession_GetSessionRef(sessionPtr),
                                         sessionPtr->closeContextPtr);
            }
            // Otherwise, it's a fatal error, because the client is not designed to
            // recover from the session closing down on it.
            else
            {
                LE_FATAL("Session closed by server (%s:%s).",
                         le_msg_GetInterfaceName(sessionPtr->interfaceRef),
                         le_msg_GetProtocolIdStr(
                                            le_msg_GetInterfaceProtocol(sessionPtr->interfaceRef)));
            }
            break;

        case LE_MSG_SESSION_STATE_CLOSED:
            LE_FATAL("Socket closed while closed?!");

        default:
            LE_FATAL("Invalid session state (%d).", sessionPtr->state);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Client-side handler for an error on a session's socket.
 */
//--------------------------------------------------------------------------------------------------
static void ClientSocketError
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    LE_ERROR("Error detected on socket for session with service (%s:%s).",
             le_msg_GetInterfaceName(sessionPtr->interfaceRef),
             le_msg_GetProtocolIdStr(
                 le_msg_GetSessionProtocol(
                     msgSession_GetSessionRef(sessionPtr))));

    switch (sessionPtr->state)
    {
        case LE_MSG_SESSION_STATE_OPENING:
            // If the socket error occurs during the opening process, just try again.
            RetryOpen(sessionPtr);
            break;

        case LE_MSG_SESSION_STATE_OPEN:
            // If the error occurs while the session is open, handle it as a close.
            // NOTE: We are currently running a handler that has the same Context Pointer
            // as the Client Socket Hang Up handler, so we can just call that handler directly.
            ClientSocketHangUp(sessionPtr);
            break;

        case LE_MSG_SESSION_STATE_CLOSED:
            LE_FATAL("Socket error while closed?!");

        default:
            LE_FATAL("Invalid session state (%d).", sessionPtr->state);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Receive messages from the socket and put them on the Receive Queue.
 */
//--------------------------------------------------------------------------------------------------
static void ReceiveMessages
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (;;)
    {
        // Create a Message object.
        le_msg_MessageRef_t msgRef = le_msg_CreateMsg(msgSession_GetSessionRef(sessionPtr));

        // Receive from the socket into the Message object.
        le_result_t result = msgMessage_Receive(sessionPtr->socketFd, msgRef);

        if (result == LE_OK)
        {
            // Received something.  Push it onto the Receive Queue for later processing.
            PushReceiveQueue(sessionPtr, msgRef);
        }
        else
        {
            // Nothing left to receive from the socket.  We are done.
            le_msg_ReleaseMsg(msgRef);
            break;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Server-side handler for when the client closes a session's socket connection.
 */
//--------------------------------------------------------------------------------------------------
static void ServerSocketHangUp
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(sessionPtr->state != LE_MSG_SESSION_STATE_OPEN,
                "Unexpected session state (%d).",
                sessionPtr->state);

    TRACE("Connection closed by client of service (%s:%s).",
          le_msg_GetInterfaceName(sessionPtr->interfaceRef),
          le_msg_GetProtocolIdStr(le_msg_GetInterfaceProtocol(sessionPtr->interfaceRef)));

    DeleteSession(sessionPtr, false);
}


//--------------------------------------------------------------------------------------------------
/**
 * Server-side handler for an error on a session's socket.
 */
//--------------------------------------------------------------------------------------------------
static void ServerSocketError
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(sessionPtr->state != LE_MSG_SESSION_STATE_OPEN,
                "Unexpected session state (%d).",
                sessionPtr->state);

    LE_ERROR("Error detected on socket for session with service (%s:%s).",
             le_msg_GetInterfaceName(sessionPtr->interfaceRef),
             le_msg_GetProtocolIdStr(le_msg_GetSessionProtocol(msgSession_GetSessionRef(sessionPtr))));

    DeleteSession(sessionPtr, false);
}


//--------------------------------------------------------------------------------------------------
/**
 * Send messages from a session's Transmit Queue until either the socket becomes full or there
 * are no more messages waiting on the queue.
 */
//--------------------------------------------------------------------------------------------------
static void SendFromTransmitQueue
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (;;)
    {
        le_msg_MessageRef_t msgRef = PopTransmitQueue(sessionPtr);

        if (msgRef == NULL)
        {
            // Since the Transmit Queue is empty, tell the FD Monitor that we don't need to be
            // notified about writeability anymore.
            DisableWriteabilityNotification(sessionPtr);
            break;
        }

        le_result_t result = msgMessage_Send(sessionPtr->socketFd, msgRef);

        switch (result)
        {
            case LE_OK:
                switch (sessionPtr->interfaceRef->interfaceType)
                {
                    // If this is the client side of the session,
                    case LE_MSG_INTERFACE_CLIENT:
                        // If a response is expected from the other side later, then put this
                        // message on the Transaction List.
                        if (msgMessage_GetTxnId(msgRef) != 0)
                        {
                            AddToTxnList(sessionPtr, msgRef);
                        }
                        // Otherwise, release it.
                        else
                        {
                            le_msg_ReleaseMsg(msgRef);
                        }

                        break;

                    // If this is the server side of the session,
                    case LE_MSG_INTERFACE_SERVER:
                        // Release the message, but first clear out the transaction ID so that
                        // the message knows that it is not being deleted without a reponse message
                        // being sent if one was expected.
                        msgMessage_SetTxnId(msgRef, 0);
                        le_msg_ReleaseMsg(msgRef);

                        break;

                    default:
                        LE_FATAL("Unhandled interface type (%d)",
                                 sessionPtr->interfaceRef->interfaceType);
                }

                break;  // Continue to loop around and send another.

            case LE_NO_MEMORY:
                // Have to wait for the socket to become writeable.  Put the message back on
                // the head of the queue and ask the FD Monitor to tell us when the socket becomes
                // writeable again.
                UnPopTransmitQueue(sessionPtr, msgRef);
                EnableWriteabilityNotification(sessionPtr);

                return;

            case LE_COMM_ERROR:
                // In this case, we expect a handler function to be called by the FD Monitor,
                // so we don't need to handle this case here.  However, we must stop
                // trying to transmit now.  Stick the current message back on the Transmit Queue
                // so it gets cleaned up with the others when the session closes.
                UnPopTransmitQueue(sessionPtr, msgRef);

                return;

            default:
                LE_FATAL("Unexpected return code %d.", result);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Client-side handler for when a Session's socket becomes ready for reading (i.e., handle
 * an incoming message).
 */
//--------------------------------------------------------------------------------------------------
static void ClientSocketReadable
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result = LE_OK;

    switch (sessionPtr->state)
    {
        case LE_MSG_SESSION_STATE_CLOSED:
            // This should never happen!
            LE_FATAL("Unexpected notification for a closed session!");
            break;

        case LE_MSG_SESSION_STATE_OPENING:
            // The Session is waiting for notification from the server that the session
            // has been opened.
            if ((result = ReceiveSessionOpenResponse(sessionPtr)) != LE_OK)
            {
                LE_WARN("Received error %d opening session (%s:%s)",
                        result,
                        le_msg_GetInterfaceName(sessionPtr->interfaceRef),
                        le_msg_GetProtocolIdStr(le_msg_GetSessionProtocol(msgSession_GetSessionRef(sessionPtr))));
                RetryOpen(sessionPtr);
            }
            else
            {
                sessionPtr->state = LE_MSG_SESSION_STATE_OPEN;

                // Call the client's completion callback.
                sessionPtr->openHandler(msgSession_GetSessionRef(sessionPtr), sessionPtr->openContextPtr);
            }
            break;

        case LE_MSG_SESSION_STATE_OPEN:
            // The Session is already open, so this is either an asynchronous response
            // message or an indication message from the server.
            ReceiveMessages(sessionPtr);
            ProcessReceivedMessages(sessionPtr);
            break;

        default:
            LE_FATAL("Unexpected session state (%d).", sessionPtr->state);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Client-side handler for a session's socket becoming writeable.  We only use this event
 * when the socket has previously reported that it couldn't accept any more messages and so
 * we need to queue messages on the session's Transmit Queue until the socket becomes writable
 * again.
 */
//--------------------------------------------------------------------------------------------------
static void ClientSocketWriteable
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    switch (sessionPtr->state)
    {
        case LE_MSG_SESSION_STATE_OPENING:
            // In this case, we don't care about this event.
            break;

        case LE_MSG_SESSION_STATE_OPEN:
            SendFromTransmitQueue(sessionPtr);
            break;

        case LE_MSG_SESSION_STATE_CLOSED:
            LE_FATAL("Socket writeable while closed?!");

        default:
            LE_FATAL("Invalid session state (%d).", sessionPtr->state);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * File descriptor monitoring event handler function for client-side of IPC sockets.
 **/
//--------------------------------------------------------------------------------------------------
static void ClientSocketEventHandler
(
    int fd,         ///< Socket file descriptor.
    short events    ///< Bit map of events that occurred (see 'man 2 poll')
)
//--------------------------------------------------------------------------------------------------
{
    // Get the Session object.
    msgSession_UnixSession_t* sessionPtr = le_fdMonitor_GetContextPtr();

    if (events & POLLIN)
    {
        ClientSocketReadable(sessionPtr);
    }

    if (events & (POLLHUP | POLLRDHUP))
    {
        ClientSocketHangUp(sessionPtr);
    }
    else if (events & POLLERR)
    {
        ClientSocketError(sessionPtr);
    }
    else if (events & POLLOUT)
    {
        ClientSocketWriteable(sessionPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Server-side handler for when a Session's socket becomes ready for reading (i.e., handle
 * an incoming message).
 */
//--------------------------------------------------------------------------------------------------
static void ServerSocketReadable
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(sessionPtr->state != LE_MSG_SESSION_STATE_OPEN,
                "Unexpected session state (%d).",
                sessionPtr->state);

    ReceiveMessages(sessionPtr);
    ProcessReceivedMessages(sessionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Server-side handler for a session's socket becoming writeable.  We only use this event
 * when the socket has previously reported that it couldn't accept any more messages and so
 * we need to queue messages on the session's Transmit Queue until the socket becomes writable
 * again.
 */
//--------------------------------------------------------------------------------------------------
static void ServerSocketWriteable
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(sessionPtr->state != LE_MSG_SESSION_STATE_OPEN,
                "Unexpected session state (%d).",
                sessionPtr->state);

    SendFromTransmitQueue(sessionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * File descriptor monitoring event handler function for server-side of IPC sockets.
 **/
//--------------------------------------------------------------------------------------------------
static void ServerSocketEventHandler
(
    int fd,         ///< Socket file descriptor.
    short events    ///< Bit map of events that occurred (see 'man 2 poll')
)
//--------------------------------------------------------------------------------------------------
{
    // Get the Session object.
    msgSession_UnixSession_t* sessionPtr = le_fdMonitor_GetContextPtr();

    if (events & POLLIN)
    {
        ServerSocketReadable(sessionPtr);
    }

    if (events & (POLLHUP | POLLRDHUP))
    {
        ServerSocketHangUp(sessionPtr);
    }
    else if (events & POLLERR)
    {
        ServerSocketError(sessionPtr);
    }
    else if (events & POLLOUT)
    {
        ServerSocketWriteable(sessionPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Start monitoring for events on a given Session's connected socket.
 *
 * @note    This function is used for both clients and servers.
 */
//--------------------------------------------------------------------------------------------------
static void StartSocketMonitoring
(
    msgSession_UnixSession_t*                  sessionPtr,
    le_fdMonitor_HandlerFunc_t  handlerFunc
)
//--------------------------------------------------------------------------------------------------
{
    const char* interfaceName = le_msg_GetInterfaceName(sessionPtr->interfaceRef);

    sessionPtr->fdMonitorRef = le_fdMonitor_Create(interfaceName,
                                                   sessionPtr->socketFd,
                                                   handlerFunc,
                                                   POLLIN);

    le_fdMonitor_SetContextPtr(sessionPtr->fdMonitorRef, sessionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Start an attempt to open a session by connecting to the Service Directory and sending it
 * a request to open a session.
 *
 * If successful, puts the Session object in the OPENING state, leaves the connection socket open
 * and stores its file descriptor in the Session object.
 *
 * If fails, leaves the Session object in the CLOSED state.
 *
 * @return
 * - LE_OK if successful.
 * - LE_COMM_ERROR if failed to connect to the Service Directory.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartSessionOpenAttempt
(
    msgSession_UnixSession_t* sessionPtr,

    bool shouldWait         ///< true = ask the Service Directory to hold onto the request until
                            ///         the binding or advertisement happens if the client
                            ///         interface is not bound or the server is not advertising
                            ///         the service at this time.
                            ///  false = fail immediately if either a binding or advertisement is
                            ///         missing at this time.
)
//--------------------------------------------------------------------------------------------------
{
    sessionPtr->state = LE_MSG_SESSION_STATE_OPENING;

    // Create a socket for the session.
    sessionPtr->socketFd = CreateSocket();

    // Connect to the Service Directory's client socket.
    le_result_t result = ConnectToServiceDirectory(sessionPtr->socketFd);
    if (result == LE_OK)
    {
        // Create an "Open" request to send to the Service Directory.
        svcdir_OpenRequest_t msg;
        msgInterface_GetInterfaceDetails(sessionPtr->interfaceRef, &(msg.interface));
        msg.shouldWait = shouldWait;

        // Send the request to the Service Directory.
        result = unixSocket_SendDataMsg(sessionPtr->socketFd, &msg, sizeof(msg));
        if (result != LE_OK)
        {
            // NOTE: This is only done when the socket is newly opened, so this shouldn't ever
            //       be LE_NO_MEMORY (send buffers full).
            LE_CRIT("Failed to send session open request to the Service Directory."
                    " Result = %d (%s)",
                    result,
                    LE_RESULT_TXT(result));

            result = LE_COMM_ERROR;
        }
    }

    // On failure, clean up.
    if (result != LE_OK)
    {
        fd_Close(sessionPtr->socketFd);
        sessionPtr->socketFd = -1;

        sessionPtr->state = LE_MSG_SESSION_STATE_CLOSED;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Attempts to open a connection to a service (via the Service Directory's client connection
 * socket).
 */
//--------------------------------------------------------------------------------------------------
static void AttemptOpen
(
    msgSession_UnixSession_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Start the session "Open" attempt.
    if (StartSessionOpenAttempt(sessionPtr, true /* wait for binding or advertisement */ ) == LE_OK)
    {
        // Set the socket non-blocking.
        fd_SetNonBlocking(sessionPtr->socketFd);

        // Start monitoring for events on this socket.
        StartSocketMonitoring(sessionPtr, ClientSocketEventHandler);

        // NOTE: The next step will be for the server to send us an LE_OK "hello" message, or the
        // connection will be closed if something goes wrong.
    }
    else
    {
        LE_FATAL("Unable to connect to the Service Directory.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Attempts to open a session, blocking (not returning) until the attempt is complete.
 *
 * Updates the session state to either OPEN or CLOSED, depending on the result.
 *
 * @return
 * - LE_OK if the session was successfully opened.
 * - LE_UNAVAILABLE if shouldWait and server not currently offering service client i/f bound to.
 * - LE_NOT_PERMITTED if shouldWait and the client interface is not bound to any service.
 * - LE_COMM_ERROR if the Service Directory cannot be reached.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AttemptOpenSync
(
    msgSession_UnixSession_t* sessionPtr,
    bool shouldWait         ///< true = if the client interface is not bound or the server is
                            ///         not advertising the service at this time, then wait
                            ///         until the binding or advertisement happens.
                            ///  false = don't wait for a binding or advertisement if either is
                            ///         missing at this time.
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;

    do
    {
        // Start the session "Open" attempt.
        result = StartSessionOpenAttempt(sessionPtr, shouldWait);

        if (result == LE_OK)
        {
            // Block until a response is received.
            result = ReceiveSessionOpenResponse(sessionPtr);

            // If a server accepted us,
            if (result == LE_OK)
            {
                // Set the socket non-blocking for future operation.
                fd_SetNonBlocking(sessionPtr->socketFd);

                // Start monitoring for events on this socket.
                StartSocketMonitoring(sessionPtr, ClientSocketEventHandler);

                sessionPtr->state = LE_MSG_SESSION_STATE_OPEN;
            }
            else
            {
                CloseSession(sessionPtr);
            }
        }

    } while (result == LE_CLOSED);

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Do deferred processing of the Receive Queue for a session.
 *
 * This is really just a way to kick start processing of messages on the session's
 * Receive Queue later, when they couldn't be processed immediately because a synchronous
 * transaction was underway at the time.
 *
 * The that Receive Queue could have already been drained before this
 * function was run, so don't get upset if there aren't any messages left to process.
 *
 * @warning The Session may have already been closed, reopened, or even deleted since the function
 *          call was queued to the Event Queue.
 *
 * @note    This function is called by the Event Loop as a "queued function".
 *          That's why the parameter list looks unusual.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessDeferredMessages
(
    void* param1Ptr,    ///< [IN] Pointer to a Session object.
    void* param2Ptr     ///< not used
)
//--------------------------------------------------------------------------------------------------
{
    msgSession_UnixSession_t* sessionPtr = param1Ptr;

    ProcessReceivedMessages(sessionPtr);

    // NOTE: Each of these queued functions holds a reference to the session object so that
    //       the session object doesn't go away.  But it could go away as soon as we release it.
    le_mem_Release(sessionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Trigger deferred message queue processing.
 */
//--------------------------------------------------------------------------------------------------
static void TriggerDeferredProcessing
(
    msgSession_UnixSession_t*  sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    // NOTE: Each of these queued functions holds a reference to the session object so that
    //       the session object doesn't go away before the queued function is run.
    le_mem_AddRef(sessionPtr);
    le_event_QueueFunction(ProcessDeferredMessages, sessionPtr, NULL);
}


// =======================================
//  PROTECTED (INTER-MODULE) FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Exposing the session object list change counter; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
size_t** msgSession_GetSessionObjListChgCntRef
(
    void
)
{
    return (&SessionObjListChangeCountRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the messagingSession module.  This must be called only once at start-up, before
 * any other functions in that module are called.
 */
//--------------------------------------------------------------------------------------------------
void msgSession_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    SessionPoolRef = le_mem_CreatePool("Session", sizeof(msgSession_UnixSession_t));
    le_mem_ExpandPool(SessionPoolRef, 10); /// @todo Make this configurable.

    TxnMapRef = le_ref_CreateMap("MsgTxnIDs", MAX_EXPECTED_TXNS);

    // Get a reference to the trace keyword that is used to control tracing in this module.
    TraceRef = le_log_GetTraceRef("messaging");
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks the interface type of a given Session reference.
 *
 * @return  interface object type.
 */
//--------------------------------------------------------------------------------------------------
msgInterface_Type_t msgSession_GetInterfaceType
(
    le_msg_SessionRef_t sessionRef
)
//--------------------------------------------------------------------------------------------------
{
    msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);
    return unixSessionPtr->interfaceRef->interfaceType;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a given Session reference is for an open session.
 *
 * @return  true if the session is still open, false if it is closed.
 */
//--------------------------------------------------------------------------------------------------
bool msgSession_IsOpen
(
    le_msg_SessionRef_t sessionRef
)
//--------------------------------------------------------------------------------------------------
{
    msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);
    return (unixSessionPtr->state == LE_MSG_SESSION_STATE_OPEN);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends a given Message object through a given Session.
 */
//--------------------------------------------------------------------------------------------------
void msgSession_SendMessage
(
    le_msg_SessionRef_t sessionRef,
    le_msg_MessageRef_t messageRef
)
//--------------------------------------------------------------------------------------------------
{
    msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);
    // Only the thread that is handling events on this socket is allowed to send messages through
    // this socket.  This prevents multi-threaded races.
    /// @todo Allow other threads to send?
    LE_FATAL_IF(le_thread_GetCurrent() != unixSessionPtr->threadRef,
                "Attempt to send by thread that doesn't own session '%s'.",
                le_msg_GetInterfaceName(le_msg_GetSessionInterface(sessionRef)));

    if (unixSessionPtr->state != LE_MSG_SESSION_STATE_OPEN)
    {
        LE_DEBUG("Discarding message sent in session that is not open.");

        le_msg_ReleaseMsg(messageRef);
    }
    else
    {
        // Put the message on the Transmit Queue.
        PushTransmitQueue(unixSessionPtr, messageRef);

        // Try to send something from the Transmit Queue.
        SendFromTransmitQueue(unixSessionPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Start an asynchronous request-response transaction.
 */
//--------------------------------------------------------------------------------------------------
void msgSession_RequestResponse
(
    le_msg_SessionRef_t sessionRef,
    le_msg_MessageRef_t msgRef
)
//--------------------------------------------------------------------------------------------------
{
    msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);

    // Only the thread that is handling events on this socket is allowed to do asynchronous
    // transactions on it.
    LE_FATAL_IF(le_thread_GetCurrent() != unixSessionPtr->threadRef,
                "Calling thread doesn't own the session '%s'.",
                le_msg_GetInterfaceName(le_msg_GetSessionInterface(sessionRef)));
    /// @todo Allow other threads to send?

    LE_FATAL_IF(unixSessionPtr->state != LE_MSG_SESSION_STATE_OPEN,
                "Attempt to send message on session that is not open.");

    // Create an ID for this transaction.
    CreateTxnId(msgRef);

    // Put the message on the Transmit Queue.
    PushTransmitQueue(unixSessionPtr, msgRef);

    // Try to send something from the Transmit Queue.
    SendFromTransmitQueue(unixSessionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Do a synchronous request-response transaction.
 */
//--------------------------------------------------------------------------------------------------
le_msg_MessageRef_t msgSession_DoSyncRequestResponse
(
    le_msg_SessionRef_t sessionRef,
    le_msg_MessageRef_t msgRef
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t rxMsgRef;
    msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);

    // Only the thread that is handling events on this socket is allowed to do synchronous
    // transactions on it.
    LE_FATAL_IF(le_thread_GetCurrent() != unixSessionPtr->threadRef,
                "Attempted synchronous operation by thread that doesn't own session '%s'.",
                le_msg_GetInterfaceName(le_msg_GetSessionInterface(sessionRef)));

    // Create an ID for this transaction.
    CreateTxnId(msgRef);

    // Put the socket into blocking mode.
    fd_SetBlocking(unixSessionPtr->socketFd);

    // Send the Request Message.
    msgMessage_Send(unixSessionPtr->socketFd, msgRef);

    // While we have not yet received the response we are waiting for, keep
    // receiving messages.  Any that we receive that don't match the transaction ID
    // that we are waiting for should be queued for later handling using a queued
    // function call.
    for (;;)
    {
        rxMsgRef = le_msg_CreateMsg(sessionRef);

        le_result_t result = msgMessage_Receive(unixSessionPtr->socketFd, rxMsgRef);

        if (result != LE_OK)
        {
            // The socket experienced an error or the connection was closed.
            // No message was received.
            le_msg_ReleaseMsg(rxMsgRef);
            rxMsgRef = NULL;
            break;
        }

        if (msgMessage_GetTxnId(rxMsgRef) == msgMessage_GetTxnId(msgRef))
        {
            // Got the synchronous response we were waiting for.
            break;
        }

        // Got some other message that we weren't waiting for.

        // If the Receive Queue is empty, queue up a function call on the Event Queue so that
        // the Event Loop will kick start processing of the Receive Queue later.
        // (If there's already something on the Receive Queue, then we've already done that.)
        if (le_dls_IsEmpty(&unixSessionPtr->receiveQueue))
        {
            TriggerDeferredProcessing(unixSessionPtr);
        }

        // Queue the received message to the Receive Queue for later processing.
        PushReceiveQueue(unixSessionPtr, rxMsgRef);
    }

    // Invalidate the ID for this transaction.
    DeleteTxnId(msgRef);

    // Don't need the request message anymore.
    le_msg_ReleaseMsg(msgRef);

    // Put the socket back into non-blocking mode.
    fd_SetNonBlocking(unixSessionPtr->socketFd);

    return rxMsgRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the interface reference for a given Session object.
 *
 * @return  The interface reference.
 */
//--------------------------------------------------------------------------------------------------
le_msg_InterfaceRef_t msgSession_GetInterfaceRef
(
    le_msg_SessionRef_t sessionRef
)
//--------------------------------------------------------------------------------------------------
{
    msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);
    return unixSessionPtr->interfaceRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a pointer to the list link inside of a Session object.  This is used to link the
 * Session onto a Service's list of open sessions.
 *
 * @return  Pointer to a link.
 */
//--------------------------------------------------------------------------------------------------
le_dls_Link_t* msgSession_GetListLink
(
    le_msg_SessionRef_t sessionRef
)
//--------------------------------------------------------------------------------------------------
{
    // Make NULL map to NULL (regardless of position of link member) so we can
    // check either session ref or link pointer against NULL.
    if (!sessionRef)
    {
        return NULL;
    }

    msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);
    return &unixSessionPtr->link;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to the Session object in which a given list link exists.
 *
 * @return The reference.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t msgSession_GetSessionContainingLink
(
    le_dls_Link_t* linkPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Make NULL map to NULL (regardless of position of link member) so we can
    // check either session ref or link pointer against NULL.
    if (!linkPtr)
    {
        return NULL;
    }

    return msgSession_GetSessionRef(CONTAINER_OF(linkPtr, msgSession_UnixSession_t, link));
}


//--------------------------------------------------------------------------------------------------
/**
 * Get Unix session pointer from a session reference.
 */
//--------------------------------------------------------------------------------------------------
static msgSession_UnixSession_t *msgSession_GetUnixSessionPtr
(
    le_msg_SessionRef_t sessionRef
)
{
    // Make NULL map to NULL (regardless of position of session member) so we can
    // check either session ref or unix session pointer against NULL.
    if (!sessionRef)
    {
        return NULL;
    }

    LE_FATAL_IF(sessionRef->type != LE_MSG_SESSION_UNIX_SOCKET,
                "Internal error: wrong session type");
    msgSession_UnixSession_t* unixSessionPtr = CONTAINER_OF(sessionRef,
                                                            msgSession_UnixSession_t,
                                                            session);
    return unixSessionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get session reference from unix session pointer
 */
//--------------------------------------------------------------------------------------------------
static le_msg_SessionRef_t msgSession_GetSessionRef
(
    msgSession_UnixSession_t *unixSessionPtr
)
{
    // Make NULL map to NULL (regardless of position of session member) so we can
    // check either session ref or unix session pointer against NULL.
    if (!unixSessionPtr)
    {
        return NULL;
    }

    return &unixSessionPtr->session;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a server-side Session object for a given client connection to a given Service.
 *
 * @return A reference to the newly created Session object, or NULL if failed.
 *
 * @note Closes the file descriptor on failure.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t msgSession_CreateServerSideSession
(
    le_msg_ServiceRef_t serviceRef,
    int                 fd          ///< [IN] File descriptor of socket connected to client.
)
//--------------------------------------------------------------------------------------------------
{
    msgInterface_UnixService_t* servicePtr = CONTAINER_OF(serviceRef,
                                                          msgInterface_UnixService_t,
                                                          service);
    // Send a Hello message (LE_OK) to the client.
    if (SendSessionOpenResponse(fd) != LE_OK)
    {
        // Something went wrong.  Abort.
        fd_Close(fd);
        return NULL;
    }

    // The Hello message was sent successfully.
    // Set the socket non-blocking for future operation.
    fd_SetNonBlocking(fd);

    // Create the Session object (adding it to the Service's list of sessions)
    msgSession_UnixSession_t* sessionPtr = CreateSession(&servicePtr->interface);

    // Record the client connection file descriptor.
    sessionPtr->socketFd = fd;

    // Start monitoring the server-side session connection socket for events.
    StartSocketMonitoring(sessionPtr, ServerSocketEventHandler);

    // The session is officially open.
    sessionPtr->state = LE_MSG_SESSION_STATE_OPEN;

    return msgSession_GetSessionRef(sessionPtr);
}


// =======================================
//  PUBLIC API FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Creates a session that will make use of a protocol to talk to a service on a given client
 * interface.
 *
 * @note    This does not actually attempt to open the session.  It just creates the session
 *          object, allowing the client the opportunity to register handlers for the session
 *          before attempting to open it using le_msg_OpenSession().
 *
 * @return  The Session reference.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_msg_CreateSession
(
    le_msg_ProtocolRef_t    protocolRef,    ///< [in] Reference to the protocol to be used.
    const char*             interfaceName   ///< [in] Name of the client-side interface.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_ClientInterfaceRef_t clientRef = msgInterface_GetClient(protocolRef, interfaceName);

    msgSession_UnixSession_t* sessionPtr = CreateSession(&clientRef->interface);

    msgInterface_Release(&clientRef->interface, false);

    return msgSession_GetSessionRef(sessionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets an opaque context value (void pointer) that can be retrieved from that session later using
 * le_msg_GetSessionContextPtr().
 */
//--------------------------------------------------------------------------------------------------
void le_msg_SetSessionContextPtr
(
    le_msg_SessionRef_t sessionRef, ///< [in] Reference to the session.

    void*               contextPtr  ///< [in] Opaque value to be returned by
                                    ///         le_msg_GetSessionContextPtr().
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(sessionRef);
    switch (sessionRef->type)
    {
        case LE_MSG_SESSION_LOCAL:
            LE_FATAL("SetSessionContextPointer not implemented for local sessions");
            break;
        case LE_MSG_SESSION_UNIX_SOCKET:
            msgSession_GetUnixSessionPtr(sessionRef)->contextPtr = contextPtr;
            break;
        default:
            LE_FATAL("Corrupted session type: %d", sessionRef->type);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Fetches the opaque context value (void pointer) that was set earlier using
 * le_msg_SetSessionContextPtr().
 *
 * @return  The contextPtr value passed into le_msg_SetSessionContextPtr(), or NULL if
 *          le_msg_SetSessionContextPtr() has not been called for this session yet.
 */
//--------------------------------------------------------------------------------------------------
void* le_msg_GetSessionContextPtr
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(sessionRef);
    switch (sessionRef->type)
    {
        case LE_MSG_SESSION_LOCAL:
            LE_FATAL("SetSessionContextPointer not implemented for local sessions");
            break;
        case LE_MSG_SESSION_UNIX_SOCKET:
            return msgSession_GetUnixSessionPtr(sessionRef)->contextPtr;
        default:
            LE_FATAL("Corrupted session type: %d", sessionRef->type);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a session.  This will end the session and free up any resources associated
 * with it.  Any pending request-response transactions in this sesssion will be terminated.
 * If the far end has registered a session close handler callback, then it will be called.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_DeleteSession
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(sessionRef);
    switch (sessionRef->type)
    {
        case LE_MSG_SESSION_LOCAL:
            msgLocal_DeleteSession(sessionRef);
            break;
        case LE_MSG_SESSION_UNIX_SOCKET:
        {
            msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);
            LE_FATAL_IF((unixSessionPtr->interfaceRef->interfaceType == LE_MSG_INTERFACE_SERVER),
                        "Server attempted to delete a session.");
            DeleteSession(unixSessionPtr, false);
            break;
        }
        default:
            LE_FATAL("Corrupted session type: %d", sessionRef->type);
    }
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
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(sessionRef);
    switch (sessionRef->type)
    {
        case LE_MSG_SESSION_LOCAL:
            LE_DEBUG("SetSessionRecv: Local session");
            msgLocal_SetSessionRecvHandler(sessionRef, handlerFunc, contextPtr);
            break;
        case LE_MSG_SESSION_UNIX_SOCKET:
        {
            LE_DEBUG("SetSessionRecv: Unix socket session");
            msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);
            unixSessionPtr->rxHandler = handlerFunc;
            unixSessionPtr->rxContextPtr = contextPtr;
            break;
        }
        default:
            LE_FATAL("Corrupted session type: %d", sessionRef->type);
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the handler callback function to be called when the session is closed from the other
 * end.  A local termination of the session will not trigger this callback.
 *
 * The handler function will be called by the Legato event loop of the thread that created
 * the session.
 *
 * @note
 * - If not set on the client side, then the framework assumes that the client is not designed
 *   to recover from the server terminating the session, and the client process will terminate
 *   if the session is terminated by the server.
 * - This is a client-only function.  Servers are expected to use le_msg_AddServiceCloseHandler()
 *   instead.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_SetSessionCloseHandler
(
    le_msg_SessionRef_t             sessionRef, ///< [in] Reference to the session.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(sessionRef);
    switch (sessionRef->type)
    {
        case LE_MSG_SESSION_LOCAL:
            // Local sessions are within the same process, so cannot be closed.
            break;
        case LE_MSG_SESSION_UNIX_SOCKET:
        {
            msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);
            unixSessionPtr->closeHandler = handlerFunc;
            unixSessionPtr->closeContextPtr = contextPtr;
            break;
        }
        default:
            LE_FATAL("Corrupted session type: %d", sessionRef->type);
    }
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
    LE_ASSERT(sessionRef);
    switch (sessionRef->type)
    {
        case LE_MSG_SESSION_LOCAL:
            messagingLocal_GetSessionCloseHandler(sessionRef, handlerFuncPtr, contextPtrPtr);
            break;
        case LE_MSG_SESSION_UNIX_SOCKET:
        {
            msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);
            *handlerFuncPtr = unixSessionPtr->closeHandler;
            *contextPtrPtr = unixSessionPtr->closeContextPtr;
            break;
        }
        default:
            LE_FATAL("Corrupted session type: %d", sessionRef->type);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Opens a session with a service, providing a function to be called-back when the session is
 * open.
 *
 * @note    Only clients open sessions.  Servers must patiently wait for clients to open sessions
 *          with them.
 *
 * @warning If the client and server don't agree on the protocol ID and maximum message size for the
 *          protocol, an attempt to open a session between that client and server will result in a
 *          fata error being logged and the client process being killed.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_OpenSession
(
    le_msg_SessionRef_t             sessionRef,     ///< [in] Reference to the session.
    le_msg_SessionEventHandler_t    callbackFunc,   ///< [in] Function to be called when open.
                                                    ///       NULL if no notification is needed.
    void*                           contextPtr      ///< [in] Opaque value to pass to the callback.
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(sessionRef);
    switch (sessionRef->type)
    {
        case LE_MSG_SESSION_LOCAL:
            // There's no async open for msgLocal, so open synchronously and immediately call
            // session open calback.
            msgLocal_OpenSessionSync(sessionRef);
            if (callbackFunc)
            {
                callbackFunc(sessionRef, contextPtr);
            }
            break;
        case LE_MSG_SESSION_UNIX_SOCKET:
        {
            msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);
            unixSessionPtr->openHandler = callbackFunc;
            unixSessionPtr->openContextPtr = contextPtr;

            AttemptOpen(unixSessionPtr);
            break;
        }
        default:
            LE_FATAL("Corrupted session type: %d", sessionRef->type);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Synchronously open a session with a service.  Blocks until the session is open or the attempt
 * is rejected.
 *
 * This function logs a fatal error and terminates the calling process if unsuccessful.
 *
 * @note    Only clients open sessions.  Servers must patiently wait for clients to open sessions
 *          with them.
 *
 * @warning If the client and server do not agree on the protocol ID and maximum message size for
 *          the protocol, then an attempt to open a session between that client and server will
 *          result in a fatal error being logged and the client process being killed.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_OpenSessionSync
(
    le_msg_SessionRef_t             sessionRef      ///< [in] Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(sessionRef);
    switch (sessionRef->type)
    {
        case LE_MSG_SESSION_LOCAL:
            msgLocal_OpenSessionSync(sessionRef);
            break;
        case LE_MSG_SESSION_UNIX_SOCKET:
        {
            le_result_t result;
            msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);

            do
            {
                result = AttemptOpenSync(unixSessionPtr, true /* wait if necessary */ );

                if (result != LE_OK)
                {
                    // Failure to connect to the Service Directory is a fatal error.
                    if (result == LE_COMM_ERROR)
                    {
                        LE_FATAL("Failed to connect to the Service Directory.");
                    }

                    // For any other error, report an error and retry.
                    le_msg_InterfaceRef_t interfaceRef = le_msg_GetSessionInterface(sessionRef);
                    LE_ERROR("Session failed (%s). Retrying... (%s:%s)",
                             LE_RESULT_TXT(result),
                             le_msg_GetInterfaceName(interfaceRef),
                             le_msg_GetProtocolIdStr(le_msg_GetInterfaceProtocol(interfaceRef)));
                }

            } while (result != LE_OK);
            break;
        }
        default:
            LE_FATAL("Corrupted session type: %d", sessionRef->type);
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Synchronously open a session with a service.  Does not wait for the session to become available
 * if not available.
 *
 * le_msg_TryOpenSessionSync() differs from le_msg_OpenSessionSync() in that
 * le_msg_TryOpenSessionSync() will not wait for a server session to become available if it is
 * not already available at the time of the call.  That is, if the client's interface is not
 * bound to any service, or if the service that it is bound to is not currently advertised
 * by the server, then le_msg_TryOpenSessionSync() will return an error code, while
 * le_msg_OpenSessionSync() will wait until the binding is created or the server advertises
 * the service (or both).
 *
 * @return
 *  - LE_OK if the session was successfully opened.
 *  - LE_UNAVAILABLE if the server is not currently offering the service to which the client is bound.
 *  - LE_NOT_PERMITTED if the client interface is not bound to any service.
 *  - LE_COMM_ERROR if the Service Directory cannot be reached.
 *
 * @note    Only clients open sessions.  Servers' must patiently wait for clients to open sessions
 *          with them.
 *
 * @warning If the client and server do not agree on the maximum message size for the protocol,
 *          then an attempt to open a session between that client and server will result in a fatal
 *          error being logged and the client process being killed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_msg_TryOpenSessionSync
(
    le_msg_SessionRef_t             sessionRef      ///< [in] Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(sessionRef);
    switch (sessionRef->type)
    {
        case LE_MSG_SESSION_LOCAL:
            return msgLocal_TryOpenSessionSync(sessionRef);
        case LE_MSG_SESSION_UNIX_SOCKET:
        {
            // Attempt a synchronous "Open" for the session.
            msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);
            return AttemptOpenSync(unixSessionPtr,
                                   false /* don't wait for binding or advertisement */ );
        }
        default:
            LE_FATAL("Corrupted session type: %d", sessionRef->type);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Common code for terminating a session.
 */
//--------------------------------------------------------------------------------------------------
static void CloseSessionCommon
(
    le_msg_SessionRef_t sessionRef,  ///< [in] Reference to the session.
    const bool mutexLocked ///< Indicates whether Mutex is already locked
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(sessionRef);
    switch (sessionRef->type)
    {
        case LE_MSG_SESSION_LOCAL:
            msgLocal_CloseSession(sessionRef);
            break;
        case LE_MSG_SESSION_UNIX_SOCKET:
        {
            msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);

            // On the server side, sessions are automatically deleted when they close.
            if (unixSessionPtr->interfaceRef->interfaceType == LE_MSG_INTERFACE_SERVER)
            {
                DeleteSession(unixSessionPtr, mutexLocked);
            }
            else if (unixSessionPtr->state != LE_MSG_SESSION_STATE_CLOSED)
            {
                CloseSession(unixSessionPtr);
            }
            break;
        }
        default:
            LE_FATAL("Corrupted session type: %d", sessionRef->type);
    }
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
//--------------------------------------------------------------------------------------------------
{
    CloseSessionCommon(sessionRef, false);
}


//--------------------------------------------------------------------------------------------------
/**
 * Terminates a session, already having acquired the Mutex lock.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_CloseSessionLocked
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    CloseSessionCommon(sessionRef, true);
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches a reference to the protocol that is being used for a given session.
 *
 * @return A reference to the protocol.
 */
//--------------------------------------------------------------------------------------------------
le_msg_ProtocolRef_t le_msg_GetSessionProtocol
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(sessionRef);
    switch (sessionRef->type)
    {
        case LE_MSG_SESSION_LOCAL:
            // Local sessions don't define a protocol reference
            return NULL;
        case LE_MSG_SESSION_UNIX_SOCKET:
        {
            msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);
            return msgInterface_GetProtocolRef(unixSessionPtr->interfaceRef);
        }
        default:
            LE_FATAL("Corrupted session type: %d", sessionRef->type);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches a reference to the interface that is associated with a given session.
 *
 * @return A reference to the interface.
 */
//--------------------------------------------------------------------------------------------------
le_msg_InterfaceRef_t le_msg_GetSessionInterface
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(sessionRef);
    switch (sessionRef->type)
    {
        case LE_MSG_SESSION_LOCAL:
            return NULL;
        case LE_MSG_SESSION_UNIX_SOCKET:
        {
            msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);
            return unixSessionPtr->interfaceRef;
        }
        default:
            LE_FATAL("Corrupted session type: %d", sessionRef->type);
    }
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
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(sessionRef);
    switch (sessionRef->type)
    {
        case LE_MSG_SESSION_LOCAL:
            // Local session is current user and process
            if (userIdPtr)
            {
                *userIdPtr = geteuid();
            }

            if (processIdPtr)
            {
                *processIdPtr = getpid();
            }
            return LE_OK;
        case LE_MSG_SESSION_UNIX_SOCKET:
        {
            struct ucred credentials;
            socklen_t credSize = sizeof(credentials);
            msgSession_UnixSession_t* unixSessionPtr = msgSession_GetUnixSessionPtr(sessionRef);

            if (unixSessionPtr->interfaceRef->interfaceType == LE_MSG_INTERFACE_CLIENT)
            {
                LE_FATAL("Server-side function called by client.");
            }

            int result = getsockopt(unixSessionPtr->socketFd, SOL_SOCKET, SO_PEERCRED,
                                    &credentials, &credSize);

            if (result == -1)
            {
                if (errno == EBADF)
                {
                    LE_DEBUG("getsockopt() reported EBADF.");
                    return LE_CLOSED;
                }
                else
                {
                    LE_FATAL("getsockopt failed with errno %m for fd %d.",
                             unixSessionPtr->socketFd);
                }
            }

            if (userIdPtr)
            {
                *userIdPtr = credentials.uid;
            }

            if (processIdPtr)
            {
                *processIdPtr = credentials.pid;
            }

            return LE_OK;
        }
        default:
            LE_FATAL("Corrupted session type: %d", sessionRef->type);
    }
}
