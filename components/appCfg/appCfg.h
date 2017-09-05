//--------------------------------------------------------------------------------------------------
/** @file appCfg.h
 *
 * This API is used to read a applications' configuration settings such as limits, fault action,
 * process list, environment variables, etc.  The caller of this API must be have privileges to
 * read the configuration system tree.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_APP_CFG_INCLUDE_GUARD
#define LEGATO_APP_CFG_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Application iterator type.
 */
//--------------------------------------------------------------------------------------------------
typedef struct appCfg_Iter_Ref* appCfg_Iter_t;


//--------------------------------------------------------------------------------------------------
/**
 * Application start options.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    APPCFG_START_MODE_AUTO,               ///< The app will be started when the framework starts.
    APPCFG_START_MODE_MANUAL              ///< The app will not be started when the framework
                                          ///<   starts.
}
appCfg_StartMode_t;


//--------------------------------------------------------------------------------------------------
/**
 * Process fault actions.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    APPCFG_FAULT_ACTION_IGNORE,           ///< A fault occured but no further action is required.
    APPCFG_FAULT_ACTION_RESTART,          ///< The process should be restarted.
    APPCFG_FAULT_ACTION_RESTART_APP,      ///< The application should be restarted.
    APPCFG_FAULT_ACTION_STOP_APP,         ///< The application should be terminated.
    APPCFG_FAULT_ACTION_REBOOT            ///< The system should be rebooted.
}
appCfg_FaultAction_t;


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for applications changes handler.  This handler will be called whenever any change
 * happens to any application.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*appCfg_ChangeHandler_t)
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the change handler.  This handler will be called whenever any change happens to any
 * application.
 *
 * @warning
 *      Only one change handler can be registered per process.
 */
//--------------------------------------------------------------------------------------------------
void appCfg_SetChangeHandler
(
    appCfg_ChangeHandler_t handler          ///< [IN] Change handler.
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes the change handler.
 */
//--------------------------------------------------------------------------------------------------
void appCfg_DeleteChangeHandler
(
    void
);


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
appCfg_Iter_t appCfg_CreateAppsIter
(
    void
);


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
appCfg_Iter_t appCfg_FindApp
(
    const char* appName         ///< [IN] Name of the app to find.
);


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
le_result_t appCfg_GetAppName
(
    appCfg_Iter_t appIterRef,   ///< [IN] Apps iterator
    char* bufPtr,               ///< [OUT] Buffer to store the app name.
    size_t bufSize              ///< [IN] Size of the buffer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the application's Secure Storage limit in bytes.
 *
 * @return
 *      The size in bytes if available.  The default size if unavailable.
 */
//--------------------------------------------------------------------------------------------------
size_t appCfg_GetSecStoreLimit
(
    appCfg_Iter_t appIterRef    ///< [IN] Apps iterator
);


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
le_result_t appCfg_GetVersion
(
    appCfg_Iter_t appIterRef,   ///< [IN] Apps iterator
    char* bufPtr,               ///< [OUT] Buffer to store the app version.
    size_t bufSize              ///< [IN] Size of the buffer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the configured start mode for the application.
 *
 * @return
 *      The configured start mode for the application.
 */
//--------------------------------------------------------------------------------------------------
appCfg_StartMode_t appCfg_GetStartMode
(
    appCfg_Iter_t appIterRef    ///< [IN] Apps iterator
);


//--------------------------------------------------------------------------------------------------
/**
 * Create a new iterator for iterating over a given application's processes.
 *
 * @return
 *      A new proc iterator, or NULL if the app iterator
 */
//--------------------------------------------------------------------------------------------------
appCfg_Iter_t appCfg_CreateAppProcIter
(
    appCfg_Iter_t appIterRef    ///< [IN] Apps iterator
);


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
le_result_t appCfg_GetProcName
(
    appCfg_Iter_t procIterRef,  ///< [IN] Apps iterator
    char* bufPtr,               ///< [OUT] Buffer to store the app name.
    size_t bufSize              ///< [IN] Size of the buffer.
);


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
le_result_t appCfg_GetProcExecName
(
    appCfg_Iter_t procIterRef,  ///< [IN] Apps iterator
    char* bufPtr,               ///< [OUT] Buffer to store the app name.
    size_t bufSize              ///< [IN] Size of the buffer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the configured fault action for the iterator's current process.
 *
 * @return
 *      Value of the configured fault action for the process.
 */
//--------------------------------------------------------------------------------------------------
appCfg_FaultAction_t appCfg_GetProcFaultAction
(
    appCfg_Iter_t procIterRef   ///< [IN] Apps iterator
);


//--------------------------------------------------------------------------------------------------
/**
 * Moves the iterator on to the next item.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if there are no more apps.
 */
//--------------------------------------------------------------------------------------------------
le_result_t appCfg_GetNextItem
(
    appCfg_Iter_t iter          ///< [IN] Apps iterator
);


//--------------------------------------------------------------------------------------------------
/**
 * Resets the iterator to the first node.
 */
//--------------------------------------------------------------------------------------------------
void appCfg_ResetIter
(
    appCfg_Iter_t iter          ///< [IN] Iterator
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes the iterator.
 */
//--------------------------------------------------------------------------------------------------
void appCfg_DeleteIter
(
    appCfg_Iter_t iter          ///< [IN] Iterator
);




#endif  // LEGATO_APP_CFG_INCLUDE_GUARD
