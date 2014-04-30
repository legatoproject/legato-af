
// -------------------------------------------------------------------------------------------------
/**
 *  @file treeUser.c
 *
 *  Implementation of the tree user module.  The tree user object's keep track of the user default
 *  trees.  In the future, tree accessibility permissions will also be add to these objects.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013, 2014. All rights reserved. Use of this work is subject
 *  to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "user.h"
#include "interfaces.h"
#include "treeDb.h"
#include "treePath.h"
#include "treeUser.h"




// -------------------------------------------------------------------------------------------------
/**
 *  Structure used to keep track of the users of the configTree.
 */
// -------------------------------------------------------------------------------------------------
typedef struct User
{
    uid_t userId;                  ///< OS id for the user.
    char userName[MAX_USER_NAME];  ///< Human friendly name for the user.
    char treeName[MAX_USER_NAME];  ///< Human friendly name for the user's default tree.

    // TODO: Other tree user permissions.
}
User_t;




/// The collection of configuration trees managed by the system.
le_hashmap_Ref_t UserCollectionRef = NULL;


// Pool of user objects.
le_mem_PoolRef_t UserPoolRef = NULL;


/// Name of the user collection object.
#define CFG_USER_COLLECTION_NAME "userCollection"


/// Name of the memory pool backing the user objects.
#define CFG_USER_POOL_NAME "userPool"




// -------------------------------------------------------------------------------------------------
/**
 *  Create a new user information block, complete with that users name, id, and default tree name.
 */
