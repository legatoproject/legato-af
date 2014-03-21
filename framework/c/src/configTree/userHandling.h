
// -------------------------------------------------------------------------------------------------
/*
 *  This file holds the code that maintains the request queuing and user information management.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#ifndef CFG_USER_HANDLING_INCLUDE_GUARD
#define CFG_USER_HANDLING_INCLUDE_GUARD




// -------------------------------------------------------------------------------------------------
/**
 *  Initialize the memory pools needed by this subsystem.
 */
// -------------------------------------------------------------------------------------------------
void UserTreeInit
(
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the info for the user for this connection.
 */
// -------------------------------------------------------------------------------------------------
UserInfo_t* GetCurrentUserInfo
(
);




// -------------------------------------------------------------------------------------------------
/**
 *  Check the path to see if the user specified a specific tree to use.
 */
// -------------------------------------------------------------------------------------------------
bool PathHasTreeSpecifier
(
    const char* pathPtr  ///< The path to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the tree the user requested in the supplied path.  If no tree was actually speified, return
 *  the user default one instead.
 */
// -------------------------------------------------------------------------------------------------
TreeInfo_t* GetRequestedTree
(
    UserInfo_t* userPtr,  ///< The user info to query.
    const char* pathPtr   ///< The path to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the default tree for the given user.
 */
// -------------------------------------------------------------------------------------------------
TreeInfo_t* GetUserDefaultTree
(
    UserInfo_t* userPtr  ///< The user to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Return a path pointer that excludes the tree name.  This function does not allocate a new
 *  string but instead returns a pointer into the supplied path string.
 */
// -------------------------------------------------------------------------------------------------
const char* GetPathOnly
(
    const char* pathPtr  ///< The path string.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Create a transaction.  If it can not be crated now, queue it for later.
 */
// -------------------------------------------------------------------------------------------------
void HandleCreateTxnRequest
(
    UserInfo_t* userPtr,              ///< The user to read.
    TreeInfo_t* treePtr,              ///< The tree we're working with.
    le_msg_SessionRef_t sessionRef,   ///< The user session this request occured on.
    le_cfg_Context_t contextRef,      ///< THe reply context.
    IteratorType_t iteratorType,      ///< What kind of iterator are we creating?
    const char* basePathPtr           ///< Initial path the iterator is pointed at.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Attempt to commit an outstanding write transaction.
 */
// -------------------------------------------------------------------------------------------------
void HandleCommitTxnRequest
(
    UserInfo_t* userPtr,              ///< The user that's requesting the commit.
    le_msg_SessionRef_t sessionRef,   ///< The user session this request occured on.
    le_cfg_Context_t contextRef,      ///< Context for the commit reply.
    le_cfg_iteratorRef_t iteratorRef  ///< Reference to the iterator that's being committed.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Delete an outstanding iterator object, freeing the transaction.
 */
// -------------------------------------------------------------------------------------------------
void HandleDeleteTxnRequest
(
    UserInfo_t* userPtr,              ///< The user that requested this action.
    le_cfg_Context_t contextRef,      ///< Context to reply on.
    le_cfg_iteratorRef_t iteratorRef  ///< The iterator object to delete.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Delete a node without an explicit transaction.
 */
// -------------------------------------------------------------------------------------------------
void HandleQuickDeleteNode
(
    UserInfo_t* userPtr,              ///< The user that's requesting the action.
    TreeInfo_t* treePtr,              ///< The tree that we're peforming the action on.
    le_msg_SessionRef_t sessionRef,   ///< The user session this request occured on.
    le_cfg_Context_t contextRef,      ///< This handle is used to generate the reply for this
                                      ///<   message.
    const char* pathPtr               ///< The path to the node to delete.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Clear out a node.
 */
// -------------------------------------------------------------------------------------------------
void HandleQuickSetEmpty
(
    UserInfo_t* userPtr,              ///< The user that's requesting the action.
    TreeInfo_t* treePtr,              ///< The tree that we're peforming the action on.
    le_msg_SessionRef_t sessionRef,   ///< The user session this request occured on.
    le_cfg_Context_t contextRef,      ///< This handle is used to generate the reply for this
                                      ///<   message.
    const char* pathPtr               ///< Path to the node to clear.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Read a string value from the node.
 */
// -------------------------------------------------------------------------------------------------
void HandleQuickGetString
(
    UserInfo_t* userPtr,          ///< The user that's requesting the action.
    TreeInfo_t* treePtr,          ///< The tree that we're peforming the action on.
    le_cfg_Context_t contextRef,  ///< This handle is used to generate the reply for this message.
    const char* pathPtr,          ///< Absolute path to the node to read.
    size_t maxString              ///< Maximum size of string to return.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Write a string value to a node in the tree.
 */
// -------------------------------------------------------------------------------------------------
void HandleQuickSetString
(
    UserInfo_t* userPtr,              ///< The user that's requesting the action.
    TreeInfo_t* treePtr,              ///< The tree that we're peforming the action on.
    le_msg_SessionRef_t sessionRef,   ///< The user session this request occured on.
    le_cfg_Context_t contextRef,      ///< This handle is used to generate the reply for this
                                      ///<   message.
    const char* pathPtr,              ///< Path to the node to read.
    const char* valuePtr              ///< Buffer to handle the new string.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get an integer value from the tee..
 */
// -------------------------------------------------------------------------------------------------
void HandleQuickGetInt
(
    UserInfo_t* userPtr,          ///< The user that's requesting the action.
    TreeInfo_t* treePtr,          ///< The tree that we're peforming the action on.
    le_cfg_Context_t contextRef,  ///< This handle is used to generate the reply for this message.
    const char* pathPtr           ///< Path to the node to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Write an integer value to the configTree..
 */
// -------------------------------------------------------------------------------------------------
void HandleQuickSetInt
(
    UserInfo_t* userPtr,              ///< The user that's requesting the action.
    TreeInfo_t* treePtr,              ///< The tree that we're peforming the action on.
    le_msg_SessionRef_t sessionRef,   ///< The user session this request occured on.
    le_cfg_Context_t contextRef,      ///< This handle is used to generate the reply for this
                                      ///<   message.
    const char* pathPtr,              ///< The path to write to.
    int value                         ///< The new value to write.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Read a new floating point value from the tree..
 */
// -------------------------------------------------------------------------------------------------
void HandleQuickGetFloat
(
    UserInfo_t* userPtr,          ///< The user that's requesting the action.
    TreeInfo_t* treePtr,          ///< The tree that we're peforming the action on.
    le_cfg_Context_t contextRef,  ///< This handle is used to generate the reply for this message.
    const char* pathPtr           ///< Path to the node to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Write a new floating point value to the tree.
 */
// -------------------------------------------------------------------------------------------------
void HandleQuickSetFloat
(
    UserInfo_t* userPtr,              ///< The user that's requesting the action.
    TreeInfo_t* treePtr,              ///< The tree that we're peforming the action on.
    le_msg_SessionRef_t sessionRef,   ///< The user session this request occured on.
    le_cfg_Context_t contextRef,      ///< This handle is used to generate the reply for this
                                      ///<   message.
    const char* pathPtr,              ///< Path to the node to write to.
    float value                       ///< Value to write.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Read a boolean value from the tree.
 */
// -------------------------------------------------------------------------------------------------
void HandleQuickGetBool
(
    UserInfo_t* userPtr,          ///< The user that's requesting the action.
    TreeInfo_t* treePtr,          ///< The tree that we're peforming the action on.
    le_cfg_Context_t contextRef,  ///< This handle is used to generate the reply for this message.
    const char* pathPtr           ///< The path to read from.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Write a boolean value to the tree.
 */
// -------------------------------------------------------------------------------------------------
void HandleQuickSetBool
(
    UserInfo_t* userPtr,              ///< The user that's requesting the action.
    TreeInfo_t* treePtr,              ///< The tree that we're peforming the action on.
    le_msg_SessionRef_t sessionRef,   ///< The user session this request occured on.
    le_cfg_Context_t contextRef,      ///< This handle is used to generate the reply for this
                                      ///<   message.
    const char* pathPtr,              ///< The path to write to.
    bool value                        ///< The value to write.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Register a new change notification handler with the tree.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_ChangeHandlerRef_t HandleAddChangeHandler
(
    UserInfo_t* userPtr,                    ///< The user that's requesting the action.
    TreeInfo_t* treePtr,                    ///< The tree that we're peforming the action on.
    le_msg_SessionRef_t sessionRef,         ///< The user session this request occured on.
    const char* pathPtr,                    ///< Where on the tree to register.
    le_cfg_ChangeHandlerFunc_t handlerPtr,  ///< The handler to register.
    void* contextPtr                        ///< The context to reply on.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Remove a previously registered handler from the tree..
 */
// -------------------------------------------------------------------------------------------------
void HandleRemoveChangeHandler
(
    le_cfg_ChangeHandlerRef_t changeHandlerRef  ///<
);




#endif
