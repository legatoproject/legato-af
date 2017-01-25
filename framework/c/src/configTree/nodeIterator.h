
// -------------------------------------------------------------------------------------------------
/**
 *  @file nodeIterator.h
 *
 *  Module that handles the configuration tree iterator functionality.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#ifndef CFG_NODE_ITERATOR_INCLUDE_GUARD
#define CFG_NODE_ITERATOR_INCLUDE_GUARD


#ifndef CFG_TREE_DB_INCLUDE_GUARD
/// Raw pointer ref for a node iterator.
typedef struct Iterator* ni_IteratorRef_t;

#endif



/// Const version of the iterator ref.
typedef struct Iterator const* ni_ConstIteratorRef_t;




// -------------------------------------------------------------------------------------------------
/**
 *  Callback that is called during itration over active iterators.
 */
// -------------------------------------------------------------------------------------------------
typedef void (*itr_ForEachHandler)
(
    ni_ConstIteratorRef_t iteratorRef,  ///< The iterator pointer.
    void* contextPtr                    ///< User context pointer.
);




// -------------------------------------------------------------------------------------------------
/*
 *  Tag an iterator for being either reading or writing.
 */
// -------------------------------------------------------------------------------------------------
typedef enum
{
    NI_READ,  ///< This is a read iterator.
    NI_WRITE  ///< THis is a write iterator.
}
ni_IteratorType_t;




//--------------------------------------------------------------------------------------------------
/**
 *  Init the node iterator subsystem and get it ready for use by the other subsystems in this
 *  daemon.
 */
//--------------------------------------------------------------------------------------------------
void ni_Init
(
    void
);




//--------------------------------------------------------------------------------------------------
/**
 *  Create a new node iterator object.
 */
//--------------------------------------------------------------------------------------------------
ni_IteratorRef_t ni_CreateIterator
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] The user session this request occured on.
    tu_UserRef_t userRef,            ///< [IN] Create the iterator for this user.
    tdb_TreeRef_t treeRef,           ///< [IN] The tree to create the iterator with.
    ni_IteratorType_t type,          ///< [IN] The type of iterator we are creating, read or write?
    const char* initialPathPtr       ///< [IN] The node to move to in the specified tree.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Create a new externally accessable reference for a given node iterator.
 */