// -------------------------------------------------------------------------------------------------
static tu_UserRef_t CreateUserInfo
(
    uid_t userId,          ///< The linux Id of the user in question.
    const char* userName,  ///< The name of the user.
    const char* treeName   ///< The name of the default tree for this user.
)
// -------------------------------------------------------------------------------------------------
{
    tu_UserRef_t userRef = le_mem_ForceAlloc(UserPoolRef);

    userRef->userId = userId;
    strncpy(userRef->userName, userName, MAX_USER_NAME);
    strncpy(userRef->treeName, treeName, MAX_TREE_NAME);

    userRef->userName[MAX_USER_NAME - 1] = 0;
    userRef->treeName[MAX_TREE_NAME - 1] = 0;

    // TODO: Load tree permissions, and possibly default tree name.

    LE_ASSERT(le_hashmap_Put(UserCollectionRef, userRef->userName, userRef) == NULL);

    LE_DEBUG("** Allocated new user object <%p>: '%s', %u with default tree, '%s'.",
             userRef,
             userRef->userName,
             userRef->userId,
             userRef->treeName);

    return userRef;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Look-up a user's information based on a given user name.
 *
 *  @return A pointer to a user information block, NULL if not found.
 */
// -------------------------------------------------------------------------------------------------
static tu_UserRef_t GetUserFromName
(
    const char* userName  ///< The user name to search for.
)
// -------------------------------------------------------------------------------------------------
{
    tu_UserRef_t userRef = le_hashmap_Get(UserCollectionRef, userName);

    return userRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Get the user info for the given user id.
 */
//--------------------------------------------------------------------------------------------------
static tu_UserRef_t GetUser
(
    uid_t userId  // The user id to look up.
)
//--------------------------------------------------------------------------------------------------
{
    // If the connected user has the same uid we're running under, treat the user as if they're
    // root.
    if (userId == geteuid())
    {
        userId = 0;
    }

    char userName[MAX_USER_NAME] = { 0 };
    tu_UserRef_t userRef = NULL;

    // At this point, grab the user's app name, which will succeed if it is an app, otherwise we get
    // the standard user name.
    if (user_GetAppName(userId, userName, MAX_USER_NAME) != LE_OK)
    {
        LE_ASSERT(user_GetName(userId, userName, MAX_USER_NAME) == LE_OK);
    }

    // Now try to look up this user in our hash table.  If not found, create it now.
    userRef = GetUserFromName(userName);
    if (userRef == NULL)
    {
        userRef = CreateUserInfo(userId, userName, userName);
    }

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

    // Startup the internal Legato user API.
    user_Init();

    // Create our memory pools and allocate the info for the root user.
    UserPoolRef = le_mem_CreatePool(CFG_USER_POOL_NAME, sizeof(User_t));
    UserCollectionRef = le_hashmap_Create(CFG_USER_COLLECTION_NAME,
                                          31,
                                          le_hashmap_HashString,
                                          le_hashmap_EqualsString);

    // Create our default root user/tree association.
    CreateUserInfo(0, "root", "system");
}




//--------------------------------------------------------------------------------------------------
/**
 *  Get the OS id for this user object.
 */
//--------------------------------------------------------------------------------------------------
uid_t tu_GetUserId
(
    tu_UserRef_t userRef  // The user object to read.
)
//--------------------------------------------------------------------------------------------------
{
    return userRef->userId;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Get the name associated with this user object.
 */
//--------------------------------------------------------------------------------------------------
const char* tu_GetUserName
(
    tu_UserRef_t userRef  // The user object to read.
)
//--------------------------------------------------------------------------------------------------
{
    return userRef->userName;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Get the information for a user on the other side of a config API connection.
 */
//--------------------------------------------------------------------------------------------------
tu_UserRef_t tu_GetUserInfo
(
    le_msg_SessionRef_t currentSession  ///< Get the user information for this message session.
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(currentSession == NULL, "Bad user message session reference.");

    // Look up the user id of the requesting connection...
    uid_t userId;

    LE_FATAL_IF(le_msg_GetClientUserId(currentSession, &userId) == LE_CLOSED,
                "tu_GetUserInfo must be called within an active connection.");

    // Now that we have a user ID, let's see if we can look them up.
    tu_UserRef_t userRef = GetUser(userId);
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
 *  Get the information for a user on the other side of a config API connection.
 */
//--------------------------------------------------------------------------------------------------
tu_UserRef_t tu_GetCurrentConfigUserInfo
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    return tu_GetUserInfo(le_cfg_GetClientSessionRef());
}





//--------------------------------------------------------------------------------------------------
/**
 *  Get the information for a user on the other side of a configAdmin API connection.
 */
//--------------------------------------------------------------------------------------------------
tu_UserRef_t tu_GetCurrentConfigAdminUserInfo
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    return tu_GetUserInfo(le_cfgAdmin_GetClientSessionRef());
}




//-------------------------------------------------------------------------------------------------
/**
 *  Get a tree for a user, if the tree is specified in the path, get that tree, (if allowed.)
 *  Otherwise get the default tree for that user.
 */
//--------------------------------------------------------------------------------------------------
tdb_TreeRef_t tu_GetRequestedTree
(
    tu_UserRef_t userRef,  ///< Get a tree for this user.
    const char* pathPtr    ///< The path to check.
)
//--------------------------------------------------------------------------------------------------
{
    if (tp_PathHasTreeSpecifier(pathPtr) == true)
    {
        char treeName[MAX_TREE_NAME] = { 0 };

        tp_GetTreeName(treeName, pathPtr);

        LE_DEBUG("Specific tree requested, %s.", treeName);
        return tdb_GetTree(treeName);
    }

    LE_DEBUG("** Getting user default tree.");
    return tdb_GetTree(userRef->treeName);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Kill a client of the configTree API.
 */
//--------------------------------------------------------------------------------------------------
void tu_TerminateClient
(
    le_msg_SessionRef_t sessionRef,  ///< The session that needs to close.
    const char* killMessage          ///< The reason the session needs to close.
)
//--------------------------------------------------------------------------------------------------
{
    tu_UserRef_t userRef = tu_GetUserInfo(sessionRef);

    LE_EMERG("A fatal error occured.  Killing sesssion <%p> for user %s, <%u>.  Reason: %s",
             sessionRef,
             userRef->userName,
             userRef->userId,
             killMessage);

    le_msg_CloseSession(sessionRef);
}
