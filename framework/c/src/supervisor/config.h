//--------------------------------------------------------------------------------------------------
/** @file config.h
 *
 * A temporary API to access the configuration tree.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


#ifndef LEGATO_SRC_CONFIG_INCLUDE_GUARD
#define LEGATO_SRC_CONFIG_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the configuration tree.
 */
//--------------------------------------------------------------------------------------------------
void cfg_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the list of values for the specified node.  The last value in the list is set to NULL.
 * The returned list must be freed by calling cfg_Free when it is no longer needed.
 *
 * @return
 *      The list of values.  Each value is a string.
 *      NULL if the node could not be found.
 */
//--------------------------------------------------------------------------------------------------
char** cfg_Get
(
    const char* nodePath    ///< [IN] The full path of the node.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the list of values for the specified node.  The last value in the list is set to NULL.
 * The returned list must be freed by calling cfg_Free when it is no longer needed.
 *
 * @return
 *      The list of values.  Each value is a string.
 *      NULL if the node could not be found.
 */
//--------------------------------------------------------------------------------------------------
char** cfg_GetRelative
(
    const char* rootPath,   ///< [IN] The root path of the node.
    const char* nodePath    ///< [IN] The node path relative to the root path.
);


//--------------------------------------------------------------------------------------------------
/**
 * Releases the resources for a value list.
 */
//--------------------------------------------------------------------------------------------------
void cfg_Free
(
    char** valueListPtr   ///< [IN] The list to release.
);


#endif  // LEGATO_SRC_CONFIG_INCLUDE_GUARD
