
// -------------------------------------------------------------------------------------------------
/*
 *  The core iterator functionality is handled here.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#ifndef CFG_ITERATOR_INCLUDE_GUARD
#define CFG_ITERATOR_INCLUDE_GUARD




// -------------------------------------------------------------------------------------------------
/**
 *  Callback that is called during itration over active iterators.
 */
// -------------------------------------------------------------------------------------------------
typedef void (*itr_ForEachHandler)
(
    le_cfg_iteratorRef_t iteratorRef,  ///< The iterator safe ref.
    IteratorInfo_t* iteratorPtr,       ///< The actual underlying iterator pointer.
    void* contextPtr                   ///< User context pointer.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Initialize the memory structures needed by the iterator subsystem.
 */
// -------------------------------------------------------------------------------------------------
void itr_Init
(
);



//--------------------------------------------------------------------------------------------------
/**
 * Fetch a pointer to a printable string containing the name of a given transaction type.
 *
 * @return A pointer to the name.
 **/
//--------------------------------------------------------------------------------------------------
const char* itr_TxnTypeString
(
    IteratorType_t type
);



// -------------------------------------------------------------------------------------------------
/**
 *  Create an iterator object that's invalid.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_iteratorRef_t itr_NewInvalidRef
(
);




// -------------------------------------------------------------------------------------------------
/**
 *  Creae a new iterator reference, safe for returning to 3rd party processes.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_iteratorRef_t itr_NewRef
(
    UserInfo_t* userPtr,             ///< Create the iterator for this user.
    TreeInfo_t* treePtr,             ///< The tree to create the iterator with.
    le_msg_SessionRef_t sessionRef,  ///< The user session this request occured on.
    IteratorType_t type,             ///< The type of iterator we are creating, read or write?
    const char* initialPathPtr       ///< The node to move to in the specified tree.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Commit the interator data to the original tree.
 */
// -------------------------------------------------------------------------------------------------
le_result_t itr_Commit
(
    UserInfo_t* userPtr,           ///< The user we are getting an iterator for.
    le_cfg_iteratorRef_t iteratorRef  ///< The iterator to commit and free.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Release the iterator without changing the original tree.
 */
// -------------------------------------------------------------------------------------------------
void itr_Release
(
    UserInfo_t* userPtr,              ///< The user we are getting an iterator for.
    le_cfg_iteratorRef_t iteratorRef  ///< The iterator to free.
);




// -------------------------------------------------------------------------------------------------
/**
 *  This function will find all iterators that are active for a given session ref.  For each found
 *  iterator the supplied function will be called, with the safe reference and the underlying
 *  iterator pointer.
 *
 *  Keep in mind that it is not safe to remove items from the list until this function returns.
 */
// -------------------------------------------------------------------------------------------------
void itr_ForEachIterForSession
(
    le_msg_SessionRef_t sessionRef,  ///< The session we're searching for.
    itr_ForEachHandler functionPtr,  ///< The function to call with our matches.
    void* contextPtr                 ///< Pass along any extra information the callback function
                                     ///<   may require.
);




// -------------------------------------------------------------------------------------------------
/**
 *  This function will find all iterators that are active for a given tree object.  For each found
 *  iterator the supplied function will be called, with the safe reference and the underlying
 *  iterator pointer.
 *
 *  Keep in mind that it is not safe to remove items from the list until this function returns.
 */
// -------------------------------------------------------------------------------------------------
void itr_ForEachIterForTree
(
    TreeInfo_t* treePtr,             ///< Ptr to the Tree Info object we're searching for.
    itr_ForEachHandler functionPtr,  ///< The function to call with our matches.
    void* contextPtr                 ///< Pass along any extra information the callback function
                                     ///<   may require.
);


// -------------------------------------------------------------------------------------------------
/**
 *  Check an iterator reference and make sure it's valid.  An iterator ref can be invalid either
 *  because the handle itself is bad, or the iterator can be in a "bad" state.
 */
// -------------------------------------------------------------------------------------------------
le_result_t itr_CheckRef
(
    UserInfo_t* userPtr,              ///< Checking the iterator for this user.
    le_cfg_iteratorRef_t iteratorRef  ///< The iterator we're checking.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get an iterator object from a safe reference.  As an extra safty/security check, we also
 *  validate the user ID.  This way a client can not attack the server by genenerating fake handles
 *  and get at sensitive information.
 *
 *  Or at least we made it a little harder to do so.
 */
// -------------------------------------------------------------------------------------------------
IteratorInfo_t* itr_GetPtr
(
    UserInfo_t* userPtr,           ///< The user we are getting an iterator for.
    le_cfg_iteratorRef_t iteratorRef  ///< The safe reference to derefernce.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Create a clone of an iterator pointer and return that clone as a safe reference.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_iteratorRef_t itr_Clone
(
    IteratorInfo_t* iteratorPtr  ///< The iterator object to clone.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Check to see if an iterator represents an active transaction.
 */
// -------------------------------------------------------------------------------------------------
bool itr_IsClosed
(
    IteratorInfo_t* iteratorPtr  ///< The iterator object to query.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Check to see if an iterator represents a write transaction.
 */
// -------------------------------------------------------------------------------------------------
bool itr_IsWriteIterator
(
    IteratorInfo_t* iteratorPtr  ///< The iterator object to query.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the tree this iterator was created on.
 */
// -------------------------------------------------------------------------------------------------
TreeInfo_t* itr_GetTree
(
    IteratorInfo_t* iteratorPtr  ///< The iterator object to query.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Move the iterator to the node specified by the path.
 */
// -------------------------------------------------------------------------------------------------
le_result_t itr_GoToNode
(
    IteratorInfo_t* iteratorPtr,  ///< The iterator to move.
    const char* pathPtr           ///< The path to move the iterator to.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Move the iterator to the parent of the node that the iterator is currently pointed at.  If the
 *  iterator is currently at the root of the tree then iterator will be set as a bad path.
 */
// -------------------------------------------------------------------------------------------------
le_result_t itr_GoToParent
(
    IteratorInfo_t* iteratorPtr  ///< The iterator to move.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Go to the first child of the node that the iterator is pointed at.
 */
// -------------------------------------------------------------------------------------------------
le_result_t itr_GoToFirstChild
(
    IteratorInfo_t* iteratorPtr  ///< The iterator to move.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Move to the sibling of the node that the iterator is pointed at.  If there are no more siblings
 *  then the iterator is not moved and the function returns false.  Otherwise the function returns
 *  true.
 */
// -------------------------------------------------------------------------------------------------
bool itr_GoToNextSibling
(
    IteratorInfo_t* iteratorPtr  ///< The iterator to move.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the name of the node that the iterator is currintly pointing at.
 *
 *  @return Returns a string buffer contianing the node's name.  Note that this buffer will later
 *          need to be released.
 */
// -------------------------------------------------------------------------------------------------
char* itr_GetNodeName
(
    IteratorInfo_t* iteratorPtr  ///< The iterator to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the path of the node that the iterator is pointed at.
 *
 *  @return Returns a string buffer contianing the node's path.  Note that this buffer will later
 *          need to be released.
 */
// -------------------------------------------------------------------------------------------------
char* itr_GetPath
(
    IteratorInfo_t* iteratorPtr  ///< The iterator to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the parent path of the node the iterator is pointed at.
 *
 *  @return Returns a string buffer contianing the parent of the current node's path.  Note that
 *          this buffer will later need to be released.
 */
// -------------------------------------------------------------------------------------------------
char* itr_GetParentPath
(
    IteratorInfo_t* iteratorPtr  ///< The iterator to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get the type of the node the iterator is currently pointing at.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_nodeType_t itr_GetNodeType
(
    IteratorInfo_t* iteratorPtr  ///< The iterator to read.
);




// -------------------------------------------------------------------------------------------------
/**
 *  Get a tree node, residing on an absolute or relative path.  However this function does not
 *  change the position of the iterator.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t itr_GetNode
(
    IteratorInfo_t* iteratorPtr,  ///< The iterator to start from.
    IteratorGetNodeFlag_t getNodeFlag,  ///<
    const char* pathPtr           ///< The path relative to the iterators location to use.
);




#endif
