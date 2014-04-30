
// -------------------------------------------------------------------------------------------------
/**
 *  @file treeUser.h
 *
 *  Implementation of the tree user module.  The tree user objects keep track of the user default
 *  trees.  In the future, tree accessibility permissions will also be add to these objects.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013, 2014. All rights reserved. Use of this work is subject
 *  to license.
 */
// -------------------------------------------------------------------------------------------------

#ifndef CFG_TREE_USER_INCLUDE_GUARD
#define CFG_TREE_USER_INCLUDE_GUARD




//--------------------------------------------------------------------------------------------------
/**
 *  Maximum size string for user names.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_USER_NAME 100




//--------------------------------------------------------------------------------------------------
/**
 *  Opaque structure for dealing with users of the config tree.
 */
//--------------------------------------------------------------------------------------------------
typedef struct User* tu_UserRef_t;




//--------------------------------------------------------------------------------------------------
/**
 *  Initialize the user subsystem and get it ready for user lookups.
 */
//--------------------------------------------------------------------------------------------------
void tu_Init
(
    void
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the OS id for this user object.
 */
//--------------------------------------------------------------------------------------------------
uid_t tu_GetUserId
(
    tu_UserRef_t userPtr  ///< The user object to read.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the name associated with this user object.
 */
//--------------------------------------------------------------------------------------------------
const char* tu_GetUserName
(
    tu_UserRef_t userPtr  ///< The user object to read.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the information for a user on the other side of a config API connection.
 *
 *  Note that if the user ID of the connecting process is the same as the user ID that the config
 *  tree was launched with, then the connected user is treated as root.
 */
//--------------------------------------------------------------------------------------------------
tu_UserRef_t tu_GetUserInfo
(
    le_msg_SessionRef_t currentSession  ///< Get the user information for this message session.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the information for a user on the other side of a config API connection.
 */
//--------------------------------------------------------------------------------------------------
tu_UserRef_t tu_GetCurrentConfigUserInfo
(
    void
);





//--------------------------------------------------------------------------------------------------
/**
 *  Get the information for a user on the other side of a configAdmin API connection.
 */
//--------------------------------------------------------------------------------------------------
tu_UserRef_t tu_GetCurrentConfigAdminUserInfo
(
    void
);





//--------------------------------------------------------------------------------------------------
/**
 *  Get a tree for a user, if the tree is specified in the path, get that tree, (if allowed.)
 *  Otherwise get the default tree for that user.
 */
//--------------------------------------------------------------------------------------------------
tdb_TreeRef_t tu_GetRequestedTree
(
    tu_UserRef_t userPtr,  ///< Get a tree for this user.
    const char* pathPtr    ///< The path to check.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Kill a client of the configTree API.
 */
//--------------------------------------------------------------------------------------------------
void tu_TerminateClient
(
    le_msg_SessionRef_t sessionRef,  ///< The session that needs to close.
    const char* killMessage          ///< The reason the session needs to close.
);




#endif