//--------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t ni_CreateRef
(
    ni_IteratorRef_t iteratorRef  ///< [IN] The iterator to generate a reference for.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Given a reference to an iterator, get the original iterator pointer.
 */
//--------------------------------------------------------------------------------------------------
ni_IteratorRef_t ni_InternalRefFromExternalRef
(
    tu_UserRef_t userRef,             ///< [IN] We're getting an iterator for this user.
    le_cfg_IteratorRef_t externalRef  ///< [IN] The reference we're to look up.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Commit the changes introduced by an iterator to the config tree.
 */
//--------------------------------------------------------------------------------------------------
void ni_Commit
(
    ni_IteratorRef_t iteratorRef  ///< [IN] The iterator to commit to the tree.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Release a given iterator.  The tree this iterator is on is unchanged by this operation.  So, if
 *  this iterator represents uncommitted writes, then they are lost at this point.
 */
//--------------------------------------------------------------------------------------------------
void ni_Release
(
    ni_IteratorRef_t iteratorRef  ///< [IN] Free the resources used by this iterator.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Close an iterator object and invalidate it's external safe reference.  (If there is one.)  Once
 *  done, this iterator is no longer accessable from outside of the process.
 *
 *  An iterator is closed with out releasing in the case where you have an open write iterator on a
 *  tree with open reads.  The iterator is closed, but it's data is not yet mergable into the tree.
 *
 *  So, the iterator is marked as closed and it's external ref is invalidated so no more work can be
 *  done with that iterator.
 */
//--------------------------------------------------------------------------------------------------
void ni_Close
(
    ni_IteratorRef_t iteratorRef  ///< [IN] The iterator object to close.
);



//--------------------------------------------------------------------------------------------------
/**
 *  Check to see if the iterator has been previously closed.
 *
 *  @return True if the tierator is closed, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool ni_IsClosed
(
    ni_ConstIteratorRef_t iteratorRef  ///< [IN] The iterator object to check.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Check to see if the iterator is ment to allow writes.
 *
 *  @return True if the iterator is writeable, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool ni_IsWriteable
(
    ni_ConstIteratorRef_t iteratorRef  ///< [IN] Does this iterator represent a write transaction?
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the reference to the session that the iterator was created on.
 *
 *  @return A session reference, or NULL if the iterator was not created by an external client.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t ni_GetSession
(
    ni_ConstIteratorRef_t iteratorRef  ///< [IN] The iterator object to read.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the user information for the client that created this iterator object.
 *
 *  @return A user information pointer.
 */
//--------------------------------------------------------------------------------------------------
tu_UserRef_t ni_GetUser
(
    ni_ConstIteratorRef_t iteratorRef  ///< [IN] The iterator object to read.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the tree object that this iterator was created on.
 *
 *  @return A pointer to a configuration tree object.
 */
//--------------------------------------------------------------------------------------------------
tdb_TreeRef_t ni_GetTree
(
    ni_ConstIteratorRef_t iteratorRef  ///< [IN] The iterator object to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  This function will find all iterators that have active safe refs.  For each found
 *  iterator the supplied function will be called.
 *
 *  Keep in mind that it is not safe to create or destroy iterators until this function returns.
 */
// -------------------------------------------------------------------------------------------------
void ni_ForEachIter
(
    itr_ForEachHandler functionPtr,  ///< [IN] The function to call with our matches.
    void* contextPtr                 ///< [IN] Pass along any extra information the callback
                                     ///<      function may require.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Move the iterator to a different node in the current tree.
 *
 *  @return LE_OK if the move was successful.  LE_OVERFLOW if the path is too large, LE_UNDERFLOW if
 *          the resultant path attempts to go up past root.
 */
//--------------------------------------------------------------------------------------------------
le_result_t ni_GoToNode
(
    ni_IteratorRef_t iteratorRef,  ///< [IN] The iterator object to access.
    const char* newPathPtr         ///< [IN] Path to the new location in the tree to jump to.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the node that the iterator is currently pointed at, ofset by the sub-path is supplied.
 *
 *  @return A pointer to the requested node, if found, otherwise, NULL.
 */
//--------------------------------------------------------------------------------------------------
tdb_NodeRef_t ni_GetNode
(
    ni_IteratorRef_t iteratorRef,  ///< [IN] The iterator object to access.
    const char* subPathPtr         ///< [IN] Optional, can be used to specify a node relative to the
                                   ///<      current one.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Attempt to get the node in question.  However, if it doesn't exist, then try to create it.
 *
 *  @return A pointer to the requested node, if found, otherwise, NULL.
 */
//--------------------------------------------------------------------------------------------------
tdb_NodeRef_t ni_TryCreateNode
(
    ni_IteratorRef_t iteratorRef,  ///< [IN] The iterator object to access.
    const char* subPathPtr         ///< [IN] Optional, can be used to specify a node relative to the
                                   ///<      current one.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Use an iterator to check to see if a node exists within the configuration tree.
 *
 *  @return True if the node in question exists, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool ni_NodeExists
(
    ni_IteratorRef_t iteratorRef,  ///< [IN] The iterator object to access.
    const char* newPathPtr         ///< [IN] Optional, can be used to specify a node relative to the
                                   ///<      current one.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Check to see if the given node is empty in an iterator.
 *
 *  @return True if the node is considered empty or non-existant.  False otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool ni_IsEmpty
(
    ni_IteratorRef_t iteratorRef,  ///< [IN] The iterator object to access.
    const char* newPathPtr         ///< [IN] Optional, can be used to specify a node relative to the
                                   ///<      current one.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Clear a given node.
 */
//--------------------------------------------------------------------------------------------------
void ni_SetEmpty
(
    ni_IteratorRef_t iteratorRef,  ///< [IN] The iterator object to access.
    const char* newPathPtr         ///< [IN] Optional, can be used to specify a node relative to the
                                   ///< [IN] current one.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Delete a node from the tree.
 */
//--------------------------------------------------------------------------------------------------
void ni_DeleteNode
(
    ni_IteratorRef_t iteratorRef,  ///< [IN] The iterator object to access.
    const char* newPathPtr         ///< [IN] Optional, can be used to specify a node relative to the
                                   ///<      current one.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Move the iterator to the current node's parent.
 *
 *  @return LE_OK oif the move was successful.  LE_NOT_FOUND if there is no parent to move to.
 */
//--------------------------------------------------------------------------------------------------
le_result_t ni_GoToParent
(
    ni_IteratorRef_t iteratorRef  ///< [IN] The iterator object to access.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Move the iterator from the current node to it's child.
 *
 *  @return LE_OK if there is a child node to go to.  LE_NOT_FOUND is returned otherwise.
 */
//--------------------------------------------------------------------------------------------------
le_result_t ni_GoToFirstChild
(
    ni_IteratorRef_t iteratorRef  ///< [IN] The iterator object to access.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Move the iterator from it's current node to the next sibling of that node.
 *
 *  @return LE_OK if there is a sibling node to move to, LE_NOT_FOUND if not.
 */
//--------------------------------------------------------------------------------------------------
le_result_t ni_GoToNextSibling
(
    ni_IteratorRef_t iteratorRef  ///< [IN] The iterator object to access.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the path for the iterator's current node.  Or if specified, the path of a node relative to
 *  the iterator's current node.
 *
 *  @return LE_OK if the path is copied successfully, LE_OVERFLOW if the path wouldn't fit within
 *          the given buffer.  LE_OVERFLOW is also returned if the new path being appended will
 *          overflow the iterator's internal buffers.
 */
//--------------------------------------------------------------------------------------------------
le_result_t ni_GetPathForNode
(
    ni_IteratorRef_t iteratorRef,  ///< [IN]  The iterator object to access.
    const char* subPathPtr,        ///< [IN]  Optional, can be used to specify a node relative to
                                   ///<       the current one.
    char* destBufferPtr,           ///< [OUT] The buffer to copy string data into.
    size_t bufferMax               ///< [IN]  The maximum size of the string buffer.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the type of node the iterator is pointing at.
 *
 *  @return A member of the le_cfg_nodeType_t indicating the type of node in question.
 *          If the node is NULL or is marked as deleted, then LE_CFG_TYPE_DOESNT_EXIST.  Othwerwise
 *          if the value is empty or the node is an empty collection LE_CFG_TYPE_EMPTY is returned.
 *          The node's internal recorded type is returned in all other cases.
 */
//--------------------------------------------------------------------------------------------------
le_cfg_nodeType_t ni_GetNodeType
(
    ni_IteratorRef_t iteratorRef,  ///< [IN] The iterator object to access.
    const char* pathPtr            ///< [IN] Optional.  If specified, this path can refer to another
                                   ///<      node in the tree.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the name of the iterator's current node.  Or, optionally a node relative to the iterator's
 *  current node.
 *
 *  @return LE_OK if the node name will fit within the supplied buffer, LE_OVERFLOW otherwise.
 *          If a fatal problem is encountered and the client connection needs to be closed LE_FAULT
 *          will be returned.
 */
//--------------------------------------------------------------------------------------------------
le_result_t ni_GetNodeName
(
    ni_IteratorRef_t iteratorRef,  ///< [IN]  The iterator object to access.
    const char* pathPtr,           ///< [IN]  Optional path to another node in the tree.
    char* destBufferPtr,           ///< [OUT] The buffer to copy string data into.
    size_t bufferMax               ///< [IN]  The maximum size of the string buffer.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the value for a given node in the tree.
 *
 *  @return LE_OK if the node name will fit within the supplied buffer, LE_OVERFLOW otherwise.
 */
//--------------------------------------------------------------------------------------------------
le_result_t ni_GetNodeValueString
(
    ni_IteratorRef_t iteratorRef,  ///< [IN]  The iterator object to access.
    const char* pathPtr,           ///< [IN]  Optional path to another node in the tree.
    char* destBufferPtr,           ///< [OUT] The buffer to copy string data into.
    size_t bufferMax,              ///< [IN]  The maximum size of the string buffer.
    const char* defaultPtr         ///< [IN]  If the value can not be found, use this one instead.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Write a string value into the config tree.
 */
//--------------------------------------------------------------------------------------------------
void ni_SetNodeValueString
(
    ni_IteratorRef_t iteratorRef,  ///< [IN] The iterator object to access.
    const char* pathPtr,           ///< [IN] Optional path to another node in the tree.
    const char* valuePtr           ///< [IN] Write this value into the tree.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Read an integer value from a node in the config tree.
 *
 *  @return The node's value as an int.  If the node holds a float value then the value will be
 *          returned truncated.  If the node doesn't hold an int or a float value, then the default
 *          value will be returned instead.
 */
//--------------------------------------------------------------------------------------------------
int32_t ni_GetNodeValueInt
(
    ni_IteratorRef_t iteratorRef,  ///< [IN] The iterator object to access.
    const char* pathPtr,           ///< [IN] Optional path to another node in the tree.
    int32_t defaultValue           ///< [IN] If the value can not be found, use this one instead.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Write an integer value into a node in the config tree.
 */
//--------------------------------------------------------------------------------------------------
void ni_SetNodeValueInt
(
    ni_IteratorRef_t iteratorRef,  ///< [IN] The iterator object to access.
    const char* pathPtr,           ///< [IN] Optional path to another node in the tree.
    int32_t value                  ///< [IN] The value to write.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Read a floating point value from a node in the config tree.
 *
 *  @return The node's value as a float.  If the node holds an int value then the value will be
 *          promoted.  If the node doesn't hold a float or int value, then the default value
 *          will be returned instead.
 */
//--------------------------------------------------------------------------------------------------
double ni_GetNodeValueFloat
(
    ni_IteratorRef_t iteratorRef,  ///< [IN] The iterator object to access.
    const char* pathPtr,           ///< [IN] Optional path to another node in the tree.
    double defaultValue            ///< [IN] If the value can not be found, use this one instead.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Write a floating point value into a node in the config tree.
 */
//--------------------------------------------------------------------------------------------------
void ni_SetNodeValueFloat
(
    ni_IteratorRef_t iteratorRef,  ///< [IN] The iterator object to access.
    const char* pathPtr,           ///< [IN] Optional path to another node in the tree.
    double value                   ///< [IN] The value to write.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Read a boolean point value from a node in the config tree.
 *
 *  @return The boolean value currently held by the node.  If the node doesn't hold a boolean type,
 *          then the default value is returned instead.
 */
//--------------------------------------------------------------------------------------------------
bool ni_GetNodeValueBool
(
    ni_IteratorRef_t iteratorRef,  ///< [IN] The iterator object to access.
    const char* pathPtr,           ///< [IN] Optional path to another node in the tree.
    bool defaultValue              ///< [IN] If the value can not be found, use this one instead.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Write a boolean point value into a node in the config tree.
 */
//--------------------------------------------------------------------------------------------------
void ni_SetNodeValueBool
(
    ni_IteratorRef_t iteratorRef,  ///< [IN] The iterator object to access.
    const char* pathPtr,           ///< [IN] Optional path to another node in the tree.
    bool value                     ///< [IN] The value to write.
);




#endif
