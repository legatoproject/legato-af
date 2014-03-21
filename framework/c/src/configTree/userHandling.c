
// -------------------------------------------------------------------------------------------------
/*
 *  This file holds the code that maintains the request queuing and user information management.
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
 *  These are the types of actions that can be queued against the tree.
 */
// -------------------------------------------------------------------------------------------------
typedef enum
{
    RQ_CREATE_WRITE_TXN,
    RQ_COMMIT_WRITE_TXN,
    RQ_CREATE_READ_TXN,
    RQ_DELETE_TXN,

    RQ_DELETE_NODE,
    RQ_SET_EMPTY,
    RQ_SET_STRING,
    RQ_SET_INT,
    RQ_SET_FLOAT,
    RQ_SET_BOOL
}
RequestType_t;




// -------------------------------------------------------------------------------------------------
/**
 *  Request structure, if the user's request on the DB can't be handled right away it is stored in
 *  this structure for later handling.
 */
// -------------------------------------------------------------------------------------------------
typedef struct
{
    RequestType_t type;                        ///< Request id.

    UserInfo_t* userPtr;                       ///< User requesting the processing.
    TreeInfo_t* treePtr;                       ///< The tree to be operated on.

    le_msg_SessionRef_t sessionRef;            ///< The context for the session the message came in
                                               ///<   on.
    le_cfg_Context_t contextRef;               ///< Message context for the request.

    union
    {
        struct
        {
            char* pathPtr;                     ///< Initial path for the requested iterator.
        }
        createTxn;                             ///< Create new transaction info.

        struct
        {
            le_cfg_iteratorRef_t iteratorRef;  ///< Ref to the iterator to commit.
        }
        commitTxn;

        struct
        {
            le_cfg_iteratorRef_t iteratorRef;  ///< Ref to the iterator to commit.
        }
        deleteTxn;

        struct
        {
            char* pathPtr;                     ///< Path to the value to operate on.

            union
            {
                char* AsString;
                int AsInt;
                float AsFloat;
                bool AsBool;
            }
            value;
        }
        writeReq;
    }
    data;

    le_sls_Link_t link;                        ///< Link to the next request in the queue.
}
UpdateRequest_t;




// The collection of configuration trees managed by the system.
le_hashmap_Ref_t UserCollectionRef = NULL;
le_mem_PoolRef_t UserPoolRef = NULL;

#define CFG_USER_COLLECTION_NAME "configTree.userCollection"
#define CFG_USER_POOL_NAME       "configTree.userPool"




// -------------------------------------------------------------------------------------------------
/**
 *  Pool that handles config update requests.
 */
// -------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t UpdateRequestPool = NULL;

#define CFG_REQUEST_POOL   "configTree.requestPool"




// -------------------------------------------------------------------------------------------------
/**
 *  Create a new user information block, complete with that users name, id, and default tree name.
 */
