
// -------------------------------------------------------------------------------------------------
/*
 *  Collection of types used internally by the configTree.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#ifndef CFG_INTERNAL_CONFIG_TYPES_INCLUDE_GUARD
#define CFG_INTERNAL_CONFIG_TYPES_INCLUDE_GUARD




//
#define MAX_USER_NAME 50


//
#define CFG_MAX_TREE_NAME 50


//
typedef struct tdb_Node_t* tdb_NodeRef_t;





// -------------------------------------------------------------------------------------------------
/*
 *  .
 */
// -------------------------------------------------------------------------------------------------
typedef enum
{
    ITY_READ,  ///<
    ITY_WRITE  ///<
}
IteratorType_t;




// -------------------------------------------------------------------------------------------------
/*
 *  .
 */
// -------------------------------------------------------------------------------------------------
typedef enum
{
    ITER_OK,        ///<
    ITER_TIMEDOUT,  ///<
    ITER_CANCELED,  ///<
    ITER_COMMITTED  ///<
}
IteratorStatus_t;





typedef enum
{
    ITER_GET_NO_DEFAULT_ROOT,
    ITER_GET_DEFAULT_ROOT
}
IteratorGetNodeFlag_t;




struct TreeInfo_t;




// -------------------------------------------------------------------------------------------------
/*
 *  Structure used to keep track of the iteration that occurs within the configTree.
 */
// -------------------------------------------------------------------------------------------------
typedef struct IteratorInfo
{
    uid_t userId;                          ///< Id of the user process that created the iterator.
    le_msg_SessionRef_t sessionRef;        ///< The IPC session the connection occured on.

    IteratorType_t type;                   ///< What kind of iterator is this?  Read or write?
    IteratorStatus_t status;               ///< Is this iterator still ok?

    tdb_NodeRef_t rootNodeRef;             ///< The root node of the tree the iterator is on.
    tdb_NodeRef_t currentNodeRef;          ///< The current node of the tree the iterator is on.

    size_t activeClones;                   ///< Does this iterator have any outstanding clones?
    struct IteratorInfo* baseIteratorPtr;  ///< If this is a cloned iterator, this is a pointer to
                                           ///<   the original.

    struct TreeInfo_t* treePtr;            ///< The tree object that the iterator is working on.
}
IteratorInfo_t;




// -------------------------------------------------------------------------------------------------
/**
 *  Structure used to keep track of the users of the configTree.
 */
// -------------------------------------------------------------------------------------------------
typedef struct
{
    uid_t userId;                  ///< OS id for the user.
    char userName[MAX_USER_NAME];  ///< Human friendly name for the user.

    char treeName[MAX_USER_NAME];  ///< Human friendly name for the user's default tree.
}
UserInfo_t;




struct IteratorInfo;




// -------------------------------------------------------------------------------------------------
/**
 *  Structure used to keep track of the trees loaded in the configTree daemon.
 */
// -------------------------------------------------------------------------------------------------
typedef struct TreeInfo_t
{
    char name[MAX_USER_NAME];

    int revisionId;                           ///< 1, 2, 3

    tdb_NodeRef_t rootNodeRef;                ///< The root node of this tree.

    size_t activeReadCount;                   ///< Count of reads that are currently active on
                                              ///<   this tree.
    struct IteratorInfo* activeWriteIterPtr;  ///< The parent write iterator that's active on
                                              ///<   this tree.  NULL if there are no writes
                                              ///<   pending.

    le_sls_List_t requestList;                ///< Each tree maintains it's own list of pending
                                              ///<   requests.
}
TreeInfo_t;




#endif
