
// -------------------------------------------------------------------------------------------------
/**
 *  @file treeDb.c
 *
 *  Implementation of the low level tree DB structure.  This code also handles the persisting of the
 *  tree db to the filesystem.
 *
 *  The tree structure looks like this:
 *
 * @verbatim

    Shadow Tree ------------+----------+  +------------------------+
                            |          |  |                        |
                            v          v  v                        |
    Tree Collection --*--> Tree --+--> Node --+--> Child List --*--+
                                  |           |
                                  |           +--> Value
                                  |           |
                                  |           +--> Handler List --*--> Handler
                                  |
                                  +--> Request Queue
                                  |
                                  +--> Write Iterator Reference
                                  |
                                  +--> Read Iterator Count

@endverbatim
 *
 *  The Tree Collection holds Tree objects. There's one Tree object for each configuration tree.
 *  They are indexed by tree name.
 *
 *  Each Tree object has a single "root" Node.
 *
 *  Each Node can have either a value or a list of child Nodes.
 *
 *  When a write transaction is started for a Tree, the iterator reference for that transaction
 *  is recorded in the Tree object.  When the transaction is committed or cancelled, that reference
 *  is cleared out.
 *
 *  When a read transaction is started for a Tree, the count of read iterators in that Tree is
 *  incremented.  When it ends, the count is decremented.
 *
 *  When client requests are received that cannot be processed immediately, because of the state
 *  of the tree the request is for (e.g., if a write transaction commit request is received while
 *  there are read transactions in progress on the tree), then the request is queued onto the tree's
 *  Request Queue.
 *
 *  <b>Shadow Trees:</b>
 *
 *  In addition, there's the notion of a "Shadow Tree", which is a tree that contains changes
 *  that have been made to another tree in a write transaction that has not yet been committed.
 *  Each node in a shadow tree is called a "Shadow Node".
 *
 *  When a write transaction is started on a tree, a shadow tree is created for that tree, and
 *  a shadow node is created for the root node.  As a shadow node is traversed (using the normal
 *  tree traversal functions), new shadow nodes are created for any nodes that have been traversed
 *  to and any of their sibling nodes.  When changes are made to a node, the new value is stored
 *  in the shadow node.  When new nodes are added, a new shadow node is created in the shadow
 *  tree.  When nodes are deleted, the shadow node is marked "deleted".
 *
 *  When a write transaction is cancelled, the shadow tree and all its shadow nodes are discarded.
 *
 *  When a write transaction is committed, the shadow tree is traversed, and any changes found
 *  in it are applied to the "original" tree that the shadow tree was shadowing.  This process is
 *  called "merging".
 *
 *  Shadow Trees don't have handlers, request queues, write iterator references or read iterator
 *  counts.
 *
 *  <b>Event Handler Registration:</b>
 *
 *  The config tree allows clients to register callbacks to be notified if certian sections of a
 *  configuration tree is modified.
 *
 *  The way this works is that a global hash map of registrations is maintained.  With the hash being
 *  generated from the path to the tree and node of interest.  So, if an program was interested in
 *  watching the apps collection in the system tree it would use the path:
 *
 *  @verbatim system:/apps @endverbatim
 *
 *  For each unique path a registration object is created, and that registration object will hold a
 *  list of event handlers for the node.
 *
 * @verbatim

    +------------------------+
    | HandlerRegistrationMap |
    +------------------------+
      |
      | Hash of 'system:/apps'  +--------------+
      *------------------------>| Registration |
                                +--------------+
                                    |
                                    |  List of handlers  +---------+
                                    +--------------------| Handler |
                                    |                    +---------+
                                    |                       |
                                    |                       +- Function Pointer
                                    |                       +- Context Pointer
                                    |                       +- Other data...
                                    |
                                    |                    +---------+
                                    +--------------------| Handler |
                                    |                    +---------+
                                    |
                                    .
                                    .
                                    .

 @endverbatim
 *
 *  The system also employs the use of SafeRefs to keep track of each registered handler so that a
 *  handler can quickly and easily remove a handler as required.
 *
 *  When a merge occurs each modified node path is checked against the registration map.  If there
 *  is a registration object for that node each of the registered handlers is invoked.
 *
 *  Handlers are registered in this hash map so that the target node doesn't need to actually exist
 *  in order to have a handler registed for it.  In fact, a handler will be called when a node is
 *  deleted and when it is recreated.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "dynamicString.h"
#include "treePath.h"
#include "treeDb.h"
#include "treeUser.h"
#include "nodeIterator.h"
#include "sysPaths.h"



/// Maximum path size for the config tree.
#define CFG_MAX_PATH_SIZE LE_CFG_STR_LEN_BYTES



/// Maximum size (in bytes) of a "small" string, including the null terminator.
#define SMALL_STR 24




//--------------------------------------------------------------------------------------------------
/**
 * Records the event registration for a given node in a given tree.
 **/
//--------------------------------------------------------------------------------------------------
typedef struct Registration
{
    char registrationPath[CFG_MAX_PATH_SIZE];  ///< Path to the node being watched.  This *must*
                                               ///<   also include the tree name.
    bool triggered;                            ///< Has this registration been triggered for
                                               ///<   callback?

    union
    {
        le_dls_List_t handlerList;   ///< List of handlers to watch the specified node.
        le_sls_Link_t link;          ///< When a client session is destroyed, all of it's handlers
                                     ///<   are automatically removed.  If a registration object is
                                     ///<   determined to be no longer required, this link is used
                                     ///<   to queue the registration object for deletion.
    };
}
Registration_t;




//--------------------------------------------------------------------------------------------------
/**
 *  Change notification handler object structure. (aka "Handler objects")
 *
 *  Each one of these is used to keep track of a client's change notification handler function
 *  registration for a particular tree node.  These are allocated from the Handler Pool and kept
 *  on a Node object's Handler List.
 **/
//--------------------------------------------------------------------------------------------------
typedef struct Handler
{
    le_dls_Link_t link;                     ///< Used to link into into the parent registration
                                            ///<   list.

    le_msg_SessionRef_t sessionRef;         ///< Session that this handler was registered on.

    le_cfg_ChangeHandlerFunc_t handlerPtr;  ///< Function to call back.
    void* contextPtr;                       ///< Context to give the function when called.

    Registration_t* registrationPtr;        ///< The registration object this handler is attached
                                            ///<   to.

    le_cfg_ChangeHandlerRef_t safeRef;      ///< The safe reference to this object.
}
Handler_t;




// -------------------------------------------------------------------------------------------------
/**
 *  Context structure used during handler cleanup.  Handler cleanup happens when a configAPI session
 *  closes and we need to make sure that registered handlers are not leaked.
 *
 *  This structure is passed to the hashmap for each handler so that it can take care of the cleanup
 *  duties.
 */
// -------------------------------------------------------------------------------------------------
typedef struct CleanUpContext
{
    le_msg_SessionRef_t sessionRef;  ///< Reference to the session that had closed.
    le_sls_List_t deleteQueue;       ///< Queue of Registration_t object to release as a result of
                                     ///<   session being closed.
}
CleanUpContext_t;




// -------------------------------------------------------------------------------------------------
/**
 *  Flags that can be set on a node to allow the code to keep track of the various changes as
 *  they're made to the nodes.
 */
// -------------------------------------------------------------------------------------------------
typedef enum
{
    NODE_FLAGS_UNSET = 0x0,  ///< No flags have been set.
    NODE_IS_SHADOW   = 0x1,  ///< The node is a shadow for a node in another tree.
    NODE_IS_MODIFIED = 0x2,  ///< This node has been modified.
    NODE_IS_DELETED  = 0x4   ///< This node has been marked as deleted, the actual deletion will
                             ///<   take place later.
}
NodeFlags_t;




// -------------------------------------------------------------------------------------------------
/**
 *  The Node object structure.
 */
// -------------------------------------------------------------------------------------------------
typedef struct Node
{
    tdb_NodeRef_t parentRef;         ///< The parent node of this one.

    le_cfg_nodeType_t type;          ///< What kind of value does this node hold.

    NodeFlags_t flags;               ///< Various flags set on the node.
    tdb_NodeRef_t shadowRef;         ///< If this node is shadowing another then the pointer to
                                     ///<   that shadowed node is here.

    dstr_Ref_t nameRef;              ///< The name of this node.

    size_t nameHash;                 ///< The hash of the name of this node.

    le_dls_Link_t siblingList;       ///< The linked list of node siblings.  All of the nodes
                                     ///<   in this list have the same parent node.

    union
    {
        dstr_Ref_t valueRef;         ///< The value of the node.  This is only valid if the
                                     ///<   node is not a stem.

        le_dls_List_t children;      ///< The linked list of children belonging to this node.
    }
    info;                            ///< The actual inforation that this node stores.
}
Node_t;




// -------------------------------------------------------------------------------------------------
/**
 *  Structure used to keep track of the trees loaded in the configTree daemon.
 */
// -------------------------------------------------------------------------------------------------
typedef struct Tree
{
    bool isDeletePending;                 ///< If this is set to true, then the tree will be deleted
                                          ///<   once the last iterator has been closed on it.  If
                                          ///<   it is set to false, the tree is left alone.

    struct Tree* originalTreeRef;         ///< If non-NULL then this points back to the original
                                          ///<   tree this one is shadowing.

    char name[MAX_TREE_NAME_BYTES];       ///< The name of this tree.

    int revisionId;                       ///< The current revision,
                                          ///<   0 - Unknonwn.
                                          ///<   1, 2, 3 is one of the rock, paper, scissors revs.

    Node_t* rootNodeRef;                  ///< The root node of this tree.

    ssize_t activeReadCount;              ///< Count of reads that are currently active on
                                          ///<   this tree.
    ni_IteratorRef_t activeWriteIterRef;  ///< The parent write iterator that's active on
                                          ///<   this tree.  NULL if there are no writes
                                          ///<   pending.

    le_sls_List_t requestList;            ///< Each tree maintains it's own list of pending
                                          ///<   requests.
}
Tree_t;




//--------------------------------------------------------------------------------------------------
/**
 * Types of lexical tokens that can be found in configuration data files.
 **/
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TT_EMPTY_VALUE,     ///< Node without any value.
    TT_BOOL_VALUE,      ///< Boolean value.
    TT_INT_VALUE,       ///< Signed integer.
    TT_FLOAT_VALUE,     ///< Floating point number.
    TT_STRING_VALUE,    ///< UTF-8 text string.
    TT_OPEN_GROUP,      ///< Start of grouping.
    TT_CLOSE_GROUP      ///< End of grouping.
}
TokenType_t;


/// Define static pool for nodes
LE_MEM_DEFINE_STATIC_POOL(nodePool, LE_CONFIG_CFGTREE_MAX_NODE_POOL_SIZE, sizeof(Node_t));

/// The memory pool responsible for tree nodes.
static le_mem_PoolRef_t NodePoolRef = NULL;


/// Define static memory for collection of configuration trees managed by the system
LE_HASHMAP_DEFINE_STATIC(TreeCollection, LE_CONFIG_CFGTREE_MAX_TREE_POOL_SIZE);

/// The collection of configuration trees managed by the system.
static le_hashmap_Ref_t TreeCollectionRef = NULL;

/// Define static pool for trees
LE_MEM_DEFINE_STATIC_POOL(treePool, LE_CONFIG_CFGTREE_MAX_TREE_POOL_SIZE, sizeof(Tree_t));

/// Pool from which Tree objects are allocated.
static le_mem_PoolRef_t TreePoolRef = NULL;


/// Define static pool for handlers
LE_MEM_DEFINE_STATIC_POOL(HandlerPool, LE_CONFIG_CFGTREE_MAX_HANDLER_POOL_SIZE,
    sizeof(Handler_t));

/// Pool for registered change handlers.
static le_mem_PoolRef_t HandlerPool = NULL;

/// Static safe ref map for the change handler objects.
LE_REF_DEFINE_STATIC_MAP(HandlerSafeRefMap, LE_CONFIG_CFGTREE_MAX_HANDLER_POOL_SIZE);

/// Safe ref map for the change handler objects.
static le_ref_MapRef_t HandlerSafeRefMap = NULL;

/// Define static memory for handler hashmap
LE_HASHMAP_DEFINE_STATIC(HandlerLookupMap, LE_CONFIG_CFGTREE_MAX_HANDLER_POOL_SIZE);

/// Hash map to keep track of event registrations based on the registered node path.
static le_hashmap_Ref_t HandlerRegistrationMap = NULL;

/// Define static pool for node handler registrations
LE_MEM_DEFINE_STATIC_POOL(RegistrationPool, LE_CONFIG_CFGTREE_MAX_HANDLER_POOL_SIZE,
    sizeof(Registration_t));

/// Pool to handle the node handler registration objects.
static le_mem_PoolRef_t RegistrationPool = NULL;


/// Define static pool for binary data buffers
LE_MEM_DEFINE_STATIC_POOL(BinaryData, LE_CONFIG_CFGTREE_MAX_BINARY_DATA_POOL_SIZE,
    LE_CFG_BINARY_LEN);

/// Pool for the binary data buffers.
le_mem_PoolRef_t BinaryDataPool = NULL;


/// Define static pool for encoded string buffers
LE_MEM_DEFINE_STATIC_POOL(EncodedString, LE_CONFIG_CFGTREE_MAX_ENCODED_STRING_POOL_SIZE,
    TDB_MAX_ENCODED_SIZE);

/// Pool for the encoded string buffers.
le_mem_PoolRef_t EncodedStringPool = NULL;


// -------------------------------------------------------------------------------------------------
/**
 *  Clear all flags from the given node.
 */
