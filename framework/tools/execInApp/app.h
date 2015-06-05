//--------------------------------------------------------------------------------------------------
/** @file execInApp/app.h
 *
 * Temporary solution until command apps are available.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
#ifndef LEGATO_SRC_APP_INCLUDE_GUARD
#define LEGATO_SRC_APP_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * The application object reference.
 */
//--------------------------------------------------------------------------------------------------
typedef struct app_Ref* app_Ref_t;


//--------------------------------------------------------------------------------------------------
/**
 * Fault action.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    APP_FAULT_ACTION_IGNORE = 0,        ///< Just ignore the fault.
    APP_FAULT_ACTION_RESTART_APP,       ///< The application should be restarted.
    APP_FAULT_ACTION_STOP_APP,          ///< The application should be stopped.
    APP_FAULT_ACTION_REBOOT             ///< The system should be rebooted.
}
app_FaultAction_t;


//--------------------------------------------------------------------------------------------------
/**
 * Application state.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    APP_STATE_STOPPED,      ///< The application sandbox (for sandboxed apps) does not exist and no
                            ///  application processes are running.
    APP_STATE_RUNNING       ///< The application sandbox (for sandboxed apps) exists and atleast one
                            ///  application process is running.
}
app_State_t;


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the application system.
 */
//--------------------------------------------------------------------------------------------------
void app_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates an application object.
 *
 * @note
 *      Only applications that have entries in the config tree can be created.
 *
 * @return
 *      A reference to the application object if success.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
app_Ref_t app_Create
(
    const char* cfgPathRootPtr          ///< [IN] The path in the config tree for this application.
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes an application.  The application must be stopped before it is deleted.
 *
 * @note If this function fails it will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void app_Delete
(
    app_Ref_t appRef                    ///< [IN] Reference to the application to delete.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's UID.
 *
 * @return
 *      The application's UID.
 */
//--------------------------------------------------------------------------------------------------
uid_t app_GetUid
(
    app_Ref_t appRef                    ///< [IN] The application reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's GID.
 *
 * @return
 *      The application's GID.
 */
//--------------------------------------------------------------------------------------------------
gid_t app_GetGid
(
    app_Ref_t appRef                    ///< [IN] The application reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's installation directory path.
 *
 * @return
 *      The application's install directory path.
 */
//--------------------------------------------------------------------------------------------------
const char* app_GetInstallDirPath
(
    app_Ref_t appRef                    ///< [IN] The application reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's sandbox path.
 *
 * @return
 *      The path.
 */
//--------------------------------------------------------------------------------------------------
const char* app_GetSandboxPath
(
    app_Ref_t appRef                    ///< [IN] The application reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's home directory path.
 *
 * If the app is sandboxed, this is relative to the sandbox's root directory.
 *
 * @return A pointer to the path.
 */
//--------------------------------------------------------------------------------------------------
const char* app_GetHomeDirPath
(
    app_Ref_t appRef
);


#endif  // LEGATO_SRC_APP_INCLUDE_GUARD
