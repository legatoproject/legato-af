/** @file messagingSession.c
 *
 * The Session module of the @ref c_messaging implementation.
 *
 * See @ref messaging.c for an overview of the @ref c_messaging implementation.
 *
 * @warning The code in this file @b must be thread safe and re-entrant.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "unixSocket.h"
#include "serviceDirectory/serviceDirectoryProtocol.h"
#include "messagingSession.h"
#include "messagingService.h"
#include "messagingProtocol.h"
#include "messagingMessage.h"
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
 * Enumerates all the possible states that a Session object can be in.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_MSG_SESSION_STATE_CLOSED,    ///< Session is closed.

    LE_MSG_SESSION_STATE_OPENING,   ///< Client is trying to open the session. Waiting for the
                                    ///  server's response. (Note: This is a client-only state.)

    LE_MSG_SESSION_STATE_OPEN,      ///< Session is open.
}
SessionState_t;


//--------------------------------------------------------------------------------------------------
/**
 * Represents a client-server session.
 *
 * This same object is used to track the session on both the server side and the client side.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_msg_Session
{
    le_dls_Link_t                   link;           ///< Used to link into the Session List.
    SessionState_t                  state;          ///< The state that the session is in.
    bool                            isClient;       ///< true = client-side, false = server-side.
    int                             socketFd;       ///< File descriptor for the connected socket.
    le_thread_Ref_t                 threadRef;      ///< The thread that handles this session.
    le_event_FdMonitorRef_t         fdMonitorRef;   ///< File descriptor monitor for the socket.
    le_msg_ServiceRef_t             serviceRef;     ///< The service being accessed.

    le_dls_List_t                   txnList;        ///< List of request messages that have been
                                                    ///  sent and are waiting for their response.

    le_dls_List_t                   transmitQueue;  ///< Queue of messages waiting to be sent.

    le_event_FdHandlerRef_t         writeabilityHandlerRef; ///< Reference for socket fd
                                                            ///  writeability notification handler.
                                                            ///  NULL if no handler is set.

    le_dls_List_t                   receiveQueue;   ///< Queue of received messages waiting to be
                                                    /// processed.

    void*                           contextPtr;     ///< The session's context pointer.
    le_msg_ReceiveHandler_t         rxHandler;      ///< Receive handler function.
    void*                           rxContextPtr;   ///< Receive handler's context pointer.
    le_msg_SessionEventHandler_t    openHandler;    ///< Open handler function.
    void*                           openContextPtr; ///< Open handler's context pointer.
    le_msg_SessionEventHandler_t    closeHandler;   ///< Close handler function.
    void*                           closeContextPtr;///< Close handler's context pointer.
}
Session_t;


//--------------------------------------------------------------------------------------------------
/**
 * Transaction Map.  This is a Safe Reference Map used to generate and match up
 * transaction IDs for request-response transactions.
 *
 * @note    Because this is shared by multiple threads, it must be protected using the Mutex.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t TxnMapRef;


// =======================================
//  PRIVATE FUNCTIONS
// =======================================

static void AttemptOpen(Session_t* sessionPtr);


//--------------------------------------------------------------------------------------------------
/**
 * Pushes a message onto the tail of the Transmit Queue.
 *
 * @note    This is used on both the client side and the server side.
 */
