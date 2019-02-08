
// -------------------------------------------------------------------------------------------------
/**
 *  @file nodeIterator.c
 *
 *  Module that handles the configuration tree iterator functionality.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "treeDb.h"
#include "treeUser.h"
#include "internalConfig.h"
#include "nodeIterator.h"




/// Pool for allocating iterator objects.
static le_mem_PoolRef_t IteratorPoolRef = NULL;

/// The static pool for handling interator safe references
LE_REF_DEFINE_STATIC_MAP(IteratorMap, LE_CONFIG_CFGTREE_MAX_ITERATOR_POOL_SIZE);

/// The pool for handing iterator safe references.
static le_ref_MapRef_t IteratorRefMap = NULL;


/// Name of the safe ref map that's backing the iterator references.
#define ITERATOR_REF_MAP_NAME "iteratorRefMap"




//--------------------------------------------------------------------------------------------------
/**
 *  The config tree iterator.
 */
//--------------------------------------------------------------------------------------------------
typedef struct Iterator
{
    le_clk_Time_t creationTime;      ///< The time this iterator was created.
    le_timer_Ref_t timerRef;         ///< Timer to make sure that the iterator hasn't overstayed
                                     ///<   it's welcome.

    le_msg_SessionRef_t sessionRef;  ///< The user session that this iterator was created for.

    tu_UserRef_t userRef;            ///< The user that this iterator was created for.
    tdb_TreeRef_t treeRef;           ///< The tree this iterator was created on.

    ni_IteratorType_t type;          ///< The type of iterator we are creating, read or write?

    bool isClosed;                   ///< Has this iterator been closed?
    bool isTerminated;               ///< Has the iterator been closed due to a fatal error?

    le_pathIter_Ref_t pathIterRef;   ///< Path to the iterator's current node.
    tdb_NodeRef_t currentNodeRef;    ///< The current node itself.


    le_cfg_IteratorRef_t reference;  ///< A safe reference to this iterator object.  This can be
                                     ///<  NULL if the iterator was created without a safe
                                     ///<  reference.  (This can happen if the iterator was created
                                     ///<  for internal use only, like for the Quick family of
                                     ///<  functions.)
}
Iterator_t;


/// Define static pool for node iterators
LE_MEM_DEFINE_STATIC_POOL(iteratorPool, LE_CONFIG_CFGTREE_MAX_ITERATOR_POOL_SIZE,
    sizeof(Iterator_t));


//--------------------------------------------------------------------------------------------------
/**
 *  Fetch a pointer to a printable string containing the name of a given transaction type.
 *
 *  @return A pointer to the name.
 */
