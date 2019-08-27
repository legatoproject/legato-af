/** @file rbtree.c
 *
 * Implementation of the Red-Black Tree data structure.
 *
 * Portions of this code are adopted from the PostgreSQL rbtree.c implementation.
 * Original Copyright: https://github.com/postgres/postgres/blob/master/COPYRIGHT
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 *
 * Portions Copyright (c) 1994, The Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without a written agreement
 * is hereby granted, provided that the above copyright notice and this
 * paragraph and the following two paragraphs appear in all copies.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
 * DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATIONS TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

/**
 * Pointer to the NULL node
 */
#define LE_RBTREE_NULL (&NullNode)

/**
 * NULL node, statically allocated
 */
static le_rbtree_Node_t NullNode =
{
    NULL,           /* Key.  */
    LE_RBTREE_NULL, /* Parent.  */
    LE_RBTREE_NULL, /* Left.  */
    LE_RBTREE_NULL, /* Right.  */
    LE_RBTREE_BLACK /* Color.  */
};


/**
 * Rotate node x to left.
 *
 * x's right child takes its place in the tree, and x becomes the left
 * child of that node.
 */
static void RotateLeft
(
    le_rbtree_Tree_t *rbt,
    le_rbtree_Node_t *x
)
{
    le_rbtree_Node_t *y = x->right;

    /* establish x->right link */
    x->right = y->left;
    if (y->left != LE_RBTREE_NULL)
    {
        y->left->parent = x;
    }
    /* establish y->parent link */
    if (y != LE_RBTREE_NULL)
    {
        y->parent = x->parent;
    }
    if (x->parent != LE_RBTREE_NULL)
    {
        if (x == x->parent->left)
        {
            x->parent->left = y;
        }
        else
        {
            x->parent->right = y;
        }
    }
    else
    {
        rbt->root = y;
    }
    /* link x and y */
    y->left = x;
    if (x != LE_RBTREE_NULL)
    {
        x->parent = y;
    }
}


/**
 * Rotate node x to right.
 *
 * x's left right child takes its place in the tree, and x becomes the right
 * child of that node.
 */
static void RotateRight
(
    le_rbtree_Tree_t *rbt,
    le_rbtree_Node_t *x
)
{
    le_rbtree_Node_t *y = x->left;

    /* establish x->left link */
    x->left = y->right;
    if (y->right != LE_RBTREE_NULL)
    {
        y->right->parent = x;
    }
    /* establish y->parent link */
    if (y != LE_RBTREE_NULL)
    {
        y->parent = x->parent;
    }
    if (x->parent != LE_RBTREE_NULL)
    {
        if (x == x->parent->right)
        {
            x->parent->right = y;
        }
        else
        {
            x->parent->left = y;
        }
    }
    else
    {
        rbt->root = y;
    }
    /* link x and y */
    y->right = x;
    if (x != LE_RBTREE_NULL)
    {
        x->parent = y;
    }
}


/**
 * Maintain Red-Black tree balance after inserting node x.
 */
