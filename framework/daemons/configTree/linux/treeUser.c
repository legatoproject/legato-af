
// -------------------------------------------------------------------------------------------------
/**
 *  @file treeUser.c
 *
 *  Implementation of the tree user module.  The tree user object's keep track of the user default
 *  trees.  In the future, tree accessibility permissions will also be add to these objects.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "limit.h"
#include "user.h"
#include "interfaces.h"
#include "treeDb.h"
#include "treePath.h"
#include "treeUser.h"
#include "internalConfig.h"




// -------------------------------------------------------------------------------------------------
/**
 *  Structure used to keep track of the users of the configTree.
 */
// -------------------------------------------------------------------------------------------------
typedef struct User
{
    uid_t userId;                              ///< OS id for the user.
    char userName[LIMIT_MAX_USER_NAME_BYTES];  ///< Human friendly name for the user.
    char treeName[LIMIT_MAX_USER_NAME_BYTES];  ///< Human friendly name for the user's default tree.
}
User_t;

/// The collection of configuration trees managed by the system.
le_hashmap_Ref_t UserCollectionRef = NULL;



/// The static hash for users
LE_HASHMAP_DEFINE_STATIC(UserCollection, LE_CONFIG_CFGTREE_MAX_USER_POOL_SIZE);

/// Create static user pool
LE_MEM_DEFINE_STATIC_POOL(UserPool, LE_CONFIG_CFGTREE_MAX_USER_POOL_SIZE, sizeof(User_t));


// Pool of user objects.
le_mem_PoolRef_t UserPoolRef = NULL;



// -------------------------------------------------------------------------------------------------
/**
 *  Create a new user information block, complete with that users name, id, and default tree name.
 */
