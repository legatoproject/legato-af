/**
 * @page c_singlyLinkedList Singly Linked List API
 *
 * @subpage le_singlyLinkedList.h "API Reference"
 *
 * <HR>
 *
 * A singly linked list is a data structure consisting of a group of nodes linked together linearly.
 * Each node consists of data elements and a link to the next node.  The main advantage of linked
 * lists over simple arrays is that the nodes can be inserted anywhere in the list without
 * reallocating the entire array because the nodes in a linked list do not need to be stored
 * contiguously in memory.  However, nodes in the list cannot be accessed by index but must be
 * accessed by traversing the list.
 *
 *
 * @section sls_createList Creating and Initializing Lists
 *
 * To create and initialize a linked list, @c create a le_sls_List_t typed list and assign
 * LE_SLS_LIST_INIT to it.  The assignment of LE_SLS_LIST_INIT can be done either when the list is
 * declared or after it's declared.  The list <b>must</b> be initialized before it can be
 * used.
 *
 * @code
 * // Create and initialized the list in the declaration.
 * le_sls_List_t MyList = LE_SLS_LIST_INIT;
 * @endcode
 *
 * Or
 *
 * @code
 * // Create list.
 * le_sls_List_t MyList;
 *
 * // Initialize the list.
 * MyList = LE_SLS_LIST_INIT;
 * @endcode
 *
 * <b> The elements of le_sls_List_t MUST NOT be accessed directly by the user. </b>
 *
 *
 * @section sls_createNode Creating and Accessing Nodes
 *
 * Nodes can contain any data in any format and is defined and created by the user.  The only
 * requirement for nodes is that it must contain a le_sls_Link_t link member.  The link member must
 * be initialized by assigning LE_SLS_LINK_INIT to it before it can be used.  Nodes can then be
 * added to the list by passing their links to the add functions (le_sls_Stack(), le_sls_Queue(),
 * etc.).  For example:
 *
 * @code
 * // The node may be defined like this.
 * typedef struct
 * {
 *      dataType someUserData;
 *      ...
 *      le_sls_Link_t myLink;
 *
 * }
 * MyNodeClass_t;
 *
 * // Create and initialize the list.
 * le_sls_List_t MyList = LE_SLS_LIST_INIT;
 *
 * void foo (void)
 * {
 *     // Create the node.  Get the memory from a memory pool previously created.
 *     MyNodeClass_t* myNodePtr = le_mem_ForceAlloc(MyNodePool);
 *
 *     // Initialize the node's link.
 *     myNodePtr->myLink = LE_SLS_LINK_INIT;
 *
 *     // Add the node to the head of the list by passing in the node's link.
 *     le_sls_Stack(&MyList, &(myNodePtr->myLink));
 * }
 * @endcode
 *
 * The links in the nodes are added to the list and not the nodes themselves.  This
 * allows a node to be simultaneously part of multiple lists simply by having multiple links and
 * adding the links into differently lists.  This also means that nodes in a list can be of
 * different types.
 *
 * Because the links and not the nodes are in the list, the user must have a way to obtain
 * the node itself from the link.  This is achieved using the @c CONTAINER_OF macro defined in
 * le_basics.h.  This code sample shows using CONTAINER_OF to obtain the node:
 *
 * @code
 * // Assuming mylist has been created and initialized and is not empty.
 * le_sls_Link_t* linkPtr = le_sls_Peek(&MyList);
 *
 * // Now we have the link but we want the node so we can access the user data.
 * // We use CONTAINER_OF to get a pointer to the node given the node's link.
 * if (linkPtr != NULL)
 * {
 *     MyNodeClass_t* myNodePtr = CONTAINER_OF(linkPtr, MyNodeClass_t, myLink);
 * }
 * @endcode
 *
 * The user is responsible for creating and freeing memory for all nodes, the linked list module
 * simply manages the links in the nodes.  The node must first be removed from all lists before its
 * memory is freed.
 *
 * <b>The elements of le_sls_Link_t MUST NOT be accessed directly by the user.</b>
 *
 *
 * @section sls_add Adding Links to a List
 *
 * To add nodes to a list pass the node's link to one of the following functions:
 *
 * - le_sls_Stack() - Adds the link to the head of the list.
 * - le_sls_Queue() - Adds the link to the tail of the list.
 * - le_sls_AddAfter() - Adds the link to a list after another specified link.
 *
 *
 * @section sls_remove Removing Links from a List
 *
 * To remove nodes from a list, use @c le_sls_Pop() to remove and return the link at the head of
 * the list.
 *
 *
 * @section sls_peek Accessing Links in a List
 *
 * To access a link in a list without removing the link, use one of the following functions:
 *
 * - @c le_sls_Peek() - Returns the link at the head of the list without removing it.
 * - @c le_sls_PeekNext() - Returns the link next to a specified link without removing it.
 * - @c le_sls_PeekTail() - Returns the link at the tail of the list without removing it.
 *
 *
 * @section sls_query Querying List Status
 *
 * The following functions can be used to query a list's current status:
 *
 * - @c le_sls_IsEmpty() - Checks if a given list is empty or not.
 * - @c le_sls_IsInList() - Checks if a specified link is in the list.
 * - @c le_sls_IsHead() - Checks if a specified link is at the head of the list.
 * - @c le_sls_IsTail() - Checks if a specified link is at the tail of the list.
 * - @c le_sls_NumLinks() - Checks the number of links currently in the list.
 * - @c le_sls_IsListCorrupted() - Checks if the list is corrupted.
 *
 *
 * @section sls_fifo Queues and Stacks
 *
 * This implementation of linked lists can easily be used as either queues or stacks.
 *
 * To use the list as a queue, restrict additions to the list to @c le_sls_Queue() and removals
 * from the list to @c le_sls_Pop().
 *
 * To use the list as a stack, restrict additions to the list to @c le_sls_Stack() and removals
 * from the list to @c le_sls_Pop().
 *
 *
 * @section sls_synch Thread Safety and Re-Entrancy
 *
 * All linked list function calls are re-entrant and thread safe themselves, but if the nodes and/or
 * list object are shared by multiple threads, then explicit steps must be taken to maintain mutual
 * exclusion of access. If you're accessing the same list from multiple threads, you @e must use a
 * @ref c_mutex "mutex" or some other form of thread synchronization to ensure only one thread
 * accesses the list at a time.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */


