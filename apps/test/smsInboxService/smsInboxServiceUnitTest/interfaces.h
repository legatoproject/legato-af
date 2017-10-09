/**
 * interfaces.h
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#ifndef _INTERFACES_H
#define _INTERFACES_H

#include "le_smsInbox1_interface.h"

#undef LE_KILL_CLIENT
#define LE_KILL_CLIENT LE_ERROR


//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_smsInbox1_GetServiceRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_smsInbox2_GetServiceRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_smsInbox1_GetClientSessionRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_smsInbox2_GetClientSessionRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called whenever one of this service's sessions is closed by
 * the client.  (STUBBED FUNCTION)
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t le_msg_AddServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
);

//--------------------------------------------------------------------------------------------------
/**
 * Reference type for referring to open message box sessions.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_smsInbox2_Session* le_smsInbox2_SessionRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference type used by Add/Remove functions for EVENT 'le_smsInbox2_RxMessage'
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_smsInbox2_RxMessageHandler* le_smsInbox2_RxMessageHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Handler for New Message.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_smsInbox2_RxMessageHandlerFunc_t)
(
    uint32_t msgId,
        ///< Message identifier.
    void* contextPtr
        ///<
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the current selected card.
 *
 * @return Number of the current selected SIM card.
 */
//--------------------------------------------------------------------------------------------------
le_sim_Id_t le_sim_GetSelectedCard
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the first Message object reference in the list of messages
 * retrieved with le_sms_CreateRxMsgList().
 *
 * @return NULL              No message found.
 * @return Msg  Message object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_GetFirst
(
    le_sms_MsgListRef_t msgListRef
        ///< [IN] Messages list.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the next Message object reference in the list of messages
 * retrieved with le_sms_CreateRxMsgList().
 *
 * @return NULL              No message found.
 * @return Msg  Message object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_GetNext
(
    le_sms_MsgListRef_t msgListRef
        ///< [IN] Messages list.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the SIM state.
 *
 * @return The current SIM state.
 *
 */
//--------------------------------------------------------------------------------------------------
le_sim_States_t le_sim_GetState
(
    le_sim_Id_t simId   ///< [IN] SIM identifier.
);

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_sim_NewState'
 *
 * This event provides information on sim state changes.
 *
 */
//--------------------------------------------------------------------------------------------------
le_sim_NewStateHandlerRef_t le_sim_AddNewStateHandler
(
    le_sim_NewStateHandlerFunc_t handlerPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Create an object's reference of the list of received messages
 * saved in the SMS message storage area.
 *
 * @return
 *      Reference to the List object. Null pointer if no messages have been retrieved.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgListRef_t le_sms_CreateRxMsgList
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Reference to a tree iterator object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_cfg_Iterator* le_cfg_IteratorRef_t;

void le_cfg_CommitTxn(le_cfg_IteratorRef_t iteratorRef);

//--------------------------------------------------------------------------------------------------
/**
 * Create a write transaction and open a new iterator for both reading and writing.
 *
 * @note This action creates a write transaction. If the app holds the iterator for
 *        longer than the configured write transaction timeout, the iterator will cancel the
 *        transaction. Other reads will fail to return data, and all writes will be thrown
 *        away.
 *
 * @note A tree transaction is global to that tree; a long-held write transaction will block
 *       other user's write transactions from being started. Other trees in the system
 *       won't be affected.
 *
 * @return This will return a newly created iterator reference.
 */
//--------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t le_cfg_CreateWriteTxn
(
    const char* basePath
        ///< [IN] Path to the location to create the new iterator.
);

//--------------------------------------------------------------------------------------------------
/**
 * Check to see if a given node in the config tree exists.
 *
 * @return True if the specified node exists in the tree. False if not.
 */
//--------------------------------------------------------------------------------------------------
bool le_cfg_NodeExists
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN] Iterator to use as a basis for the transaction.
    const char* path
        ///< [IN] Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.
);

//--------------------------------------------------------------------------------------------------
/**
 * Read a signed integer value from the config tree.
 *
 * If the underlying value is not an integer, the default value will be returned instead. The
 * default value is also returned if the node does not exist or if it's empty.
 *
 * If the value is a floating point value, then it will be rounded and returned as an integer.
 *
 * Valid for both read and write transactions.
 *
 * If the path is empty, the iterator's current node will be read.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_cfg_GetInt
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN] Iterator to use as a basis for the transaction.
    const char* path,
        ///< [IN] Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.
    int32_t defaultValue
        ///< [IN] Default value to use if the original can't be
        ///<   read.
);

//--------------------------------------------------------------------------------------------------
/**
 * Close and free the given iterator object. If the iterator is a write iterator, the transaction
 * will be canceled. If the iterator is a read iterator, the transaction will be closed.
 *
 * @note This operation will also delete the iterator object.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_CancelTxn
(
    le_cfg_IteratorRef_t iteratorRef
        ///< [IN] Iterator object to close.
);

//--------------------------------------------------------------------------------------------------
/**
 * Write a signed integer value to the config tree. Only valid during a
 * write transaction.
 *
 * If the path is empty, the iterator's current node will be set.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_SetInt
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    int32_t value
        ///< [IN]
        ///< Value to write.
);

//--------------------------------------------------------------------------------------------------
/**
 * Close the write iterator and commit the write transaction. This updates the config tree
 * with all of the writes that occured using the iterator.
 *
 * @note This operation will also delete the iterator object.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_CommitTxn
(
    le_cfg_IteratorRef_t iteratorRef
        ///< [IN]
        ///< Iterator object to commit.
);

//--------------------------------------------------------------------------------------------------
/**
 * Create a read transaction and open a new iterator for traversing the config tree.
 *
 * @note This action creates a read lock on the given tree, which will start a read-timeout.
 *        Once the read timeout expires, all active read iterators on that tree will be
 *        expired and the clients will be killed.
 *
 * @note A tree transaction is global to that tree; a long-held read transaction will block other
 *        user's write transactions from being committed.
 *
 * @return This will return a newly created iterator reference.
 */
//--------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t le_cfg_CreateReadTxn
(
    const char* basePath    ///< [IN] Path to the location to create the new iterator.
);
#endif /* interfaces.h */