// -------------------------------------------------------------------------------------------------
static tu_UserRef_t CreateUserInfo
(
    uid_t userId,          ///< [IN] The Linux Id of the user in question.
    const char* userName,  ///< [IN] The name of the user.
    const char* treeName   ///< [IN] The name of the default tree for this user.
)
// -------------------------------------------------------------------------------------------------
{
    tu_UserRef_t userRef = le_mem_ForceAlloc(UserPoolRef);

    userRef->userId = userId;
    LE_ASSERT(le_utf8_Copy(userRef->userName, userName, sizeof(userRef->userName), NULL) == LE_OK);
    LE_ASSERT(le_utf8_Copy(userRef->treeName, treeName, sizeof(userRef->treeName), NULL) == LE_OK);

    LE_ASSERT(le_hashmap_Put(UserCollectionRef, &userRef->userId, userRef) == NULL);

    LE_DEBUG("** Allocated new user object <%p>: '%s', %u with default tree, '%s'.",
             userRef,
             userRef->userName,
             userRef->userId,
             userRef->treeName);

    return userRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Free up after a freed user object.
 */
//--------------------------------------------------------------------------------------------------
static void UserDestructor
(
    void* objectPtr  ///< [IN] The memory object to destroy.
)
//--------------------------------------------------------------------------------------------------
{
    tu_UserRef_t userRef = (tu_UserRef_t)objectPtr;

    le_hashmap_Remove(UserCollectionRef, &userRef->userId);
    memset(userRef, 0, sizeof(User_t));
}




//--------------------------------------------------------------------------------------------------
/**
 *  Get the user info for the given user id.  If the given user info has not been created yet, it is
 *  done so now.
 *
 *  @return A user info reference.
 */
//--------------------------------------------------------------------------------------------------
static tu_UserRef_t GetUser
(
    uid_t userId,     ///< [IN]  The user id to look up.
    bool* wasCreated  ///< [OUT] Was the user info created for this request?  Pass NULL if you don't
                      ///<       need this.
)
//--------------------------------------------------------------------------------------------------
{
    // If the connected user has the same uid we're running under, treat the user as if they're
    // root.
    if (userId == geteuid())
    {
        userId = 0;
    }

    if (wasCreated)
    {
        *wasCreated = false;
    }

    // Now try to look up this user in our hash table.  If not found, create it now.
    tu_UserRef_t userRef = le_hashmap_Get(UserCollectionRef, &userId);

    if (userRef == NULL)
    {
        // At this point, grab the user's app name, which will succeed if it is an app, otherwise we get
        // the standard user name.

        char userName[LIMIT_MAX_USER_NAME_BYTES] = "";

        if (user_GetAppName(userId, userName, sizeof(userName)) != LE_OK)
        {
            LE_ASSERT(user_GetName(userId, userName, sizeof(userName)) == LE_OK);
        }

        userRef = CreateUserInfo(userId, userName, userName);

        if (wasCreated)
        {
            *wasCreated = true;
        }
    }

    return userRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Take a given permission enum value and return a string that represents it.  This is used for
 *  logging permission requests.
 *
 *  @return A constant string that best represents the passed in enumeration value.  Do not free
 *          this string, it's statically allocated.
 */
//--------------------------------------------------------------------------------------------------
static const char* PermissionStr
(
    tu_TreePermission_t permission  ///< [IN] The requested permission flag.
)
//--------------------------------------------------------------------------------------------------
{
    char* permissionStr;

    switch (permission)
    {
        case TU_TREE_READ:
            permissionStr = "read";
            break;

        case TU_TREE_WRITE:
            permissionStr = "write";
            break;

        default:
            permissionStr = "unknown";
            break;
    }

    return permissionStr;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Get the information for a user on the other side of a config API connection.
 *
 *  Note that if the user ID of the connecting process is the same as the user ID that the config
 *  tree was launched with, then the connected user is treated as root.
 *
 *  @return A reference to the user information that represents the user on the other end of the
 *          current API connection.
 */
//--------------------------------------------------------------------------------------------------
static tu_UserRef_t GetUserInfo
(
    le_msg_SessionRef_t currentSession,  ///< [IN]  Get the user information for this message
                                         ///<       session.
    bool* wasCreated                     ///< [OUT] Was the user info created for this request?
                                         ///<       Pass NULL if you don't need this.
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(currentSession == NULL, "Bad user message session reference.");

    // Look up the user id of the requesting connection...
    uid_t userId;

    LE_FATAL_IF(le_msg_GetClientUserId(currentSession, &userId) == LE_CLOSED,
                "tu_GetUserInfo must be called within an active connection.");

    // Now that we have a user ID, let's see if we can look them up.
    tu_UserRef_t userRef = GetUser(userId, wasCreated);
    LE_ASSERT(userRef != NULL);

    LE_DEBUG("** Found user <%p>: '%s', %u with default tree, '%s'.",
             userRef,
             userRef->userName,
             userRef->userId,
             userRef->treeName);

    return userRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Initialize the user subsystem and get it ready for user lookups.
 */
//--------------------------------------------------------------------------------------------------
void tu_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Initialize Tree User subsystem.");

    LE_ASSERT(sizeof(uint32_t) >= sizeof(uid_t));

    // Startup the internal Legato user API.
    user_Init();

    // Create our memory pools and allocate the info for the root user.
    UserPoolRef = le_mem_InitStaticPool(UserPool, LE_CONFIG_CFGTREE_MAX_USER_POOL_SIZE,
                                        sizeof(User_t));
    UserCollectionRef = le_hashmap_InitStatic(UserCollection,
                                              LE_CONFIG_CFGTREE_MAX_USER_POOL_SIZE,
                                              le_hashmap_HashUInt32,
                                              le_hashmap_EqualsUInt32);

    le_mem_SetDestructor(UserPoolRef, UserDestructor);

    // Create our default root user/tree association.
    CreateUserInfo(0, "root", "system");
}




//--------------------------------------------------------------------------------------------------
/**
 *  Function called when an IPC session is connected to the configTree server.  This will allocate
 *  a user record, (if required,) and up it's connection count.
 */
//--------------------------------------------------------------------------------------------------
void tu_SessionConnected
(
    le_msg_SessionRef_t sessionRef  ///< [IN] The session just connected.
)
//--------------------------------------------------------------------------------------------------
{
    bool wasCreated;
    tu_UserRef_t userRef = GetUserInfo(sessionRef, &wasCreated);

    if (   (wasCreated == false)
        && (tu_GetUserId(userRef) != 0))
    {
        le_mem_AddRef(userRef);
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Called when a client session is disconnected.
 */
//--------------------------------------------------------------------------------------------------
void tu_SessionDisconnected
(
    le_msg_SessionRef_t sessionRef  ///< [IN] The session just disconnected.
)
//--------------------------------------------------------------------------------------------------
{
    tu_UserRef_t userRef = GetUserInfo(sessionRef, NULL);
    uid_t userId = tu_GetUserId(userRef);

    // If this isn't the root user, de-ref the user info.  (We don't free the root user.)
    if (userId != 0)
    {
        le_mem_Release(userRef);
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Get the OS id for this user object.
 *
 *  @return The OS style user Id for the given user.
 */
//--------------------------------------------------------------------------------------------------
uid_t tu_GetUserId
(
    tu_UserRef_t userRef  // [IN] The user object to read.
)
//--------------------------------------------------------------------------------------------------
{
    if (userRef == NULL)
    {
        return -1;
    }

    return userRef->userId;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Get the name associated with this user object.
 *
 *  @return The OS level user name for the given user.
 */
//--------------------------------------------------------------------------------------------------
const char* tu_GetUserName
(
    tu_UserRef_t userRef  // [IN] The user object to read.
)
//--------------------------------------------------------------------------------------------------
{
    if (userRef == NULL)
    {
        return "<Internal User>";
    }

    return userRef->userName;
}




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
)
//--------------------------------------------------------------------------------------------------
{
    return GetUserInfo(le_cfg_GetClientSessionRef(), NULL);
}




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
)
//--------------------------------------------------------------------------------------------------
{
    return GetUserInfo(le_cfgAdmin_GetClientSessionRef(), NULL);
}




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
)
//--------------------------------------------------------------------------------------------------
{
    char treeName[MAX_TREE_NAME_BYTES] = "";

    // If the path has the tree name embedded, extract it now.  Otherwise, check to see if the user
    // is trying to write to the default tree.  If it is we extract the tree name for checking
    // permission just like if they explicitly specified the tree name.  If the user is simply
    // trying to read from their default tree, then we grant it without resorting to an ACL lookup.
    if (tp_PathHasTreeSpecifier(pathPtr) == true)
    {
        tp_GetTreeName(treeName, pathPtr);
        LE_DEBUG("** Specific tree requested, '%s'.", treeName);

        // Make sure that this isn't the user's didn't just specify their own default tree.  If they
        // did and they're looking for read access, then just go ahead and grant it.
        if (   (permission == TU_TREE_READ)
            && (strcmp(treeName, userRef->treeName) == 0))
        {
            return tdb_GetTree(userRef->treeName);
        }
    }
    else if (permission == TU_TREE_WRITE)
    {
        LE_DEBUG("** Attempting write access on the default tree, '%s'.", userRef->treeName);
        strncpy(treeName, userRef->treeName, sizeof(treeName));
        treeName[MAX_TREE_NAME_BYTES - 1] = '\0';
    }
    else
    {
        LE_DEBUG("** Opening the default tree, '%s' with read only access.", userRef->treeName);
        return tdb_GetTree(userRef->treeName);
    }

    // If we got this far, it's because we have a tree that we need to do an ACL lookup on.  So do
    // so now, if that check fails, we simply bail.
    if (   (ic_CheckTreePermission(permission, userRef->userName, treeName) == false)
        && (userRef->userId != 0))
    {
        LE_ERROR("The user, '%s', id: %d, does not have %s permission on the tree '%s'.",
                 userRef->userName,
                 userRef->userId,
                 PermissionStr(permission),
                 treeName);

        return NULL;
    }

    // Looks like the user has permission, so grab the tree.
    return tdb_GetTree(treeName);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Kill a client of the configTree API.
 */
//--------------------------------------------------------------------------------------------------
void tu_TerminateConfigClient
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] The session that needs to close.
    const char* killMessage          ///< [IN] The reason the session needs to close.
)
//--------------------------------------------------------------------------------------------------
{
    tu_UserRef_t userRef = GetUserInfo(sessionRef, NULL);

    LE_EMERG("A fatal error occurred.  Killing config session <%p> for user %s, <%u>.  Reason: %s",
             sessionRef,
             tu_GetUserName(userRef),
             tu_GetUserId(userRef),
             killMessage);

    le_msg_CloseSession(sessionRef);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Kill a client of the configTree admin API.
 */
//--------------------------------------------------------------------------------------------------
void tu_TerminateConfigAdminClient
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] The session that needs to close.
    const char* killMessage          ///< [IN] The reason the session needs to close.
)
//--------------------------------------------------------------------------------------------------
{
    tu_UserRef_t userRef = GetUserInfo(sessionRef, NULL);

    LE_EMERG("A fatal error occurred.  Killing admin session <%p> for user %s, <%u>.  Reason: %s",
             sessionRef,
             tu_GetUserName(userRef),
             tu_GetUserId(userRef),
             killMessage);

    le_msg_CloseSession(sessionRef);
}
