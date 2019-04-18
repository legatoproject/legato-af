/**
 * @page c_doublyLinkedList Doubly Linked List API
 *
 * @subpage le_doublyLinkedList.h "API Reference"
 *
 * <HR>
 *
 * A doubly linked list is a data structure consisting of a group of nodes linked together linearly.
 * Each node consists of data elements with links to the next node and previous nodes.  The main
 * advantage of linked lists (over simple arrays) is the nodes can be inserted and removed
 * anywhere in the list without reallocating the entire array. Linked list nodes don't
 * need to be stored contiguously in memory, but nodes then you can't access by
 * index, you have to be access by traversing the list.
 *
 * @section dls_createList Creating and Initializing Lists
 *
 * To create and initialize a linked list the user must create a le_dls_List_t typed list and assign
 * LE_DLS_LIST_INIT to it.  The assignment of LE_DLS_LIST_INIT can be done either when the list is
 * declared or after its declared.  The list <b>must</b> be initialized before it can be
 * used.
 *
 * @code
 * // Create and initialized the list in the declaration.
 * le_dls_List_t MyList = LE_DLS_LIST_INIT;
 * @endcode
 *
 * Or
 *
 * @code
 * // Create list.
 * le_dls_List_t MyList;
 *
 * // Initialize the list.
 * MyList = LE_DLS_LIST_INIT;
 * @endcode
 *
 * <b> Elements of le_dls_List_t MUST NOT be accessed directly by the user. </b>
 *
 *
 * @section dls_createNode Creating and Accessing Nodes
 *
 * Nodes can contain any data in any format and is defined and created by the user.  The only
 * requirement for nodes is that it must contain a @c le_dls_Link_t link member. The link member must
 * be initialized by assigning LE_DLS_LINK_INIT to it before it can be used.  Nodes can then be
 * added to the list by passing their links to the add functions (le_dls_Stack(), le_dls_Queue(),
 * etc.).  For example:
 *
 * @code
 * // The node may be defined like this.
 * typedef struct
 * {
 *      dataType someUserData;
 *      ...
 *      le_dls_Link_t myLink;
 *
 * }
 * MyNodeClass_t;
 *
 * // Create and initialize the list.
 * le_dls_List_t MyList = LE_DLS_LIST_INIT;
 *
 * void foo (void)
 * {
 *     // Create the node.  Get the memory from a memory pool previously created.
 *     MyNodeClass_t* myNodePtr = le_mem_ForceAlloc(MyNodePool);
 *
 *     // Initialize the node's link.
 *     myNodePtr->myLink = LE_DLS_LINK_INIT;
 *
 *     // Add the node to the head of the list by passing in the node's link.
 *     le_dls_Stack(&MyList, &(myNodePtr->myLink));
 * }
 * @endcode
 *
 * The links in the nodes are actually added to the list and not the nodes themselves. This
 * allows a node to be included on multiple lists through links
 * added to different lists. It also allows linking different type nodes in a list.
 *
 * To obtain the node itself, use the @c CONTAINER_OF macro defined in
 * le_basics.h.  Here's a code sample using CONTAINER_OF to obtain the node:
 *
 * @code
 * // Assuming mylist has been created and initialized and is not empty.
 * le_dls_Link_t* linkPtr = le_dls_Peek(&MyList);
 *
 * // Now we have the link but still need the node to access user data.
 * // We use CONTAINER_OF to get a pointer to the node given the node's link.
 * if (linkPtr != NULL)
 * {
 *     MyNodeClass_t* myNodePtr = CONTAINER_OF(linkPtr, MyNodeClass_t, myLink);
 * }
 * @endcode
 *
 * The user is responsible for creating and freeing memory for all nodes; the linked list module
 * only manages the links in the nodes.  The node must be removed from all lists before its
 * memory can be freed.
 *
 * <b>The elements of @c le_dls_Link_t MUST NOT be accessed directly by the user.</b>
 *
 *
 * @section dls_add Adding Links to a List
 *
 * To add nodes to a list, pass the node's link to one of these functions:
 *
 * - @c le_dls_Stack() - Adds the link to the head of the list.
 * - @c le_dls_Queue() - Adds the link to the tail of the list.
 * - @c le_dls_AddAfter() - Adds the link to a list after another specified link.
 * - @c le_dls_AddBefore() - Adds the link to a list before another specified link.
 *
 *
 * @section dls_remove Removing Links from a List
 *
 * To remove nodes from a list, use one of these functions:
 *
 * - @c le_dls_Pop() - Removes and returns the link at the head of the list.
 * - @c le_dls_PopTail() - Removes and returns the link at the tail of the list.
 * - @c le_dls_Remove() - Remove a specified link from the list.
 *
 *
 * @section dls_peek Accessing Links in a List
 *
 * To access a link in a list without removing the link, use one of these functions:
 *
 * - @c le_dls_Peek() - Returns the link at the head of the list without removing it.
 * - @c le_dls_PeekTail() - Returns the link at the tail of the list without removing it.
 * - @c le_dls_PeekNext() - Returns the link next to a specified link without removing it.
 * - @c le_dls_PeekPrev() - Returns the link previous to a specified link without removing it.
 *
 *
 * @section dls_swap Swapping Links
 *
 * To swap two links, use:
 *
 * - @c le_dls_Swap() - Swaps the position of two links in a list.
 *
 * @section dls_sort Sorting Lists
 *
 * To sort a list use:
 *
 * @c le_dls_Sort() - Sorts a list
 *
 * @section dls_query Querying List Status
 *
 * These functions can be used to query a list's current status:
 *
 * - @c le_dls_IsEmpty() - Checks if a given list is empty.
 * - @c le_dls_IsInList() - Checks if a specified link is in the list.
 * - @c le_dls_IsHead() - Checks if a specified link is at the head of the list.
 * - @c le_dls_IsTail() - Checks if a specified link is at the tail of the list.
 * - @c le_dls_NumLinks() - Checks the number of links currently in the list.
 * - @c le_dls_IsListCorrupted() - Checks if the list is corrupted.
 *
 *
 * @section dls_fifo Queues and Stacks
 *
 * This implementation of linked lists can be used for either queues or stacks.
 *
 * To use the list as a queue, restrict additions to the list to @c le_dls_Queue() and removals
 * from the list to @c le_dls_Pop().
 *
 * To use the list as a stack,  restrict additions to the list to @c le_dls_Stack() and removals
 * from the list to @c le_dls_Pop().
 *
 *
 * @section dls_synch Thread Safety and Re-Entrancy
 *
 * All linked list function calls are re-entrant and thread safe themselves, but if the nodes and/or
 * list object are shared by multiple threads, explicit steps must be taken to maintain mutual
 * exclusion of access. If you're accessing the same list from multiple threads, you @e must use a
 * @ref c_mutex "mutex" or some other form of thread synchronization to ensure only one thread
 * accesses the list at a time.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */


