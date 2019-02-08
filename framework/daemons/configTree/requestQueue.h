
// -------------------------------------------------------------------------------------------------
/**
 *  @file requestQueue.h
 *
 *  This module takes care of handling and as required, queuing tree requests from the users of the
 *  config tree API.  So, if a request can not be handled right away, it is queued for later
 *  processing.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#ifndef CFG_REQUEST_QUEUE_INCLUDE_GUARD
#define CFG_REQUEST_QUEUE_INCLUDE_GUARD


// -------------------------------------------------------------------------------------------------
/**
 *  These are the types of queueable actions that can be queued against the tree.
 */
// -------------------------------------------------------------------------------------------------
typedef enum
{
    RQ_INVALID,

    RQ_CREATE_WRITE_TXN,
    RQ_COMMIT_WRITE_TXN,
    RQ_CREATE_READ_TXN,
    RQ_DELETE_TXN,

    RQ_DELETE_NODE,
    RQ_SET_EMPTY,
    RQ_SET_STRING,
    RQ_SET_BINARY,
    RQ_SET_INT,
    RQ_SET_FLOAT,
    RQ_SET_BOOL
}
RequestType_t;


//--------------------------------------------------------------------------------------------------
/**
 *  Initialize the memory pools needed by this subsystem.
 */
//--------------------------------------------------------------------------------------------------
void rq_Init
(
    void
);




// -------------------------------------------------------------------------------------------------
/**
 *  Whenever a configAPI session is closed, this function is called to do the clean up work.  Any
 *  active requests for that session are automatically canceled.
 */
// -------------------------------------------------------------------------------------------------
void rq_CleanUpForSession
(
    le_msg_SessionRef_t sessionRef  ///< [IN] Reference to the session that's going away.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Create a transaction.  If it can not be crated now, queue it for later.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleCreateTxnRequest
(
    tu_UserRef_t userRef,              ///< [IN] The user to read.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree we're working with.
    le_msg_SessionRef_t sessionRef,    ///< [IN] The user session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] THe reply context.
    ni_IteratorType_t iterType,        ///< [IN] What kind of iterator are we creating?
    const char* pathPtr                ///< [IN] Initial path the iterator is pointed at.
);





// -------------------------------------------------------------------------------------------------
/**
 *  Attempt to commit an outstanding write transaction.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleCommitTxnRequest
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Context for the commit reply.
    ni_IteratorRef_t iteratorRef       ///< [IN] Pointer to the iterator that's being committed.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Delete an outstanding iterator object, freeing the transaction.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleCancelTxnRequest
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Context for the commit reply.
    ni_IteratorRef_t iteratorRef       ///< [IN] Pointer to the iterator that's being deleted.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Delete a node without an explicit transaction.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickDeleteNode
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr                ///< [IN] The path to the node to delete.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Clear out a node's contents and leave it empty.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickSetEmpty
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr                ///< [IN] The path to the node to clear.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Read a string value from the node.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickGetString
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    size_t maxString,                  ///< [IN] Maximum string the caller can handle.
    const char* defaultValuePtr        ///< [IN] If the value doesn't exist use this value instead.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Write a string or binary value to a node in the tree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickSetData
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    const char* valuePtr,              ///< [IN] The value to set.
    RequestType_t reqType              ///< [IN] Request type: whether this a string or
                                       ///<      an encoded binary data.
);



// -------------------------------------------------------------------------------------------------
/**
 *  Read a binary data from the node.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickGetBinary
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    size_t maxBinary,                  ///< [IN] Maximum data the caller can handle.
    const uint8_t* defaultValuePtr,    ///< [IN] If the value doesn't exist use this value instead.
    size_t defaultValueSize            ///< [IN] Default value size
);



// -------------------------------------------------------------------------------------------------
/**
 *  Get an integer value from the tree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickGetInt
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    int32_t defaultValue               ///< [IN] If the value doesn't exist use this value instead.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Write an integer value to the configTree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickSetInt
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    int32_t value                      ///< [IN] The value to set.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get a floating point value from the tree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickGetFloat
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    double defaultValue                ///< [IN] If the value doesn't exist use this value instead.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Write a floating point value to the configTree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickSetFloat
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    double value                       ///< [IN] The value to set.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get a boolean value from the tree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickGetBool
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    bool defaultValue                  ///< [IN] If the value doesn't exist use this value instead.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Write a boolean value to the configTree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickSetBool
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    bool value                         ///< [IN] The value to set.
);




#endif
