
// -------------------------------------------------------------------------------------------------
/*
 *  Highlevel impoementation of the configuration Tree API.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "user.h"
#include "interfaces.h"
#include "internalCfgTypes.h"
#include "treeDb.h"
#include "userHandling.h"
#include "iterator.h"
#include "stringBuffer.h"





// -------------------------------------------------------------------------------------------------
/**
 *  Get an iterator pointer from an iterator reference.
 */
// -------------------------------------------------------------------------------------------------
static IteratorInfo_t* GetIteratorFromRef
(
    le_cfg_iteratorRef_t iteratorRef  ///< The iterator reference to extract a pointer from.
)
// -------------------------------------------------------------------------------------------------
{
    // Get the user info for the process that sent the request.
    UserInfo_t* userPtr = GetCurrentUserInfo();

    if (userPtr == NULL)
    {
        LE_DEBUG("Could not read user info.");
        return NULL;
    }

    // Now, get the iterator subsystem to give us back an iterator pointer.
    IteratorInfo_t* iteratorPtr = itr_GetPtr(userPtr, iteratorRef);

    if (iteratorPtr == NULL)
    {
        LE_DEBUG("Bad iterator reference.");
    }

    return iteratorPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Handle both the create read and write transacion requests.
 */
// -------------------------------------------------------------------------------------------------
static void CreateTransaction
(
    le_cfg_Context_t contextRef,  ///< Message context for the request.
    const char* pathPtr,          ///< Initial path for the transaction.
    IteratorType_t request        ///< The requested iterator type to create.
)
// -------------------------------------------------------------------------------------------------
{
    // Check to see if this user has access to the tree/path in question.
    UserInfo_t* userPtr = GetCurrentUserInfo();
    TreeInfo_t* treePtr = GetRequestedTree(userPtr, pathPtr);

    if (treePtr == NULL)
    {
        le_cfg_CreateWriteTxnRespond(contextRef, NULL);
    }
    else
    {
        // Try to create the new iterator.  If it can not be created now, it'll be queued for
        // creation later.
        HandleCreateTxnRequest(userPtr,
                               treePtr,
                               le_cfg_GetClientSessionRef(),
                               contextRef,
                               request,
                               GetPathOnly(pathPtr));
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get a node from an iterator object.
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t GetNodeRef
(
    le_cfg_iteratorRef_t iteratorRef,   ///< The iterator to read.
    IteratorGetNodeFlag_t getNodeFlag,  ///<
    const char* pathPtr                 ///< The path to the node in the iterator's tree.
)
// -------------------------------------------------------------------------------------------------
{
    IteratorInfo_t* iteratorPtr = GetIteratorFromRef(iteratorRef);

    if (   (iteratorPtr == NULL)
        || (itr_IsClosed(iteratorPtr)))
    {
        return NULL;
    }

    return itr_GetNode(iteratorPtr, ITER_GET_NO_DEFAULT_ROOT, pathPtr);
}




// -------------------------------------------------------------------------------------------------
//  Key/value iteration.
// -------------------------------------------------------------------------------------------------




// -------------------------------------------------------------------------------------------------
/**
 *  Create a read transaction and open a new iterator for traversing the configuration tree.
 *
 *  @Note: This action will create a read transaction that will exist for the lifetime of all
 *         active iterators.  If the application holds the iterator for past the configured read
 *         transaction timeout, active iterators will become invalid and no longer return data.
 *
 *  @Note: A tree transaction is global, so a long held read transaction will block other users
 *         write transactions from being comitted.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_CreateReadTxn
(
    le_cfg_Context_t contextRef,  ///< This handle is used to generate the reply for this message.
    const char* basePathPtr       ///< Path to the location to create the new iterator.  This path
                                  ///<  is based off of the user's root node.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Creating a new read transaction on path <%s>.", basePathPtr);
    CreateTransaction(contextRef, basePathPtr, ITY_READ);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Create a write transaction and open a new iterator for both reading and writing.
 &
 *  @Note: This action will create a write transaction.  If the application holds the iterator for
 *         past the configured write transaction timeout, the iterator will cancel the transaction.
 *         All further reads will fail to return data and all writes will be thrown away.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_CreateWriteTxn
(
    le_cfg_Context_t contextRef,  ///< This handle is used to generate the reply for this message.
    const char* basePathPtr       ///< Path to the location to create the new iterator.  This path
                                  ///<   is based off of the user's root node.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Creating a new write transaction on path <%s>.", basePathPtr);
    CreateTransaction(contextRef, basePathPtr, ITY_WRITE);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Close the write iterator and commit the write transaction.  This will update the config tree
 *  with all of the writes that have occured using the iterator.
 *
 *  If the transaction had timedout, or if the iterator has been moved out of bounds the commit will
 *  fail.
 *
 *  @Note: This operation will also delete the iterator object, therefore you will not have to call
 *         DeleteIterator on it.
 *
 *  @Note: All clones of this iterator will also have to commit their write transactions before the
 *         transaction is actually comitted to the tree.
 *
 *  @return This function will return one of the following:
 *
 *          - LE_OK            - The commit was completed successfuly.
 *          - LE_BAD_PARAMETER - Attempted to use an invalid iterator for this request.
 *          - LE_NOT_PERMITTED - Attempted to use an iterator that has been moved out of bounds.
 *          - LE_TIMEOUT       - The transaction had timed out.
 *          - LE_CLOSED        - The transaction has been canceled by one of the clones of this
 *                               iterator.  Nothing has been committed.
 *          - LE_WOULD_BLOCK   - The data has been committed to the parent transaction but other
 *                               clones of this transaction are still outstanding.  This data will
 *                               only be comitted to the live tree when all clones have been
 *                               comitted successfuly.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_CommitWrite
(
    le_cfg_Context_t contextRef,   ///< This handle is used to generate the reply for this message.
    le_cfg_iteratorRef_t iteratorRef  ///< The iterator object to commit.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Comitting a write transaction.  Iterator <%p>", iteratorRef);

    // Actually get the iterator pointer, and make sure it's writeable.  Note that at this point we
    // assume we'll get a non-NULL ptr back, as we checked the ref previously.
    IteratorInfo_t* iteratorPtr = GetIteratorFromRef(iteratorRef);


    if (   (iteratorPtr == NULL)
        || (itr_IsWriteIterator(iteratorPtr) == false))
    {
        LE_ERROR("The reference was bad, or the iterator was read only.");
        le_cfg_CommitWriteRespond(contextRef, LE_BAD_PARAMETER);
    }
    else
    {
        // Commit the transaction or schedule it for later.
        HandleCommitTxnRequest(GetCurrentUserInfo(),
                               le_cfg_GetClientSessionRef(),
                               contextRef,
                               iteratorRef);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Close and free the given iterator object.  If the iterator is a write iterator, the transaction
 *  will be canceled.  If the iterator is a read iterator the transaction will simply be closed.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_DeleteIterator
(
    le_cfg_Context_t contextRef,   ///< This handle is used to generate the reply for this message.
    le_cfg_iteratorRef_t iteratorRef  ///< The iterator object to close.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Delete iterator <%p>", iteratorRef);

    IteratorInfo_t* iteratorPtr = GetIteratorFromRef(iteratorRef);


    if (iteratorPtr == NULL)
    {
        le_cfg_DeleteIteratorRespond(contextRef);
    }
    else
    {
        // Process this request, and process any outstanding requests as well.
        HandleDeleteTxnRequest(GetCurrentUserInfo(), contextRef, iteratorRef);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Change the stem that the iterator is currently pointing at.  The path passed in can be an
 *  absolute or a relative path from the iterators current location.
 *
 *  Calling MoveIterator with a path of "." will simply jump the iterator back to the first sub-item
 *  of the current stem.
 *
 *  @return This function will return one of the following values:
 *
 *          - LE_OK            - The commit was completed successfuly.
 *          - LE_BAD_PARAMETER - Attempted to use an invalid iterator for this request.
 *          - LE_NOT_PERMITTED - Attempted to move the iterator outside of the allowed area.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GoToNode
(
    le_cfg_Context_t contextRef,       ///< This handle is used to generate the reply for this message.
    le_cfg_iteratorRef_t iteratorRef,  ///< The iterator to move.
    const char* newPathPtr             ///< Absolute or relative path from the current location to
                                       ///<   jump to.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Switching iterator, <%p>, to node, <%s>.", iteratorRef, newPathPtr);

    IteratorInfo_t* iteratorPtr = GetIteratorFromRef(iteratorRef);

    if (iteratorPtr == NULL)
    {
        le_cfg_GoToNodeRespond(contextRef, LE_BAD_PARAMETER);
        return;
    }

    if (PathHasTreeSpecifier(newPathPtr))
    {
        le_cfg_GoToNodeRespond(contextRef, LE_NOT_PERMITTED);
        return;
    }

    le_cfg_GoToNodeRespond(contextRef, itr_GoToNode(iteratorPtr, newPathPtr));
}




// -------------------------------------------------------------------------------------------------
/**
 *  Go to the parent of the node the iterator is currently pointed at.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GoToParent
(
    le_cfg_Context_t contextRef,   ///< This handle is used to generate the reply for this message.
    le_cfg_iteratorRef_t iteratorRef  ///< The iterator to move.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Switching iterator, <%p>, to parent node.", iteratorRef);

    IteratorInfo_t* iteratorPtr = GetIteratorFromRef(iteratorRef);

    if (iteratorPtr == NULL)
    {
        le_cfg_GoToParentRespond(contextRef, LE_BAD_PARAMETER);
    }
    else
    {
        le_cfg_GoToParentRespond(contextRef, itr_GoToParent(iteratorPtr));
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Go to the first child of the node that the iterator is currently pointed at.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GoToFirstChild
(
    le_cfg_Context_t contextRef,   ///< This handle is used to generate the reply for this message.
    le_cfg_iteratorRef_t iteratorRef  ///< The iterator to move.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Switching iterator, <%p>, to first child.", iteratorRef);

    IteratorInfo_t* iteratorPtr = GetIteratorFromRef(iteratorRef);

    if (iteratorPtr == NULL)
    {
        le_cfg_GoToFirstChildRespond(contextRef, LE_BAD_PARAMETER);
    }
    else
    {
        le_cfg_GoToFirstChildRespond(contextRef, itr_GoToFirstChild(iteratorPtr));
    }
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
 *  @return This function will return one of the following values:
 *
 *          - LE_OK            - The commit was completed successfuly.
 *          - LE_BAD_PARAMETER - Attempted to use an invalid iterator for this request.
 *          - LE_NOT_FOUND     - The iterator has reached the end of the current list of sub nodes.
 *                               This is also returned if the the current node has no sub items.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GoToNextSibling
(
    le_cfg_Context_t contextRef,   ///< This handle is used to generate the reply for this message.
    le_cfg_iteratorRef_t iteratorRef  ///< Get the next sub-item of the current node.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Switching iterator, <%p>, to next sibling.", iteratorRef);

    IteratorInfo_t* iteratorPtr = GetIteratorFromRef(iteratorRef);

    if (iteratorPtr == NULL)
    {
        le_cfg_GoToNextSiblingRespond(contextRef, LE_BAD_PARAMETER);
    }
    else
    {
        le_cfg_GoToNextSiblingRespond(contextRef,
                                   itr_GoToNextSibling(iteratorPtr) ? LE_OK : LE_OUT_OF_RANGE);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check the iterator to see the interator represents a write transaction.
 *
 *  @return - true - This is a write transaction object.
 *          - false - This is a read only transaction object.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_IsWriteable
(
    le_cfg_Context_t contextRef,      ///< This handle is used to generate the reply for this
                                      ///<   message.
    le_cfg_iteratorRef_t iteratorRef  ///< The iterator to query.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Checking iterator, <%p>, to see if it's writable.", iteratorRef);

    IteratorInfo_t* iteratorPtr = GetIteratorFromRef(iteratorRef);

    if (iteratorPtr == NULL)
    {
        le_cfg_IsWriteableRespond(contextRef, false);
    }
    else
    {
        le_cfg_IsWriteableRespond(contextRef, itr_IsWriteIterator(iteratorPtr));
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check to see if the iterator reference points to a valid iterator object.
 *
 *  @note An iterator is considered no longer valid if one if its clones cancels the underlying
 *        transaction.
 *
 *  @return - true  - The object is valid and can be used.
 *          - false - The object is no longer valid either through a security violation or having
 *                    been cancelled.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_IsValid
(
    le_cfg_Context_t contextRef,      ///< This handle is used to generate the reply for this
                                      ///<   message.
    le_cfg_iteratorRef_t iteratorRef  ///< The iterator to query.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Checking iterator, <%p> for validity.", iteratorRef);

    bool isValid = GetIteratorFromRef(iteratorRef) != NULL;

    LE_DEBUG("isValid == %s.", isValid?"TRUE":"FALSE");

    le_cfg_IsValidRespond(contextRef, isValid);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get path to the node that the iterator is currently pointed at.
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
 *  @return - LE_OK            - The write was completed successfuly.
 *          - LE_OVERFLOW      - The supplied string buffer was not large enough to hold the value.
 *          - LE_BAD_PARAMETER - The supplied iterator reference was invalid.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GetPath
(
    le_cfg_Context_t contextRef,       ///< This handle is used to generate the reply for this message.
    le_cfg_iteratorRef_t iteratorRef,  ///< The iterator to move.
    size_t pathSize                    ///< Size of the buffer to hold the response.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Getting path for current iterator <%p> node.", iteratorRef);

    IteratorInfo_t* iteratorPtr = GetIteratorFromRef(iteratorRef);

    if (iteratorPtr == NULL)
    {
        le_cfg_GetPathRespond(contextRef, LE_BAD_PARAMETER, "");
    }
    else
    {
        char* strBufferPtr = itr_GetPath(iteratorPtr);
        le_result_t result = LE_OK;

        if ((strnlen(strBufferPtr, SB_SIZE) + 1) > pathSize)
        {
            result = LE_OVERFLOW;
            strBufferPtr[pathSize - 1] = 0;
        }

        le_cfg_GetPathRespond(contextRef, result, strBufferPtr);
        sb_Release(strBufferPtr);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the path to the parent of the node that the iterator is currently pointed at.
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
 *  If the iterator was currently pointing at valueB, then GetParentPath would return the following
 *  path:
 *
 *  @code
 *  /baseNode/childA/
 *  @endcode
 *
 *  @return - LE_OK            - The write was completed successfuly.
 *          - LE_OVERFLOW      - The supplied string buffer was not large enough to hold the value.
 *          - LE_BAD_PARAMETER - The supplied iterator reference was invalid.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GetParentPath
(
    le_cfg_Context_t contextRef,       ///< This handle is used to generate the reply for this
                                       ///<   message.
    le_cfg_iteratorRef_t iteratorRef,  ///< The iterator object to read.
    size_t pathSize                    ///< Size of the buffer to hold the response.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Getting path for an iterator <%p> node's parent.", iteratorRef);

    IteratorInfo_t* iteratorPtr = GetIteratorFromRef(iteratorRef);

    if (iteratorPtr == NULL)
    {
        le_cfg_GetParentPathRespond(contextRef, LE_BAD_PARAMETER, "");
    }
    else
    {
        char* strBufferPtr = itr_GetParentPath(iteratorPtr);
        le_result_t result = LE_OK;

        if ((strnlen(strBufferPtr, SB_SIZE) + 1) > pathSize)
        {
            result = LE_OVERFLOW;
            strBufferPtr[pathSize - 1] = 0;
        }

        le_cfg_GetParentPathRespond(contextRef, result, strBufferPtr);
        sb_Release(strBufferPtr);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the type of node that the iterator is currently pointing at.
 *
 *  @return le_cfg_nodeType_t value indicating the stored value.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GetNodeType
(
    le_cfg_Context_t contextRef,      ///< This handle is used to generate the reply for this
                                      ///<   message.
    le_cfg_iteratorRef_t iteratorRef  ///< The iterator object to use to read from the tree.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Getting the type for an iterator's <%p> current node.", iteratorRef);

    IteratorInfo_t* iteratorPtr = GetIteratorFromRef(iteratorRef);

    if (iteratorPtr == NULL)
    {
        le_cfg_GetNodeTypeRespond(contextRef, LE_CFG_TYPE_DENIED);
    }
    else
    {
        le_cfg_GetNodeTypeRespond(contextRef, itr_GetNodeType(iteratorPtr));
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the name of the node that the iterator is currently pointing at.
 *
 *  @return - LE_OK           - The write was completed successfuly.
 *          - LE_OVERFLOW      - The supplied string buffer was not large enough to hold the value.
 *          - LE_BAD_PARAMETER - The supplied iterator reference was invalid.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GetNodeName
(
    le_cfg_Context_t contextRef,       ///< This handle is used to generate the reply for this
                                       ///<   message.
    le_cfg_iteratorRef_t iteratorRef,  ///< The iterator object to use to read from the tree.
    size_t maxString                   ///< Maximum length of the dest buffer.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Getting the name for an iterator's <%p> current node.", iteratorRef);

    IteratorInfo_t* iteratorPtr = GetIteratorFromRef(iteratorRef);

    if (iteratorPtr == NULL)
    {
        le_cfg_GetNodeNameRespond(contextRef, LE_BAD_PARAMETER, "");
    }
    else
    {
        char* strBufferPtr = itr_GetNodeName(iteratorPtr);
        le_result_t result = LE_OK;

        if ((strnlen(strBufferPtr, SB_SIZE) + 1) > maxString)
        {
            result = LE_OVERFLOW;
            strBufferPtr[maxString - 1] = 0;
        }

        le_cfg_GetNodeNameRespond(contextRef, result, strBufferPtr);
        sb_Release(strBufferPtr);
    }
}




// -------------------------------------------------------------------------------------------------
//  Update handling.
// -------------------------------------------------------------------------------------------------




// -------------------------------------------------------------------------------------------------
/**
 *  Register a call back on a given stem object.  Once registered, if that stem or if any of it's
 *  children are read from, written to, created or deleted, then this function will be called.
 *
 *  @return A handle to the event registration.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_ChangeHandlerRef_t le_cfg_AddChangeHandler
(
    const char* newPathPtr,                 ///< The path in the tree to monitor.
    le_cfg_ChangeHandlerFunc_t handlerPtr,  ///< The handler to add to the tree.
    void* contextPtr                        ///< Context for the handler to receive on callback.
)
// -------------------------------------------------------------------------------------------------
{
    /*UserInfo_t* userPtr = GetCurrentUserInfo();
    TreeInfo_t* treePtr = GetRequestedTree(userPtr, newPathPtr);

    if (   (userPtr == NULL)
        || (treePtr == NULL))
    {
        le_cfg_AddChangeHandlerRespond(contextRef, contextPtr, NULL);
    }
    else
    {
        le_cfg_AddChangeHandlerRespond(contextRef,
                                    contextPtr,
                                    HandleAddChangeHandler(userPtr,
                                                           treePtr,
                                                           le_cfg_GetClientSessionRef(),
                                                           GetPathOnly(newPathPtr),
                                                           handlerPtr,
                                                           contextPtr));
    }*/

    return NULL;
}




// -------------------------------------------------------------------------------------------------
/*
 *  Remove the change handler from the given node.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_RemoveChangeHandler
(
    le_cfg_ChangeHandlerRef_t handlerRef  ///< The handler to mremove.
)
// -------------------------------------------------------------------------------------------------
{
    /*HandleRemoveChangeHandler(handlerRef);
    le_cfg_RemoveChangeHandlerRespond(contextRef);*/
}




// -------------------------------------------------------------------------------------------------
//  Import and export of the tree data.
// -------------------------------------------------------------------------------------------------




// -------------------------------------------------------------------------------------------------
/**
 *  Read a subset of the configuration tree from the given filePath.  That tree then overwrites the
 *  node at the given nodePath.
 *
 *  This function will import a sub-tree as part of the iterator's current transaction.  This allows
 *  you to create an iterator on a given node.  Import a sub-tree, and then examine the contents of
 *  the import before deciding to commit the new data.
 *
 *  @return This function will return one of the following values:
 *
 *          - LE_OK            - The commit was completed successfuly.
 *          - LE_NOT_PERMITTED - Attempted to import to a section of the tree the connection doesn't
 *                               have access to.
 *          - LE_FAULT         - An I/O error occured while reading the data.
 *          - LE_FORMAT_ERROR  - The configuration data being imported appears corrupted.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_ImportTree
(
    le_cfgAdmin_Context_t contextRef,  ///< This handle is used to generate the reply for this
                                       ///<   message.
    le_cfg_iteratorRef_t iteratorRef,  ///< The write iterator that is being used for the import.
    const char* filePathPtr,           ///< Import the tree data from the this file.
    const char* nodePathPtr            ///< Where in the tree should this import happen?  Leave as
                                       ///<   an empty string to use the iterator's current node.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Importing a tree from <%s> onto node <%s>, using iterator, <%p>.",
             filePathPtr, nodePathPtr, iteratorRef);

    tdb_NodeRef_t nodeRef = GetNodeRef(iteratorRef, ITER_GET_NO_DEFAULT_ROOT, nodePathPtr);
    le_result_t result;

    if (nodeRef != NULL)
    {
        int fileRef = le_flock_Open(filePathPtr, LE_FLOCK_READ);

        if (fileRef > 0)
        {
            result = tdb_ReadTreeNode(nodeRef, fileRef) == true ? LE_OK : LE_FORMAT_ERROR;
            le_flock_Close(fileRef);
        }
    }
    else
    {
        result = LE_NOT_PERMITTED;
    }

    le_cfgAdmin_ImportTreeRespond(contextRef, result);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Take a node given from nodePath and stream it and it's children to the file given by filePath.
 *
 *  This funciton uses the iterator's read transaction, and takes a snapshot of the current state of
 *  the tree.  The data write happens immediately.
 *
 *  @return This function will return one of the following values:
 *
 *          - LE_OK            - The commit was completed successfuly.
 *          - LE_BAD_PARAMETER - The specified path does not exist in the config tree.
 *          - LE_NOT_PERMITTED - Attempted to export from a section of the tree the connection
 *                               doesn't have access to.
 *          - LE_FAULT         - An I/O error occured while writing the data.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_ExportTree
(
    le_cfgAdmin_Context_t contextRef,  ///< This handle is used to generate the reply for this
                                       ///<   message.
    le_cfg_iteratorRef_t iteratorRef,  ///< The write iterator that is being used for the export.
    const char* filePathPtr,           ///< Import the tree data from the this file.
    const char* nodePathPtr            ///< Where in the tree should this export happen?  Leave as
                                       ///<   an empty string to use the iterator's current node.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Exporting a tree from node <%s> into file <%s>, using iterator, <%p>.",
             nodePathPtr, filePathPtr, iteratorRef);

    tdb_NodeRef_t nodeRef = GetNodeRef(iteratorRef, ITER_GET_NO_DEFAULT_ROOT, nodePathPtr);
    le_result_t result;

    if (nodeRef != NULL)
    {
        umask(0);

        int fileRef = le_flock_Create(filePathPtr,
                                      LE_FLOCK_WRITE,
                                      LE_FLOCK_REPLACE_IF_EXIST,
                                      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

        if (fileRef > 0)
        {
            tdb_WriteTreeNode(nodeRef, fileRef);
            le_flock_Close(fileRef);
            result = LE_OK;
        }
    }
    else
    {
        result = LE_NOT_PERMITTED;
    }

    le_cfgAdmin_ExportTreeRespond(contextRef, result);
}




// -------------------------------------------------------------------------------------------------
//  Basic reading/writing, creation/deletion.
// -------------------------------------------------------------------------------------------------




// -------------------------------------------------------------------------------------------------
/**
 *  Delete the leaf or stem specified by the path.  If the node doesn't exist, nothing happens.  All
 *  child nodes are also deleted.
 *
 *  @return - LE_OK            - The commit was completed successfuly.
 *          - LE_BAD_PARAMETER - Attempted to use an invalid path for this request.
 *          - LE_NOT_PERMITTED - Attempted to use a path that is out of bounds.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickDeleteNode
(
    le_cfg_Context_t contextRef,  ///< This handle is used to generate the reply for this message.
    const char* pathPtr           ///< The path to the node to delete.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick delete, <%s>.", pathPtr);

    UserInfo_t* userPtr = GetCurrentUserInfo();
    HandleQuickDeleteNode(userPtr,
                          GetRequestedTree(userPtr, pathPtr),
                          le_cfg_GetClientSessionRef(),
                          contextRef,
                          GetPathOnly(pathPtr));
}




// -------------------------------------------------------------------------------------------------
/**
 *  Clear out the leaf's value.  If it doesn't exist it will be created, but have no value.
 *
 *  @return - LE_OK            - The commit was completed successfuly.
 *          - LE_BAD_PARAMETER - Attempted to use an invalid path for this request.
 *          - LE_NOT_PERMITTED - Attempted to use a path that is out of bounds.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetEmpty
(
    le_cfg_Context_t contextRef,  ///< This handle is used to generate the reply for this message.
    const char* pathPtr           ///< The item in the tree to clear.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick set empty, <%s>.", pathPtr);

    UserInfo_t* userPtr = GetCurrentUserInfo();
    HandleQuickSetEmpty(userPtr,
                        GetRequestedTree(userPtr, pathPtr),
                        le_cfg_GetClientSessionRef(),
                        contextRef,
                        GetPathOnly(pathPtr));
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a string value from the configuration tree.  If the stored value is not a string the value
 *  will be converted into a string.
 *
 *  If the value is a number, then a string with that number is returned.  If the value is empty, or
 *  the iterator is invalid, an empty string is returned.  If the value is boolean, then the string,
 *  "true" or "false" is returned.
 *
 *  @return - LE_OK            - The commit was completed successfuly.
 *          - LE_BAD_PARAMETER - Attempted to use an invalid path for this request.
 *          - LE_NOT_PERMITTED - Attempted to use a path that is out of bounds.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickGetString
(
    le_cfg_Context_t contextRef,  ///< This handle is used to generate the reply for this message.
    const char* pathPtr,          ///< Path in the tree to read from.
    size_t maxString
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick get string, <%s>.", pathPtr);

    UserInfo_t* userPtr = GetCurrentUserInfo();
    HandleQuickGetString(userPtr,
                         GetRequestedTree(userPtr, pathPtr),
                         contextRef,
                         GetPathOnly(pathPtr),
                         maxString);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a string value to the configuration tree.
 *
 *  When strings are written to the configuration tree, an attempt is made to guess the type of the
 *  string.
 *
 *  The algorithim used for this guess is as follows:
 *
 *  - If the string is the literal value, "true" or "false" then the value is treated as a boolean.
 *  - If the string contains nothing but numeric characters, optionally starting with a - then it is
 *    treated as an integer.
 *  - If the value contains a decmal place, and/or an exponent, then it's treated as a float.
 *  - All other values are treated as a string.
 *
 *  @return - LE_OK            - The commit was completed successfuly.
 *          - LE_BAD_PARAMETER - Attempted to use an invalid path for this request.
 *          - LE_NOT_PERMITTED - Attempted to use a path that is out of bounds.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetString
(
    le_cfg_Context_t contextRef,  ///< This handle is used to generate the reply for this message.
    const char* pathPtr,          ///< Path in the tree to write to.
    const char* valuePtr          ///< The value to write.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick set string, <%s>.", pathPtr);

    UserInfo_t* userPtr = GetCurrentUserInfo();
    HandleQuickSetString(userPtr,
                         GetRequestedTree(userPtr, pathPtr),
                         le_cfg_GetClientSessionRef(),
                         contextRef,
                         GetPathOnly(pathPtr),
                         valuePtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a signed integer value from the configuration tree.  If the underlying value is not an
 *  integer it will be converted.
 *
 *  If the value is a string, then 0 is returned.  If the value is a float, then a truncated int is
 *  returned.  If the value is a bool then a 0 or a 1 is returned.
 *
 *  @return - LE_OK            - The commit was completed successfuly.
 *          - LE_BAD_PARAMETER - Attempted to use an invalid path for this request.
 *          - LE_NOT_PERMITTED - Attempted to use a path that is out of bounds.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickGetInt
(
    le_cfg_Context_t contextRef,  ///< This handle is used to generate the reply for this message.
    const char* pathPtr           ///< Path in the tree to read from.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick get int, <%s>.", pathPtr);

    UserInfo_t* userPtr = GetCurrentUserInfo();
    HandleQuickGetInt(userPtr,
                      GetRequestedTree(userPtr, pathPtr),
                      contextRef,
                      GetPathOnly(pathPtr));
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a signed integer value to the configuration tree.  If the iterator is invalid then the
 *  write request is ignored.
 *
 *  @return - LE_OK            - The commit was completed successfuly.
 *          - LE_BAD_PARAMETER - Attempted to use an invalid path for this request.
 *          - LE_NOT_PERMITTED - Attempted to use a path that is out of bounds.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetInt
(
    le_cfg_Context_t contextRef,  ///< This handle is used to generate the reply for this message.
    const char* pathPtr,          ///< Path in the tree to write to.
    int32_t value                 ///< The value to write.
)
// -------------------------------------------------------------------------------------------------
{
    UserInfo_t* userPtr = GetCurrentUserInfo();
    HandleQuickSetInt(userPtr,
                      GetRequestedTree(userPtr, pathPtr),
                      le_cfg_GetClientSessionRef(),
                      contextRef,
                      GetPathOnly(pathPtr),
                      value);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a 64-bit floating point value from the configuration tree.  If the underlying value is not
 *  a float, then it will be converted.
 *
 *  If the value is an int, then a straight conversion will be performed.  Bool values will be read
 *  as either a floating point, 0 or 1.  Strings and empty values are read as 0.0.
 *
 *  @return - LE_OK            - The commit was completed successfuly.
 *          - LE_BAD_PARAMETER - Attempted to use an invalid path for this request.
 *          - LE_NOT_PERMITTED - Attempted to use a path that is out of bounds.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickGetFloat
(
    le_cfg_Context_t contextRef,  ///< This handle is used to generate the reply for this message.
    const char* pathPtr           ///< Path in the tree to read from.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick get float, <%s>.", pathPtr);

    UserInfo_t* userPtr = GetCurrentUserInfo();
    HandleQuickGetFloat(userPtr,
                        GetRequestedTree(userPtr, pathPtr),
                        contextRef,
                        GetPathOnly(pathPtr));
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a 64-bit floating point value to the configuration tree.  If the iterator is invalid then
 *  the write request is ignored.
 *
 *  @return - LE_OK            - The commit was completed successfuly.
 *          - LE_BAD_PARAMETER - Attempted to use an invalid path for this request.
 *          - LE_NOT_PERMITTED - Attempted to use a path that is out of bounds.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetFloat
(
    le_cfg_Context_t contextRef,  ///< This handle is used to generate the reply for this message.
    const char* pathPtr,          ///< Path in the tree to write to.
    double value                  ///< The value to write.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick set float, <%s>.", pathPtr);

    UserInfo_t* userPtr = GetCurrentUserInfo();
    HandleQuickSetFloat(userPtr,
                        GetRequestedTree(userPtr, pathPtr),
                        le_cfg_GetClientSessionRef(),
                        contextRef,
                        GetPathOnly(pathPtr),
                        value);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a value from the tree as a boolean.  Null or empty values are considered false, non-zero
 *  values are considered true.
 *
 *  If the value is a non-empty string, a true is returned, false otherwise.  If the value is a
 *  number and non-zero then a true is returned, again, false otherwise.
 *
 *  @return - LE_OK            - The commit was completed successfuly.
 *          - LE_BAD_PARAMETER - Attempted to use an invalid path for this request.
 *          - LE_NOT_PERMITTED - Attempted to use a path that is out of bounds.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickGetBool
(
    le_cfg_Context_t contextRef,  ///< This handle is used to generate the reply for this message.
    const char* pathPtr           ///< Path in the tree to read from.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick get bool, <%s>.", pathPtr);

    UserInfo_t* userPtr = GetCurrentUserInfo();
    HandleQuickGetBool(userPtr,
                       GetRequestedTree(userPtr, pathPtr),
                       contextRef,
                       GetPathOnly(pathPtr));
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a boolean value to the configuration tree.  If the iterator is invalid then the write
 *  request is ignored.
 *
 *  @return - LE_OK            - The commit was completed successfuly.
 *          - LE_BAD_PARAMETER - Attempted to use an invalid path for this request.
 *          - LE_NOT_PERMITTED - Attempted to use a path that is out of bounds.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetBool
(
    le_cfg_Context_t contextRef,  ///< This handle is used to generate the reply for this message.
    const char* pathPtr,          ///< Path in the tree to write to.
    bool value                    ///< The value to write.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Quick set bool, <%s>.", pathPtr);

    UserInfo_t* userPtr = GetCurrentUserInfo();
    HandleQuickSetBool(userPtr,
                       GetRequestedTree(userPtr, pathPtr),
                       le_cfg_GetClientSessionRef(),
                       contextRef,
                       GetPathOnly(pathPtr),
                       value);
}




// -------------------------------------------------------------------------------------------------
//  Transactional reading/writing, creation/deletion.
// -------------------------------------------------------------------------------------------------




// -------------------------------------------------------------------------------------------------
/**
 *  Delete the node specified by the path.  If the node doesn't exist, nothing happens.  If the node
 *  has children then all of the child nodes are also deleted.
 *
 *  If the path is empty, then the iterator's current node is deleted.
 *
 *  This function is only valid during a write transaction.
 *
 *  @note If the iterator is invalid, or it's not writeable then this request will be ignored.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_DeleteNode
(
    le_cfg_Context_t contextRef,    ///< This handle is used to generate the reply for this message.
    le_cfg_iteratorRef_t iteratorRef,  ///< The transaction the write occurs on.
    const char* pathPtr             ///< Path to the node to delete.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Iterator <%p>, delete node, <%s>.", iteratorRef, pathPtr);

    tdb_NodeRef_t nodeRef = GetNodeRef(iteratorRef, ITER_GET_NO_DEFAULT_ROOT, pathPtr);

    if (nodeRef != NULL)
    {
        tdb_DeleteNode(nodeRef);
    }

    le_cfg_DeleteNodeRespond(contextRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check if the given leaf is empty.  A leaf is also considered empty if it doesn't yet exist.
 *
 *  If the path is NULL or empty, then the iterator's current node is queried for emptiness.
 *
 *  This function is valid for both read and write transactions.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_IsEmpty
(
    le_cfg_Context_t contextRef,    ///< This handle is used to generate the reply for this message.
    le_cfg_iteratorRef_t iteratorRef,  ///< The transaction the read occurs on.
    const char* pathPtr             ///< Path to the node to check.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Iterator <%p>, check for empty node, <%s>.", iteratorRef, pathPtr);

    tdb_NodeRef_t nodeRef = GetNodeRef(iteratorRef, ITER_GET_NO_DEFAULT_ROOT, pathPtr);

    if (nodeRef == NULL)
    {
        le_cfg_IsEmptyRespond(contextRef, true);
    }
    else
    {
        le_cfg_IsEmptyRespond(contextRef, tdb_IsNodeEmpty(nodeRef));
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Clear out the leaf's value.  If it doesn't exist it will be created, but have no value.
 *
 *  If the path is NULL or empty, then the iterator's current node will be cleared.
 *
 *  This function is only valid during a write transaction.
 *
 *  @note If the iterator is invalid, or it's not writeable then this request will be ignored.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetEmpty
(
    le_cfg_Context_t contextRef,       ///< This handle is used to generate the reply for this
                                       ///<   message.
    le_cfg_iteratorRef_t iteratorRef,  ///< The iterator object to update.
    const char* pathPtr                ///< Path to the node to clear.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Iterator <%p>, set node empty, <%s>.", iteratorRef, pathPtr);

    tdb_NodeRef_t nodeRef = GetNodeRef(iteratorRef, ITER_GET_NO_DEFAULT_ROOT, pathPtr);

    if (nodeRef != NULL)
    {
        tdb_ClearNode(nodeRef);
    }

    le_cfg_SetEmptyRespond(contextRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a string value from the configuration tree.  If the stored value is not a string the value
 *  will be converted into a string.
 *
 *  If the value is a number, then a string with that number is returned.  If the value is empty, or
 *  the iterator is invalid, an empty string is returned.  If the value is boolean, then the string,
 *  "true" or "false" is returned.
 *
 *  This function is valid for both read and write transactions.
 *
 *  If the path is empty, then the iterator's current node will be read.
 *
 *  If the iterator is invalid, then an empty string is returned.
 *
 *  @return - LE_OK       - The write was completed successfuly.
 *          - LE_OVERFLOW - The supplied string buffer was not large enough to hold the value.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GetString
(
    le_cfg_Context_t contextRef,       ///< This handle is used to generate the reply for this message.
    le_cfg_iteratorRef_t iteratorRef,  ///< The absolute or relative path to read from.
    const char* pathPtr,               ///< The buffer to write the value into.
    size_t maxString                   ///< The size of the return buffer.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Iterator <%p>, get node string, <%s>.", iteratorRef, pathPtr);

    tdb_NodeRef_t nodeRef = GetNodeRef(iteratorRef, ITER_GET_NO_DEFAULT_ROOT, pathPtr);
    char* stringBuffer = sb_Get();
    le_result_t result = LE_OK;

    if (nodeRef != NULL)
    {
        tdb_GetAsString(nodeRef, stringBuffer, SB_SIZE);

        if ((strnlen(stringBuffer, SB_SIZE) + 1) > maxString)
        {
            result = LE_OVERFLOW;
            stringBuffer[maxString - 1] = 0;
        }
    }

    le_cfg_GetStringRespond(contextRef, result, stringBuffer);
    sb_Release(stringBuffer);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a string value to the configuration tree.  If the iterator is invalid then the write
 *  request is ignored.
 *
 *  This function is only valid during a write transaction.
 *
 *  If the path is NULL or empty, then the iterator's current node will be set.
 *
 *  @note If the iterator is invalid, or it's not writeable then this request will be ignored.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetString
(
    le_cfg_Context_t contextRef,       ///< This handle is used to generate the reply for this
                                       ///<   message.
    le_cfg_iteratorRef_t iteratorRef,  ///< The write transaction to use.
    const char* pathPtr,               ///< The absolute or relative path to the value to update.
    const char* valuePtr               ///< The value to write.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Iterator <%p>, set node string, <%s>.", iteratorRef, pathPtr);

    tdb_NodeRef_t nodeRef = GetNodeRef(iteratorRef, ITER_GET_NO_DEFAULT_ROOT, pathPtr);

    if (nodeRef != NULL)
    {
        tdb_SetAsString(nodeRef, valuePtr);
    }

    le_cfg_SetStringRespond(contextRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a signed integer value from the configuration tree.  If the underlying value is not an
 *  integer it will be converted.
 *
 *  If the value is a string, then 0 is returned.  If the value is a float, then a truncated int is
 *  returned.  If the value is a bool then a 0 or a 1 is returned.
 *
 *  This function is valid for both read and write transactions.
 *
 *  If the path is NULL or empty, then the iterator's current node will be read.
 *
 *  If the iterator is invalid, or the value is empty, then a 0 is returned.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GetInt
(
    le_cfg_Context_t contextRef,       ///< This handle is used to generate the reply for this message.
    le_cfg_iteratorRef_t iteratorRef,  ///< Use this iterator as a starting point.
    const char* pathPtr                ///< The path to the value to read.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Iterator <%p>, get node value as int, <%s>.", iteratorRef, pathPtr);

    tdb_NodeRef_t nodeRef = GetNodeRef(iteratorRef, ITER_GET_NO_DEFAULT_ROOT, pathPtr);
    int32_t value = 0;

    if (nodeRef != NULL)
    {
        value = tdb_GetAsInt(nodeRef);
    }

    le_cfg_GetIntRespond(contextRef, value);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a signed integer value to the configuration tree.  If the iterator is invalid then the
 *  write request is ignored.
 *
 *  This function is only valid during a write transaction.
 *
 *  If the path is NULL or empty, then the iterator's current node will be set.
 *
 *  @note If the iterator is invalid, or it's not writeable then this request will be ignored.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetInt
(
    le_cfg_Context_t contextRef,       ///< This handle is used to generate the reply for this
                                       ///<   message.
    le_cfg_iteratorRef_t iteratorRef,  ///< Use this iterator as the starting point.
    const char* pathPtr,               ///< The absolute or relative path to the value to update.
    int32_t value                      ///< The value to write.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Iterator <%p>, set node value as int, <%s>.", iteratorRef, pathPtr);

    tdb_NodeRef_t nodeRef = GetNodeRef(iteratorRef, ITER_GET_NO_DEFAULT_ROOT, pathPtr);

    if (nodeRef != NULL)
    {
        tdb_SetAsInt(nodeRef, value);
    }

    le_cfg_SetIntRespond(contextRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a 32-bit floating point value from the configuration tree.  If the underlying value is not
 *  a float, then it will be converted.
 *
 *  If the value is an int, then a straight conversion will be performed.  Bool values will be read
 *  as either a floating point, 0 or 1.  Strings and empty values are read as 0.0.
 *
 *  If the path is NULL or empty, then the iterator's current node will be read.
 *
 *  If the iterator is invalid, or the value is empty, then a 0.0 is returned.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GetFloat
(
    le_cfg_Context_t contextRef,    ///< This handle is used to generate the reply for this message.
    le_cfg_iteratorRef_t iteratorRef,  ///< The iterator transaction to read from.
    const char* pathPtr             ///< The absolute or relative path to the value to update.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Iterator <%p>, get node value as float, <%s>.", iteratorRef, pathPtr);

    tdb_NodeRef_t nodeRef = GetNodeRef(iteratorRef, ITER_GET_NO_DEFAULT_ROOT, pathPtr);
    double value = 0.0;

    if (nodeRef != NULL)
    {
        value = tdb_GetAsFloat(nodeRef);
    }

    le_cfg_GetFloatRespond(contextRef, value);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a 64-bit floating point value to the configuration tree.  If the iterator is invalid then
 *  the write request is ignored.
 *
 *  This function is only valid during a write transaction.
 *
 *  If the path is empty, then the iterator's current node will be set.
 *
 *  @note If the iterator is invalid, or it's not writeable then this request will be ignored.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetFloat
(
    le_cfg_Context_t contextRef,       ///< This handle is used to generate the reply for this
                                       ///<   message.
    le_cfg_iteratorRef_t iteratorRef,  ///< The iterator transaction to write to.
    const char* pathPtr,               ///< The absolute or relative path to the value to update.
    double value                       ///< The new value to write.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Iterator <%p>, set node value as float, <%s>.", iteratorRef, pathPtr);

    tdb_NodeRef_t nodeRef = GetNodeRef(iteratorRef, ITER_GET_NO_DEFAULT_ROOT, pathPtr);

    if (nodeRef != NULL)
    {
        tdb_SetAsFloat(nodeRef, value);
    }

    le_cfg_SetFloatRespond(contextRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a value from the tree as a boolean.  Null or empty values are considered false, non-zero
 *  values are considered true.
 *
 *  If the value is a non-empty string, a true is returned, false otherwise.  If the value is a
 *  number and non-zero then a true is returned, again, false otherwise.
 *
 *  This function is valid for both read and write transactions.
 *
 *  If the path is NULL or empty, then the iterator's current node will be read.
 *
 *  If the iterator is invalid, or the value is empty, then a false is returned.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GetBool
(
    le_cfg_Context_t contextRef,    ///< This handle is used to generate the reply for this message.
    le_cfg_iteratorRef_t iteratorRef,  ///< The iterator handing the transacion we're using.
    const char* pathPtr             ///< The absolute or relative path to the value to read.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Iterator <%p>, get node value as bool, <%s>.", iteratorRef, pathPtr);

    tdb_NodeRef_t nodeRef = GetNodeRef(iteratorRef, ITER_GET_NO_DEFAULT_ROOT, pathPtr);
    bool value = false;

    if (nodeRef != NULL)
    {
        value = tdb_GetAsBool(nodeRef);
    }

    le_cfg_GetBoolRespond(contextRef, value);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a boolean value to the configuration tree.  If the iterator is invalid then the write
 *  request is ignored.
 *
 *  This function is only valid during a write transaction.
 *
 *  If the path is NULL or empty, then the iterator's current node will be set.
 *
 *  @note If the iterator is invalid, or it's not writeable then this request will be ignored.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetBool
(
    le_cfg_Context_t contextRef,    ///< This handle is used to generate the reply for this message.
    le_cfg_iteratorRef_t iteratorRef,  ///< The iterator to use for the transaction.
    const char* pathPtr,            ///< The absolute or relative path to the value to update.
    bool value                      ///< The value to write.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Iterator <%p>, set node value as bool, <%s>.", iteratorRef, pathPtr);

    tdb_NodeRef_t nodeRef = GetNodeRef(iteratorRef, ITER_GET_NO_DEFAULT_ROOT, pathPtr);

    if (nodeRef != NULL)
    {
        tdb_SetAsBool(nodeRef, value);
    }

    le_cfg_SetBoolRespond(contextRef);
}




// -------------------------------------------------------------------------------------------------
//  Listing configuration trees.
// -------------------------------------------------------------------------------------------------




// -------------------------------------------------------------------------------------------------
/**
 *  Create an iterator that can list all of the trees registered with the system.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_CreateTreeIterator
(
    le_cfgAdmin_Context_t contextRef  ///< This handle is used to generate the reply for this message.
)
// -------------------------------------------------------------------------------------------------
{
    le_cfgAdmin_CreateTreeIteratorRespond(contextRef, NULL);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Call this function when you are done with the iterator.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_DeleteTreeIterator
(
    le_cfgAdmin_Context_t contextRef,          ///< This handle is used to generate the reply for
                                               ///<   this message.
    le_cfgAdmin_treeIteratorRef_t iteratorRef  ///< The iterator object to dispose of.
)
// -------------------------------------------------------------------------------------------------
{
    le_cfgAdmin_DeleteTreeIteratorRespond(contextRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read the name of the tree currently pointed at by the iterator.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_GetTreeName
(
    le_cfgAdmin_Context_t contextRef,           ///< This handle is used to generate the reply for
                                                ///<   this message.
    le_cfgAdmin_treeIteratorRef_t iteratorRef,  ///< The iterator object to read.
    size_t maxNameBuffer                        ///< The maximum size of the returnable string.
)
// -------------------------------------------------------------------------------------------------
{
    le_cfgAdmin_GetTreeNameRespond(contextRef, "");
}




// -------------------------------------------------------------------------------------------------
/**
 *  Move onto the next tree in the list.  If there are no more trees this function returns false,
 *  otherwise true is returned.
 */
// -------------------------------------------------------------------------------------------------
void le_cfgAdmin_NextTree
(
    le_cfgAdmin_Context_t contextRef,          ///< This handle is used to generate the reply for
                                               ///<   this message.
    le_cfgAdmin_treeIteratorRef_t iteratorRef  ///< The iterator object to increment.
)
// -------------------------------------------------------------------------------------------------
{
    le_cfgAdmin_NextTreeRespond(contextRef, false);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Initialize all of our memory pools, and make sure that configuration trees are loaded up and
 *  ready to go.
 */
// -------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    user_Init();

    le_cfg_StartServer("configTree");
    le_cfgAdmin_StartServer("configTreeAdmin");

    UserTreeInit();
    itr_Init();
    sb_Init();
    tdb_Init();

    //InitializeInternalConfig();


    // Close the fd that we inherited from the Supervisor.  This will let the Supervisor know that
    // we are initialized.  Then re-open it to /dev/null so that it cannot be reused later.
    FILE* filePtr;

    do
    {
        filePtr = freopen("/dev/null", "r", stdin);
    }
    while ((filePtr == NULL) && (errno == EINTR));

    LE_FATAL_IF(filePtr == NULL, "Failed to redirect standard in to /dev/null.  %m.");

    LE_INFO("The configTree service has been started.");
}
