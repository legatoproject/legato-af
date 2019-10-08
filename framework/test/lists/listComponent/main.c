/**
 * This module is for unit testing the list modules in the legato
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

#include "legato.h"

#define LIST_SIZE           101
#define MAX_LIST_SIZE       1024
#define REMOVE_THRESHOLD    RAND_MAX / 2

static void TestDoublyLinkLists(size_t maxListSize);
static void TestSinglyLinkLists(size_t maxListSize);


COMPONENT_INIT
{
    uint32_t maxListSize = 0;

    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    // Get the max size list
    if (le_arg_NumArgs() >= 1)
    {
        const char* maxListSizePtr = le_arg_GetArg(1);
        LE_TEST_ASSERT(maxListSizePtr != NULL, "maxListSizePtr is NULL");
        maxListSize = atoi(maxListSizePtr);
        LE_TEST_ASSERT(maxListSize < MAX_LIST_SIZE, "List size too large for test");
    }

    if (maxListSize <= 0)
    {
        maxListSize = LIST_SIZE;
    }

    LE_TEST_INFO("Setting list size to %d.", LIST_SIZE);

    TestDoublyLinkLists(maxListSize);

    TestSinglyLinkLists(maxListSize);

    LE_TEST_EXIT;
}

// Node definition for doubly-linked list tests
typedef struct
{
    le_dls_Link_t link;
    uint32_t id;
} dlsIdRecord_t;

bool RecordGreaterThan(le_dls_Link_t* aLinkPtr, le_dls_Link_t* bLinkPtr)
{
    dlsIdRecord_t *aPtr = CONTAINER_OF(aLinkPtr, dlsIdRecord_t, link);
    dlsIdRecord_t *bPtr = CONTAINER_OF(bLinkPtr, dlsIdRecord_t, link);

    return (aPtr->id > bPtr->id);
}

static void TestDoublyLinkLists(size_t maxListSize)
{
    size_t i;
    le_dls_List_t list0, list1;
    le_dls_Link_t* removedLinksPtr0[maxListSize];
    le_dls_Link_t* removedLinksPtr1[maxListSize];

    memset(removedLinksPtr0, 0, sizeof(removedLinksPtr0));
    memset(removedLinksPtr1, 0, sizeof(removedLinksPtr1));

    LE_TEST_INFO("Unit Test for le_doublyLinkedList module.");

    //
    // Multiple list creation
    //
    // Initialize the lists
    list0 = LE_DLS_LIST_INIT;
    list1 = LE_DLS_LIST_INIT;
    LE_TEST_OK(!le_dls_IsListCorrupted(&list0) && !le_dls_IsListCorrupted(&list1),
               "Created two doubly linked lists");

    //
    // Attempt to query empty list
    //
    LE_TEST_OK(le_dls_Peek(&list0) == NULL, "Peek on empty list0 returns NULL");
    LE_TEST_OK(le_dls_PeekTail(&list0) == NULL, "PeekTail on empty list0 returns NULL");
    LE_TEST_OK(le_dls_Peek(&list1) == NULL, "Peek on empty list1 returns NULL");
    LE_TEST_OK(le_dls_PeekTail(&list1) == NULL, "PeekTail on empty list1 returns NULL");

    //
    // Node insertions
    //
    {
        dlsIdRecord_t* newNodePtr;

        // Insert to the tail
        for (i = 0; i < maxListSize; i++)
        {
            // Create the new node
            newNodePtr = (dlsIdRecord_t*)malloc(sizeof(dlsIdRecord_t));
            LE_TEST_ASSERT(newNodePtr, "Allocated list0 node %" PRIuS, i);

            newNodePtr->id = i;

            // Initialize the link.
            newNodePtr->link = LE_DLS_LINK_INIT;

            // Insert the new node to the tail.
            le_dls_Queue(&list0, &(newNodePtr->link));
        }
        LE_TEST_INFO("%" PRIuS " nodes were added to the tail of list0.", maxListSize);

        // Insert to the head
        for (i = 0; i < maxListSize; i++)
        {
            // Create the new node
            newNodePtr = (dlsIdRecord_t*)malloc(sizeof(dlsIdRecord_t));
            LE_TEST_ASSERT(newNodePtr, "Allocated list1 node %" PRIuS, i);

            newNodePtr->id = i;

            // Initialize the link.
            newNodePtr->link = LE_DLS_LINK_INIT;

            // Insert the new node to the tail.
            le_dls_Stack(&list1, &(newNodePtr->link));
        }
        LE_TEST_INFO("%" PRIuS " nodes were added to the head of list1.", maxListSize);
    }

    //
    // Check that all the nodes have been added properly
    //
    {
        dlsIdRecord_t* nodePtr;
        le_dls_Link_t* link0Ptr = le_dls_Peek(&list0);
        le_dls_Link_t* link1Ptr = le_dls_PeekTail(&list1);

        LE_TEST_ASSERT(link0Ptr, "Get head of list0");
        LE_TEST_ASSERT(link1Ptr, "Get tail of list1");

        i = 0;
        do
        {
            // Get the node from list 0
            nodePtr = CONTAINER_OF(link0Ptr, dlsIdRecord_t, link);
            LE_TEST_OK(nodePtr, "nodePtr %" PRIuS " of list0 is not NULL", i);
            LE_TEST_OK(nodePtr && nodePtr->id == i,
                "Incorrect node pointer in node %" PRIuS " of list0", i);

            // Get the node from list 1
            nodePtr = CONTAINER_OF(link1Ptr, dlsIdRecord_t, link);
            LE_TEST_OK(nodePtr, "nodePtr %" PRIuS " of list1 is not NULL", i);
            LE_TEST_OK(nodePtr && nodePtr->id == i,
                "Incorrect node pointer in node %" PRIuS " of list1", i);

            // Move to the next node.
            link0Ptr = le_dls_PeekNext(&list0, link0Ptr);
            link1Ptr = le_dls_PeekPrev(&list1, link1Ptr);
            i++;

        } while (link0Ptr != NULL || link1Ptr != NULL);

        // Make there's the correct number of nodes in the list
        LE_TEST_OK(i == maxListSize, "%" PRIuS " nodes in the list", maxListSize);
    }

    LE_TEST_INFO("Checked that all nodes added to the head and tails are all correct.");


    //
    // Remove random nodes
    //

    //seed the random number generator with the clock
    srand(time(NULL));

    {
        // Start at the end of the lists and randomly remove links.
        le_dls_Link_t* linkToRemovePtr;
        le_dls_Link_t* link0Ptr = le_dls_PeekTail(&list0);
        le_dls_Link_t* link1Ptr = le_dls_Peek(&list1);

        size_t r0 = 0;
        size_t r1 = 0;
        while (link0Ptr && r0 < maxListSize)
        {
            // For list 0
            if ( (rand() < REMOVE_THRESHOLD) && (r0 < maxListSize) )
            {
                // Mark this node for removal.
                linkToRemovePtr = link0Ptr;
                LE_TEST_ASSERT(linkToRemovePtr != NULL, "linkToRemovePtr is NULL");

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
        }


        while (link1Ptr && r1 < maxListSize)
        {
            // For list 1
            if ( (rand() < REMOVE_THRESHOLD) )
            {
                // Mark this node for removal.
                linkToRemovePtr = link1Ptr;
                LE_TEST_ASSERT(linkToRemovePtr != NULL, "linkToRemovePtr is NULL");

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
        };

        LE_TEST_INFO("Randomly removed %" PRIuS " nodes from list0.", r0);
        LE_TEST_INFO("Randomly removed %" PRIuS " nodes from list1.", r1);
    }


    //
    // Check that the proper nodes were removed
    //
    {
        int numNodesRemoved = 0;

        // For list 0.
        // Check that the nodes in the removed nodes are indeed not in the list.
        for (i = 0; i < maxListSize; i++)
        {
            if (removedLinksPtr0[i] == NULL)
            {
                break;
            }

            LE_TEST_OK(!le_dls_IsInList(&list0, removedLinksPtr0[i]),
                       "Check removed node %" PRIuS " is not in list0", i);

            numNodesRemoved++;
        }

        // Compare the list count.
        LE_TEST_OK(numNodesRemoved + le_dls_NumLinks(&list0) == maxListSize,
                   "Number of nodes removed correct");

        // For list 1.
        // Check that the nodes in the removed nodes are indeed not in the list.
        numNodesRemoved = 0;
        for (i = 0; i < maxListSize; i++)
        {
            if (removedLinksPtr1[i] == NULL)
            {
                break;
            }

            LE_TEST_OK(!le_dls_IsInList(&list1, removedLinksPtr1[i]),
                       "Check removed node %" PRIuS " is not in list1", i);

            numNodesRemoved++;
        }

        // Compare the list count.
        LE_TEST_OK(numNodesRemoved + le_dls_NumLinks(&list1) == maxListSize,
                   "Number of nodes removed correct");
    }

    LE_TEST_INFO("Checked that nodes were removed correctly.");


    //
    // Add the randomly removed nodes back in.
    //
    {
        dlsIdRecord_t *nodePtr, *removedNodePtr;
        le_dls_Link_t* linkPtr;

        // For list 0.
        for (i = 0; i < maxListSize; i++)
        {
            if (removedLinksPtr0[i] == NULL)
            {
                break;
            }

            removedNodePtr = CONTAINER_OF(removedLinksPtr0[i], dlsIdRecord_t, link);

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
                    nodePtr = CONTAINER_OF(linkPtr, dlsIdRecord_t, link);
                    LE_TEST_ASSERT(nodePtr != NULL, "nodePtr is NULL");

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
        for (i = 0; i < maxListSize; i++)
        {
            if (removedLinksPtr1[i] == NULL)
            {
                break;
            }

            removedNodePtr = CONTAINER_OF(removedLinksPtr1[i], dlsIdRecord_t, link);

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
                    nodePtr = CONTAINER_OF(linkPtr, dlsIdRecord_t, link);
                    LE_TEST_ASSERT(nodePtr != NULL, "nodePtr is NULL");

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

    LE_TEST_INFO("Added all randomly removed nodes back in.\n");

    //Check that the list is correct.
    {
        dlsIdRecord_t* nodePtr;
        le_dls_Link_t* link0Ptr = le_dls_Peek(&list0);
        le_dls_Link_t* link1Ptr = le_dls_PeekTail(&list1);

        LE_TEST_ASSERT(link0Ptr, "Get head of list0");
        LE_TEST_ASSERT(link1Ptr, "Get head of list1");

        for (i = 0; link0Ptr != NULL; ++i, link0Ptr = le_dls_PeekNext(&list0, link0Ptr))
        {
            // Get the node from list 0
            nodePtr = CONTAINER_OF(link0Ptr, dlsIdRecord_t, link);
            LE_TEST_ASSERT(nodePtr, "Get node from list0");
            LE_TEST_OK(nodePtr->id == i, "Node %" PRIuS " from list0 is in the correct order", i);
        }

        LE_TEST_OK(i == maxListSize, "List0 has expected size %" PRIuS, maxListSize);

        for (i = 0; link1Ptr != NULL; ++i, link1Ptr = le_dls_PeekPrev(&list1, link1Ptr))
        {
            // Get the node from list 1
            nodePtr = CONTAINER_OF(link1Ptr, dlsIdRecord_t, link);
            LE_TEST_ASSERT(nodePtr, "Get node from list1");
            LE_TEST_OK(nodePtr->id == i, "Node %" PRIuS " from list1 is in the correct order", i);
        }

        LE_TEST_OK(i == maxListSize, "List1 has expected size %" PRIuS, maxListSize);
    }

    LE_TEST_INFO("Checked that all nodes are now added back in in the correct order.");


    //
    // Swap nodes.
    //
    {
        //Swap all the nodes in the list so the list is in reverse order.
        le_dls_Link_t* linkPtr, *tmpLinkPtr;
        le_dls_Link_t* otherlinkPtr;
        dlsIdRecord_t* nodePtr, *otherNodePtr;

        // For list 0.
        linkPtr = le_dls_Peek(&list0);
        otherlinkPtr = le_dls_PeekTail(&list0);
        for (i = 0; i < (le_dls_NumLinks(&list0) / 2); i++)
        {
            nodePtr = CONTAINER_OF(linkPtr, dlsIdRecord_t, link);
            LE_TEST_ASSERT(nodePtr != NULL, "nodePtr is NULL");

            otherNodePtr = CONTAINER_OF(otherlinkPtr, dlsIdRecord_t, link);
            LE_TEST_ASSERT(otherNodePtr != NULL, "otherNodePtr is NULL");

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
            nodePtr = CONTAINER_OF(linkPtr, dlsIdRecord_t, link);
            LE_TEST_ASSERT(nodePtr != NULL, "nodePtr is NULL");

            otherNodePtr = CONTAINER_OF(otherlinkPtr, dlsIdRecord_t, link);
            LE_TEST_ASSERT(otherNodePtr != NULL, "nodePtr is NULL");

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

    LE_TEST_INFO("Reversed the order of both lists using swap.");

    //Check that the list is correct.
    {
        dlsIdRecord_t* nodePtr;
        le_dls_Link_t* link0Ptr = le_dls_PeekTail(&list0);
        le_dls_Link_t* link1Ptr = le_dls_Peek(&list1);

        LE_TEST_ASSERT(link0Ptr, "Get head of list0");
        LE_TEST_ASSERT(link1Ptr, "Get head of list1");

        for (i = 0; link0Ptr != NULL; ++i, link0Ptr = le_dls_PeekPrev(&list0, link0Ptr))
        {
            // Get the node from list 0
            nodePtr = CONTAINER_OF(link0Ptr, dlsIdRecord_t, link);
            LE_TEST_ASSERT(link0Ptr, "Find node of link0");

            // Check the node.
            LE_TEST_OK(nodePtr->id == i, "Node %" PRIuS " of list0 is in the correct spot", i);
        }

        LE_TEST_OK(i == maxListSize, "List0 has expected size %"PRIuS, maxListSize);

        for (i = 0; link1Ptr != NULL; ++i, link1Ptr = le_dls_PeekNext(&list1, link1Ptr))
        {
            // Get the node from list 1
            nodePtr = CONTAINER_OF(link1Ptr, dlsIdRecord_t, link);
            LE_TEST_ASSERT(nodePtr != NULL, "nodePtr is NULL");

            // Check the node.
            LE_TEST_OK(nodePtr->id == i, "Node %" PRIuS " of list1 is in the correct spot", i);
        }

        LE_TEST_OK(i == maxListSize, "List1 has expected size %" PRIuS, maxListSize);
    }

    LE_TEST_INFO("Checked that all nodes are now correctly in the reverse order.");

    {
        // Now randomize list0 by selecting items randomly and moving to the back of the list.
        for (i = maxListSize - 1; i >= 1; --i)
        {
            int j = rand() % (i+1);
            le_dls_Link_t* itemToMove = le_dls_Peek(&list0);
            while (j--)
            {
                LE_TEST_ASSERT(itemToMove != NULL, "itemToMove is NULL");
                itemToMove = le_dls_PeekNext(&list0, itemToMove);
            }

            LE_TEST_ASSERT(itemToMove != NULL, "itemToMove is NULL");
            le_dls_Remove(&list0, itemToMove);
            le_dls_Queue(&list0, itemToMove);
        }

        LE_TEST_INFO("Sorting shuffled list");

        // Sort the list descending
        le_dls_Sort(&list0, RecordGreaterThan);

        // And check it's in the correct order
        i = 0;
        dlsIdRecord_t* nodePtr;
        LE_DLS_FOREACH(&list0, nodePtr, dlsIdRecord_t, link)
        {
            LE_TEST_ASSERT(nodePtr, "nodePtr is not NULL");
            LE_TEST_OK(nodePtr->id == maxListSize - i - 1, "Node %" PRIuS " is in correct spot", i);
            ++i;
        }
    }

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

    LE_TEST_INFO("Popped all the nodes except one from the head of list0.");
    LE_TEST_INFO("Popped half the nodes from the tail of list1.");

    // Check that the list is still intact.
    {
        dlsIdRecord_t* nodePtr;

        // For list 0.
        le_dls_Link_t* linkPtr = le_dls_Peek(&list0);
        nodePtr = CONTAINER_OF(linkPtr, dlsIdRecord_t, link);
        LE_TEST_ASSERT(nodePtr != NULL, "nodePtr is NULL");

        LE_TEST_OK(nodePtr->id == maxListSize-1, "Correct node at head of list0");

        // Check that the number of links left is correct.
        LE_TEST_OK(le_dls_NumLinks(&list0) == 1, "Only 1 node left in list0");

        // For list1.
        linkPtr = le_dls_Peek(&list1);
        for (i = 0; linkPtr; ++i, linkPtr = le_dls_PeekNext(&list1, linkPtr))
        {
            nodePtr = CONTAINER_OF(linkPtr, dlsIdRecord_t, link);
            LE_TEST_ASSERT(nodePtr != NULL, "nodePtr is NULL");

            LE_TEST_OK(nodePtr->id == i, "Node %" PRIuS " is correct", i);
        }

        LE_TEST_OK(i == maxListSize - (maxListSize / 2),
                   "Check for expected number of items in list1");
    }

    LE_TEST_INFO("Checked that all nodes were properly popped from the lists.");


    //
    // Check for list corruption.
    //
    {
        le_dls_Link_t* linkPtr;

        LE_TEST_OK(!le_dls_IsListCorrupted(&list1), "Check list1 is not corrupt");

        // Access one of the links directly.  This should corrupt the list.
        linkPtr = le_dls_PeekTail(&list1);
        LE_TEST_ASSERT(linkPtr != NULL, "got first item from list1");
        linkPtr = le_dls_PeekPrev(&list1, linkPtr);
        LE_TEST_ASSERT(linkPtr != NULL, "got second item from list1");
        linkPtr->prevPtr = linkPtr;
        LE_TEST_INFO("Corrupted list1");

        LE_TEST_OK(le_dls_IsListCorrupted(&list1), "Checking list1 is corrupt");
    }

    LE_TEST_INFO("Finished tests for doublyLinkedList module");
}

// Node definition for singly-linked list tests.
typedef struct
{
    le_sls_Link_t link;
    uint32_t id;
} slsIdRecord_t;

bool RecordLessThan(le_sls_Link_t* aLinkPtr, le_sls_Link_t* bLinkPtr)
{
    slsIdRecord_t *aPtr = CONTAINER_OF(aLinkPtr, slsIdRecord_t, link);
    slsIdRecord_t *bPtr = CONTAINER_OF(bLinkPtr, slsIdRecord_t, link);

    return (aPtr->id < bPtr->id);
}

static void TestSinglyLinkLists(size_t maxListSize)
{
    size_t i;
    le_sls_List_t list0, list1;

    LE_TEST_INFO("Unit Test for le_singlyLinkedList module.");

    //
    // Multiple list creation
    //
    // Initialize the lists
    list0 = LE_SLS_LIST_INIT;
    list1 = LE_SLS_LIST_INIT;
    LE_TEST_OK(!le_sls_IsListCorrupted(&list0) && !le_sls_IsListCorrupted(&list1),
               "Created two singly linked lists");

    //
    // Attempt to query empty list
    //
    LE_TEST_OK(le_sls_Peek(&list0) == NULL, "Check no head of empty list");
    LE_TEST_OK(le_sls_Pop(&list0) == NULL, "Check cannot pop from empty list");

    //
    // Node insertions
    //
    {
        slsIdRecord_t* newNodePtr;
        le_sls_Link_t* prevLinkPtr = NULL;

        // Queue nodes to list0.
        for (i = 0; i < maxListSize; i++)
        {
            // Create the new node
            newNodePtr = (slsIdRecord_t*)malloc(sizeof(slsIdRecord_t));
            LE_TEST_ASSERT(newNodePtr, "allocate node %" PRIuS " for list0", i);

            newNodePtr->id = i;

            // Initialize the link.
            newNodePtr->link = LE_SLS_LINK_INIT;

            if (!prevLinkPtr || (i < maxListSize/2))
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
        LE_TEST_INFO("%" PRIuS " nodes were queued to the tail of list0.\n", maxListSize);

        // Stack nodes to list1.
        for (i = 0; i < maxListSize; i++)
        {
            // Create the new node
            newNodePtr = (slsIdRecord_t*)malloc(sizeof(slsIdRecord_t));
            LE_TEST_ASSERT(newNodePtr, "allocated node %" PRIuS " for list1", i);

            newNodePtr->id = i;

            // Initialize the link.
            newNodePtr->link = LE_SLS_LINK_INIT;

            // Insert the new node to the head.
            le_sls_Stack(&list1, &(newNodePtr->link));
        }
        LE_TEST_INFO("%" PRIuS " nodes were stacked to the head of list1.\n", maxListSize);
    }

    //
    // Check that all the nodes have been added properly
    //
    {
        slsIdRecord_t* nodePtr;

        i = 0;
        LE_SLS_FOREACH(&list0, nodePtr, slsIdRecord_t, link)
        {
            LE_TEST_ASSERT(nodePtr, "nodePtr is not NULL");

            LE_TEST_OK(nodePtr->id == i, "node %" PRIuS " is in correct spot", i);
            ++i;
        }

        LE_TEST_OK(i == maxListSize, "list0 has the correct size");

        i = 0;
        LE_SLS_FOREACH(&list1, nodePtr, slsIdRecord_t, link)
        {
            LE_TEST_ASSERT(nodePtr, "nodePtr is not NULL");

            LE_TEST_OK(nodePtr->id == maxListSize - i - 1, "Node %" PRIuS " is in correct spot", i);
            ++i;
        }

        LE_TEST_OK(i == maxListSize, "list1 has the correct size");
    }

    LE_TEST_INFO("Checked that all nodes added to the head and tails are all correct.");

    {
        slsIdRecord_t* nodePtr;

        // Now randomize list0 by selecting items randomly and moving to the back of the list.
        for (i = maxListSize - 1; i >= 1; --i)
        {
            int j = rand() % (i+1);
            le_sls_Link_t *itemToMove = le_sls_Peek(&list0), *prevItem = NULL;
            while (j--)
            {
                prevItem = itemToMove;
                LE_TEST_ASSERT(itemToMove != NULL, "itemToMove is NULL");
                itemToMove = le_sls_PeekNext(&list0, itemToMove);
            }

            if (prevItem)
            {
                le_sls_RemoveAfter(&list0, prevItem);
            }
            else
            {
                le_sls_Pop(&list0);
            }

            LE_TEST_ASSERT(itemToMove != NULL, "itemToMove is NULL");

            le_sls_Queue(&list0, itemToMove);
        }

        LE_TEST_INFO("Sorting shuffled list");

        le_sls_Sort(&list0, RecordLessThan);

        i = 0;
        LE_SLS_FOREACH(&list0, nodePtr, slsIdRecord_t, link)
        {
            LE_TEST_ASSERT(nodePtr, "got node ptr");
            LE_TEST_OK(nodePtr->id == i,
                       "node %" PRIuS " (value %"PRIu32") in correct location", i, nodePtr->id);
            ++i;
        }
    }

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

    LE_TEST_INFO("Popped half the nodes from the head of list0.\n");

    // Check that the list is still in tact.
    {
        slsIdRecord_t* nodePtr;

        i = maxListSize/2;
        LE_SLS_FOREACH(&list0, nodePtr, slsIdRecord_t, link)
        {
            LE_TEST_ASSERT(nodePtr, "nodePtr is not NULL");

            LE_TEST_OK(nodePtr->id == i, "Node %" PRIuS " is in correct location", i);
            i++;
        }

        // Check that the number of links left is correct.
        LE_TEST_OK(i == maxListSize, "list0 has correct size after popping half of list");
    }

    //
    // Check for list corruption.
    //
    {
        le_sls_Link_t* linkPtr;

        LE_TEST_OK(!le_sls_IsListCorrupted(&list0), "list0 is not corrupted");

        // Access one of the links directly.  This should corrupt the list.
        linkPtr = le_sls_Peek(&list0);
        linkPtr = le_sls_PeekNext(&list0, linkPtr);
        LE_TEST_ASSERT(linkPtr != NULL, "linkPtr is NULL");

        linkPtr->nextPtr = NULL;
        LE_TEST_INFO("Corrupted list 0");

        LE_TEST_OK(le_sls_IsListCorrupted(&list0), "list0 is corrupted");
    }

    LE_TEST_INFO("Unit test for singlyLinkedList module finished.");
}
