
// -------------------------------------------------------------------------------------------------
/**
 *  @file configTreeApi.c
 *
 *  Highlevel impoementation of the configuration Tree API.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "dynamicString.h"
#include "treeDb.h"
#include "treeUser.h"
#include "treePath.h"
#include "nodeIterator.h"
#include "requestQueue.h"




// -------------------------------------------------------------------------------------------------
/**
 *  Handle both the create read and write transacion requests.
 */
// -------------------------------------------------------------------------------------------------
static void CreateTransaction
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Message context for the request.
    ni_IteratorType_t request,         ///< [IN] The requested iterator type to create.
    const char* pathPtr                ///< [IN] Initial path for the transaction.
)
// -------------------------------------------------------------------------------------------------
{
    // Check to see if this user has access to the tree/path in question.
    tu_UserRef_t userRef = tu_GetCurrentConfigUserInfo();
    tdb_TreeRef_t treeRef = NULL;

    if (request == NI_WRITE)
    {
        treeRef = tu_GetRequestedTree(userRef, TU_TREE_WRITE, pathPtr);
    }
    else
    {
        treeRef = tu_GetRequestedTree(userRef, TU_TREE_READ, pathPtr);
    }

    if (treeRef == NULL)
    {
        tu_TerminateConfigClient(le_cfg_GetClientSessionRef(),
                                 "A configuration tree could not be opened for a new transaction.");
    }
    else
    {
        // Try to create the new iterator.  If it can't be created now, it'll be queued for
        // creation later.
        rq_HandleCreateTxnRequest(userRef,
                                  treeRef,
                                  le_cfg_GetClientSessionRef(),
                                  commandRef,
                                  request,
                                  tp_GetPathOnly(pathPtr));
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get an iterator pointer from an iterator reference.
 *
 *  @return An internal reference to the iterator.  Or NULL the safe reference could not be
 *          resolved.
 */
// -------------------------------------------------------------------------------------------------
static ni_IteratorRef_t GetIteratorFromRef
(
    le_cfg_IteratorRef_t externalRef  ///< [IN] The iterator reference to extract a pointer from.
)
// -------------------------------------------------------------------------------------------------
{
    ni_IteratorRef_t iteratorRef = ni_InternalRefFromExternalRef(tu_GetCurrentConfigUserInfo(),
                                                                 externalRef);

    if (iteratorRef == NULL)
    {
        tu_TerminateConfigClient(le_cfg_GetClientSessionRef(), "Bad iterator reference.");
    }

    return iteratorRef;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get an iterator pointer from an iterator reference.
 *
 *  @return An internal reference to the iterator.  Or NULL the safe reference could not be
 *          resolved.  NULL is also returned if the iterator in question is not writeable.
 */
// -------------------------------------------------------------------------------------------------
static ni_IteratorRef_t GetWriteIteratorFromRef
(
    le_cfg_IteratorRef_t externalRef  ///< [IN] Iterator reference to extract a pointer from.
)
// -------------------------------------------------------------------------------------------------
{
    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);

    if (   (iteratorRef != NULL)
        && (ni_IsWriteable(iteratorRef) == false))
    {
        tu_TerminateConfigClient(le_cfg_GetClientSessionRef(),
                                 "This operation requires a write iterator.");

        return NULL;
    }

    return iteratorRef;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check the given path and make sure that it doesn't try to change trees.
 */
// -------------------------------------------------------------------------------------------------
static bool CheckPathForSpecifier
(
    const char* pathPtr  ///< [IN] Path string to check.
)
// -------------------------------------------------------------------------------------------------
{
    if (tp_PathHasTreeSpecifier(pathPtr))
    {
        tu_TerminateConfigClient(le_cfg_GetClientSessionRef(),
                                 "Can not change trees in the middle of a transaction.");

        return true;
    }

    return false;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check the size of a requested string buffer.  If it's larger than what we can handle
 *  internally, truncate it to what we can handle.
 */
// -------------------------------------------------------------------------------------------------
static size_t MaxStr
(
    size_t requestedMax  ///< [IN] Requested maximum string size.
)
// -------------------------------------------------------------------------------------------------
{
    if (requestedMax > LE_CFG_STR_LEN_BYTES)
    {
        LE_DEBUG("Truncating output string buffer from %" PRIdS " to %d.",
                 requestedMax,
                 LE_CFG_STR_LEN_BYTES);

        return LE_CFG_STR_LEN_BYTES;
    }

    return requestedMax;
}


// -------------------------------------------------------------------------------------------------
/**
 *  Check the size of a requested binary data buffer.  If it's larger than what we can handle
 *  internally, truncate it to what we can handle.
 */
// -------------------------------------------------------------------------------------------------
static size_t MaxBinary
(
    size_t requestedMax  ///< [IN] Requested maximum string size.
)
// -------------------------------------------------------------------------------------------------
{
    if (requestedMax > LE_CFG_BINARY_LEN)
    {
        LE_DEBUG("Truncating output binary buffer from %zd to %d.",
                 requestedMax,
                 LE_CFG_BINARY_LEN);

        return LE_CFG_BINARY_LEN;
    }

    return requestedMax;
}


// -------------------------------------------------------------------------------------------------
/**
 *  Called by the "Quick" functions to get a reference to the tree the user wants.  If the tree
 *  retreival fails for any reason, (as in, permission error,) terminate the client.
 *
 *  @note If the permission check fails, then terminate client will be called.
 *
 *  @return A reference to the requested tree.  Otherwise, NULL, if permission check fails.
 */
// -------------------------------------------------------------------------------------------------
static tdb_TreeRef_t QuickGetTree
(
    tu_UserRef_t userRef,            ///< [IN] Get a tree for this user.
    tu_TreePermission_t permission,  ///< [IN] Try to get a tree with this permission.
    const char* pathPtr              ///< [IN] Path to check.
)
// -------------------------------------------------------------------------------------------------
{
    tdb_TreeRef_t treeRef = tu_GetRequestedTree(userRef, permission, pathPtr);

    if (treeRef == NULL)
    {
        tu_TerminateConfigClient(le_cfg_GetClientSessionRef(),
                                 "The requested configuration tree could not be opened.");
    }

    return treeRef;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Create a read transaction and open a new iterator for traversing the configuration tree.
 *
 *  @note: This action creates a read lock on the given tree.  Which will start a read-timeout.
 *         Once the read timeout expires, then all active read iterators on that tree will be
 *         expired and the clients killed.
 *
 *  @note: A tree transaction is global to that tree; a long held read transaction will block other
 *         users write transactions from being comitted.
 *
 *  @return This will return a newly created iterator reference.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_CreateReadTxn
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    const char* basePathPtr            ///< [IN] Path to the location to create the new iterator.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Creating a new read transaction on path <%s>.", basePathPtr);
    CreateTransaction(commandRef, NI_READ, basePathPtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Create a write transaction and open a new iterator for both reading and writing.
 *
 *  @note: This action creates a write transaction. If the application holds the iterator for
 *         longer than the configured write transaction timeout, the iterator will cancel the
 *         transaction.  All further reads will fail to return data and all writes will be thrown
 *         away.
 *
 *  @note A tree transaction is global to that tree, so a long held write transaction will block
 *        other user's write transactions from being started.  However other trees in the system
 *        will be unaffected.
 *
 *  \b Responds \b With:
 *
 *  This will respond with a newly created iterator reference.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_CreateWriteTxn
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    const char* basePathPtr            ///< [IN] Path to the location to create the new iterator.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Creating a new write transaction on path <%s>.", basePathPtr);
    CreateTransaction(commandRef, NI_WRITE, basePathPtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Close the write iterator and commit the write transaction.  This updates the config tree
 *  with all of the writes that occured using the iterator.
 *
 *  @note: This operation will also delete the iterator object.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_CommitTxn
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef   ///< [IN] Iterator object to commit.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Comitting a tree transaction on iterator ref: <%p>.", externalRef);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);

    if (iteratorRef)
    {
        rq_HandleCommitTxnRequest(commandRef, iteratorRef);
    }
    else
    {
        le_cfg_CommitTxnRespond(commandRef);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Close and free the given iterator object.  If the iterator is a write iterator, the transaction
 *  will be canceled.  If the iterator is a read iterator, the transaction will be closed.
 *
 *  @note: This operation will also delete the iterator object.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_CancelTxn
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef   ///< [IN] Iterator object to close.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Cancelling a transaction on iterator ref: <%p>.", externalRef);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);

    if (iteratorRef)
    {
        rq_HandleCancelTxnRequest(commandRef, iteratorRef);
    }
    else
    {
        le_cfg_CancelTxnRespond(commandRef);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Change the node that the iterator is pointing to.  The path passed can be an absolute or a
 *  relative path from the iterators current location.
 *
 *  The target node does not need to exist.  When a write iterator is used to go to a non-existant
 *  node, the node is automaticly created when a value is written to it or any of it's children.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GoToNode
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator to move.
    const char* newPathPtr             ///< [IN] Absolute or relative path from the current
                                       ///<      location.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Changing iterator <%p> to location, \"%s\".", externalRef, newPathPtr);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);

    if (   (iteratorRef != NULL)
        && (CheckPathForSpecifier(newPathPtr) == false))
    {
        le_result_t result = ni_GoToNode(iteratorRef, newPathPtr);

        if (result == LE_UNDERFLOW)
        {
            tu_TerminateConfigClient(le_cfg_GetClientSessionRef(),
                                     "An attempt was made to traverse up past the root node.");
        }
        else if (result == LE_OVERFLOW)
        {
            tu_TerminateConfigClient(le_cfg_GetClientSessionRef(),
                                     "Internal path buffer overflow.");
        }
    }

    le_cfg_GoToNodeRespond(commandRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Move the iterator to the parent of the node for the iterator.
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *          - LE_OK        - Commit was completed successfuly.
 *          - LE_NOT_FOUND - Current node is the root node: has no parent.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GoToParent
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef   ///< [IN] Iterator to move.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Moving iterator <%p> to parent node.", externalRef);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);
    le_result_t result = LE_OK;

    if (iteratorRef)
    {
        result = ni_GoToParent(iteratorRef);
    }

    le_cfg_GoToParentRespond(commandRef, result);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Move the iterator to the the first child of the node that the iterator is currently pointed.
 *
 *  For read iterators without children, this function will fail.  If the iterator is a write
 *  iterator, then a new node is automatically created.  If this node or an newly created
 *  children of this node are not written to, then this node will not persist even if the iterator is
 *  comitted.
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *          - LE_OK        - Move was completed successfuly.
 *          - LE_NOT_FOUND - The given node has no children.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GoToFirstChild
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef   ///< [IN] Iterator object to move.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Moving iterator <%p> to first child node.", externalRef);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);
    le_result_t result = LE_OK;

    if (iteratorRef)
    {
        result = ni_GoToFirstChild(iteratorRef);
    }

    le_cfg_GoToFirstChildRespond(commandRef, result);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Jump the iterator to the next child node of the current node.  Assuming the following tree:
 *
 *  @code
 *  baseNode/
 *    childA/
 *      valueA
 *      valueB
 *  @endcode
 *
 *  If the iterator is moved to the path, "/baseNode/childA/valueA".  After the first
 *  GoToNextSibling the iterator will be pointing at valueB.  A second call to GoToNextSibling
 *  will cause the function to return LE_NOT_FOUND.
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *          - LE_OK            - Commit was completed successfuly.
 *          - LE_NOT_FOUND     - Iterator has reached the end of the current list of siblings.
 *                               Also returned if the the current node has no siblings.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GoToNextSibling
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef   ///< [IN] Iterator to iterate.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Moving iterator <%p> to next sibling of the current node.", externalRef);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);
    le_result_t result = LE_OK;

    if (iteratorRef)
    {
        result = ni_GoToNextSibling(iteratorRef);
    }

    le_cfg_GoToNextSiblingRespond(commandRef, result);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get path to the node that the iterator is currently pointed.
 *
 *  Assuming the following tree:
 *
 *  @code
 *  baseNode/
 *    childA/
 *      valueA
 *      valueB
 *  @endcode
 *
 *  If the iterator was currently pointing at valueA, then GetPath would return the following path:
 *
 *  @code
 *  /baseNode/childA/valueA
 *  @endcode
 *
 *  Optionally, a path to another node can be supplied to this function.  If the iterator is
 *  again on valueA and the relative path "..." is supplied, this function will return the
 *  following path:
 *
 *  @code
 *  /baseNode/childA/
 *  @endcode
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *          - LE_OK            - The write was completed successfuly.
 *          - LE_OVERFLOW      - The supplied string buffer was not large enough to hold the value.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GetPath
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator to move.
    const char* pathPtr,               ///< [IN] Absolute or relative path to read from.
    size_t maxNewPath                  ///< [IN] Maximum size of the result string.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Reading the iterator's <%p> current path.", externalRef);
    LE_DEBUG_IF((pathPtr != NULL) && (strlen(pathPtr) != 0), "** Offset by \"%s\"", pathPtr);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    le_result_t result = LE_OK;

    if ((NULL != pathPtr) && (NULL != iteratorRef)
        && (false == CheckPathForSpecifier(pathPtr)))
    {
        result = ni_GetPathForNode(iteratorRef, pathPtr, strBuffer, MaxStr(maxNewPath));
    }

    le_cfg_GetPathRespond(commandRef, result, strBuffer);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the type of node that the iterator is currently pointing at.
 *
 *  \b Responds \b With:
 *
 *  le_cfg_nodeType_t value indicating the stored value.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GetNodeType
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator object to use to read from the tree.
    const char* pathPtr                ///< [IN] Absolute or relative path to read from.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Reading the iterator's <%p> current node's type.", externalRef);
    LE_DEBUG_IF((pathPtr != NULL) && (strlen(pathPtr) != 0), "** Offset by \"%s\"", pathPtr);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);
    le_cfg_nodeType_t nodeType = LE_CFG_TYPE_DOESNT_EXIST;

    if ((NULL != pathPtr) && (NULL != iteratorRef)
        && (false == CheckPathForSpecifier(pathPtr)))
    {
        nodeType = ni_GetNodeType(iteratorRef, pathPtr);
    }

    le_cfg_GetNodeTypeRespond(commandRef, nodeType);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the name of the node that the iterator is currently pointing at.
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *          - LE_OK       Write was completed successfuly.
 *          - LE_OVERFLOW Supplied string buffer was not large enough to hold the value.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GetNodeName
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator object to use to read from the tree.
    const char* pathPtr,               ///< [IN] Absolute or relative path to read from.
    size_t maxName                     ///< [IN] Maximum size of the result string.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Reading the iterator's <%p> current node's name.", externalRef);
    LE_DEBUG_IF((pathPtr != NULL) && (strlen(pathPtr) != 0), "** Offset by \"%s\"", pathPtr);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    le_result_t result = LE_OK;

    if ((NULL != pathPtr) && (NULL != iteratorRef)
        && (false == CheckPathForSpecifier(pathPtr)))
    {
        result = ni_GetNodeName(iteratorRef, pathPtr, strBuffer, MaxStr(maxName));
    }

    le_cfg_GetNodeNameRespond(commandRef, result, strBuffer);
}




// -------------------------------------------------------------------------------------------------
//  Update handling.
// -------------------------------------------------------------------------------------------------




// -------------------------------------------------------------------------------------------------
/**
 *  Register a call back on a given node object.  Once registered, this function is called if the
 *  node or if any of it's children are read from, written to, created or deleted.
 *
 *  @return A handle to the event registration.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_ChangeHandlerRef_t le_cfg_AddChangeHandler
(
    const char* newPathPtr,                 ///< [IN] Path to the object to watch.
    le_cfg_ChangeHandlerFunc_t handlerPtr,  ///< [IN] Function to call back.
    void* contextPtr                        ///< [IN] Context to give the function when called.
)
// -------------------------------------------------------------------------------------------------
{
    tu_UserRef_t userRef = tu_GetCurrentConfigUserInfo();
    le_cfg_ChangeHandlerRef_t handlerRef = NULL;

    if (userRef != NULL)
    {
        tdb_TreeRef_t treeRef = tu_GetRequestedTree(userRef, TU_TREE_READ, newPathPtr);

        if (treeRef != NULL)
        {
            handlerRef = tdb_AddChangeHandler(treeRef,
                                              le_cfg_GetClientSessionRef(),
                                              newPathPtr,
                                              handlerPtr,
                                              contextPtr);
        }
    }

    if (handlerRef == NULL)
    {
        tu_TerminateConfigClient(le_cfg_GetClientSessionRef(),
                                 "Change handler registration failed.");
    }

    return handlerRef;
}




//--------------------------------------------------------------------------------------------------
/**
 * This function removes a handler.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_RemoveChangeHandler
(
    le_cfg_ChangeHandlerRef_t handlerRef  ///< [IN] Previously registered handler to remove.
)
// -------------------------------------------------------------------------------------------------
{
    tdb_RemoveChangeHandler(handlerRef, le_cfg_GetClientSessionRef());
}




// -------------------------------------------------------------------------------------------------
//  Transactional reading/writing, creation/deletion.
// -------------------------------------------------------------------------------------------------




// -------------------------------------------------------------------------------------------------
/**
 *  Delete the node specified by the path.  If the node doesn't exist, nothing happens.  All child
 *  nodes are also deleted.
 *
 *  If the path is empty, the iterator's current node is deleted.
 *
 *  Only valid during a write transaction.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_DeleteNode
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator to use as a basis for the transaction.
    const char* pathPtr                ///< [IN] Absolute or relative path to the node to delete.
                                       ///<      If absolute path is given, it's rooted off of the
                                       ///<      user's root node.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Deleting iterator's <%p> current node.", externalRef);
    LE_DEBUG_IF((pathPtr != NULL) && (strlen(pathPtr) != 0), "** Offset by \"%s\"", pathPtr);

    ni_IteratorRef_t iteratorRef = GetWriteIteratorFromRef(externalRef);

    if ((NULL != pathPtr) && (NULL != iteratorRef)
        && (false == CheckPathForSpecifier(pathPtr)))
    {
        ni_DeleteNode(iteratorRef, pathPtr);
    }

    le_cfg_DeleteNodeRespond(commandRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check if the given node is empty.  A node is considered empty if it has no value.  A node is
 *  also considered empty if it doesn't yet exist.
 *
 *  If the path is empty, the iterator's current node is queried for emptiness.
 *
 *  Valid for both read and write transactions.
 *
 *  \b Responds \b With:
 *
 *  A true if the node is considered empty, false if not.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_IsEmpty
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator to use as a basis for the transaction.
    const char* pathPtr                ///< [IN] Absolute or relative path to read from.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Checking to see if an iterator's <%p> current node is empty.", externalRef);
    LE_DEBUG_IF((pathPtr != NULL) && (strlen(pathPtr) != 0), "** Offset by \"%s\"", pathPtr);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);
    bool isEmpty = false;

    if ((NULL != pathPtr) && (NULL != iteratorRef)
        && (false == CheckPathForSpecifier(pathPtr)))
    {
        isEmpty = ni_IsEmpty(iteratorRef, pathPtr);
    }

    le_cfg_IsEmptyRespond(commandRef, isEmpty);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Clear out the nodes's value.  If it doesn't exist it will be created, but have no value.
 *
 *  If the path is empty, the iterator's current node will be cleared.  If the node is a stem,
 *  all children will be removed from the tree.
 *
 *  Only valid during a write transaction.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetEmpty
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator to use as a basis for the transaction.
    const char* pathPtr                ///< [IN] Absolute or relative path to read from.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Checking to see if an iterator's <%p> current node is empty.", externalRef);
    LE_DEBUG_IF((pathPtr != NULL) && (strlen(pathPtr) != 0), "** Offset by \"%s\"", pathPtr);

    ni_IteratorRef_t iteratorRef = GetWriteIteratorFromRef(externalRef);

    if ((NULL != pathPtr) && (NULL != iteratorRef)
        && (false == CheckPathForSpecifier(pathPtr)))
    {
        ni_SetEmpty(iteratorRef, pathPtr);
    }

    le_cfg_SetEmptyRespond(commandRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check to see if a given node in the configuration tree exists.
 *
 *  \b Responds \b With:
 *
 *  True if the specified node exists in the tree.  False if not.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_NodeExists
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator to use as a basis for the transaction.
    const char* pathPtr                ///< [IN] Absolute or relative path to read from.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Checking to see if an iterator's <%p> current node exists.", externalRef);
    LE_DEBUG_IF((pathPtr != NULL) && (strlen(pathPtr) != 0), "** Offset by \"%s\"", pathPtr);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);
    bool exists = false;

    if ((NULL != pathPtr) && (NULL != iteratorRef)
        && (false == CheckPathForSpecifier(pathPtr)))
    {
        exists = ni_NodeExists(iteratorRef, pathPtr);
    }

    le_cfg_NodeExistsRespond(commandRef, exists);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a string value from the configuration tree.  If the value isn't a string, or if the node is
 *  empty or doesn't exist, the default value will be returned.
 *
 *  Valid for both read and write transactions.
 *
 *  If the path is empty, the iterator's current node will be read.
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *          - LE_OK       - Read was completed successfully.
 *          - LE_OVERFLOW - Supplied string buffer was not large enough to hold the value.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GetString
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator to use as a basis for the transaction.
    const char* pathPtr,               ///< [IN] Absolute or relative path to read from.
    size_t maxString,                  ///< [IN] Maximum size of the result string.
    const char* defaultValue           ///< [IN] Default value to use if the original can't  be.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Reading the string value of the iterator's <%p> current node.", externalRef);
    LE_DEBUG_IF((pathPtr != NULL) && (strlen(pathPtr) != 0), "** Offset by \"%s\"", pathPtr);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    le_result_t result = LE_OK;

    if ((NULL != pathPtr) && (NULL != iteratorRef)
        && (false == CheckPathForSpecifier(pathPtr)))
    {
        result = ni_GetNodeValueString(iteratorRef,
                                       pathPtr,
                                       strBuffer,
                                       MaxStr(maxString),
                                       defaultValue);
    }

    le_cfg_GetStringRespond(commandRef, result, strBuffer);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a string value to the configuration tree.  Only valid during a write
 *  transaction.
 *
 *  If the path is empty, the iterator's current node will be set.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetString
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator to use as a basis for the transaction.
    const char* pathPtr,               ///< [IN] Full or relative path to the value to write.
    const char* value                  ///< [IN] Value to write.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Writing the string value of the iterator's <%p> current node to \"%s\".",
             externalRef,
             value);
    LE_DEBUG_IF((pathPtr != NULL) && (strlen(pathPtr) != 0), "** Offset by \"%s\"", pathPtr);

    ni_IteratorRef_t iteratorRef = GetWriteIteratorFromRef(externalRef);

    if ((NULL != pathPtr) && (NULL != iteratorRef)
        && (false == CheckPathForSpecifier(pathPtr)))
    {
        ni_SetNodeValueString(iteratorRef, pathPtr, value);
    }

    le_cfg_SetStringRespond(commandRef);
}


// -------------------------------------------------------------------------------------------------
/**
 *  Read a binary data from the configuration tree.  If the the node has a wrong type, is
 *  empty or doesn't exist, the default value will be returned.
 *
 *  Valid for both read and write transactions.
 *
 *  If the path is empty, the iterator's current node will be read.
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *          - LE_OK             - Read was completed successfully.
 *          - LE_FORMAT_ERROR   - if data can't be decoded.
 *          - LE_OVERFLOW       - Supplied buffer was not large enough to hold the value.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GetBinary
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator to use as a basis for the transaction.
    const char* pathPtr,               ///< [IN] Absolute or relative path to read from.
    size_t maxBinary,                  ///< [IN] Maximum size of the result binary data.
    const uint8_t* defaultValue,       ///< [IN] Default value to use if the original can't be.
    size_t defaultValueSize            ///< [IN] Default value size.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Reading the binary value of the iterator's <%p> current node.", externalRef);
    LE_DEBUG_IF((pathPtr != NULL) && (strlen(pathPtr) != 0), "** Offset by \"%s\"", pathPtr);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);
    le_result_t result = LE_OK;
    uint8_t* binaryBuf = le_mem_ForceAlloc(tdb_GetBinaryDataMemoryPool());
    size_t binaryLen = MaxBinary(maxBinary);

    char* stringBuf = le_mem_ForceAlloc(tdb_GetEncodedStringMemoryPool());
    stringBuf[0] = 0;
    size_t stringBufSize = TDB_MAX_ENCODED_SIZE;

    // Encode the default valaue
    char* defaultStringBuf = le_mem_ForceAlloc(tdb_GetEncodedStringMemoryPool());
    defaultStringBuf[0] = 0;
    size_t defaultEncodedSize = TDB_MAX_ENCODED_SIZE;
    result = le_base64_Encode(defaultValue,
                              defaultValueSize,
                              defaultStringBuf,
                              &defaultEncodedSize);
    if (LE_OK != result)
    {
        LE_ERROR("ERROR encoding default value: %s", LE_RESULT_TXT(result));
        // Encode error - sending back the default value
        le_cfg_GetBinaryRespond(commandRef, LE_FORMAT_ERROR, defaultValue, defaultValueSize);
    }

    // Get the encoded string
    result = LE_OK;
    if ((NULL != pathPtr) && (NULL != iteratorRef)
        && (false == CheckPathForSpecifier(pathPtr)))
    {
        result = ni_GetNodeValueString(iteratorRef,
                                       pathPtr,
                                       stringBuf,
                                       stringBufSize,
                                       defaultStringBuf);
    }
    if (LE_OK != result)
    {
        // Node not found or has empty type: sending back the default value
        le_cfg_GetBinaryRespond(commandRef, result, defaultValue, defaultValueSize);
    }
    else
    {
        // Decode the string into binary data.
        result = le_base64_Decode(stringBuf,
                                  strlen(stringBuf),
                                  binaryBuf,
                                  &binaryLen);
        if (LE_OK != result)
        {
            LE_ERROR("ERROR decoding binary data: %s", LE_RESULT_TXT(result));
        }

        // Send back the decoded value
        le_cfg_GetBinaryRespond(commandRef, result, binaryBuf, binaryLen);
    }

    le_mem_Release(binaryBuf);
    le_mem_Release(stringBuf);
    le_mem_Release(defaultStringBuf);
}


// -------------------------------------------------------------------------------------------------
/**
 *  Write a binary data to the configuration tree.  Only valid during a write
 *  transaction.
 *
 *  If the path is empty, the iterator's current node will be set.
 *
 *  @note Binary data cannot be written to the 'system' tree.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetBinary
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator to use as a basis for the transaction.
    const char* pathPtr,               ///< [IN] Full or relative path to the value to write.
    const uint8_t* valuePtr,           ///< [IN] Binary data.
    const size_t size                  ///< [IN] Size of the binary data.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("Writing the binary data of the iterator's <%p> current node, size %" PRIuS,
             externalRef, size);

    LE_DEBUG_IF((pathPtr != NULL) && (strlen(pathPtr) != 0), "** Offset by \"%s\"", pathPtr);

    char* stringBuf = le_mem_ForceAlloc(tdb_GetEncodedStringMemoryPool());
    size_t encodedSize = TDB_MAX_ENCODED_SIZE;

    ni_IteratorRef_t iteratorRef = GetWriteIteratorFromRef(externalRef);

    if ((NULL != pathPtr) && (NULL != iteratorRef)
        && (false == CheckPathForSpecifier(pathPtr)))
    {
        // check whether this is not a system tree
        const char* treeNamePtr = tdb_GetTreeName(ni_GetTree(iteratorRef));
        if (0 == strcmp(treeNamePtr, "system"))
        {
            LE_ERROR("Binary data is not supported for the system tree");
            goto cleanup;
        }

        // encode the data and write it to the node
        le_result_t result = le_base64_Encode(valuePtr, size, stringBuf, &encodedSize);
        if (LE_OK != result)
        {
            LE_ERROR("ERROR encoding binary data: %s", LE_RESULT_TXT(result));
            goto cleanup;
        }
        ni_SetNodeValueString(iteratorRef, pathPtr, stringBuf);
    }

cleanup:
    le_cfg_SetBinaryRespond(commandRef);
    le_mem_Release(stringBuf);
}


// -------------------------------------------------------------------------------------------------
/**
 *  Read a signed integer value from the configuration tree.
 *
 *  If the underlying value is not an integer, the default value will be returned instead.  The
 *  default value is also returned if the node does not exist or if it's empty.
 *
 *  If the value is a floating point value, it will be rounded and returned as an integer.
 *
 *  Valid for both read and write transactions.
 *
 *  If the path is empty, the iterator's current node will be read.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GetInt
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator to use as a basis for the transaction.
    const char* pathPtr,               ///< [IN] Full or relative path to the value to read.
    int32_t defaultValue               ///< [IN] Default value to use if the original can't be.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Reading an integer value of the iterator's <%p> current node.", externalRef);
    LE_DEBUG_IF((pathPtr != NULL) && (strlen(pathPtr) != 0), "** Offset by \"%s\"", pathPtr);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);
    int value = 0;

    if ((NULL != pathPtr) && (NULL != iteratorRef)
        && (false == CheckPathForSpecifier(pathPtr)))
    {
        value = ni_GetNodeValueInt(iteratorRef, pathPtr, defaultValue);
    }

    le_cfg_GetIntRespond(commandRef, value);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a signed integer value to the configuration tree.  Only valid during a
 *  write transaction.
 *
 *  If the path is empty, the iterator's current node will be set.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetInt
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator to use as a basis for the transaction.
    const char* pathPtr,               ///< [IN] Full or relative path to the value to write.
    int32_t value                      ///< [IN] Value to write.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Setting an integer value of the iterator's <%p> current node to %" PRId32 ".",
             externalRef,
             value);
    LE_DEBUG_IF((pathPtr != NULL) && (strlen(pathPtr) != 0), "** Offset by \"%s\"", pathPtr);

    ni_IteratorRef_t iteratorRef = GetWriteIteratorFromRef(externalRef);

    if ((NULL != pathPtr) && (NULL != iteratorRef)
        && (false == CheckPathForSpecifier(pathPtr)))
    {
        ni_SetNodeValueInt(iteratorRef, pathPtr, value);
    }

    le_cfg_SetIntRespond(commandRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a 64-bit floating point value from the configuration tree.
 *
 *  If the value is an integer, the value will be promoted to a float.  Otherwise, if the
 *  underlying value is not a float or integer, the default value will be returned.
 *
 *  If the path is empty, the iterator's current node will be read.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GetFloat
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator to use as a basis for the transaction.
    const char* pathPtr,               ///< [IN] Full or relative path to the value to read.
    double defaultValue                ///< [IN] Default value to use if the original can't be.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Reading the float value of the iterator's <%p> current node.", externalRef);
    LE_DEBUG_IF((pathPtr != NULL) && (strlen(pathPtr) != 0), "** Offset by \"%s\"", pathPtr);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);
    double value = 0.0;

    if ((NULL != pathPtr) && (NULL != iteratorRef)
        && (false == CheckPathForSpecifier(pathPtr)))
    {
        value = ni_GetNodeValueFloat(iteratorRef, pathPtr, defaultValue);
    }

    le_cfg_GetFloatRespond(commandRef, value);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a 64-bit floating point value to the configuration tree.  Only valid
 *  during a write transaction.
 *
 *  If the path is empty, the iterator's current node will be set.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetFloat
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator to use as a basis for the transaction.
    const char* pathPtr,               ///< [IN] Full or relative path to the value to write.
    double value                       ///< [IN] Value to write.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Setting the float value of the iterator's <%p> current node to %f.",
             externalRef,
             value);
    LE_DEBUG_IF((pathPtr != NULL) && (strlen(pathPtr) != 0), "** Offset by \"%s\"", pathPtr);

    ni_IteratorRef_t iteratorRef = GetWriteIteratorFromRef(externalRef);

    if ((NULL != pathPtr) && (NULL != iteratorRef)
        && (false == CheckPathForSpecifier(pathPtr)))
    {
        ni_SetNodeValueFloat(iteratorRef, pathPtr, value);
    }

    le_cfg_SetFloatRespond(commandRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a value from the tree as a boolean.  If the node is empty or doesn't exist, the default
 *  value is returned.  The default value is also returned if the node is of a different type than
 *  expected.
 *
 *  Valid for both read and write transactions.
 *
 *  If the path is empty, the iterator's current node will be read.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GetBool
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator to use as a basis for the transaction.
    const char* pathPtr,               ///< [IN] Full or relative path to the value to read.
    bool defaultValue                  ///< [IN] Default value to use if the original can't be.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Reading the bool value of the iterator's <%p> current node.", externalRef);
    LE_DEBUG_IF((pathPtr != NULL) && (strlen(pathPtr) != 0), "** Offset by \"%s\"", pathPtr);

    ni_IteratorRef_t iteratorRef = GetIteratorFromRef(externalRef);
    bool value = defaultValue;

    if ((NULL != pathPtr) && (NULL != iteratorRef)
        && (false == CheckPathForSpecifier(pathPtr)))
    {
        value = ni_GetNodeValueBool(iteratorRef, pathPtr, defaultValue);
    }

    le_cfg_GetBoolRespond(commandRef, value);
}





// -------------------------------------------------------------------------------------------------
/**
 *  Write a boolean value to the configuration tree.  Only valid during a write
 *  transaction.
 *
 *  If the path is empty, the iterator's current node will be set.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetBool
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    le_cfg_IteratorRef_t externalRef,  ///< [IN] Iterator to use as a basis for the transaction.
    const char* pathPtr,               ///< [IN] Full or relative path to the value to write.
    bool value                         ///< [IN] Value to write.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Setting the bool value of the iterator's <%p> current node to %d.",
             externalRef,
             value);
    LE_DEBUG_IF((pathPtr != NULL) && (strlen(pathPtr) != 0), "** Offset by \"%s\"", pathPtr);

    ni_IteratorRef_t iteratorRef = GetWriteIteratorFromRef(externalRef);

    if ((NULL != pathPtr) && (NULL != iteratorRef)
        && (false == CheckPathForSpecifier(pathPtr)))
    {
        ni_SetNodeValueBool(iteratorRef, pathPtr, value);
    }

    le_cfg_SetBoolRespond(commandRef);
}






// -------------------------------------------------------------------------------------------------
//  Basic reading/writing, creation/deletion.
// -------------------------------------------------------------------------------------------------




// -------------------------------------------------------------------------------------------------
/**
 *  Delete the node specified by the path.  If the node doesn't exist, nothing happens.  All child
 *  nodes are also deleted.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickDeleteNode
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    const char* pathPtr                ///< [IN] Path to the node to delete.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Deleting node at \"%p\".", pathPtr);

    tu_UserRef_t userRef = tu_GetCurrentConfigUserInfo();
    tdb_TreeRef_t treeRef = QuickGetTree(userRef, TU_TREE_WRITE, pathPtr);

    if (treeRef != NULL)
    {
        rq_HandleQuickDeleteNode(le_cfg_GetClientSessionRef(),
                                 commandRef,
                                 userRef,
                                 treeRef,
                                 tp_GetPathOnly(pathPtr));
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Make a given node empty.  If the node doesn't currently exist, it's created as a new empty
 *  node.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetEmpty
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    const char* pathPtr                ///< [IN] Absolute or relative path to read from.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick clear node at \"%p\".", pathPtr);

    tu_UserRef_t userRef = tu_GetCurrentConfigUserInfo();
    tdb_TreeRef_t treeRef = QuickGetTree(userRef, TU_TREE_WRITE, pathPtr);

    if (treeRef != NULL)
    {
        rq_HandleQuickSetEmpty(le_cfg_GetClientSessionRef(),
                               commandRef,
                               userRef,
                               treeRef,
                               tp_GetPathOnly(pathPtr));
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a string value from the configuration tree.  If the value isn't a string, or if the node is
 *  empty or doesn't exist, the default value will be returned.
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *          - LE_OK            - Commit was completed successfully.
 *          - LE_OVERFLOW      - Supplied string buffer was not large enough to hold the value.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickGetString
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    const char* pathPtr,               ///< [IN] Path to read from.
    size_t maxString,                  ///< [IN] Maximum string to return.
    const char* defaultValuePtr        ///< [IN] Default value to use if the original can't be.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick get node string value at \"%p\".", pathPtr);

    tu_UserRef_t userRef = tu_GetCurrentConfigUserInfo();
    tdb_TreeRef_t treeRef = QuickGetTree(userRef, TU_TREE_READ, pathPtr);

    if (treeRef != NULL)
    {
        rq_HandleQuickGetString(le_cfg_GetClientSessionRef(),
                                commandRef,
                                userRef,
                                treeRef,
                                tp_GetPathOnly(pathPtr),
                                MaxStr(maxString),
                                defaultValuePtr);
    }
}



// -------------------------------------------------------------------------------------------------
/**
 *  Write a string value to the configuration tree.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetString
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    const char* pathPtr,               ///< [IN] Path to the value to write.
    const char* valuePtr               ///< [IN] Value to write.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick get node string value at \"%p\".", pathPtr);

    tu_UserRef_t userRef = tu_GetCurrentConfigUserInfo();
    tdb_TreeRef_t treeRef = QuickGetTree(userRef, TU_TREE_WRITE, pathPtr);

    if (treeRef != NULL)
    {
        rq_HandleQuickSetData(le_cfg_GetClientSessionRef(),
                              commandRef,
                              userRef,
                              treeRef,
                              tp_GetPathOnly(pathPtr),
                              valuePtr,
                              RQ_SET_STRING);
    }
}


// -------------------------------------------------------------------------------------------------
/**
 * Writes a binary data to the config tree.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickGetBinary
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    const char* pathPtr,               ///< [IN] Absolute or relative path to read from.
    size_t maxBinary,                  ///< [IN] Maximum size of the result binary data.
    const uint8_t* defaultValuePtr,    ///< [IN] Default value to use if the original can't be.
    size_t defaultValueSize            ///< [IN] Default value size.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick get node binary value at \"%p\".", pathPtr);

    tu_UserRef_t userRef = tu_GetCurrentConfigUserInfo();
    tdb_TreeRef_t treeRef = QuickGetTree(userRef, TU_TREE_READ, pathPtr);

    if (treeRef != NULL)
    {
        rq_HandleQuickGetBinary(le_cfg_GetClientSessionRef(),
                                commandRef,
                                userRef,
                                treeRef,
                                tp_GetPathOnly(pathPtr),
                                MaxBinary(maxBinary),
                                defaultValuePtr,
                                defaultValueSize);
    }
}


// -------------------------------------------------------------------------------------------------
/**
 *  Write a binary value to the configuration tree.
 *
 *  @note Binary data cannot be written to the 'system' tree.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetBinary
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    const char* pathPtr,               ///< [IN] Full or relative path to the value to write.
    const uint8_t* valuePtr,           ///< [IN] Binary data.
    const size_t size                  ///< [IN] Size of the binary data.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick set node binary value at \"%p\".", pathPtr);

    tu_UserRef_t userRef = tu_GetCurrentConfigUserInfo();
    tdb_TreeRef_t treeRef = QuickGetTree(userRef, TU_TREE_WRITE, pathPtr);

    if (treeRef != NULL)
    {
        // check whether this is not a system tree
        const char* treeNamePtr = tdb_GetTreeName(treeRef);
        if (0 == strcmp(treeNamePtr, "system"))
        {
            LE_ERROR("Binary data is not supported for the system tree");
            le_cfg_QuickSetBinaryRespond(commandRef);
            return;
        }

        // encode the data and write it to the node
        char* stringBuf = le_mem_ForceAlloc(tdb_GetEncodedStringMemoryPool());
        stringBuf[0] = 0;
        size_t encodedSize = TDB_MAX_ENCODED_SIZE;

        le_result_t result = le_base64_Encode(valuePtr, size, stringBuf, &encodedSize);
        if (LE_OK != result)
        {
            LE_ERROR("ERROR encoding binary data: %s", LE_RESULT_TXT(result));
        }
        else
        {
            rq_HandleQuickSetData(le_cfg_GetClientSessionRef(),
                                  commandRef,
                                  userRef,
                                  treeRef,
                                  tp_GetPathOnly(pathPtr),
                                  stringBuf,
                                  RQ_SET_BINARY);
        }
        le_mem_Release(stringBuf);
    }
}


// -------------------------------------------------------------------------------------------------
/**
 *  Read a signed integer value from the configuration tree.  If the value is a float, it's
 *  truncated.  Otherwise If the underlying value is not an integer or a float, the default value
 *  will be returned instead.
 *
 *  If the value is empty or the node doesn't exist, the default value is returned instead.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickGetInt
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    const char* pathPtr,               ///< [IN] Path to the value to write.
    int32_t defaultValue               ///< [IN] Default value to use if the original can't be.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick get node int value at \"%p\".", pathPtr);

    tu_UserRef_t userRef = tu_GetCurrentConfigUserInfo();
    tdb_TreeRef_t treeRef = QuickGetTree(userRef, TU_TREE_READ, pathPtr);

    if (treeRef != NULL)
    {
        rq_HandleQuickGetInt(le_cfg_GetClientSessionRef(),
                             commandRef,
                             userRef,
                             treeRef,
                             tp_GetPathOnly(pathPtr),
                             defaultValue);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a signed integer value to the configuration tree.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetInt
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    const char* pathPtr,               ///< [IN] Path to the value to write.
    int32_t value                      ///< [IN] Value to write.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick set node int value at \"%p\".", pathPtr);

    tu_UserRef_t userRef = tu_GetCurrentConfigUserInfo();
    tdb_TreeRef_t treeRef = QuickGetTree(userRef, TU_TREE_WRITE, pathPtr);

    if (treeRef != NULL)
    {
        rq_HandleQuickSetInt(le_cfg_GetClientSessionRef(),
                             commandRef,
                             userRef,
                             treeRef,
                             tp_GetPathOnly(pathPtr),
                             value);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a 64-bit floating point value from the configuration tree.  If the value is an integer,
 *  it's promoted to a float.  Otherwise, if the underlying value is not a float, or an
 *  integer the default value will be returned.
 *
 *  If the value is empty or the node doesn't exist, the default value is returned.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickGetFloat
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    const char* pathPtr,               ///< [IN] Path to the value to write.
    double defaultValue                ///< [IN] Default value to use if the original can't be.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick get node float value at \"%p\".", pathPtr);

    tu_UserRef_t userRef = tu_GetCurrentConfigUserInfo();
    tdb_TreeRef_t treeRef = QuickGetTree(userRef, TU_TREE_READ, pathPtr);

    if (treeRef != NULL)
    {
        rq_HandleQuickGetFloat(le_cfg_GetClientSessionRef(),
                               commandRef,
                               userRef,
                               treeRef,
                               tp_GetPathOnly(pathPtr),
                               defaultValue);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a 64-bit floating point value to the configuration tree.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetFloat
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    const char* pathPtr,               ///< [IN] Path to the value to write.
    double value                       ///< [IN] Value to write.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick set node float value at \"%p\".", pathPtr);

    tu_UserRef_t userRef = tu_GetCurrentConfigUserInfo();
    tdb_TreeRef_t treeRef = QuickGetTree(userRef, TU_TREE_WRITE, pathPtr);

    if (treeRef != NULL)
    {
        rq_HandleQuickSetFloat(le_cfg_GetClientSessionRef(),
                               commandRef,
                               userRef,
                               treeRef,
                               tp_GetPathOnly(pathPtr),
                               value);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a value from the tree as a boolean.  If the node is empty or doesn't exist, the default
 *  value is returned.  This is also true if the node is of a different type than expected.
 *
 *  If the value is empty or the node doesn't exist, the default value is returned instead.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickGetBool
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    const char* pathPtr,               ///< [IN] Path to the value to write.
    bool defaultValue                  ///< [IN] Default value to use if the original can't be.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick get node bool value at \"%p\".", pathPtr);

    tu_UserRef_t userRef = tu_GetCurrentConfigUserInfo();
    tdb_TreeRef_t treeRef = QuickGetTree(userRef, TU_TREE_READ, pathPtr);

    if (treeRef != NULL)
    {
        rq_HandleQuickGetBool(le_cfg_GetClientSessionRef(),
                              commandRef,
                              userRef,
                              treeRef,
                              tp_GetPathOnly(pathPtr),
                              defaultValue);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a boolean value to the configuration tree.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetBool
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Reference used to generate a reply for this
                                       ///<      request.
    const char* pathPtr,               ///< [IN] Path to the value to write.
    bool value                         ///< [IN] Value to write.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick set node bool value at \"%p\".", pathPtr);

    tu_UserRef_t userRef = tu_GetCurrentConfigUserInfo();
    tdb_TreeRef_t treeRef = QuickGetTree(userRef, TU_TREE_WRITE, pathPtr);

    if (treeRef != NULL)
    {
        rq_HandleQuickSetBool(le_cfg_GetClientSessionRef(),
                              commandRef,
                              userRef,
                              treeRef,
                              tp_GetPathOnly(pathPtr),
                              value);
    }
}
