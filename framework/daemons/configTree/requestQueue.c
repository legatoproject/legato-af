
// -------------------------------------------------------------------------------------------------
/**
 *  @file requestQueue.c
 *
 *  This module takes care of handling and as required, queuing tree requests from the users of the
 *  config tree API.  So, if a request can not be handled right away, it is queued for later
 *  processing.
 *
 *  This module also takes care of handling call backs to the user so that they can know their
 *  request has been completed.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "treeDb.h"
#include "treeUser.h"
#include "nodeIterator.h"
#include "requestQueue.h"


// -------------------------------------------------------------------------------------------------
/**
 *  Request structure, if the user's request on the DB can't be handled right away it is stored in
 *  this structure for later handling.
 */
// -------------------------------------------------------------------------------------------------
typedef struct UpdateRequest
{
    RequestType_t type;                          ///< Request id.

    tu_UserRef_t userRef;                        ///< User requesting the processing.
    tdb_TreeRef_t treeRef;                       ///< The tree to be operated on.

    le_msg_SessionRef_t sessionRef;              ///< The context for the session the message came
                                                 ///<   in on.
    le_cfg_ServerCmdRef_t commandRef;            ///< Message context for the request.

    union
    {
        struct
        {
            char pathPtr[LE_CFG_STR_LEN_BYTES];  ///< Initial path for the requested iterator.
        }
        createTxn;                               ///< Create new transaction info.

        struct
        {
            ni_IteratorRef_t iteratorRef;        ///< Ptr to the iterator to commit.
        }
        commitTxn;

        struct
        {
            ni_IteratorRef_t iteratorRef;        ///< Ptr to the iterator to commit.
        }
        deleteTxn;

        struct
        {
            char pathPtr[LE_CFG_STR_LEN_BYTES];  ///< Path to the value to operate on.

            union
            {
                char AsStringPtr[TDB_MAX_ENCODED_SIZE];
                int AsInt;
                float AsFloat;
                bool AsBool;
            }
            value;
        }
        writeReq;
    }
    data;

    le_sls_Link_t link;                     ///< Link to the next request in the queue.
}
UpdateRequest_t;


// -------------------------------------------------------------------------------------------------
/**
 *  When client sessions are closed, this structure is used as part of the clean up process.
 */
// -------------------------------------------------------------------------------------------------
typedef struct SessionCloseInfo
{
    le_sls_List_t list;              ///< Used to store a list of iterator delete requests.
    le_msg_SessionRef_t sessionRef;  ///< The session that is being cleaned up.
}
SessionCloseInfo_t;


/// Define static pool for config update requests
LE_MEM_DEFINE_STATIC_POOL(requestPool, LE_CONFIG_CFGTREE_MAX_UPDATE_POOL_SIZE,
    sizeof(UpdateRequest_t));


/// Pool that handles config update requests.
static le_mem_PoolRef_t RequestPool = NULL;


// -------------------------------------------------------------------------------------------------
/**
 *  Create a new request block.
 */
