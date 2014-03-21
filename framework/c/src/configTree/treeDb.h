
// -------------------------------------------------------------------------------------------------
/*
 *  Implementation of the low level tree DB structure.  This code also handles the persisting of the
 *  tree db to the filesystem.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#ifndef CFG_TREE_DB_INCLUDE_GUARD
#define CFG_TREE_DB_INCLUDE_GUARD




// Tree iterator refrence typedef.
typedef struct tdb_Iter* tdb_IterRef_t;




// -------------------------------------------------------------------------------------------------
/**
 *  Allocate our pools and load the config data from the filesystem.
 */
// -------------------------------------------------------------------------------------------------
void tdb_Init
(
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the named tree.
 */
// -------------------------------------------------------------------------------------------------
TreeInfo_t* tdb_GetTree
(
    const char* treeNamePtr  ///< The tree to load.
);




//--------------------------------------------------------------------------------------------------
/**
 * Gets an interator for step-by-step iteration over the tree collection. In this mode the iteration
 * is controlled by the calling function using the le_ref_NextNode() function.  There is one
 * iterator and calling this function resets the iterator position to the start of the map.  The
 * iterator is not ready for data access until le_ref_NextNode() has been called at least once.
 *
 * @return  Returns A reference to a tree iterator which is ready for tdb_NextNode() to be called on
 *          it.
 */
//--------------------------------------------------------------------------------------------------
tdb_IterRef_t tdb_GetTreeIterator
(
    void
);




//--------------------------------------------------------------------------------------------------
/**
 * Moves the iterator to the next key/value pair in the map.  If the hashmap is modified during
 * iteration then this function will return an error.
 *
 * @return  Returns LE_OK unless you go past the end of the map, then returns LE_NOT_FOUND.
 *          If the iterator has been invalidated by the map changing or you have previously
 *          received a LE_NOT_FOUND then this returns LE_FAULT.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tdb_NextNode
(
    tdb_IterRef_t iteratorRef  ///< Reference to the iterator.
);




//--------------------------------------------------------------------------------------------------
/**
 * Retrieves a pointer to the value which the iterator is currently pointing at.  If the iterator
 * has just been initialized and le_ref_NextNode() has not been called, or if the iterator has been
 * invalidated then this will return NULL.
 *
 * @return  A pointer to the current value, or NULL if the iterator has been invalidated or is not
 *          ready.
 */
//--------------------------------------------------------------------------------------------------
TreeInfo_t* tdb_iter_GetTree
(
    tdb_IterRef_t iteratorRef  ///< Reference to the iterator.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Called to commit the changes to a given config tree.
 */
// -------------------------------------------------------------------------------------------------
void tdb_CommitTree
(
    TreeInfo_t* treePtr  ///< The tree to be committed to the filesystem.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Serialize a tree node and it's children to a file in the filesystem.
 */
// -------------------------------------------------------------------------------------------------
void tdb_WriteTreeNode
(
    tdb_NodeRef_t nodePtr,  ///< Write the contents of this node to a file descriptor.
    int descriptor          ///< The file descriptor to write to.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Read a configuration tree node's contents from the file system.
 */
// -------------------------------------------------------------------------------------------------
bool tdb_ReadTreeNode
(
    tdb_NodeRef_t nodePtr,  ///< The node to write the new data to.
    int descriptor          ///< The file to read from.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Called to create a new tree that shadows an existing one.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_ShadowTree
(
    tdb_NodeRef_t originalPtr  ///< The tree to shadow.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Traverse a shadow tree and merge it's changes back into it's parent.
 */
// -------------------------------------------------------------------------------------------------
void tdb_MergeTree
(
    tdb_NodeRef_t shadowTreeRef  ///< Merge the ndoes from this tree into their base tree.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Call this to realease a tree.
 */
// -------------------------------------------------------------------------------------------------
void tdb_ReleaseTree
(
    tdb_NodeRef_t treeRef  ///< The tree to free.  Note that this doesn't have to be the root node.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Given a base node and a path, find another node in the tree.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetNode
(
    tdb_NodeRef_t baseNodePtr,  ///< The base node to start from.
    const char* nodePathPtr,    ///< The path we're searching for in the tree.
    bool forceCreate            ///< Should the nodes on the path be created?  Even if this isn't
                                ///<   a shadow tree?
);




// -------------------------------------------------------------------------------------------------
/**
 *  Delete a given node and all of it's children.
 */
// -------------------------------------------------------------------------------------------------
void tdb_DeleteNode
(
    tdb_NodeRef_t nodePtr  ///< The node to delete.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Find a root node for the given tree node.
 *
 *  @return The root node of the tree that the given node is a part of.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetRootNode
(
    tdb_NodeRef_t nodePtr  ///< The tree node to traverse.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the parent of the given node.
 *
 *  @return The parent node of the given node.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetParentNode
(
    tdb_NodeRef_t nodePtr  ///< Find the parent for this node.
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
    tdb_NodeRef_t nodePtr  ///< The node to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the next sibling for a given node.
 *
 *  @return The next sibling node for the given node.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetNextSibling
(
    tdb_NodeRef_t nodePtr  ///< The node to read.
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
    tdb_NodeRef_t nodePtr  ///< The node to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  This function will return the first active, that is not deleted, sibling of the given node.
 *
 *  @return The next "live" node in the sibling chain.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetNextActiveSibling
(
    tdb_NodeRef_t nodePtr  ///< The node to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the count of the children of this node.
 */
// -------------------------------------------------------------------------------------------------
size_t tdb_ChildNodeCount
(
    tdb_NodeRef_t nodePtr  ///< The node to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Check to see if a given node pointer represents a deleted node.
 */
// -------------------------------------------------------------------------------------------------
bool tdb_IsDeleted
(
    tdb_NodeRef_t nodePtr  ///< The node to check.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the name of a given node.
 */
// -------------------------------------------------------------------------------------------------
void tdb_GetName
(
    tdb_NodeRef_t nodePtr,  ///< The node to read.
    char* stringPtr,        ///< Destination buffer to hold the name.
    size_t maxSize          ///< Size of this buffer.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Compute the length of the name.
 */
// -------------------------------------------------------------------------------------------------
size_t tdb_GetNameLength
(
    tdb_NodeRef_t nodePtr  ///< The node to read.
);



// -------------------------------------------------------------------------------------------------
/**
 *  Set the name of a given node.  If this is a node in a shadow tree, the name is then overwritten.
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetName
(
    tdb_NodeRef_t nodePtr,  ///< The node to write to.
    const char* stringPtr   ///< The name to give to the node.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the nodes string value and copy into the destination buffer.
 */
// -------------------------------------------------------------------------------------------------
void tdb_GetAsString
(
    tdb_NodeRef_t nodePtr,  ///< The node object to read.
    char* stringPtr,        ///< Target buffer for the value string.
    size_t maxSize          ///< Maximum size the buffer can hold.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Set the given node to a string value.  If the given node is a stem then all children will be
 *  lost.
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetAsString
(
    tdb_NodeRef_t nodePtr,  ///< The node to set.
    const char* stringPtr   ///< The value to write to the node.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Read the given node and interpret it as a boolean value.
 */
// -------------------------------------------------------------------------------------------------
bool tdb_GetAsBool
(
    tdb_NodeRef_t nodePtr  ///< The node to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Overwite a node value as a new boolen value..
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetAsBool
(
    tdb_NodeRef_t nodePtr,  ///< The node t write.
    bool value              ///< The new value to write to that node.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Read the given node and interpret it as an integer value.
 */
// -------------------------------------------------------------------------------------------------
int tdb_GetAsInt
(
    tdb_NodeRef_t nodePtr  ///<
);




// -------------------------------------------------------------------------------------------------
/**
 *  Set an integer value to a given node, overwriting the previous value.
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetAsInt
(
    tdb_NodeRef_t nodePtr,  ///< The ndoe to write to.
    int value               ///< The value to write.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Read the given node and interpret it as a floating point value.
 */
// -------------------------------------------------------------------------------------------------
float tdb_GetAsFloat
(
    tdb_NodeRef_t nodePtr  ///< The ndoe to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Overwite a given ndoes value with a floating point one.
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetAsFloat
(
    tdb_NodeRef_t nodePtr,  ///< The node to write
    float value             ///< The value to write to that node.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Reate the current type id out of the node.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_nodeType_t tdb_GetTypeId
(
    tdb_NodeRef_t nodePtr  ///< The node to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Is the node currently empty?
 */
// -------------------------------------------------------------------------------------------------
bool tdb_IsNodeEmpty
(
    tdb_NodeRef_t nodePtr  ///< The ndoe to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Clear out the contents of the node.
 */
// -------------------------------------------------------------------------------------------------
void tdb_ClearNode
(
    tdb_NodeRef_t nodePtr  ///< The node to clear.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Append an update watch callback.
 */
// -------------------------------------------------------------------------------------------------
void tdb_AppendCallback
(
    tdb_NodeRef_t nodeRef,                         ///< The node to watch.
    le_cfg_ChangeHandlerFunc_t updateCallbackPtr,  ///< The function to call on update.
    void* contextPtr                               ///< A context to supply the callback with.
);




#endif