static void InsertFixup
(
    le_rbtree_Tree_t *rbt,
    le_rbtree_Node_t *x
)
{
   /*
    * x is always a red node. Initially, it is the newly inserted node. Each
    * iteration of this loop moves it higher up in the tree.
    */
    while (x != rbt->root && x->parent->color == LE_RBTREE_RED)
    {
        if (x->parent == x->parent->parent->left)
        {
            le_rbtree_Node_t *y = x->parent->parent->right;

            if (y->color == LE_RBTREE_RED)
            {
                /* uncle is LE_RBTREE_RED */
                x->parent->color = LE_RBTREE_BLACK;
                y->color = LE_RBTREE_BLACK;
                x->parent->parent->color = LE_RBTREE_RED;

                x = x->parent->parent;
            }
            else
            {
                /* uncle is LE_RBTREE_BLACK */
                if (x == x->parent->right)
                {
                    /* make x a left child */
                    x = x->parent;
                    RotateLeft(rbt, x);
                }

                /* recolor and rotate */
                x->parent->color = LE_RBTREE_BLACK;
                x->parent->parent->color = LE_RBTREE_RED;
                RotateRight(rbt, x->parent->parent);
            }
        }
        else
        {
            /* mirror image of above code */
            le_rbtree_Node_t *y = x->parent->parent->left;

            if (y->color == LE_RBTREE_RED)
            {
                /* uncle is LE_RBTREE_RED */
                x->parent->color = LE_RBTREE_BLACK;
                y->color = LE_RBTREE_BLACK;
                x->parent->parent->color = LE_RBTREE_RED;

                x = x->parent->parent;
            }
            else
            {
                /* uncle is LE_RBTREE_BLACK */
                if (x == x->parent->left)
                {
                    x = x->parent;
                    RotateRight(rbt, x);
                }
                x->parent->color = LE_RBTREE_BLACK;
                x->parent->parent->color = LE_RBTREE_RED;

                RotateLeft(rbt, x->parent->parent);
            }
        }
    }
    /*
     * The root may already have been black; if not, the black-height of every
     * node in the tree increases by one.
     */
    rbt->root->color = LE_RBTREE_BLACK;
}


/**
 * Maintain Red-Black tree balance after deleting a black node.
 */
