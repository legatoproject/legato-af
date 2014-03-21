
// -------------------------------------------------------------------------------------------------
/*
 *  The core iterator functionality is handled here.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "user.h"
#include "stringBuffer.h"
#include "interfaces.h"
#include "internalCfgTypes.h"
#include "treeDb.h"
#include "iterator.h"
#include "userHandling.h"




// -------------------------------------------------------------------------------------------------
/**
 *  Pool for allocating iterator objects.
 */
// -------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t IteratorPool = NULL;

#define ITERATOR_POOL_NAME "configTree.iteratorPool"
#define INITIAL_MAX_ITERATORS 20




// -------------------------------------------------------------------------------------------------
/**
 *  The pool for handing iterator safe references.
 */
// -------------------------------------------------------------------------------------------------
static le_ref_MapRef_t IteratorRefMap = NULL;

#define ITERATOR_REF_MAP_NAME "configTree.iteratorRefMap"




// -------------------------------------------------------------------------------------------------
/**
 *  Release an iterator and free up it's memory.  If it's a write iterator, merge it's tree data
 *  back into the trees parent.
 */
// -------------------------------------------------------------------------------------------------
static void ReleaseIteratorPtr
(
    IteratorInfo_t* iteratorPtr  ///< The iterator the close down.
)
{
    LE_ASSERT(iteratorPtr != NULL);

    LE_DEBUG("** Releasing iterator pointer.");

// TODO: Check to see if any of the iterators have canceled the write.

    // If this iterator is mearly a clone, simply detach this clone from the original.  Once that's
    // done, free up the iterator's memory.  We don't need to free the config tree because that's
    // owned by the parent iterator.
    IteratorInfo_t* baseIterPtr = iteratorPtr->baseIteratorPtr;

    if (baseIterPtr != NULL)
    {
        LE_DEBUG("** Processing iterator clone.");

        if (iteratorPtr->status == ITER_CANCELED)
        {
            baseIterPtr->status = ITER_CANCELED;
        }

        iteratorPtr->baseIteratorPtr->activeClones--;

        // Release the memory for this iterator.
        le_mem_Release(iteratorPtr);

        // Now, check to see if we can do the same for the parent iterator.  This function is
        // recursive, but only to one level, as all iterator clones are peers of eachother.
        LE_ASSERT(baseIterPtr->baseIteratorPtr == NULL);
        ReleaseIteratorPtr(baseIterPtr);

        return;
    }

    LE_DEBUG("** Releasing master iterator.");

    // Ok.  This is a master iterator.  If the iterator is still active, or has active clones then
    // we can not shut it down just yet.
    if (   (iteratorPtr->status == ITER_OK)
        || (iteratorPtr->activeClones > 0))
    {
        LE_DEBUG("** There are still active clones.");
        return;
    }


    LE_DEBUG("** There are no active clones.");


    if (iteratorPtr->type == ITY_WRITE)
    {
        if (iteratorPtr->status == ITER_COMMITTED)
        {
            LE_DEBUG("** Committing write iterator.");
            tdb_MergeTree(iteratorPtr->rootNodeRef);
            tdb_CommitTree(iteratorPtr->treePtr);
        }

        LE_DEBUG("** Releasing iterator's tree.");
        tdb_ReleaseTree(iteratorPtr->rootNodeRef);

        LE_DEBUG("** Clearing active write operation.");
        LE_ASSERT(iteratorPtr->treePtr->activeWriteIterPtr == iteratorPtr);
        iteratorPtr->treePtr->activeWriteIterPtr = NULL;
    }
    else
    {
        LE_DEBUG("** Clearing active read operation.");
        iteratorPtr->treePtr->activeReadCount--;
    }

    LE_DEBUG("** Free object memory.");
    le_mem_Release(iteratorPtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Convert a iterator status to a le_result_t.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t StatusToResult
(
    IteratorInfo_t* iteratorPtr  ///< The iterator to read.
)
{

    if (iteratorPtr->currentNodeRef == NULL)
    {
        return LE_NOT_PERMITTED;
    }

    le_result_t result;

    switch (iteratorPtr->status)
    {
        case ITER_OK:
            LE_DEBUG("** Iterator OK.");
            result = LE_OK;
            break;

        case ITER_TIMEDOUT:
            LE_DEBUG("** Iterator timed out.");
            result = LE_TIMEOUT;
            break;

        case ITER_CANCELED:
            LE_DEBUG("** Iterator canceled.");
            result = LE_CLOSED;
            break;

        case ITER_COMMITTED:
            LE_DEBUG("** Iterator committed.");
            result = LE_CLOSED;
            break;

        default:
            LE_DEBUG("** Iterator unknown state.");
            result = LE_BAD_PARAMETER;
            break;
    }

    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the current node that the iterator is pointed at.  If the iterator isn't currently on a node
 *  then the iterator's tree root is returned instead.
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t GetCurrentNode
(
    IteratorInfo_t* iteratorPtr,  ///< The iterator to read.
    IteratorGetNodeFlag_t forceRootNode
)
{
    LE_DEBUG("** Getting current node.");
    tdb_NodeRef_t nodePtr = iteratorPtr->currentNodeRef;

    if (   (nodePtr == NULL)
        && (forceRootNode == ITER_GET_DEFAULT_ROOT))
    {
        LE_DEBUG("** No current node, defaulting to root.");
        nodePtr = iteratorPtr->rootNodeRef;
    }

    return nodePtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the path to the tree node in question.
 */
// -------------------------------------------------------------------------------------------------
static char* PathFromNode
(
    tdb_NodeRef_t nodeRef  ///< The node to trace.
)
{
    // Get storage for the new string.
    char* bufferPtr = sb_Get();

    // The way this function works is to start at the current node, write that name to the end of
    // the new string.  Then, going up the node chane we prepend each name onto the path until we
    // hit the root node.

    // Start at the end of the path, and move backwords twice, once for the trailing NULL.
    char* workingPtr = bufferPtr + SB_SIZE;
    workingPtr--;


    // Keep going while we have parents to go through.
    while (nodeRef != NULL)
    {
        // If the node is a stem, then make sure that we leave room for a slash.
        le_cfg_nodeType_t type = tdb_GetTypeId(nodeRef);

        if (type == LE_CFG_TYPE_STEM)
        {
            workingPtr--;

            if (workingPtr < bufferPtr)
            {
                workingPtr = bufferPtr;
                break;
            }
        }


        // Now figure out how much space we need in the string for the this node's name.
        size_t len = tdb_GetNameLength(nodeRef);
        workingPtr -= len;

        // Make sure that we didn't underflow.  If we did back up back to the last level.
        if (workingPtr < bufferPtr)
        {
            workingPtr += len;

            if (type == LE_CFG_TYPE_STEM)
            {
                workingPtr++;
            }

            break;
        }

        // Get the node name and copy it directly into our working path.
        tdb_GetName(nodeRef, workingPtr, len);

        if (type == LE_CFG_TYPE_STEM)
        {
            workingPtr[len] = '/';
        }

        // Jump up to the parent and do it all again.
        nodeRef = tdb_GetParentNode(nodeRef);
    }


    LE_DEBUG("** Computed node path, <%s>.", workingPtr);


    // The calling code expects a string buffer it can simply free, but the start pointer is
    // somewhere in the middle of the buffer.  So, create a returnable buffer and copy the completed
    // path string out to the beginning of the new buffer.

    char* finalBuffer = sb_Get();
    strncpy(finalBuffer, workingPtr, SB_SIZE);

    sb_Release(bufferPtr);

    return finalBuffer;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Initialize the memory structures needed by the iterator subsystem.
 */
// -------------------------------------------------------------------------------------------------
void itr_Init
(
)
{
    IteratorPool = le_mem_CreatePool(ITERATOR_POOL_NAME, sizeof(IteratorInfo_t));
    le_mem_ExpandPool(IteratorPool, INITIAL_MAX_ITERATORS);

    IteratorRefMap = le_ref_CreateMap(ITERATOR_REF_MAP_NAME, INITIAL_MAX_ITERATORS);
}



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
)
{
    switch (type)
    {
        case ITY_READ:  return "read";
        case ITY_WRITE: return "write";
        default:    LE_FATAL("Invalid transaction type %d", type);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Create an iterator object that's invalid.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_iteratorRef_t itr_NewInvalidRef
(
)
{
    IteratorInfo_t* iteratorPtr = le_mem_ForceAlloc(IteratorPool);
    memset(iteratorPtr, 0, sizeof(IteratorInfo_t));

    iteratorPtr->userId = -1;
    iteratorPtr->type = ITY_READ;
    iteratorPtr->status = ITER_CANCELED;

    return le_ref_CreateRef(IteratorRefMap, iteratorPtr);
}




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
)
{
    // Make sure that everything is sane.
    LE_ASSERT(treePtr != NULL);
    LE_ASSERT(userPtr != NULL);
    LE_ASSERT(type == ITY_READ || type == ITY_WRITE);
    LE_ASSERT(initialPathPtr != NULL);


    // Create our default init.  Untouched fields are left at 0.
    IteratorInfo_t* iteratorPtr = le_mem_ForceAlloc(IteratorPool);
    memset(iteratorPtr, 0, sizeof(IteratorInfo_t));

    iteratorPtr->userId = userPtr->userId;
    iteratorPtr->treePtr = treePtr;
    iteratorPtr->sessionRef = sessionRef;
    iteratorPtr->type = type;
    iteratorPtr->status = ITER_OK;


    // If this is a write iterator, we need to shadow the tree for the write transaction.  So, we
    // take care to do this before we even attempt to look for the path the user wanted us to find.

    iteratorPtr->rootNodeRef = treePtr->rootNodeRef;

    if (iteratorPtr->type == ITY_WRITE)
    {
        LE_DEBUG("** Create shadow tree for write iterator and register against tree.");
        iteratorPtr->rootNodeRef = tdb_ShadowTree(iteratorPtr->rootNodeRef);

        LE_ASSERT(treePtr->activeWriteIterPtr == NULL);
        treePtr->activeWriteIterPtr = iteratorPtr;
    }
    else
    {
        LE_DEBUG("** Register read iterator against tree.");
        treePtr->activeReadCount++;
    }


    iteratorPtr->currentNodeRef = tdb_GetNode(iteratorPtr->rootNodeRef, initialPathPtr, false);


    // Create and return a new safe ref.
    le_cfg_iteratorRef_t iteratorRef = le_ref_CreateRef(IteratorRefMap, iteratorPtr);

    LE_DEBUG("Created a new %s iterator object <%p> for user %u (%s).",
             itr_TxnTypeString(type),
             iteratorRef,
             userPtr->userId,
             userPtr->userName);

    return iteratorRef;

}




// -------------------------------------------------------------------------------------------------
/**
 *  Commit the interator data to the original tree.
 */
// -------------------------------------------------------------------------------------------------
le_result_t itr_Commit
(
    UserInfo_t* userPtr,              ///< The user we are getting an iterator for.
    le_cfg_iteratorRef_t iteratorRef  ///< The iterator to commit and free.
)
{
    LE_DEBUG("** ITER Commit.");


    // Make sure that the iterator ref is valid for the given user.

    LE_ASSERT(userPtr != NULL);
    LE_ASSERT(iteratorRef != NULL);

    IteratorInfo_t* iteratorPtr = itr_GetPtr(userPtr, iteratorRef);

    if (iteratorPtr == NULL)
    {
        return LE_BAD_PARAMETER;
    }


    // Mark the iterator as comitted, and destroy the safe ref right away.  This is so that if we
    // need to keep the iterator around because other iterators have cloned it the user can not
    // accidently inappropriatly access it.

    iteratorPtr->status = ITER_COMMITTED;

    le_ref_DeleteRef(IteratorRefMap, iteratorRef);


    // Now we try to release the iterator.  If it needs to be kept around because there are clones
    // of it, it will get fully freed when the last clone is destroyed.
    ReleaseIteratorPtr(iteratorPtr);

    return LE_OK;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Release the iterator without changing the original tree.
 */
// -------------------------------------------------------------------------------------------------
void itr_Release
(
    UserInfo_t* userPtr,           ///< The user we are getting an iterator for.
    le_cfg_iteratorRef_t iteratorRef  ///< The iterator to free.
)
{
    // Make sure that we have a valid iterator.
    LE_DEBUG("** ITER Release.");

    LE_ASSERT(userPtr != NULL);
    LE_ASSERT(iteratorRef != NULL);

    IteratorInfo_t* iteratorPtr = itr_GetPtr(userPtr, iteratorRef);

    if (iteratorPtr == NULL)
    {
        return;
    }


    // Mark the iterator as having been canceled.  This way if we have a write iterator no data will
    // end up being comitted.

    iteratorPtr->status = ITER_CANCELED;

    le_ref_DeleteRef(IteratorRefMap, iteratorRef);
    ReleaseIteratorPtr(iteratorPtr);
}




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
)
{
    LE_ASSERT(functionPtr != NULL);

    le_ref_IterRef_t refIterator = le_ref_GetIterator(IteratorRefMap);

    while (le_ref_NextNode(refIterator) == LE_OK)
    {
        IteratorInfo_t const* iteratorPtr = le_ref_GetValue(refIterator);

        if (   (iteratorPtr != NULL)
            && (iteratorPtr->sessionRef == sessionRef))
        {
            functionPtr((le_cfg_iteratorRef_t)le_ref_GetSafeRef(refIterator),
                        (IteratorInfo_t*)iteratorPtr,
                        contextPtr);
        }
    }
}




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
)
{
    LE_ASSERT(functionPtr != NULL);

    le_ref_IterRef_t refIterator = le_ref_GetIterator(IteratorRefMap);

    while (le_ref_NextNode(refIterator) == LE_OK)
    {
        IteratorInfo_t const* iteratorPtr = le_ref_GetValue(refIterator);

        if (   (iteratorPtr != NULL)
            && (iteratorPtr->treePtr == treePtr))
        {
            functionPtr((le_cfg_iteratorRef_t)le_ref_GetSafeRef(refIterator),
                        (IteratorInfo_t*)iteratorPtr,
                        contextPtr);
        }
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check an iterator reference and make sure it's valid.  An iterator ref can be invalid either
 *  because the handle itself is bad, or the iterator can be in a "bad" state.
 */
// -------------------------------------------------------------------------------------------------
le_result_t itr_CheckRef
(
    UserInfo_t* userPtr,           ///< Checking the iterator for this user.
    le_cfg_iteratorRef_t iteratorRef  ///< The iterator we're checking.
)
{
    LE_DEBUG("** Check iterator ref <%p>", iteratorRef);

    // Do we even have a valid ref?
    if (iteratorRef == NULL)
    {
        LE_DEBUG("** Null ref.");
        return LE_BAD_PARAMETER;
    }


    LE_DEBUG("** Safe ref lookup.");

    // Look it up from the ref map.  If it can't be found, or the userId is a mismatch then treat
    // the ref as bad.  I don't want different users trying to look up each other's handles.
    IteratorInfo_t* iteratorPtr = le_ref_Lookup(IteratorRefMap, iteratorRef);

    if (   (iteratorPtr == NULL)
        || (iteratorPtr->userId != userPtr->userId))
    {
        LE_DEBUG("** Iterator not found or bad user.");
        return LE_BAD_PARAMETER;
    }


    // Finally report back the state of the iterator we found.
    return StatusToResult(iteratorPtr);
}




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
)
{
    LE_DEBUG("** Get iterator ptr from ref <%p>", iteratorRef);

    // Do we even have a valid ref?
    if (iteratorRef == NULL)
    {
        return NULL;
    }


    LE_DEBUG("** Safe ref lookup.");

    // Look it up from the ref map.  If it can't be found, or the userId is a mismatch then treat
    // the ref as bad.  I don't want different users trying to look up each other's handles.
    IteratorInfo_t* iteratorPtr = le_ref_Lookup(IteratorRefMap, iteratorRef);

    if (   (iteratorPtr == NULL)
        || (iteratorPtr->userId != userPtr->userId))
    {
        LE_DEBUG("** Iterator not found or bad user.");
        return NULL;
    }


    // Everything looks good, return the pointer.
    return iteratorPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Create a clone of an iterator pointer and return that clone as a safe reference.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_iteratorRef_t itr_Clone
(
    IteratorInfo_t* iteratorPtr  ///< The iterator object to clone.
)
{
    LE_ASSERT(iteratorPtr != NULL);

    if (iteratorPtr->status != ITER_OK)
    {
        LE_DEBUG("** Attempt to clone bad iterator.");
        return NULL;
    }

    // Find the base iterator in the chain.  It's either going to be the iterator passed in, or at
    // most one level of nesting to a parent iterator.
    IteratorInfo_t* originalPtr = iteratorPtr->baseIteratorPtr != NULL
                                ? iteratorPtr->baseIteratorPtr
                                : iteratorPtr;

    LE_ASSERT(originalPtr->baseIteratorPtr == NULL);


    // Start copying values from the base itertor.  Untouched fields are left at 0.
    IteratorInfo_t* clonePtr = le_mem_ForceAlloc(IteratorPool);
    memset(clonePtr, 0, sizeof(IteratorInfo_t));

    clonePtr->userId = originalPtr->userId;
    clonePtr->type = originalPtr->type;
    clonePtr->status = ITER_OK;

    clonePtr->currentNodeRef = iteratorPtr->currentNodeRef;

    if (clonePtr->currentNodeRef != NULL)
    {
        clonePtr->rootNodeRef = tdb_GetRootNode(clonePtr->currentNodeRef);
    }
    else
    {
        LE_ASSERT(iteratorPtr->rootNodeRef != NULL);

        clonePtr->rootNodeRef = iteratorPtr->rootNodeRef;

        if (originalPtr->type == ITY_WRITE)
        {
            clonePtr->rootNodeRef = tdb_ShadowTree(clonePtr->rootNodeRef);
        }

        clonePtr->currentNodeRef = clonePtr->rootNodeRef;
    }

    // Make sure that we've properly bound our iterator into the chain.  Also, let the original
    // iterator keep track of it's children.
    clonePtr->baseIteratorPtr = iteratorPtr;
    iteratorPtr->activeClones++;

    originalPtr->activeClones++;


    return le_ref_CreateRef(IteratorRefMap, clonePtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check to see if an iterator represents an active transaction.
 */
// -------------------------------------------------------------------------------------------------
bool itr_IsClosed
(
    IteratorInfo_t* iteratorPtr  ///< The iterator object to query.
)
{
    return iteratorPtr->status != ITER_OK;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check to see if an iterator represents a write transaction.
 */
// -------------------------------------------------------------------------------------------------
bool itr_IsWriteIterator
(
    IteratorInfo_t* iteratorPtr  ///< The iterator object to query.
)
{
    LE_ASSERT(iteratorPtr != NULL);
    return (iteratorPtr->type == ITY_WRITE) && (iteratorPtr->status == ITER_OK);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the tree this iterator was created on.
 */
// -------------------------------------------------------------------------------------------------
TreeInfo_t* itr_GetTree
(
    IteratorInfo_t* iteratorPtr  ///< The iterator object to query.
)
{
    return iteratorPtr->treePtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Move the iterator to the node specified by the path.
 */
// -------------------------------------------------------------------------------------------------
le_result_t itr_GoToNode
(
    IteratorInfo_t* iteratorPtr,  ///< The iterator to move.
    const char* pathPtr           ///< The path to move the iterator to.
)
{
    LE_ASSERT(iteratorPtr != NULL);
    LE_ASSERT(pathPtr != NULL);

    if (iteratorPtr->status == ITER_OK)
    {
        tdb_NodeRef_t nodePtr = tdb_GetNode(GetCurrentNode(iteratorPtr,
                                                                 ITER_GET_DEFAULT_ROOT),
                                                  pathPtr,
                                                  false);

        // If the node is NULL it means we have tried to navigate to a
        // non-existent node in a read transaction
        if (NULL == nodePtr)
        {
            return LE_NOT_FOUND;
        }

        iteratorPtr->currentNodeRef = nodePtr;
    }

    return StatusToResult(iteratorPtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Move the iterator to the parent of the node that the iterator is currently pointed at.  If the
 *  iterator is currently at the root of the tree then iterator will be set as a bad path.
 */
// -------------------------------------------------------------------------------------------------
le_result_t itr_GoToParent
(
    IteratorInfo_t* iteratorPtr  ///< The iterator to move.
)
{
    LE_ASSERT(iteratorPtr != NULL);

    if (iteratorPtr->status == ITER_OK)
    {
        iteratorPtr->currentNodeRef = tdb_GetParentNode(GetCurrentNode(iteratorPtr,
                                                                       ITER_GET_NO_DEFAULT_ROOT));
    }

    return StatusToResult(iteratorPtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Go to the first child of the node that the iterator is pointed at.
 */
// -------------------------------------------------------------------------------------------------
le_result_t itr_GoToFirstChild
(
    IteratorInfo_t* iteratorPtr  ///< The iterator to move.
)
{
    LE_ASSERT(iteratorPtr != NULL);

    if (iteratorPtr->status == ITER_OK)
    {
        tdb_NodeRef_t newRef = tdb_GetFirstActiveChildNode(GetCurrentNode(iteratorPtr,
                                                                         ITER_GET_NO_DEFAULT_ROOT));

        // Was a child node found?  If it wasn't report it as such.  Otherwise update our current
        // node and report our status.

        if (newRef != NULL)
        {
            iteratorPtr->currentNodeRef = newRef;
            return StatusToResult(iteratorPtr);
        }

        return LE_NOT_FOUND;
    }

    return LE_NOT_PERMITTED;
}




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
)
{
    LE_ASSERT(iteratorPtr != NULL);

    // If there is no current node, then there isn't much else to do.  Return false to indicate that
    // there are no more siblings.
    if (iteratorPtr->currentNodeRef == NULL)
    {
        return false;
    }

    // There is a current node so there is a possiblity of it having siblings.
    tdb_NodeRef_t nextPtr = NULL;

    if (iteratorPtr->status == ITER_OK)
    {
        // Attempt to grab the next sibling in the chain.
        nextPtr = tdb_GetNextActiveSibling(iteratorPtr->currentNodeRef);

        if (nextPtr != NULL)
        {
            // It was successful, so update the current noded.  Otherwise the iterator returns false
            // and stays where it is.
            iteratorPtr->currentNodeRef = nextPtr;
        }
        else
        {
            LE_DEBUG("** End of sibling chain.");
        }
    }

    // Let the user know if we successfuly moved or not.
    return nextPtr != NULL;
}




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
)
{
    LE_ASSERT(iteratorPtr != NULL);

    if (iteratorPtr->status != ITER_OK)
    {
        // There is no name to get, so return an empty string.
        return sb_Get();
    }


    // We have a current node, so get it's name.
    char* stringPtr = sb_Get();
    tdb_GetName(GetCurrentNode(iteratorPtr, ITER_GET_NO_DEFAULT_ROOT), stringPtr, SB_SIZE);

    return stringPtr;
}




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
)
{
    LE_ASSERT(iteratorPtr != NULL);

    if (iteratorPtr->status != ITER_OK)
    {
        // There is no path to get, so return an empty string.
        return sb_Get();
    }

    // Crab the current node and generate a path string for it.
    return PathFromNode(GetCurrentNode(iteratorPtr, ITER_GET_NO_DEFAULT_ROOT));
}




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
)
{
    LE_ASSERT(iteratorPtr != NULL);

    if (iteratorPtr->status != ITER_OK)
    {
        return sb_Get();
    }

    // Get the parent of the current node and generate a path string for it.
    return PathFromNode(tdb_GetParentNode(GetCurrentNode(iteratorPtr, ITER_GET_NO_DEFAULT_ROOT)));
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the type of the node the iterator is currently pointing at.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_nodeType_t itr_GetNodeType
(
    IteratorInfo_t* iteratorPtr  ///< The iterator to read.
)
{
    LE_ASSERT(iteratorPtr != NULL);

    if (iteratorPtr->status != ITER_OK)
    {
        return LE_CFG_TYPE_DENIED;
    }

    return tdb_GetTypeId(GetCurrentNode(iteratorPtr, ITER_GET_NO_DEFAULT_ROOT));
}




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
)
{
    LE_ASSERT(iteratorPtr != NULL);

    if (iteratorPtr->status != ITER_OK)
    {
        return NULL;
    }

    return tdb_GetNode(GetCurrentNode(iteratorPtr, getNodeFlag), pathPtr, false);
}
