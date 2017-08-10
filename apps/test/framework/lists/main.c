 /**
  * This module is for unit testing the le_doublyLinkedList module in the legato
  * runtime library (liblegato.so).
  *
  * The following is a list of the test cases:
  *
  * - Multiple list creation.
  * - link insertions.
  * - link removal.
  * - Accessing nodes.
  * - Checking list consistencies.
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#include <time.h>
#include "legato.h"


#define LIST_SIZE           101
#define REMOVE_THRESHOLD    RAND_MAX / 2
#define REMOVE_SIZE         1000

static le_result_t TestDoublyLinkLists(size_t maxListSize);
static le_result_t TestSinglyLinkLists(size_t maxListSize);

COMPONENT_INIT
{
    uint32_t maxListSize = 0;
    le_result_t result;

    // Get the max size list
    if (le_arg_NumArgs() >= 1)
    {
        const char* maxListSizePtr = le_arg_GetArg(1);
        if (NULL == maxListSizePtr)
        {
            LE_ERROR("maxListSizePtr is NULL");
            exit(EXIT_FAILURE);
        }
        maxListSize = atoi(maxListSizePtr);
    }

    if (maxListSize <= 0)
    {
        printf("Setting list size to %d.\n", LIST_SIZE);
        maxListSize = LIST_SIZE;
    }

    result = TestDoublyLinkLists(maxListSize);

    if (result != LE_OK)
    {
        exit(result);
    }

    exit(TestSinglyLinkLists(maxListSize));
}


static le_result_t TestDoublyLinkLists(size_t maxListSize)
{
    // Node definition.
    typedef struct
    {
        le_dls_Link_t link;
        uint32_t id;
    }
    idRecord_t;

    int i;
    le_dls_List_t list0, list1;
    le_dls_Link_t* removedLinksPtr0[REMOVE_SIZE] = {NULL};
    le_dls_Link_t* removedLinksPtr1[REMOVE_SIZE] = {NULL};

    printf("\n");
    printf("*** Unit Test for le_doublyLinkedList module. ***\n");

    //
    // Multiple list creation
    //
    // Initialize the lists
    list0 = LE_DLS_LIST_INIT;
    list1 = LE_DLS_LIST_INIT;
    printf("Two doubly linked lists were successfully created.\n");


    //
    // Attempt to query empty list
    //
    if ( (le_dls_Peek(&list0) != NULL) || (le_dls_PeekTail(&list0) != NULL) ||
         (le_dls_Pop(&list0) != NULL) || (le_dls_PopTail(&list0) != NULL) )
    {
        printf("Query of empty list failed: %d", __LINE__);
        return LE_FAULT;
    }
    printf("Query of empty list correct.\n");


    //
    // Node insertions
    //
    {
        idRecord_t* newNodePtr;

        // Insert to the tail
        for (i = 0; i < maxListSize; i++)
        {
            // Create the new node
            newNodePtr = (idRecord_t*)malloc(sizeof(idRecord_t));
            if (!newNodePtr)
            {
                printf("newNodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            newNodePtr->id = i;

            // Initialize the link.
            newNodePtr->link = LE_DLS_LINK_INIT;

            // Insert the new node to the tail.
            le_dls_Queue(&list0, &(newNodePtr->link));
        }
        printf("%zu nodes were added to the tail of list0.\n", maxListSize);

        // Insert to the head
        for (i = 0; i < maxListSize; i++)
        {
            // Create the new node
            newNodePtr = (idRecord_t*)malloc(sizeof(idRecord_t));
            if (!newNodePtr)
            {
                printf("newNodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            newNodePtr->id = i;

            // Initialize the link.
            newNodePtr->link = LE_DLS_LINK_INIT;

            // Insert the new node to the tail.
            le_dls_Stack(&list1, &(newNodePtr->link));
        }
        printf("%zu nodes were added to the head of list1.\n", maxListSize);
    }

    //
    // Check that all the nodes have been added properly
    //
    {
        idRecord_t* nodePtr;
        le_dls_Link_t* link0Ptr = le_dls_Peek(&list0);
        le_dls_Link_t* link1Ptr = le_dls_PeekTail(&list1);

        if ( (link0Ptr == NULL) || (link1Ptr == NULL) )
        {
            printf("Link error: %d", __LINE__);
            return LE_FAULT;
        }

        i = 0;
        do
        {
            // Get the node from list 0
            nodePtr = CONTAINER_OF(link0Ptr, idRecord_t, link);
            if (!nodePtr)
            {
                printf("nodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            // Check the node.
            if ( nodePtr->id != i)
            {
                printf("Link error: %d", __LINE__);
                return LE_FAULT;
            }

            // Get the node from list 1
            nodePtr = CONTAINER_OF(link1Ptr, idRecord_t, link);
            if (!nodePtr)
            {
                printf("nodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            // Check the node.
            if ( nodePtr->id != i)
            {
                printf("Link error: %d", __LINE__);
                return LE_FAULT;
            }

            // Move to the next node.
            link0Ptr = le_dls_PeekNext(&list0, link0Ptr);
            link1Ptr = le_dls_PeekPrev(&list1, link1Ptr);
            i++;

        } while (link0Ptr != NULL);

        // Make sure everything is correct.
        if ( (i != maxListSize) || (link1Ptr != NULL) )
        {
            printf("Link error: %d", __LINE__);
            return LE_FAULT;
        }
    }

    printf("Checked that all nodes added to the head and tails are all correct.\n");


    //
    // Remove random nodes
    //

    //seed the random number generator with the clock
    srand((unsigned int)clock());

    {
        // Start at the end of the lists and randomly remove links.
        le_dls_Link_t* linkToRemovePtr;
        le_dls_Link_t* link0Ptr = le_dls_PeekTail(&list0);
        le_dls_Link_t* link1Ptr = le_dls_Peek(&list1);

        int r0 = 0;
        int r1 = 0;
        do
        {
            // For list 0
            if ( (rand() < REMOVE_THRESHOLD) && (r0 < REMOVE_SIZE) )
            {
                // Mark this node for removal.
                linkToRemovePtr = link0Ptr;
                if (!linkToRemovePtr)
                {
                   printf("linkToRemovePtr is NULL: %d", __LINE__);
                   return LE_FAULT;
                }

                // Move to the next node.
                link0Ptr = le_dls_PeekPrev(&list0, link0Ptr);

                // Remove the node.
                le_dls_Remove(&list0, linkToRemovePtr);

                // Store the removed node for later use.
                removedLinksPtr0[r0++] = linkToRemovePtr;
            }
            else
            {
                // Just move one
                link0Ptr = le_dls_PeekPrev(&list0, link0Ptr);
            }


            // For list 1
            if ( (rand() < REMOVE_THRESHOLD) && (r1 < REMOVE_SIZE) )
            {
                // Mark this node for removal.
                linkToRemovePtr = link1Ptr;
                if (!linkToRemovePtr)
                {
                   printf("linkToRemovePtr is NULL: %d", __LINE__);
                   return LE_FAULT;
                }

                // Move to the next node.
                link1Ptr = le_dls_PeekNext(&list1, link1Ptr);

                // Remove the node.
                le_dls_Remove(&list1, linkToRemovePtr);

                // Store the removed node for later use.
                removedLinksPtr1[r1++] = linkToRemovePtr;
            }
            else
            {
                // Just move to the next node
                link1Ptr = le_dls_PeekNext(&list1, link1Ptr);
            }
        } while (link0Ptr != NULL);

        printf("Randomly removed %d nodes from list0.\n", r0);
        printf("Randomly removed %d nodes from list1.\n", r1);
    }


    //
    // Check that the proper nodes were removed
    //
    {
        int numNodesRemoved = 0;

        // For list 0.
        // Check that the nodes in the removed nodes are indeed not in the list.
        for (i = 0; i < REMOVE_SIZE; i++)
        {
            if (removedLinksPtr0[i] == NULL)
            {
                break;
            }

            if (le_dls_IsInList(&list0, removedLinksPtr0[i]))
            {
                printf("Node removal incorrect: %d", __LINE__);
                return LE_FAULT;
            }

            numNodesRemoved++;
        }

        // Compare the list count.
        if ( (numNodesRemoved != maxListSize - le_dls_NumLinks(&list0)) ||
             (le_dls_NumLinks(&list0) == maxListSize) )
        {
            printf("Node removal incorrect: %d", __LINE__);
            return LE_FAULT;
        }

        // For list 1.
        // Check that the nodes in the removed nodes are indeed not in the list.
        numNodesRemoved = 0;
        for (i = 0; i < REMOVE_SIZE; i++)
        {
            if (removedLinksPtr1[i] == NULL)
            {
                break;
            }

            if (le_dls_IsInList(&list1, removedLinksPtr1[i]))
            {
                printf("Node removal incorrect: %d", __LINE__);
                return LE_FAULT;
            }

            numNodesRemoved++;
        }

        // Compare the list count.
        if ( (numNodesRemoved != maxListSize - le_dls_NumLinks(&list1)) || (le_dls_NumLinks(&list1) == maxListSize) )
        {
            printf("Node removal incorrect: %d", __LINE__);
            return LE_FAULT;
        }
    }

    printf("Checked that nodes were removed correctly.\n");


    //
    // Add the randomly removed nodes back in.
    //
    {
        idRecord_t *nodePtr, *removedNodePtr;
        le_dls_Link_t* linkPtr;

        // For list 0.
        for (i = 0; i < REMOVE_SIZE; i++)
        {
            if (removedLinksPtr0[i] == NULL)
            {
                break;
            }

            removedNodePtr = CONTAINER_OF(removedLinksPtr0[i], idRecord_t, link);

            if (removedNodePtr->id == maxListSize-1)
            {
                le_dls_Queue(&list0, removedLinksPtr0[i]);
            }
            else
            {
                // Search the list for the place to insert this.
                linkPtr = le_dls_PeekTail(&list0);
                do
                {
                    // Get the node
                    nodePtr = CONTAINER_OF(linkPtr, idRecord_t, link);
                    if (!nodePtr)
                    {
                       printf("nodePtr is NULL: %d", __LINE__);
                       return LE_FAULT;
                    }

                    // Find the id that is just before this one.
                    if (nodePtr->id == removedNodePtr->id + 1)
                    {
                        le_dls_AddBefore(&list0, linkPtr, removedLinksPtr0[i]);
                        break;
                    }

                    linkPtr = le_dls_PeekPrev(&list0, linkPtr);

                } while (linkPtr != NULL);
            }
        }


        // For list 1.
        for (i = 0; i < REMOVE_SIZE; i++)
        {
            if (removedLinksPtr1[i] == NULL)
            {
                break;
            }

            removedNodePtr = CONTAINER_OF(removedLinksPtr1[i], idRecord_t, link);

            if (removedNodePtr->id == maxListSize-1)
            {
                le_dls_Stack(&list1, removedLinksPtr1[i]);
            }
            else
            {
                // Search the list for the place to insert this.
                linkPtr = le_dls_Peek(&list1);
                do
                {
                    // Get the node
                    nodePtr = CONTAINER_OF(linkPtr, idRecord_t, link);
                    if (!nodePtr)
                    {
                       printf("nodePtr is NULL: %d", __LINE__);
                       return LE_FAULT;
                    }

                    // Find the id that is just before this one.
                    if (nodePtr->id == removedNodePtr->id + 1)
                    {
                        le_dls_AddAfter(&list1, linkPtr, removedLinksPtr1[i]);
                        break;
                    }

                    linkPtr = le_dls_PeekNext(&list1, linkPtr);

                } while (linkPtr != NULL);
            }
        }
    }

    printf("Added all randomly removed nodes back in.\n");

    //Check that the list is correct.
    {
        idRecord_t* nodePtr;
        le_dls_Link_t* link0Ptr = le_dls_Peek(&list0);
        le_dls_Link_t* link1Ptr = le_dls_PeekTail(&list1);

        if ( (link0Ptr == NULL) || (link1Ptr == NULL) )
        {
            printf("Link error: %d", __LINE__);
            return LE_FAULT;
        }

        i = 0;
        do
        {
            // Get the node from list 0
            nodePtr = CONTAINER_OF(link0Ptr, idRecord_t, link);
            if (!nodePtr)
            {
                printf("nodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            // Check the node.
            if ( nodePtr->id != i)
            {
                printf("Link error: %d", __LINE__);
                return LE_FAULT;
            }

            // Get the node from list 1
            nodePtr = CONTAINER_OF(link1Ptr, idRecord_t, link);
            if (!nodePtr)
            {
                printf("nodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            // Check the node.
            if ( nodePtr->id != i)
            {
                printf("Link error: %d", __LINE__);
                return LE_FAULT;
            }

            // Move to the next node.
            link0Ptr = le_dls_PeekNext(&list0, link0Ptr);
            link1Ptr = le_dls_PeekPrev(&list1, link1Ptr);
            i++;

        } while (link0Ptr != NULL);

        // Make sure everything is correct.
        if ( (i != maxListSize) || (link1Ptr != NULL) )
        {
            printf("Link error: %d", __LINE__);
            return LE_FAULT;
        }
    }

    printf("Checked that all nodes are now added back in in the correct order.\n");


    //
    // Swap nodes.
    //
    {
        //Swap all the nodes in the list so the list is in reverse order.
        le_dls_Link_t* linkPtr, *tmpLinkPtr;
        le_dls_Link_t* otherlinkPtr;
        idRecord_t* nodePtr, *otherNodePtr;

        // For list 0.
        linkPtr = le_dls_Peek(&list0);
        otherlinkPtr = le_dls_PeekTail(&list0);
        for (i = 0; i < (le_dls_NumLinks(&list0) / 2); i++)
        {
            nodePtr = CONTAINER_OF(linkPtr, idRecord_t, link);
            if (!nodePtr)
            {
                printf("nodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            otherNodePtr = CONTAINER_OF(otherlinkPtr, idRecord_t, link);
            if (!otherNodePtr)
            {
                printf("otherNodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            if (nodePtr->id < otherNodePtr->id)
            {
                le_dls_Swap(&list0, linkPtr, otherlinkPtr);
            }
            else
            {
                break;
            }

            // switch the pointers back but not the links.
            tmpLinkPtr = linkPtr;
            linkPtr = otherlinkPtr;
            otherlinkPtr = tmpLinkPtr;

            linkPtr = le_dls_PeekNext(&list0, linkPtr);
            otherlinkPtr = le_dls_PeekPrev(&list0, otherlinkPtr);
        }


        // For list 1.
        linkPtr = le_dls_Peek(&list1);
        otherlinkPtr = le_dls_PeekTail(&list1);
        for (i = 0; i < (le_dls_NumLinks(&list1) / 2); i++)
        {
            nodePtr = CONTAINER_OF(linkPtr, idRecord_t, link);
            if (!nodePtr)
            {
                printf("nodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            otherNodePtr = CONTAINER_OF(otherlinkPtr, idRecord_t, link);
            if (!otherNodePtr)
            {
                printf("otherNodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            if (nodePtr->id > otherNodePtr->id)
            {
                le_dls_Swap(&list1, linkPtr, otherlinkPtr);
            }
            else
            {
                break;
            }

            // switch the pointers back but not the links.
            tmpLinkPtr = linkPtr;
            linkPtr = otherlinkPtr;
            otherlinkPtr = tmpLinkPtr;

            linkPtr = le_dls_PeekNext(&list1, linkPtr);
            otherlinkPtr = le_dls_PeekPrev(&list1, otherlinkPtr);
        }
    }

    printf("Reversed the order of both lists using swap.\n");

    //Check that the list is correct.
    {
        idRecord_t* nodePtr;
        le_dls_Link_t* link0Ptr = le_dls_PeekTail(&list0);
        le_dls_Link_t* link1Ptr = le_dls_Peek(&list1);

        if ( (link0Ptr == NULL) || (link1Ptr == NULL) )
        {
            printf("Link error: %d", __LINE__);
            return LE_FAULT;
        }

        i = 0;
        do
        {
            // Get the node from list 0
            nodePtr = CONTAINER_OF(link0Ptr, idRecord_t, link);
            if (!nodePtr)
            {
                printf("nodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            // Check the node.
            if ( nodePtr->id != i)
            {
                printf("Link error: %d", __LINE__);
                return LE_FAULT;
            }

            // Get the node from list 1
            nodePtr = CONTAINER_OF(link1Ptr, idRecord_t, link);
            if (!nodePtr)
            {
                printf("nodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            // Check the node.
            if ( nodePtr->id != i)
            {
                printf("Link error: %d", __LINE__);
                return LE_FAULT;
            }

            // Move to the next node.
            link0Ptr = le_dls_PeekPrev(&list0, link0Ptr);
            link1Ptr = le_dls_PeekNext(&list1, link1Ptr);
            i++;

        } while (link0Ptr != NULL);

        // Make sure everything is correct.
        if ( (i != maxListSize) || (link1Ptr != NULL) )
        {
            printf("Link error: %d", __LINE__);
            return LE_FAULT;
        }
    }

    printf("Checked that all nodes are now correctly in the reverse order.\n");


    //
    // Pop nodes.
    //
    {
        //pop all of list0 except for one node.  Save the first node using swap before the pop.
        for (i = maxListSize; i > 1; i--)
        {
            // get the first two links.
            le_dls_Link_t* linkPtr = le_dls_Peek(&list0);
            le_dls_Link_t* otherlinkPtr = le_dls_PeekNext(&list0, linkPtr);

            // swap the first two links.
            le_dls_Swap(&list0, linkPtr, otherlinkPtr);

            // pop the first link.
            le_dls_Pop(&list0);
        }


        //pop half the list.
        for (i = 0; i < (maxListSize / 2); i++)
        {
            le_dls_PopTail(&list1);
        }
    }

    printf("Popped all the nodes except one from the head of list0.\n");
    printf("Popped half the nodes from the tail of list1.\n");

    // Check that the list is still in tact.
    {
        idRecord_t* nodePtr;

        // For list 0.
        le_dls_Link_t* linkPtr = le_dls_Peek(&list0);
        nodePtr = CONTAINER_OF(linkPtr, idRecord_t, link);
        if (!nodePtr)
        {
           printf("nodePtr is NULL: %d", __LINE__);
           return LE_FAULT;
        }

        if (nodePtr->id != maxListSize-1)
        {
            printf("Link error: %d", __LINE__);
        }

        // Check that the number of links left is correct.
        if (le_dls_NumLinks(&list0) != 1)
        {
            printf("Wrong number of links: %d", __LINE__);
            return LE_FAULT;
        }

        // For list1.
        linkPtr = le_dls_Peek(&list1);
        i = 0;
        do
        {
            nodePtr = CONTAINER_OF(linkPtr, idRecord_t, link);
            if (!nodePtr)
            {
                printf("nodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            if (nodePtr->id != i++)
            {
                printf("Link error: %d", __LINE__);
                return LE_FAULT;
            }

            linkPtr = le_dls_PeekNext(&list1, linkPtr);
        } while(linkPtr != NULL);

        // Check that the number of links left is correct.
        if (i != maxListSize - (maxListSize / 2))
        {
            printf("Wrong number of links: %d", __LINE__);
            return LE_FAULT;
        }
    }

    printf("Checked that all nodes were properly popped from the lists.\n");


    //
    // Check for list corruption.
    //
    {
        le_dls_Link_t* linkPtr;

        if (le_dls_IsListCorrupted(&list1))
        {
            printf("List1 is corrupt but shouldn't be: %d", __LINE__);
            return LE_FAULT;
        }
        printf("Checked that List1 is not corrupt.\n");

        // Access one of the links directly.  This should corrupt the list.
        linkPtr = le_dls_PeekTail(&list1);
        LE_ASSERT(linkPtr != NULL);
        linkPtr = le_dls_PeekPrev(&list1, linkPtr);
        LE_ASSERT(linkPtr != NULL);
        linkPtr->prevPtr = linkPtr;

        if (!le_dls_IsListCorrupted(&list1))
        {
            printf("List1 is not corrupted but should be: %d", __LINE__);
            return LE_FAULT;
        }
        printf("List1 is supposed to be corrupted.  CRIT log message can be ignored.\n");
    }

    printf("Checked lists for corruption.\n");


    printf("*** Unit Test for le_doublyLinkedList module passed. ***\n");
    printf("\n");
    return LE_OK;
}


static le_result_t TestSinglyLinkLists(size_t maxListSize)
{
    // Node definition.
    typedef struct
    {
        le_sls_Link_t link;
        uint32_t id;
    }
    idRecord_t;

    int i;
    le_sls_List_t list0, list1;

    printf("\n");
    printf("*** Unit Test for le_singlyLinkedList module. ***\n");

    //
    // Multiple list creation
    //
    // Initialize the lists
    list0 = LE_SLS_LIST_INIT;
    list1 = LE_SLS_LIST_INIT;
    printf("One singly linked list was successfully created.\n");


    //
    // Attempt to query empty list
    //
    if ( (le_sls_Peek(&list0) != NULL) || (le_sls_Pop(&list0) != NULL) )
    {
        printf("Query of empty list failed: %d", __LINE__);
        return LE_FAULT;
    }
    printf("Query of empty list correct.\n");


    //
    // Node insertions
    //
    {
        idRecord_t* newNodePtr;
        le_sls_Link_t* prevLinkPtr = NULL;

        // Queue nodes to list0.
        for (i = 0; i < maxListSize; i++)
        {
            // Create the new node
            newNodePtr = (idRecord_t*)malloc(sizeof(idRecord_t));
            if (!newNodePtr)
            {
                printf("newNodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            newNodePtr->id = i;

            // Initialize the link.
            newNodePtr->link = LE_SLS_LINK_INIT;

            if (i < maxListSize/2)
            {
                // Insert the new node to the tail.
                le_sls_Queue(&list0, &(newNodePtr->link));
            }
            else
            {
                // Insert to the tail using insert after.
                le_sls_AddAfter(&list0, prevLinkPtr, &(newNodePtr->link));
            }

            prevLinkPtr = &(newNodePtr->link);
        }
        printf("%zu nodes were queued to the tail of list0.\n", maxListSize);

        // Stack nodes to list1.
        for (i = 0; i < maxListSize; i++)
        {
            // Create the new node
            newNodePtr = (idRecord_t*)malloc(sizeof(idRecord_t));
            if (!newNodePtr)
            {
                printf("newNodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            newNodePtr->id = i;

            // Initialize the link.
            newNodePtr->link = LE_SLS_LINK_INIT;

            // Insert the new node to the head.
            le_sls_Stack(&list1, &(newNodePtr->link));
        }
        printf("%zu nodes were stacked to the head of list1.\n", maxListSize);
    }

    //
    // Check that all the nodes have been added properly
    //
    {
        idRecord_t* nodePtr;
        le_sls_Link_t* link0Ptr = le_sls_Peek(&list0);
        le_sls_Link_t* link1Ptr = le_sls_Peek(&list1);

        if ( (link0Ptr == NULL) || (link1Ptr == NULL) )
        {
            printf("Link error: %d", __LINE__);
            return LE_FAULT;
        }

        i = 0;
        do
        {
            // Get the node from list 0
            nodePtr = CONTAINER_OF(link0Ptr, idRecord_t, link);
            if (!nodePtr)
            {
                printf("nodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            // Check the node.
            if ( nodePtr->id != i)
            {
                printf("Link error: %d", __LINE__);
                return LE_FAULT;
            }

            // Get the node from list 1
            nodePtr = CONTAINER_OF(link1Ptr, idRecord_t, link);
            if (!nodePtr)
            {
                printf("nodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            // Check the node.
            if ( nodePtr->id != maxListSize - i - 1)
            {
                printf("Link error: %d", __LINE__);
                return LE_FAULT;
            }

            // Move to the next node.
            link0Ptr = le_sls_PeekNext(&list0, link0Ptr);
            link1Ptr = le_sls_PeekNext(&list1, link1Ptr);
            i++;

        } while (link0Ptr != NULL);

        // Make sure everything is correct.
        if ( (i != maxListSize) || (link1Ptr != NULL) )
        {
            printf("Link error: %d", __LINE__);
            return LE_FAULT;
        }
    }

    printf("Checked that all nodes added to the head and tails are all correct.\n");


    //
    // Pop nodes.
    //
    {
        //pop half the list.
        for (i = 0; i < (maxListSize / 2); i++)
        {
            le_sls_Pop(&list0);
        }
    }

    printf("Popped half the nodes from the head of list0.\n");

    // Check that the list is still in tact.
    {
        idRecord_t* nodePtr;

        // For list 0.
        le_sls_Link_t* linkPtr = le_sls_Peek(&list0);
        i = maxListSize/2;
        do
        {
            nodePtr = CONTAINER_OF(linkPtr, idRecord_t, link);
            if (!nodePtr)
            {
                printf("nodePtr is NULL: %d", __LINE__);
                return LE_FAULT;
            }

            if (nodePtr->id != i++)
            {
                printf("Link error: %d", __LINE__);
                return LE_FAULT;
            }

            linkPtr = le_sls_PeekNext(&list0, linkPtr);
        } while (linkPtr != NULL);

        // Check that the number of links left is correct.
        if (i != maxListSize)
        {
            printf("Wrong number of links: %d", __LINE__);
            return LE_FAULT;
        }
    }
    printf("Checked that all nodes were properly popped from the lists.\n");


    //
    // Check for list corruption.
    //
    {
        le_sls_Link_t* linkPtr;

        if (le_sls_IsListCorrupted(&list0))
        {
            printf("List0 is corrupt but shouldn't be: %d", __LINE__);
            return LE_FAULT;
        }
        printf("Checked that List0 is not corrupt.\n");

        // Access one of the links directly.  This should corrupt the list.
        linkPtr = le_sls_Peek(&list0);
        linkPtr = le_sls_PeekNext(&list0, linkPtr);
        if (!linkPtr)
        {
           LE_ERROR("linkPtr is null !!!");
           return LE_FAULT;
        }

        linkPtr->nextPtr = NULL;

        if (!le_sls_IsListCorrupted(&list0))
        {
            printf("List0 is not corrupted but should be: %d", __LINE__);
            return LE_FAULT;
        }
    }

    printf("Checked lists for corruption.\n");

    printf("*** Unit Test for le_singlyLinkedList module passed. ***\n");
    printf("\n");
    return LE_OK;
}