// -------------------------------------------------------------------------------------------------
static UpdateRequest_t* NewRequestBlock
(
    RequestType_t type,               ///< [IN] The type of request to create.
    tu_UserRef_t userRef,             ///< [IN] Create the request for this user.
    tdb_TreeRef_t treeRef,            ///< [IN] The tree to use in the request.
    le_msg_SessionRef_t sessionRef,   ///< [IN] Session that the request came in on.
    le_cfg_ServerCmdRef_t commandRef  ///< [IN] The context to reply on.
)
// -------------------------------------------------------------------------------------------------
{
    UpdateRequest_t* requestPtr = le_mem_ForceAlloc(RequestPool);

    memset(requestPtr, 0, sizeof(UpdateRequest_t));

    requestPtr->type = type;
    requestPtr->userRef = userRef;
    requestPtr->treeRef = treeRef;
    requestPtr->sessionRef = sessionRef;
    requestPtr->commandRef = commandRef;
    requestPtr->link = LE_SLS_LINK_INIT;

    LE_DEBUG("** Allocated request block <%p>.", requestPtr);

    return requestPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Free up the request block.
 */
// -------------------------------------------------------------------------------------------------
static void ReleaseRequestBlock
(
    UpdateRequest_t* requestPtr  ///< [IN] The request to free.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Releasing request block <%p>.", requestPtr);

    requestPtr->type = RQ_INVALID;
    le_mem_Release(requestPtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Queue a generic request object for later processing.
 */
// -------------------------------------------------------------------------------------------------
static void QueueRequest
(
    le_sls_List_t* listPtr,      ///< [IN] Queue the request onto this list.
    UpdateRequest_t* requestPtr  ///< [IN] THe actual request info itself.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Queuing request block <%p>.", requestPtr);
    le_sls_Queue(listPtr, &(requestPtr->link));
}




// -------------------------------------------------------------------------------------------------
/**
 *  Queue a create transaction request.
 */
// -------------------------------------------------------------------------------------------------
static void QueueCreateTxnRequest
(
    tu_UserRef_t userRef,              ///< [IN] The user that requested this.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree we're working on.
    le_msg_SessionRef_t sessionRef,    ///< [IN] The user session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Context for this request.
    ni_IteratorType_t iteratorType,    ///< [IN] The kind of iterator to create.
    const char* basePathPtr            ///< [IN] The initial path for the iterator.
)
// -------------------------------------------------------------------------------------------------
{
    RequestType_t type = iteratorType == NI_READ ? RQ_CREATE_READ_TXN : RQ_CREATE_WRITE_TXN;
    UpdateRequest_t* requestPtr = NewRequestBlock(type, userRef, treeRef, sessionRef, commandRef);

    LE_ASSERT(le_utf8_Copy(requestPtr->data.createTxn.pathPtr,
                           basePathPtr,
                           sizeof(requestPtr->data.createTxn.pathPtr),
                           NULL) == LE_OK);

    QueueRequest(tdb_GetRequestQueue(treeRef), requestPtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Queue a request to delete an iterator and it's transaction.
 */
// -------------------------------------------------------------------------------------------------
static void QueueDeleteTxnRequest
(
    ni_IteratorRef_t iteratorRef,  ///< [IN] The underlying iterator pointer.
    le_sls_List_t* listPtr         ///< [IN] The to remove the request from.
)
// -------------------------------------------------------------------------------------------------
{
    // This is an internal request...  That is requests from the outside of this application always
    // succeed and do not get queued up.

    // However when a session gets closed that's a different matter.  We need to iterate the list of
    // open transactions in that case and can not delete anything while an iteration is ongoing.  So
    // we have to record all of the iterators that need deletion and actually handle that deletion
    // as a seperate step.
    UpdateRequest_t* requestPtr = NewRequestBlock(RQ_DELETE_TXN,
                                                  ni_GetUser(iteratorRef),
                                                  ni_GetTree(iteratorRef),
                                                  NULL,
                                                  NULL);

    requestPtr->data.deleteTxn.iteratorRef = iteratorRef;

    QueueRequest(listPtr, requestPtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Process all of the queued requests.
 */
// -------------------------------------------------------------------------------------------------
static void ProcessRequestQueue
(
    le_sls_List_t* listPtr,                ///< [IN] Process any pending requests in this list.
    le_msg_SessionRef_t ignoreSessionRef   ///< [IN] Throw away any requests that occured on this
                                           ///<      session.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Processing request queue now.");


    // Extract the request queue. Go through the requests and process them.  If required, the
    // handlers will requeue requests.

    le_sls_List_t list = *listPtr;
    *listPtr = LE_SLS_LIST_INIT;

    le_sls_Link_t* linkPtr = le_sls_Pop(&list);

    while (linkPtr != NULL)
    {
        UpdateRequest_t* requestPtr = CONTAINER_OF(linkPtr, UpdateRequest_t, link);

        // If this request belongs to a session that's been closed,
        if (   (ignoreSessionRef != NULL)
            && (requestPtr->sessionRef == ignoreSessionRef))
        {
            LE_DEBUG("** Dropping orphaned request block <%p>, from user %u (%s) on tree '%s'.",
                     requestPtr,
                     tu_GetUserId(requestPtr->userRef),
                     tu_GetUserName(requestPtr->userRef),
                     tdb_GetTreeName(requestPtr->treeRef));
        }
        else
        {
            LE_DEBUG("** Process request block <%p>.", requestPtr);

            switch (requestPtr->type)
            {
                case RQ_CREATE_WRITE_TXN:
                    LE_DEBUG("Starting deferred write txn for user %u (%s) on tree '%s'.",
                             tu_GetUserId(requestPtr->userRef),
                             tu_GetUserName(requestPtr->userRef),
                             tdb_GetTreeName(requestPtr->treeRef));

                    rq_HandleCreateTxnRequest(requestPtr->userRef,
                                              requestPtr->treeRef,
                                              requestPtr->sessionRef,
                                              requestPtr->commandRef,
                                              NI_WRITE,
                                              requestPtr->data.createTxn.pathPtr);
                    break;

                case RQ_COMMIT_WRITE_TXN:
                    LE_DEBUG("Committing deferred write txn for user %u (%s) on tree '%s'.",
                             tu_GetUserId(requestPtr->userRef),
                             tu_GetUserName(requestPtr->userRef),
                             tdb_GetTreeName(requestPtr->treeRef));

                    rq_HandleCommitTxnRequest(requestPtr->commandRef,
                                              requestPtr->data.commitTxn.iteratorRef);
                    break;

               case RQ_CREATE_READ_TXN:
                    LE_DEBUG("Starting deferred read txn for user %u (%s) on tree '%s'.",
                             tu_GetUserId(requestPtr->userRef),
                             tu_GetUserName(requestPtr->userRef),
                             tdb_GetTreeName(requestPtr->treeRef));

                    rq_HandleCreateTxnRequest(requestPtr->userRef,
                                              requestPtr->treeRef,
                                              requestPtr->sessionRef,
                                              requestPtr->commandRef,
                                              NI_READ,
                                              requestPtr->data.createTxn.pathPtr);
                    break;

                case RQ_DELETE_TXN:
                    LE_DEBUG("Handling deferred iterator delete for user %u (%s) on tree '%s'.",
                             tu_GetUserId(requestPtr->userRef),
                             tu_GetUserName(requestPtr->userRef),
                             tdb_GetTreeName(requestPtr->treeRef));

                    rq_HandleCancelTxnRequest(requestPtr->commandRef,
                                              requestPtr->data.deleteTxn.iteratorRef);
                    break;

                case RQ_DELETE_NODE:
                    LE_DEBUG("Processing deferred quick delete for user %u (%s) on tree '%s'.",
                             tu_GetUserId(requestPtr->userRef),
                             tu_GetUserName(requestPtr->userRef),
                             tdb_GetTreeName(requestPtr->treeRef));

                    rq_HandleQuickDeleteNode(requestPtr->sessionRef,
                                             requestPtr->commandRef,
                                             requestPtr->userRef,
                                             requestPtr->treeRef,
                                             requestPtr->data.writeReq.pathPtr);
                    break;

                case RQ_SET_EMPTY:
                    LE_DEBUG("Processing deferred quick 'set empty' for user %u (%s) on tree '%s'.",
                             tu_GetUserId(requestPtr->userRef),
                             tu_GetUserName(requestPtr->userRef),
                             tdb_GetTreeName(requestPtr->treeRef));

                    rq_HandleQuickSetEmpty(requestPtr->sessionRef,
                                           requestPtr->commandRef,
                                           requestPtr->userRef,
                                           requestPtr->treeRef,
                                           requestPtr->data.writeReq.pathPtr);
                    break;

                case RQ_SET_STRING:
                case RQ_SET_BINARY:
                    LE_DEBUG("Processing deferred quick 'set string/binary' for user %u (%s) on tree '%s'.",
                             tu_GetUserId(requestPtr->userRef),
                             tu_GetUserName(requestPtr->userRef),
                             tdb_GetTreeName(requestPtr->treeRef));

                    rq_HandleQuickSetData(requestPtr->sessionRef,
                                          requestPtr->commandRef,
                                          requestPtr->userRef,
                                          requestPtr->treeRef,
                                          requestPtr->data.writeReq.pathPtr,
                                          requestPtr->data.writeReq.value.AsStringPtr,
                                          requestPtr->type);
                    break;

                case RQ_SET_INT:
                    LE_DEBUG("Processing deferred quick 'set int' for user %u (%s) on tree '%s'.",
                             tu_GetUserId(requestPtr->userRef),
                             tu_GetUserName(requestPtr->userRef),
                             tdb_GetTreeName(requestPtr->treeRef));

                    rq_HandleQuickSetInt(requestPtr->sessionRef,
                                         requestPtr->commandRef,
                                         requestPtr->userRef,
                                         requestPtr->treeRef,
                                         requestPtr->data.writeReq.pathPtr,
                                         requestPtr->data.writeReq.value.AsInt);
                    break;

                case RQ_SET_FLOAT:
                    LE_DEBUG("Processing deferred quick 'set float' for user %u (%s) on tree '%s'.",
                              tu_GetUserId(requestPtr->userRef),
                              tu_GetUserName(requestPtr->userRef),
                              tdb_GetTreeName(requestPtr->treeRef));

                     rq_HandleQuickSetFloat(requestPtr->sessionRef,
                                           requestPtr->commandRef,
                                           requestPtr->userRef,
                                           requestPtr->treeRef,
                                           requestPtr->data.writeReq.pathPtr,
                                           requestPtr->data.writeReq.value.AsFloat);
                    break;

                case RQ_SET_BOOL:
                    LE_DEBUG("Processing deferred quick 'set bool' for user %u (%s) on tree '%s'.",
                             tu_GetUserId(requestPtr->userRef),
                             tu_GetUserName(requestPtr->userRef),
                             tdb_GetTreeName(requestPtr->treeRef));

                    rq_HandleQuickSetBool(requestPtr->sessionRef,
                                          requestPtr->commandRef,
                                          requestPtr->userRef,
                                          requestPtr->treeRef,
                                          requestPtr->data.writeReq.pathPtr,
                                          requestPtr->data.writeReq.value.AsBool);
                    break;

                case RQ_INVALID:
                    LE_FATAL("Invalid request block used.");
            }
        }

        ReleaseRequestBlock(requestPtr);
        linkPtr = le_sls_Pop(&list);
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Check to see if the given tree is open for quick writes.
 *
 *  @return True if a quick write can safely be performed.  False if not.
 */
//--------------------------------------------------------------------------------------------------
static bool CanQuickSet
(
    tdb_TreeRef_t treeRef  ///< [IN] The tree to check.
)
//--------------------------------------------------------------------------------------------------
{
    // If there are active readers or writers on the tree then a quick write should be defered.
    if (   (tdb_HasActiveReaders(treeRef) == false)
        && (tdb_GetActiveWriteIter(treeRef) == NULL))
    {
        return true;
    }

    return false;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Called for each active iterator.  If the iterator belongs to the sesion being closed, then it is
 *  queued for deletion.
 *
 *  The iterator delete request is queued, because it is not safe to try to delete them while
 *  iterating over them.
 */
//--------------------------------------------------------------------------------------------------
static void OnIteratorSessionClosed
(
    ni_ConstIteratorRef_t iteratorRef,  ///< [IN] The iterator pointer.
    void* contextPtr                    ///< [IN] User context pointer.
)
//--------------------------------------------------------------------------------------------------
{
    SessionCloseInfo_t* closeInfoPtr = (SessionCloseInfo_t*)contextPtr;

    if (   (ni_GetSession(iteratorRef) == closeInfoPtr->sessionRef)
        && (ni_IsClosed(iteratorRef) == false))
    {
        QueueDeleteTxnRequest((ni_IteratorRef_t)iteratorRef, &closeInfoPtr->list);
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Initialize the memory pools needed by this subsystem.
 */
//--------------------------------------------------------------------------------------------------
void rq_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Initialize Request Queue subsystem.");

    RequestPool = le_mem_InitStaticPool(requestPool, LE_CONFIG_CFGTREE_MAX_UPDATE_POOL_SIZE,
                                        sizeof(UpdateRequest_t));
}




// -------------------------------------------------------------------------------------------------
/**
 *  Whenever a configAPI session is closed, this function is called to do the clean up work.  Any
 *  active requests for that session are automatically canceled.
 */
// -------------------------------------------------------------------------------------------------
void rq_CleanUpForSession
(
    le_msg_SessionRef_t sessionRef  ///< [IN] Reference to the session that's going away.
)
//--------------------------------------------------------------------------------------------------
{
    SessionCloseInfo_t closeInfo = { LE_SLS_LIST_INIT, sessionRef };


    // Grab all open iterators attached to this session and queue them to close.  Once that's done
    // process that request queue.
    ni_ForEachIter(OnIteratorSessionClosed, &closeInfo);
    ProcessRequestQueue(&closeInfo.list, NULL);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Create a transaction.  If it can not be crated now, queue it for later.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleCreateTxnRequest
(
    tu_UserRef_t userRef,              ///< [IN] The user to read.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree we're working with.
    le_msg_SessionRef_t sessionRef,    ///< [IN] The user session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] THe reply context.
    ni_IteratorType_t iterType,        ///< [IN] What kind of iterator are we creating?
    const char* pathPtr                ///< [IN] Initial path the iterator is pointed at.
)
//--------------------------------------------------------------------------------------------------
{
    ni_IteratorRef_t writeIteratorRef = tdb_GetActiveWriteIter(treeRef);

    if (   (iterType == NI_READ)
        && (writeIteratorRef != NULL)
        && (ni_IsClosed(writeIteratorRef)))
    {
        QueueCreateTxnRequest(userRef, treeRef, sessionRef, commandRef, iterType, pathPtr);
    }
    else if (   (iterType == NI_WRITE)
             && (writeIteratorRef != NULL))
    {
        QueueCreateTxnRequest(userRef, treeRef, sessionRef, commandRef, iterType, pathPtr);
    }
    else
    {
        ni_IteratorRef_t iteratorRef = ni_CreateIterator(sessionRef,
                                                         userRef,
                                                         treeRef,
                                                         iterType,
                                                         pathPtr);
        if (iteratorRef == NULL)
        {
            tu_TerminateConfigClient(sessionRef, "Could not create iterator for client.");
        }

        if (iterType == NI_READ)
        {
            le_cfg_CreateReadTxnRespond(commandRef, ni_CreateRef(iteratorRef));
        }
        else
        {
            le_cfg_CreateWriteTxnRespond(commandRef, ni_CreateRef(iteratorRef));
        }
    }
}





// -------------------------------------------------------------------------------------------------
/**
 *  Attempt to commit an outstanding write transaction.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleCommitTxnRequest
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Context for the commit reply.
    ni_IteratorRef_t iteratorRef       ///< [IN] Pointer to the iterator that's being committed.
)
//--------------------------------------------------------------------------------------------------
{
    if (ni_IsWriteable(iteratorRef) == false)
    {
        // Kill the iterator but do not try to comit it.
        ni_Release(iteratorRef);

        le_cfg_CommitTxnRespond(commandRef);
        ProcessRequestQueue(tdb_GetRequestQueue(ni_GetTree(iteratorRef)), NULL);
    }
    else if (tdb_HasActiveReaders(ni_GetTree(iteratorRef)) == false)
    {
        ni_Close(iteratorRef);
        ni_Commit(iteratorRef);
        ni_Release(iteratorRef);

        le_cfg_CommitTxnRespond(commandRef);
        ProcessRequestQueue(tdb_GetRequestQueue(ni_GetTree(iteratorRef)), NULL);
    }
    else
    {
        UpdateRequest_t* requestPtr = NewRequestBlock(RQ_COMMIT_WRITE_TXN,
                                                      ni_GetUser(iteratorRef),
                                                      ni_GetTree(iteratorRef),
                                                      ni_GetSession(iteratorRef),
                                                      commandRef);

        requestPtr->data.commitTxn.iteratorRef = iteratorRef;
        QueueRequest(tdb_GetRequestQueue(ni_GetTree(iteratorRef)), requestPtr);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Delete an outstanding iterator object, freeing the transaction.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleCancelTxnRequest
(
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] Context for the commit reply.
    ni_IteratorRef_t iteratorRef       ///< [IN] Pointer to the iterator that's being deleted.
)
//--------------------------------------------------------------------------------------------------
{
    // Get request queue
    le_sls_List_t* requestQueue = tdb_GetRequestQueue(ni_GetTree(iteratorRef));

    // Kill the iterator but do not try to commit it.
    ni_Release(iteratorRef);

    // If there is a context for this handler, then respond to a waiting client.
    // If the commandRef is NULL, then that means that this delete request was generated internally
    // and there is no one to reply to.
    if (commandRef)
    {
        le_cfg_CancelTxnRespond(commandRef);
    }

    // Try to handle the tree's request backlog.  (If any.)
    ProcessRequestQueue(requestQueue, NULL);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Delete a node without an explicit transaction.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickDeleteNode
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr                ///< [IN] The path to the node to delete.
)
//--------------------------------------------------------------------------------------------------
{
    if (CanQuickSet(treeRef) == false)
    {
        UpdateRequest_t* requestPtr = NewRequestBlock(RQ_DELETE_NODE,
                                                      userRef,
                                                      treeRef,
                                                      sessionRef,
                                                      commandRef);

        LE_ASSERT(le_utf8_Copy(requestPtr->data.writeReq.pathPtr,
                               pathPtr,
                               sizeof(requestPtr->data.writeReq.pathPtr),
                               NULL) == LE_OK);

        QueueRequest(tdb_GetRequestQueue(treeRef), requestPtr);
    }
    else
    {
        ni_IteratorRef_t iteratorRef = ni_CreateIterator(sessionRef,
                                                         userRef,
                                                         treeRef,
                                                         NI_WRITE,
                                                         pathPtr);

        ni_DeleteNode(iteratorRef, NULL);
        ni_Commit(iteratorRef);
        ni_Release(iteratorRef);

        le_cfg_QuickDeleteNodeRespond(commandRef);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Clear out a node's contents and leave it empty.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickSetEmpty
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr                ///< [IN] The path to the node to clear.
)
//--------------------------------------------------------------------------------------------------
{
    if (CanQuickSet(treeRef) == false)
    {
        UpdateRequest_t* requestPtr = NewRequestBlock(RQ_SET_EMPTY,
                                                      userRef,
                                                      treeRef,
                                                      sessionRef,
                                                      commandRef);

        LE_ASSERT(le_utf8_Copy(requestPtr->data.writeReq.pathPtr,
                               pathPtr,
                               sizeof(requestPtr->data.writeReq.pathPtr),
                               NULL) == LE_OK);

        QueueRequest(tdb_GetRequestQueue(treeRef), requestPtr);
    }
    else
    {
        ni_IteratorRef_t iteratorRef = ni_CreateIterator(sessionRef,
                                                         userRef,
                                                         treeRef,
                                                         NI_WRITE,
                                                         pathPtr);

        ni_SetEmpty(iteratorRef, NULL);
        ni_Commit(iteratorRef);
        ni_Release(iteratorRef);

        le_cfg_QuickSetEmptyRespond(commandRef);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a string value from the node.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickGetString
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    size_t maxString,                  ///< [IN] Maximum string the caller can handle.
    const char* defaultValuePtr        ///< [IN] If the value doesn't exist use this value instead.
)
//--------------------------------------------------------------------------------------------------
{
    ni_IteratorRef_t iteratorRef = ni_CreateIterator(sessionRef,
                                                     userRef,
                                                     treeRef,
                                                     NI_READ,
                                                     pathPtr);

    char strBufferPtr[LE_CFG_STR_LEN_BYTES] = "";

    if (maxString > LE_CFG_STR_LEN_BYTES)
    {
        maxString = LE_CFG_STR_LEN_BYTES;
    }

    le_result_t result = ni_GetNodeValueString(iteratorRef,
                                               pathPtr,
                                               strBufferPtr,
                                               maxString,
                                               defaultValuePtr);

    le_cfg_QuickGetStringRespond(commandRef, result, strBufferPtr);

    ni_Release(iteratorRef);
}



// -------------------------------------------------------------------------------------------------
/**
 *  Write a string or binary value to a node in the tree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickSetData
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    const char* valuePtr,              ///< [IN] The value to set.
    RequestType_t reqType              ///< [IN] Request type: whether this a string or
                                       ///<      an encoded binary data.
)
//--------------------------------------------------------------------------------------------------
{
    if (CanQuickSet(treeRef) == false)
    {
        UpdateRequest_t* requestPtr = NewRequestBlock(
                                        reqType,
                                        userRef,
                                        treeRef,
                                        sessionRef,
                                        commandRef);

        LE_ASSERT(le_utf8_Copy(requestPtr->data.writeReq.pathPtr,
                               pathPtr,
                               sizeof(requestPtr->data.writeReq.pathPtr),
                               NULL) == LE_OK);

        LE_ASSERT(le_utf8_Copy(requestPtr->data.writeReq.value.AsStringPtr,
                               valuePtr,
                               sizeof(requestPtr->data.writeReq.value.AsStringPtr),
                               NULL) == LE_OK);


        QueueRequest(tdb_GetRequestQueue(treeRef), requestPtr);
    }
    else
    {
        ni_IteratorRef_t iteratorRef = ni_CreateIterator(sessionRef,
                                                         userRef,
                                                         treeRef,
                                                         NI_WRITE,
                                                         pathPtr);

        ni_SetNodeValueString(iteratorRef, NULL, valuePtr);
        ni_Commit(iteratorRef);
        ni_Release(iteratorRef);
        if (RQ_SET_BINARY == reqType)
        {
            le_cfg_QuickSetBinaryRespond(commandRef);
        }
        else if (RQ_SET_STRING == reqType)
        {
            le_cfg_QuickSetStringRespond(commandRef);
        }
        else
        {
            LE_ERROR("Unexpected request type: %d", reqType);
        }
    }
}


// -------------------------------------------------------------------------------------------------
/**
 *  Read a binary data from the node.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickGetBinary
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    size_t maxBinary,                  ///< [IN] Maximum data the caller can handle.
    const uint8_t* defaultValuePtr,    ///< [IN] If the value doesn't exist use this value instead.
    size_t defaultValueSize            ///< [IN] Default value size
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result = LE_OK;

    ni_IteratorRef_t iteratorRef = ni_CreateIterator(sessionRef,
                                                     userRef,
                                                     treeRef,
                                                     NI_READ,
                                                     pathPtr);

    char* stringBuf = le_mem_ForceAlloc(tdb_GetEncodedStringMemoryPool());
    stringBuf[0] = 0;

    uint8_t* binaryBuf = le_mem_ForceAlloc(tdb_GetBinaryDataMemoryPool());
    size_t binaryLen = maxBinary;
    if (maxBinary > LE_CFG_BINARY_LEN)
    {
        binaryLen = LE_CFG_BINARY_LEN;
    }

    // Encode the default valaue
    char* defaultStringBuf = le_mem_ForceAlloc(tdb_GetEncodedStringMemoryPool());
    defaultStringBuf[0] = 0;
    size_t defaultEncodedSize = TDB_MAX_ENCODED_SIZE;
    result = le_base64_Encode(defaultValuePtr,
                              defaultValueSize,
                              defaultStringBuf,
                              &defaultEncodedSize);
    if (LE_OK != result)
    {
        LE_ERROR("ERROR encoding default value: %s", LE_RESULT_TXT(result));
        // Encode error - sending back the default value
        le_cfg_QuickGetBinaryRespond(commandRef,
                                     LE_FORMAT_ERROR,
                                     defaultValuePtr,
                                     defaultValueSize);
    }

    // Get the encoded string
    result = ni_GetNodeValueString(iteratorRef,
                                   pathPtr,
                                   stringBuf,
                                   TDB_MAX_ENCODED_SIZE,
                                   defaultStringBuf);
    if (LE_OK != result)
    {
        // Node not found or has empty type: sending back the default value
        le_cfg_QuickGetBinaryRespond(commandRef, result, defaultValuePtr, defaultValueSize);
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
            LE_ERROR("ERROR decoding default value: %s", LE_RESULT_TXT(result));
        }

        // Send back the decoded value
        le_cfg_QuickGetBinaryRespond(commandRef, result, binaryBuf, binaryLen);
    }

    ni_Release(iteratorRef);
    le_mem_Release(stringBuf);
    le_mem_Release(binaryBuf);
    le_mem_Release(defaultStringBuf);
}


// -------------------------------------------------------------------------------------------------
/**
 *  Get an integer value from the tree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickGetInt
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    int32_t defaultValue               ///< [IN] If the value doesn't exist use this value instead.
)
//--------------------------------------------------------------------------------------------------
{
    ni_IteratorRef_t iteratorRef = ni_CreateIterator(sessionRef,
                                                     userRef,
                                                     treeRef,
                                                     NI_READ,
                                                     pathPtr);

    le_cfg_QuickGetIntRespond(commandRef, ni_GetNodeValueInt(iteratorRef, NULL, defaultValue));
    ni_Release(iteratorRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write an integer value to the configTree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickSetInt
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    int32_t value                      ///< [IN] The value to set.
)
//--------------------------------------------------------------------------------------------------
{
    if (CanQuickSet(treeRef) == false)
    {
        UpdateRequest_t* requestPtr = NewRequestBlock(RQ_SET_INT,
                                                      userRef,
                                                      treeRef,
                                                      sessionRef,
                                                      commandRef);

        LE_ASSERT(le_utf8_Copy(requestPtr->data.writeReq.pathPtr,
                               pathPtr,
                               sizeof(requestPtr->data.writeReq.pathPtr),
                               NULL) == LE_OK);

        requestPtr->data.writeReq.value.AsInt = value;

        QueueRequest(tdb_GetRequestQueue(treeRef), requestPtr);
    }
    else
    {
        ni_IteratorRef_t iteratorRef = ni_CreateIterator(sessionRef,
                                                         userRef,
                                                         treeRef,
                                                         NI_WRITE,
                                                         pathPtr);

        ni_SetNodeValueInt(iteratorRef, NULL, value);
        ni_Commit(iteratorRef);
        ni_Release(iteratorRef);

        le_cfg_QuickSetIntRespond(commandRef);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get a floating point value from the tree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickGetFloat
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    double defaultValue                ///< [IN] If the value doesn't exist use this value instead.
)
//--------------------------------------------------------------------------------------------------
{
    ni_IteratorRef_t iteratorRef = ni_CreateIterator(sessionRef,
                                                     userRef,
                                                     treeRef,
                                                     NI_READ,
                                                     pathPtr);

    le_cfg_QuickGetFloatRespond(commandRef, ni_GetNodeValueFloat(iteratorRef, NULL, defaultValue));
    ni_Release(iteratorRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a floating point value to the configTree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickSetFloat
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    double value                       ///< [IN] The value to set.
)
//--------------------------------------------------------------------------------------------------
{
    if (CanQuickSet(treeRef) == false)
    {
        UpdateRequest_t* requestPtr = NewRequestBlock(RQ_SET_FLOAT,
                                                      userRef,
                                                      treeRef,
                                                      sessionRef,
                                                      commandRef);

        LE_ASSERT(le_utf8_Copy(requestPtr->data.writeReq.pathPtr,
                               pathPtr,
                               sizeof(requestPtr->data.writeReq.pathPtr),
                               NULL) == LE_OK);

        requestPtr->data.writeReq.value.AsFloat = value;

        QueueRequest(tdb_GetRequestQueue(treeRef), requestPtr);
    }
    else
    {
        ni_IteratorRef_t iteratorRef = ni_CreateIterator(sessionRef,
                                                         userRef,
                                                         treeRef,
                                                         NI_WRITE,
                                                         pathPtr);

        ni_SetNodeValueFloat(iteratorRef, NULL, value);
        ni_Commit(iteratorRef);
        ni_Release(iteratorRef);

        le_cfg_QuickSetFloatRespond(commandRef);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get a boolean value from the tree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickGetBool
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    bool defaultValue                  ///< [IN] If the value doesn't exist use this value instead.
)
//--------------------------------------------------------------------------------------------------
{
    ni_IteratorRef_t iteratorRef = ni_CreateIterator(sessionRef,
                                                     userRef,
                                                     treeRef,
                                                     NI_READ,
                                                     pathPtr);

    le_cfg_QuickGetBoolRespond(commandRef, ni_GetNodeValueBool(iteratorRef, NULL, defaultValue));
    ni_Release(iteratorRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a boolean value to the configTree.
 */
// -------------------------------------------------------------------------------------------------
void rq_HandleQuickSetBool
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] The session this request occured on.
    le_cfg_ServerCmdRef_t commandRef,  ///< [IN] This handle is used to generate the reply for this
                                       ///<      message.
    tu_UserRef_t userRef,              ///< [IN] The user that's requesting the action.
    tdb_TreeRef_t treeRef,             ///< [IN] The tree that we're peforming the action on.
    const char* pathPtr,               ///< [IN] The path to the node to access.
    bool value                         ///< [IN] The value to set.
)
//--------------------------------------------------------------------------------------------------
{
    if (CanQuickSet(treeRef) == false)
    {
        UpdateRequest_t* requestPtr = NewRequestBlock(RQ_SET_BOOL,
                                                      userRef,
                                                      treeRef,
                                                      sessionRef,
                                                      commandRef);

        LE_ASSERT(le_utf8_Copy(requestPtr->data.writeReq.pathPtr,
                               pathPtr,
                               sizeof(requestPtr->data.writeReq.pathPtr),
                               NULL) == LE_OK);

        requestPtr->data.writeReq.value.AsBool = value;

        QueueRequest(tdb_GetRequestQueue(treeRef), requestPtr);
    }
    else
    {
        ni_IteratorRef_t iteratorRef = ni_CreateIterator(sessionRef,
                                                         userRef,
                                                         treeRef,
                                                         NI_WRITE,
                                                         pathPtr);

        ni_SetNodeValueBool(iteratorRef, NULL, value);
        ni_Commit(iteratorRef);
        ni_Release(iteratorRef);

        le_cfg_QuickSetBoolRespond(commandRef);
    }
}
