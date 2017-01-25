
// -------------------------------------------------------------------------------------------------
/**
 *  @file internalConfig.h
 *
 *  This module handles the details for managing the configTree's own configuration.  The configTree
 *  looks in the "system" tree for all of it's configuration.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#ifndef CFG_INTERNAL_CONFIG_INCLUDE_GUARD
#define CFG_INTERNAL_CONFIG_INCLUDE_GUARD





//--------------------------------------------------------------------------------------------------
/**
 *  Initialize and load the configTree's internal configuration.
 */
//--------------------------------------------------------------------------------------------------
void ic_Init
(
    void
);




//--------------------------------------------------------------------------------------------------
/**
 *  Function called to check if the given user has the requested permission on the given tree.
 *
 *  @return True if the user has the requested permission on the tree, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool ic_CheckTreePermission
(
    tu_TreePermission_t permission,  ///< Try to get a tree with this permission.
    const char* userName,            ///< The user we're looking up in the config.
    const char* treeName             ///< The name of the tree we're looking for.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Read the current transaction timeout from the configtree's internal data.
 *
 *  @return A timeout count in seconds.
 */
//--------------------------------------------------------------------------------------------------
time_t ic_GetTransactionTimeout
(
    void
);




#endif