/** @file le_doublyLinkedList.h
 *
 * Legato @ref c_doublyLinkedList include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_DOUBLY_LINKED_LIST_INCLUDE_GUARD
#define LEGATO_DOUBLY_LINKED_LIST_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * This link object must be included in each user node.  The node's link object is used to add the
 * node to a list.  A node may have multiple link objects which would allow the node to be part of
 * multiple lists simultaneously.  This link object must be initialized by assigning
 * LE_DLS_LINK_INIT to it.
 *
 * @warning The structure's content MUST NOT be accessed directly.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_dls_Link
{
    struct le_dls_Link* nextPtr;       ///< Next link pointer.
    struct le_dls_Link* prevPtr;       ///< Previous link pointer.
}
le_dls_Link_t;


//--------------------------------------------------------------------------------------------------
/**
 * This is the list object.  User must create this list object and initialize it by assigning
 * LE_DLS_LIST_INIT to it.
 *
 * @warning User MUST NOT access the contents of this structure directly.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t* headLinkPtr;        ///< Link to list head.
}
le_dls_List_t;


//--------------------------------------------------------------------------------------------------
/**
 * This is a comparator function for sorting a list.
 *
 * This must return true if @c a goes before @c b in the list.
 */
//--------------------------------------------------------------------------------------------------
typedef bool (*le_dls_LessThanFunc_t)(le_dls_Link_t* a, le_dls_Link_t* b);


//--------------------------------------------------------------------------------------------------
/**
 * When a list is created it must be initialized by assigning this macro to the list before the list
 * can be used.
 */
//--------------------------------------------------------------------------------------------------
#define LE_DLS_LIST_INIT (le_dls_List_t){NULL}


//--------------------------------------------------------------------------------------------------
/**
 * When a link is created it must be initialized by assigning this macro to the link before it can
 * be used.
 */
