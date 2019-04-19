//--------------------------------------------------------------------------------------------------
/** @file appCfg.c
 *
 * Provides read access to application configuration settings by reading the configuration
 * database's system tree.  The caller of this API must be have privileges to read the configuration
 * system tree.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "appCfg.h"
#include "interfaces.h"
#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the list of all applications.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_APPS_LIST                               "/apps"


//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
#define CFG_PROCS_LIST                              "./procs"


//--------------------------------------------------------------------------------------------------
/**
 * Node in the config that contains the secure storage limit in bytes.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_LIMIT_SEC_STORE                         "maxSecureStorageBytes"


//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
#define CFG_APP_VERSION                             "version"


//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
#define CFG_APP_START_MANUAL                        "startManual"


//--------------------------------------------------------------------------------------------------
/**
 * .
 */
//--------------------------------------------------------------------------------------------------
#define CFG_PROC_EXEC_NAME                          "args/0"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the fault action for a process.
 *
 * The fault action value must be either IGNORE, RESTART, RESTART_APP, TERMINATE_APP or REBOOT.
 *
 * If this entry in the config tree is missing or is empty, APP_PROC_IGNORE is assumed.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_FAULT_ACTION                       "faultAction"


//--------------------------------------------------------------------------------------------------
/**
 * Fault action string definitions.
 */
//--------------------------------------------------------------------------------------------------
#define IGNORE_STR                                  "ignore"
#define RESTART_STR                                 "restart"
#define RESTART_APP_STR                             "restartApp"
#define STOP_APP_STR                                "stopApp"
#define REBOOT_STR                                  "reboot"


//--------------------------------------------------------------------------------------------------
/**
 * Expect only one open iterator per processes
 */
//--------------------------------------------------------------------------------------------------
#define HIGH_APPS_ITER                                 1


//--------------------------------------------------------------------------------------------------
/**
 * Secure Storage default limit in bytes.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_LIMIT_SEC_STORE                     8192


//--------------------------------------------------------------------------------------------------
/**
 * Change handler.
 */
//--------------------------------------------------------------------------------------------------
static appCfg_ChangeHandler_t ChangeHandler = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Config change handler reference.
 */
//--------------------------------------------------------------------------------------------------
le_cfg_ChangeHandlerRef_t ChangeHandlerRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * The type of the iterator being used.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    ITER_TYPE_APP,
    ITER_TYPE_PROC
}
IterType_t;


//--------------------------------------------------------------------------------------------------
/**
 * The application iterator.
 */
//--------------------------------------------------------------------------------------------------
typedef struct appCfg_Iter_Ref
{
    IterType_t type;
    le_cfg_IteratorRef_t cfgIter;
    bool atFirst;
}
AppsIter_t;


//--------------------------------------------------------------------------------------------------
/**
 * Static pool for the application iterators.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(AppsIter, HIGH_APPS_ITER, sizeof(AppsIter_t));


//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for the application iterators.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t AppIterPool;


//--------------------------------------------------------------------------------------------------
/**
 * Convert the iterator type enumeration to a string.
 *
 * @return
 *     A string representation of the enumeration.
 */
