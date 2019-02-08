
// -------------------------------------------------------------------------------------------------
/**
 * @file treeDb.h
 *
 *  Interface to the low level tree DB structure.
 *
 *  There are basically two types of tree:
 *   - Named trees
 *   - Shadow trees
 *
 *  Both are accessed the same basic way:
 *   1. Get a reference to the tree.
 *   2. Get a reference to the root node of the tree.
 *   3. Traverse the tree.
 *   4. Read/write the node values and/or create/delete new child nodes.
 *
 *  The shadow trees are used to support write transactions.  When a write transaction is
 *  started for a given named tree, a shadow tree is created to "shadow" that named tree.
 *  All changes to the tree are only done through the shadow tree.  When the write transaction
 *  is committed, the shadow tree is "merged" back into the named tree and then the shadow tree
 *  is deleted.   To cancel a write transaction, just delete the shadow tree without merging
 *  it back into the named tree.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#ifndef CFG_TREE_DB_INCLUDE_GUARD
#define CFG_TREE_DB_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Maximum size of the binary data encoded into string (including terminating zero).
 */
//--------------------------------------------------------------------------------------------------
#define TDB_MAX_ENCODED_SIZE    (LE_BASE64_ENCODED_SIZE(LE_CFG_BINARY_LEN) + 1)

/// Reference to a configuration tree.
typedef struct Tree* tdb_TreeRef_t;


/// Reference to a node in a configuration tree.
typedef struct Node* tdb_NodeRef_t;


/// Forward declaration.  See @ref nodeIterator.h for details.
typedef struct Iterator* ni_IteratorRef_t;




// -------------------------------------------------------------------------------------------------
/**
 *  Initialize the tree DB subsystem, and automatically load the system tree from the filesystem.
 *
 *  This function should be called once at start-up before any other Tree DB functions are called.
 */