//--------------------------------------------------------------------------------------------------
static void PushTransmitQueue
(
    Session_t*              sessionPtr,
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
    Session_t* sessionPtr
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
    Session_t* sessionPtr,
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
    Session_t*              sessionPtr,
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
    Session_t* sessionPtr
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
    Session_t* sessionPtr,
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
    Session_t* sessionPtr,
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
    Session_t* sessionPtr
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
    Session_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t msgRef;

    while (NULL != (msgRef = PopTransmitQueue(sessionPtr)))
    {
        // On the client side,
        if (sessionPtr->isClient)
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
    Session_t* sessionPtr
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
static Session_t* CreateSession
(
    le_msg_ServiceRef_t     serviceRef,     ///< [in] Reference to the session's service.
    bool                    isClient        ///< [in] true = client-side, false = server-side.
)
//--------------------------------------------------------------------------------------------------
{
    Session_t* sessionPtr = le_mem_ForceAlloc(SessionPoolRef);

    sessionPtr->link = LE_DLS_LINK_INIT;
    sessionPtr->state = LE_MSG_SESSION_STATE_CLOSED;
    sessionPtr->isClient = isClient;
    sessionPtr->threadRef = le_thread_GetCurrent();
    sessionPtr->socketFd = -1;
    sessionPtr->fdMonitorRef = NULL;

    sessionPtr->txnList = LE_DLS_LIST_INIT;
    sessionPtr->transmitQueue = LE_DLS_LIST_INIT;
    sessionPtr->writeabilityHandlerRef = NULL;
    sessionPtr->receiveQueue = LE_DLS_LIST_INIT;

    sessionPtr->contextPtr = NULL;
    sessionPtr->rxHandler = NULL;
    sessionPtr->rxContextPtr = NULL;
    sessionPtr->openHandler = NULL;
    sessionPtr->openContextPtr = NULL;
    sessionPtr->closeHandler = NULL;
    sessionPtr->closeContextPtr = NULL;

    sessionPtr->serviceRef = serviceRef;
    msgService_AddSession(serviceRef, sessionPtr);

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
    Session_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    sessionPtr->state = LE_MSG_SESSION_STATE_CLOSED;

    // Always notify the server on close.
    if (sessionPtr->isClient == false)
    {
        // Note: This needs to be done before the FD is closed, in case someone wants to check the
        //       credentials in their callback.
        msgService_CallCloseHandler(sessionPtr->serviceRef, sessionPtr);
    }

    // Delete the socket and the FD Monitor.
    if (sessionPtr->fdMonitorRef != NULL)
    {
        le_event_DeleteFdMonitor(sessionPtr->fdMonitorRef);
        sessionPtr->fdMonitorRef = NULL;
    }
    fd_Close(sessionPtr->socketFd);
    sessionPtr->socketFd = -1;

    // If there are any messages stranded on the transmit queue, the pending transaction list,
    // or the receive queue, clean them all up.
    if (sessionPtr->isClient == false)
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
    Session_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Close the session, if it isn't already closed.
    if (sessionPtr->state != LE_MSG_SESSION_STATE_CLOSED)
    {
        CloseSession(sessionPtr);
    }

    // Remove the Session from the Service's Session List.
    msgService_RemoveSession(sessionPtr->serviceRef, sessionPtr);

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
 * Calls LE_FATAL() on error.
 **/
//--------------------------------------------------------------------------------------------------
static void ConnectToServiceDirectory
(
    int fd  ///< [IN] File descriptor of socket to be connected.
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result = unixSocket_Connect(fd, LE_SVCDIR_CLIENT_SOCKET_NAME);

    if (result != LE_OK)
    {
        LE_FATAL("Failed to connect to Service Directory. Result = %d (%s).",
                 result,
                 LE_RESULT_TXT(result));
    }

}


static void ClientSocketWriteable(int);
static void ServerSocketWriteable(int);

//--------------------------------------------------------------------------------------------------
/**
 * Tells a Session object's FD Monitor to start notifying us when the session's socket FD becomes
 * writeable.
 **/
//--------------------------------------------------------------------------------------------------
static void EnableWriteabilityNotification
(
    Session_t*  sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (sessionPtr->writeabilityHandlerRef == NULL)
    {
        le_event_FdHandlerFunc_t handlerFunc;

        if (sessionPtr->isClient)
        {
            handlerFunc = ClientSocketWriteable;
        }
        else
        {
            handlerFunc = ServerSocketWriteable;
        }

        sessionPtr->writeabilityHandlerRef = le_event_SetFdHandler(sessionPtr->fdMonitorRef,
                                                                   LE_EVENT_FD_WRITEABLE,
                                                                   handlerFunc);
        le_event_SetFdHandlerContextPtr(sessionPtr->writeabilityHandlerRef, sessionPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Tells a Session object's FD Monitor to stop notifying us when the session's socket FD is
 * writeable.
 **/
//--------------------------------------------------------------------------------------------------
static inline void DisableWriteabilityNotification
(
    Session_t*  sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (sessionPtr->writeabilityHandlerRef != NULL)
    {
        le_event_ClearFdHandler(sessionPtr->writeabilityHandlerRef);
        sessionPtr->writeabilityHandlerRef = NULL;
    }
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
    Session_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    CloseSession(sessionPtr);

    le_msg_ServiceRef_t serviceRef = le_msg_GetSessionService(sessionPtr);
    LE_ERROR("Retrying service (%s:%s)...",
             le_msg_GetServiceName(serviceRef),
             le_msg_GetProtocolIdStr(le_msg_GetServiceProtocol(serviceRef)));

    AttemptOpen(sessionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Receives an LE_OK session open response from the server.
 *
 * @note    This is used only on the client side.
 *
 * @return  LE_OK if successful, something else if failed.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReceiveSessionOpenResponse
(
    Session_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    // We expect to receive a very small message (one LE_OK).
    le_result_t result;
    le_result_t serverResponse;
    size_t  bytesReceived = sizeof(serverResponse);

    result = unixSocket_ReceiveDataMsg(sessionPtr->socketFd, &serverResponse, &bytesReceived);

    if (result == LE_OK)
    {
        if (serverResponse != LE_OK)
        {
            LE_FATAL("Unexpected server response (%d).", serverResponse);
        }

        le_msg_ServiceRef_t serviceRef = le_msg_GetSessionService(sessionPtr);
        TRACE("Session opened with service (%s:%s)",
              le_msg_GetServiceName(serviceRef),
              le_msg_GetProtocolIdStr(le_msg_GetServiceProtocol(serviceRef)));
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
    Session_t*          sessionPtr,
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
                le_msg_GetServiceName(sessionPtr->serviceRef),
                le_msg_GetProtocolIdStr(le_msg_GetServiceProtocol(sessionPtr->serviceRef)));
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
    Session_t*  sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* linkPtr;

    while (NULL != (linkPtr = le_dls_Pop(&sessionPtr->receiveQueue)))
    {
        le_msg_MessageRef_t msgRef = msgMessage_GetMessageContainingLink(linkPtr);

        if (sessionPtr->isClient)
        {
            ProcessMessageFromServer(sessionPtr, msgRef);
        }
        else
        {
            msgService_ProcessMessageFromClient(sessionPtr->serviceRef, msgRef);
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
    int fd      ///< [in] The file descriptor that experienced the event.
)
//--------------------------------------------------------------------------------------------------
{
    // Get the Session object.
    Session_t* sessionPtr = le_event_GetContextPtr();

    TRACE("Socket closed for session with service (%s:%s).",
          le_msg_GetServiceName(sessionPtr->serviceRef),
          le_msg_GetProtocolIdStr(le_msg_GetServiceProtocol(sessionPtr->serviceRef)));

    switch (sessionPtr->state)
    {
        case LE_MSG_SESSION_STATE_OPENING:
            // If the socket closes during the session opening process, just try again.
            RetryOpen(sessionPtr);
            break;

        case LE_MSG_SESSION_STATE_OPEN:
            // If the session has a close handler registered, then close the session and call
            // the handler.
            if (sessionPtr->closeHandler != NULL)
            {
                CloseSession(sessionPtr);
                sessionPtr->closeHandler(sessionPtr, sessionPtr->closeContextPtr);
            }
            // Otherwise, it's a fatal error, because the client is not designed to
            // recover from the session closing down on it.
            else
            {
                LE_FATAL("Session closed by server (%s:%s).",
                         le_msg_GetServiceName(sessionPtr->serviceRef),
                         le_msg_GetProtocolIdStr(le_msg_GetServiceProtocol(sessionPtr->serviceRef)));
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
    int fd      ///< [in] The file descriptor that experienced the event.
)
//--------------------------------------------------------------------------------------------------
{
    // Get the Session object.
    Session_t* sessionPtr = le_event_GetContextPtr();

    LE_ERROR("Error detected on socket for session with service (%s:%s).",
             le_msg_GetServiceName(sessionPtr->serviceRef),
             le_msg_GetProtocolIdStr(le_msg_GetSessionProtocol(sessionPtr)));

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
            ClientSocketHangUp(fd);
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
    Session_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (;;)
    {
        // Create a Message object.
        le_msg_MessageRef_t msgRef = le_msg_CreateMsg(sessionPtr);

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
    int fd      ///< [in] The file descriptor that experienced the event.
)
//--------------------------------------------------------------------------------------------------
{
    // Get the Session object.
    Session_t* sessionPtr = le_event_GetContextPtr();

    LE_FATAL_IF(sessionPtr->state != LE_MSG_SESSION_STATE_OPEN,
                "Unexpected session state (%d).",
                sessionPtr->state);

    TRACE("Connection closed by client of service (%s:%s).",
          le_msg_GetServiceName(sessionPtr->serviceRef),
          le_msg_GetProtocolIdStr(le_msg_GetServiceProtocol(sessionPtr->serviceRef)));

    DeleteSession(sessionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Server-side handler for an error on a session's socket.
 */
//--------------------------------------------------------------------------------------------------
static void ServerSocketError
(
    int fd      ///< [in] The file descriptor that experienced the event.
)
//--------------------------------------------------------------------------------------------------
{
    // Get the Session object.
    Session_t* sessionPtr = le_event_GetContextPtr();

    LE_ERROR("Error detected on socket for session with service (%s:%s).",
             le_msg_GetServiceName(sessionPtr->serviceRef),
             le_msg_GetProtocolIdStr(le_msg_GetSessionProtocol(sessionPtr)));

    LE_FATAL_IF(sessionPtr->state != LE_MSG_SESSION_STATE_OPEN,
                "Unexpected session state (%d).",
                sessionPtr->state);

    DeleteSession(sessionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Send messages from a session's Transmit Queue until either the socket becomes full or there
 * are no more messages waiting on the queue.
 */
//--------------------------------------------------------------------------------------------------
static void SendFromTransmitQueue
(
    Session_t* sessionPtr
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
                // If this is the client side of the session,
                if (sessionPtr->isClient)
                {
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
                }
                // If this is the server side of the session,
                else
                {
                    // Release the message, but first clear out the transaction ID so that
                    // the message knows that it is not being deleted without a reponse message
                    // being sent if one was expected.
                    msgMessage_SetTxnId(msgRef, 0);
                    le_msg_ReleaseMsg(msgRef);
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
    int fd      ///< [in] The file descriptor that experienced the event.
)
//--------------------------------------------------------------------------------------------------
{
    // Get the Session object.
    Session_t* sessionPtr = le_event_GetContextPtr();

    switch (sessionPtr->state)
    {
        case LE_MSG_SESSION_STATE_CLOSED:
            // This should never happen!
            LE_FATAL("Unexpected notification for a closed session!");
            break;

        case LE_MSG_SESSION_STATE_OPENING:
            // The Session is waiting for notification from the server that the session
            // has been opened.
            if (ReceiveSessionOpenResponse(sessionPtr) != LE_OK)
            {
                RetryOpen(sessionPtr);
            }
            else
            {
                sessionPtr->state = LE_MSG_SESSION_STATE_OPEN;

                // Call the client's completion callback.
                sessionPtr->openHandler(sessionPtr, sessionPtr->openContextPtr);
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
 * we need to queue messages on the session't Transmit Queue until the socket becomes writable
 * again.
 */
//--------------------------------------------------------------------------------------------------
static void ClientSocketWriteable
(
    int fd      ///< [in] The file descriptor that experienced the event.
)
//--------------------------------------------------------------------------------------------------
{
    // Get the Session object.
    Session_t* sessionPtr = le_event_GetContextPtr();

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
 * Server-side handler for when a Session's socket becomes ready for reading (i.e., handle
 * an incoming message).
 */
//--------------------------------------------------------------------------------------------------
static void ServerSocketReadable
(
    int fd      ///< [in] The file descriptor that experienced the event.
)
//--------------------------------------------------------------------------------------------------
{
    // Get the Session object.
    Session_t* sessionPtr = le_event_GetContextPtr();

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
 * we need to queue messages on the session't Transmit Queue until the socket becomes writable
 * again.
 */
//--------------------------------------------------------------------------------------------------
static void ServerSocketWriteable
(
    int fd      ///< [in] The file descriptor that experienced the event.
)
//--------------------------------------------------------------------------------------------------
{
    // Get the Session object.
    Session_t* sessionPtr = le_event_GetContextPtr();

    LE_FATAL_IF(sessionPtr->state != LE_MSG_SESSION_STATE_OPEN,
                "Unexpected session state (%d).",
                sessionPtr->state);

    SendFromTransmitQueue(sessionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Start monitoring for readable, writeable, hang-up, and error events on a given Session's
 * connected socket.
 *
 * @note    This function is used for both clients and servers.
 */
//--------------------------------------------------------------------------------------------------
static void StartSocketMonitoring
(
    Session_t*                  sessionPtr,
    le_event_FdHandlerFunc_t    readableHandler,
    le_event_FdHandlerFunc_t    closedHandler,
    le_event_FdHandlerFunc_t    errorHandler
)
//--------------------------------------------------------------------------------------------------
{
    le_event_FdHandlerRef_t handlerRef;

    const char* serviceName = le_msg_GetServiceName(sessionPtr->serviceRef);

    sessionPtr->fdMonitorRef = le_event_CreateFdMonitor(serviceName, sessionPtr->socketFd);

    handlerRef = le_event_SetFdHandler(sessionPtr->fdMonitorRef,
                                       LE_EVENT_FD_READABLE,
                                       readableHandler);
    le_event_SetFdHandlerContextPtr(handlerRef, sessionPtr);

    handlerRef = le_event_SetFdHandler(sessionPtr->fdMonitorRef,
                                       LE_EVENT_FD_READ_HANG_UP,
                                       closedHandler);
    le_event_SetFdHandlerContextPtr(handlerRef, sessionPtr);

    handlerRef = le_event_SetFdHandler(sessionPtr->fdMonitorRef,
                                       LE_EVENT_FD_WRITE_HANG_UP,
                                       closedHandler);
    le_event_SetFdHandlerContextPtr(handlerRef, sessionPtr);

    handlerRef = le_event_SetFdHandler(sessionPtr->fdMonitorRef,
                                       LE_EVENT_FD_ERROR,
                                       errorHandler);
    le_event_SetFdHandlerContextPtr(handlerRef, sessionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Attempts to open a connection to a service (via the Service Directory's client connection
 * socket).
 */
//--------------------------------------------------------------------------------------------------
static void AttemptOpen
(
    Session_t* sessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Create a socket for the session.
    sessionPtr->socketFd = CreateSocket();

    // Connect to the Service Directory's client socket.
    ConnectToServiceDirectory(sessionPtr->socketFd);

    // Send the service identification information to the Service Directory.
    msgService_SendServiceId(sessionPtr->serviceRef, sessionPtr->socketFd);

    // Set the socket non-blocking.
    fd_SetNonBlocking(sessionPtr->socketFd);

    // Start monitoring for events on this socket.
    StartSocketMonitoring(sessionPtr,
                          ClientSocketReadable,
                          ClientSocketHangUp,
                          ClientSocketError);

    // NOTE: The next step will be for the server to send us an LE_OK "hello" message, or the
    // connection will be closed if something goes wrong.
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
    Session_t* sessionPtr = param1Ptr;

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
    Session_t*  sessionPtr
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
    SessionPoolRef = le_mem_CreatePool("Session", sizeof(Session_t));
    le_mem_ExpandPool(SessionPoolRef, 10); /// @todo Make this configurable.

    TxnMapRef = le_ref_CreateMap("MsgTxnIDs", MAX_EXPECTED_TXNS);

    // Get a reference to the trace keyword that is used to control tracing in this module.
    TraceRef = le_log_GetTraceRef("messaging");
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a given Session reference is for the client side or the server side of a session.
 *
 * @return  true if client-side, false if server-side.
 */
//--------------------------------------------------------------------------------------------------
bool msgSession_IsClient
(
    le_msg_SessionRef_t sessionRef
)
//--------------------------------------------------------------------------------------------------
{
    return sessionRef->isClient;
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
    return (sessionRef->state == LE_MSG_SESSION_STATE_OPEN);
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
    // Only the thread that is handling events on this socket is allowed to send messages through
    // this socket.  This prevents multi-threaded races.
    /// @todo Allow other threads to send?
    LE_FATAL_IF(le_thread_GetCurrent() != sessionRef->threadRef,
                "Attempt to send by thread that doesn't own session '%s'.",
                le_msg_GetServiceName(le_msg_GetSessionService(sessionRef)));

    if (sessionRef->state != LE_MSG_SESSION_STATE_OPEN)
    {
        LE_DEBUG("Discarding message sent in session that is not open.");

        le_msg_ReleaseMsg(messageRef);
    }
    else
    {
        // Put the message on the Transmit Queue.
        PushTransmitQueue(sessionRef, messageRef);

        // Try to send something from the Transmit Queue.
        SendFromTransmitQueue(sessionRef);
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
    // Only the thread that is handling events on this socket is allowed to do asynchronous
    // transactions on it.
    LE_FATAL_IF(le_thread_GetCurrent() != sessionRef->threadRef,
                "Calling thread  doesn't own the session '%s'.",
                le_msg_GetServiceName(le_msg_GetSessionService(sessionRef)));
    /// @todo Allow other threads to send?

    LE_FATAL_IF(sessionRef->state != LE_MSG_SESSION_STATE_OPEN,
                "Attempt to send message on session that is not open.");

    // Create an ID for this transaction.
    CreateTxnId(msgRef);

    // Put the message on the Transmit Queue.
    PushTransmitQueue(sessionRef, msgRef);

    // Try to send something from the Transmit Queue.
    SendFromTransmitQueue(sessionRef);
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

    // Only the thread that is handling events on this socket is allowed to do synchronous
    // transactions on it.
    LE_FATAL_IF(le_thread_GetCurrent() != sessionRef->threadRef,
                "Attempted synchronous operation by thread that doesn't own session '%s'.",
                le_msg_GetServiceName(le_msg_GetSessionService(sessionRef)));

    // Create an ID for this transaction.
    CreateTxnId(msgRef);

    // Put the socket into blocking mode.
    fd_SetBlocking(sessionRef->socketFd);

    // Send the Request Message.
    msgMessage_Send(sessionRef->socketFd, msgRef);

    // While we have not yet received the response we are waiting for, keep
    // receiving messages.  Any that we receive that don't match the transaction ID
    // that we are waiting for should be queued for later handling using a queued
    // function call.
    for (;;)
    {
        rxMsgRef = le_msg_CreateMsg(sessionRef);

        le_result_t result = msgMessage_Receive(sessionRef->socketFd, rxMsgRef);

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
        if (le_dls_IsEmpty(&sessionRef->receiveQueue))
        {
            TriggerDeferredProcessing(sessionRef);
        }

        // Queue the received message to the Receive Queue for later processing.
        PushReceiveQueue(sessionRef, rxMsgRef);
    }

    // Invalidate the ID for this transaction.
    DeleteTxnId(msgRef);

    // Don't need the request message anymore.
    le_msg_ReleaseMsg(msgRef);

    // Put the socket back into non-blocking mode.
    fd_SetNonBlocking(sessionRef->socketFd);

    return rxMsgRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the service reference for a given Session object.
 *
 * @return  The service reference.
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t msgSession_GetServiceRef
(
    le_msg_SessionRef_t sessionRef
)
//--------------------------------------------------------------------------------------------------
{
    return sessionRef->serviceRef;
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
    return &sessionRef->link;
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
    return CONTAINER_OF(linkPtr, Session_t, link);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a server-side Session object for a given client connection to a given Service.
 *
 * @return A reference to the newly created Session object, or NULL if failed.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t msgSession_CreateServerSideSession
(
    le_msg_ServiceRef_t serviceRef,
    int                 fd          ///< [IN] File descriptor of socket connected to client.
)
//--------------------------------------------------------------------------------------------------
{
    // Send a Hello message (LE_OK) to the client.
    if (SendSessionOpenResponse(fd) != LE_OK)
    {
        // Something went wrong.  Abort.
        return NULL;
    }

    // The Hello message was sent successfully.
    // Set the socket non-blocking for future operation.
    fd_SetNonBlocking(fd);

    // Create the Session object (adding it to the Service's list of sessions)
    Session_t* sessionPtr = CreateSession(serviceRef, false /* isClient */ );

    // Record the client connection file descriptor.
    sessionPtr->socketFd = fd;

    // Start monitoring the server-side session connection socket for events.
    StartSocketMonitoring(sessionPtr,
                          ServerSocketReadable,
                          ServerSocketHangUp,
                          ServerSocketError);

    // The session is officially open.
    sessionPtr->state = LE_MSG_SESSION_STATE_OPEN;

    return sessionPtr;
}


// =======================================
//  PUBLIC API FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Creates a session that will make use of a given protocol to talk to a given service.
 *
 * @note    This does not actually attempt to open the session.  It just creates the session
 *          object, allowing the client the opportunity to register handlers for the session
 *          before attempting to open it using le_msg_OpenSession().
 *
 * @return  The service reference.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_msg_CreateSession
(
    le_msg_ProtocolRef_t    protocolRef,    ///< [in] Reference to the protocol to be used.
    const char*             serviceName     ///< [in] Name of the service instance.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_ServiceRef_t serviceRef = msgService_GetService(protocolRef, serviceName);

    Session_t* sessionPtr = CreateSession(serviceRef, true /* isClient */);

    msgService_Release(serviceRef);

    return sessionPtr;
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
    sessionRef->contextPtr = contextPtr;
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
    return sessionRef->contextPtr;
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
    LE_FATAL_IF(!(sessionRef->isClient), "Server attempted to delete a session.");

    DeleteSession(sessionRef);
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
 *
 * @todo    Should we allow servers to use le_msg_SetSessionRecvHandler() too?
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
    sessionRef->rxHandler = handlerFunc;
    sessionRef->rxContextPtr = contextPtr;
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
 *
 * @todo    Should we allow servers to use le_msg_SetSessionCloseHandler() too?
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
    sessionRef->closeHandler = handlerFunc;
    sessionRef->closeContextPtr = contextPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Opens a session with a service, providing a function to be called-back when the session is
 * open.
 *
 * @note    Only clients open sessions.  Servers' must patiently wait for clients to open sessions
 *          with them.
 *
 * @warning If the client and server do not agree on the maximum message size for the protocol,
 *          then an attempt to open a session between that client and server will result in a fatal
 *          error being logged and the client process being killed.
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
    sessionRef->openHandler = callbackFunc;
    sessionRef->openContextPtr = contextPtr;

    sessionRef->state = LE_MSG_SESSION_STATE_OPENING;

    AttemptOpen(sessionRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Synchronously open a session with a service.  Blocks until the session is open or the attempt
 * is rejected.
 *
 * This function logs a fatal error and terminates the calling process if unsuccessful.
 *
 * @note    Only clients open sessions.  Servers' must patiently wait for clients to open sessions
 *          with them.
 *
 * @warning If the client and server do not agree on the maximum message size for the protocol,
 *          then an attempt to open a session between that client and server will result in a fatal
 *          error being logged and the client process being killed.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_OpenSessionSync
(
    le_msg_SessionRef_t             sessionRef      ///< [in] Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;

    sessionRef->state = LE_MSG_SESSION_STATE_OPENING;

    do
    {
        // Create a socket for the session.
        sessionRef->socketFd = CreateSocket();

        // Connect to the Service Directory's client socket.
        ConnectToServiceDirectory(sessionRef->socketFd);

        // Send the service identification information to the Service Directory.
        msgService_SendServiceId(sessionRef->serviceRef, sessionRef->socketFd);

        // Block until a response is received.
        result = ReceiveSessionOpenResponse(sessionRef);

        // If a server accepted us,
        if (result == LE_OK)
        {
            // Set the socket non-blocking for future operation.
            fd_SetNonBlocking(sessionRef->socketFd);

            // Start monitoring for events on this socket.
            StartSocketMonitoring(sessionRef,
                                  ClientSocketReadable,
                                  ClientSocketHangUp,
                                  ClientSocketError);

            sessionRef->state = LE_MSG_SESSION_STATE_OPEN;
        }
        // Failed attempt.  Retry.
        else
        {
            CloseSession(sessionRef);

            le_msg_ServiceRef_t serviceRef = le_msg_GetSessionService(sessionRef);
            LE_ERROR("Retrying service (%s:%s)...",
                     le_msg_GetServiceName(serviceRef),
                     le_msg_GetProtocolIdStr(le_msg_GetServiceProtocol(serviceRef)));

        }

    } while (result != LE_OK);
}


//--------------------------------------------------------------------------------------------------
/**
 * Attempt to synchronously open a session with a service, but just quietly return an error
 * code if the Service Directory is not running or is unreachable for some other reason.
 *
 * This is needed by the Log API, since logging should work even if the Service Directory isn't
 * running.
 *
 * @return  LE_OK if successful.
 *          LE_COMM_ERROR if the Service Directory cannot be reached.
 *
 * @note    Only clients open sessions.  Servers' must patiently wait for clients to open sessions
 *          with them.
 *
 * @warning If the client and server do not agree on the maximum message size for the protocol,
 *          then an attempt to open a session between that client and server will result in a fatal
 *          error being logged and the client process being killed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t msgSession_TryOpenSessionSync
(
    le_msg_SessionRef_t             sessionRef      ///< [in] Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;

    sessionRef->state = LE_MSG_SESSION_STATE_OPENING;

    do
    {
        // Create a socket for the session.
        sessionRef->socketFd = CreateSocket();

        // Connect to the Service Directory's client socket.
        result = unixSocket_Connect(sessionRef->socketFd, LE_SVCDIR_CLIENT_SOCKET_NAME);
        if (result != LE_OK)
        {
            LE_DEBUG("Failed to open connection to Service Directory (%s).", LE_RESULT_TXT(result));
            return LE_COMM_ERROR;
        }

        // Send the service identification information to the Service Directory.
        msgService_SendServiceId(sessionRef->serviceRef, sessionRef->socketFd);

        // Block until a response is received.
        result = ReceiveSessionOpenResponse(sessionRef);

        // If a server accepted us,
        if (result == LE_OK)
        {
            // Set the socket non-blocking for future operation.
            fd_SetNonBlocking(sessionRef->socketFd);

            // Start monitoring for events on this socket.
            StartSocketMonitoring(sessionRef,
                                  ClientSocketReadable,
                                  ClientSocketHangUp,
                                  ClientSocketError);

            sessionRef->state = LE_MSG_SESSION_STATE_OPEN;
        }
        // Failed attempt.  Retry.
        else
        {
            CloseSession(sessionRef);

            le_msg_ServiceRef_t serviceRef = le_msg_GetSessionService(sessionRef);
            LE_ERROR("Retrying service (%s:%s)...",
                     le_msg_GetServiceName(serviceRef),
                     le_msg_GetProtocolIdStr(le_msg_GetServiceProtocol(serviceRef)));

        }

    } while (result != LE_OK);

    return LE_OK;
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
    // On the server side, sessions are automatically deleted when they close.
    if (sessionRef->isClient == false)
    {
        DeleteSession(sessionRef);
    }
    else if (sessionRef->state != LE_MSG_SESSION_STATE_CLOSED)
    {
        CloseSession(sessionRef);
    }
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
    return msgService_GetProtocolRef(sessionRef->serviceRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches a reference to the service that is associated with a given session.
 *
 * @return A reference to the service.
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_msg_GetSessionService
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
)
//--------------------------------------------------------------------------------------------------
{
    return sessionRef->serviceRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Fetches the user ID of the client at the far end of a given IPC session.
 *
 * @warning This function can only be called for the server-side of a session.
 *
 * @return LE_OK if successful.
 *         LE_CLOSED if the session has closed.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t le_msg_GetClientUserId
(
    le_msg_SessionRef_t sessionRef, ///< [in] Reference to the session.
    uid_t*              userIdPtr   ///< [out] Ptr to where the result is to be stored on success.
)
{
    return le_msg_GetClientUserCreds( sessionRef, userIdPtr, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Fetches the user PID of the client at the far end of a given IPC session.
 *
 * @warning This function can only be called for the server-side of a session.
 *
 * @return LE_OK if successful.
 *         LE_CLOSED if the session has closed.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t le_msg_GetClientProcessId
(
    le_msg_SessionRef_t sessionRef,   ///< [in] Reference to the session.
    pid_t*              processIdPtr  ///< [out] Ptr to where the result is to be stored on success.
)
{
    return le_msg_GetClientUserCreds( sessionRef, NULL, processIdPtr);
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
    struct ucred credentials;
    socklen_t credSize = sizeof(credentials);

    if (sessionRef->isClient)
    {
        LE_FATAL("Server-side function called by client.");
    }

    int result = getsockopt(sessionRef->socketFd, SOL_SOCKET, SO_PEERCRED, &credentials, &credSize);

    if (result == -1)
    {
        if (errno == EBADF)
        {
            LE_DEBUG("getsockopt() reported EBADF.");
            return LE_CLOSED;
        }
        else
        {
            LE_FATAL("getsockopt failed with errno %m for fd %d.", sessionRef->socketFd);
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
