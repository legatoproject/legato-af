/**
 * @file messagingCommon.h
 *
 * Declaration of common messaging functions used by all messaging transport mechanisms.
 */

#ifndef LIBLEGATO_MESSAGING_COMMON_H_INCLUDE_GUARD
#define LIBLEGATO_MESSAGING_COMMON_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Generic session object.  Used internally as part of low-level messaging implementaion.
 */
//--------------------------------------------------------------------------------------------------
struct le_msg_Session
{
    enum
    {
        LE_MSG_SESSION_UNIX_SOCKET,
        LE_MSG_SESSION_LOCAL
    } type;
};


//--------------------------------------------------------------------------------------------------
/**
 * Initialize common functionality used by all messaging transport mechanisms.
 */
//--------------------------------------------------------------------------------------------------
void msgCommon_Init
(
    void
);

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
);

#endif /* LIBLEGATO_MESSAGING_COMMON_H_INCLUDE_GUARD */
