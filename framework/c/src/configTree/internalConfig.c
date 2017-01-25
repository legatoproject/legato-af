
// -------------------------------------------------------------------------------------------------
/**
 *  @file internalConfig.c
 *
 *  This module handles the details for managing the configTree's own configuration.  The configTree
 *  looks in the "system" tree for all of it's configuration.
 *
 *  @section cfg_acl The configTree ACLs
 *
 *  While the root user can access any and all trees, all other users of the system are more locked
 *  down.  Every non-root user of the configTree is allowed read access to their own default tree.
 *  (That is a tree with the same name as the user.)  However these users are not allowed to write
 *  to this tree, or access any other trees in the system.
 *
 *  To try to access other trees results in access termination.
 *
 *  To grant an app user access to other trees in the system the configTree uses two sets of
 *  white lists.  One to grant read only access, a second to grant read and write access.  To
 *  actually grant this access, simply enter the tree name in the appropriate list.
 *
 *  The structure of the ACL config data is as follows:
 *
@verbatim
/
  apps/
    myApp/
      configLimits/
        acl/
          someReadableTree<string> == read
          someWriteableTree<string> == write
@endverbatim
 *
 *  Where, myApp is the name of the application user in question.  So, given the above configuration
 *  the application 'myApp' has read access to the trees, 'myApp,' and 'someReadableTree.'  The
 *  application also has write access to the tree 'someWriteableTree.'  However all other trees in
 *  the system are off limits.
 *
 *  If you wanted instead to grant 'myApp' read access to all of the trees in the system then you
 *  would instead put the special value 'allAccess' into the configLimits collection with the value
 *  of "read", it would look as follows:
 *
@verbatim
/
  apps/
    myApp/
      configLimits/
        allAccess<string> == read
        acl/
          someWriteableTree<string> == write
@endverbatim
 *
 *  If instead you wanted the application to have read and write access to all trees in the system,
 *  you would set the value allAccess to "write" instead.
 *
 *  If the user is not an application, then the configuration is exactly the same, except it's
 *  stored under the users collection instead of under the apps collection.
 *
@verbatim
/
  apps/
    myApp/
      configLimits/
        acl/
          someReadableTree<string> == read
          someWriteableTree<string> == write
  users/
    SomeUser/
      configLimits/
        allAccess<string> == write
@endverbatim
 *
 *
 *  @section cfg_timeout The configTree Timeout
 *
 *  The configTree's transaction timeout is configured under:
 *
@verbatim
/
  configTree/
    transactionTimeout<int> == 30
@endverbatim
 *
 *  This value is used for both read and write transactions.  If this value is not set then a value
 *  of 30 seconds is used as the default.
 *
 *  So, once a transaction is created using either le_cfg_CreateReadTxn, or le_cfg_CreateWriteTxn it
 *  has the configured amount of time to complete.  If the transaction is not completed within the
 *  timeout then the client that owns the transaction is disconnected so that other pending
 *  transactions may continue.
 *
 * <HR>
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "treeDb.h"
#include "treeUser.h"
#include "nodeIterator.h"
#include "internalConfig.h"




/// Cached value for the transaction timeout.
static time_t TransactionTimeout = 0;


/// Path to the configTree's global configuration.
#define GLOBAL_CONFIG_PATH "/configTree"




//--------------------------------------------------------------------------------------------------
/**
 *  Everytime the configTree's global configuration changes, this function is called to load the
 *  updated data.
 */
