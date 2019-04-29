/**
 * @file messagingCommon.c
 *
 * Common messaging functions used by all messaging transport mechanisms.
 */

#include "legato.h"

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
 * Initialize common functionality used by all messaging transport mechanisms.
 */
//--------------------------------------------------------------------------------------------------
void msgCommon_Init
(
    void
)
{
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
 * Call the receive function for a message.
 *
 * This sets the message in thread-local storage
 */
//--------------------------------------------------------------------------------------------------
void msgCommon_CallRecvHandler
(
    le_msg_ReceiveHandler_t  recvHandler,   ///< [IN] Handler for the received message.
    le_msg_MessageRef_t      msgRef,        ///< [IN] Message reference for the received message.
    void*                    recvContextPtr ///< [IN] contextPtr parameter for recvHandler
)
{
    // Set the thread-local received message reference so it can be retrieved by the handler.
    pthread_setspecific(ThreadLocalRxMsgKey, msgRef);

    // Call the handler function.
    recvHandler(msgRef, recvContextPtr);

    // Clear the thread-local reference.
    pthread_setspecific(ThreadLocalRxMsgKey, NULL);
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
    return le_msg_GetClientUserCreds(sessionRef, userIdPtr, NULL);
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
    return le_msg_GetClientUserCreds(sessionRef, NULL, processIdPtr);
}