static void DeleteFixup
(
    le_rbtree_Tree_t *rbt,
    le_rbtree_Node_t *x
)
{
    /*
     * x is always a black node.  Initially, it is the former child of the
     * deleted node.  Each iteration of this loop moves it higher up in the
     * tree.
     */
    while (x != rbt->root && x->color == LE_RBTREE_BLACK)
    {
        /*
         * Left and right cases are symmetric.  Any nodes that are children of
         * x have a black-height one less than the remainder of the nodes in
         * the tree.  We rotate and recolor nodes to move the problem up the
         * tree: at some stage we'll either fix the problem, or reach the root
         * (where the black-height is allowed to decrease).
         */
        if (x == x->parent->left)
        {
            le_rbtree_Node_t    *w = x->parent->right;

            if (w->color == LE_RBTREE_RED)
            {
                w->color = LE_RBTREE_BLACK;
                x->parent->color = LE_RBTREE_RED;

                RotateLeft(rbt, x->parent);
                w = x->parent->right;
            }

            if (w->left->color == LE_RBTREE_BLACK && w->right->color == LE_RBTREE_BLACK)
            {
                w->color = LE_RBTREE_RED;

                x = x->parent;
            }
            else
            {
                if (w->right->color == LE_RBTREE_BLACK)
                {
                    w->left->color = LE_RBTREE_BLACK;
                    w->color = LE_RBTREE_RED;

                    RotateRight(rbt, w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = LE_RBTREE_BLACK;
                w->right->color = LE_RBTREE_BLACK;

                RotateLeft(rbt, x->parent);
                x = rbt->root;    /* Arrange for loop to terminate. */
            }
        }
        else
        {
            le_rbtree_Node_t *w = x->parent->left;

            if (w->color == LE_RBTREE_RED)
            {
                w->color = LE_RBTREE_BLACK;
                x->parent->color = LE_RBTREE_RED;

                RotateRight(rbt, x->parent);
                w = x->parent->left;
            }

            if (w->right->color == LE_RBTREE_BLACK && w->left->color == LE_RBTREE_BLACK)
            {
                w->color = LE_RBTREE_RED;

                x = x->parent;
            }
            else
            {
                if (w->left->color == LE_RBTREE_BLACK)
                {
                    w->right->color = LE_RBTREE_BLACK;
                    w->color = LE_RBTREE_RED;

                    RotateLeft(rbt, w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = LE_RBTREE_BLACK;
                w->left->color = LE_RBTREE_BLACK;

                RotateRight(rbt, x->parent);
                x = rbt->root;    /* Arrange for loop to terminate. */
            }
        }
    }
    x->color = LE_RBTREE_BLACK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Red-Black Tree.
 */
//--------------------------------------------------------------------------------------------------
void le_rbtree_InitTree
(
    le_rbtree_Tree_t *tree,         ///< [IN] Pointer to the tree object
    le_rbtree_CompareFunc_t compFn  ///< [IN] Pointer to the comparator function
)
{
    tree->root = LE_RBTREE_NULL;
    tree->size = 0;
    tree->compFn = compFn;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Node Link.
 */
//--------------------------------------------------------------------------------------------------
void le_rbtree_InitNode
(
    le_rbtree_Node_t *node,      ///< [INOUT] Node link pointer.
    void *key                    ///< [IN] Key pointer.
)
{
    *node = *LE_RBTREE_NULL;
    node->key = key;
    node->color = LE_RBTREE_NO_COLOR;
}

//--------------------------------------------------------------------------------------------------
/**
 * Tests if the Tree is empty.
 *
 * @return  Returns true if empty, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool le_rbtree_IsEmpty
(
    const le_rbtree_Tree_t *rbt     ///< [IN] Tree containing currentLinkPtr.
)
{
    return (rbt->size == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Calculates size of the Tree.
 *
 * @return  The number of elementskeys in the Tree.
 */
//--------------------------------------------------------------------------------------------------
size_t le_rbtree_Size
(
    const le_rbtree_Tree_t *rbt     ///< [IN] Tree containing currentLinkPtr.
)
{
    return rbt->size;
}

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
    const le_rbtree_Tree_t *rbt    ///< [IN] Tree to get the object from.
)
{
    le_rbtree_Node_t *node = rbt->root;

    while (node->left != LE_RBTREE_NULL)
    {
        node = node->left;
    }
    return node != LE_RBTREE_NULL ? node : NULL;
}

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
    const le_rbtree_Tree_t *rbt    ///< [IN] Tree to get the object from.
)
{
    le_rbtree_Node_t *node = rbt->root;

    while (node->right != LE_RBTREE_NULL)
    {
        node = node->right;
    }
    return node != LE_RBTREE_NULL ? node : NULL;
}

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
    const le_rbtree_Tree_t *rbt,    ///< [IN] Tree to get the object from.
    le_rbtree_Node_t *x             ///< [IN] Current node pointer.
)
{
    LE_UNUSED(rbt);

    if (x->right != LE_RBTREE_NULL)
    {
        for (x = x->right; x->left != LE_RBTREE_NULL;)
        {
             x = x->left;
        }
    }
    else
    {
        le_rbtree_Node_t *temp = x->parent;
        while (temp != LE_RBTREE_NULL && temp->right == x)
        {
            x = temp;
            temp = temp->parent;
        }
        x = temp;
    }
    return x != LE_RBTREE_NULL ? x : NULL;
}


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
    const le_rbtree_Tree_t *rbt,         ///< [IN] Tree containing currentLinkPtr.
    le_rbtree_Node_t *x                  ///< [IN] Get the link that is relative to this link.
)
{
    LE_UNUSED(rbt);

    if (x->left != LE_RBTREE_NULL)
    {
        for (x = x->left; x->right != LE_RBTREE_NULL;)
        {
            x = x->right;
        }
    }
    else
    {
        le_rbtree_Node_t *temp = x->parent;
        while (temp != LE_RBTREE_NULL && temp->left == x)
        {
            x = temp;
            temp = temp->parent;
        }
        x = temp;
    }
    return x != LE_RBTREE_NULL ? x : NULL;
}


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
    le_rbtree_Tree_t *rbt,             ///< [IN] Tree to insert into.
    le_rbtree_Node_t *element          ///< [IN] New link to add.
)
{
    int cmp = 0;
    le_rbtree_Node_t *current, *parent = LE_RBTREE_NULL;
    le_rbtree_CompareFunc_t compFn = rbt->compFn;

    /* find where node belongs */
    current = rbt->root;
    while (current != LE_RBTREE_NULL)
    {
        cmp = (compFn)(element->key, current->key);
        if (cmp == 0)
        {
            /* entry with equal key already exist */
            return NULL;
        }
        parent = current;
        current = cmp < 0 ? current->left : current->right;
    }

    element->color = LE_RBTREE_RED;
    element->left = LE_RBTREE_NULL;
    element->right = LE_RBTREE_NULL;

    /* insert node in tree */
    current = element;
    if (parent != LE_RBTREE_NULL)
    {
        current->parent = parent;
        if (cmp < 0)
        {
            parent->left = current;
        }
        else
        {
            parent->right = current;
        }
        InsertFixup(rbt, current);
    }
    else
    {
        rbt->root = current;
        current->parent = LE_RBTREE_NULL;
        current->color = LE_RBTREE_BLACK;
    }
    ++rbt->size;
    return element;
}


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
    le_rbtree_Tree_t *rbt,    ///< [IN] Tree to get the object from.
    void *key                 ///< [IN] Pointer to the key to be retrieved
)
{
    le_rbtree_Node_t *node = rbt->root;
    le_rbtree_CompareFunc_t compFn = rbt->compFn;

    while (node != LE_RBTREE_NULL)
    {
        int cmp = (compFn)(key, node->key);
        if (cmp == 0)
        {
            return node;
        }
        else if (cmp < 0)
        {
            node = node->left;
        }
        else
        {
            node = node->right;
        }
    }
    return NULL;
}


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
    le_rbtree_Tree_t *rbt,     ///< [IN] Tree to remove from.
    le_rbtree_Node_t *z        ///< [IN] Node to remove.
)
{
    le_rbtree_Node_t *y;
    le_rbtree_Node_t *child;

    if (NULL == z || (LE_RBTREE_NULL == z) || (LE_RBTREE_NO_COLOR == z->color))
    {
        /* Node is not in the tree */
        return NULL;
    }

    /*
     * y is the node that will actually be removed from the tree.  This will
     * be z if z has fewer than two children, or the tree successor of z
     * otherwise.
     */
    if (z->left == LE_RBTREE_NULL || z->right == LE_RBTREE_NULL)
    {
        /* y has a NULL node as a child */
        y = z;
    }
    else
    {
        /* find tree successor */
        for (y = z->right; y->left != LE_RBTREE_NULL;)
        {
            y = y->left;
        }
    }
    /* x is y's only child */
    child = (y->left != LE_RBTREE_NULL) ? y->left : y->right;

    /* Remove y from the tree. */
    child->parent = y->parent;
    if (y->parent != LE_RBTREE_NULL)
    {
        if (y->parent->left == y)
        {
            y->parent->left = child;
        }
        else
        {
            y->parent->right = child;
        }
    }
    else
    {
        rbt->root = child;
    }

    /*
     * If we removed the tree successor of z rather than z itself, then move
     * the data for the removed node to the one we were supposed to remove.
     */
    if (y != z)
    {
        y->parent = z->parent;
        y->left = z->left;
        y->right = z->right;
        y->color = z->color;

        if (z->parent != LE_RBTREE_NULL)
        {
            if (z->parent->left == z)
            {
                z->parent->left = y;
            }
            else
            {
                z->parent->right = y;
            }
        }
        if (z->left != LE_RBTREE_NULL)
        {
            z->left->parent = y;
        }
        if (z->right != LE_RBTREE_NULL)
        {
            z->right->parent = y;
        }

        if (rbt->root == z)
        {
            rbt->root = y;
        }
    }

    /* reset the deleted node parent-left-right pointers, but preserve the key */
    le_rbtree_InitNode(z, z->key);

    /*
     * Removing a black node might make some paths from root to leaf contain
     * fewer black nodes than others, or it might make two red nodes adjacent.
     */
    if (y->color == LE_RBTREE_BLACK)
    {
        if (child != LE_RBTREE_NULL)
        {
            DeleteFixup(rbt, child);
        }
    }

    --rbt->size;
    return z;
}


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
    le_rbtree_Tree_t *rbt,     ///< [IN] Tree to remove from.
    void *key                  ///< [IN] Pointer to the key to be retrieved
)
{
    le_rbtree_Node_t *z;

    z = le_rbtree_Find(rbt, key);
    if (NULL == z)
    {
        /* Node is not in the tree */
        return NULL;
    }

    return le_rbtree_Remove(rbt, z);
}
