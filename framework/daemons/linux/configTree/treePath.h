
// -------------------------------------------------------------------------------------------------
/**
 *  @file treePath.h
 *
 *  Simple path helper functions.  To support users specifying tree names as part of a path, these
 *  functions allow detecting tree names wihtin a path, as well as the seperation of the path from
 *  the tree name.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#ifndef CFG_TREE_PATH_INCLUDE_GUARD
#define CFG_TREE_PATH_INCLUDE_GUARD



/// The max size of a config tree name.
#define MAX_TREE_NAME_LEN   LIMIT_MAX_USER_NAME_LEN



// Same as before, but this time in bytes.
#define MAX_TREE_NAME_BYTES LIMIT_MAX_USER_NAME_BYTES




//--------------------------------------------------------------------------------------------------
/**
 *  Check a path and see if there is a tree name embedded.
 */
//--------------------------------------------------------------------------------------------------
bool tp_PathHasTreeSpecifier
(
    const char* pathPtr  ///< [IN] The path to check.
);




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
    char* treeNamePtr,   ///< [OUT] Buffer to hold the tree name.
    const char* pathPtr  ///< [IN] Extract the tree name out of this path.
);




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
);




#endif