//--------------------------------------------------------------------------------------------------
static const char* TypeString
(
    ni_IteratorType_t type   ///< [IN] Iterator type to turn into a string.
)
//--------------------------------------------------------------------------------------------------
{
    switch (type)
    {
        case NI_READ:  return "read";
        case NI_WRITE: return "write";
        default:       LE_FATAL("Invalid transaction type %d", type);
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Timer callback, this function will take care of reporting that the transaction timeout has
 *  expired.  It will also close the offending session.
 */
//--------------------------------------------------------------------------------------------------
static void OnTransactionTimeout
(
    le_timer_Ref_t timerRef   ///< The timer that expired.
)
//--------------------------------------------------------------------------------------------------
{
    // Extract the iterator reference out of the timer object.  Then perform a sanity check to make
    // sure everything is going to plan.
    ni_IteratorRef_t iteratorRef = (ni_IteratorRef_t)le_timer_GetContextPtr(timerRef);
    LE_ASSERT(iteratorRef->timerRef == timerRef);

    // For clearer message reporting, figure out if this is a read or write transaction.
    char* iterTypePtr = "Read";

    if (ni_IsWriteable(iteratorRef))
    {
        iterTypePtr = "Write";
    }

    if (iteratorRef->isTerminated)
    {
        LE_DEBUG("Previously terminated iterator, <%p> timed out.", iteratorRef);
        return;
    }

    // Report the failure in the log, and close the client session.  Once the session is closed all
    // of that user's resources within the configTree will be naturally cleaned up.
    LE_EMERG("%s transaction <%p> timer expired, for user %s, <%d>.",
             iterTypePtr,
             iteratorRef->reference,
             tu_GetUserName(iteratorRef->userRef),
             tu_GetUserId(iteratorRef->userRef));

    tu_TerminateConfigClient(iteratorRef->sessionRef, "Transaction timeout.");
}




//--------------------------------------------------------------------------------------------------
/**
 *  Create a new path iterator based off of node iterators current path iterator.  Then optionally,
 *  apply a sub-path to this new iterator.
 *
 *  @note   This function will terminate the client on any errors encountered.
 *
 *  @return LE_OK if all went to plan.  LE_OVERFLOW if the new path is too large to be held in an
 *          iterator.  LE_UNDERFLOW if the applied sub-path attempts go to up past root.
 *          The newly created iterator is returned as an out-parameter.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CloneAndAppendPath
(
    ni_IteratorRef_t iteratorRef,         ///< [IN]  The iterator to read.
    const char* subPathPtr,               ///< [IN]  Optionally, a sub-path to traverse to.
    le_pathIter_Ref_t* newPathIterRefPtr  ///< [OUT] Pointer to a to-be newly created ref.  Must be
                                          ///<       NULL on entry!
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(*newPathIterRefPtr == NULL);


    le_pathIter_Ref_t newPathRef = le_pathIter_Clone(iteratorRef->pathIterRef);
    le_result_t result = LE_OK;

    if (subPathPtr != NULL)
    {
        result = le_pathIter_Append(newPathRef, subPathPtr);
    }

    if (result != LE_OK)
    {
        char* terminateMessage;

        switch (result)
        {
            case LE_OVERFLOW:
                terminateMessage = "Specified path too large.";
                break;

            case LE_UNDERFLOW:
                terminateMessage = "Specified path attempts to iterate below root.";
                break;

            default:
                terminateMessage = "Unexpected error while appending path.";
                break;
        }

        iteratorRef->isTerminated = true;
        tu_TerminateConfigClient(iteratorRef->sessionRef, terminateMessage);

        le_pathIter_Delete(newPathRef);
        return result;
    }

    *newPathIterRefPtr = newPathRef;
    return result;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Init the node iterator subsystem and get it ready for use by the other subsystems in this
 *  daemon.
 */
//--------------------------------------------------------------------------------------------------
void ni_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Initialize Node Iterator subsystem.");

    IteratorPoolRef = le_mem_InitStaticPool(iteratorPool,
                                            LE_CONFIG_CFGTREE_MAX_ITERATOR_POOL_SIZE,
                                            sizeof(Iterator_t));

    IteratorRefMap = le_ref_InitStaticMap(IteratorMap, LE_CONFIG_CFGTREE_MAX_ITERATOR_POOL_SIZE);
}




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
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(treeRef != NULL);

    // Allocate the object and setup it's initial properties.
    ni_IteratorRef_t iteratorRef = le_mem_ForceAlloc(IteratorPoolRef);

    iteratorRef->creationTime = le_clk_GetRelativeTime();
    iteratorRef->sessionRef = sessionRef;
    iteratorRef->userRef = userRef;
    iteratorRef->treeRef = treeRef;
    iteratorRef->type = type;
    iteratorRef->reference = NULL;
    iteratorRef->isClosed = false;
    iteratorRef->isTerminated = false;

    // Setup the timeout timer for this transaction, if it's been configured.
    time_t configTimeout = ic_GetTransactionTimeout();

    if (configTimeout > 0)
    {
        le_clk_Time_t timeout = { configTimeout, 0 };
        iteratorRef->timerRef = le_timer_Create("Transaction Timer");

        LE_ASSERT(le_timer_SetInterval(iteratorRef->timerRef, timeout) == LE_OK);
        LE_ASSERT(le_timer_SetHandler(iteratorRef->timerRef, OnTransactionTimeout) == LE_OK);
        LE_ASSERT(le_timer_SetContextPtr(iteratorRef->timerRef, iteratorRef) == LE_OK);
        LE_ASSERT(le_timer_SetWakeup(iteratorRef->timerRef, false) == LE_OK);

        LE_ASSERT(le_timer_Start(iteratorRef->timerRef) == LE_OK);
    }
    else
    {
        iteratorRef->timerRef = NULL;
    }

    // If this is a write iterator, then shadow the tree instead of accessing it directly.
    if (iteratorRef->type == NI_WRITE)
    {
        iteratorRef->treeRef = tdb_ShadowTree(iteratorRef->treeRef);
    }

    // Get the root node of the requested tree, or if this is a write iterator...  Get the shadowed
    // root node of the tree.
    iteratorRef->currentNodeRef = tdb_GetRootNode(iteratorRef->treeRef);
    iteratorRef->pathIterRef = le_pathIter_CreateForUnix("/");


    LE_DEBUG("Created a new %s iterator object <%p> for user %u (%s) on tree %s.",
             TypeString(type),
             iteratorRef,
             tu_GetUserId(userRef),
             tu_GetUserName(userRef),
             tdb_GetTreeName(treeRef));

     // If we were given an initial path, go to it now.  Otherwise stay on the root node.
    if (initialPathPtr)
    {
        ni_GoToNode(iteratorRef, initialPathPtr);
    }

    // Update the tree so that it can keep track of this iterator.
    tdb_RegisterIterator(treeRef, iteratorRef);

    // All done.
    return iteratorRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Create a new externally accessable reference for a given node iterator.
 */
//--------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t ni_CreateRef
(
    ni_IteratorRef_t iteratorRef  ///< [IN] The iterator to generate a reference for.
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(iteratorRef != NULL);
    LE_ASSERT(iteratorRef->reference == NULL);

    iteratorRef->reference = le_ref_CreateRef(IteratorRefMap, iteratorRef);
    LE_DEBUG("Created a new reference <%p> for iterator <%p>.",
             iteratorRef->reference,
             iteratorRef);

    return iteratorRef->reference;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Given a reference to an iterator, get the original iterator pointer.
 */
//--------------------------------------------------------------------------------------------------
ni_IteratorRef_t ni_InternalRefFromExternalRef
(
    tu_UserRef_t userRef,             ///< [IN] We're getting an iterator for this user.
    le_cfg_IteratorRef_t externalRef  ///< [IN] The reference we're to look up.
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(externalRef != NULL);
    LE_ASSERT(userRef != NULL);

    ni_IteratorRef_t iteratorRef = le_ref_Lookup(IteratorRefMap, externalRef);

    LE_ASSERT(iteratorRef != NULL);
    LE_ASSERT(iteratorRef->userRef != NULL);

    if (   (iteratorRef == NULL)
        || (tu_GetUserId(userRef) != tu_GetUserId(iteratorRef->userRef)))
    {
        LE_ERROR("Iterator reference <%p> not found.", externalRef);
        return NULL;
    }

    return iteratorRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Commit the changes introduced by an iterator to the config tree.
 */
//--------------------------------------------------------------------------------------------------
void ni_Commit
(
    ni_IteratorRef_t iteratorRef  ///< [IN] The iterator to commit to the tree.
)
//--------------------------------------------------------------------------------------------------
{
    if (iteratorRef->type == NI_WRITE)
    {
        tdb_MergeTree(iteratorRef->treeRef);
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Release a given iterator.  The tree this iterator is on is unchanged by this operation.  So, if
 *  this iterator represents uncommitted writes, then they are lost at this point.
 */
//--------------------------------------------------------------------------------------------------
void ni_Release
(
    ni_IteratorRef_t iteratorRef  ///< [IN] Free the resources used by this iterator.
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(iteratorRef != NULL);

    // Make sure that the transaction timer isn't still running.
    if (iteratorRef->timerRef != NULL)
    {
        if (le_timer_GetExpiryCount(iteratorRef->timerRef) == 0)
        {
            le_timer_Stop(iteratorRef->timerRef);
        }

        le_timer_Delete(iteratorRef->timerRef);
        iteratorRef->timerRef = NULL;
    }

    // Release the rest of the iterator's resources.
    LE_DEBUG("Releasing iterator, <%p> with a lifetime of %" PRId32 " seconds.",
             iteratorRef,
             (uint32_t)(le_clk_GetRelativeTime().sec - iteratorRef->creationTime.sec));

    ni_Close(iteratorRef);
    tdb_UnregisterIterator(iteratorRef->treeRef, iteratorRef);

    le_pathIter_Delete(iteratorRef->pathIterRef);

    tdb_ReleaseTree(iteratorRef->treeRef);
    le_mem_Release(iteratorRef);
}




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
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(iteratorRef != NULL);

    LE_DEBUG("Closing iterator, <%p>.", iteratorRef);
    LE_DEBUG_IF(iteratorRef->reference != NULL,
                "Releasing associated reference, <%p>.",
                iteratorRef->reference);

    if (iteratorRef->reference)
    {
        le_ref_DeleteRef(IteratorRefMap, iteratorRef->reference);
        iteratorRef->reference = 0;
    }

    iteratorRef->isClosed = true;
}



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
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(iteratorRef != NULL);
    return iteratorRef->isClosed;
}




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
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(iteratorRef != NULL);
    return iteratorRef->type == NI_WRITE;
}




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
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(iteratorRef != NULL);
    return iteratorRef->sessionRef;
}




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
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(iteratorRef != NULL);
    return iteratorRef->userRef;
}




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
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(iteratorRef != NULL);
    return iteratorRef->treeRef;
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(functionPtr != NULL);

    le_ref_IterRef_t refIterator = le_ref_GetIterator(IteratorRefMap);

    while (le_ref_NextNode(refIterator) == LE_OK)
    {
        ni_ConstIteratorRef_t iteratorRef = le_ref_GetValue(refIterator);

        if (iteratorRef != NULL)
        {
            functionPtr(iteratorRef, contextPtr);
        }
    }
}




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
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(iteratorRef != NULL);
    LE_ASSERT(le_pathIter_IsAbsolute(iteratorRef->pathIterRef) == true);

    le_result_t result = le_pathIter_Append(iteratorRef->pathIterRef, newPathPtr);

    if (result == LE_OK)
    {
        tdb_TreeRef_t treeRef = iteratorRef->treeRef;

        iteratorRef->currentNodeRef = tdb_GetNode(tdb_GetRootNode(treeRef),
                                                  iteratorRef->pathIterRef);

    }

    return result;
}




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
)
//--------------------------------------------------------------------------------------------------
{
    // Copy iterator's existing path, and append the new sub path to the copied path.  Once
    // that's done, attempt to find the requested node in the tree.  If the node still can not be
    // found, return NULL.
    le_pathIter_Ref_t newPathRef = NULL;

    if (CloneAndAppendPath(iteratorRef, subPathPtr, &newPathRef) != LE_OK)
    {
        return NULL;
    }

    tdb_NodeRef_t nodeRef = tdb_GetNode(tdb_GetRootNode(iteratorRef->treeRef), newPathRef);

    le_pathIter_Delete(newPathRef);

    return nodeRef;
}




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
)
//--------------------------------------------------------------------------------------------------
{
    // Clone the iterator's original path and, if supplied, append the new sub path onto this new
    // path.
    le_pathIter_Ref_t newPathRef = NULL;

    if (CloneAndAppendPath(iteratorRef, subPathPtr, &newPathRef) != LE_OK)
    {
        return NULL;
    }

    // Attempt to find the node in the tree.  If not found attempt to create the new node in the
    // tree.
    tdb_NodeRef_t rootNodeRef = tdb_GetRootNode(iteratorRef->treeRef);
    tdb_NodeRef_t nodeRef = tdb_GetNode(rootNodeRef, newPathRef);

    if (nodeRef == NULL)
    {
        nodeRef = tdb_CreateNodePath(rootNodeRef, newPathRef);
    }

    le_pathIter_Delete(newPathRef);
    return nodeRef;
}




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
)
//--------------------------------------------------------------------------------------------------
{
    tdb_NodeRef_t nodeRef = ni_GetNode(iteratorRef, newPathPtr);

    if (nodeRef == NULL)
    {
        return false;
    }

    return tdb_GetNodeType(nodeRef) != LE_CFG_TYPE_DOESNT_EXIST;
}




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
)
//--------------------------------------------------------------------------------------------------
{
    tdb_NodeRef_t nodeRef = ni_GetNode(iteratorRef, newPathPtr);

    if (nodeRef)
    {
        return tdb_IsNodeEmpty(nodeRef);
    }

    return true;
}




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
)
//--------------------------------------------------------------------------------------------------
{
    tdb_NodeRef_t nodeRef = ni_TryCreateNode(iteratorRef, newPathPtr);

    if (nodeRef)
    {
        tdb_SetEmpty(nodeRef);
        tdb_EnsureExists(nodeRef);
    }
}




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
)
//--------------------------------------------------------------------------------------------------
{
    // Delete the requested node, and then see if we can find our way back to where we were.
    tdb_NodeRef_t nodeRef = ni_GetNode(iteratorRef, newPathPtr);

    if (nodeRef != NULL)
    {
        tdb_DeleteNode(nodeRef);
        iteratorRef->currentNodeRef = ni_GetNode(iteratorRef, "");
    }
}




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
)
//--------------------------------------------------------------------------------------------------
{
    // Update our path.
    if (le_pathIter_Append(iteratorRef->pathIterRef, "..") == LE_UNDERFLOW)
    {
        // Looks like there are no more parents in the chain.
        return LE_NOT_FOUND;
    }

    // Now, if we have a current node, just get it's parent node.  Otherwise make an attempt to see
    // if the requested parent node exists.
    if (iteratorRef->currentNodeRef != NULL)
    {
        iteratorRef->currentNodeRef = tdb_GetNodeParent(iteratorRef->currentNodeRef);
        LE_ASSERT(iteratorRef->currentNodeRef != NULL);
    }
    else
    {
        // Make an attempt to get the new current node.
        iteratorRef->currentNodeRef = ni_GetNode(iteratorRef, "");
    }

    return LE_OK;
}




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
)
//--------------------------------------------------------------------------------------------------
{
    if (iteratorRef->currentNodeRef != NULL)
    {
        tdb_NodeRef_t newNodeRef = tdb_GetFirstActiveChildNode(iteratorRef->currentNodeRef);

        if (newNodeRef == NULL)
        {
            return LE_NOT_FOUND;
        }

        iteratorRef->currentNodeRef = newNodeRef;

        char name[LE_CFG_NAME_LEN_BYTES] = "";

        tdb_GetNodeName(newNodeRef, name, sizeof(name));
        le_pathIter_Append(iteratorRef->pathIterRef, name);

        return LE_OK;
    }

    return LE_NOT_FOUND;
}




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
)
//--------------------------------------------------------------------------------------------------
{
    // If the current node exists, then look to it for a sibling node.  Otherwise, a non-existant
    // node can not have siblings.
    if (iteratorRef->currentNodeRef != NULL)
    {
        tdb_NodeRef_t newNodeRef = tdb_GetNextActiveSiblingNode(iteratorRef->currentNodeRef);

        if (newNodeRef == NULL)
        {
            return LE_NOT_FOUND;
        }

        // Looks like we found a new node, so replace the node name at the end of the path.
        iteratorRef->currentNodeRef = newNodeRef;

        char name[LE_CFG_NAME_LEN_BYTES] = "";
        tdb_GetNodeName(newNodeRef, name, sizeof(name));

        if (le_pathIter_GoToEnd(iteratorRef->pathIterRef) != LE_NOT_FOUND)
        {
            le_pathIter_Truncate(iteratorRef->pathIterRef);
        }

        le_pathIter_Append(iteratorRef->pathIterRef, name);

        return LE_OK;
    }

    return LE_NOT_FOUND;
}




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
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(iteratorRef != NULL);
    LE_ASSERT(destBufferPtr != NULL);
    LE_ASSERT(bufferMax > 0);

    // Check to see if they're looking for a path to a node relative to the current one.
    if (   (subPathPtr != NULL)
        && (strcmp(subPathPtr, "") != 0))
    {
        // Build up a new path based on the existing path.
        le_pathIter_Ref_t newPathRef = le_pathIter_Clone(iteratorRef->pathIterRef);
        le_result_t result = le_pathIter_Append(newPathRef, subPathPtr);

        if (result == LE_OVERFLOW)
        {
            tu_TerminateConfigClient(iteratorRef->sessionRef, "Specified path too large.");
        }
        else if (result == LE_UNDERFLOW)
        {
            tu_TerminateConfigClient(iteratorRef->sessionRef,
                                     "Specified path attempts to iterate below root.");
        }
        else
        {
            result = le_pathIter_GetPath(newPathRef, destBufferPtr, bufferMax);
        }

        le_pathIter_Delete(newPathRef);
        return result;
    }

    // Simply return the current path.
    return le_pathIter_GetPath(iteratorRef->pathIterRef, destBufferPtr, bufferMax);
}




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
)
//--------------------------------------------------------------------------------------------------
{
    return tdb_GetNodeType(ni_GetNode(iteratorRef, pathPtr));
}




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
)
//--------------------------------------------------------------------------------------------------
{
    // Make sure we were given a buffer.
    if (bufferMax == 0)
    {
        return LE_OVERFLOW;
    }

    // If we have a current node, get it's name.  Otherwise we'll have to get the name from the
    // path.
    tdb_NodeRef_t nodeRef = ni_GetNode(iteratorRef, pathPtr);

    // If the iterator was closed during the GetNode, then that means there was a fatal problem
    // encountered.
    if (iteratorRef->isTerminated)
    {
        // At this point we know the client has been disconnected.  So just return fault so that the
        // calling code can know this.
        return LE_FAULT;
    }

    if (nodeRef != NULL)
    {
        return tdb_GetNodeName(nodeRef, destBufferPtr, bufferMax);
    }

    // Looks like a node wasn't found.  So, try to get the name of the node from the sub-path.  Or
    // if a sub-path was not specified, get the name from the iterator's base path.
    *destBufferPtr = '\0';

    if (strcmp(pathPtr, "") != 0)
    {
        le_pathIter_Ref_t subPathIter = le_pathIter_CreateForUnix(pathPtr);
        le_result_t result = le_pathIter_GoToEnd(iteratorRef->pathIterRef);

        if (result == LE_OK)
        {
            result = le_pathIter_GetCurrentNode(subPathIter, destBufferPtr, bufferMax);
        }

        le_pathIter_Delete(subPathIter);
        return result;
    }

    LE_ASSERT(le_pathIter_GoToEnd(iteratorRef->pathIterRef) == LE_OK);
    return le_pathIter_GetCurrentNode(iteratorRef->pathIterRef, destBufferPtr, bufferMax);
}




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
)
//--------------------------------------------------------------------------------------------------
{
    tdb_NodeRef_t nodeRef = ni_GetNode(iteratorRef, pathPtr);

    if (nodeRef == NULL)
    {
        return le_utf8_Copy(destBufferPtr, defaultPtr, bufferMax, NULL);
    }

    return tdb_GetValueAsString(nodeRef, destBufferPtr, bufferMax, defaultPtr);
}




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
)
//--------------------------------------------------------------------------------------------------
{
    tdb_NodeRef_t nodeRef = ni_TryCreateNode(iteratorRef, pathPtr);

    if (nodeRef)
    {
        tdb_SetValueAsString(nodeRef, valuePtr);
    }
}




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
)
//--------------------------------------------------------------------------------------------------
{
    tdb_NodeRef_t nodeRef = ni_GetNode(iteratorRef, pathPtr);

    if (nodeRef == NULL)
    {
        return defaultValue;
    }

    return tdb_GetValueAsInt(nodeRef, defaultValue);
}




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
)
//--------------------------------------------------------------------------------------------------
{
    tdb_NodeRef_t nodeRef = ni_TryCreateNode(iteratorRef, pathPtr);

    if (nodeRef)
    {
        tdb_SetValueAsInt(nodeRef, value);
    }
}




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
)
//--------------------------------------------------------------------------------------------------
{
    tdb_NodeRef_t nodeRef = ni_GetNode(iteratorRef, pathPtr);

    if (nodeRef == NULL)
    {
        return defaultValue;
    }

    return tdb_GetValueAsFloat(nodeRef, defaultValue);
}




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
)
//--------------------------------------------------------------------------------------------------
{
    tdb_NodeRef_t nodeRef = ni_TryCreateNode(iteratorRef, pathPtr);

    if (nodeRef)
    {
        tdb_SetValueAsFloat(nodeRef, value);
    }
}




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
)
//--------------------------------------------------------------------------------------------------
{
    tdb_NodeRef_t nodeRef = ni_GetNode(iteratorRef, pathPtr);

    if (nodeRef == NULL)
    {
        return defaultValue;
    }

    return tdb_GetValueAsBool(nodeRef, defaultValue);
}




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
)
//--------------------------------------------------------------------------------------------------
{
    tdb_NodeRef_t nodeRef = ni_TryCreateNode(iteratorRef, pathPtr);

    if (nodeRef)
    {
        tdb_SetValueAsBool(nodeRef, value);
    }
}