//--------------------------------------------------------------------------------------------------
#define LE_DLS_LINK_INIT (le_dls_Link_t){NULL, NULL}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a link at the head of the list.
 */
//--------------------------------------------------------------------------------------------------
void le_dls_Stack
(
    le_dls_List_t* listPtr,            ///< [IN] List to add to.
    le_dls_Link_t* newLinkPtr          ///< [IN] New link to add.
);


//--------------------------------------------------------------------------------------------------
/**
 * Adds a link to the tail of the list.
 */
//--------------------------------------------------------------------------------------------------
void le_dls_Queue
(
    le_dls_List_t* listPtr,            ///< [IN] List to add to.
    le_dls_Link_t* newLinkPtr          ///< [IN] New link to add.
);


//--------------------------------------------------------------------------------------------------
/**
 * Adds a link after currentLinkPtr.  User must ensure that currentLinkPtr is in the list
 * otherwise the behaviour of this function is undefined.
 */
//--------------------------------------------------------------------------------------------------
void le_dls_AddAfter
(
    le_dls_List_t* listPtr,            ///< [IN] List to add to.
    le_dls_Link_t* currentLinkPtr,     ///< [IN] New link will be inserted after this link.
    le_dls_Link_t* newLinkPtr          ///< [IN] New link to add.
);


//--------------------------------------------------------------------------------------------------
/**
 * Adds a link after currentLinkPtr.  User must ensure that currentLinkPtr is in the list
 * otherwise the behaviour of this function is undefined.
 */
//--------------------------------------------------------------------------------------------------
void le_dls_AddBefore
(
    le_dls_List_t* listPtr,            ///< [IN] List to add to.
    le_dls_Link_t* currentLinkPtr,     ///< [IN] New link will be inserted before this link.
    le_dls_Link_t* newLinkPtr          ///< [IN] New link to add.
);


//--------------------------------------------------------------------------------------------------
/**
 * Removes and returns the link at the head of the list.
 *
 * @return
 *      Removed link.
 *      NULL if the link is not available because the list is empty.
 */
//--------------------------------------------------------------------------------------------------
le_dls_Link_t* le_dls_Pop
(
    le_dls_List_t* listPtr             ///< [IN] List to remove from.
);


//--------------------------------------------------------------------------------------------------
/**
 * Removes and returns the link at the tail of the list.
 *
 * @return
 *      The removed link.
 *      NULL if the link is not available because the list is empty.
 */
//--------------------------------------------------------------------------------------------------
le_dls_Link_t* le_dls_PopTail
(
    le_dls_List_t* listPtr             ///< [IN] List to remove from.
);


//--------------------------------------------------------------------------------------------------
/**
 * Removes the specified link from the list.  Ensure the link is in the list
 * otherwise the behaviour of this function is undefined.
 */
//--------------------------------------------------------------------------------------------------
void le_dls_Remove
(
    le_dls_List_t* listPtr,             ///< [IN] List to remove from.
    le_dls_Link_t* linkToRemovePtr      ///< [IN] Link to remove.
);


//--------------------------------------------------------------------------------------------------
/**
 * Returns the link at the head of the list without removing it from the list.
 *
 * @return
 *      Pointer to the head link if successful.
 *      NULL if the list is empty.
 */
//--------------------------------------------------------------------------------------------------
le_dls_Link_t* le_dls_Peek
(
    const le_dls_List_t* listPtr            ///< [IN] The list.
);


//--------------------------------------------------------------------------------------------------
/**
 * Returns the link at the tail of the list without removing it from the list.
 *
 * @return
 *      A pointer to the tail link if successful.
 *      NULL if the list is empty.
 */
