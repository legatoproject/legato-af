 /** @file singlyLinkedList.c
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#include "legato.h"


//------------------------------------------------------------------------------------------------------------
/**
 * Adds a link at the head of the list.
 */
//------------------------------------------------------------------------------------------------------------
void le_sls_Stack
(
    le_sls_List_t* listPtr,            ///< [IN] The list to add to.
    le_sls_Link_t* newLinkPtr          ///< [IN] The new link to add.
)
{
    if (listPtr->tailLinkPtr == NULL)
    {
        // Add to an empty list.
        newLinkPtr->nextPtr = newLinkPtr;

        listPtr->tailLinkPtr = newLinkPtr;
    }
    else
    {
        // Set the new link's next pointer to the head of the list.
        newLinkPtr->nextPtr = listPtr->tailLinkPtr->nextPtr;

        // Set the tail's next pointer to the new link making the new link the head.
        listPtr->tailLinkPtr->nextPtr = newLinkPtr;
    }
}


//------------------------------------------------------------------------------------------------------------
/**
 * Adds a link to the tail of the list.
 */
//------------------------------------------------------------------------------------------------------------
void le_sls_Queue
(
    le_sls_List_t* listPtr,            ///< [IN] The list to add to.
    le_sls_Link_t* newLinkPtr          ///< [IN] The new link to add.
)
{
    if (listPtr->tailLinkPtr == NULL)
    {
        // Add to an empty list.
        newLinkPtr->nextPtr = newLinkPtr;

        listPtr->tailLinkPtr = newLinkPtr;
    }
    else
    {
        le_sls_AddAfter(listPtr, listPtr->tailLinkPtr, newLinkPtr);
    }
}


//------------------------------------------------------------------------------------------------------------
/**
 * Adds a link after currentLinkPtr.  The user must ensure that currentLinkPtr is in the list otherwise
 * the behaviour of this function is undefined.
 */
//------------------------------------------------------------------------------------------------------------
void le_sls_AddAfter
(
    le_sls_List_t* listPtr,            ///< [IN] The list to add to.
    le_sls_Link_t* currentLinkPtr,     ///< [IN] The new link will be inserted after this link.
    le_sls_Link_t* newLinkPtr          ///< [IN] The new link to add.
)
{
    newLinkPtr->nextPtr = currentLinkPtr->nextPtr;
    currentLinkPtr->nextPtr = newLinkPtr;

    if (currentLinkPtr == listPtr->tailLinkPtr)
    {
        // Update the tail pointer.
        listPtr->tailLinkPtr = newLinkPtr;
    }
}


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
)
{
    // Are there any items in the list after the current one?
    le_sls_Link_t* nextPtr = currentLinkPtr->nextPtr;

    // if the next item in the list is pointing to the head, let's not remove it
    if (nextPtr == listPtr->tailLinkPtr->nextPtr)
    {
        // Nope, so there isn't anything to remove.
        return NULL;
    }

    // Bump out the link in the middle and return a pointer to it so that the caller can decide what
    // to do with it.
    currentLinkPtr->nextPtr = nextPtr->nextPtr;

    // if the item getting removed is the last one in the list, update the tail pointer
    if (nextPtr == listPtr->tailLinkPtr)
    {
        listPtr->tailLinkPtr = currentLinkPtr;
    }

    // remove the item from the list; this item can be freed after this step
    nextPtr->nextPtr = NULL;

    return nextPtr;
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
le_sls_Link_t* le_sls_Pop
(
    le_sls_List_t* listPtr             ///< [IN] The list to remove from.
)
{
    if (listPtr->tailLinkPtr == NULL)
    {
        // List is empty.
        return NULL;
    }
    else if (listPtr->tailLinkPtr->nextPtr == listPtr->tailLinkPtr)
    {
        // List only has one node.
        le_sls_Link_t* linkToPopPtr = listPtr->tailLinkPtr;

        listPtr->tailLinkPtr = NULL;

        return linkToPopPtr;
    }
    else
    {
        le_sls_Link_t* linkToPopPtr = listPtr->tailLinkPtr->nextPtr;

        listPtr->tailLinkPtr->nextPtr = linkToPopPtr->nextPtr;

        return linkToPopPtr;
    }
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
le_sls_Link_t* le_sls_Peek
(
    const le_sls_List_t* listPtr            ///< [IN] The list.
)
{
    if (listPtr->tailLinkPtr == NULL)
    {
        return NULL;
    }

    return listPtr->tailLinkPtr->nextPtr;
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
le_sls_Link_t* le_sls_PeekTail
(
    const le_sls_List_t* listPtr            ///< [IN] The list.
)
{
    return listPtr->tailLinkPtr;
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
le_sls_Link_t* le_sls_PeekNext
(
    const le_sls_List_t* listPtr,           ///< [IN] The list containing currentLinkPtr.
    const le_sls_Link_t* currentLinkPtr     ///< [IN] Get the link that is relative to this link.
)
{
    if (currentLinkPtr == listPtr->tailLinkPtr)
    {
        // We are at the tail already so there is no next link.
        return NULL;
    }

    return currentLinkPtr->nextPtr;
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
bool le_sls_IsInList
(
    const le_sls_List_t* listPtr,    ///< [IN] The list to check.
    const le_sls_Link_t* linkPtr     ///< [IN] Check if this link is in the list.
)
{
    if (listPtr->tailLinkPtr == NULL)
    {
        return false;
    }

    // Go through list looking for the link.
    le_sls_Link_t* currentLinkPtr = listPtr->tailLinkPtr;
    do
    {
        if (currentLinkPtr == linkPtr)
        {
            return true;
        }

        // Move to the next link
        currentLinkPtr = currentLinkPtr->nextPtr;

    } while (currentLinkPtr != listPtr->tailLinkPtr);  // Stop when we've come full circle.

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
size_t le_sls_NumLinks
(
    const le_sls_List_t* listPtr       ///< [IN] The list to count.
)
{
    if (listPtr->tailLinkPtr == NULL)
    {
        return 0;
    }

    // Go through the list and count the nodes.
    le_sls_Link_t* currentLinkPtr = listPtr->tailLinkPtr;
    size_t count = 0;
    do
    {
        count++;

        // Move to the next link
        currentLinkPtr = currentLinkPtr->nextPtr;

    } while (currentLinkPtr != listPtr->tailLinkPtr);  // Stop when we've come full circle.

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
bool le_sls_IsListCorrupted
(
    const le_sls_List_t* listPtr    ///< [IN] The list to check.
)
{
    if (listPtr == NULL)
    {
        return true;
    }

    if (listPtr->tailLinkPtr == NULL)
    {
        return false;
    }

    // Go through the list and check each link.
    le_sls_Link_t* currentLinkPtr = listPtr->tailLinkPtr;
    do
    {
        if (currentLinkPtr->nextPtr == NULL)
        {
            return true;
        }

        currentLinkPtr = currentLinkPtr->nextPtr;

    } while (currentLinkPtr != listPtr->tailLinkPtr);

    return false;
}

