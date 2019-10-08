/**
 * @page c_redBlackTree Red/Black Tree API
 *
 * @ref le_redBlackTree.h "API Reference"
 *
 * <HR>
 *
 * A Red-Black Tree is a data structure representing a self-balancing binary search tree.
 * Tree consists of nodes maintaining links to the parent, left and right nodes. Advantage over a
 * linked-list is faster search based on the key comparison. Advantage over a hashtable is
 * simplified memory management (no additional allocation within the library), better
 * scalability up and down, and possibility to easily iterate the set in ascending/descending order.
 *
 * @section rbtree_createTree Creating and Initializing Red-Black Trees
 *
 * To create and initialize an RB Tree the user must create an le_rbtree_Tree_t typed object and
 * initialize it using le_rbtree_InitTree() routine. At this time the user has to provide a pointer
 * to the comparator function, that provides a way to perform a comparison between objects.
 * The tree <b>must</b> be initialized before it can be used.
 *
 * @code
 * // provide the comparator function
 * static int compare(const void* a, const void* b)
 * {
 *    // return negative, 0, or positive value
 * }
 *
 * // Create the tree.
 * le_rbtree_Tree_t MyTree;
 *
 * // Initialize the tree.
 * le_rbtree_InitTree(&MyTree, compare);
 * @endcode
 *
 * <b> Elements of @c le_rbtree_Tree_t MUST NOT be accessed directly by the user. </b>
 *
 *
 * @section rbtree_createNode Creating and Accessing Nodes
 *
 * Nodes can contain any data in any format and is defined and created by the user. The only
 * requirement for nodes is that it must contain an @c le_rbtree_Node_t link member. The link member
 * must be initialized by calling @c le_rbtree_InitNode() before it is added to the tree; at this
 * time, pointer to the key of this object must be provided.
 * The node can be added to the tree using the function @c le_rbtree_Insert().
 *
 * @code
 * // The node may be defined like this.
 * typedef struct
 * {
 *      MyKeyType key;
 *      dataType someUserData;
 *      ...
 *      le_rbtree_Node_t myLink;
 * }
 * MyNodeClass_t;
 *
 * void foo (void)
 * {
 *     // Create the node.  Get the memory from a memory pool previously created.
 *     MyNodeClass_t* myNodePtr = le_mem_ForceAlloc(MyNodePool);
 *
 *     // Initialize the node's link.
 *     le_rbtree_InitNode(&MyTree, &myNodePtr->key);
 *
 *     // Add the node to the tree by passing in the node's link.
 *     le_rbtree_Insert(&MyTree, &myNodePtr->myLink);
 * }
 * @endcode
 *
 * @section rbtree_find Finding node in a Tree
 *
 * To find the node in the tree by the given key, use @c le_rbtree_Find() function.
 * To obtain the object itself, use the @c CONTAINER_OF macro defined in
 * le_basics.h. Here's a code sample using @c CONTAINER_OF to obtain the node:
 *
 * @code
 * // declare and initialize the key
 * MyKeyType key = <...>;
 * // Assuming MyTree has been created and initialized and is not empty.
 * le_rbtree_Node_t* linkPtr = le_rbtree_Find(&MyTree, &key);
 *
 * // Now we have the link but still need the node to access user data.
 * // We use CONTAINER_OF to get a pointer to the node given the node's link.
 * if (linkPtr != NULL)
 * {
 *     MyNodeClass_t* myNodePtr = CONTAINER_OF(linkPtr, MyNodeClass_t, myLink);
 * }
 * @endcode
 *
 * The user is responsible for creating and freeing memory for all nodes; the RB Tree module
 * only manages the links in the nodes. The node must be removed from all trees before its
 * memory can be freed.
 *
 * <b>The elements of @c le_rbtree_Node_t MUST NOT be accessed directly by the user.</b>
 *
 * @section rbtree_traverse Traversing a Tree
 *
 * Tree can be traversed in an ascending or descending order (in the sense of greater/lesser
 * provided by the comparator function):
 *
 * @code
 * // Ascending order
 * le_rbtree_Node_t* linkPtr;
 * for (linkPtr = le_rbtree_GetFirst(&MyTree);
 *      linkPtr != NULL;
 *      linkPtr = le_rbtree_GetNext(&MyTree, linkPtr))
 * {
 *     MyNodeClass_t* myNodePtr = CONTAINER_OF(linkPtr, MyNodeClass_t, myLink);
 * }
 *
 * // Descending order
 * le_rbtree_Node_t* linkPtr;
 * for (linkPtr = le_rbtree_GetLast(&MyTree);
 *      linkPtr != NULL;
 *      linkPtr = le_rbtree_GetPrev(&MyTree, linkPtr))
 * {
 *     MyNodeClass_t* myNodePtr = CONTAINER_OF(linkPtr, MyNodeClass_t, myLink);
 * }
 * @endcode
 *
 * @section rbtree_remove Removing node from a Tree
 *
 * To remove a node from a Tree, use le_rbtree_RemoveByKey() function:
 * @code
 *     // Remove the node
 *     le_rbtree_RemoveByKey(&MyTree, &(myNodePtr->key));
 *     // Free the object
 *     le_mem_Release(myNodePtr);
 * @endcode
 *
 * or le_rbtree_Remove() function:
 * @code
 *     // Remove the node
 *     le_rbtree_Remove(&MyTree, &(myNodePtr->myLink));
 *     // Free the object
 *     le_mem_Release(myNodePtr);
 * @endcode
 *
 * @section rbtree_synch Thread Safety and Re-Entrancy
 *
 * All Red-Black Tree function calls are re-entrant and thread safe themselves, but if the nodes
 * and/or Tree object are shared by multiple threads, explicit steps must be taken to maintain
 * mutual exclusion of access. If you're accessing the same tree from multiple threads, you @e must
 * use a @ref c_mutex "mutex" or some other form of thread synchronization to ensure only one thread
 * accesses the tree at a time.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

/** @file le_redBlackTree.h
 *
 * Legato @ref c_redBlackTree include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_RED_BLACK_TREE_INCLUDE_GUARD
#define LEGATO_RED_BLACK_TREE_INCLUDE_GUARD

/**
 * Color type for Red-Black tree.
 */