// -------------------------------------------------------------------------------------------------
void tdb_Init
(
    void
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the named tree.
 *
 *  @return Pointer to the named tree object.
 */
// -------------------------------------------------------------------------------------------------
tdb_TreeRef_t tdb_GetTree
(
    const char* treeNamePtr  ///< [IN] The tree to load.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Called to delete the given tree both from memory and from the filesystem.
 *
 *  If the given tree has active iterators on it, then it will only be marked for deletion.  After
 *  all of the iterators close, the tree will be removed from the system automatically.
 */
// -------------------------------------------------------------------------------------------------
void tdb_DeleteTree
(
    tdb_TreeRef_t treeRef  ///< [IN] The tree object to permanently delete.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Called to get the pointer to the tree collection iterator.
 *
 *  @return Reference to the tree collection iterator.
 */
// -------------------------------------------------------------------------------------------------
le_hashmap_It_Ref_t tdb_GetTreeIterRef
(
    void
);




// -------------------------------------------------------------------------------------------------
/**
 *  Called to create a new tree that shadows an existing one.
 *
 *  @return Pointer to the new shadow tree.
 */
// -------------------------------------------------------------------------------------------------
tdb_TreeRef_t tdb_ShadowTree
(
    tdb_TreeRef_t treeRef  ///< [IN] The tree to shadow.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Called to create a new tree that shadows an existing one.
 *
 *  @return Pointer to the tree name string.
 */
// -------------------------------------------------------------------------------------------------
const char* tdb_GetTreeName
(
    tdb_TreeRef_t treeRef  ///< [IN] The tree object to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Called to get the root node of a tree object.
 *
 *  @return A pointer to the root node of a tree.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetRootNode
(
    tdb_TreeRef_t treeRef  ///< [IN] The tree object to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get a pointer to the write iterator that's active on the current tree.
 *
 *  @return A pointer to the write iterator currently active on the tree.  NULL if there isn't an
 *          iterator on the tree.
 */
// -------------------------------------------------------------------------------------------------
ni_IteratorRef_t tdb_GetActiveWriteIter
(
    tdb_TreeRef_t treeRef  ///< [IN] The tree object to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Call to check for any active read iterator's on the tree.
 *
 *  @return True if there are active iterators on the tree, False otherwise.
 */
// -------------------------------------------------------------------------------------------------
bool tdb_HasActiveReaders
(
    tdb_TreeRef_t treeRef  ///< [IN] The tree object to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Register an iterator on the given tree.
 */
// -------------------------------------------------------------------------------------------------
void tdb_RegisterIterator
(
    tdb_TreeRef_t treeRef,        ///< [IN] The tree object to update.
    ni_IteratorRef_t iteratorRef  ///< [IN] The iterator object we're registering.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Remove a prior iterator registration from a tree object.
 */
// -------------------------------------------------------------------------------------------------
void tdb_UnregisterIterator
(
    tdb_TreeRef_t treeRef,        ///< [IN] The tree object to update.
    ni_IteratorRef_t iteratorRef  ///< [IN] The iterator object we're removing from the tree.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the list of requests on this tree.
 *
 *  @return Pointer to the request queue for this tree.
 */
// -------------------------------------------------------------------------------------------------
le_sls_List_t* tdb_GetRequestQueue
(
    tdb_TreeRef_t treeRef  ///< [IN] The tree object to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Merge a shadow tree into the original tree it was created from.  Once the change is merged the
 *  updated tree is serialized to the filesystem.
 */
// -------------------------------------------------------------------------------------------------
void tdb_MergeTree
(
    tdb_TreeRef_t shadowTreeRef  ///< [IN] Merge the ndoes from this tree into their base tree.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Call this to realease a tree.
 */
// -------------------------------------------------------------------------------------------------
void tdb_ReleaseTree
(
    tdb_TreeRef_t treeRef  ///< [IN] The tree to free.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Read a configuration tree node's contents from the file system.
 *
 *  @note On exit the descriptor's file pointer will be at EOF.  If the function fails, then the
 *        file pointer will be somewhere in the middle of the file.
 *
 *  @return True if the read is successful, or false if not.
 */
// -------------------------------------------------------------------------------------------------
bool tdb_ReadTreeNode
(
    tdb_NodeRef_t nodeRef,  ///< [IN] The node to write the new data to.
    FILE* filePtr           ///< [IN] The file to read from.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Serialize a tree node and it's children to a file in the filesystem.
 *
 *  @return LE_OK if the write succeeded, LE_IO_ERROR if the write failed.
 */
// -------------------------------------------------------------------------------------------------
le_result_t tdb_WriteTreeNode
(
    tdb_NodeRef_t nodeRef,  ///< [IN] Write the contents of this node to a file descriptor.
    FILE* filePtr           ///< [IN] The file descriptor to write to.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Given a base node and a path, find another node in the tree.
 *
 *  @return A reference to the required node if found, NULL if not.  NULL is also returned if the
 *          path is eitehr too big to process or if a node name within the path is too large.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetNode
(
    tdb_NodeRef_t baseNodeRef,     ///< [IN] The base node to start from.
    le_pathIter_Ref_t nodePathRef  ///< [IN] The path we're searching for in the tree.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Traverse the given path and create nodes as needed.
 *
 *  @return The found or newly created node at the end of the given path.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_CreateNodePath
(
    tdb_NodeRef_t baseNodeRef,     ///< [IN] The base node to start from.
    le_pathIter_Ref_t nodePathRef  ///< [IN] The path we're creating within the tree.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Make sure that the given node and any of it's parents are not marked as having been deleted.
 */
// -------------------------------------------------------------------------------------------------
void tdb_EnsureExists
(
    tdb_NodeRef_t nodeRef   ///< [IN] Update this node, and all of it's parentage and make sure none
                            ///<      of them are marked for deletion.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the name of a given node.
 *
 *  @return LE_OK if the name copied successfuly.  LE_OVERFLOW if not.
 */
// -------------------------------------------------------------------------------------------------
le_result_t tdb_GetNodeName
(
    tdb_NodeRef_t nodeRef,  ///< [IN]  The node to read.
    char* stringPtr,        ///< [OUT] Destination buffer to hold the name.
    size_t maxSize          ///< [IN]  Size of this buffer.
);


// -------------------------------------------------------------------------------------------------
/**
 *  Get the name hash of a given node.
 *
 *  @return name hash
 */
// -------------------------------------------------------------------------------------------------
size_t tdb_GetNodeNameHash
(
    tdb_NodeRef_t nodeRef   ///< [IN]  The node to read.
);


// -------------------------------------------------------------------------------------------------
/**
 *  Set the name of a given node.  But also validate the name as there are certain names that nodes
 *  shouldn't have.
 *
 *  @return LE_OK if the set is successful.  LE_FORMAT_ERROR if the name contains illegal
 *          characters, or otherwise would not work as a node name.  LE_OVERFLOW if the name is too
 *          long.  LE_DUPLICATE, if there is another node with the new name in the same collection.
 */
// -------------------------------------------------------------------------------------------------
le_result_t tdb_SetNodeName
(
    tdb_NodeRef_t nodeRef,  ///< [IN] The node to update.
    const char* stringPtr   ///< [IN] New name for the node.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Call to read out what kind of value the node object holds.
 *
 *  @return A member of the le_cfg_nodeType_t indicating the type of node in question.
 *          If the node is NULL or is marked as deleted, then LE_CFG_TYPE_DOESNT_EXIST.  Othwerwise
 *          if the value is empty or the node is an empty collection LE_CFG_TYPE_EMPTY is returned.
 *          The node's recorded type is returned in all other cases.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_nodeType_t tdb_GetNodeType
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Is the node currently empty?
 *
 *  @return If tdb_GetNodeType would return either LE_CFG_TYPE_EMPTY or LE_CFG_TYPE_DOESNT_EXIST
 *          then this function will return true.  Otherwise this function will return false.
 */
// -------------------------------------------------------------------------------------------------
bool tdb_IsNodeEmpty
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Clear out the data from a node, releasing any children it may have.
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetEmpty
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node to clear.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Delete a given node from it's tree.  If it has children, they will be deleted too.
 */
// -------------------------------------------------------------------------------------------------
void tdb_DeleteNode
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node to delete.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the parent of the given node.
 *
 *  @return The parent node of the given node.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetNodeParent
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node object to read the parent object from.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Called to get the first child node of this node.  If this node has no children, then return
 *  NULL.
 *
 *  @return The first child of the given node.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetFirstChildNode
(
    tdb_NodeRef_t nodeRef  ///< [IN] Get the first child of this node.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the next sibling for a given node.
 *
 *  @return The next sibling node for the given node.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetNextSiblingNode
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node object to iterate from.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Like tdb_GetFirstChildNode this will return a child of the given parent node.  However, this
 *  function will ignore all nodes that are marked as deleted.
 *
 *  @return The first not-deleted child node of the given node.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetFirstActiveChildNode
(
    tdb_NodeRef_t nodeRef  ///< [IN] Get the first child of this node.
);




// -------------------------------------------------------------------------------------------------
/**
 *  This function will return the first active, that is not deleted, sibling of the given node.
 *
 *  @return The next "live" node in the sibling chain.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetNextActiveSiblingNode
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node object to iterate from.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the nodes string value and copy into the destination buffer.
 */
// -------------------------------------------------------------------------------------------------
le_result_t tdb_GetValueAsString
(
    tdb_NodeRef_t nodeRef,  ///< [IN]  The node object to read.
    char* stringPtr,        ///< [OUT] Target buffer for the value string.
    size_t maxSize,         ///< [IN]  Maximum size the buffer can hold.
    const char* defaultPtr  ///< [IN]  Default value to use in the event that the requested value
                            ///<       doesn't exist.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Set the given node to a string value.  If the given node is a stem then all children will be
 *  lost.
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetValueAsString
(
    tdb_NodeRef_t nodeRef,  ///< [IN] The node to set.
    const char* stringPtr   ///< [IN] The value to write to the node.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Read the given node and interpret it as a boolean value.
 *
 *  @return The node's value as a 32-bit boolean.  If the node doesn't exists or has the wrong type
 *          the default value is returned instead.
 */
// -------------------------------------------------------------------------------------------------
bool tdb_GetValueAsBool
(
    tdb_NodeRef_t nodeRef,  ///< [IN] The node to read.
    bool defaultValue       ///< [IN] Default value to use in the event that the requested value
                            ///<      doesn't exist.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Overwite a node value as a new boolen value..
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetValueAsBool
(
    tdb_NodeRef_t nodeRef,  ///< [IN] The node to write to.
    bool value              ///< [IN] The new value to write to that node.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Read the given node and interpret it as an integer value.
 *
 *  @return The node's current value as an int.  If the value was originaly a float then it is
 *          rounded.  If the node doesn't exist or is some other type then the default value is
 *          returned.
 */
// -------------------------------------------------------------------------------------------------
int32_t tdb_GetValueAsInt
(
    tdb_NodeRef_t nodeRef,  ///< [IN] The node to read from.
    int32_t defaultValue    ///< [IN] Default value to use in the event that the requested value
                            ///<      doesn't exist.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Set an integer value to a given node, overwriting the previous value.
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetValueAsInt
(
    tdb_NodeRef_t nodeRef,  ///< [IN] The node to write to.
    int value               ///< [IN] The value to write.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Read the given node and interpret it as a floating point value.
 *
 *  @return The node's value as a 64-bit floating point number.  If the value is an int, it is
 *          converted.  Otherwise, the default value is returned.
 */
// -------------------------------------------------------------------------------------------------
double tdb_GetValueAsFloat
(
    tdb_NodeRef_t nodeRef,  ///< [IN] The node to read.
    double defaultValue     ///< [IN] Default value to use in the event that the requested value
                            ///<      doesn't exist.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Overwite a given node's value with a floating point one.
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetValueAsFloat
(
    tdb_NodeRef_t nodeRef,  ///< [IN] The node to write
    double value            ///< [IN] The value to write to that node.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Registers a handler function to be called when a node at or below a given path changes.
 *
 *  @return A new safe ref backed object, or NULL if the creation failed.
 */
//--------------------------------------------------------------------------------------------------
le_cfg_ChangeHandlerRef_t tdb_AddChangeHandler
(
    tdb_TreeRef_t treeRef,                  ///< [IN] The tree to register the handler on.
    le_msg_SessionRef_t sessionRef,         ///< [IN] The session that the request came in on.
    const char* pathPtr,                    ///< [IN] Path of the node to watch.
    le_cfg_ChangeHandlerFunc_t handlerPtr,  ///< [IN] Function to call back.
    void* contextPtr                        ///< [IN] Opaque value to pass to the function when
                                            ///<      called.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Deregisters a handler function that was registered using tdb_AddChangeHandler().
 */
//--------------------------------------------------------------------------------------------------
void tdb_RemoveChangeHandler
(
    le_cfg_ChangeHandlerRef_t handlerRef,  ///< [IN] Reference returned by tdb_AddChangeHandler().
    le_msg_SessionRef_t sessionRef         ///< [IN] The session of the user making this request.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Clean out any event handlers registered on the given session.
 */
//--------------------------------------------------------------------------------------------------
void tdb_CleanUpHandlers
(
    le_msg_SessionRef_t sessionRef  ///< [IN] The session that's been closed.
);



//--------------------------------------------------------------------------------------------------
/**
 *  Getter function for the binary data memory pool
 *
 *  @return Reference to the binary data pool
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t tdb_GetBinaryDataMemoryPool
(
    void
);



//--------------------------------------------------------------------------------------------------
/**
 *  Getter function for the encoded string memory pool
 *
 *  @return Reference to the encoded string pool
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t tdb_GetEncodedStringMemoryPool
(
    void
);



#endif