//--------------------------------------------------------------------------------------------------
le_dls_Link_t* le_dls_PeekTail
(
    const le_dls_List_t* listPtr            ///< [IN] The list.
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks if a list is empty.
 *
 * @return
 *      true if empty, false if not empty.
 */
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE bool le_dls_IsEmpty
(
    const le_dls_List_t* listPtr            ///< [IN] The list.
)
//--------------------------------------------------------------------------------------------------
{
    return (le_dls_Peek(listPtr) == NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Returns the link next to currentLinkPtr (i.e., the link beside currentLinkPtr that is closer to the
 * tail) without removing it from the list.  User must ensure that currentLinkPtr is in the list
 * otherwise the behaviour of this function is undefined.
 *
 * @return
 *      Pointer to the next link if successful.
 *      NULL if there is no link next to the currentLinkPtr (currentLinkPtr is at the tail of the
 *      list).
 */
//--------------------------------------------------------------------------------------------------
le_dls_Link_t* le_dls_PeekNext
(
    const le_dls_List_t* listPtr,           ///< [IN] List containing currentLinkPtr.
    const le_dls_Link_t* currentLinkPtr     ///< [IN] Get the link that is relative to this link.
);


//--------------------------------------------------------------------------------------------------
/**
 * Returns the link previous to currentLinkPtr without removing it from the list.  User must
 * ensure that currentLinkPtr is in the list otherwise the behaviour of this function is undefined.
 *
 * @return
 *      Pointer to the previous link if successful.
 *      NULL if there is no link previous to the currentLinkPtr (currentLinkPtr is at the head of
 *      the list).
 */
//--------------------------------------------------------------------------------------------------
le_dls_Link_t* le_dls_PeekPrev
(
    const le_dls_List_t* listPtr,          ///< [IN] List containing currentLinkPtr.
    const le_dls_Link_t* currentLinkPtr    ///< [IN] Get the link that is relative to this link.
);


//--------------------------------------------------------------------------------------------------
/**
 * Swaps the position of two links in the list. User must ensure that both links are in the
 * list otherwise the behaviour of this function is undefined.
 */
//--------------------------------------------------------------------------------------------------
void le_dls_Swap
(
    le_dls_List_t* listPtr,         ///< [IN] List containing the links to swap.
    le_dls_Link_t* linkPtr,         ///< [IN] One of the two link pointers to swap.
    le_dls_Link_t* otherLinkPtr     ///< [IN] Other link pointer to swap.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sort a list in ascending order.
 */
//--------------------------------------------------------------------------------------------------
void le_dls_Sort
(
    le_dls_List_t* listPtr,                 ///< [IN] List to sort
    le_dls_LessThanFunc_t comparatorPtr     ///< [IN] Comparator function for sorting
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks if a link is in the list.
 *
 * @return
 *      true if the link is in the list.
 *      false if the link is not in the list.
 */
//--------------------------------------------------------------------------------------------------
bool le_dls_IsInList
(
    const le_dls_List_t* listPtr,    ///< [IN] List to check.
    const le_dls_Link_t* linkPtr     ///< [IN] Check if this link is in the list.
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks if a link is at the head of the list (next to be popped).
 *
 * @return
 *    - true if the link is at the head of the list.
 *    - false if not.
 */
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE bool le_dls_IsHead
(
    const le_dls_List_t* listPtr,    ///< [IN] List to check.
    const le_dls_Link_t* linkPtr     ///< [IN] Check if this link is at the head of the list.
)
{
    return (le_dls_Peek(listPtr) == linkPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if a link is at the tail of the list (last to be popped).
 *
 * @return
 *    - true if the link is at the tail of the list.
 *    - false if not.
 */
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE bool le_dls_IsTail
(
    const le_dls_List_t* listPtr,    ///< [IN] List to check.
    const le_dls_Link_t* linkPtr     ///< [IN] Check if this link is at the tail of the list.
)
{
    return (le_dls_PeekTail(listPtr) == linkPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Returns the number of links in a list.
 *
 * @return
 *      Number of links.
 */
//--------------------------------------------------------------------------------------------------
size_t le_dls_NumLinks
(
    const le_dls_List_t* listPtr       ///< [IN] List to count.
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the list is corrupted.
 *
 * @return
 *      true if the list is corrupted.
 *      false if the list is not corrupted.
 */
//--------------------------------------------------------------------------------------------------
bool le_dls_IsListCorrupted
(
    const le_dls_List_t* listPtr    ///< [IN] List to check.
);

//--------------------------------------------------------------------------------------------------
/**
 * Simple iteration through a doubly linked list
 */
//--------------------------------------------------------------------------------------------------
#define LE_DLS_FOREACH(listPtr, iteratorPtr, type, member)              \
    for ((iteratorPtr) = CONTAINER_OF(le_dls_Peek(listPtr), type, member); \
         &((iteratorPtr)->member);                                      \
         (iteratorPtr) = CONTAINER_OF(le_dls_PeekNext((listPtr),&((iteratorPtr)->member)), \
                                      type, member))


#endif  // LEGATO_DOUBLY_LINKED_LIST_INCLUDE_GUARD