typedef enum
{
    LE_RBTREE_NO_COLOR,
    LE_RBTREE_BLACK,
    LE_RBTREE_RED
}
le_rbtree_Color_t;

/**
 * The type of the node of the Red-Black Tree.
 */
typedef struct le_rbtree_Node
{
    void *key;                      ///< Key pointer.
    struct le_rbtree_Node *parent;  ///< Parent link pointer.
    struct le_rbtree_Node *left;    ///< Left node link pointer.
    struct le_rbtree_Node *right;   ///< Right node link pointer.
    le_rbtree_Color_t color;        ///< Color
}
le_rbtree_Node_t;

//--------------------------------------------------------------------------------------------------
/**
 * This is a comparator function to compare objects in a tree.
 *
 *  @return negative, 0, positive number if the first key is less, equal, or greater than
 *          the second one.
 */
//--------------------------------------------------------------------------------------------------
typedef int (*le_rbtree_CompareFunc_t)
(
    const void *key1Ptr,   ///< [IN] Pointer to the key of the first object
    const void *key2Ptr    ///< [IN] Pointer to the key of the second object
);


//--------------------------------------------------------------------------------------------------
/**
 * This is the RBTree object. User must initialize it by calling le_rbtree_InitTree.
 *
 * @warning User MUST NOT access the contents of this structure directly.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_rbtree_Tree
{
    le_rbtree_Node_t *root;         ///< Root tree node.
    size_t size;                    ///< Number of elements in the tree.
    le_rbtree_CompareFunc_t compFn; ///< Key comparison function.
}
le_rbtree_Tree_t;


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Red-Black Tree.
 */
