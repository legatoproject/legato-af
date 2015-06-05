//--------------------------------------------------------------------------------------------------
/** @file instStat.c
 *
 * Provides notification for when applications get installed and uninstalled.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "appCfg.h"
#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * Estimated maximum number of installed apps at a time.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_ESTIMATED_NUM_APPS                  29


//--------------------------------------------------------------------------------------------------
/**
 * Estimated maximum number of handlers at a time.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_ESTIMATED_NUM_HANDLERS              11


//--------------------------------------------------------------------------------------------------
/**
 * Reference map of handler functions.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t HandlerRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * App install event.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t AppInstEvent;


//--------------------------------------------------------------------------------------------------
/**
 * App uninstall event.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t AppUninstEvent;


//--------------------------------------------------------------------------------------------------
/**
 * Hashed map of cached list of installed applications.
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t CachedApps;


//--------------------------------------------------------------------------------------------------
/**
 * Hashed map of current list of installed applications.
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t CurrentApps;


//--------------------------------------------------------------------------------------------------
/**
 * Memory pool of application names.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t AppNamePool;


//--------------------------------------------------------------------------------------------------
/**
 * App install event handler function responsible for calling the client's handler with the proper
 * parameters and context pointer.
 */
//--------------------------------------------------------------------------------------------------
static void AppInstallDispatcher
(
    void*   reportPtr,
    void*   secondLayerFunc
)
{
    le_instStat_AppInstallHandlerFunc_t clientHandler = secondLayerFunc;

    const char* appNamePtr = reportPtr;

    clientHandler(appNamePtr, le_event_GetContextPtr());
}


//--------------------------------------------------------------------------------------------------
/**
 * App uninstall event handler function responsible for calling the client's handler with the proper
 * parameters and context pointer.
 */
