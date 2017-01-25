
// -------------------------------------------------------------------------------------------------
/**
 *  @file treeUser.h
 *
 *  Implementation of the tree user module.  The tree user objects keep track of the user default
 *  trees.  In the future, tree accessibility permissions will also be add to these objects.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#ifndef CFG_TREE_USER_INCLUDE_GUARD
#define CFG_TREE_USER_INCLUDE_GUARD




//--------------------------------------------------------------------------------------------------
/**
 *  Opaque structure for dealing with users of the config tree.
 */
//--------------------------------------------------------------------------------------------------
typedef struct User* tu_UserRef_t;




//--------------------------------------------------------------------------------------------------
/**
 *  Types of user permissions on the configuration trees.
 */
//--------------------------------------------------------------------------------------------------
typedef enum tu_TreePermission
{
    TU_TREE_READ,  ///< The user can read from the tree but not write.
    TU_TREE_WRITE  ///< The user can read and write from/to the given tree.
}
tu_TreePermission_t;




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
 *  Function called when an IPC session is connected to the configTree server.  This will allocate
 *  a user record, (if required,) and up it's connection count.
 */
//--------------------------------------------------------------------------------------------------
void tu_SessionConnected
(
    le_msg_SessionRef_t sessionRef  ///< [IN] The session just connected.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Called when a client session is disconnected.
 */
//--------------------------------------------------------------------------------------------------
void tu_SessionDisconnected
(
    le_msg_SessionRef_t sessionRef  ///< [IN] The session just disconnected.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the OS Id for this user object.
 *
 *  @return The OS style user Id for the given user.
 */
//--------------------------------------------------------------------------------------------------
uid_t tu_GetUserId
(
    tu_UserRef_t userPtr  ///< [IN] The user object to read.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the name associated with this user object.
 *
 *  @return The OS level user name for the given user.
 */
//--------------------------------------------------------------------------------------------------
const char* tu_GetUserName
(
    tu_UserRef_t userPtr  ///< [IN] The user object to read.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the information for the current user on the other side of a config API connection.
 *
 *  @note This function must be called within the context of one of the configAPI service handlers.
 *
 *  @note If the user ID of the connecting process is the same as the user ID that the config tree
 *        was launched with, then the connected user is treated as root.
 *
 *  @return A reference to the user information that represents the user on the other end of the
 *          current API connection.
 */
//--------------------------------------------------------------------------------------------------
tu_UserRef_t tu_GetCurrentConfigUserInfo
(
    void
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the information for the current user user on the other side of a configAdmin API connection.
 *
 *  @note This function must be called within the context of one of the configAdminAPI service
 *        handlers.
 *
 *  @return A reference to the user information that represents the user on the other end of the
 *          current API connection.
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
 *
 *  @return A reference to the requested tree.  If the user does not have the requested permission
 *          on the tree a NULL is returned instead.
 */
//--------------------------------------------------------------------------------------------------
tdb_TreeRef_t tu_GetRequestedTree
(
    tu_UserRef_t userRef,            ///< [IN] Get a tree for this user.
    tu_TreePermission_t permission,  ///< [IN] Try to get a tree with this permission.
    const char* pathPtr              ///< [IN] The path to check.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Kill a client of the configTree API.
 */
//--------------------------------------------------------------------------------------------------
void tu_TerminateConfigClient
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] The session that needs to close.
    const char* killMessage          ///< [IN] The reason the session needs to close.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Kill a client of the configTree admin API.
 */
//--------------------------------------------------------------------------------------------------
void tu_TerminateConfigAdminClient
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] The session that needs to close.
    const char* killMessage          ///< [IN] The reason the session needs to close.
);




#endif
