
// -------------------------------------------------------------------------------------------------
/**
 * @file treeDb.c
 *
 * Implementation of the low level tree DB structure.  This code also handles the persisting of the
 * tree db to the filesystem.
 *
 * The tree structure looks like this:
 *
@verbatim

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
 * The Tree Collection holds Tree objects. There's one Tree object for each configuration tree.
 * They are indexed by tree name.
 *
 * Each Tree object has a single "root" Node.
 *
 * Each Node can have either a value or a list of child Nodes.
 *
 * When a write transaction is started for a Tree, the iterator reference for that transaction
 * is recorded in the Tree object.  When the transaction is committed or cancelled, that reference
 * is cleared out.
 *
 * When a read transaction is started for a Tree, the count of read iterators in that Tree is
 * incremented.  When it ends, the count is decremented.
 *
 * When client requests are received that cannot be processed immediately, because of the state
 * of the tree the request is for (e.g., if a write transaction commit request is received while
 * there are read transactions in progress on the tree), then the request is queued onto the tree's
 * Request Queue.
 *
 * <b>Shadow Trees:</b>
 *
 * In addition, there's the notion of a "Shadow Tree", which is a tree that contains changes
 * that have been made to another tree in a write transaction that has not yet been committed.
 * Each node in a shadow tree is called a "Shadow Node".
 *
 * When a write transaction is started on a tree, a shadow tree is created for that tree, and
 * a shadow node is created for the root node.  As a shadow node is traversed (using the normal
 * tree traversal functions), new shadow nodes are created for any nodes that have been traversed
 * to and any of their sibling nodes.  When changes are made to a node, the new value is stored
 * in the shadow node.  When new nodes are added, a new shadow node is created in the shadow
 * tree.  When nodes are deleted, the shadow node is marked "deleted".
 *
 * When a write transaction is cancelled, the shadow tree and all its shadow nodes are discarded.
 *
 * When a write transaction is committed, the shadow tree is traversed, and any changes found
 * in it are applied to the "original" tree that the shadow tree was shadowing.  This process is
 * called "merging".
 *
 * Shadow Trees don't have handlers, request queues, write iterator references or read iterator
 * counts.
 *
 * ----
 *  Copyright (C) Sierra Wireless, Inc. 2013, 2014. All rights reserved. Use of this work is subject
 *  to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "stringBuffer.h"
#include "dynamicString.h"
#include "treePath.h"
#include "treeDb.h"
#include "treeUser.h"
#include "nodeIterator.h"




/// Path to the config tree directory in the linux filesystem.
#define CFG_TREE_PATH "/opt/legato/configTree"



/// Maximum size (in bytes) of a "small" string, including the null terminator.
#define SMALL_STR 24




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



//--------------------------------------------------------------------------------------------------
/**
 * Change notification handler object structure. (aka "Handler objects")
 *
 * Each one of these is used to keep track of a client's change notification handler function
 * registration for a particular tree node.  These are allocated from the Handler Pool and kept
 * on a Node object's Handler List.
 **/
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t link;                     ///< Used to link into the Node object's Handler List.
    le_cfg_ChangeHandlerFunc_t handlerPtr;  ///< Function to call back.
    void* contextPtr;                       ///< Context to give the function when called.
}
Handler_t;




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

    le_dls_Link_t siblingList;       ///< The linked list of node siblings.  All of the nodes
                                     ///<   in this list have the same parent node.

    le_dls_List_t handlerList;       ///< List of change notification handler objects registered
                                     ///    for this node.

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
    struct Tree* originalTreeRef;     ///< If non-NULL then this points back to the original
                                          ///<   tree this one is shadowing.

    char name[MAX_TREE_NAME];             ///< The name of this tree.

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




/// The memory pool responsible for tree nodes.
le_mem_PoolRef_t NodePoolRef = NULL;
#define CFG_NODE_POOL_NAME "nodePool"

/// The collection of configuration trees managed by the system.
le_hashmap_Ref_t TreeCollectionRef = NULL;

/// Pool from which Tree objects are allocated.
le_mem_PoolRef_t TreePoolRef = NULL;

#define CFG_TREE_COLLECTION_NAME "treeCollection"
#define CFG_TREE_POOL_NAME       "treePool"




// -------------------------------------------------------------------------------------------------
/**
 *  Clear all flags from the given node.
 */