/** @file le_singlyLinkedList.h
 *
 * Legato @ref c_singlyLinkedList include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_SINGLY_LINKED_LIST_INCLUDE_GUARD
#define LEGATO_SINGLY_LINKED_LIST_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * This link object must be included in each user node.  The node's link object is used to add the
 * node to a list.  A node may have multiple link objects which would allow the node to be part of
 * multiple lists simultaneously.  This link object must be initialized by assigning
 * LE_SLS_LINK_INIT to it.
 *
 * @warning The user MUST NOT access the contents of this structure directly.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sls_Link
{
    struct le_sls_Link* nextPtr;       ///< Next link pointer.
}
le_sls_Link_t;


//--------------------------------------------------------------------------------------------------
/**
 * This is the list object.  Create this list object and initialize it by assigning
 * LE_SLS_LIST_INIT to it.
 *
 * @warning DON'T access the contents of this structure directly.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sls_Link_t* tailLinkPtr;        ///< Tail link pointer.
}
le_sls_List_t;


//--------------------------------------------------------------------------------------------------
/**
 * This is a comparator function for sorting a list.
 *
 * This must return true if @c a goes before @c b in the list.
 */
//--------------------------------------------------------------------------------------------------
typedef bool (*le_sls_LessThanFunc_t)(le_sls_Link_t* a, le_sls_Link_t* b);


//--------------------------------------------------------------------------------------------------
/**
 * When a list is created, it must be initialized by assigning this macro to the list before the list
 * can be used.
 */
//--------------------------------------------------------------------------------------------------
#define LE_SLS_LIST_INIT (le_sls_List_t){NULL}


//--------------------------------------------------------------------------------------------------
/**
 * When a link is created, it must be initialized by assigning this macro to the link before it can
 * be used.
 */
//--------------------------------------------------------------------------------------------------
#define LE_SLS_LINK_INIT (le_sls_Link_t){NULL}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a link at the head of the list.
 */
//--------------------------------------------------------------------------------------------------
void le_sls_Stack
(
    le_sls_List_t* listPtr,            ///< [IN] List to add to.
    le_sls_Link_t* newLinkPtr          ///< [IN] New link to add.
);


//--------------------------------------------------------------------------------------------------
/**
 * Adds a link to the tail of the list.
 */
//--------------------------------------------------------------------------------------------------
void le_sls_Queue
(
    le_sls_List_t* listPtr,            ///< [IN] List to add to.
    le_sls_Link_t* newLinkPtr          ///< [IN] New link to add.
);


//--------------------------------------------------------------------------------------------------
/**
 * Adds a link after currentLinkPtr, or to the beginning of the list if currentLinkPtr is NULL.
 * Ensure that currentLinkPtr is in the list (or NULL) otherwise the behaviour of this function is
 * undefined.
 */
//--------------------------------------------------------------------------------------------------
void le_sls_AddAfter
(
    le_sls_List_t* listPtr,            ///< [IN] List to add to.
    le_sls_Link_t* currentLinkPtr,     ///< [IN] New link will be inserted after this link.
    le_sls_Link_t* newLinkPtr          ///< [IN] New link to add.
);


//--------------------------------------------------------------------------------------------------
/**
 * Removes the link found after currentLinkPtr.  The user must ensure that currentLinkPtr is in the
 * list otherwise the behaviour of this function is undefined.
 *
 * @return
 *      Pointer to the removed link.
 *      NULL if there are no more links in the list after currentLinkPtr.
 */
