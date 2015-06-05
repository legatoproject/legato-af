/** @file secStoreServer.c
 *
 * Secure Storage Daemon.  This daemon controls application and user access to secure storage.
 *
 * Each application and user is given a separate area in secure storage that they can access.
 * Applications and users can only access their area of secure storage.  Each application and user
 * has a limit to the amount of space they can use in secure storage.  For applications this limit
 * is defined in the application's adef file.  For non-app users the secure storage limit is a
 * default value.
 *
 * This daemon controls access to applications and user areas of secure storage by automatically
 * pre-pending the app name or user name to the access paths.  For example, when application "foo"
 * writes item "bar" the item will be stored as "/app/foo/bar".  Also, if a non-app user "foo"
 * writes item "bar" the item will be stored as "/foo/bar".
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "../appCfg/appCfg.h"
#include "pa_secStore.h"
#include "limit.h"
#include "user.h"


//--------------------------------------------------------------------------------------------------
/**
 * Gets the name of the currently connected client.  If the client process is part of a Legato app
 * then the name will be the name of the app.  If the client process is not part of a Legato app
 * then the name will be the process's effective user name.
 *
 * This function must be called within an IPC message handler from the client.
 *
 * @note
 *      If the caller supplied buffer is too small to fit the name of the client this function will
 *      kill the calling process.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.  In this case the client will be killed.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetClientName
(
    char* bufPtr,                   ///< [OUT] Buffer to store the client name.
    size_t bufSize,                 ///< [IN] Size of the buffer.
    bool* isApp                     ///< [OUT] Set to true if the client is an app.
)
{
    // Get the client's credentials.
    pid_t pid;
    uid_t uid;

    if (le_msg_GetClientUserCreds(le_secStore_GetClientSessionRef(), &uid, &pid) != LE_OK)
    {
        LE_KILL_CLIENT("Could not get credentials for the client.");
        return LE_FAULT;
    }

    // Look up the process's application name.
    le_result_t result = le_appInfo_GetName(pid, bufPtr, bufSize);

    if (result == LE_OK)
    {
        *isApp = true;
        return LE_OK;
    }

    LE_FATAL_IF(result == LE_OVERFLOW, "Buffer too small to contain the application name.");

    // The process was not an app.  Get the linux user name for the process.
    result = user_GetName(uid, bufPtr, bufSize);

    if (result == LE_OK)
    {
        *isApp = false;
        return LE_OK;
    }

    LE_FATAL_IF(result == LE_OVERFLOW, "Buffer too small to contain the user name.");

    // Could not get the user name.
    LE_KILL_CLIENT("Could not get user name for pid %d (uid %d).", pid, uid);

    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the path to the client's area in secure storage.  If the client is an application the path
 * will be "/app/clientName".  If the client is not an application the path will be "/clientName".
 *
 * @note
 *      If the caller buffer is too small this function will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
static void GetClientPath
(
    const char* clientNamePtr,              ///< [IN] Name of the client.
    bool isApp,                             ///< [IN] true if the client is an app.
    char* bufPtr,                           ///< [OUT] Buffer to contain the path.
    size_t bufSize                          ///< [IN] Size of the buffer.
)
{
    bufPtr[0] = '\0';

    if (isApp)
    {
        LE_FATAL_IF(le_path_Concat("/", bufPtr, bufSize, "/app", clientNamePtr, NULL) != LE_OK,
                    "Buffer too small for secure storage path for app %s.", clientNamePtr);
    }
    else
    {
        LE_FATAL_IF(le_path_Concat("/", bufPtr, bufSize, clientNamePtr, NULL) != LE_OK,
                    "Buffer too small for secure storage path for user %s.", clientNamePtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if there is enough space in the client's area of secure storage for the client to write
 * the item.
 *
 * @return
 *      LE_OK if the item would fit in the client's area of secure storage.
 *      LE_NO_MEMORY if there is not enough memory to store the item.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CheckClientLimit
(
    const char* clientNamePtr,              ///< [IN] Name of the client.
    const char* clientPathPtr,              ///< [IN] Path to the client's area in secure storage.
    const char* itemNamePtr,                ///< [IN] Name of the item.
    size_t itemSize                         ///< [IN] Size, in bytes, of the item.
)
{
    // Get the secure storage limit for the client.
    appCfg_Iter_t iter = appCfg_FindApp(clientNamePtr);
    size_t secStoreLimit = appCfg_GetSecStoreLimit(iter);
    appCfg_DeleteIter(iter);

    // Get the current amount of space used by the client.
    size_t usedSpace = 0;
    le_result_t result = pa_secStore_GetSize(clientPathPtr, &usedSpace);

    if ( (result == LE_UNAVAILABLE) || (result == LE_FAULT) )
    {
        return LE_FAULT;
    }

    // Get the size of the item in the secure storage if it already exists.
    char itemPath[LIMIT_MAX_PATH_BYTES] = "";

    LE_FATAL_IF(le_path_Concat("/", itemPath, sizeof(itemPath), clientPathPtr, itemNamePtr, NULL) != LE_OK,
                "Client %s's path for item %s is too long.", clientNamePtr, itemNamePtr);

    size_t origItemSize = 0;
    result = pa_secStore_GetSize(itemPath, &origItemSize);

    if ( (result == LE_UNAVAILABLE) || (result == LE_FAULT) )
    {
        return LE_FAULT;
    }

    // Calculate if replacing the item would fit within the limit.
    if (((ssize_t)(secStoreLimit - usedSpace + origItemSize - itemSize)) >= 0)
    {
        return LE_OK;
    }

    return LE_NO_MEMORY;
}



//--------------------------------------------------------------------------------------------------
/**
 * Writes an item to secure storage.  If the item already exists then it will be overwritten with
 * the new value.  If the item does not already exist then it will be created.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NO_MEMORY if there is not enough memory to store the item.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_secStore_Write
(
    const char* name,               ///< [IN] Name of the secure storage item.
    const uint8_t* bufPtr,          ///< [IN] Buffer contain the data to store.
    size_t bufNumElements           ///< [IN] Size of buffer.
)
{
    // Check parameters.
    if ( (name == NULL) || (!isalnum(name[0])) )
    {
        LE_KILL_CLIENT("Item name is invalid.");
        return LE_FAULT;
    }

    if (bufPtr == NULL)
    {
        LE_KILL_CLIENT("Client buffer should not be NULL.");
        return LE_FAULT;
    }

    // Get the client's name and see if it is an app.
    bool isApp;
    char clientName[LIMIT_MAX_USER_NAME_BYTES];

    if (GetClientName(clientName, sizeof(clientName), &isApp) != LE_OK)
    {
        return LE_FAULT;
    }

    // Get the path to the client's secure storage area.
    char path[LIMIT_MAX_PATH_BYTES];
    GetClientPath(clientName, isApp, path, sizeof(path));

    // Check the available limit for the client.
    le_result_t result = CheckClientLimit(clientName, path, name, bufNumElements);
    if (result != LE_OK)
    {
        return result;
    }

    // Write the item to the secure storage.
    if (le_path_Concat("/", path, sizeof(path), name, NULL) != LE_OK)
    {
        LE_ERROR("Client %s's path for item %s is too long.", clientName, name);
        return LE_FAULT;
    }

    return pa_secStore_Write(path, bufPtr, bufNumElements);
}


//--------------------------------------------------------------------------------------------------
/**
 * Reads an item from secure storage.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer is too small to hold the entire item.  No data will be written to
 *                  the buffer in this case.
 *      LE_NOT_FOUND if the item does not exist.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_secStore_Read
(
    const char* name,               ///< [IN] Name of the secure storage item.
    uint8_t* bufPtr,                ///< [OUT] Buffer to store the data in.
    size_t* bufNumElementsPtr       ///< [INOUT] Size of buffer.
)
{
    // Check parameters.
    if ( (name == NULL) || (!isalnum(name[0])) )
    {
        LE_KILL_CLIENT("Item name is invalid.");
        return LE_FAULT;
    }

    if (bufPtr == NULL)
    {
        LE_KILL_CLIENT("Client buffer should not be NULL.");
        return LE_FAULT;
    }

    // Get the client's name and see if it is an app.
    bool isApp;
    char clientName[LIMIT_MAX_USER_NAME_BYTES];

    if (GetClientName(clientName, sizeof(clientName), &isApp) != LE_OK)
    {
        return LE_FAULT;
    }

    // Get the path to the client's secure storage area.
    char path[LIMIT_MAX_PATH_BYTES];
    GetClientPath(clientName, isApp, path, sizeof(path));

    // Read the item from the secure storage.
    if (le_path_Concat("/", path, sizeof(path), name, NULL) != LE_OK)
    {
        LE_ERROR("Client %s's path for item %s is too long.", clientName, name);
        return LE_FAULT;
    }

    return pa_secStore_Read(path, bufPtr, bufNumElementsPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes an item from secure storage.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the item does not exist.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_secStore_Delete
(
    const char* name    ///< [IN] Name of the secure storage item.
)
{
    // Check parameters.
    if ( (name == NULL) || (!isalnum(name[0])) )
    {
        LE_KILL_CLIENT("Item name is invalid.");
        return LE_FAULT;
    }

    // Get the client's name and see if it is an app.
    bool isApp;
    char clientName[LIMIT_MAX_USER_NAME_BYTES];

    if (GetClientName(clientName, sizeof(clientName), &isApp) != LE_OK)
    {
        return LE_FAULT;
    }

    // Get the path to the client's secure storage area.
    char path[LIMIT_MAX_PATH_BYTES];
    GetClientPath(clientName, isApp, path, sizeof(path));

    // Delete the item from the secure storage.
    if (le_path_Concat("/", path, sizeof(path), name, NULL) != LE_OK)
    {
        LE_ERROR("Client %s's path for item %s is too long.", clientName, name);
        return LE_FAULT;
    }

    return pa_secStore_Delete(path);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes all items for an application.  This handler gets called when an application is
 * uninstalled.
 *
 * @note
 *      We do not get notified when users are removed so users that store data in secure storage
 *      need to clean up after themselves.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteAppItems
(
    const char* appNamePtr,             ///< [IN] Name of the application.
    void* contextPtr                    ///< [IN] Not used.
)
{
    // Get the path to the client's secure storage area.
    char path[LIMIT_MAX_PATH_BYTES];
    GetClientPath(appNamePtr, true, path, sizeof(path));

    // Delete the path from the secure storage.  Can't do anything if there is an error so no need
    // check the return code.
    pa_secStore_Delete(path);
}


//--------------------------------------------------------------------------------------------------
/**
 * The secure storage daemon's initialization function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Register a handler to be called when ever any apps are uninstalled.
    le_instStat_AddAppUninstallEventHandler(DeleteAppItems, NULL);
}
