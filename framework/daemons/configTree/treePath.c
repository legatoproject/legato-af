
// -------------------------------------------------------------------------------------------------
/**
 *  @file treePath.c
 *
 *  Simple path helper functions.  To support users specifying tree names as part of a path, these
 *  functions allow detecting tree names within a path, as well as the separation of the path from
 *  the tree name.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
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
    const char* pathPtr  ///< [IN] The path to check.
)
//--------------------------------------------------------------------------------------------------
{
    return strchr(pathPtr, ':') != NULL;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Copies the tree name from the given path pointer.  Only if there actually is a tree name in that
 *  path.
 *
 *  @note It is assumed that the buffer treeNamePtr is at least MAX_TREE_NAME_BYTES bytes in size.
 */
//--------------------------------------------------------------------------------------------------
void tp_GetTreeName
(
    char* treeNamePtr,   ///< [OUT] Buffer to hold the tree name.
    const char* pathPtr  ///< [IN]  Extract the tree name out of this path.
)
//--------------------------------------------------------------------------------------------------
{
    // Check and make sure there's a tree name in the path in the first place.
    char* posPtr = strchr(pathPtr, ':');

    if (posPtr == NULL)
    {
        treeNamePtr[0] = 0;
        return;
    }

    // Looks like there is a tree name, so copy it out.  Also, make sure that the name doesn't
    // overflow.  If the name overflows, truncate and report as a warning.
    size_t numBytes = 0;
    le_result_t result = le_utf8_CopyUpToSubStr(treeNamePtr,
                                                pathPtr,
                                                ":",
                                                MAX_TREE_NAME_BYTES,
                                                &numBytes);

    LE_WARN_IF(result != LE_OK,
               "Tree name from path, '%s', truncated to %" PRIuS " bytes, '%s'",
               pathPtr,
               numBytes,
               treeNamePtr);
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
    const char* pathPtr  ///< [IN] The path to strip of a tree name, if any.
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