//--------------------------------------------------------------------------------------------------
le_sls_Link_t* le_sls_RemoveAfter
(
    le_sls_List_t* listPtr,            ///< [IN] The list to remove from.
    le_sls_Link_t* currentLinkPtr      ///< [IN] The link after this one will be removed from the
                                       ///<      list.
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
le_sls_Link_t* le_sls_Pop
(
    le_sls_List_t* listPtr             ///< [IN] List to remove from.
);


//--------------------------------------------------------------------------------------------------
/**
 * Returns the link at the head of the list without removing it from the list.
 *
 * @return
 *     Pointer to the head link if successful.
 *      NULL if the list is empty.
 */
//--------------------------------------------------------------------------------------------------
le_sls_Link_t* le_sls_Peek
(
    const le_sls_List_t* listPtr            ///< [IN] The list.
);


//------------------------------------------------------------------------------------------------------------
/**
 * Returns the link at the tail of the list without removing it from the list.
 *
 * @return
 *      A pointer to the tail link if successful.
 *      NULL if the list is empty.
 */
//------------------------------------------------------------------------------------------------------------
le_sls_Link_t* le_sls_PeekTail
(
    const le_sls_List_t* listPtr            ///< [IN] The list.
);


//--------------------------------------------------------------------------------------------------
/**
 * Returns the link next to currentLinkPtr (i.e., the link beside currentLinkPtr that's closer to the
 * tail) without removing it from the list. Ensure currentLinkPtr is in the list
 * otherwise the behaviour of this function is undefined.
 *
 * @return
 *      Pointer to the next link if successful.
 *      NULL if there is no link next to the currentLinkPtr (currentLinkPtr is at the tail of the
 *      list).
 */
//--------------------------------------------------------------------------------------------------
le_sls_Link_t* le_sls_PeekNext
(
    const le_sls_List_t* listPtr,           ///< [IN] List containing currentLinkPtr.
    const le_sls_Link_t* currentLinkPtr     ///< [IN] Get the link that is relative to this link.
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks if a list is empty.
 *
 * @return
 *      true if empty, false if not empty.
 */
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE bool le_sls_IsEmpty
(
    const le_sls_List_t* listPtr            ///< [IN] The list.
)
//--------------------------------------------------------------------------------------------------
{
    return (le_sls_Peek(listPtr) == NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sort a list in ascending order.
 */
//--------------------------------------------------------------------------------------------------
void le_sls_Sort
(
    le_sls_List_t* listPtr,                 ///< [IN] List to sort
    le_sls_LessThanFunc_t comparatorPtr     ///< [IN] Comparator function for sorting
);

//--------------------------------------------------------------------------------------------------
/**
 * Checks if a link is in the list.
 *
 * @return
 *    - true if the link is in the list.
 *    - false if the link is not in the list.
 */
//--------------------------------------------------------------------------------------------------
bool le_sls_IsInList
(
    const le_sls_List_t* listPtr,    ///< [IN] List to check.
    const le_sls_Link_t* linkPtr     ///< [IN] Check if this link is in the list.
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
LE_DECLARE_INLINE bool le_sls_IsHead
(
    const le_sls_List_t* listPtr,    ///< [IN] List to check.
    const le_sls_Link_t* linkPtr     ///< [IN] Check if this link is at the head of the list.
)
{
    return (le_sls_Peek(listPtr) == linkPtr);
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
LE_DECLARE_INLINE bool le_sls_IsTail
(
    const le_sls_List_t* listPtr,    ///< [IN] List to check.
    const le_sls_Link_t* linkPtr     ///< [IN] Check if this link is at the tail of the list.
)
{
    return (le_sls_PeekTail(listPtr) == linkPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Returns the number of links in a list.
 *
 * @return
 *      Number of links.
 */
//--------------------------------------------------------------------------------------------------
size_t le_sls_NumLinks
(
    const le_sls_List_t* listPtr       ///< [IN] List to count.
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
bool le_sls_IsListCorrupted
(
    const le_sls_List_t* listPtr    ///< [IN] List to check.
);

//--------------------------------------------------------------------------------------------------
/**
 * Simple iteration through a singly linked list
 */
//--------------------------------------------------------------------------------------------------
#define LE_SLS_FOREACH(listPtr, iteratorPtr, type, member)              \
    for ((iteratorPtr) = CONTAINER_OF(le_sls_Peek(listPtr), type, member); \
         &((iteratorPtr)->member);                                      \
         (iteratorPtr) = CONTAINER_OF(le_sls_PeekNext((listPtr),&((iteratorPtr)->member)), \
                                      type, member))



#endif  // LEGATO_SINGLY_LINKED_LIST_INCLUDE_GUARD