//--------------------------------------------------------------------------------------------------
static const char* TypeToStr
(
    IterType_t type                         ///< [IN] Type enumeration to convert to string.
)
{
    char* typeStr;

    switch (type)
    {
        case ITER_TYPE_APP:  typeStr = "application iterator"; break;
        case ITER_TYPE_PROC: typeStr = "process iterator";     break;
        default:             typeStr = "unknown iterator";     break;
    }

    return typeStr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check the iterator to make sure it is the type expected.  FATAL out if it is not.
 */
//--------------------------------------------------------------------------------------------------
static void CheckFor
(
    appCfg_Iter_t iterRef,
    IterType_t type
)
{
    LE_FATAL_IF(iterRef == NULL, "Iterator reference can not be NULL.");
    LE_FATAL_IF(iterRef->type != type,
                "Expected %s, but received %s instead.",
                TypeToStr(type),
                TypeToStr(iterRef->type));
}


//--------------------------------------------------------------------------------------------------
/**
 * Calls user's change handler when the config changes.
 */
//--------------------------------------------------------------------------------------------------
static void ConfigChangeHandler
(
    void* contextPtr
)
{
    if (ChangeHandler != NULL)
    {
        ChangeHandler();
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the change handler.  This handler will be called whenever any change happens to any
 * application.
 *
 * @warning
 *      Only one change handler can be registered per process.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void appCfg_SetChangeHandler
(
    appCfg_ChangeHandler_t handler          ///< [IN] Change handler.
)
{
    LE_FATAL_IF(handler == NULL, "Change handler cannot be NULL.");

    ChangeHandler = handler;

    // Set our config change handler to the root of the apps list.
    ChangeHandlerRef = le_cfg_AddChangeHandler(CFG_APPS_LIST, ConfigChangeHandler, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes the change handler.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void appCfg_DeleteChangeHandler
(
    void
)
{
    ChangeHandler = NULL;

    // Remove our config change handler to the root of the apps list.
    le_cfg_RemoveChangeHandler(ChangeHandlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates an iterator that can be used to iterate over the list of apps.
 *
 * @note
 *      Iterators have a time out and must be deleted before the timeout expires.
 *
 * @return
 *      Reference to the iterator.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED appCfg_Iter_t appCfg_CreateAppsIter
(
    void
)
{
    AppsIter_t* iterPtr = le_mem_ForceAlloc(AppIterPool);

    iterPtr->type = ITER_TYPE_APP;
    iterPtr->cfgIter = le_cfg_CreateReadTxn(CFG_APPS_LIST);
    iterPtr->atFirst = true;

    return iterPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates an iterator that can be used to iterate over the list of apps, but start the iterator at
 * the given app.
 *
 * @note
 *      Iterators have a time out and must be deleted before the timeout expires.
 *
 * @return
 *      Reference to the iterator, or NULL if the app was not found.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED appCfg_Iter_t appCfg_FindApp
(
    const char* appName         ///< [IN] Name of the app to find.
)
{
    AppsIter_t* iterPtr = appCfg_CreateAppsIter();

    le_cfg_GoToNode(iterPtr->cfgIter, appName);

    if (le_cfg_NodeExists(iterPtr->cfgIter, "") == false)
    {
        appCfg_DeleteIter(iterPtr);
        return NULL;
    }

    return iterPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the name of the application that the iterator is currently pointing at.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer was not big enough for the name.
 *      LE_NOT_FOUND if the iterator is not pointing at an application.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t appCfg_GetAppName
(
    appCfg_Iter_t appIterRef,   ///< [IN] Apps iterator
    char* bufPtr,               ///< [OUT] Buffer to store the app name.
    size_t bufSize              ///< [IN] Size of the buffer.
)
{
    CheckFor(appIterRef, ITER_TYPE_APP);

    if (le_cfg_NodeExists(appIterRef->cfgIter, "") == false)
    {
        return LE_NOT_FOUND;
    }

    return le_cfg_GetNodeName(appIterRef->cfgIter, "", bufPtr, bufSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the application's Secure Storage limit in bytes.
 *
 * @return
 *      The size in bytes if available.  The default size if unavailable.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED size_t appCfg_GetSecStoreLimit
(
    appCfg_Iter_t appIterRef    ///< [IN] Apps iterator
)
{
    CheckFor(appIterRef, ITER_TYPE_APP);

    return le_cfg_GetInt(appIterRef->cfgIter, CFG_LIMIT_SEC_STORE, DEFAULT_LIMIT_SEC_STORE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the version string for the application that the iterator is currently pointing at.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer was not big enough for the version.
 *      LE_NOT_FOUND if the iterator is not pointing at an application.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t appCfg_GetVersion
(
    appCfg_Iter_t appIterRef,   ///< [IN] Apps iterator
    char* bufPtr,               ///< [OUT] Buffer to store the app version.
    size_t bufSize              ///< [IN] Size of the buffer.
)
{
    CheckFor(appIterRef, ITER_TYPE_APP);

    if (le_cfg_NodeExists(appIterRef->cfgIter, "") == false)
    {
        return LE_NOT_FOUND;
    }

    return le_cfg_GetString(appIterRef->cfgIter, CFG_APP_VERSION, bufPtr, bufSize, "");
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the configured start mode for the application.
 *
 * @return
 *      The configured start mode for the application.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED appCfg_StartMode_t appCfg_GetStartMode
(
    appCfg_Iter_t appIterRef    ///< [IN] Apps iterator
)
{
    CheckFor(appIterRef, ITER_TYPE_APP);

    bool startManual = le_cfg_GetBool(appIterRef->cfgIter, CFG_APP_START_MANUAL, false);

    if (startManual)
    {
        return APPCFG_START_MODE_MANUAL;
    }

    return APPCFG_START_MODE_AUTO;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a new iterator for iterating over a given application's processes.
 *
 * @return
 *      A new proc iterator, or NULL if the app iterator
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED appCfg_Iter_t appCfg_CreateAppProcIter
(
    appCfg_Iter_t appIterRef    ///< [IN] Apps iterator
)
{
    CheckFor(appIterRef, ITER_TYPE_APP);

    char pathStr[LE_CFG_STR_LEN_BYTES] = "";
    le_cfg_GetPath(appIterRef->cfgIter, "", pathStr, sizeof(pathStr));

    AppsIter_t* iterPtr = le_mem_ForceAlloc(AppIterPool);

    iterPtr->type = ITER_TYPE_PROC;
    iterPtr->cfgIter = le_cfg_CreateReadTxn(pathStr);
    le_cfg_GoToNode(iterPtr->cfgIter, CFG_PROCS_LIST);
    iterPtr->atFirst = true;

    return iterPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the name of the current application process.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer was not big enough for the name.
 *      LE_NOT_FOUND if the iterator is not pointing at an application.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t appCfg_GetProcName
(
    appCfg_Iter_t procIterRef,  ///< [IN] Apps iterator
    char* bufPtr,               ///< [OUT] Buffer to store the app name.
    size_t bufSize              ///< [IN] Size of the buffer.
)
{
    CheckFor(procIterRef, ITER_TYPE_PROC);

    if (le_cfg_NodeExists(procIterRef->cfgIter, "") == false)
    {
        return LE_NOT_FOUND;
    }

    return le_cfg_GetNodeName(procIterRef->cfgIter, "", bufPtr, bufSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the executable name of the current application's process.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer was not big enough for the name.
 *      LE_NOT_FOUND if the iterator is not pointing at an application.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t appCfg_GetProcExecName
(
    appCfg_Iter_t procIterRef,  ///< [IN] Apps iterator
    char* bufPtr,               ///< [OUT] Buffer to store the app name.
    size_t bufSize              ///< [IN] Size of the buffer.
)
{
    CheckFor(procIterRef, ITER_TYPE_PROC);

    if (le_cfg_NodeExists(procIterRef->cfgIter, "") == false)
    {
        return LE_NOT_FOUND;
    }

    return le_cfg_GetString(procIterRef->cfgIter, CFG_PROC_EXEC_NAME, bufPtr, bufSize, "");
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the configured fault action for the iterator's current process.
 *
 * @return
 *      Value of the configured fault action for the process.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED appCfg_FaultAction_t appCfg_GetProcFaultAction
(
    appCfg_Iter_t procIterRef   ///< [IN] Apps iterator
)
{
    CheckFor(procIterRef, ITER_TYPE_PROC);

    char faultActionStr[LIMIT_MAX_FAULT_ACTION_NAME_BYTES];
    le_result_t result = le_cfg_GetString(procIterRef->cfgIter,
                                          CFG_NODE_FAULT_ACTION,
                                          faultActionStr,
                                          sizeof(faultActionStr),
                                          "");

    if (result != LE_OK)
    {
        LE_CRIT("Fault action string for process is too long.  Assume fault action is 'ignore'.");
        return APPCFG_FAULT_ACTION_IGNORE;
    }

    if (strcmp(faultActionStr, RESTART_STR) == 0)
    {
        return APPCFG_FAULT_ACTION_RESTART;
    }

    if (strcmp(faultActionStr, RESTART_APP_STR) == 0)
    {
        return APPCFG_FAULT_ACTION_RESTART_APP;
    }

    if (strcmp(faultActionStr, STOP_APP_STR) == 0)
    {
        return APPCFG_FAULT_ACTION_STOP_APP;
    }

    if (strcmp(faultActionStr, REBOOT_STR) == 0)
    {
        return APPCFG_FAULT_ACTION_REBOOT;
    }

    if (strcmp(faultActionStr, IGNORE_STR) == 0)
    {
        return APPCFG_FAULT_ACTION_IGNORE;
    }

    LE_WARN("Unrecognized fault action '%s'.  Defaulting to fault action 'ignore'.",
            faultActionStr);

    return APPCFG_FAULT_ACTION_IGNORE;
}


//--------------------------------------------------------------------------------------------------
/**
 * Moves the iterator on to the next item.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if there are no more apps.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t appCfg_GetNextItem
(
    appCfg_Iter_t iter          ///< [IN] Apps iterator
)
{
    le_result_t result;

    if (iter->atFirst)
    {
        result = le_cfg_GoToFirstChild(iter->cfgIter);
        iter->atFirst = false;
    }
    else
    {
        result = le_cfg_GoToNextSibling(iter->cfgIter);
    }

    if (result != LE_OK)
    {
        return LE_NOT_FOUND;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Resets the iterator to the first node.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void appCfg_ResetIter
(
    appCfg_Iter_t iter          ///< [IN] Iterator
)
{
    iter->atFirst = true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes the iterator.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void appCfg_DeleteIter
(
    appCfg_Iter_t iter          ///< [IN] Iterator
)
{
    le_cfg_CancelTxn(iter->cfgIter);

    le_mem_Release(iter);
}


//--------------------------------------------------------------------------------------------------
/**
 * App cfg's initialization function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    AppIterPool = le_mem_InitStaticPool(AppsIter, HIGH_APPS_ITER, sizeof(AppsIter_t));
}
