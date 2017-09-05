 /** @file doublyLinkedList.c
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#include "legato.h"


//------------------------------------------------------------------------------------------------------------
/**
 * Adds a link at the head of the list.
 */
//------------------------------------------------------------------------------------------------------------
void le_dls_Stack
(
    le_dls_List_t* listPtr,            ///< [IN] The list to add to.
    le_dls_Link_t* newLinkPtr          ///< [IN] The new link to add.
)
{
    if (listPtr->headLinkPtr == NULL)
    {
        // Add to an empty list.
        newLinkPtr->nextPtr = newLinkPtr;
        newLinkPtr->prevPtr = newLinkPtr;

        listPtr->headLinkPtr = newLinkPtr;
    }
    else
    {
        le_dls_AddBefore(listPtr, listPtr->headLinkPtr, newLinkPtr);
    }
}


//------------------------------------------------------------------------------------------------------------
/**
 * Adds a link to the tail of the list.
 */
//------------------------------------------------------------------------------------------------------------
void le_dls_Queue
(
    le_dls_List_t* listPtr,            ///< [IN] The list to add to.
    le_dls_Link_t* newLinkPtr          ///< [IN] The new link to add.
)
{
    if (listPtr->headLinkPtr == NULL)
    {
        // Add to an empty list.
        newLinkPtr->nextPtr = newLinkPtr;
        newLinkPtr->prevPtr = newLinkPtr;

        listPtr->headLinkPtr = newLinkPtr;
    }
    else
    {
        le_dls_AddAfter(listPtr, listPtr->headLinkPtr->prevPtr, newLinkPtr);
    }
}


//------------------------------------------------------------------------------------------------------------
/**
 * Adds a link after currentLinkPtr.  The user must ensure that currentLinkPtr is in the list otherwise
 * the behaviour of this function is undefined.
 */
//------------------------------------------------------------------------------------------------------------
void le_dls_AddAfter
(
    le_dls_List_t* listPtr,            ///< [IN] The list to add to.
    le_dls_Link_t* currentLinkPtr,     ///< [IN] The new link will be inserted after this link.
    le_dls_Link_t* newLinkPtr          ///< [IN] The new link to add.
)
{
    (void)listPtr;  // Cast to void to avoid compiler warning.  Remove if listPtr is used.

    newLinkPtr->nextPtr = currentLinkPtr->nextPtr;
    newLinkPtr->prevPtr = currentLinkPtr;

    currentLinkPtr->nextPtr->prevPtr = newLinkPtr;
    currentLinkPtr->nextPtr = newLinkPtr;
}


//------------------------------------------------------------------------------------------------------------
/**
 * Adds a link after currentLinkPtr.  The user must ensure that currentLinkPtr is in the list otherwise
 * the behaviour of this function is undefined.
 */
//------------------------------------------------------------------------------------------------------------
void le_dls_AddBefore
(
    le_dls_List_t* listPtr,            ///< [IN] The list to add to.
    le_dls_Link_t* currentLinkPtr,     ///< [IN] The new link will be inserted before this link.
    le_dls_Link_t* newLinkPtr          ///< [IN] The new link to add.
)
{
    newLinkPtr->nextPtr = currentLinkPtr;
    newLinkPtr->prevPtr = currentLinkPtr->prevPtr;

    currentLinkPtr->prevPtr->nextPtr = newLinkPtr;
    currentLinkPtr->prevPtr = newLinkPtr;

    // Update the list's head pointer if neccessary.
    if (currentLinkPtr == listPtr->headLinkPtr)
    {
        listPtr->headLinkPtr = newLinkPtr;
    }
}


//------------------------------------------------------------------------------------------------------------
/**
 * Removes and returns the link at the head of the list.
 *
 * @return
 *      The removed link.
 *      NULL if the link is not available because the list is empty.
 */
