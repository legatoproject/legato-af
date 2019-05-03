 /**
  * This module is for unit testing Red-Black Tree module in the legato
  * runtime library (liblegato.so).
  *
  * The following is a list of the test cases:
  *
  * - Tree creation
  * - Adding nodes
  * - Finding a node
  * - Traversing the tree
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#include "legato.h"

static int compare(const void* a, const void* b)
{
    return strcmp((char*) a, (char*) b);
}

typedef struct
{
    char name[16];
}
item_key_t;

typedef struct
{
    le_rbtree_Node_t link;
    item_key_t key;
    char name[16];
    int value;
}
item_t;

void RbtreeScaleTest
(
    void
)
{
#define MAX_RBTREE_SIZE 100000
    le_rbtree_Tree_t tree;
    le_rbtree_InitTree(&tree, compare);

    item_t item[MAX_RBTREE_SIZE];
    int i;
    le_rbtree_Node_t* linkPtr = NULL;
    item_t *itemPtr = NULL;

    LE_TEST_INFO("***** Starting Red-Black Tree test.");
    LE_TEST_INFO("writing %d even items", MAX_RBTREE_SIZE/2);
    for (i = 0; i < MAX_RBTREE_SIZE; i+=2)
    {
        snprintf(item[i].key.name, sizeof(item[i].key.name), "item-%06d", i);
        item[i].value = i;
        le_rbtree_InitNode(&item[i].link, &item[i].key);
        le_rbtree_Node_t* result = le_rbtree_Insert(&tree, &item[i].link);
        if (NULL == result)
        {
            LE_TEST_FATAL("Error: Insert returned NULL");
        }
    }

    LE_TEST_INFO("writing %d duplicated items", MAX_RBTREE_SIZE/2);
    for (i = 0; i < MAX_RBTREE_SIZE; i+=2)
    {
        le_rbtree_Node_t* result = le_rbtree_Insert(&tree, &item[i].link);
        if (NULL != result)
        {
            LE_TEST_FATAL("Error: Inserting duplicated item returned non-NULL");
        }
    }

    LE_TEST_INFO("checking tree size");
    if (MAX_RBTREE_SIZE/2 != le_rbtree_Size(&tree))
    {
        LE_TEST_FATAL("Error: RBtree Size returned incorrect value %"PRIuS, le_rbtree_Size(&tree));
    }

    LE_TEST_INFO("writing %d odd items", MAX_RBTREE_SIZE/2);
    for (i = 1; i < MAX_RBTREE_SIZE; i+=2)
    {
        snprintf(item[i].key.name, sizeof(item[i].key.name), "item-%06d", i);
        item[i].value = i;
        le_rbtree_InitNode(&item[i].link, &item[i].key);
        le_rbtree_Node_t* result = le_rbtree_Insert(&tree, &item[i].link);
        if (NULL == result)
        {
            LE_TEST_FATAL("Error: Insert returned NULL");
        }
    }

    LE_TEST_INFO("reading %d items", MAX_RBTREE_SIZE);
    for (i = 0; i < MAX_RBTREE_SIZE; i++)
    {
        item_key_t key;
        snprintf(key.name, sizeof(key.name), "item-%06d", i);
        linkPtr = le_rbtree_Find(&tree, &key);
        if (NULL == linkPtr)
        {
            LE_TEST_FATAL("item %d not found", i);
        }
        else
        {
            itemPtr = CONTAINER_OF(linkPtr, item_t, link);
            if (i != itemPtr->value)
            {
                LE_TEST_FATAL("Error: item %d has wrong value %d", i, itemPtr->value);
            }
        }
    }

    LE_TEST_INFO("selectively deleting (by key) even items 0, 2, 4,...");
    for (i = 0; i < MAX_RBTREE_SIZE; i += 2)
    {
        item_key_t key;
        snprintf(key.name, sizeof(key.name), "item-%06d", i);
        if (NULL == le_rbtree_RemoveByKey(&tree, &key))
        {
            LE_TEST_FATAL("Error: deleting item %d returned NULL", i);
        }
    }

    LE_TEST_INFO("verify that deleted items can't be found");
    for (i = 0; i < MAX_RBTREE_SIZE; i += 2)
    {
        item_key_t key;
        snprintf(key.name, sizeof(key.name), "item-%06d", i);
        linkPtr = le_rbtree_Find(&tree, &key);
        if (NULL != linkPtr)
        {
            LE_TEST_FATAL("Error: found previously deleted item %d", i);
        }
    }

    LE_TEST_INFO("Verify that remaining odd items are still there");
    for (i = 1; i < MAX_RBTREE_SIZE; i += 2)
    {
        item_key_t key;
        snprintf(key.name, sizeof(key.name), "item-%06d", i);
        linkPtr = le_rbtree_Find(&tree, &key);
        if (NULL == linkPtr)
        {
            LE_TEST_FATAL("Error: item %d not found", i);
        }
        itemPtr = CONTAINER_OF(linkPtr, item_t, link);
        if (itemPtr->value != i)
        {
            LE_TEST_FATAL("Error: item %d has wrong value %d", i, itemPtr->value);
        }
    }

    LE_TEST_INFO("walking the tree forward");
    int value = 0;
    int count = 0;
    for (linkPtr = le_rbtree_GetFirst(&tree);
         linkPtr != NULL;
         linkPtr = le_rbtree_GetNext(&tree, linkPtr))
    {
        itemPtr = CONTAINER_OF(linkPtr, item_t, link);
        if (value >= itemPtr->value)
        {
            LE_TEST_FATAL("Error: incorrect order walking forward");
        }
        value = itemPtr->value;
        count++;
    }
    LE_TEST_OK(count == MAX_RBTREE_SIZE/2, "forward walk: item count correct");

    LE_TEST_INFO("walking the tree backward");
    value = MAX_RBTREE_SIZE;
    count = 0;
    for (linkPtr = le_rbtree_GetLast(&tree);
         linkPtr != NULL;
         linkPtr = le_rbtree_GetPrev(&tree, linkPtr))
    {
        itemPtr = CONTAINER_OF(linkPtr, item_t, link);
        if (value <= itemPtr->value)
        {
            LE_TEST_FATAL("Error: incorrect order walking backward");
        }
        value = itemPtr->value;
        count++;
    }
    LE_TEST_OK(count == MAX_RBTREE_SIZE/2, "backward walk: item count correct");

    LE_TEST_INFO("trying to delete (by key) again already deleted items 0, 2, 4,...");
    for (i = 0; i < MAX_RBTREE_SIZE; i += 2)
    {
        item_key_t key;
        snprintf(key.name, sizeof(key.name), "item-%06d", i);
        if (NULL != le_rbtree_RemoveByKey(&tree, &key))
        {
            LE_TEST_FATAL("Error: deleting already deleted item %d returned non-NULL", i);
        }
    }
    LE_TEST_INFO("trying to delete (by link) again already deleted items 0, 2, 4,...");
    for (i = 0; i < MAX_RBTREE_SIZE; i += 2)
    {
        if (NULL != le_rbtree_Remove(&tree, &item[i].link))
        {
            LE_TEST_FATAL("Error: deleting already deleted item %d returned non-NULL", i);
        }
    }
    LE_TEST_INFO("deleting (by link) the remaining items 1, 3, 5,...");
    for (i = 1; i < MAX_RBTREE_SIZE; i += 2)
    {
        if (NULL == le_rbtree_Remove(&tree, &item[i].link))
        {
            LE_TEST_FATAL("Error: deleting item %d returned NULL", i);
        }
    }

    LE_TEST_INFO("Checking whether the tree is empty");
    if (NULL != le_rbtree_GetFirst(&tree))
    {
        LE_TEST_FATAL("Error: GetFirst returned non-NULL on the empty tree");
    }
    if (NULL != le_rbtree_GetLast(&tree))
    {
        LE_TEST_FATAL("Error: GetLast returned non-NULL on the empty tree");
    }
    if (!le_rbtree_IsEmpty(&tree))
    {
        LE_TEST_FATAL("Error: IsEmpty returned false");
    }
    if (0 != le_rbtree_Size(&tree))
    {
        LE_TEST_FATAL("Error: Size returned non-zero");
    }

    LE_TEST_INFO("***** Red-Black Tree test done.");
}

COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    RbtreeScaleTest();

    LE_TEST_EXIT;
}