//--------------------------------------------------------------------------------------------------
static void AppUninstallDispatcher
(
    void*   reportPtr,
    void*   secondLayerFunc
)
{
    le_instStat_AppUninstallHandlerFunc_t clientHandler = secondLayerFunc;

    const char* appNamePtr = reportPtr;

    clientHandler(appNamePtr, le_event_GetContextPtr());
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_instStat_AppInstallEvent'
 *
 * This event provides a notification of when an application is installed.
 */
//--------------------------------------------------------------------------------------------------
le_instStat_AppInstallEventHandlerRef_t le_instStat_AddAppInstallEventHandler
(
    le_instStat_AppInstallHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    // Check parameters.
    if (handlerPtr == NULL)
    {
        LE_KILL_CLIENT("Null handlerPtr");
        return NULL;
    }

    // Add an event handler for this handler.
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("AppInstallHandler",
                                                                  AppInstEvent,
                                                                  AppInstallDispatcher,
                                                                  handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return le_ref_CreateRef(HandlerRefMap, handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_instStat_AppInstallEvent'
 */
//--------------------------------------------------------------------------------------------------
void le_instStat_RemoveAppInstallEventHandler
(
    le_instStat_AppInstallEventHandlerRef_t addHandlerRef
        ///< [IN]
)
{
    le_event_HandlerRef_t handlerRef = le_ref_Lookup(HandlerRefMap, addHandlerRef);

    if (handlerRef == NULL)
    {
        LE_KILL_CLIENT("Invalid handler reference.");
        return;
    }

    le_event_RemoveHandler(handlerRef);

    le_ref_DeleteRef(HandlerRefMap, addHandlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_instStat_AppUninstallEvent'
 *
 * This event provides a notification of when an application is uninstalled.
 */
//--------------------------------------------------------------------------------------------------
le_instStat_AppUninstallEventHandlerRef_t le_instStat_AddAppUninstallEventHandler
(
    le_instStat_AppUninstallHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    // Check parameters.
    if (handlerPtr == NULL)
    {
        LE_KILL_CLIENT("Null handlerPtr");
        return NULL;
    }

    // Add an event handler for this handler.
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("AppUninstallHandler",
                                                                  AppUninstEvent,
                                                                  AppUninstallDispatcher,
                                                                  handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return le_ref_CreateRef(HandlerRefMap, handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_instStat_AppUninstallEvent'
 */
//--------------------------------------------------------------------------------------------------
void le_instStat_RemoveAppUninstallEventHandler
(
    le_instStat_AppUninstallEventHandlerRef_t addHandlerRef
        ///< [IN]
)
{
    le_event_HandlerRef_t handlerRef = le_ref_Lookup(HandlerRefMap, addHandlerRef);

    if (handlerRef == NULL)
    {
        LE_KILL_CLIENT("Invalid handler reference.");
        return;
    }

    le_event_RemoveHandler(handlerRef);

    le_ref_DeleteRef(HandlerRefMap, addHandlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Disables app install/uninstall notifications.
 */
//--------------------------------------------------------------------------------------------------
static void DisableAppNotification
(
    void
)
{
    LE_CRIT("Disabling reporting of app install/uninstall.");

    // Remove the change notification on the config.
    appCfg_DeleteChangeHandler();
}


//--------------------------------------------------------------------------------------------------
/**
 * Removes all app names from the hash map and release the memory for the app names.
 */
//--------------------------------------------------------------------------------------------------
static void ClearAppHashmap
(
    le_hashmap_Ref_t hashmap
)
{
    le_hashmap_It_Ref_t iter = le_hashmap_GetIterator(hashmap);

    while(le_hashmap_NextNode(iter) == LE_OK)
    {
        void* keyPtr = (void*)le_hashmap_GetKey(iter);

        LE_ASSERT(keyPtr != NULL);
        LE_ASSERT(le_hashmap_Remove(hashmap, keyPtr) != NULL);

        le_mem_Release(keyPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Reads the current list of apps and stores them in the specified hash map.
 */
//--------------------------------------------------------------------------------------------------
static void GetAppList
(
    le_hashmap_Ref_t hashmap
)
{
    // Clear the hashmap first.
    ClearAppHashmap(hashmap);

    // Populate the hashmap from the configs list of apps.
    appCfg_Iter_t appIter = appCfg_CreateAppsIter();

    while(1)
    {
        char* appNamePtr = le_mem_ForceAlloc(AppNamePool);

        le_result_t result = appCfg_GetNextItem(appIter);

        if (result == LE_NOT_FOUND)
        {
            // No more apps.
            appCfg_DeleteIter(appIter);
            le_mem_Release(appNamePtr);
            break;
        }

        result = appCfg_GetAppName(appIter, appNamePtr, LIMIT_MAX_APP_NAME_BYTES);

        if (result == LE_OVERFLOW)
        {
            LE_CRIT("App name '%s..' is too long.  Config may have been corrupted.", appNamePtr);

            DisableAppNotification();

            appCfg_DeleteIter(appIter);
            le_mem_Release(appNamePtr);

            return;
        }

        // Add the app to the hashmap.  Don't have a value so just use a literal.
        LE_ASSERT(le_hashmap_Put(hashmap, appNamePtr, (void*)1) == NULL);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if the app is newly installed.  If so, add it to our cache and report the event.
 */
//--------------------------------------------------------------------------------------------------
static void CheckForInstalledApp
(
    const char* appNamePtr          ///< [IN] Check if this app has been installed.
)
{
    // Check if this app is in the Cached list.
    if (le_hashmap_Get(CachedApps, appNamePtr) == NULL)
    {
        // This is a newly installed app.  Add it to the cached list.
        char* newAppNamePtr = le_mem_ForceAlloc(AppNamePool);

        LE_ASSERT(le_utf8_Copy(newAppNamePtr, appNamePtr, LIMIT_MAX_APP_NAME_BYTES, NULL) == LE_OK);

        LE_ASSERT(le_hashmap_Put(CachedApps, newAppNamePtr, (void*)1) == NULL);

        // Report the app install event.
        le_event_Report(AppInstEvent, newAppNamePtr, LIMIT_MAX_APP_NAME_BYTES);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if the app has been uninstalled.  If so remove it from our cache and report the event.
 */
//--------------------------------------------------------------------------------------------------
static void CheckForUninstalledApp
(
    const char* appNamePtr          ///< [IN] Check if this app has been uninstalled.
)
{
    // Check if this app is in the current list.
    if (le_hashmap_Get(CurrentApps, appNamePtr) == NULL)
    {
        // This is app was uninstalled.  Remove it from our cached list.
        LE_ASSERT(le_hashmap_Remove(CachedApps, appNamePtr) != NULL);

        // Report the app uninstall event.
        le_event_Report(AppUninstEvent, (char*)appNamePtr, LIMIT_MAX_APP_NAME_BYTES);

        le_mem_Release((char*)appNamePtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks to see if applications have been installed/uninstalled and reports the appropriate events.
 */
//--------------------------------------------------------------------------------------------------
static void CheckForAppChanges
(
    void
)
{
    // Get the current list of apps.
    GetAppList(CurrentApps);

    // Iterate through the current list of apps to see if they are in our cached list.  If not then
    // the app was newly installed.
    le_hashmap_It_Ref_t iter = le_hashmap_GetIterator(CurrentApps);

    while(le_hashmap_NextNode(iter) == LE_OK)
    {
        CheckForInstalledApp(le_hashmap_GetKey(iter));
    }

    // Iterate through the cached list of apps to see if they are in the current list.  If not then
    // the app was uninstalled.
    iter = le_hashmap_GetIterator(CachedApps);

    while(le_hashmap_NextNode(iter) == LE_OK)
    {
        CheckForUninstalledApp(le_hashmap_GetKey(iter));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Install Status initialization function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{

    // Create safe reference for customer handlers.
    HandlerRefMap = le_ref_CreateMap("InstAppHandlers", MAX_ESTIMATED_NUM_HANDLERS);

    // Create events to report customer handlers.
    AppInstEvent = le_event_CreateId("AppInstEvent", LIMIT_MAX_APP_NAME_BYTES);
    AppUninstEvent = le_event_CreateId("AppUninstEvent", LIMIT_MAX_APP_NAME_BYTES);

    // Create the hash maps to store the list of applications.
    CachedApps = le_hashmap_Create("CachedApps", MAX_ESTIMATED_NUM_APPS,
                                   le_hashmap_HashString, le_hashmap_EqualsString);

    CurrentApps = le_hashmap_Create("CurrentApps", MAX_ESTIMATED_NUM_APPS,
                                    le_hashmap_HashString, le_hashmap_EqualsString);

    // Create the memory pool for app names.
    AppNamePool = le_mem_CreatePool("Appnames", LIMIT_MAX_APP_NAME_BYTES);

    // Register for a change notification on the config.
    appCfg_SetChangeHandler(CheckForAppChanges);

    // Read the initial set of installed apps.
    GetAppList(CachedApps);
}