//------------------------------------------------------------------------------------------------------------
le_dls_Link_t* le_dls_Pop
(
    le_dls_List_t* listPtr             ///< [IN] The list to remove from.
)
{
    // Check parameters.
    if (listPtr->headLinkPtr == NULL)
    {
        // List is empty.
        return NULL;
    }

    le_dls_Link_t* linkToPopPtr = listPtr->headLinkPtr;

    le_dls_Remove(listPtr, linkToPopPtr);

    return linkToPopPtr;
}


//------------------------------------------------------------------------------------------------------------
/**
 * Removes and returns the link at the tail of the list.
 *
 * @return
 *      The removed link.
 *      NULL if the link is not available because the list is empty.
 */
//------------------------------------------------------------------------------------------------------------
le_dls_Link_t* le_dls_PopTail
(
    le_dls_List_t* listPtr             ///< [IN] The list to remove from.
)
{
    if (listPtr->headLinkPtr == NULL)
    {
        // List is empty.
        return NULL;
    }

    le_dls_Link_t* linkToPopPtr = listPtr->headLinkPtr->prevPtr;

    le_dls_Remove(listPtr, linkToPopPtr);

    return linkToPopPtr;
}


//------------------------------------------------------------------------------------------------------------
/**
 * Removes the specified link from the list.  The user must ensure that currentLinkPtr is in the list otherwise
 * the behaviour of this function is undefined.
 */
//------------------------------------------------------------------------------------------------------------
void le_dls_Remove
(
    le_dls_List_t* listPtr,             ///< [IN] The list to remove from.
    le_dls_Link_t* linkToRemovePtr      ///< [IN] The link to remove.
)
{
    if (linkToRemovePtr->nextPtr == linkToRemovePtr)
    {
        // There is only one link so empty out the list.
        listPtr->headLinkPtr = NULL;
    }
    else
    {
        le_dls_Link_t* nextLinkPtr = linkToRemovePtr->nextPtr;
        le_dls_Link_t* prevLinkPtr = linkToRemovePtr->prevPtr;

        nextLinkPtr->prevPtr = prevLinkPtr;
        prevLinkPtr->nextPtr = nextLinkPtr;

        // Update the head pointer if necessary.
        if (linkToRemovePtr == listPtr->headLinkPtr)
        {
            listPtr->headLinkPtr = nextLinkPtr;
        }
    }

    linkToRemovePtr->nextPtr = NULL;
    linkToRemovePtr->prevPtr = NULL;
}


//------------------------------------------------------------------------------------------------------------
/**
 * Returns the link at the head of the list without removing it from the list.
 *
 * @return
 *      A pointer to the head link if successful.
 *      NULL if the list is empty.
 */
//------------------------------------------------------------------------------------------------------------
le_dls_Link_t* le_dls_Peek
(
    const le_dls_List_t* listPtr            ///< [IN] The list.
)
{
    return listPtr->headLinkPtr;
}


//------------------------------------------------------------------------------------------------------------
/**
 * Returns the link at the tail of the list without removing it from the list.
 *
 * @return
 *      A pointer to the tail link if successful.
 *      NULL if the list is empty.
 */
//------------------------------------------------------------------------------------------------------------
le_dls_Link_t* le_dls_PeekTail
(
    const le_dls_List_t* listPtr            ///< [IN] The list.
)
{
    if (listPtr->headLinkPtr == NULL)
    {
        return NULL;
    }
    else
    {
        return listPtr->headLinkPtr->prevPtr;
    }
}


//------------------------------------------------------------------------------------------------------------
/**
 * Returns the link next to currentLinkPtr (ie. the link beside currentLinkPtr that is closer to the tail)
 * without removing it from the list.  The user must ensure that currentLinkPtr is in the list otherwise
 * the behaviour of this function is undefined.
 *
 * @return
 *      A pointer to the next link if successful.
 *      NULL if there is no link next to the currentLinkPtr (currentLinkPtr is at the tail of the list).
 */
