
// -------------------------------------------------------------------------------------------------
/**
 *  @file requestQueue.h
 *
 *  This module takes care of handling and as required, queuing tree requests from the users of the
 *  config tree API.  So, if a request can not be handled right away, it is queued for later
 *  processing.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013, 2014. All rights reserved. Use of this work is subject
 *  to license.
 */
// -------------------------------------------------------------------------------------------------

#ifndef CFG_REQUEST_QUEUE_INCLUDE_GUARD
#define CFG_REQUEST_QUEUE_INCLUDE_GUARD




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
    le_msg_SessionRef_t sessionRef  ///< Reference to the session that's going away.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Create a transaction.  If it can not be crated now, queue it for later.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleCreateTxnRequest
(
    tu_UserRef_t userRef,              ///< The user to read.
    tdb_TreeRef_t treeRef,             ///< The tree we're working with.
    le_msg_SessionRef_t sessionRef,    ///< The user session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< THe reply context.
    ni_IteratorType_t iterType,        ///< What kind of iterator are we creating?
    const char* pathPtr                ///< Initial path the iterator is pointed at.
);





// -------------------------------------------------------------------------------------------------
/**
 *  Attempt to commit an outstanding write transaction.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleCommitTxnRequest
(
    le_cfg_ServerCmdRef_t commandRef,  ///< Context for the commit reply.
    ni_IteratorRef_t iteratorRef       ///< Pointer to the iterator that's being committed.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Delete an outstanding iterator object, freeing the transaction.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleCancelTxnRequest
(
    le_cfg_ServerCmdRef_t commandRef,  ///< Context for the commit reply.
    ni_IteratorRef_t iteratorRef       ///< Pointer to the iterator that's being deleted.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Delete a node without an explicit transaction.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickDeleteNode
(
    le_msg_SessionRef_t sessionRef,    ///< The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< This handle is used to generate the reply for this
                                       ///<   message.
    tu_UserRef_t userRef,              ///< The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< The tree that we're peforming the action on.
    const char* pathPtr                ///< The path to the node to delete.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Clear out a node's contents and leave it empty.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickSetEmpty
(
    le_msg_SessionRef_t sessionRef,    ///< The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< This handle is used to generate the reply for this
                                       ///<   message.
    tu_UserRef_t userRef,              ///< The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< The tree that we're peforming the action on.
    const char* pathPtr                ///< The path to the node to clear.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Read a string value from the node.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickGetString
(
    le_msg_SessionRef_t sessionRef,    ///< The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< This handle is used to generate the reply for this
                                       ///<   message.
    tu_UserRef_t userRef,              ///< The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< The tree that we're peforming the action on.
    const char* pathPtr,               ///< The path to the node to access.
    size_t maxString,                  ///< Maximum string the caller can handle.
    const char* defaultValuePtr        ///< If the value doesn't exist use this value instead.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Write a string value to a node in the tree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickSetString
(
    le_msg_SessionRef_t sessionRef,    ///< The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< This handle is used to generate the reply for this
                                       ///<   message.
    tu_UserRef_t userRef,              ///< The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< The tree that we're peforming the action on.
    const char* pathPtr,               ///< The path to the node to access.
    const char* valuePtr               ///< The value to set.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get an integer value from the tree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickGetInt
(
    le_msg_SessionRef_t sessionRef,    ///< The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< This handle is used to generate the reply for this
                                       ///<   message.
    tu_UserRef_t userRef,              ///< The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< The tree that we're peforming the action on.
    const char* pathPtr,               ///< The path to the node to access.
    int32_t defaultValue               ///< If the value doesn't exist use this value instead.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Write an integer value to the configTree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickSetInt
(
    le_msg_SessionRef_t sessionRef,    ///< The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< This handle is used to generate the reply for this
                                       ///<   message.
    tu_UserRef_t userRef,              ///< The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< The tree that we're peforming the action on.
    const char* pathPtr,               ///< The path to the node to access.
    int32_t value                      ///< The value to set.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get a floating point value from the tree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickGetFloat
(
    le_msg_SessionRef_t sessionRef,    ///< The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< This handle is used to generate the reply for this
                                       ///<   message.
    tu_UserRef_t userRef,              ///< The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< The tree that we're peforming the action on.
    const char* pathPtr,               ///< The path to the node to access.
    double defaultValue                ///< If the value doesn't exist use this value instead.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Write a floating point value to the configTree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickSetFloat
(
    le_msg_SessionRef_t sessionRef,    ///< The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< This handle is used to generate the reply for this
                                       ///<   message.
    tu_UserRef_t userRef,              ///< The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< The tree that we're peforming the action on.
    const char* pathPtr,               ///< The path to the node to access.
    double value                       ///< The value to set.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get a boolean value from the tree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickGetBool
(
    le_msg_SessionRef_t sessionRef,    ///< The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< This handle is used to generate the reply for this
                                       ///<   message.
    tu_UserRef_t userRef,              ///< The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< The tree that we're peforming the action on.
    const char* pathPtr,               ///< The path to the node to access.
    bool defaultValue                  ///< If the value doesn't exist use this value instead.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Write a boolean value to the configTree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickSetBool
(
    le_msg_SessionRef_t sessionRef,    ///< The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< This handle is used to generate the reply for this
                                       ///<   message.
    tu_UserRef_t userRef,              ///< The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< The tree that we're peforming the action on.
    const char* pathPtr,               ///< The path to the node to access.
    bool value                         ///< The value to set.
);




#endif