// -------------------------------------------------------------------------------------------------
static UserInfo_t* CreateUserInfo
(
    uid_t userId,          ///< The linux Id of the user in question.
    const char* userName,  ///< The name of the user.
    const char* treeName   ///< The name of the default tree for this user.
)
// -------------------------------------------------------------------------------------------------
{
    UserInfo_t* userPtr = le_mem_ForceAlloc(UserPoolRef);

    userPtr->userId = userId;
    strncpy(userPtr->userName, userName, MAX_USER_NAME);
    strncpy(userPtr->treeName, treeName, MAX_USER_NAME);

    // TODO: Load tree permissions, and possibly default tree name.

    LE_ASSERT(le_hashmap_Put(UserCollectionRef, userPtr->userName, userPtr) == NULL);

    return userPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Look-up a user's information based on a given user name.
 *
 *  @return A pointer to a user information block, NULL if not found.
 */
// -------------------------------------------------------------------------------------------------
static UserInfo_t* GetUserFromName
(
    const char* userName  ///< The user name to search for.
)
// -------------------------------------------------------------------------------------------------
{
    UserInfo_t* userPtr = le_hashmap_Get(UserCollectionRef, userName);

    return userPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Look-up a user based on a given user Id.  If the user doesn't exist, a information block will be
 *  created.
 *
 *  @return A pointer to the found, or newly created user information block.
 */
// -------------------------------------------------------------------------------------------------
static UserInfo_t* GetUser
(
    uid_t userId  ///< The user to search for.
)
// -------------------------------------------------------------------------------------------------
{
    char userName[MAX_USER_NAME];
    UserInfo_t* userPtr = NULL;

    // At this point, grab the user's app name, if it is an app, otherwise we get the standard
    // user name.
    if (   (user_GetAppName(userId, userName, MAX_USER_NAME) == LE_OK)
        || (user_GetName(userId, userName, MAX_USER_NAME) == LE_OK))
    {
        userPtr = GetUserFromName(userName);

        if (userPtr == NULL)
        {
            userPtr = CreateUserInfo(userId, userName, userName);
        }
    }

    return userPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Extract the tre name from the specified path.
 */
// -------------------------------------------------------------------------------------------------
static void CopyNameFromPath
(
    char* destPtr,       ///< The destination buffer for the tree name.
    const char* pathPtr  ///< The path to read.
)
// -------------------------------------------------------------------------------------------------
{
    char* posPtr = strchr(pathPtr, ':');

    if (posPtr == NULL)
    {
        return;
    }


    size_t count = posPtr - pathPtr;

    if (count > CFG_MAX_TREE_NAME)
    {
        count = CFG_MAX_TREE_NAME;
    }

    strncpy(destPtr, pathPtr, count);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Create a new request block.
 */
// -------------------------------------------------------------------------------------------------
static UpdateRequest_t* NewRequestBlock
(
    RequestType_t type,              ///< The type of request to create.
    UserInfo_t* userPtr,             ///< Create the request for this user.
    TreeInfo_t* treePtr,             ///< The tree to use in the request.
    le_msg_SessionRef_t sessionRef,  ///< Session that the request came in on.
    le_cfg_Context_t contextRef      ///< The context to reply on.
)
// -------------------------------------------------------------------------------------------------
{
    UpdateRequest_t* requestPtr = le_mem_ForceAlloc(UpdateRequestPool);

    memset(requestPtr, 0, sizeof(UpdateRequest_t));

    requestPtr->type = type;
    requestPtr->userPtr = userPtr;
    requestPtr->treePtr = treePtr;
    requestPtr->sessionRef = sessionRef;
    requestPtr->contextRef = contextRef;
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
    UpdateRequest_t* requestPtr  ///< The request to free.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Releasing request block <%p>.", requestPtr);

    switch (requestPtr->type)
    {
        case RQ_CREATE_WRITE_TXN:
            sb_Release(requestPtr->data.createTxn.pathPtr);
            break;

        case RQ_COMMIT_WRITE_TXN:
            break;

        case RQ_DELETE_TXN:
            break;

        case RQ_CREATE_READ_TXN:
            sb_Release(requestPtr->data.createTxn.pathPtr);
            break;

        case RQ_DELETE_NODE:
            sb_Release(requestPtr->data.writeReq.pathPtr);
            break;

        case RQ_SET_EMPTY:
            sb_Release(requestPtr->data.writeReq.pathPtr);
            break;

        case RQ_SET_STRING:
            sb_Release(requestPtr->data.writeReq.pathPtr);
            sb_Release(requestPtr->data.writeReq.value.AsString);
            break;

        case RQ_SET_INT:
            sb_Release(requestPtr->data.writeReq.pathPtr);
            break;

        case RQ_SET_FLOAT:
            sb_Release(requestPtr->data.writeReq.pathPtr);
            break;

        case RQ_SET_BOOL:
            sb_Release(requestPtr->data.writeReq.pathPtr);
            break;
    }

    le_mem_Release(requestPtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Queue a generic request object for later processing.
 */
// -------------------------------------------------------------------------------------------------
static void QueueRequest
(
    le_sls_List_t* listPtr,      ///< The tree the request is on.
    UpdateRequest_t* requestPtr  ///< THe actual request info itself.
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
    UserInfo_t* userPtr,              ///< The user that requested this.
    TreeInfo_t* treePtr,              ///< The tree we're working on.
    le_msg_SessionRef_t sessionRef,   ///< The user session this request occured on.
    le_cfg_Context_t contextRef,      ///< Context for this request.
    IteratorType_t iteratorType,      ///< The kind of iterator to create.
    const char* basePathPtr           ///< The initial path for the iterator.
)
// -------------------------------------------------------------------------------------------------
{
    RequestType_t type = iteratorType == ITY_READ ? RQ_CREATE_READ_TXN : RQ_CREATE_WRITE_TXN;
    UpdateRequest_t* requestPtr = NewRequestBlock(type, userPtr, treePtr, sessionRef, contextRef);

    requestPtr->data.createTxn.pathPtr = sb_NewCopy(basePathPtr);

    QueueRequest(&treePtr->requestList, requestPtr);
}



// -------------------------------------------------------------------------------------------------
/**
 *  Queue a commit iterator request for later processing.
 */
// -------------------------------------------------------------------------------------------------
static void QueueCommitTxnRequest
(
    UserInfo_t* userPtr,              ///< The user that requested this.
    TreeInfo_t* treePtr,              ///< The tree we're working on.
    le_msg_SessionRef_t sessionRef,   ///< The user session this request occured on.
    le_cfg_Context_t contextRef,      ///< Context for this request.
    le_cfg_iteratorRef_t iteratorRef  ///< The iterator that's being comitted.
)
// -------------------------------------------------------------------------------------------------
{
    UpdateRequest_t* requestPtr = NewRequestBlock(RQ_COMMIT_WRITE_TXN,
                                                  userPtr,
                                                  treePtr,
                                                  sessionRef,
                                                  contextRef);

    requestPtr->data.commitTxn.iteratorRef = iteratorRef;

    QueueRequest(&treePtr->requestList, requestPtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Queue a request to delete an iterator and it's transaction.
 */
// -------------------------------------------------------------------------------------------------
static void QueueDeleteTxnRequest
(
    le_cfg_iteratorRef_t iteratorRef,  ///< External safe refernce to the iterator to delete.
    IteratorInfo_t* iteratorPtr,       ///< The underlying iterator pointer.
    le_sls_List_t* listPtr             ///< The queue to queue this request to.
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
                                                  GetUser(iteratorPtr->userId),
                                                  iteratorPtr->treePtr,
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
    le_sls_List_t* listPtr,                ///< Process any pending requests in this list.
    le_msg_SessionRef_t ignoreSessionRef   ///< Throw away any requests that occured on this
                                           ///<   session.
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
            LE_DEBUG("Dropping orphaned request block, from user %u (%s) on tree '%s'.",
                     requestPtr->userPtr->userId,
                     requestPtr->userPtr->userName,
                     requestPtr->treePtr->name);
            continue;
        }

        LE_DEBUG("** Process request block <%p>.", requestPtr);

        switch (requestPtr->type)
        {
            case RQ_CREATE_WRITE_TXN:
                LE_DEBUG("Starting deferred write txn for user %u (%s) on tree '%s'.",
                         requestPtr->userPtr->userId,
                         requestPtr->userPtr->userName,
                         requestPtr->treePtr->name);
                HandleCreateTxnRequest(requestPtr->userPtr,
                                       requestPtr->treePtr,
                                       requestPtr->sessionRef,
                                       requestPtr->contextRef,
                                       ITY_WRITE,
                                       requestPtr->data.createTxn.pathPtr);
                break;

            case RQ_COMMIT_WRITE_TXN:
                LE_DEBUG("Committing deferred write txn for user %u (%s) on tree '%s'.",
                         requestPtr->userPtr->userId,
                         requestPtr->userPtr->userName,
                         requestPtr->treePtr->name);
                HandleCommitTxnRequest(requestPtr->userPtr,
                                       requestPtr->sessionRef,
                                       requestPtr->contextRef,
                                       requestPtr->data.commitTxn.iteratorRef);
                break;

            case RQ_CREATE_READ_TXN:
                LE_DEBUG("Starting deferred read txn for user %u (%s) on tree '%s'.",
                         requestPtr->userPtr->userId,
                         requestPtr->userPtr->userName,
                         requestPtr->treePtr->name);
                HandleCreateTxnRequest(requestPtr->userPtr,
                                       requestPtr->treePtr,
                                       requestPtr->sessionRef,
                                       requestPtr->contextRef,
                                       ITY_READ,
                                       requestPtr->data.createTxn.pathPtr);
                break;

            case RQ_DELETE_TXN:
                LE_DEBUG("Handling deferred iterator delete for user %u (%s) on tree '%s'.",
                         requestPtr->userPtr->userId,
                         requestPtr->userPtr->userName,
                         requestPtr->treePtr->name);
                HandleDeleteTxnRequest(requestPtr->userPtr,
                                       requestPtr->contextRef,
                                       requestPtr->data.deleteTxn.iteratorRef);
                break;

            case RQ_DELETE_NODE:
                LE_DEBUG("Processing deferred quick delete for user %u (%s) on tree '%s'.",
                         requestPtr->userPtr->userId,
                         requestPtr->userPtr->userName,
                         requestPtr->treePtr->name);
                HandleQuickDeleteNode(requestPtr->userPtr,
                                      requestPtr->treePtr,
                                      requestPtr->sessionRef,
                                      requestPtr->contextRef,
                                      requestPtr->data.writeReq.pathPtr);
                break;

            case RQ_SET_EMPTY:
                LE_DEBUG("Processing deferred quick 'set empty' for user %u (%s) on tree '%s'.",
                         requestPtr->userPtr->userId,
                         requestPtr->userPtr->userName,
                         requestPtr->treePtr->name);
                HandleQuickSetEmpty(requestPtr->userPtr,
                                    requestPtr->treePtr,
                                    requestPtr->sessionRef,
                                    requestPtr->contextRef,
                                    requestPtr->data.writeReq.pathPtr);
                break;

            case RQ_SET_STRING:
                LE_DEBUG("Processing deferred quick 'set string' for user %u (%s) on tree '%s'.",
                         requestPtr->userPtr->userId,
                         requestPtr->userPtr->userName,
                         requestPtr->treePtr->name);
                HandleQuickSetString(requestPtr->userPtr,
                                     requestPtr->treePtr,
                                     requestPtr->sessionRef,
                                     requestPtr->contextRef,
                                     requestPtr->data.writeReq.pathPtr,
                                     requestPtr->data.writeReq.value.AsString);
                break;

            case RQ_SET_INT:
                LE_DEBUG("Processing deferred quick 'set int' for user %u (%s) on tree '%s'.",
                         requestPtr->userPtr->userId,
                         requestPtr->userPtr->userName,
                         requestPtr->treePtr->name);
                HandleQuickSetInt(requestPtr->userPtr,
                                  requestPtr->treePtr,
                                  requestPtr->sessionRef,
                                  requestPtr->contextRef,
                                  requestPtr->data.writeReq.pathPtr,
                                  requestPtr->data.writeReq.value.AsInt);
                break;

            case RQ_SET_FLOAT:
                LE_DEBUG("Processing deferred quick 'set float' for user %u (%s) on tree '%s'.",
                         requestPtr->userPtr->userId,
                         requestPtr->userPtr->userName,
                         requestPtr->treePtr->name);
                HandleQuickSetFloat(requestPtr->userPtr,
                                    requestPtr->treePtr,
                                    requestPtr->sessionRef,
                                    requestPtr->contextRef,
                                    requestPtr->data.writeReq.pathPtr,
                                    requestPtr->data.writeReq.value.AsFloat);
                break;

            case RQ_SET_BOOL:
                LE_DEBUG("Processing deferred quick 'set bool' for user %u (%s) on tree '%s'.",
                         requestPtr->userPtr->userId,
                         requestPtr->userPtr->userName,
                         requestPtr->treePtr->name);
                HandleQuickSetBool(requestPtr->userPtr,
                                   requestPtr->treePtr,
                                   requestPtr->sessionRef,
                                   requestPtr->contextRef,
                                   requestPtr->data.writeReq.pathPtr,
                                   requestPtr->data.writeReq.value.AsBool);
                break;
        }

        ReleaseRequestBlock(requestPtr);
        linkPtr = le_sls_Pop(&list);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Called to check to see if one of the quick set funcions can be handled now, or if it has to be
 *  queueed for later.
 */
// -------------------------------------------------------------------------------------------------
static bool CanQuickSet
(
    TreeInfo_t* treePtr  ///< the tree to check.
)
// -------------------------------------------------------------------------------------------------
{
    if (   (treePtr->activeReadCount > 0)
        || (treePtr->activeWriteIterPtr != NULL))
    {
        return false;
    }

    return true;
}




// -------------------------------------------------------------------------------------------------
/**
 *  This function is called for each open iterator on a given session object.  This function simply
 *  queues that iterator for later deletion.
 *
 *  The actual delete doesn't happen now because it is not safe to remove items from the underlying
 *  collection while it is being iterated.
 */
// -------------------------------------------------------------------------------------------------
static void OnIteratorSessionClosed
(
    le_cfg_iteratorRef_t iteratorRef,  ///< Safe reference to the iterator to delete.
    IteratorInfo_t* iteratorPtr,       ///< The underlying iterator pointer.
    void* contextPtr                   ///< A pointer to the list we're to queue deletions to.
)
// -------------------------------------------------------------------------------------------------
{
    LE_WARN("**** Closing orphaned iterator.");
    QueueDeleteTxnRequest(iteratorRef, iteratorPtr, (le_sls_List_t*)contextPtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Internal handler called whenever a client disconnects from the configTree server.  This handler
 *  will look for any iterators that were opened by that collection and queue them for deletion.
 */
// -------------------------------------------------------------------------------------------------
static void HandleConfigClientClose
(
    le_msg_SessionRef_t sessionRef,  ///< Reference to the session that's going away.
    void* contextPtr                 ///< Our callback context.  Configured as NULL.
)
// -------------------------------------------------------------------------------------------------
{
    le_sls_List_t list = LE_SLS_LIST_INIT;


    // Grab all open iterators attached to this session and queue them to close.  Once that's done
    // process that request queue.
    itr_ForEachIterForSession(sessionRef, OnIteratorSessionClosed, &list);
    ProcessRequestQueue(&list, NULL);


    // At this point, iterators have been removed from the system...  Process any request backlogs
    // on the open trees.  However, ignore any requests that originated from the closed session.
    tdb_IterRef_t iterRef = tdb_GetTreeIterator();

    while (tdb_NextNode(iterRef) == LE_OK)
    {
        ProcessRequestQueue(&tdb_iter_GetTree(iterRef)->requestList, sessionRef);
    }


    // TODO: Release user info if all sessions for that user have closed.
}




// -------------------------------------------------------------------------------------------------
/**
 *  Initialize the memory pools needed by this subsystem.
 */
// -------------------------------------------------------------------------------------------------
void UserTreeInit
(
)
// -------------------------------------------------------------------------------------------------
{
    UpdateRequestPool = le_mem_CreatePool(CFG_REQUEST_POOL, sizeof(UpdateRequest_t));

    UserPoolRef = le_mem_CreatePool(CFG_USER_POOL_NAME, sizeof(UserInfo_t));
    UserCollectionRef = le_hashmap_Create(CFG_USER_COLLECTION_NAME,
                                          31,
                                          le_hashmap_HashString,
                                          le_hashmap_EqualsString);

    CreateUserInfo(0, "root", "system");
    le_msg_SetServiceCloseHandler(le_cfg_GetServiceRef(), HandleConfigClientClose, NULL);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the info for the user for this connection.
 */
// -------------------------------------------------------------------------------------------------
UserInfo_t* GetCurrentUserInfo
(
)
// -------------------------------------------------------------------------------------------------
{
    uid_t userId;

    // Look up the user id of the requesting connection...
    le_msg_SessionRef_t currentSession = le_cfg_GetClientSessionRef();

    if (currentSession == 0)
    {
        currentSession = le_cfgAdmin_GetClientSessionRef();
    }

    LE_FATAL_IF(currentSession == 0,
                "GetCurrentUserInfo must be called within an active message session.");

    LE_FATAL_IF(le_msg_GetClientUserId(currentSession, &userId) == LE_CLOSED,
                "GetCurrentUserInfo must be called within an active connection.");


    // Now that we have a user ID, let's see if we can look them up.
    UserInfo_t* userPtr = NULL;

    // If the connected user is root, or if the connected user has the same uid we're running under
    // look, up the root user info.
    if (   (userId == 0)
        || (userId == geteuid()))
    {
        userPtr = le_hashmap_Get(UserCollectionRef, "root");
        LE_ASSERT(userPtr != NULL);
    }
    else
    {
        userPtr = GetUser(userId);
    }

    if (userPtr != NULL)
    {
        LE_DEBUG("---- Found user: %p, %s - %s", userPtr, userPtr->userName, userPtr->treeName);
    }
    else
    {
        LE_DEBUG("---- User information could not be retreived.")
    }

    return userPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check the path to see if the user specified a specific tree to use.
 */
// -------------------------------------------------------------------------------------------------
bool PathHasTreeSpecifier
(
    const char* pathPtr  ///< The path to read.
)
// -------------------------------------------------------------------------------------------------
{
    return strchr(pathPtr, ':') != NULL;
}




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
)
// -------------------------------------------------------------------------------------------------
{
    if (PathHasTreeSpecifier(pathPtr) == true)
    {
        char treeName[CFG_MAX_TREE_NAME] = { 0 };

        CopyNameFromPath(treeName, pathPtr);

        LE_DEBUG("** Specific tree requested <%s>.", treeName);
        return tdb_GetTree(treeName);
    }

    LE_DEBUG("** Getting user default tree.");
    return tdb_GetTree(userPtr->treeName);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the default tree for the given user.
 */
// -------------------------------------------------------------------------------------------------
TreeInfo_t* GetUserDefaultTree
(
    UserInfo_t* userPtr  ///< The user to read.
)
// -------------------------------------------------------------------------------------------------
{
    return tdb_GetTree(userPtr->treeName);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Return a path pointer that excludes the tree name.  This function does not allocate a new
 *  string but instead returns a pointer into the supplied path string.
 */
// -------------------------------------------------------------------------------------------------
const char* GetPathOnly
(
    const char* pathPtr  ///< The path string.
)
// -------------------------------------------------------------------------------------------------
{
    char* posPtr = strchr(pathPtr, ':');

    if (posPtr == NULL)
    {
        return pathPtr;
    }

    return ++posPtr;
}




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
    le_cfg_Context_t contextRef,      ///< The reply context.
    IteratorType_t iteratorType,      ///< What kind of iterator are we creating?
    const char* basePathPtr           ///< Initial path the iterator is pointed at.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(userPtr != NULL);
    LE_ASSERT(treePtr != NULL);
    LE_ASSERT((iteratorType == ITY_READ) || (iteratorType == ITY_WRITE));

    // If this is a read transaction and there is an active write transaction on this tree waiting
    // to close, don't start the read transaction now...  Queue this read for until after the write
    // has finished.
    if (   (iteratorType == ITY_READ)
        && (treePtr->activeWriteIterPtr != NULL)
        && (itr_IsClosed(treePtr->activeWriteIterPtr)))
    {
        LE_DEBUG("Deferring read txn for user %u (%s) on tree '%s'. User %u is writing.",
                 userPtr->userId,
                 userPtr->userName,
                 treePtr->name,
                 treePtr->activeWriteIterPtr->userId);
        QueueCreateTxnRequest(userPtr, treePtr, sessionRef, contextRef, iteratorType, basePathPtr);
    }
    else if (   (iteratorType == ITY_WRITE)
             && (treePtr->activeWriteIterPtr != NULL))
    {
        // Looks like we're trying to crate a write transaction while one is currently in progress.
        // This one needs to wait until the current one is complete.
        LE_DEBUG("Deferring write txn for user %u (%s) on tree '%s'. User %u is writing.",
                 userPtr->userId,
                 userPtr->userName,
                 treePtr->name,
                 treePtr->activeWriteIterPtr->userId);
        QueueCreateTxnRequest(userPtr, treePtr, sessionRef, contextRef, iteratorType, basePathPtr);
    }
    else
    {
        // Ok, there aren't any roadblocks in our way...  Create a new transaction and respond to
        // the caller.
        LE_DEBUG("** Creating a new iterator object.");
        le_cfg_iteratorRef_t iterRef = itr_NewRef(userPtr,
                                                  treePtr,
                                                  sessionRef,
                                                  iteratorType,
                                                  basePathPtr);

        if (iteratorType == ITY_READ)
        {
            le_cfg_CreateReadTxnRespond(contextRef, iterRef);
        }
        else
        {
            le_cfg_CreateWriteTxnRespond(contextRef, iterRef);
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Print information about a given iterator (if DEBUG enabled).
 **/
//--------------------------------------------------------------------------------------------------
static void PrintTreeNameForIter
(
    le_cfg_iteratorRef_t iteratorRef,  ///< The iterator safe ref.
    IteratorInfo_t* iteratorPtr,       ///< The actual underlying iterator pointer.
    void* contextPtr                   ///< User context pointer.
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("    Iterator <%p>: User %u, session %p, txn type %s",
             iteratorRef,
             iteratorPtr->userId,
             iteratorPtr->sessionRef,
             itr_TxnTypeString(iteratorPtr->type));
}


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
)
// -------------------------------------------------------------------------------------------------
{
    IteratorInfo_t* iteratorPtr = itr_GetPtr(userPtr, iteratorRef);
    TreeInfo_t* treePtr = itr_GetTree(iteratorPtr);

    LE_ASSERT(iteratorPtr->type == ITY_WRITE);

    // Check to make sure that there aren't any active reads on the tree.
    if (treePtr->activeReadCount == 0)
    {
        // The tree is open, so commit our write iterator now.  Then try to act on any queued up
        // requests the tree may still have.
        LE_DEBUG("Committing write txn for user %u (%s).", userPtr->userId, userPtr->userName);

        le_cfg_CommitWriteRespond(contextRef, itr_Commit(userPtr, iteratorRef));
        ProcessRequestQueue(&treePtr->requestList, NULL);
    }
    else
    {
        // Looks like there are still active users of the tree so queue this request up for later.
        LE_DEBUG("Deferring write txn for user %u (%s) on tree '%s'. Users are reading.",
                 userPtr->userId,
                 userPtr->userName,
                 treePtr->name);
        itr_ForEachIterForTree(treePtr, PrintTreeNameForIter, NULL);
        QueueCommitTxnRequest(userPtr, treePtr, sessionRef, contextRef, iteratorRef);
    }
}



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
)
// -------------------------------------------------------------------------------------------------
{
    IteratorInfo_t* iteratorPtr = itr_GetPtr(userPtr, iteratorRef);
    TreeInfo_t* treePtr = itr_GetTree(iteratorPtr);

    LE_DEBUG("Cancelling %s txn for user %u (%s).",
             itr_TxnTypeString(iteratorPtr->type),
             userPtr->userId,
             userPtr->userName);

    itr_Release(userPtr, iteratorRef);

    // If the context ref is NULL then we know this was due to an internal request...  Ie a client
    // session closed with active iterators.  If so, then there's no need to respond to the client.
    if (contextRef != NULL)
    {
        le_cfg_DeleteIteratorRespond(contextRef);
    }

    // Try to handle the tree's backlog.  (If any.)
    ProcessRequestQueue(&treePtr->requestList, NULL);
}




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
)
// -------------------------------------------------------------------------------------------------
{
    if (CanQuickSet(treePtr) == false)
    {
        UpdateRequest_t* requestPtr = NewRequestBlock(RQ_DELETE_NODE,
                                                      userPtr,
                                                      treePtr,
                                                      sessionRef,
                                                      contextRef);

        requestPtr->data.writeReq.pathPtr = sb_NewCopy(pathPtr);

        QueueRequest(&treePtr->requestList, requestPtr);
    }
    else
    {
        LE_DEBUG("** Handling quick delete.");

        tdb_NodeRef_t nodePtr = tdb_GetNode(treePtr->rootNodeRef, pathPtr, false);

        if (nodePtr == NULL)
        {
            le_cfg_QuickDeleteNodeRespond(contextRef, LE_NOT_PERMITTED);
        }
        else
        {
            tdb_DeleteNode(nodePtr);
            tdb_CommitTree(treePtr);

            le_cfg_QuickDeleteNodeRespond(contextRef, LE_OK);
        }
    }
}




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
)
// -------------------------------------------------------------------------------------------------
{
    if (CanQuickSet(treePtr) == false)
    {
        UpdateRequest_t* requestPtr = NewRequestBlock(RQ_SET_EMPTY,
                                                      userPtr,
                                                      treePtr,
                                                      sessionRef,
                                                      contextRef);

        requestPtr->data.writeReq.pathPtr = sb_NewCopy(pathPtr);

        QueueRequest(&treePtr->requestList, requestPtr);
    }
    else
    {
        LE_DEBUG("** Handling quick set empty.");

        tdb_NodeRef_t nodePtr = tdb_GetNode(treePtr->rootNodeRef, pathPtr, false);

        if (nodePtr == NULL)
        {
            LE_DEBUG("** Could not get requested node.");
            le_cfg_QuickSetEmptyRespond(contextRef, LE_NOT_PERMITTED);
        }
        else
        {
            LE_DEBUG("** Handling node clear.");

            tdb_ClearNode(nodePtr);
            tdb_CommitTree(treePtr);

            le_cfg_QuickSetEmptyRespond(contextRef, LE_OK);
        }
    }
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Handling quick get string.");
    tdb_NodeRef_t nodePtr = tdb_GetNode(treePtr->rootNodeRef, pathPtr, false);
    le_result_t result = LE_OK;
    char* valuePtr = sb_Get();

    if (   (nodePtr != NULL)
        && (tdb_IsDeleted(nodePtr) == false))
    {
        tdb_GetAsString(nodePtr, valuePtr, SB_SIZE);
        LE_DEBUG("** Value <%s>.", valuePtr);

        if ((strnlen(valuePtr, SB_SIZE) + 1) > maxString)
        {
            LE_DEBUG("** Value overflow.");
            result = LE_OVERFLOW;
            valuePtr[maxString] = 0;
        }
    }
    else
    {
        LE_DEBUG("** Node not found.");
        result = LE_NOT_PERMITTED;
    }

    le_cfg_QuickGetStringRespond(contextRef, result, valuePtr);
    sb_Release(valuePtr);
}




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
)
// -------------------------------------------------------------------------------------------------
{
    if (CanQuickSet(treePtr) == false)
    {
        UpdateRequest_t* requestPtr = NewRequestBlock(RQ_SET_STRING,
                                                      userPtr,
                                                      treePtr,
                                                      sessionRef,
                                                      contextRef);

        requestPtr->data.writeReq.pathPtr = sb_NewCopy(pathPtr);
        requestPtr->data.writeReq.value.AsString = sb_NewCopy(valuePtr);

        QueueRequest(&treePtr->requestList, requestPtr);
    }
    else
    {
        LE_DEBUG("** Handling quick set string.");
        tdb_NodeRef_t nodePtr = tdb_GetNode(treePtr->rootNodeRef, pathPtr, true);

        if (nodePtr == NULL)
        {
            LE_DEBUG("** Node not found.");
            le_cfg_QuickSetStringRespond(contextRef, LE_NOT_PERMITTED);
        }
        else
        {
            LE_DEBUG("** Setting value <%s>.", valuePtr);
            tdb_SetAsString(nodePtr, valuePtr);
            tdb_CommitTree(treePtr);

            le_cfg_QuickSetStringRespond(contextRef, LE_OK);
        }
    }
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Handling quick get int.");
    tdb_NodeRef_t nodePtr = tdb_GetNode(treePtr->rootNodeRef, pathPtr, false);
    le_result_t result = LE_OK;
    int value = 0;

    if (   (nodePtr != NULL)
        && (tdb_IsDeleted(nodePtr) == false))
    {
        value = tdb_GetAsInt(nodePtr);
        LE_DEBUG("** Value <%d>.", value);
    }
    else
    {
        LE_DEBUG("** Node not found.");
        result = LE_NOT_PERMITTED;
    }

    le_cfg_QuickGetIntRespond(contextRef, result, value);
}




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
)
// -------------------------------------------------------------------------------------------------
{
    if (CanQuickSet(treePtr) == false)
    {
        UpdateRequest_t* requestPtr = NewRequestBlock(RQ_SET_INT,
                                                      userPtr,
                                                      treePtr,
                                                      sessionRef,
                                                      contextRef);

        requestPtr->data.writeReq.pathPtr = sb_NewCopy(pathPtr);
        requestPtr->data.writeReq.value.AsInt = value;

        QueueRequest(&treePtr->requestList, requestPtr);
    }
    else
    {
        tdb_NodeRef_t nodePtr = tdb_GetNode(treePtr->rootNodeRef, pathPtr, true);

        if (nodePtr == NULL)
        {
            le_cfg_QuickSetIntRespond(contextRef, LE_NOT_PERMITTED);
        }
        else
        {
            tdb_SetAsInt(nodePtr, value);
            tdb_CommitTree(treePtr);

            le_cfg_QuickSetIntRespond(contextRef, LE_OK);
        }
    }
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Handling quick get float.");
    tdb_NodeRef_t nodePtr = tdb_GetNode(treePtr->rootNodeRef, pathPtr, false);
    le_result_t result = LE_OK;
    float value = 0.0f;

    if (   (nodePtr != NULL)
        && (tdb_IsDeleted(nodePtr) == false))
    {
        value = tdb_GetAsFloat(nodePtr);
        LE_DEBUG("** Value <%F>.", value);
    }
    else
    {
        LE_DEBUG("** Node not found.");
        result = LE_NOT_PERMITTED;
    }

    le_cfg_QuickGetFloatRespond(contextRef, result, value);
}




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
)
// -------------------------------------------------------------------------------------------------
{
    if (CanQuickSet(treePtr) == false)
    {
        UpdateRequest_t* requestPtr = NewRequestBlock(RQ_SET_FLOAT,
                                                      userPtr,
                                                      treePtr,
                                                      sessionRef,
                                                      contextRef);

        requestPtr->data.writeReq.pathPtr = sb_NewCopy(pathPtr);
        requestPtr->data.writeReq.value.AsFloat = value;

        QueueRequest(&treePtr->requestList, requestPtr);
    }
    else
    {
        tdb_NodeRef_t nodePtr = tdb_GetNode(treePtr->rootNodeRef, pathPtr, true);

        if (nodePtr == NULL)
        {
            le_cfg_QuickSetFloatRespond(contextRef, LE_NOT_PERMITTED);
        }
        else
        {
            tdb_SetAsFloat(nodePtr, value);
            tdb_CommitTree(treePtr);

            le_cfg_QuickSetFloatRespond(contextRef, LE_OK);
        }
    }
}




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
)
// -------------------------------------------------------------------------------------------------
{
    tdb_NodeRef_t nodePtr = tdb_GetNode(treePtr->rootNodeRef, pathPtr, false);
    le_result_t result = LE_OK;
    bool value = false;

    if (   (nodePtr != NULL)
        && (tdb_IsDeleted(nodePtr) == false))
    {
        value = tdb_GetAsBool(nodePtr);
    }
    else
    {
        result = LE_NOT_PERMITTED;
    }

    le_cfg_QuickGetBoolRespond(contextRef, result, value);
}




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
)
// -------------------------------------------------------------------------------------------------
{
    if (CanQuickSet(treePtr) == false)
    {
        UpdateRequest_t* requestPtr = NewRequestBlock(RQ_SET_BOOL,
                                                      userPtr,
                                                      treePtr,
                                                      sessionRef,
                                                      contextRef);

        requestPtr->data.writeReq.pathPtr = sb_NewCopy(pathPtr);
        requestPtr->data.writeReq.value.AsBool = value;

        QueueRequest(&treePtr->requestList, requestPtr);
    }
    else
    {
        tdb_NodeRef_t nodePtr = tdb_GetNode(treePtr->rootNodeRef, pathPtr, true);

        if (nodePtr == NULL)
        {
            le_cfg_QuickSetBoolRespond(contextRef, LE_NOT_PERMITTED);
        }
        else
        {
            tdb_SetAsBool(nodePtr, value);
            tdb_CommitTree(treePtr);

            le_cfg_QuickSetBoolRespond(contextRef, LE_OK);
        }
    }
}




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
)
// -------------------------------------------------------------------------------------------------
{
    // Not implemented.
    return NULL;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Remove a previously registered handler from the tree..
 */
// -------------------------------------------------------------------------------------------------
void HandleRemoveChangeHandler
(
    le_cfg_ChangeHandlerRef_t changeHandlerRef  ///<
)
// -------------------------------------------------------------------------------------------------
{
    // Not implemented.
}