//------------------------------------------------------------------------------------------------------------
le_dls_Link_t* le_dls_PeekNext
(
    const le_dls_List_t* listPtr,           ///< [IN] The list containing currentLinkPtr.
    const le_dls_Link_t* currentLinkPtr     ///< [IN] Get the link that is relative to this link.
)
{
    if (currentLinkPtr == listPtr->headLinkPtr->prevPtr)
    {
        // We are at the tail already so there is no next link.
        return NULL;
    }

    return currentLinkPtr->nextPtr;
}


//------------------------------------------------------------------------------------------------------------
/**
 * Returns the link previous to currentLinkPtr without removing it from the list.  The user must ensure that
 * currentLinkPtr is in the list otherwise the behaviour of this function is undefined.
 *
 * @return
 *      A pointer to the previous link if successful.
 *      NULL if there is no link previous to the currentLinkPtr (currentLinkPtr is at the head of the list).
 */
//------------------------------------------------------------------------------------------------------------
le_dls_Link_t* le_dls_PeekPrev
(
    const le_dls_List_t* listPtr,          ///< [IN] The list containing currentLinkPtr.
    const le_dls_Link_t* currentLinkPtr    ///< [IN] Get the link that is relative to this link.
)
{
    if (currentLinkPtr == listPtr->headLinkPtr)
    {
        // We are at the head already so there is no prev link.
        return NULL;
    }

    return currentLinkPtr->prevPtr;
}


//------------------------------------------------------------------------------------------------------------
/**
 * Swaps the position of two links in the list.  The user must ensure that both links are in the list
 * otherwise the behaviour of this function is undefined.
 */
//------------------------------------------------------------------------------------------------------------
void le_dls_Swap
(
    le_dls_List_t* listPtr,         ///< [IN] The list containing the links to swap.
    le_dls_Link_t* linkPtr,         ///< [IN] One of the two link pointers to swap.
    le_dls_Link_t* otherLinkPtr     ///< [IN] The other link pointer to swap.
)
{
    if (linkPtr->nextPtr == otherLinkPtr)
    {
        if (linkPtr->prevPtr == otherLinkPtr)
        {
            // There are only at most two nodes in the list.  So we just have to move the head.
            listPtr->headLinkPtr = listPtr->headLinkPtr->nextPtr;

            return;  // Don't need to update the head pointer anymore.
        }
        else
        {
            // linkPtr is right before otherLinkPtr
            linkPtr->prevPtr->nextPtr = otherLinkPtr;
            otherLinkPtr->nextPtr->prevPtr = linkPtr;

            linkPtr->nextPtr = otherLinkPtr->nextPtr;
            otherLinkPtr->prevPtr = linkPtr->prevPtr;

            linkPtr->prevPtr = otherLinkPtr;
            otherLinkPtr->nextPtr = linkPtr;
        }
    }
    else if (linkPtr->prevPtr == otherLinkPtr)
    {
        // otherLinkPtr is right before linkPtr;
        otherLinkPtr->prevPtr->nextPtr = linkPtr;
        linkPtr->nextPtr->prevPtr = otherLinkPtr;

        otherLinkPtr->nextPtr = linkPtr->nextPtr;
        linkPtr->prevPtr = otherLinkPtr->prevPtr;

        otherLinkPtr->prevPtr = linkPtr;
        linkPtr->nextPtr = otherLinkPtr;
    }
    else
    {
        le_dls_Link_t* nextLinkPtr = linkPtr->nextPtr;
        le_dls_Link_t* prevLinkPtr = linkPtr->prevPtr;
        le_dls_Link_t* otherNextLinkPtr = otherLinkPtr->nextPtr;
        le_dls_Link_t* otherPrevLinkPtr = otherLinkPtr->prevPtr;

        // Swap the linkPtr's neighbour's links.
        nextLinkPtr->prevPtr = otherLinkPtr;
        prevLinkPtr->nextPtr = otherLinkPtr;

        // Swap the otherLinkPtr's neighbour's links.
        otherNextLinkPtr->prevPtr = linkPtr;
        otherPrevLinkPtr->nextPtr = linkPtr;

        // Swap the linkPtr's links.
        linkPtr->nextPtr = otherNextLinkPtr;
        linkPtr->prevPtr = otherPrevLinkPtr;

        // Swap the otherLinkPtr's links.
        otherLinkPtr->nextPtr = nextLinkPtr;
        otherLinkPtr->prevPtr = prevLinkPtr;
    }

    // Update the head pointer.
    if (linkPtr == listPtr->headLinkPtr)
    {
        listPtr->headLinkPtr = otherLinkPtr;
    }
    else if (otherLinkPtr == listPtr->headLinkPtr)
    {
        listPtr->headLinkPtr = linkPtr;
    }
}


