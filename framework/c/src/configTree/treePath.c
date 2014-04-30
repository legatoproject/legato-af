
// -------------------------------------------------------------------------------------------------
/**
 *  @file treePath.c
 *
 *  Simple path helper functions.  To support users specifying tree names as part of a path, these
 *  functions allow detecting tree names wihtin a path, as well as the seperation of the path from
 *  the tree name.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013, 2014. All rights reserved. Use of this work is subject
 *  to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "treePath.h"




//--------------------------------------------------------------------------------------------------
/**
 *  Check a path and see if there is a tree name embedded.
 */
//--------------------------------------------------------------------------------------------------
bool tp_PathHasTreeSpecifier
(
    const char* pathPtr  ///< The path to check.
)
//--------------------------------------------------------------------------------------------------
{
    return strchr(pathPtr, ':') != NULL;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Copies the tree name from the given path poitner.  Only if there actually is a tree name in that
 *  path.
 *
 *  @note It is assumed that the buffer treeNamePtr is at least MAX_TREE_NAME bytes in size.
 */
//--------------------------------------------------------------------------------------------------
void tp_GetTreeName
(
    char* treeNamePtr,   ///< Buffer to hold the tree name.
    const char* pathPtr  ///< Extract the tree name out of this path.
)
//--------------------------------------------------------------------------------------------------
{
    char* posPtr = strchr(pathPtr, ':');

    if (posPtr == NULL)
    {
        treeNamePtr[0] = 0;
        return;
    }


    size_t count = posPtr - pathPtr;

    if (count > MAX_TREE_NAME)
    {
        count = MAX_TREE_NAME;
    }

    strncpy(treeNamePtr, pathPtr, count);
    treeNamePtr[MAX_TREE_NAME - 1] = 0;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Return a path pointer that excludes the tree name.  This function does not allocate a new
 *  string but instead returns a pointer into the supplied path string.
 *
 *  @return A pointer to the path substring within the tree name/path combo.
 */
//--------------------------------------------------------------------------------------------------
const char* tp_GetPathOnly
(
    const char* pathPtr  ///< The path to strip of a tree name, if any.
)
//--------------------------------------------------------------------------------------------------
{
    char* posPtr = strchr(pathPtr, ':');

    if (posPtr == NULL)
    {
        return pathPtr;
    }

    return ++posPtr;
}