// -------------------------------------------------------------------------------------------------
static void ClearFlags
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node to update.
)
// -------------------------------------------------------------------------------------------------
{
    nodeRef->flags = NODE_FLAGS_UNSET;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check to see if this node is in fact a shadow node.
 */
// -------------------------------------------------------------------------------------------------
static bool IsShadow
(
    const tdb_NodeRef_t nodeRef  ///< [IN] The node to check.
)
// -------------------------------------------------------------------------------------------------
{
    return (nodeRef->flags & NODE_IS_SHADOW) != 0;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Set the shadow flag in this node.
 */
// -------------------------------------------------------------------------------------------------
static void SetShadowFlag
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node to update.
)
// -------------------------------------------------------------------------------------------------
{
    nodeRef->flags |= NODE_IS_SHADOW;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check to see if this node has been modified.
 */
// -------------------------------------------------------------------------------------------------
static bool IsModified
(
    const tdb_NodeRef_t nodeRef  ///< [IN] The node to read.
)
// -------------------------------------------------------------------------------------------------
{
    return (nodeRef->flags & NODE_IS_MODIFIED) != 0;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Mark the node as modified.
 */
// -------------------------------------------------------------------------------------------------
static void SetModifiedFlag
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node to update.
)
// -------------------------------------------------------------------------------------------------
{
    nodeRef->flags |= NODE_IS_MODIFIED;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Clear the modified flag.
 */
// -------------------------------------------------------------------------------------------------
static void ClearModifiedFlag
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node to update.
)
// -------------------------------------------------------------------------------------------------
{
    nodeRef->flags &= ~NODE_IS_MODIFIED;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Has the node been marked as deleted?
 */
// -------------------------------------------------------------------------------------------------
static bool IsDeleted
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node to read.
)
// -------------------------------------------------------------------------------------------------
{
    return (nodeRef->flags & NODE_IS_DELETED) != 0;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Set the deleted flag on the node.
 */
// -------------------------------------------------------------------------------------------------
static void SetDeletedFlag
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node to update.
)
// -------------------------------------------------------------------------------------------------
{
    nodeRef->flags |= NODE_IS_DELETED;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Clear the deleted flag on a node.
 */
// -------------------------------------------------------------------------------------------------
static void ClearDeletedFlag
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node to update.
)
// -------------------------------------------------------------------------------------------------
{
    nodeRef->flags &= ~NODE_IS_DELETED;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Allocate a new node and fill out it's default information.
 *
 *  @return The newly created node.
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t NewNode
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    // Create a new blank node.
    tdb_NodeRef_t newNodeRef = le_mem_ForceAlloc(NodePoolRef);

    newNodeRef->parentRef = NULL;
    newNodeRef->type = LE_CFG_TYPE_EMPTY;
    ClearFlags(newNodeRef);
    newNodeRef->shadowRef = NULL;
    newNodeRef->nameRef = NULL;
    newNodeRef->nameHash = 0;
    newNodeRef->siblingList = LE_DLS_LINK_INIT;
    memset(&newNodeRef->info, 0, sizeof(newNodeRef->info));

    return newNodeRef;
}




// -------------------------------------------------------------------------------------------------
/**
 *  The node destructor function.  This will take care of freeing a node's string values and any
 *  children it may have.  Called automaticly by the memory system when a node is released.
 */
// -------------------------------------------------------------------------------------------------
static void NodeDestructor
(
    void* objectPtr  ///< [IN] The generic object to free.
)
// -------------------------------------------------------------------------------------------------
{
    tdb_NodeRef_t nodeRef = (tdb_NodeRef_t)objectPtr;

    if (nodeRef->nameRef)
    {
        dstr_Release(nodeRef->nameRef);
    }

    switch (nodeRef->type)
    {
        case LE_CFG_TYPE_EMPTY:
        case LE_CFG_TYPE_DOESNT_EXIST:
            // Nothing to do here.
            break;

        case LE_CFG_TYPE_STRING:
        case LE_CFG_TYPE_BOOL:
        case LE_CFG_TYPE_INT:
        case LE_CFG_TYPE_FLOAT:
            if (nodeRef->info.valueRef)
            {
                dstr_Release(nodeRef->info.valueRef);
            }
            break;

        case LE_CFG_TYPE_STEM:
            {
                tdb_NodeRef_t childRef = tdb_GetFirstChildNode(nodeRef);

                while (childRef != NULL)
                {
                    tdb_NodeRef_t nextChildRef = tdb_GetNextSiblingNode(childRef);

                    le_mem_Release(childRef);
                    childRef = nextChildRef;
                }
            }
            break;
    }

    if (nodeRef->parentRef != NULL)
    {
        LE_ASSERT(nodeRef->parentRef->type == LE_CFG_TYPE_STEM);
        LE_ASSERT(le_dls_IsEmpty(&nodeRef->parentRef->info.children) == false);
        LE_ASSERT(le_dls_IsInList(&nodeRef->parentRef->info.children, &nodeRef->siblingList));

        le_dls_Remove(&nodeRef->parentRef->info.children, &nodeRef->siblingList);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Allocate a new node from our pool, and turn it into a shadow of an existing node.
 *
 *  @return A new node that shadows the existing node.
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t NewShadowNode
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node to shadow.
)
// -------------------------------------------------------------------------------------------------
{
    // Allocate a new blank node.
    tdb_NodeRef_t newShadowRef = NewNode();

    // Turn it into a shadow of the original node.  It's possible for nodeRef to be NULL.  We could
    // be creating a shadow node for which no original exists.  Which is the case when creating a
    // new path that didn't exist in the original tree.
    if (nodeRef != NULL)
    {
        newShadowRef->type = nodeRef->type;
        newShadowRef->flags = nodeRef->flags;
        newShadowRef->shadowRef = nodeRef;

        // Now, if the parent node, (if there is a parent node,) is marked as deleted, then do the
        // same with this new node.
        if (   (nodeRef->parentRef != NULL)
            && (IsDeleted(nodeRef->parentRef)))
        {
            SetDeletedFlag(newShadowRef);
        }
    }

    SetShadowFlag(newShadowRef);
    return newShadowRef;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Create a new node and insert it into the given node's children collection.
 *
 *  @return The newly create node, already inserted into the supplied node's child collection.
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t NewChildNode
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node to be given with a new child.
)
// -------------------------------------------------------------------------------------------------
{
    // If the node is currently empty, then turn it into a stem.
    if (nodeRef->type == LE_CFG_TYPE_EMPTY)
    {
        nodeRef->type = LE_CFG_TYPE_STEM;
    }

    LE_ASSERT(nodeRef->type == LE_CFG_TYPE_STEM);

    // Create a new node.  Then set it's parent to the given node
    tdb_NodeRef_t newRef = NewNode();

    newRef->parentRef = nodeRef;
    newRef->type = LE_CFG_TYPE_EMPTY;

    // Get the new node to inheret the parent's shadow and deletion flags.
    if (IsShadow(nodeRef))
    {
        SetShadowFlag(newRef);
        SetDeletedFlag(newRef);
    }

    if (IsDeleted(nodeRef))
    {
        SetDeletedFlag(newRef);
    }

    // Now make sure to add the new child node to the end of the parents collection.
    le_dls_Queue(&nodeRef->info.children, &newRef->siblingList);

    // Finally return the newly created node to the caller.
    return newRef;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Called to shadow a node's collection of children.
 */
// -------------------------------------------------------------------------------------------------
static void ShadowChildren
(
    tdb_NodeRef_t shadowParentRef  ///< [IN] The node we're shadowing the children of.
)
// -------------------------------------------------------------------------------------------------
{
    // If the parent node isn't a stem then there isn't much else to do here.
    if (shadowParentRef->type != LE_CFG_TYPE_STEM)
    {
        return;
    }

    // Does this node have any children currently?  If yes, then we don't need to do anything else.
    if (le_dls_IsEmpty(&shadowParentRef->info.children) == false)
    {
        return;
    }

    // Has this node been modified?  If so, then the shadow children may have been cleared from
    // this collection.
    if (IsModified(shadowParentRef) == true)
    {
        return;
    }

    // This node has no shadow children.  So what we do now is check the original node... Does it
    // have any children?  If it does, we simply recreate the whole collection now.  (We do not
    // recurse into the grandchildren though.)  Doing this now makes life simpler, instead of doing
    // this piecemeal and possibly out of order.
    tdb_NodeRef_t originalRef = shadowParentRef->shadowRef;

    if (   (originalRef == NULL)
        || (originalRef->type != LE_CFG_TYPE_STEM))
    {
        return;
    }

    // Simply iterate through the original collection and add a new shadow child to our own
    // collection.
    tdb_NodeRef_t originalChildRef = tdb_GetFirstChildNode(originalRef);

    while (originalChildRef != NULL)
    {
        tdb_NodeRef_t newShadowRef = NewShadowNode(originalChildRef);
        newShadowRef->parentRef = shadowParentRef;

        le_dls_Queue(&shadowParentRef->info.children, &newShadowRef->siblingList);

        originalChildRef = tdb_GetNextSiblingNode(originalChildRef);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Search up through a node tree until we find the root node.
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t GetRootParentNode
(
    tdb_NodeRef_t nodeRef  ///< [IN] Find the greatest grand parent of this node.
)
// -------------------------------------------------------------------------------------------------
{
    tdb_NodeRef_t parentRef = NULL;

    while (nodeRef != NULL)
    {
        parentRef = nodeRef;
        nodeRef = tdb_GetNodeParent(nodeRef);
    }

    return parentRef;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Called to look for a named child in a given node's child collection.
 *
 *  @return Reference to the found child node, or NULL if a node was not found.
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t GetNamedChild
(
    tdb_NodeRef_t nodeRef,  ///< [IN] The node to search.
    const char* nameRef     ///< [IN] The name we're searching for.
)
// -------------------------------------------------------------------------------------------------
{
    // Is this one of the "special" names?
    if (strcmp(nameRef, ".") == 0)
    {
        return nodeRef;
    }

    if (strcmp(nameRef, "..") == 0)
    {
        return nodeRef->parentRef;
    }

    // If the current node isn't a stem, then this node can't have any children.
    if (nodeRef->type != LE_CFG_TYPE_STEM)
    {
        return NULL;
    }

    // Search the child list for a node with the given name.
    tdb_NodeRef_t currentRef = tdb_GetFirstChildNode(nodeRef);
    char currentNameRef[LE_CFG_NAME_LEN_BYTES] = "";
    size_t stringHash = le_hashmap_HashString(nameRef);
    size_t nodeHash;

    while (currentRef != NULL)
    {
        nodeHash = tdb_GetNodeNameHash(currentRef);

        // if the hash doesn't match, the name is different. If the hash matches, there is
        // a small possibility of collision, and the string comparison is required.
        if (stringHash == nodeHash)
        {
            tdb_GetNodeName(currentRef, currentNameRef, sizeof(currentNameRef));

            if (strncmp(currentNameRef, nameRef, sizeof(currentNameRef)) == 0)
            {
                return currentRef;
            }
        }

        currentRef = tdb_GetNextSiblingNode(currentRef);
    }

    // Looks like there was no node to return.
    return NULL;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Called to create a named child in a nodes child collection.  However, this function will only
 *  create nodes on shadow trees.
 *
 *  @return Reference to newly created child node, or NULL if it can not be created.
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t CreateNamedChild
(
    tdb_NodeRef_t nodeRef,  ///< [IN] The node to search.
    const char* nameRef     ///< [IN] The name we're searching for.
)
// -------------------------------------------------------------------------------------------------
{
    tdb_NodeRef_t childRef = NULL;

    if (IsShadow(nodeRef) == true)
    {
        // If the node isn't a stem, then convert it into an empty one now.
        if (   (nodeRef->type != LE_CFG_TYPE_STEM)
            && (nodeRef->type != LE_CFG_TYPE_EMPTY))
        {
            tdb_SetEmpty(nodeRef);
            nodeRef->type = LE_CFG_TYPE_STEM;
            nodeRef->info.children = LE_DLS_LIST_INIT;
        }

        // Create the node, and set it's deleted flag as it hasn't been used for anything yet.
        childRef = NewChildNode(nodeRef);
        SetDeletedFlag(childRef);

        // Set the name of the new node, if that fails then the user had given us a bad name for the
        // new node.  So in that case free the node and return NULL.
        if (tdb_SetNodeName(childRef, nameRef) != LE_OK)
        {
            le_mem_Release(childRef);
            childRef = NULL;
        }
    }

    return childRef;
}


// -------------------------------------------------------------------------------------------------
/**
 *  Check to see if a given node exists within a node's child collection.
 *
 *  @return True if the given node exists within the parent node's collection.  False if not.
 */
// -------------------------------------------------------------------------------------------------
static bool NodeExists
(
    tdb_NodeRef_t parentRef,  ///< [IN] The parent node to search.
    const char* namePtr       ///< [IN] The name to search for.
)
// -------------------------------------------------------------------------------------------------
{
    tdb_NodeRef_t currentRef = tdb_GetFirstChildNode(parentRef);
    char currentName[LE_CFG_NAME_LEN_BYTES] = "";

    while (currentRef != NULL)
    {
        tdb_GetNodeName(currentRef, currentName, sizeof(currentName));

        if (strncmp(currentName, namePtr, sizeof(currentName)) == 0)
        {
            return true;
        }

        currentRef = tdb_GetNextSiblingNode(currentRef);
    }

    return false;
}


// -------------------------------------------------------------------------------------------------
/**
 *  Check the given node type and see if it should have a string value.
 *
 *  @return True if the given node could hold a string value.  False if not.
 */
// -------------------------------------------------------------------------------------------------
static bool IsStringType
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node to read.
)
// -------------------------------------------------------------------------------------------------
{
    le_cfg_nodeType_t type = tdb_GetNodeType(nodeRef);

    if (   (type == LE_CFG_TYPE_STRING)
        || (type == LE_CFG_TYPE_BOOL)
        || (type == LE_CFG_TYPE_INT)
        || (type == LE_CFG_TYPE_FLOAT))
    {
        return true;
    }

    return false;
}




// -------------------------------------------------------------------------------------------------
/**
 *  This function will copy a string value from an original tree node into a node that has shadowed
 *  it.
 */
// -------------------------------------------------------------------------------------------------
static void PropagateValue
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node ref to update.
)
// -------------------------------------------------------------------------------------------------
{
    // If the node doesn't even have a ref to an original node, then there's nothing left to do
    // here.
    tdb_NodeRef_t shadowRef = nodeRef->shadowRef;

    if (shadowRef == NULL)
    {
        return;
    }

    // Ok, figure out the type for this node.  If it has a value, and the original
    if (   (IsStringType(nodeRef) == true)
        && (IsStringType(shadowRef) == true)
        && (nodeRef->info.valueRef == NULL)
        && (shadowRef->info.valueRef != NULL))
    {
        // Looks like the value hasn't been propagated or changed yet.  So, do so now.
        nodeRef->info.valueRef = dstr_NewFromDstr(shadowRef->info.valueRef);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Merge a shadow node with the original it represents.
 */
// -------------------------------------------------------------------------------------------------
static void MergeNode
(
    tdb_NodeRef_t nodeRef  ///< [IN] The shadow node to merge.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    // If this shadow node for some reason doesn't have a ref check for an original version of it in
    // the original tree.  This shadow node may have been destroyed and re-created loosing this
    // link.
    if (nodeRef->shadowRef == NULL)
    {
        tdb_NodeRef_t shadowedParentRef = tdb_GetNodeParent(nodeRef)->shadowRef;

        if (shadowedParentRef != NULL)
        {
            char name[LE_CFG_NAME_LEN_BYTES] = "";

            tdb_GetNodeName(nodeRef, name, sizeof(name));
            nodeRef->shadowRef = GetNamedChild(shadowedParentRef, name);
        }
    }

    // If this node has been marked as deleted, then simply drop the original node and move on.
    if (IsDeleted(nodeRef))
    {
        if (   (nodeRef->shadowRef != NULL)
            && (tdb_GetNodeParent(nodeRef->shadowRef) != NULL))
        {
            le_mem_Release(nodeRef->shadowRef);
        }
        else
        {
            // We delete every node but the root node.  Since this is the root node, we just need
            // to clear it out.
            tdb_SetEmpty(nodeRef->shadowRef);
        }

        return;
    }

    // If the original node doesn't exist, create it now.
    tdb_NodeRef_t originalRef = nodeRef->shadowRef;

    if (originalRef == NULL)
    {
        LE_ASSERT(nodeRef->parentRef != NULL);
        LE_ASSERT(nodeRef->parentRef->shadowRef != NULL);

        nodeRef->shadowRef = originalRef = NewChildNode(nodeRef->parentRef->shadowRef);
    }

    ClearModifiedFlag(originalRef);

    // If the name has been changed, then copy it over now.
    if (dstr_IsNullOrEmpty(nodeRef->nameRef) == false)
    {
        if (originalRef->nameRef != NULL)
        {
            dstr_Copy(originalRef->nameRef, nodeRef->nameRef);
        }
        else
        {
            originalRef->nameRef = dstr_NewFromDstr(nodeRef->nameRef);
        }
        originalRef->nameHash = nodeRef->nameHash;
    }

    // Check the types of the original and the shadow nodes.  If the new node has been cleared,
    // then clear out the original node.  If the types have changed, then clear out the original so
    // that we can properly populate it again.
    le_cfg_nodeType_t nodeType = tdb_GetNodeType(nodeRef);

    if (   (nodeType == LE_CFG_TYPE_EMPTY)
        || (nodeType != originalRef->type))
    {
        tdb_SetEmpty(originalRef);
    }

    // Ok, we know that the node hasn't been deleted.  Check to see if it's considered empty and
    // that it isn't a stem.  If not, then copy over the string value.
    if (   (nodeType != LE_CFG_TYPE_EMPTY)
        && (nodeType != LE_CFG_TYPE_STEM))
    {
        if (nodeRef->info.valueRef != NULL)
        {
            if (originalRef->info.valueRef != NULL)
            {
                dstr_Copy(originalRef->info.valueRef, nodeRef->info.valueRef);
            }
            else
            {
                originalRef->info.valueRef = dstr_NewFromDstr(nodeRef->info.valueRef);
            }

            // Propigate over the type as that may have changed, like going from an int value to a
            // bool value.

            originalRef->type = nodeRef->type;
        }
    }

    // Now at this point, if both the original and the shadow node are stems, we'll let the function
    // InternalMergeTree take care of the children, (if any.)

    // If the original has been cleared out, we can still just rely on InternalMergeTree to
    // propigate over the new nodes.
}




// -------------------------------------------------------------------------------------------------
/**
 *  Called to fire any callbacks registered on the given node path.  If nothing is registered on the
 *  given path, nothing happens.
 */
// -------------------------------------------------------------------------------------------------
static void TriggerCallbacks
(
    le_pathIter_Ref_t pathRef  ///< [IN] The path to search for callback registrations.
)
// -------------------------------------------------------------------------------------------------
{
    // Read the path out of the buffer.
    char pathBuffer[CFG_MAX_PATH_SIZE] = { 0 };
    if (le_pathIter_GetPath(pathRef, pathBuffer, sizeof(pathBuffer)) != LE_OK)
    {
        LE_ERROR("Callback path buffer overflow.");
        return;
    }

    // Try to find a registration object for this path.  If one is found, flag it for calling once
    // the merge is complete.
    Registration_t* foundRegistrationPtr = le_hashmap_Get(HandlerRegistrationMap, pathBuffer);

    if (foundRegistrationPtr != NULL)
    {
        foundRegistrationPtr->triggered = true;
    }
}





// -------------------------------------------------------------------------------------------------
/**
 *  Go through all of the registered event callbacks, and fire the call backs for each of the
 *  registrations that has been makred as triggered.
 *
 *  Once this is done, the triggered flag is cleared for next time.
 */
// -------------------------------------------------------------------------------------------------
static void FireTriggeredCallbacks
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    // Go through the registration map.
    le_hashmap_It_Ref_t handlerIterRef = le_hashmap_GetIterator(HandlerRegistrationMap);

    while (le_hashmap_NextNode(handlerIterRef) == LE_OK)
    {
        // For each registration, check to see if it was triggered.
        Registration_t* registrationPtr = (Registration_t*)le_hashmap_GetValue(handlerIterRef);

        if (registrationPtr->triggered)
        {
            // This registration has been triggered, so call all of the handlers attached to it.
            le_dls_Link_t* linkPtr = le_dls_Peek(&registrationPtr->handlerList);

            while (linkPtr != NULL)
            {
                Handler_t* handlerObjectPtr = CONTAINER_OF(linkPtr, Handler_t, link);

                handlerObjectPtr->handlerPtr(handlerObjectPtr->contextPtr);
                linkPtr = le_dls_PeekNext(&registrationPtr->handlerList, linkPtr);
            }

            // Now that that's done, clear the triggered flag.
            registrationPtr->triggered = false;
        }
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check the given node to see if it was renamed.
 *
 *  @return True if the node was renamed within this transaction.  False if not.
 */
// -------------------------------------------------------------------------------------------------
static bool WasRenamed
(
    tdb_NodeRef_t nodeRef  ///< [IN] Check this node to see if it was renamed in this transaction.
)
// -------------------------------------------------------------------------------------------------
{
    if (IsModified(nodeRef) == false)
    {
        // The node wasn't even modified, so it can not have been renamed.
        return false;
    }

    if (nodeRef->shadowRef == NULL)
    {
        // If the node doesn't have a shadow reference, then most likely this is a new node and not
        // a rename of an existing one.
        return false;
    }

    if (nodeRef->nameRef == NULL)
    {
        // The shadow node does not have a local copy of a name, so it can not have been renamed.
        // It must have been modified for other reasons.
        return false;
    }

    // Looks like the node has a new name.
    return true;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check the original non-shadow node to see if it will need to be cleared during the merge.
 *
 *  @return True if the merge will clear out the original value.  False if not.
 */
// -------------------------------------------------------------------------------------------------
static bool OriginalToBeCleared
(
    tdb_NodeRef_t nodeRef  ///< [IN] The shadow node to check.
)
// -------------------------------------------------------------------------------------------------
{
    le_cfg_nodeType_t nodeType = tdb_GetNodeType(nodeRef);

    if (   (nodeType == LE_CFG_TYPE_EMPTY)
        || (nodeType != tdb_GetNodeType(nodeRef->shadowRef)))
    {
        return true;
    }

    return false;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Append the name of a node onto the end of a path object.
 */
// -------------------------------------------------------------------------------------------------
static void AppendNodeName
(
    le_pathIter_Ref_t pathRef,  ///< [IN] The path we're appending to.
    tdb_NodeRef_t nodeRef       ///< [IN] The node we're appending.
)
// -------------------------------------------------------------------------------------------------
{
    char nodeName[LE_CFG_NAME_LEN_BYTES] = "";

    LE_ASSERT(tdb_GetNodeName(nodeRef, nodeName, sizeof(nodeName)) == LE_OK);
    le_result_t result = le_pathIter_Append(pathRef, nodeName);

    LE_WARN_IF(result != LE_OK,
               "Could not append node '%s' onto the update callback tracking path.  "
               "Reason: %d, '%s'.",
               nodeName,
               result,
               LE_RESULT_TXT(result));
}




// -------------------------------------------------------------------------------------------------
/**
 *  Create a new config path for the tree name given.
 *
 *  @return A new config path, rooted on the given tree.
 */
// -------------------------------------------------------------------------------------------------
static le_pathIter_Ref_t CreateBasePath
(
    const char* treeNamePtr  ///< [IN] The tree name to use for the new path object.
)
// -------------------------------------------------------------------------------------------------
{
    char basePath[CFG_MAX_PATH_SIZE] = { 0 };

    snprintf(basePath, sizeof(basePath), "%s:/", treeNamePtr);
    return le_pathIter_CreateForUnix(basePath);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Generate a config path to the given node.
 */
// -------------------------------------------------------------------------------------------------
static void GeneratePath
(
    le_pathIter_Ref_t pathRef,  ///< [IN] The path object we're updating.
    tdb_NodeRef_t nodeRef       ///< [IN] The node we're creating a path for.
)
// -------------------------------------------------------------------------------------------------
{
    if (nodeRef == NULL)
    {
        return;
    }

    GeneratePath(pathRef, nodeRef->parentRef);
    AppendNodeName(pathRef, nodeRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Trigger callbacks for this node and all of it's children.
 */
// -------------------------------------------------------------------------------------------------
static void FireAllChildren
(
    le_pathIter_Ref_t pathRef,  ///< [IN] Path to the parent of the current node.
    tdb_NodeRef_t nodeRef       ///< [IN] Node and any children to merge.
)
// -------------------------------------------------------------------------------------------------
{
    // Add this node to the path we're using to find registered callbacks.
    AppendNodeName(pathRef, nodeRef);

    // If the node is a stem then traverse it's children and try to trigger callbacks for them.  If
    // there are no callbacks registered for those nodes, then nothing will happen.
    if (nodeRef->type == LE_CFG_TYPE_STEM)
    {
        tdb_NodeRef_t childRef = tdb_GetFirstChildNode(nodeRef);

        while (childRef != NULL)
        {
            FireAllChildren(pathRef, childRef);
            childRef = tdb_GetNextSiblingNode(childRef);
        }
    }

    // Like with the children, try to do the same for this node.  Then remove this node from the
    // tracking path.
    TriggerCallbacks(pathRef);
    le_pathIter_Truncate(pathRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check a given shadow node and the original node it's shadowing.  If the original has children
 *  that will be lost because of a merge, then we need to fire callbacks for those nodes that are
 *  about to go away.
 *
 *  The algorithm employed by this function is as follows:
 *
 *      1. Check the original node for the given shadow node.  If it exists and is a stem node,
 *         mark all of the children as deleted.  (This is done with the expectation that the
 *         original tree does not have nodes with the deleted flag set.)
 *
 *      2. Go through the shadow collection, and any shadow children that have links to the original
 *         nodes, clear the deleted flag.  These nodes are still considered "live."
 *
 *      3. Travers the original children one more time.  For any node that is still marked as
 *         deleted we queue up an event handler.  As this node has been removed from the colection
 *         and will be removed as part of the final merge.  The delete flag is also cleared at this
 *         step to ensure that there are no external side effects.
 */
// -------------------------------------------------------------------------------------------------
static void FireLostChildren
(
    le_pathIter_Ref_t pathRef,   ///< [IN] Path to the parent of the current node.
    tdb_NodeRef_t shadowNodeRef  ///< [IN] Node and any children to merge.
)
// -------------------------------------------------------------------------------------------------
{
    // Is the original a stem?  If no, then done.
    tdb_NodeRef_t originalRef = shadowNodeRef->shadowRef;

    if (originalRef->type != LE_CFG_TYPE_STEM)
    {
        return;
    }

    // Mark all originals deleted.
    tdb_NodeRef_t originalChildRef = tdb_GetFirstChildNode(originalRef);

    while (originalChildRef != NULL)
    {
        // Children in the original tree shouldn't currently be marked as deleted.
        LE_ASSERT(IsDeleted(originalChildRef) == false);

        SetDeletedFlag(originalChildRef);
        originalChildRef = tdb_GetNextSiblingNode(originalChildRef);
    }

    // Follow through all of the shadow links and unmark deletions.
    if (shadowNodeRef->type == LE_CFG_TYPE_STEM)
    {
        tdb_NodeRef_t shadowChildRef = tdb_GetFirstChildNode(shadowNodeRef);

        while (shadowChildRef != NULL)
        {
            if (shadowChildRef->shadowRef != NULL)
            {
                ClearDeletedFlag(shadowChildRef->shadowRef);
            }

            shadowChildRef = tdb_GetNextSiblingNode(shadowChildRef);
        }
    }

    // Fire on all original nodes still marked.  But also take care to clear the deleted flags here
    // in order to leave everything as it was.
    originalChildRef = tdb_GetFirstChildNode(originalRef);

    while (originalChildRef != NULL)
    {
        if (IsDeleted(originalChildRef) == true)
        {
            FireAllChildren(pathRef, originalChildRef);
            ClearDeletedFlag(originalChildRef);
        }

        originalChildRef = tdb_GetNextSiblingNode(originalChildRef);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Recursive function to merge a collection of shadow nodes with the original tree.
 *
 *  @return True if the given node or any if it's children have been modified.  False if not.
 */
// -------------------------------------------------------------------------------------------------
static bool InternalMergeTree
(
    const char* treeNamePtr,    ///< [IN] The name of the tree we're merging.
    le_pathIter_Ref_t pathRef,  ///< [IN] Path to the parent of hte current node.
    tdb_NodeRef_t nodeRef,      ///< [IN] Node and any children to merge.
    bool forceFire              ///< [IN] Should update handlers be fired for this node and all it's
                                ///<      children, regardless of wether or not this node has been
                                ///<      directly modified?
)
// -------------------------------------------------------------------------------------------------
{
    bool isModified = IsModified(nodeRef);
    bool renamed = WasRenamed(nodeRef);

    // If this node was renamed, then all children also need to be triggered as well.
    forceFire = renamed || forceFire;

    // If this node has been renamed, marked as deleted or set empty, then all of the children need
    // notifications fired on the original nodes.
    if (   (renamed == true)
        || (IsDeleted(nodeRef) == true)
        || (OriginalToBeCleared(nodeRef) == true))
    {
        le_pathIter_Ref_t originalPathRef = CreateBasePath(treeNamePtr);

        if (nodeRef->shadowRef != NULL)
        {
            GeneratePath(originalPathRef, nodeRef->shadowRef->parentRef);
            FireAllChildren(originalPathRef, nodeRef->shadowRef);
        }

        le_pathIter_Delete(originalPathRef);
    }
    else if (   (isModified == true)
             && (nodeRef->type == LE_CFG_TYPE_STEM))
    {
        le_pathIter_Ref_t originalPathRef = CreateBasePath(treeNamePtr);

        GeneratePath(originalPathRef, nodeRef->shadowRef);
        FireLostChildren(originalPathRef, nodeRef);

        le_pathIter_Delete(originalPathRef);
    }

    AppendNodeName(pathRef, nodeRef);

    // IF this node is modified, mearge it.  If this node is a stem, then merge it's children.  Keep
    // track of whether any of those children have been modified as well.
    if (isModified)
    {
        MergeNode(nodeRef);
    }

    if (   (nodeRef->type == LE_CFG_TYPE_STEM)
        && (IsDeleted(nodeRef) == false))
    {
        nodeRef = tdb_GetFirstChildNode(nodeRef);

        while (nodeRef != NULL)
        {
            tdb_NodeRef_t nextNodeRef = tdb_GetNextSiblingNode(nodeRef);

            isModified = InternalMergeTree(treeNamePtr, pathRef, nodeRef, forceFire) || isModified;
            nodeRef = nextNodeRef;
        }
    }

    // If this node, or any of it's children have been modified.  Try to fire any callbacks that may
    // be registered.
    if (isModified || forceFire)
    {
        TriggerCallbacks(pathRef);
    }

    // Now remove this node from the tracking path and let our caller know if any modifications have
    // happened at this level or lower.
    if (le_pathIter_GoToEnd(pathRef) == LE_OK)
    {
        le_pathIter_Truncate(pathRef);
    }

    return isModified;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Create a new tree object and set it to default values.
 *
 *  @return A ref to the newly created tree object.
 */
// -------------------------------------------------------------------------------------------------
tdb_TreeRef_t NewTree
(
    const char* treeNameRef,   ///< [IN] The name of the new tree.
    tdb_NodeRef_t rootNodeRef  ///< [IN] The root node of this new tree.
)
// -------------------------------------------------------------------------------------------------
{
    tdb_TreeRef_t treeRef = le_mem_ForceAlloc(TreePoolRef);

    LE_ASSERT(le_utf8_Copy(treeRef->name, treeNameRef, MAX_TREE_NAME_BYTES, NULL) == LE_OK);

    treeRef->isDeletePending = false;
    treeRef->originalTreeRef = NULL;
    treeRef->revisionId = 0;
    treeRef->rootNodeRef = (rootNodeRef != NULL) ? rootNodeRef : NewNode();
    treeRef->activeReadCount = 0;
    treeRef->activeWriteIterRef = NULL;
    treeRef->requestList = LE_SLS_LIST_INIT;

    return treeRef;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Destructor called when a tree object is to be freed from memory.
 */
// -------------------------------------------------------------------------------------------------
static void TreeDestructor
(
    void* objectPtr  ///< The memory object to destruct.
)
// -------------------------------------------------------------------------------------------------
{
    tdb_TreeRef_t treeRef = (tdb_TreeRef_t)objectPtr;

    // Kill the root node.
    le_mem_Release(treeRef->rootNodeRef);
    treeRef->rootNodeRef = NULL;

    // Sanity check, is the tree actually ready to clean up?
    LE_ASSERT(treeRef->activeReadCount == 0);
    LE_ASSERT(treeRef->activeWriteIterRef == NULL);
    LE_ASSERT(le_sls_IsEmpty(&treeRef->requestList) == true);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Create a path to a tree file with the given revision id.
 *
 *  @return A stringBuffer backed string containing the full path to the tree file.
 */
// -------------------------------------------------------------------------------------------------
static void GetTreePath
(
    const char* treeNameRef,  ///< [IN] The name of the tree we're generating a name for.
    int revisionId,           ///< [IN] Generate a name based on the tree revision.
    char* pathBuffer,         ///< [IN] Buffer to hold the new path.
    size_t pathSize           ///< [IN] Size of the path buffer.
)
// -------------------------------------------------------------------------------------------------
{
    // paper    --> rock       1 -> 2
    // rock     --> scissors   2 -> 3
    // scissors --> paper      3 -> 1

    static const char* revNames[] = { "paper", "rock", "scissors" };
    int printSize;

    LE_ASSERT((revisionId >= 1) && (revisionId <= 3));

    printSize = snprintf(pathBuffer,
                         pathSize,
                         "%s/%s.%s",
                         CFG_TREE_PATH,
                         treeNameRef,
                         revNames[revisionId - 1]);

    if (printSize >= pathSize)
    {
       LE_ERROR("Unable to store config tree path in buffer");
       pathBuffer[0] = '\0';
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check to see if a configTree file at the given revision already exists in the filesystem.
 *
 *  @note If the tree file exists, but is empty, then it is invalid and will be deleted.
 *
 *  @return True if the named file exists, false otherwise.
 */
// -------------------------------------------------------------------------------------------------
static bool TreeFileExists
(
    const char* treeNameRef,  ///< [IN] Name of the tree to check.
    int revisionId            ///< [IN] The revision of the tree to check against.
)
// -------------------------------------------------------------------------------------------------
{
    char fullPath[LE_CFG_STR_LEN_BYTES] = "";
    GetTreePath(treeNameRef, revisionId, fullPath, sizeof(fullPath));

    // Make sure this part was successful.
    if (fullPath[0] == '\0')
    {
        return false;
    }

    // stat() the file to see if it exists and get its size.
    struct stat s;
    if (stat(fullPath, &s) == -1)
    {
        LE_DEBUG("Can't stat file '%s' (%m).", fullPath);
        return false;
    }

    // Make sure it's a regular file.
    if (!S_ISREG(s.st_mode))
    {
        LE_FATAL("Object at '%s' is not a regular file.", fullPath);
    }

    // If it's zero size, delete it and report that it doesn't exist.
    if (s.st_size == 0)
    {
        if (unlink(fullPath) == -1)
        {
            LE_FATAL("Failed to unlink empty file '%s' (%m).", fullPath);
        }

        return false;
    }

    // NOTE: The Config Tree generally runs as root, so permissions should be irrelevant.

    return true;
}




// -------------------------------------------------------------------------------------------------
/**
 * Check the filesystem and get the current "valid" version of the file and update the tree object
 * with that version number.
 *
 * If there are two files for a given tree, we use the older one.  The idea being, if there are
 * two versions of the same file in the filesystem then there was a system failure during a save
 * operation.  So we abandon the newer (probably incomplete) file and go with the older file;
 * unless the size of the older file is zero, which can happen if deletion of that file is
 * interrupted.
 */
// -------------------------------------------------------------------------------------------------
static void UpdateRevision
(
    tdb_TreeRef_t treeRef  ///< [IN] Update the revision for this tree object.
)
// -------------------------------------------------------------------------------------------------
{
    int newRevision = 0;

    if (TreeFileExists(treeRef->name, 1))
    {
        if (TreeFileExists(treeRef->name, 3))
        {
            newRevision = 3;
        }
        else
        {
            newRevision = 1;
        }
    }
    else if (TreeFileExists(treeRef->name, 3))
    {
        if (TreeFileExists(treeRef->name, 2))
        {
            newRevision = 2;
        }
        else
        {
            newRevision = 3;
        }
    }
    else if (TreeFileExists(treeRef->name, 2))
    {
        newRevision = 2;
    }

    treeRef->revisionId = newRevision;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Peek into the input stream one character ahead.
 */
// -------------------------------------------------------------------------------------------------
static signed char PeekChar
(
    FILE* filePtr  ///< [IN] The file stream to peek into.
)
// -------------------------------------------------------------------------------------------------
{
    char next = fgetc(filePtr);
    ungetc(next, filePtr);

    return next;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Skip any whitespace encountered in the input stream.  Stop skipping once we hit a valid token.
 *
 *  @return LE_OK if the whitespace is skiped and there is still more file to read.
 *          LE_OUT_OF_RANGE if the end of the file is hit.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t SkipWhiteSpace
(
    FILE* filePtr  ///< [IN] The file stream to seek through.
)
// -------------------------------------------------------------------------------------------------
{
    bool done = false;
    bool isEof = false;

    while (done == false)
    {
        switch (PeekChar(filePtr))
        {
            case '\n':
            case '\r':
            case '\t':
            case ' ':
                // Eat the character.
                fgetc(filePtr);
                break;

            case EOF:
                done = true;
                isEof = true;
                break;

            default:
                done = true;
                break;
        }
    }

    return isEof == true ? LE_OUT_OF_RANGE : LE_OK;
}


// -------------------------------------------------------------------------------------------------
/**
 *  Skip to the next occurence of the given character.
 *
 *  @return LE_OK if the character is found.
 *          LE_OUT_OF_RANGE if the end of the file is hit.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t SkipToNextChar
(
    FILE* filePtr,          ///< [IN] The file stream to seek through.
    signed char nextChar    ///< [IN]  The character we're searching for.
)
{
    signed char next;

    while ((next = fgetc(filePtr)) != nextChar)
    {
        if (next == EOF)
        {
            return LE_OUT_OF_RANGE;
        }
    }

    return LE_OK;
}


// -------------------------------------------------------------------------------------------------
/**
 *  Read a boolean literal from the input file.
 *
 *  @return LE_OK if the literal could be read.
 *          LE_FORMAT_ERROR if the literal could not be read.
 */
// -------------------------------------------------------------------------------------------------
static bool ReadBoolToken
(
    FILE* filePtr,     ///< [IN]  The file we're reading from.
    char* stringPtr,   ///< [OUT] String buffer to hold the token we've read.
    size_t stringSize  ///< [IN]  How big is the supplied string buffer?
)
// -------------------------------------------------------------------------------------------------
{
    signed char next = fgetc(filePtr);

    if (   (next == 't')
        || (next == 'f'))
    {
        stringPtr[0] = next;
        stringPtr[1] = 0;

        return LE_OK;
    }

    return LE_FORMAT_ERROR;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a textual literal from the input file, the read is terminated successfuly if the terminal
 *  character is found.
 *
 *  @return LE_OK if the string is read from the file.
 *          LE_FORMAT_ERROR if the text fails to be read from the file.
 *          LE_OVERFLOW if the text doesn't fit in provided buffer (truncated).
 */
// -------------------------------------------------------------------------------------------------
static le_result_t ReadTextLiteral
(
    FILE* filePtr,        ///< [IN]  The file we're reading from.
    char* stringPtr,      ///< [OUT] String buffer to hold the token we've read.
    size_t stringSize,    ///< [IN]  How big is the supplied string buffer?
    signed char terminal  ///< [IN]  The terminal character we're searching for.
)
// -------------------------------------------------------------------------------------------------
{
    signed char next;
    size_t count = 0;

    char* oldPtr = stringPtr;

    while ((next = fgetc(filePtr)) != terminal)
    {
        if (next == EOF)
        {
            LE_ERROR("Missing end specifier, ']' in int value.");
            return LE_FORMAT_ERROR;
        }

        if (next == '\\')
        {
            next = fgetc(filePtr);

            if (next == EOF)
            {
                LE_ERROR("Unexpected EOF after finding \\ character.");
                return LE_FORMAT_ERROR;
            }
        }

        if (count >= (stringSize - 1))
        {
            // truncate the string to the buffer size
            *stringPtr = 0;

            LE_ERROR("String literal is too large.  (%" PRIdS "/%" PRIdS ")",
                     strlen(oldPtr),
                     stringSize);
            // move the file pointer to the terminal character (e.g. closing quote)
            if (LE_OK == SkipToNextChar(filePtr, terminal))
            {
                return LE_OVERFLOW;
            }
            else
            {
                return LE_FORMAT_ERROR;
            }
        }

        *stringPtr = next;

        ++stringPtr;
        ++count;
    }

    *stringPtr = 0;

    return LE_OK;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read an integer token string from the file.
 *
 *  @return LE_OK if the string is read from the file.
 *          LE_FORMAT_ERROR if the text fails to be read from the file.
 *          LE_OVERFLOW if the text doesn't fit in provided buffer (truncated).
 */
// -------------------------------------------------------------------------------------------------
static le_result_t ReadIntToken
(
    FILE* filePtr,     ///< [IN]  The file we're reading from.
    char* stringPtr,   ///< [OUT] String buffer to hold the token we've read.
    size_t stringSize  ///< [IN]  How big is the supplied string buffer?
)
// -------------------------------------------------------------------------------------------------
{
    le_result_t result = ReadTextLiteral(filePtr, stringPtr, stringSize, ']');

    // TODO: Validate the int string.

    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a floating point token string from the file.
 *
 *  @return LE_OK if the string is read from the file.
 *          LE_FORMAT_ERROR if the text fails to be read from the file.
 *          LE_OVERFLOW if the text doesn't fit in provided buffer (truncated).
 */
// -------------------------------------------------------------------------------------------------
static le_result_t ReadFloatToken
(
    FILE* filePtr,     ///< [IN]  The file we're reading from.
    char* stringPtr,   ///< [OUT] String buffer to hold the token we've read.
    size_t stringSize  ///< [IN]  How big is the supplied string buffer?
)
// -------------------------------------------------------------------------------------------------
{
    le_result_t result = ReadTextLiteral(filePtr, stringPtr, stringSize, ')');

    // TODO: Validate the float string.

    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a string from the config tree file.
 *
 *  @return LE_OK if the string is read from the file.
 *          LE_FORMAT_ERROR if the text fails to be read from the file.
 *          LE_OVERFLOW if the text doesn't fit in provided buffer (truncated).
 */
// -------------------------------------------------------------------------------------------------
static le_result_t ReadStringToken
(
    FILE* filePtr,     ///< [IN]  The file we're reading from.
    char* stringPtr,   ///< [OUT] String buffer to hold the token we've read.
    size_t stringSize  ///< [IN]  How big is the supplied string buffer?
)
// -------------------------------------------------------------------------------------------------
{
    le_result_t result = ReadTextLiteral(filePtr, stringPtr, stringSize, '"');

    // TODO: Validate the literal string.

    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a token from the input stream.
 *
 *  @return LE_OK if a token could be read.  LE_OUT_OF_RANGE if the end of the stream is reached
 *          before a token could be finished.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t ReadToken
(
    FILE* filePtr,         ///< [IN]  The file we're reading from.
    char* stringPtr,       ///< [OUT] String buffer to hold the token we've read.
    size_t stringSize,     ///< [IN]  How big is the supplied string buffer?
    TokenType_t* typePtr   ///< [OUT] The type of token read from the file.
)
// -------------------------------------------------------------------------------------------------
{
    *stringPtr = 0;

    if (SkipWhiteSpace(filePtr) != LE_OK)
    {
        return LE_OUT_OF_RANGE;
    }

    signed char next;

    while ((next = fgetc(filePtr)) != EOF)
    {
        switch (next)
        {
            case '~':
                *typePtr = TT_EMPTY_VALUE;
                return LE_OK;

            case '!':
                *typePtr = TT_BOOL_VALUE;
                return ReadBoolToken(filePtr, stringPtr, stringSize);

            case '[':
                *typePtr = TT_INT_VALUE;
                return ReadIntToken(filePtr, stringPtr, stringSize);

            case '(':
                *typePtr = TT_FLOAT_VALUE;
                return ReadFloatToken(filePtr, stringPtr, stringSize);

            case '\"':
                *typePtr = TT_STRING_VALUE;
                return ReadStringToken(filePtr, stringPtr, stringSize);

            case '{':
                *typePtr = TT_OPEN_GROUP;
                return LE_OK;

            case '}':
                *typePtr = TT_CLOSE_GROUP;
                return LE_OK;

            default:
                LE_ERROR("Unexpected character in input stream.");
                return LE_FORMAT_ERROR;
        }
    }

    return LE_OUT_OF_RANGE;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write data to the output stream.  This function will record any faults to the system log.
 *
 *  @return LE_OK if the write succeeded, LE_IO_ERROR if the write failed.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t WriteFile
(
    FILE* filePtr,        ///< [IN] The file being written to.
    const void* dataPtr,  ///< [IN] The data being written to the file.
    size_t dataSize       ///< [IN] The amount of data being written.
)
// -------------------------------------------------------------------------------------------------
{
    ssize_t written = fwrite(dataPtr, 1, dataSize, filePtr);

    if (ferror(filePtr) != 0)
    {
        LE_EMERG("Failed to write to config tree file.");
        return LE_IO_ERROR;
    }

    if (written < dataSize)
    {
        LE_EMERG("Data truncated while writing to configuration file.");
        return LE_IO_ERROR;
    }

    return LE_OK;
}



// -------------------------------------------------------------------------------------------------
/**
 *  Write a string token to the output stream.  This function will write the string and escape all
 *  control characters as it does so.
 *
 *  @return LE_OK if the write succeeded, LE_IO_ERROR if the write failed.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t WriteStringValue
(
    FILE* filePtr,         ///< [IN] The file to write to.
    char startChar,        ///< [IN] The delimiter to use.
    char endChar,          ///< [IN] The closing delimiter to use.
    const char* stringPtr  ///< [IN] The actual string to write.
)
// -------------------------------------------------------------------------------------------------
{
    le_result_t result = LE_OK;

    result = WriteFile(filePtr, &startChar, 1);

    while (   (*stringPtr != 0)
           && (result == LE_OK))
    {
        if (   (*stringPtr == '\"')
            || (*stringPtr == '\\'))
        {
            result = WriteFile(filePtr, "\\", 1);
        }

        if (result == LE_OK)
        {
            result = WriteFile(filePtr, stringPtr, 1);
        }

        stringPtr++;
    }

    if (result == LE_OK)
    {
        char strBuffer[2] = { endChar, ' ' };
        result = WriteFile(filePtr, strBuffer, sizeof(strBuffer));
    }

    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a node value from the given file.  If the value is a collection, then read in those nodes
 *  too.
 *
 *  @return LE_OK if the read is successful.
 *          LE_FORMAT_ERROR if parse errors are encountered.
 *          LE_NOT_FOUND if the end of file is reached.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t InternalReadNode
(
    tdb_NodeRef_t nodeRef,  ///< [IN] The node we're reading a value for.
    FILE* filePtr,          ///< [IN] The file we're reading the value from.
    size_t pathLen          ///< [IN] The length of the path including nodeRef.
)
// -------------------------------------------------------------------------------------------------
{
    char* stringBuffer = le_mem_ForceAlloc(EncodedStringPool);
    size_t stringBufferSize = TDB_MAX_ENCODED_SIZE;
    le_result_t result = LE_OK;

    TokenType_t tokenType;

    // Try to read this node's value.
    result = ReadToken(filePtr, stringBuffer, stringBufferSize, &tokenType);
    if ((result != LE_OK) && (result != LE_OVERFLOW))
    {
        LE_ERROR("Unexpected EOF or bad token in file.");
        result = LE_FORMAT_ERROR;
        goto cleanup;
    }
    // if result is OVERFLOW (i.e. string is truncated), we still should be able
    // to proceed with parsing the rest of the file.
    result = LE_OK;

    tdb_SetEmpty(nodeRef);

    switch (tokenType)
    {
        case TT_BOOL_VALUE:
            tdb_SetValueAsString(nodeRef, stringBuffer);
            nodeRef->type = LE_CFG_TYPE_BOOL;
            break;

        case TT_INT_VALUE:
            tdb_SetValueAsString(nodeRef, stringBuffer);
            nodeRef->type = LE_CFG_TYPE_INT;
            break;

        case TT_FLOAT_VALUE:
            tdb_SetValueAsString(nodeRef, stringBuffer);
            nodeRef->type = LE_CFG_TYPE_FLOAT;
            break;

        case TT_STRING_VALUE:
            tdb_SetValueAsString(nodeRef, stringBuffer);
            break;

        case TT_EMPTY_VALUE:
            // The node has already been cleared, so there's nothing left to do but make sure that
            // the node exists.
            ClearDeletedFlag(nodeRef);
            break;

        case TT_OPEN_GROUP:
            while (tokenType != TT_CLOSE_GROUP)
            {
                if (ReadToken(filePtr, stringBuffer, stringBufferSize, &tokenType) != LE_OK)
                {
                    LE_ERROR("Unexpected EOF or bad token in file while looking for '}'.");
                    result = LE_FORMAT_ERROR;
                    goto cleanup;
                }

                if (tokenType == TT_STRING_VALUE)
                {
                    size_t strLen = le_utf8_NumBytes(stringBuffer);
                    size_t newPathLen = pathLen + 1 + strLen;

                    if (newPathLen > LE_CFG_STR_LEN)
                    {
                        LE_ERROR("New path length for node '%s' is too long.  %" PRIuS
                                 " of %" PRIuS " bytes.",
                                 stringBuffer,
                                 strLen,
                                 (size_t)LE_CFG_STR_LEN);

                        result = LE_FORMAT_ERROR;
                        goto cleanup;
                    }

                    tdb_NodeRef_t childRef = GetNamedChild(nodeRef, stringBuffer);

                    if (childRef == NULL)
                    {
                        childRef = NewChildNode(nodeRef);
                        if (tdb_SetNodeName(childRef, stringBuffer) != LE_OK)
                        {
                            LE_ERROR("Bad node name, '%s'.", stringBuffer);
                            result = LE_FORMAT_ERROR;
                            goto cleanup;
                        }

                        LE_DEBUG("New node, %s", stringBuffer);
                    }

                    tdb_EnsureExists(childRef);

                    result = InternalReadNode(childRef, filePtr, newPathLen);

                    if (result != LE_OK)
                    {
                        goto cleanup;
                    }
                }
                else if (tokenType == TT_CLOSE_GROUP)
                {
                    break;
                }
                else
                {
                    LE_ERROR("Unexpected token in found while looking for '}'.");
                    result = LE_FORMAT_ERROR;
                    goto cleanup;
                }
            }
            break;

        case TT_CLOSE_GROUP:
        default:
            LE_ERROR("Unexpected token found.");
            result = LE_FORMAT_ERROR;
            goto cleanup;
    }

    if (IsShadow(nodeRef) == false)
    {
        ClearModifiedFlag(nodeRef);
    }
    else
    {
        SetModifiedFlag(nodeRef);
    }

    tdb_EnsureExists(nodeRef);

cleanup:
    le_mem_Release(stringBuffer);
    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Serialize a tree node and it's children to a file in the filesystem.
 *
 *  @return LE_OK if the write succeeded, LE_IO_ERROR if the write failed.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t InternalWriteNode
(
    tdb_NodeRef_t nodeRef,  ///< [IN] The node being written.
    FILE* filePtr           ///< [IN] The file being written to.
)
// -------------------------------------------------------------------------------------------------
{
    // If there is no node to write, or if the node is marked as having been deleted...  Then write
    // a blank node.
    if (   (nodeRef == NULL)
        || (IsDeleted(nodeRef) == true))
    {
        return WriteFile(filePtr, "~ ", 2);
    }

    // Get the node's value as a string.
    char* stringBuffer = le_mem_ForceAlloc(EncodedStringPool);
    size_t stringBufferSize = TDB_MAX_ENCODED_SIZE;
    le_result_t result = LE_OK;

    tdb_GetValueAsString(nodeRef, stringBuffer, stringBufferSize, "");

    // Now, depending on the type of node, write out any required format information.
    switch (nodeRef->type)
    {
        case LE_CFG_TYPE_EMPTY:
        case LE_CFG_TYPE_DOESNT_EXIST:
            result = WriteFile(filePtr, "~ ", 2);
            break;

        case LE_CFG_TYPE_BOOL:
            {
                const char boolBuffer[3] = { '!', stringBuffer[0], ' ' };
                result = WriteFile(filePtr, boolBuffer, sizeof(boolBuffer));
            }
            break;

        case LE_CFG_TYPE_STRING:
            result = WriteStringValue(filePtr, '\"', '\"', stringBuffer);
            break;

        case LE_CFG_TYPE_INT:
            result = WriteStringValue(filePtr, '[', ']', stringBuffer);
            break;

        case LE_CFG_TYPE_FLOAT:
            result = WriteStringValue(filePtr, '(', ')', stringBuffer);
            break;

        // Looks like this node is a collection, so write out it's child nodes now.
        case LE_CFG_TYPE_STEM:
            if ((result = WriteFile(filePtr, "{ ", 2)) == LE_OK)
            {
                tdb_NodeRef_t childRef = tdb_GetFirstActiveChildNode(nodeRef);

                while (   (childRef != NULL)
                       && (result == LE_OK))
                {
                    tdb_GetNodeName(childRef, stringBuffer, stringBufferSize);
                    result = WriteStringValue(filePtr, '\"', '\"', stringBuffer);

                    if (result == LE_OK)
                    {
                        result = InternalWriteNode(childRef, filePtr);
                    }

                    childRef = tdb_GetNextActiveSiblingNode(childRef);
                }

                if (result == LE_OK)
                {
                    result = WriteFile(filePtr, "} ", 2);
                }
            }
            break;
    }

    le_mem_Release(stringBuffer);
    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Calculate the number of bytes required to store a node path, including seperators and a trailing
 *  NULL.
 *
 *  @return The amount of bytes required to store the whole path string.
 */
// -------------------------------------------------------------------------------------------------
static size_t ComputePathLength
(
    tdb_NodeRef_t nodeRef  ///< [IN] Compute a path for this node.
)
// -------------------------------------------------------------------------------------------------
{
    size_t pathLen = 0;
    char nodeName[LE_CFG_NAME_LEN_BYTES] = "";

    while (nodeRef != NULL)
    {
        LE_ASSERT(tdb_GetNodeName(nodeRef, nodeName, sizeof(nodeName)) == LE_OK);

        // Add this path segment's length to our running total, along with the required path
        // seperator.
        pathLen += 1 + le_utf8_NumBytes(nodeName);
        nodeRef = tdb_GetNodeParent(nodeRef);
    }

    // Don't forget to include a spot for the trailing NULL.
    return pathLen + 1;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Bump up the version id of this tree.
 */
// -------------------------------------------------------------------------------------------------
static void IncrementRevision
(
    tdb_TreeRef_t treeRef  ///< [IN] Increment the revision of this tree.
)
// -------------------------------------------------------------------------------------------------
{
    treeRef->revisionId++;

    if (treeRef->revisionId > 3)
    {
        treeRef->revisionId = 1;
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Attempt to load a configuration tree from a config file.  This function will look for the latest
 *  valid version of the config file and load that one.
 */
// -------------------------------------------------------------------------------------------------
static void LoadTree
(
    tdb_TreeRef_t treeRef  ///< [IN] The tree object to load from the filesystem.
)
// -------------------------------------------------------------------------------------------------
{
    // If we don't know the revision then hunt it out from the filesystem.
    if (treeRef->revisionId == 0)
    {
        UpdateRevision(treeRef);
    }

    // If this tree has no root, create it now.
    if (treeRef->rootNodeRef == NULL)
    {
        treeRef->rootNodeRef = NewNode();
    }

    // Ok, if we found a valid revision of the tree in the fs, try to load it now.
    if (treeRef->revisionId != 0)
    {
        char pathPtr[LE_CFG_STR_LEN_BYTES] = "";
        GetTreePath(treeRef->name, treeRef->revisionId, pathPtr, sizeof(pathPtr));

        LE_DEBUG("** Loading configuration tree from '%s'.", pathPtr);

        FILE* fileRef;

        fileRef = fopen(pathPtr, "r");

        tdb_EnsureExists(treeRef->rootNodeRef);

        if (!fileRef)
        {
            LE_ERROR("Could not open configuration tree file: %s, reason: %s",
                     pathPtr,
                     strerror(errno));
        }
        else
        {
            if (tdb_ReadTreeNode(treeRef->rootNodeRef, fileRef) == false)
            {
                LE_ERROR("Could not parse configuration tree file: %s.", pathPtr);
                le_mem_Release(treeRef->rootNodeRef);
                treeRef->rootNodeRef = NewNode();
            }

            fclose(fileRef);
        }
    }
}



// -------------------------------------------------------------------------------------------------
/**
 *  Removes the handler object from the given registration object.  This function will also free the
 *  memory that the handler object had used.
 */
// -------------------------------------------------------------------------------------------------
static void RemoveHandler
(
    Registration_t* registrationPtr,  ///< [IN] The registration object to remove the link from.
    Handler_t* handlerPtr             ///< [IN] The handler object we're removing.
)
// -------------------------------------------------------------------------------------------------
{
    // Kill the ref, and remove the object from the registration list.
    le_ref_DeleteRef(HandlerSafeRefMap, handlerPtr->safeRef);
    le_dls_Remove(&registrationPtr->handlerList, &handlerPtr->link);

    // Clear out the link data, just to be safe.
    handlerPtr->link = LE_DLS_LINK_INIT;
    handlerPtr->sessionRef = NULL;
    handlerPtr->registrationPtr = NULL;
    handlerPtr->safeRef = NULL;

    // Finally kill the object.
    le_mem_Release(handlerPtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  This function is called by the hash map ForEach function, which is invoked when a session closed
 *  event occurs.
 *
 *  This function takes care of cleaning out orphaned event handlers from the registration objects
 *  currently stored in the registration hash map.  If a given registration handler is no longer
 *  required then the object itself is queued for deletion.  It is queued and not deleted in place
 *  because the hash map does not support deleting objects in the middle of an iteration.
 *
 *  @return True.  This function always returns true to indicate that iteration should continue
 *          until the end of the hash map.
 */
// -------------------------------------------------------------------------------------------------
static bool OnHandlerRegistrationCleanup
(
    const void* keyPtr,    ///< [IN] The key used by this hash entry.
    const void* valuePtr,  ///< [IN] The registration object.
    void* contextPtr       ///< [IN] Context info including the ref for the session that closed.
)
// -------------------------------------------------------------------------------------------------
{
    // Convert our pointers into something useable.
    Registration_t* registrationPtr = (Registration_t*)valuePtr;
    CleanUpContext_t* cleanUpContextPtr = (CleanUpContext_t*)contextPtr;

    // Go through this registration object's list of update handlers and check to see if they were
    // registered on the target session.  If so, free them from the list.
    le_dls_Link_t* linkPtr = le_dls_Peek(&registrationPtr->handlerList);

    while (linkPtr != NULL)
    {
        Handler_t* handlerObjectPtr = CONTAINER_OF(linkPtr, Handler_t, link);
        linkPtr = le_dls_PeekNext(&registrationPtr->handlerList, linkPtr);

        if (handlerObjectPtr->sessionRef == cleanUpContextPtr->sessionRef)
        {
            RemoveHandler(registrationPtr, handlerObjectPtr);
        }
    }

    // Now, check to see if there are any handlers left in this object.  If the registration object
    // is empty, then queue it for deletion.
    if (le_dls_IsEmpty(&registrationPtr->handlerList))
    {
        registrationPtr->link = LE_SLS_LINK_INIT;
        le_sls_Queue(&cleanUpContextPtr->deleteQueue, &registrationPtr->link);
    }

    // We want to continue iterating through the collection.
    return true;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Call this function to delete a tree file from the filesystem.
 */
// -------------------------------------------------------------------------------------------------
static void DeleteTreeFile
(
    const char* filePathPtr  ///< Path to the tree file in question.
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Deleting tree file, '%s'.", filePathPtr);

    if (unlink(filePathPtr) != 0)
    {
        LE_ERROR("File delete failure, '%s', reason '%m'.", filePathPtr);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Find the root node represented by the path ref.
 *
 *  If the path is an absolute path, then the base node for the reference is the root node of the
 *  tree in question.
 *
 *  If the path is a relative path, then the base node of the request is the node given.
 *
 *  @return A reference to the base node of the operation.
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t GetPathBaseNodeRef
(
    tdb_NodeRef_t nodeRef,         ///< [IN] The base node to start from.
    le_pathIter_Ref_t nodePathRef  ///< [IN] The path we're searching for in the tree.
)
// -------------------------------------------------------------------------------------------------
{
    // If the path is absolute and the node we were given is NOT the root node of it's tree, find
    // the root node of the tree.  Otherwise just return the node reference we were given.
    if (   (le_pathIter_IsAbsolute(nodePathRef))
        && (nodeRef->parentRef != NULL))
    {
        nodeRef = GetRootParentNode(nodeRef);
    }

    return nodeRef;
}


// -------------------------------------------------------------------------------------------------
/**
 *  Initialize the tree DB subsystem, and automaticly load the system tree from the filesystem.
 */
// -------------------------------------------------------------------------------------------------
void tdb_Init
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Initialize Tree DB subsystem.");

    // Initialize the memory pools.
    NodePoolRef = le_mem_InitStaticPool(nodePool, LE_CONFIG_CFGTREE_MAX_NODE_POOL_SIZE,
                                        sizeof(Node_t));
    le_mem_SetDestructor(NodePoolRef, NodeDestructor);
    le_mem_SetNumObjsToForce(NodePoolRef, 50);    // Grow in chunks of 50 blocks.

    TreePoolRef = le_mem_InitStaticPool(treePool, LE_CONFIG_CFGTREE_MAX_TREE_POOL_SIZE,
                                        sizeof(Tree_t));
    le_mem_SetDestructor(TreePoolRef, TreeDestructor);

    TreeCollectionRef = le_hashmap_InitStatic(TreeCollection,
                                              LE_CONFIG_CFGTREE_MAX_TREE_POOL_SIZE,
                                              le_hashmap_HashString,
                                              le_hashmap_EqualsString);

    HandlerRegistrationMap = le_hashmap_InitStatic(HandlerLookupMap,
                                                   LE_CONFIG_CFGTREE_MAX_HANDLER_POOL_SIZE,
                                                   le_hashmap_HashString,
                                                   le_hashmap_EqualsString);

    HandlerSafeRefMap = le_ref_InitStaticMap(HandlerSafeRefMap,
                                             LE_CONFIG_CFGTREE_MAX_HANDLER_POOL_SIZE);

    HandlerPool = le_mem_InitStaticPool(HandlerPool, LE_CONFIG_CFGTREE_MAX_HANDLER_POOL_SIZE, sizeof(Handler_t));

    RegistrationPool = le_mem_InitStaticPool(RegistrationPool,
                                             LE_CONFIG_CFGTREE_MAX_HANDLER_POOL_SIZE,
                                             sizeof(Registration_t));

    BinaryDataPool = le_mem_InitStaticPool(BinaryData,
                                           LE_CONFIG_CFGTREE_MAX_BINARY_DATA_POOL_SIZE,
                                           LE_CFG_BINARY_LEN);
    EncodedStringPool = le_mem_InitStaticPool(EncodedString,
                                              LE_CONFIG_CFGTREE_MAX_ENCODED_STRING_POOL_SIZE,
                                              TDB_MAX_ENCODED_SIZE);

    // Preload the system tree.
    tdb_GetTree("system");
}




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
)
// -------------------------------------------------------------------------------------------------
{
    // Check to see if we have this tree loaded up in our map.
    tdb_TreeRef_t treeRef = le_hashmap_Get(TreeCollectionRef, treeNamePtr);

    if (treeRef == NULL)
    {
        // Looks like we don't so create an object for it, and add it to our map.
        treeRef = NewTree(treeNamePtr, NULL);
        le_hashmap_Put(TreeCollectionRef, treeRef->name, treeRef);

        LoadTree(treeRef);
    }

    // Finally return the tree we have to the user.
    return treeRef;
}




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
)
// -------------------------------------------------------------------------------------------------
{
    // Check to see if there are any active iterators on the tree.  If there are, simply mark the
    // tree for deletion for now.
    if (   (tdb_GetActiveWriteIter(treeRef) == NULL)
        && (tdb_HasActiveReaders(treeRef) == 0)
        && (le_sls_IsEmpty(&treeRef->requestList)))
    {
        int id;

        // Looks like there's no one on the tree, so delete any tree files that may exist.  Then
        // kill the tree itself.
        LE_DEBUG("** Deleting configuration tree, '%s'.", treeRef->name);

        for (id = 1; id <= 3; id++)
        {
            if (TreeFileExists(treeRef->name, id))
            {
                char filePathPtr[LE_CFG_STR_LEN_BYTES] = "";
                GetTreePath(treeRef->name, id, filePathPtr, sizeof(filePathPtr));

                DeleteTreeFile(filePathPtr);
            }
        }

        LE_ASSERT(le_hashmap_Remove(TreeCollectionRef, treeRef->name) == treeRef);
        le_mem_Release(treeRef);
    }
    else
    {
        LE_WARN("** Configuration tree, '%s', deletion requested.  "
                "However there are still active iterators.  "
                "Marking for later deletion.",
                treeRef->name);

        treeRef->isDeletePending = true;
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Called to get the poitner to the tree collection iterator.
 *
 *  @return Reference to the tree collection iterator.
 */
// -------------------------------------------------------------------------------------------------
le_hashmap_It_Ref_t tdb_GetTreeIterRef
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    return le_hashmap_GetIterator(TreeCollectionRef);
}



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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(treeRef->originalTreeRef == NULL);
    tdb_TreeRef_t shadowRef = NewTree(treeRef->name, NewShadowNode(treeRef->rootNodeRef));
    shadowRef->originalTreeRef = treeRef;

    return shadowRef;
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(treeRef != NULL);
    return treeRef->name;
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(treeRef != NULL);
    return treeRef->rootNodeRef;
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(treeRef != NULL);

    if (treeRef->originalTreeRef != NULL)
    {
        return treeRef->originalTreeRef->activeWriteIterRef;
    }

    return treeRef->activeWriteIterRef;
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(treeRef != NULL);

    if (treeRef->originalTreeRef != NULL)
    {
        return treeRef->originalTreeRef->activeReadCount;
    }

    return treeRef->activeReadCount != 0;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Register an iterator on the given tree.
 */
// -------------------------------------------------------------------------------------------------
void tdb_RegisterIterator
(
    tdb_TreeRef_t treeRef,        ///< [IN] The tree object to update.
    ni_IteratorRef_t iteratorRef  ///< [IN] The iterator object we're registering.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(treeRef != NULL);
    LE_ASSERT(iteratorRef != NULL);

    if (treeRef->originalTreeRef != NULL)
    {
        treeRef = treeRef->originalTreeRef;
    }

    if (ni_IsWriteable(iteratorRef))
    {
        LE_ASSERT(treeRef->activeWriteIterRef == NULL);
        treeRef->activeWriteIterRef = iteratorRef;
        LE_ASSERT(treeRef->activeWriteIterRef != NULL);
    }
    else
    {
        treeRef->activeReadCount++;
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Remove a prior iterator registration from a tree object.
 */
// -------------------------------------------------------------------------------------------------
void tdb_UnregisterIterator
(
    tdb_TreeRef_t treeRef,        ///< [IN] The tree object to update.
    ni_IteratorRef_t iteratorRef  ///< [IN] The iterator object we're removing from the tree.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(treeRef != NULL);
    LE_ASSERT(iteratorRef != NULL);

    if (treeRef->originalTreeRef != NULL)
    {
        treeRef = treeRef->originalTreeRef;
    }

    if (ni_IsWriteable(iteratorRef))
    {
        LE_FATAL_IF(treeRef->activeWriteIterRef != iteratorRef,
                    "Internal error, unregistering write iterator <%p>, "
                    "but tree had write iterator <%p> registered on tree <%p>.",
                    iteratorRef,
                    treeRef->activeWriteIterRef,
                    treeRef);

        treeRef->activeWriteIterRef = NULL;
    }
    else
    {
        treeRef->activeReadCount--;
        LE_ASSERT(treeRef->activeReadCount >= 0);
    }

    if (treeRef->isDeletePending)
    {
        tdb_DeleteTree(treeRef);
    }
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(treeRef != NULL);

    if (treeRef->originalTreeRef != NULL)
    {
        return &treeRef->originalTreeRef->requestList;
    }

    return &treeRef->requestList;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Merge a shadow tree into the original tree it was created from.  Once the change is merged the
 *  updated tree is serialized to the filesystem.
 */
// -------------------------------------------------------------------------------------------------
void tdb_MergeTree
(
    tdb_TreeRef_t shadowTreeRef  ///< [IN] Merge the nodes from this tree into their base tree.
)
// -------------------------------------------------------------------------------------------------
{
    // Get our shadow tree's root node and merge it's changes into the real tree.  Create a path
    // iterator to track the merge and allow for update handlers to be called.
    tdb_NodeRef_t nodeRef = shadowTreeRef->rootNodeRef;
    le_pathIter_Ref_t pathRef = CreateBasePath(shadowTreeRef->originalTreeRef->name);

    InternalMergeTree(shadowTreeRef->originalTreeRef->name, pathRef, nodeRef, false);
    le_pathIter_Delete(pathRef);

    // Now, go through and call the triggered callbacks.
    FireTriggeredCallbacks();

    // Now increment revision of the tree and open a tree file for writing.
    tdb_TreeRef_t originalTreeRef = shadowTreeRef->originalTreeRef;
    int oldId = originalTreeRef->revisionId;

    IncrementRevision(originalTreeRef);

    char filePath[LE_CFG_STR_LEN_BYTES] = "";
    GetTreePath(originalTreeRef->name, originalTreeRef->revisionId, filePath, sizeof(filePath));

    LE_DEBUG("Changes merged, now attempting to serialize the tree to '%s'.", filePath);

    FILE* filePtr = NULL;

    filePtr = fopen(filePath, "w+");

    if (!filePtr && (EROFS == errno))
    {
        // In case we are R/O for the config tree, we discard the update to flash
        return;
    }

    if (!filePtr)
    {
        LE_EMERG("Failed to open config file '%s' (%m).", filePath);
        LE_EMERG("Changes have been merged in memory, however they could not be committed to the "
                 "filesystem!!");
        return;
    }

    // We have a tree file to write to, so stream the new tree to it then close the output file.
    le_result_t writeResult = tdb_WriteTreeNode(originalTreeRef->rootNodeRef, filePtr);

    int retVal = fclose(filePtr);
    LE_EMERG_IF(retVal == EOF,
                "An error occurred while closing the tree file: %s", strerror(errno));

    // Finally remove the old version of the tree file, if there is one.
    if (writeResult == LE_OK)
    {
        if (   (oldId != 0)
            && (TreeFileExists(originalTreeRef->name, oldId)))
        {
            GetTreePath(originalTreeRef->name, oldId, filePath, sizeof(filePath));
            DeleteTreeFile(filePath);
        }
    }
    else
    {
        // The write failed, delete the new file we attempted to create.
        LE_EMERG("The attempt to write to the config tree file, '%s,' failed.", filePath);
        DeleteTreeFile(filePath);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Call this to realease a tree.
 */
// -------------------------------------------------------------------------------------------------
void tdb_ReleaseTree
(
    tdb_TreeRef_t treeRef  ///< [IN] The tree to free.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(treeRef != NULL);

    if (treeRef->originalTreeRef != NULL)
    {
        le_mem_Release(treeRef);
    }

    // TODO: Possibly free regular trees if there are no active iterators on it?
    //       Should timeouts be used for this?
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);
    LE_ASSERT(filePtr != NULL);

    // Clear out any contents that the node may have, and make sure that it isn't marked as deleted.
    tdb_SetEmpty(nodeRef);
    tdb_EnsureExists(nodeRef);

    // Ok read the specified node from the file object.  If the read fails, report it and clear out
    // the node.  We shouldn't be leaving the node in a half initialized state.
    bool result = true;

    // Compute starting point, how big is the path so far??
    // Must already be less than, LE_CFG_STR_LEN.
    size_t pathLen = ComputePathLength(nodeRef);

    if (pathLen >= LE_CFG_STR_LEN)
    {
        result = false;
    }
    else
    {
        if (InternalReadNode(nodeRef, filePtr, pathLen) != LE_OK)
        {
            tdb_SetEmpty(nodeRef);
            result = false;
        }

        // Make sure that there aren't any unexpected tokens left in the file.
        if (SkipWhiteSpace(filePtr) != LE_OUT_OF_RANGE)
        {
            LE_ERROR("Unexpected token in file.");
            result = false;
        }
    }

    return result;
}




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
)
// -------------------------------------------------------------------------------------------------
{
    // Write the data, then close up the file.
    return InternalWriteNode(nodeRef, filePtr);
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(baseNodeRef != NULL);
    LE_ASSERT(nodePathRef != NULL);

    // Check to see if we're starting at the given node, or that node's root node.
    tdb_NodeRef_t currentRef = GetPathBaseNodeRef(baseNodeRef, nodePathRef);

    // Now start moving along the path, moving the current node along as we go.  The called function
    // also deals with . and .. names in the path as well, returning the current and parent nodes
    // respectivly.
    char nameRef[LE_CFG_NAME_LEN_BYTES] = "";

    le_result_t result = le_pathIter_GoToStart(nodePathRef);

    while (   (result != LE_NOT_FOUND)
           && (currentRef != NULL))
    {
        result = le_pathIter_GetCurrentNode(nodePathRef, nameRef, sizeof(nameRef));

        if (result == LE_OVERFLOW)
        {
            LE_ERROR("Path segment overflow on path.");
            currentRef = NULL;
        }
        else if (result == LE_OK)
        {
            currentRef = GetNamedChild(currentRef, nameRef);
            result = le_pathIter_GoToNext(nodePathRef);
        }
    }

    // Finally return the last node we traversed to.
    return currentRef;
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(baseNodeRef != NULL);
    LE_ASSERT(nodePathRef != NULL);

    // Check to see if we're starting at the given node, or that node's root node.
    tdb_NodeRef_t currentRef = GetPathBaseNodeRef(baseNodeRef, nodePathRef);

    // Now start moving along the path, moving the current node along as we go.  The called function
    // also deals with . and .. names in the path as well, returning the current and parent nodes
    // respectivly.
    char nameRef[LE_CFG_NAME_LEN_BYTES] = "";

    le_result_t result = le_pathIter_GoToStart(nodePathRef);

    while (   (result != LE_NOT_FOUND)
           && (currentRef != NULL))
    {
        result = le_pathIter_GetCurrentNode(nodePathRef, nameRef, sizeof(nameRef));

        if (result == LE_OK)
        {
            tdb_NodeRef_t childRef = GetNamedChild(currentRef, nameRef);

            if (childRef == NULL)
            {
                childRef = CreateNamedChild(currentRef, nameRef);
            }

            currentRef = childRef;
            result = le_pathIter_GoToNext(nodePathRef);
        }
        else if (result == LE_OVERFLOW)
        {
            LE_ERROR("Path segment overflow on path.");
            currentRef = NULL;
        }
        else
        {
            LE_ERROR("Unexpected error with path segment, '%s': %d.",
                     LE_RESULT_TXT(result),
                     result);
            currentRef = NULL;
        }
    }

    // Finally return the last node we traversed to.
    return currentRef;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Make sure that the given node and any of it's parents are not marked as having been deleted.
 */
// -------------------------------------------------------------------------------------------------
void tdb_EnsureExists
(
    tdb_NodeRef_t nodeRef   ///< [IN] Update this node, and all of it's parentage and make sure none
                            ///<      of them are marked for deletion.
)
// -------------------------------------------------------------------------------------------------
{
    while (nodeRef != NULL)
    {
        ClearDeletedFlag(nodeRef);
        nodeRef = tdb_GetNodeParent(nodeRef);
    }
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);
    LE_ASSERT(stringPtr != NULL);

    stringPtr[0] = 0;

    // Get the name pointer from the ndoe.  However if this is a shadow node, then this name may be
    // NULL.  The reason that the name may be NULL is because the client never changed the name of
    // the node.  So, we just get the name from the original node, saving memory.  However, nodes
    // like the root node of a tree also do not have names.
    dstr_Ref_t nameRef = nodeRef->nameRef;

    if (   (IsShadow(nodeRef))
        && (nodeRef->nameRef == NULL)
        && (nodeRef->shadowRef != NULL))
    {
        nameRef = nodeRef->shadowRef->nameRef;
    }

    // If the node has a name, copy it into the user buffer now.
    if (nameRef != NULL)
    {
        return dstr_CopyToCstr(stringPtr, maxSize, nameRef, NULL);
    }

    return LE_OK;
}


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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    if (   (IsShadow(nodeRef))
        && (nodeRef->nameRef == NULL)
        && (nodeRef->shadowRef != NULL))
    {
        return nodeRef->shadowRef->nameHash;
    }
    else
    {
        return nodeRef->nameHash;
    }
}


// -------------------------------------------------------------------------------------------------
/**
 *  Set the name of a given node.  But also validate the name as there are certain names that nodes
 *  shouldn't have.
 *
 *  @return LE_OK if the set is successful.  LE_FORMAT_ERROR if the name contains illegial
 *          characters, or otherwise would not work as a node name.  LE_OVERFLOW if the name is too
 *          long.  LE_DUPLICATE, if there is another node with the new name in the same collection.
 */
// -------------------------------------------------------------------------------------------------
le_result_t tdb_SetNodeName
(
    tdb_NodeRef_t nodeRef,  ///< [IN] The node to update.
    const char* stringPtr   ///< [IN] New name for the node.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    if (   (stringPtr == NULL)
        || (strcmp(stringPtr, "") == 0)
        || (strcmp(stringPtr, ".") == 0)
        || (strcmp(stringPtr, "..") == 0)
        || (strchr(stringPtr, '/') != NULL)
        || (strchr(stringPtr, ':') != NULL))
    {
        return LE_FORMAT_ERROR;
    }

    // You can't change the name of the root node.
    if (nodeRef->parentRef == NULL)
    {
        return LE_FORMAT_ERROR;
    }

    if (strlen(stringPtr) > LE_CFG_NAME_LEN)
    {
        return LE_OVERFLOW;
    }

    // Check for a duplicate name in this collection.
    if (   (nodeRef->parentRef != NULL)
        && (NodeExists(nodeRef->parentRef, stringPtr) == true))
    {
        return LE_DUPLICATE;
    }

    // Copy over the new name.  Note that we don't care if this node is a shadow node.  Coping over
    // the name is taken care of as part of the merge process.
    if (nodeRef->nameRef == NULL)
    {
        nodeRef->nameRef = dstr_NewFromCstr(stringPtr);
    }
    else
    {
        dstr_CopyFromCstr(nodeRef->nameRef, stringPtr);
    }
    nodeRef->nameHash = le_hashmap_HashString(stringPtr);

    // If this is a shadow node and this is the change that modified it, then try to get it's
    // children now.  This is done so that later when this node is merged the merge code doesn't end
    // up thinking that the child nodes where removed.
    if (   (IsShadow(nodeRef) == true)
        && (IsModified(nodeRef) == false))
    {
        // Note that we don't bother checking to see if this is even a stem as tdb_GetFirstChildNode
        // will take care of that.
        tdb_GetFirstChildNode(nodeRef);
    }

    // Make sure that we know to merge this node later.  Also, if the original node has a value,
    // (that is, it's neither empty or a stem, but does have a value,) make sure that it's
    // propagated over to this shadow node.  This is so that the merging code doesn't think we've
    // emptied the value out over the course of this transaction.
    PropagateValue(nodeRef);
    SetModifiedFlag(nodeRef);

    return LE_OK;
}




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
)
// -------------------------------------------------------------------------------------------------
{
    // First, has this node been marked as deleted?
    if (   (nodeRef == NULL)
        || (IsDeleted(nodeRef)))
    {
        return LE_CFG_TYPE_DOESNT_EXIST;
    }

    // If the node is a stem but has no children, then treat the node as empty.
    if (   (nodeRef->type == LE_CFG_TYPE_STEM)
        && (tdb_GetFirstActiveChildNode(nodeRef) == NULL))
    {
        return LE_CFG_TYPE_EMPTY;
    }

    // If the node isn't a stem and there is no string value then this node is definitly empty.
    if (   (nodeRef->type != LE_CFG_TYPE_STEM)
        && (nodeRef->info.valueRef == NULL))
    {
        // Return the shadow reference if available.
        if (IsShadow(nodeRef))
        {
            return tdb_GetNodeType(nodeRef->shadowRef);
        }

        return LE_CFG_TYPE_EMPTY;
    }

    // Otherwise simply return the type recorded in this node.
    return nodeRef->type;
}




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
)
// -------------------------------------------------------------------------------------------------
{
    le_cfg_nodeType_t type = tdb_GetNodeType(nodeRef);

    return    (type == LE_CFG_TYPE_EMPTY)
           || (type == LE_CFG_TYPE_DOESNT_EXIST);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Clear out the data from a node, releasing any children it may have.
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetEmpty
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node to clear.
)
// -------------------------------------------------------------------------------------------------
{
    if (nodeRef == NULL)
    {
        return;
    }

    le_cfg_nodeType_t type = tdb_GetNodeType(nodeRef);

    // If the node is already empty then there isn't much left to do.
    if (   (type == LE_CFG_TYPE_EMPTY)
        || (type == LE_CFG_TYPE_DOESNT_EXIST))
    {
        return;
    }

    // If this is a stem node, then go through and clear out the children.
    if (nodeRef->type == LE_CFG_TYPE_STEM)
    {
        tdb_NodeRef_t childRef = tdb_GetFirstChildNode(nodeRef);

        while (childRef != NULL)
        {
            tdb_NodeRef_t nextChildRef = tdb_GetNextSiblingNode(childRef);

            // We don't remove the child from the list explicitly, because the destructor will take
            // care of that for us.
            le_mem_Release(childRef);
            childRef = nextChildRef;
        }

        nodeRef->info.children = LE_DLS_LIST_INIT;
    }
    else if (nodeRef->info.valueRef)
    {
        // It's a string value, so free it now.
        dstr_Release(nodeRef->info.valueRef);
        nodeRef->info.valueRef = NULL;
    }

    // Mark the node as being emtpy, and that it has been modified.
    nodeRef->type = LE_CFG_TYPE_EMPTY;
    SetModifiedFlag(nodeRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Delete a given node from it's tree.
 */
// -------------------------------------------------------------------------------------------------
void tdb_DeleteNode
(
    tdb_NodeRef_t nodeRef  ///< [IN] The node to delete.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    // Mark the node as having been modified.  Clear out any children, and mark the node itself as
    // deleted.  If this isn't a shadow node, then just free the memory now.
    SetModifiedFlag(nodeRef);

    if (nodeRef->type == LE_CFG_TYPE_STEM)
    {
        tdb_NodeRef_t childRef = tdb_GetFirstActiveChildNode(nodeRef);

        while (childRef != NULL)
        {
            tdb_NodeRef_t nextChildRef = tdb_GetNextActiveSiblingNode(childRef);
            tdb_DeleteNode(childRef);

            childRef = nextChildRef;
        }
    }

    if (   (IsShadow(nodeRef))
        || (tdb_GetNodeParent(nodeRef) == NULL))
    {
        SetDeletedFlag(nodeRef);
    }
    else
    {
        le_mem_Release(nodeRef);
    }
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);
    return nodeRef->parentRef;
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    // Is this the type of node that has children?
    if (   (   (nodeRef->type != LE_CFG_TYPE_STEM)
            || (le_dls_IsEmpty(&nodeRef->info.children) == true))
        && (IsShadow(nodeRef) == false))
    {
        return NULL;
    }

    // If the node is a shadow node, and it doesn't have any children call ShadowChildren to
    // propigate over the original collection of child nodes into this one.
    if (   (IsShadow(nodeRef))
        && (le_dls_Peek(&nodeRef->info.children) == NULL)
        && (nodeRef->shadowRef != NULL))
    {
        if (IsModified(nodeRef) == false)
        {
            ShadowChildren(nodeRef);
        }
    }

    // Just return the first child of this node...  Or NULL if it doesn't have one.
    le_dls_Link_t* linkPtr = le_dls_Peek(&nodeRef->info.children);

    return linkPtr == NULL ? NULL
                           : CONTAINER_OF(linkPtr, Node_t, siblingList);
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    if (nodeRef->parentRef == NULL)
    {
        return NULL;
    }

    le_dls_Link_t* linkPtr = le_dls_PeekNext(&nodeRef->parentRef->info.children,
                                             &nodeRef->siblingList);

    return linkPtr == NULL ? NULL
                           : CONTAINER_OF(linkPtr, Node_t, siblingList);
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    tdb_NodeRef_t childRef = tdb_GetFirstChildNode(nodeRef);

    if (   (childRef != NULL)
        && (IsDeleted(childRef) == true))
    {
        return tdb_GetNextActiveSiblingNode(childRef);
    }

    return childRef;
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    tdb_NodeRef_t nextPtr = tdb_GetNextSiblingNode(nodeRef);

    while (   (nextPtr != NULL)
           && (IsDeleted(nextPtr) == true))
    {
        nextPtr = tdb_GetNextSiblingNode(nextPtr);
    }

    return nextPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the nodes string value and copy into the destination buffer.
 *
 *  @return LE_OK if the value is copied ok.
 *          LE_OVERFLOW if the value can not fit in the supplied buffer.
 */
// -------------------------------------------------------------------------------------------------
le_result_t tdb_GetValueAsString
(
    tdb_NodeRef_t nodeRef,  ///< [IN]  The node object to read.
    char* stringPtr,        ///< [OUT] Target buffer for the value string.
    size_t maxSize,         ///< [IN]  Maximum size the buffer can hold.
    const char* defaultPtr  ///< [IN]  Default value to use in the event that the requested value
                            ///<       doesn't exist.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    stringPtr[0] = 0;
    le_cfg_nodeType_t type = tdb_GetNodeType(nodeRef);

    // If there is no value, just give the default value back.
    if (   (type == LE_CFG_TYPE_EMPTY)
        || (type == LE_CFG_TYPE_DOESNT_EXIST)
        || (type == LE_CFG_TYPE_STEM))
    {
        return le_utf8_Copy(stringPtr, defaultPtr, maxSize, NULL);
    }

    // Check to see if we have the value locally, or if we need to go back to the original node for
    // the value.
    if (nodeRef->info.valueRef == NULL)
    {
        if (IsShadow(nodeRef))
        {
            LE_ASSERT(nodeRef->shadowRef != NULL);
            return dstr_CopyToCstr(stringPtr, maxSize, nodeRef->shadowRef->info.valueRef, NULL);
        }

        return LE_OK;
    }

    return dstr_CopyToCstr(stringPtr, maxSize, nodeRef->info.valueRef, NULL);
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    // Make sure the node is cleared out and value is set to it's default state.
    if (   (nodeRef->type == LE_CFG_TYPE_STEM)
        || (nodeRef->type != LE_CFG_TYPE_EMPTY))
    {
        tdb_SetEmpty(nodeRef);
        nodeRef->info.valueRef = NULL;
    }

    // Mark this as a string node, and copy over the value.
    nodeRef->type = LE_CFG_TYPE_STRING;

    if (nodeRef->info.valueRef == NULL)
    {
        nodeRef->info.valueRef = dstr_NewFromCstr(stringPtr);
    }
    else
    {
        dstr_CopyFromCstr(nodeRef->info.valueRef, stringPtr);
    }

    // Make sure the system knows this node has been modified so that it can be included for merging
    // into the original tree.  Also, make sure that this node and it's parents are not marked as
    // having been deleted.
    SetModifiedFlag(nodeRef);
    tdb_EnsureExists(nodeRef);
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    bool result;

    switch (tdb_GetNodeType(nodeRef))
    {
        // If this isn't a bool node, then return the default value.
        case LE_CFG_TYPE_BOOL:
            {
                char buffer[SMALL_STR] = { 0 };

                LE_FATAL_IF(tdb_GetValueAsString(nodeRef, buffer, SMALL_STR, "") == LE_OVERFLOW,
                            "Internal error, bool value string too large.");

                if (strcmp(buffer, "f") == 0)
                {
                    result = false;
                }
                else
                {
                    result = true;
                }
            }
            break;

        default:
            result = defaultValue;
            break;
    }

    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Overwite a node value as a new boolen value..
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetValueAsBool
(
    tdb_NodeRef_t nodeRef,  ///< [IN] The node to write to.
    bool value              ///< [IN] The new value to write to that node.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    tdb_SetValueAsString(nodeRef, value ? "t" : "f");
    nodeRef->type = LE_CFG_TYPE_BOOL;
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    int result;

    switch (tdb_GetNodeType(nodeRef))
    {
        // Convert from either the underlying string directly or convert from a stored floating
        // point value.
        case LE_CFG_TYPE_INT:
            {
                char buffer[SMALL_STR] = { 0 };

                tdb_GetValueAsString(nodeRef, buffer, SMALL_STR, "");
                result = atoi(buffer);
            }
            break;

        case LE_CFG_TYPE_FLOAT:
            {
                double newValue = tdb_GetValueAsFloat(nodeRef, 0.0);
                result = (int)(newValue >= 0.0 ? newValue + 0.5 : newValue - 0.5);
            }
            break;

        default:
            result = defaultValue;
            break;
    }

    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Set an integer value to a given node, overwriting the previous value.
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetValueAsInt
(
    tdb_NodeRef_t nodeRef,  ///< [IN] The node to write to.
    int value               ///< [IN] The value to write.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    char buffer[SMALL_STR] = { 0 };
    snprintf(buffer, SMALL_STR, "%d", value);

    tdb_SetValueAsString(nodeRef, buffer);
    nodeRef->type = LE_CFG_TYPE_INT;
}




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
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    double result;

    switch (nodeRef->type)
    {
        case LE_CFG_TYPE_INT:
            result = tdb_GetValueAsInt(nodeRef, 0);
            break;

        case LE_CFG_TYPE_FLOAT:
            {
                char buffer[LE_CFG_STR_LEN_BYTES] = { 0 };

                tdb_GetValueAsString(nodeRef, buffer, LE_CFG_STR_LEN_BYTES, "");
                result = atof(buffer);
            }
            break;

        default:
            result = defaultValue;
            break;
    }

    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Overwite a given node's value with a floating point one.
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetValueAsFloat
(
    tdb_NodeRef_t nodeRef,  ///< [IN] The node to write
    double value            ///< [IN] The value to write to that node.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    char buffer[LE_CFG_STR_LEN_BYTES] = { 0 };

    snprintf(buffer, LE_CFG_STR_LEN_BYTES, "%f", value);
    tdb_SetValueAsString(nodeRef, buffer);
    nodeRef->type = LE_CFG_TYPE_FLOAT;
}




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
)
//--------------------------------------------------------------------------------------------------
{
    le_pathIter_Ref_t pathIterRef = NULL;
    char newPathBuffer[CFG_MAX_PATH_SIZE] = { 0 };

    // Check to see if the tree was specified in the request.  If it wasn't then add the user's tree
    // to the path now.
    if (tp_PathHasTreeSpecifier(pathPtr))
    {
        pathIterRef = le_pathIter_CreateForUnix(pathPtr);
    }
    else
    {
        snprintf(newPathBuffer, sizeof(newPathBuffer), "%s:%s", treeRef->name, pathPtr);
        pathIterRef = le_pathIter_CreateForUnix(newPathBuffer);
    }

    // Get the normalized path out of the iterator object.  If the tree specifier got removed for
    // any reason during the normalization, or if the internal path exceeded our buffer then return
    // failure now.
    le_result_t result = le_pathIter_GetPath(pathIterRef, newPathBuffer, sizeof(newPathBuffer));
    le_pathIter_Delete(pathIterRef);

    if (result != LE_OK)
    {
        LE_ERROR("Change registration path error, %d: '%s'.", result, LE_RESULT_TXT(result));
        return NULL;
    }

    if (tp_PathHasTreeSpecifier(newPathBuffer) == false)
    {
        LE_ERROR("Failed to set tree for event registration.");
        return NULL;
    }

    // Find the registration object for the given node, if it currently exists.
    Registration_t* foundRegistrationPtr = le_hashmap_Get(HandlerRegistrationMap, newPathBuffer);

    if (foundRegistrationPtr == NULL)
    {
        // Looks like a registration object hasn't been created yet.  So, do so now and add it to
        // our map.
        foundRegistrationPtr = le_mem_ForceAlloc(RegistrationPool);

        foundRegistrationPtr->triggered = false;

        foundRegistrationPtr->handlerList = LE_DLS_LIST_INIT;
        le_utf8_Copy(foundRegistrationPtr->registrationPath,
                     newPathBuffer,
                     sizeof(foundRegistrationPtr->registrationPath),
                     NULL);

        le_hashmap_Put(HandlerRegistrationMap,
                       foundRegistrationPtr->registrationPath,
                       foundRegistrationPtr);
    }

    // Add this handler to the registration object to keep track of it for later.
    Handler_t* handlerObjectPtr = le_mem_ForceAlloc(HandlerPool);

    handlerObjectPtr->link = LE_DLS_LINK_INIT;
    handlerObjectPtr->sessionRef = sessionRef;
    handlerObjectPtr->handlerPtr = handlerPtr;
    handlerObjectPtr->contextPtr = contextPtr;
    handlerObjectPtr->registrationPtr = foundRegistrationPtr;
    handlerObjectPtr->safeRef = le_ref_CreateRef(HandlerSafeRefMap, handlerObjectPtr);

    le_dls_Queue(&foundRegistrationPtr->handlerList, &handlerObjectPtr->link);

    return handlerObjectPtr->safeRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Deregisters a handler function that was registered using tdb_AddChangeHandler().
 */
//--------------------------------------------------------------------------------------------------
void tdb_RemoveChangeHandler
(
    le_cfg_ChangeHandlerRef_t handlerRef,  ///< [IN] Reference returned by tdb_AddChangeHandler().
    le_msg_SessionRef_t sessionRef         ///< [IN] The session of the user making this request.
)
//--------------------------------------------------------------------------------------------------
{
    // Simply look up the handler object in the safe ref map, then make sure that the object was
    // found and belongs to the user session.
    Handler_t* handlerObjectPtr = le_ref_Lookup(HandlerSafeRefMap, handlerRef);

    if (   (handlerObjectPtr != NULL)
        && (handlerObjectPtr->sessionRef == sessionRef))
    {
        Registration_t* registrationPtr = handlerObjectPtr->registrationPtr;

        // Remove the handler object from the registration object's list.
        RemoveHandler(registrationPtr, handlerObjectPtr);

        // If there are no more handlers in this registration object, kill the object.
        if (le_dls_IsEmpty(&registrationPtr->handlerList))
        {
            le_hashmap_Remove(HandlerRegistrationMap, registrationPtr->registrationPath);
            le_mem_Release(registrationPtr);
        }
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Clean out any event handlers registered on the given session.
 */
//--------------------------------------------------------------------------------------------------
void tdb_CleanUpHandlers
(
    le_msg_SessionRef_t sessionRef  ///< [IN] The session that's been closed.
)
//--------------------------------------------------------------------------------------------------
{
    // Go through all of the registration objects and their registered event handlers.  Remove any
    // that belong to the given session.  If the reg object is rendered empty by this, then queue
    // up that object in the supplied delete queue.
    CleanUpContext_t cleanUpContext = { sessionRef, LE_SLS_LIST_INIT };
    le_hashmap_ForEach(HandlerRegistrationMap, OnHandlerRegistrationCleanup, &cleanUpContext);

    // Now.  Go through the cleanUpContext delete queue and remove the registration object found
    // within.
    le_sls_Link_t* linkPtr = NULL;

    while ((linkPtr = le_sls_Pop(&cleanUpContext.deleteQueue)) != NULL)
    {
        Registration_t* registrationPtr = CONTAINER_OF(linkPtr, Registration_t, link);

        le_hashmap_Remove(HandlerRegistrationMap, registrationPtr->registrationPath);
        le_mem_Release(registrationPtr);
    }
}



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
)
{
    return BinaryDataPool;
}



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
)
{
    return EncodedStringPool;
}