//------------------------------------------------------------------------------------------------------------
/**
 * Checks if a link is in the list.
 *
 * @return
 *      true if the link is in the list.
 *      false if the link is not in the list.
 */
//------------------------------------------------------------------------------------------------------------
bool le_dls_IsInList
(
    const le_dls_List_t* listPtr,    ///< [IN] The list to check.
    const le_dls_Link_t* linkPtr     ///< [IN] Check if this link is in the list.
)
{
    if (listPtr->headLinkPtr == NULL)
    {
        return false;
    }

    // Start at the head of the list looking for the link.
    le_dls_Link_t* currentLinkPtr = listPtr->headLinkPtr;
    do
    {
        if (currentLinkPtr == linkPtr)
        {
            return true;
        }

        // Move to the next link
        currentLinkPtr = currentLinkPtr->nextPtr;

    } while (currentLinkPtr != listPtr->headLinkPtr);  // Stop when we've come full circle.

    return false;
}


//------------------------------------------------------------------------------------------------------------
/**
 * Counts the number of links in a list.
 *
 * @return
 *      The number of links.
 */
//------------------------------------------------------------------------------------------------------------
size_t le_dls_NumLinks
(
    const le_dls_List_t* listPtr       ///< [IN] The list to count.
)
{
    if (listPtr->headLinkPtr == NULL)
    {
        return 0;
    }

    // Start at the head of the list.
    le_dls_Link_t* currentLinkPtr = listPtr->headLinkPtr;
    size_t count = 0;
    do
    {
        count++;

        // Move to the next link
        currentLinkPtr = currentLinkPtr->nextPtr;

    } while (currentLinkPtr != listPtr->headLinkPtr);  // Stop when we've come full circle.

    return count;
}


//------------------------------------------------------------------------------------------------------------
/**
 * Checks if the list is corrupted.
 *
 * @return
 *      true if the list is corrupted.
 *      false if the list is not corrupted.
 */
//------------------------------------------------------------------------------------------------------------
bool le_dls_IsListCorrupted
(
    const le_dls_List_t* listPtr    ///< [IN] The list to check.
)
{
    if (listPtr == NULL)
    {
        return true;
    }

    if (listPtr->headLinkPtr == NULL)
    {
        return false;
    }

    // Go through the list and check each link.
    le_dls_Link_t* currentLinkPtr = listPtr->headLinkPtr;
    do
    {
        if (currentLinkPtr->nextPtr == NULL)
        {
            LE_CRIT("currentLinkPtr->nextPtr is NULL");
            return true;
        }
        if (currentLinkPtr->prevPtr == NULL)
        {
            LE_CRIT("currentLinkPtr->prevPtr is NULL");
            return true;
        }
        if (currentLinkPtr->nextPtr->prevPtr != currentLinkPtr)
        {
            LE_CRIT("currentLinkPtr->nextPtr->prevPtr is not currentLinkPtr");
            return true;
        }
        if (currentLinkPtr->prevPtr->nextPtr != currentLinkPtr)
        {
            LE_CRIT("currentLinkPtr->prevPtr->nextPtr is not currentLinkPtr");
            return true;
        }

        currentLinkPtr = currentLinkPtr->nextPtr;

    } while (currentLinkPtr != listPtr->headLinkPtr);

    return false;
}