//--------------------------------------------------------------------------------------------------
static void OnGlobalConfigChanged
(
    void* contextPtr  ///< User data pointer, however this is unused here.
)
//--------------------------------------------------------------------------------------------------
{
    ni_IteratorRef_t iteratorRef = ni_CreateIterator(NULL,
                                                     NULL,
                                                     tdb_GetTree("system"),
                                                     NI_READ,
                                                     GLOBAL_CONFIG_PATH);

    TransactionTimeout = ni_GetNodeValueInt(iteratorRef, "transactionTimeout", 30);
    ni_Release(iteratorRef);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Check the node that the iterator is positioned at and check to see if the requested permission
 *  is satisfied by the string value found there.
 *
 *  @return A true if the requested permission is satified by the string value found in the config.
 *          False is returned if the config is not present or corrupt somehow.  False is also
 *          returned if the requred permission is not set in the config.
 */
//--------------------------------------------------------------------------------------------------
static bool CheckPermissionStr
(
    ni_IteratorRef_t iteratorRef,   ///< [IN] The iterator to read from.
    tu_TreePermission_t permission  ///< [IN] The permission we're looking for.
)
//--------------------------------------------------------------------------------------------------
{
    // Start off reading the string in question from the config.  If the string is too big, then
    // right away we know that the config is, "off."
    char permissionStr[16] = "";

    if (ni_GetNodeValueString(iteratorRef, "", permissionStr, sizeof(permissionStr), "") != LE_OK)
    {
        return false;
    }

    // Ok, we have a string value, so is write permission set?  If so, then that satisfies both read
    // and write requests.
    if (strcmp(permissionStr, "write") == 0)
    {
        return true;
    }

    // Ok, the value wasn't set to write.  If the caller was requesting write permission then we're
    // done.
    if (permission == TU_TREE_WRITE)
    {
        return false;
    }

    // The caller was requesting read permission, so make sure that the value was set to read.  If
    // so, then again, we're done.
    if (strcmp(permissionStr, "read") == 0)
    {
        return true;
    }

    // Looks like an invalid string was set, so report it and fail the permission check.  No need to
    // report anything if the value wasn't set in the first place.
    if (strcmp(permissionStr, "") != 0)
    {
        char pathStr[LE_CFG_STR_LEN_BYTES] = "";

        LE_ASSERT(ni_GetPathForNode(iteratorRef, "", pathStr, sizeof(pathStr)) == LE_OK);
        LE_WARN("Bad permission value, '%s', for node, '%s'.", permissionStr, pathStr);
    }

    return false;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Initialize and load the configTree's internal configuration.
 */
//--------------------------------------------------------------------------------------------------
void ic_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    tdb_TreeRef_t systemRef = tdb_GetTree("system");

    tdb_AddChangeHandler(systemRef, NULL, GLOBAL_CONFIG_PATH, OnGlobalConfigChanged, NULL);
    OnGlobalConfigChanged(NULL);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Function called to check if the given user has the requested permission on the given tree.
 *
 *  @return True if the user has the requested permission on the tree, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool ic_CheckTreePermission
(
    tu_TreePermission_t permission,  ///< [IN] Try to get a tree with this permission.
    const char* userName,            ///< [IN] The user we're looking up in the config.
    const char* treeName             ///< [IN] The name of the tree we're looking for.
)
//--------------------------------------------------------------------------------------------------
{
    // Start off by looking for the user config.  This config either exits in the system config tree
    // under '/apps/<userName>', or '/users/<userName>' depending on if the given user represents an
    // application or a regular user.
    ni_IteratorRef_t iteratorRef = ni_CreateIterator(NULL,
                                                     NULL,
                                                     tdb_GetTree("system"),
                                                     NI_READ,
                                                     "/apps");

    LE_ASSERT(ni_GoToNode(iteratorRef, userName) == LE_OK);

    if (ni_NodeExists(iteratorRef, "") == false)
    {
        // The user doesn't represent a configured application, so look under the generic user
        // config.  If that fails, we're done.
        LE_ASSERT(ni_GoToNode(iteratorRef, "/users") == LE_OK);
        LE_ASSERT(ni_GoToNode(iteratorRef, userName) == LE_OK);

        if (ni_NodeExists(iteratorRef, "") == false)
        {
            ni_Release(iteratorRef);
            return false;
        }
    }

    LE_ASSERT(ni_GoToNode(iteratorRef, "./configLimits/") == LE_OK);

    // Ok, we have the app vs. regular user issue sorted out.  Now we can check to see if an
    // allAccess flag has been set.  If it has and it gives us the permission we're looking for then
    // we're done.
    if (ni_NodeExists(iteratorRef, "allAccess") == true)
    {
        LE_ASSERT(ni_GoToNode(iteratorRef, "allAccess") == LE_OK);

        if (CheckPermissionStr(iteratorRef, permission) == true)
        {
            ni_Release(iteratorRef);
            return true;
        }

        LE_ASSERT(ni_GoToNode(iteratorRef, "..") == LE_OK);
    }

    // Looks like the global permission has not been set, so we have to dig into the ACL and check
    // for the permission specificly registered on the tree in question.
    LE_ASSERT(ni_GoToNode(iteratorRef, "acl") == LE_OK);
    LE_ASSERT(ni_GoToNode(iteratorRef, treeName) == LE_OK);

    bool result = CheckPermissionStr(iteratorRef, permission);

    ni_Release(iteratorRef);

    return result;
}




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
)
//--------------------------------------------------------------------------------------------------
{
    return TransactionTimeout;
}