//--------------------------------------------------------------------------------------------------
void le_rbtree_InitTree
(
    le_rbtree_Tree_t *treePtr,      ///< [IN] Pointer to the tree object.
    le_rbtree_CompareFunc_t compFn  ///< [IN] Pointer to the comparator function.
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Node Link.
 */
//--------------------------------------------------------------------------------------------------
void le_rbtree_InitNode
(
    le_rbtree_Node_t *linkPtr,      ///< [INOUT] Node link pointer.
    void *keyPtr                    ///< [IN] Key pointer.
);

//--------------------------------------------------------------------------------------------------
/**
 * Insert a new node in the tree. If Node with matching key is already in the tree, does nothing
 * (no update).
 *
 * @return Pointer to the Node inserted in the tree.
 *         NULL if the node already exists in the tree (duplicate).
 */
//--------------------------------------------------------------------------------------------------
le_rbtree_Node_t* le_rbtree_Insert
(
    le_rbtree_Tree_t *treePtr,            ///< [IN] Tree to insert into.
    le_rbtree_Node_t *newLinkPtr          ///< [IN] New link to add.
);

//--------------------------------------------------------------------------------------------------
/**
 * Find object in the tree for the given key.
 *
 * @return Pointer to the Node found in the tree.
 *         NULL if not found.
 */
//--------------------------------------------------------------------------------------------------
le_rbtree_Node_t* le_rbtree_Find
(
    le_rbtree_Tree_t *treePtr,    ///< [IN] Tree to get the object from.
    void *keyPtr                  ///< [IN] Pointer to the key to be retrieved
);

//--------------------------------------------------------------------------------------------------
/**
 * Removes the specified node from the tree.
 *
 * @return Pointer to the node removed from the tree.
 *         NULL if not found.
 */
//--------------------------------------------------------------------------------------------------
le_rbtree_Node_t* le_rbtree_Remove
(
    le_rbtree_Tree_t *treePtr,    ///< [IN] Tree to remove from.
    le_rbtree_Node_t *linkPtr     ///< [IN] Link to remove.
);

//--------------------------------------------------------------------------------------------------
/**
 * Removes the specified node from the tree by the specified key.
 *
 * @return Pointer to the node removed from the tree.
 *         NULL if not found.
 */
//--------------------------------------------------------------------------------------------------
le_rbtree_Node_t* le_rbtree_RemoveByKey
(
    le_rbtree_Tree_t *treePtr,    ///< [IN] Tree to remove from.
    void *keyPtr                  ///< [IN] Pointer to the key to be removed
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the first (smallest) node in the tree.
 *
 * @return Pointer to the node if successful.
 *         NULL if the tree is empty.
 */
//--------------------------------------------------------------------------------------------------
le_rbtree_Node_t* le_rbtree_GetFirst
(
    const le_rbtree_Tree_t *treePtr    ///< [IN] Tree to get the object from.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the last (greatest) node in the tree.
 *
 * @return Pointer to the Node if successful.
 *         NULL if the tree is empty.
 */
//--------------------------------------------------------------------------------------------------
le_rbtree_Node_t* le_rbtree_GetLast
(
    const le_rbtree_Tree_t *treePtr    ///< [IN] Tree to get the object from.
);

//--------------------------------------------------------------------------------------------------
/**
 * Returns the node next to currentLinkPtr without removing it from the tree.
 * User must ensure that currentLinkPtr is in the tree.
 *
 * @return
 *      Pointer to the next link if successful.
 *      NULL if there is no node greater than the currentLinkPtr.
 */
//--------------------------------------------------------------------------------------------------
le_rbtree_Node_t* le_rbtree_GetNext
(
    const le_rbtree_Tree_t *treePtr,   ///< [IN] Tree to get the object from.
    le_rbtree_Node_t *currentLinkPtr   ///< [IN] Current node pointer.
);

//--------------------------------------------------------------------------------------------------
/**
 * Returns the node previous to currentLinkPtr without removing it from the tree.
 * User must ensure that currentLinkPtr is in the tree.
 *
 * @return
 *      Pointer to the previous link if successful.
 *      NULL if there is no node smaller than the currentLinkPtr.
 */
//--------------------------------------------------------------------------------------------------
le_rbtree_Node_t* le_rbtree_GetPrev
(
    const le_rbtree_Tree_t *treePtr,        ///< [IN] Tree containing currentLinkPtr.
    le_rbtree_Node_t *currentLinkPtr        ///< [IN] Get the link that is relative to this link.
);

//--------------------------------------------------------------------------------------------------
/**
 * Tests if the Tree is empty.
 *
 * @return  Returns true if empty, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool le_rbtree_IsEmpty
(
    const le_rbtree_Tree_t *treePtr         ///< [IN] Tree containing currentLinkPtr.
);

//--------------------------------------------------------------------------------------------------
/**
 * Calculates size of the Tree.
 *
 * @return  The number of elementskeys in the Tree.
 */
//--------------------------------------------------------------------------------------------------
size_t le_rbtree_Size
(
    const le_rbtree_Tree_t *treePtr         ///< [IN] Tree containing currentLinkPtr.
);

#endif  // LEGATO_RED_BLACK_TREE_INCLUDE_GUARD