// -------------------------------------------------------------------------------------------------
static void ClearFlags
(
    tdb_NodeRef_t nodeRef  ///< The node to update.
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
    const tdb_NodeRef_t nodeRef  ///< The node to check.
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
    tdb_NodeRef_t nodeRef  ///< The node to update.
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
static bool IsModifed
(
    const tdb_NodeRef_t nodeRef  ///< The node to read.
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
    tdb_NodeRef_t nodeRef  ///< The node to update.
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
    tdb_NodeRef_t nodeRef  ///< The node to update.
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
    tdb_NodeRef_t nodeRef  ///< The node to read.
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
    tdb_NodeRef_t nodeRef  ///<
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
    tdb_NodeRef_t nodeRef  ///< The node to update.
)
// -------------------------------------------------------------------------------------------------
{
    nodeRef->flags &= ~NODE_IS_DELETED;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Allocate a new node and fill out it's default information.
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
    void* objectPtr  ///< The generic object to free.
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
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t NewShadowNode
(
    tdb_NodeRef_t nodeRef  ///< The node to shadow.
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
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t NewChildNode
(
    tdb_NodeRef_t nodeRef  ///< The node to be given with a new child.
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
    tdb_NodeRef_t shadowParentRef  ///< The node we're reading.
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
    if (IsModifed(shadowParentRef) == true)
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
    tdb_NodeRef_t nodeRef  ///< Find the greatest grand parent of this node.
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
 *  Called to look for a named child in a collection.  If the given node shadows another and the
 *  child wasn't found in this collection the search then folloows up the shadow chain.
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t GetNamedChild
(
    tdb_NodeRef_t nodeRef,  ///< The node to search.
    const char* nameRef     ///< The name we're searching for.
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

    // If this is a shdadow node, and the current node isn't a stem, then convert this node into an
    // empty stem node now.
    if (   (IsShadow(nodeRef))
        && (nodeRef->type != LE_CFG_TYPE_STEM))
    {
        if (nodeRef->type != LE_CFG_TYPE_EMPTY)
        {
            tdb_SetEmpty(nodeRef);
        }

        nodeRef->type = LE_CFG_TYPE_STEM;
        nodeRef->info.children = LE_DLS_LIST_INIT;
    }

    // If the node still isn't a stem at this point then it can not possibly have children.
    if (nodeRef->type != LE_CFG_TYPE_STEM)
    {
        return NULL;
    }

    // Search the child list for a node with the given name.
    tdb_NodeRef_t currentRef = tdb_GetFirstChildNode(nodeRef);
    char* currentNameRef = sb_Get();

    while (currentRef != NULL)
    {
        tdb_GetNodeName(currentRef, currentNameRef, SB_SIZE);

        if (strncmp(currentNameRef, nameRef, SB_SIZE) == 0)
        {
            sb_Release(currentNameRef);
            return currentRef;
        }

        currentRef = tdb_GetNextSiblingNode(currentRef);
    }

    sb_Release(currentNameRef);

    // At this point the node has not been found.  Check to see if we can create a new node.  If we
    // can, do so now and add it to the parent's list.  But mark is as deleted as this node does not
    // officially exist yet.  (The deleted flag will be removed if this node or one if it's
    // children has a value written to it.)
    if (IsShadow(nodeRef))
    {
        tdb_NodeRef_t childRef = NewChildNode(nodeRef);

        if (tdb_SetNodeName(childRef, nameRef) == LE_OK)
        {
            return childRef;
        }
        else
        {
            le_mem_Release(childRef);
        }
    }

    // Nope, no creation was allowed, so there is no node to return.
    return NULL;
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
    tdb_NodeRef_t parentRef,  ///< The parent node to search.
    const char* namePtr       ///< The name to search for.
)
// -------------------------------------------------------------------------------------------------
{
    tdb_NodeRef_t currentRef = tdb_GetFirstChildNode(parentRef);
    char currentName[MAX_NODE_NAME] = { 0 };

    while (currentRef != NULL)
    {
        tdb_GetNodeName(currentRef, currentName, MAX_NODE_NAME);

        if (strncmp(currentName, namePtr, MAX_NODE_NAME) == 0)
        {
            return true;
        }

        currentRef = tdb_GetNextSiblingNode(currentRef);
    }

    return false;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Merge a shadow node with the original it represents.
 */
// -------------------------------------------------------------------------------------------------
static void MergeNode
(
    tdb_NodeRef_t nodeRef  ///< The shadow node to merge.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    // This node is being merged, so make sure that it isn't marked as modified any more.
    ClearModifiedFlag(nodeRef);

    // If this shadow node for some reason doesn't have a ref check for an original version of it in
    // the original tree.  This shadow node may have been destroyed and re-created loosing this
    // link.
    if (nodeRef->shadowRef == NULL)
    {
        tdb_NodeRef_t shadowedParentRef = tdb_GetNodeParent(nodeRef->parentRef->shadowRef);

        if (shadowedParentRef != NULL)
        {
            char name[MAX_NODE_NAME] = { 0 };

            tdb_GetNodeName(nodeRef, name, MAX_NODE_NAME);
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
    }


    // Check the types of the original and the shadow nodes.  If the new node has been cleared.
    // Then clear out the original node.  If one is a stem and the other isn't, clear out the
    // original because things are going to be changing.
    le_cfg_nodeType_t nodeType = tdb_GetNodeType(nodeRef);
    le_cfg_nodeType_t originalType = tdb_GetNodeType(originalRef);

    if (   (nodeType == LE_CFG_TYPE_EMPTY)
        || (   (originalType == LE_CFG_TYPE_STEM)
            && (nodeType != LE_CFG_TYPE_STEM))
        || (   (originalType != LE_CFG_TYPE_STEM)
            && (nodeType == LE_CFG_TYPE_STEM)))
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
 *  Recursive function to merge a collection of shadow nodes with the original tree.
 */
// -------------------------------------------------------------------------------------------------
static void InternalMergeTree
(
    tdb_NodeRef_t nodeRef  ///< Node and any children to merge.
)
// -------------------------------------------------------------------------------------------------
{
    if (IsModifed(nodeRef))
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

            InternalMergeTree(nodeRef);
            nodeRef = nextNodeRef;
        }
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Make sure that the given node and any of it's parents are not marked as having been deleted.
 */
// -------------------------------------------------------------------------------------------------
static void EnsureExists
(
    tdb_NodeRef_t nodeRef   ///< Update this node, and all of it's parentage and make sure none of
                            ///<   them are marked for deletion.
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
 *  Create a new tree object and set it to default values.
 */
// -------------------------------------------------------------------------------------------------
tdb_TreeRef_t NewTree
(
    const char* treeNameRef,   ///< The name of the new tree.
    tdb_NodeRef_t rootNodeRef  ///< The root node of this new tree.
)
// -------------------------------------------------------------------------------------------------
{
    tdb_TreeRef_t treeRef = le_mem_ForceAlloc(TreePoolRef);

    strncpy(treeRef->name, treeNameRef, MAX_TREE_NAME);

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
 *  Create a path to a tree file with the given revision id.
 */
// -------------------------------------------------------------------------------------------------
static char* GetTreePath
(
    const char* treeNameRef,  ///< The name of the tree we're generating a name for.
    int revisionId            ///< Generate a name based on the tree revision.
)
// -------------------------------------------------------------------------------------------------
{
    // paper    --> rock       1 -> 2
    // rock     --> scissors   2 -> 3
    // scissors --> paper      3 -> 1

    static const char* revNames[] = { "paper", "rock", "scissors" };
    char* fullPathPtr = NULL;

    LE_ASSERT((revisionId >= 1) && (revisionId <= 3));

    fullPathPtr = sb_Get();

    snprintf(fullPathPtr,
             SB_SIZE,
             "%s/%s.%s",
             CFG_TREE_PATH,
             treeNameRef,
             revNames[revisionId - 1]);

    return fullPathPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check to see if a configTree file at the given revision already exists in the filesystem.
 */
// -------------------------------------------------------------------------------------------------
static bool TreeFileExists
(
    const char* treeNameRef,  ///< Name of the tree to check.
    int revisionId            ///< The revision of the tree to check against.
)
// -------------------------------------------------------------------------------------------------
{
    char* fullPathPtr = GetTreePath(treeNameRef, revisionId);

    // Make sure this part was successful.
    if (fullPathPtr == NULL)
    {
        return false;
    }

    // Now call into the Linux and ask the file system if the file exists.
    bool result = false;

    if (access(fullPathPtr, R_OK) != -1)
    {
        result = true;
    }

    sb_Release(fullPathPtr);

    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check the filesystem and get the "valid" version of the file.  That is, if there are two files
 *  for a given tree, we use the older one.  The idea being, if there are two versions of the same
 *  file in the filesystem then it's highly likely there was a system failure during a streaming
 *  operation.  So we abandon the newer file and go with the older more reliable file.
 */
// -------------------------------------------------------------------------------------------------
static void UpdateRevision
(
    tdb_TreeRef_t treeRef  ///< Update the revision for this tree object.
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

        newRevision = 1;
    }
    else if (TreeFileExists(treeRef->name, 3))
    {
        if (TreeFileExists(treeRef->name, 2))
        {
            newRevision = 2;
        }

        newRevision = 3;
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
    FILE* filePtr  ///< The file stream to peek into.
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
    FILE* filePtr  ///< The file stream to seek through.
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
 *  Read a boolean literal from the input file.
 *
 *  @return LE_OK if the literal could be read.
 *          LE_FORMAT_ERROR if the literal could not be read.
 */
// -------------------------------------------------------------------------------------------------
static bool ReadBoolToken
(
    FILE* filePtr,     ///< The file we're reading from.
    char* stringPtr,   ///< String buffer to hold the token we've read.
    size_t stringSize  ///< How big is the supplied string buffer?
)
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
 */
// -------------------------------------------------------------------------------------------------
static le_result_t ReadTextLiteral
(
    FILE* filePtr,        ///< The file we're reading from.
    char* stringPtr,      ///< String buffer to hold the token we've read.
    size_t stringSize,    ///< How big is the supplied string buffer?
    signed char terminal  ///< The terminal character we're searching for.
)
// -------------------------------------------------------------------------------------------------
{
    signed char next;
    size_t count = 0;

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

        *stringPtr = next;

        ++stringPtr;
        ++count;

        if (count >= (stringSize - 1))
        {
            *stringPtr = 0;

            LE_ERROR("String literal too large.");
            return LE_FORMAT_ERROR;
        }
    }

    *stringPtr = 0;

    return LE_OK;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read an integer token string from the file.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t ReadIntToken
(
    FILE* filePtr,     ///< The file we're reading from.
    char* stringPtr,   ///< String buffer to hold the token we've read.
    size_t stringSize  ///< How big is the supplied string buffer?
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
 */
// -------------------------------------------------------------------------------------------------
static le_result_t ReadFloatToken
(
    FILE* filePtr,     ///< The file we're reading from.
    char* stringPtr,   ///< String buffer to hold the token we've read.
    size_t stringSize  ///< How big is the supplied string buffer?
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
 */
// -------------------------------------------------------------------------------------------------
static le_result_t ReadStringToken
(
    FILE* filePtr,     ///< The file we're reading from.
    char* stringPtr,   ///< String buffer to hold the token we've read.
    size_t stringSize  ///< How big is the supplied string buffer?
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
 */
// -------------------------------------------------------------------------------------------------
static le_result_t ReadToken
(
    FILE* filePtr,         ///< The file we're reading from.
    char* stringPtr,       ///< String buffer to hold the token we've read.
    size_t stringSize,     ///< How big is the supplied string buffer?
    TokenType_t* typePtr   ///< OUT: The type of token read from the file.
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
 *  Read a node value from the given file.  If the value is a collection, then read in those nodes
 *  too.
 *
 *  @return LE_OK if the read is scuccessful.
 *          LE_FORMAT_ERROR if parse errors are encountered.
 *          LE_NOT_FOUND if the end of file is reached.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t InternalReadNode
(
    tdb_NodeRef_t nodeRef,  ///< The node we're reading a value for.
    FILE* filePtr           ///< The file we're reading the value from.
)
// -------------------------------------------------------------------------------------------------
{
    static char stringBuffer[MAX_NODE_NAME] = { 0 };

    TokenType_t tokenType;

    // Try to read this node's value.
    if (ReadToken(filePtr, stringBuffer, MAX_NODE_NAME, &tokenType) != LE_OK)
    {
        LE_ERROR("Unexpected EOF or bad token in file.");
        return LE_FORMAT_ERROR;
    }

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
                if (ReadToken(filePtr, stringBuffer, MAX_NODE_NAME, &tokenType) != LE_OK)
                {
                    LE_ERROR("Unexpected EOF or bad token in file while looking for '}'.");
                    return LE_FORMAT_ERROR;
                }

                if (tokenType == TT_STRING_VALUE)
                {
                    tdb_NodeRef_t childRef = GetNamedChild(nodeRef, stringBuffer);

                    if (childRef == NULL)
                    {
                        childRef = NewChildNode(nodeRef);
                        if (tdb_SetNodeName(childRef, stringBuffer) != LE_OK)
                        {
                            LE_ERROR("Bad node name, '%s'.", stringBuffer);
                            return LE_FORMAT_ERROR;
                        }

                        LE_DEBUG("New node, %s", stringBuffer);
                    }

                    EnsureExists(childRef);

                    le_result_t result = InternalReadNode(childRef, filePtr);

                    if (result != LE_OK)
                    {
                        return result;
                    }
                }
                else if (tokenType == TT_CLOSE_GROUP)
                {
                    break;
                }
                else
                {
                    LE_ERROR("Unexpected token in found while looking for '}'.");
                    return LE_FORMAT_ERROR;
                }
            }
            break;

        case TT_CLOSE_GROUP:
        default:
            LE_ERROR("Unexpected token found.");
            return LE_FORMAT_ERROR;
    }

    return LE_OK;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a string token to the output stream.  This function will write the string and escape all
 *  control characters as it does so.
 */
// -------------------------------------------------------------------------------------------------
static void WriteStringValue
(
    int descriptor,        ///< The file to write to.
    char startChar,        ///< The delimiter to use.
    char endChar,          ///< The closing delimiter to use.
    const char* stringPtr  ///< The actual string to write.
)
// -------------------------------------------------------------------------------------------------
{
    write(descriptor, &startChar, 1);

    while (*stringPtr != 0)
    {
        if (   (*stringPtr == '\"')
            || (*stringPtr == '\\'))
        {
            write(descriptor, "\\", 1);
        }

        write(descriptor, stringPtr, 1);
        stringPtr++;
    }

    write(descriptor, &endChar, 1);
    write(descriptor, " ", 1);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Bump up the version id of this tree.
 */
// -------------------------------------------------------------------------------------------------
static void IncrementRevision
(
    tdb_TreeRef_t treeRef
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
    tdb_TreeRef_t treeRef  ///< The tree object to load from the filesystem.
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
        char* pathPtr = GetTreePath(treeRef->name, treeRef->revisionId);

        LE_DEBUG("** Loading configuration tree from <%s>.", pathPtr);

        int fileRef = -1;

        do
        {
            fileRef = open(pathPtr, O_RDONLY);
        }
        while ((fileRef == -1) && (errno == EINTR));

        EnsureExists(treeRef->rootNodeRef);

        if (fileRef == -1)
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

            int retVal = -1;

            do
            {
                retVal = close(fileRef);
            }
            while ((retVal == -1) && (errno == EINTR));
        }

        sb_Release(pathPtr);
    }
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
    NodePoolRef = le_mem_CreatePool(CFG_NODE_POOL_NAME, sizeof(Node_t));
    le_mem_SetDestructor(NodePoolRef, NodeDestructor);

    TreePoolRef = le_mem_CreatePool(CFG_TREE_POOL_NAME, sizeof(Tree_t));
    TreeCollectionRef = le_hashmap_Create(CFG_TREE_COLLECTION_NAME,
                                          31,
                                          le_hashmap_HashString,
                                          le_hashmap_EqualsString);

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
    const char* treeNameRef  ///< The tree to load.
)
// -------------------------------------------------------------------------------------------------
{
    // Check to see if we have this tree loaded up in our map.
    tdb_TreeRef_t treeRef = le_hashmap_Get(TreeCollectionRef, treeNameRef);

    if (treeRef == NULL)
    {
        // Looks like we don't so create an object for it, and add it to our map.
        treeRef = NewTree(treeNameRef, NULL);
        le_hashmap_Put(TreeCollectionRef, treeRef->name, treeRef);

        LoadTree(treeRef);
    }

    // Finally return the tree we have to the user.
    return treeRef;
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
    tdb_TreeRef_t treeRef  ///< The tree to shadow.
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
    tdb_TreeRef_t treeRef  ///< The tree object to read.
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
    tdb_TreeRef_t treeRef  ///< The tree object to read.
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
    tdb_TreeRef_t treeRef  ///< The tree object to read.
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
    tdb_TreeRef_t treeRef  ///< The tree object to read.
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
    tdb_TreeRef_t treeRef,        ///< The tree object to update.
    ni_IteratorRef_t iteratorRef  ///< The iterator object we're registering.
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
    tdb_TreeRef_t treeRef,        ///< The tree object to update.
    ni_IteratorRef_t iteratorRef  ///< The iterator object we're removing from the tree.
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
    tdb_TreeRef_t treeRef  ///< The tree object to read.
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
    tdb_TreeRef_t shadowTreeRef  ///< Merge the ndoes from this tree into their base tree.
)
// -------------------------------------------------------------------------------------------------
{
    // Get our shadow tree's root node and merge it's changes into the real tree.
    tdb_NodeRef_t nodeRef = shadowTreeRef->rootNodeRef;
    InternalMergeTree(nodeRef);

    // Now increment revision of the tree and open a tree file for writing.
    tdb_TreeRef_t originalTreeRef = shadowTreeRef->originalTreeRef;
    int oldId = originalTreeRef->revisionId;

    IncrementRevision(originalTreeRef);
    char* newFilePath = GetTreePath(originalTreeRef->name, originalTreeRef->revisionId);

    LE_DEBUG("Changes mearged, now attempting to serialize the tree to <%s>.", newFilePath);

    int fileRef = -1;

    do
    {
        fileRef = open(newFilePath, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    }
    while (   (fileRef == -1)
           && (errno == EINTR));

    sb_Release(newFilePath);

    if (fileRef == -1)
    {
        LE_EMERG("Changes have been merged in memory, however they could not be committed to the "
                 "filesystem!!  Reason: %s", strerror(errno));
        return;
    }

    // We have a tree file to write to, so stream the new tree to it then close the output file.
    tdb_WriteTreeNode(originalTreeRef->rootNodeRef, fileRef);

    int retVal = -1;

    do
    {
        retVal = close(fileRef);
    }
    while ((retVal == -1) && (errno == EINTR));

    LE_EMERG_IF(retVal == -1, "An error occured while closing the tree file: %s", strerror(errno));


    // Finally remove the old version of the tree file, if there is one.
    if (   (oldId != 0)
        && (TreeFileExists(originalTreeRef->name, oldId)))
    {
        char* oldFilePath = GetTreePath(originalTreeRef->name, oldId);

        LE_DEBUG("Removing obsolete tree file, <%s>.", oldFilePath);

        unlink(oldFilePath);
        sb_Release(oldFilePath);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Call this to realease a tree.
 */
// -------------------------------------------------------------------------------------------------
void tdb_ReleaseTree
(
    tdb_TreeRef_t treeRef  ///< The tree to free.  Note that this doesn't have to be the root node.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(treeRef != NULL);

    if (treeRef->originalTreeRef != NULL)
    {
        le_mem_Release(treeRef->rootNodeRef);
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
    tdb_NodeRef_t nodeRef,  ///< The node to write the new data to.
    int descriptor          ///< The file to read from.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);
    LE_ASSERT(descriptor != -1);

    // Clear out any contents that the node may have, and make sure that it isn't marked as deleted.
    tdb_SetEmpty(nodeRef);
    EnsureExists(nodeRef);


    // Duplicate the file descriptor, this is because we later use a C library file pointer for the
    // parsing routines.  When the file pointer is closed it also closes the underlying descriptor,
    // which may not be what the caller wants or expects.
    int newDescriptor = -1;

    do
    {
        newDescriptor = dup(descriptor);
    }
    while (   (newDescriptor == -1)
           && (errno == EINTR));

    if (newDescriptor == -1)
    {
        LE_ERROR("Could not duplicate file descriptor, reason: %s", strerror(errno));
        return false;
    }


    FILE* filePtr = fdopen(newDescriptor, "r");

    if (filePtr == NULL)
    {
        int oldErrno = errno;
        int closeResult;

        do
        {
            closeResult = close(newDescriptor);
        }
        while (   (closeResult == -1)
               && (errno == EINTR));

        LE_ERROR("Could not access the input stream for tree import, reason: %s",
                 strerror(oldErrno));
        return false;
    }

    // Ok read the specified node from the file object.  If the read fails, report it and clear out
    // the node.  We shouldn't be leaving the node in a half initialized state.
    bool result = true;

    if (InternalReadNode(nodeRef, filePtr) != LE_OK)
    {
        tdb_SetEmpty(nodeRef);
        result = false;
    }

    // Make sure that there aren't any unexpected tokens left in the file.
    if (SkipWhiteSpace(filePtr) != LE_OUT_OF_RANGE)
    {
        LE_ERROR("Unexpected token in file.");
        return false;
    }

    // Finally close our file object and return the result.
    int closeResult = EOF;

    do
    {
        closeResult = fclose(filePtr);
    }
    while (   (closeResult == EOF)
           && (errno == EINTR));

    if (closeResult == EOF)
    {
        LE_ERROR("Could not properly close file, reason: %s", strerror(errno));
    }


    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Serialize a tree node and it's children to a file in the filesystem.
 */
// -------------------------------------------------------------------------------------------------
void tdb_WriteTreeNode
(
    tdb_NodeRef_t nodeRef,  ///< Write the contents of this node to a file descriptor.
    int descriptor          ///< The file descriptor to write to.
)
// -------------------------------------------------------------------------------------------------
{
    // If the node is marked as having been deleted, don't save it.
    if (IsDeleted(nodeRef))
    {
        return;
    }

    // Get the node's value as a string.
    static char stringBuffer[MAX_NODE_NAME];

    tdb_GetValueAsString(nodeRef, stringBuffer, MAX_NODE_NAME, "");

    // Now, depending on the type of node, write out any required format information.
    switch (nodeRef->type)
    {
        case LE_CFG_TYPE_EMPTY:
            write(descriptor, "~ ", 2);
            break;

        case LE_CFG_TYPE_BOOL:
            write(descriptor, "!", 1);
            write(descriptor, stringBuffer, 1);
            write(descriptor, " ", 1);
            break;

        case LE_CFG_TYPE_STRING:
            WriteStringValue(descriptor, '\"', '\"', stringBuffer);
            break;

        case LE_CFG_TYPE_INT:
            WriteStringValue(descriptor, '[', ']', stringBuffer);
            break;

        case LE_CFG_TYPE_FLOAT:
            WriteStringValue(descriptor, '(', ')', stringBuffer);
            break;

        // Looks like this node is a collection, so write out it's child nodes now.
        case LE_CFG_TYPE_STEM:
            {
                write(descriptor, "{ ", 2);

                tdb_NodeRef_t childRef = tdb_GetFirstActiveChildNode(nodeRef);

                while (childRef != NULL)
                {
                    tdb_GetNodeName(childRef, stringBuffer, MAX_NODE_NAME);
                    WriteStringValue(descriptor, '\"', '\"', stringBuffer);

                    tdb_WriteTreeNode(childRef, descriptor);

                    childRef = tdb_GetNextActiveSiblingNode(childRef);
                }

                write(descriptor, "} ", 2);
            }
            break;

        // Not much to do here.
        case LE_CFG_TYPE_DOESNT_EXIST:
            break;
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Given a base node and a path, find another node in the tree.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetNode
(
    tdb_NodeRef_t baseNodeRef,     ///< The base node to start from.
    le_pathIter_Ref_t nodePathRef  ///< The path we're searching for in the tree.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(baseNodeRef != NULL);
    LE_ASSERT(nodePathRef != NULL);

    // Check to see if we're starting at the given node, or that node's root node.
    tdb_NodeRef_t currentRef = baseNodeRef;

    if (le_pathIter_IsAbsolute(nodePathRef))
    {
        currentRef = GetRootParentNode(currentRef);
    }

    // Now start moving along the path, moving the current node along as we go.  The called function
    // also deals with . and .. names in the path as well, returning the current and parent nodes
    // respectivly.
    char nameRef[MAX_NODE_NAME] = { 0 };

    le_result_t result = le_pathIter_GoToStart(nodePathRef);

    while (   (result != LE_NOT_FOUND)
           && (currentRef != NULL))
    {
        result = le_pathIter_GetCurrentNode(nodePathRef, nameRef, MAX_NODE_NAME);

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
 *  Get the name of a given node.
 */
// -------------------------------------------------------------------------------------------------
le_result_t tdb_GetNodeName
(
    tdb_NodeRef_t nodeRef,  ///< The node to read.
    char* stringPtr,        ///< Destination buffer to hold the name.
    size_t maxSize          ///< Size of this buffer.
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
 *  Set the name of a given node.  But also validate the name as there are certain names that nodes
 *  shouldn't have.
 *
 *  @return LE_OK if the set is successful.  LE_FORMAT_ERROR if the name contains illegial
 *          characters, or otherwise would not work as a node name.  LE_DUPLICATE, if there is
 *          another node with the new name in the same collection.
 */
// -------------------------------------------------------------------------------------------------
le_result_t tdb_SetNodeName
(
    tdb_NodeRef_t nodeRef,  ///< The node to read.
    const char* stringPtr   ///< New name for the node.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    if (   (stringPtr == NULL)
        || (strcmp(stringPtr, "") == 0)
        || (strcmp(stringPtr, ".") == 0)
        || (strcmp(stringPtr, "..") == 0)
        || (strchr(stringPtr, '/') != NULL))
    {
        return LE_FORMAT_ERROR;
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

    // Make sure that we know to merge this node later.
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
    tdb_NodeRef_t nodeRef  ///< The node to read.
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
        if (   (IsShadow(nodeRef))
            && (IsModifed(nodeRef) == false))
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
    tdb_NodeRef_t nodeRef  ///< The node to read.
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
    tdb_NodeRef_t nodeRef  ///< The node to clear.
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
    tdb_NodeRef_t nodeRef  ///< The node to delete.
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
    tdb_NodeRef_t nodeRef  ///< The node object to iterate from.
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
    tdb_NodeRef_t nodeRef  ///< The node object to iterate from.
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
        if (IsModifed(nodeRef) == false)
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
    tdb_NodeRef_t nodeRef  ///< The node object to iterate from.
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
    tdb_NodeRef_t nodeRef  ///< The node object to iterate from.
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
    tdb_NodeRef_t nodeRef  ///< The node object to iterate from.
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
    tdb_NodeRef_t nodeRef,  ///< The node object to read.
    char* stringPtr,        ///< Target buffer for the value string.
    size_t maxSize,         ///< Maximum size the buffer can hold.
    const char* defaultPtr  ///< Default value to use in the event that the requested value doesn't
                            ///<   exist.
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
    tdb_NodeRef_t nodeRef,  ///< The node to set.
    const char* stringPtr   ///< The value to write to the node.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    // Make sure the node is cleared out and value is set to it's default state.
    if (   (nodeRef->type == LE_CFG_TYPE_STEM)
        || (nodeRef->type == LE_CFG_TYPE_EMPTY))
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
    EnsureExists(nodeRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read the given node and interpret it as a boolean value.
 */
// -------------------------------------------------------------------------------------------------
bool tdb_GetValueAsBool
(
    tdb_NodeRef_t nodeRef,  ///< The node to read.
    bool defaultValue       ///< Default value to use in the event that the requested value doesn't
                            ///<   exist.
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
    tdb_NodeRef_t nodeRef,  ///< The node to write to.
    bool value              ///< The new value to write to that node.
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
    tdb_NodeRef_t nodeRef,  ///< The node to read from.
    int32_t defaultValue    ///< Default value to use in the event that the requested value doesn't
                            ///<   exist.
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
    tdb_NodeRef_t nodeRef,  ///< The node to write to.
    int value               ///< The value to write.
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
 */
// -------------------------------------------------------------------------------------------------
double tdb_GetValueAsFloat
(
    tdb_NodeRef_t nodeRef,  ///< The node to read.
    double defaultValue     ///< Default value to use in the event that the requested value doesn't
                            ///<   exist.
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
                char buffer[SMALL_STR] = { 0 };

                tdb_GetValueAsString(nodeRef, buffer, SMALL_STR, "");
                result = (float)atof(buffer);
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
    tdb_NodeRef_t nodeRef,  ///< The node to write
    double value            ///< The value to write to that node.
)
// -------------------------------------------------------------------------------------------------
{
    LE_ASSERT(nodeRef != NULL);

    char buffer[SMALL_STR] = { 0 };

    snprintf(buffer, SMALL_STR, "%f", value);
    tdb_SetValueAsString(nodeRef, buffer);
    nodeRef->type = LE_CFG_TYPE_FLOAT;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Registers a handler function to be called when a node at or below a given path changes.
 */
//--------------------------------------------------------------------------------------------------
le_cfg_ChangeHandlerRef_t tdb_AddChangeHandler
(
    tdb_TreeRef_t tree,                     ///< The tree to register the handler on.
    const char* pathPtr,                    ///< Path of the node to watch.
    le_cfg_ChangeHandlerFunc_t handlerPtr,  ///< Function to call back.
    void* contextPtr                        ///< Opaque value to pass to the function when called.
)
//--------------------------------------------------------------------------------------------------
{
    return NULL;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Deregisters a handler function that was registered using tdb_AddChangeHandler().
 */
//--------------------------------------------------------------------------------------------------
void tdb_RemoveChangeHandler
(
    le_cfg_ChangeHandlerRef_t   handlerRef  ///< Reference returned by tdb_AddChangeHandler().
)
//--------------------------------------------------------------------------------------------------
{
}
