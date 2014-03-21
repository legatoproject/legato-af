//--------------------------------------------------------------------------------------------------
/** @file app.h
 *
 * API for working with Legato user applications.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
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
 * Starts an application.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_Start
(
    app_Ref_t appRef                    ///< [IN] Reference to the application to start.
);


//--------------------------------------------------------------------------------------------------
/**
 * Stops an application.  This is an asynchronous function call that returns immediately but
 * the application may not stop right away.  Set a app stopped handler using app_SetStoppedHandler
 * to get notified when the application actually stops.
 */
//--------------------------------------------------------------------------------------------------
void app_Stop
(
    app_Ref_t appRef                    ///< [IN] Reference to the application to stop.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's state.
 *
 * @return
 *      The application's state.
 */
//--------------------------------------------------------------------------------------------------
app_State_t app_GetState
(
    app_Ref_t appRef                    ///< [IN] Reference to the application to stop.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's name.
 *
 * @return
 *      The application's name.
 */
//--------------------------------------------------------------------------------------------------
const char* app_GetName
(
    app_Ref_t appRef                    ///< [IN] The application reference.
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
 * Gets an application's sandbox path.
 *
 * @return
 *      The application's GID.
 */
//--------------------------------------------------------------------------------------------------
const char* app_GetSandboxPath
(
    app_Ref_t appRef                    ///< [IN] The application reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's configuration path.
 *
 * @return
 *      The application's configuration path.
 */
//--------------------------------------------------------------------------------------------------
const char* app_GetConfigPath
(
    app_Ref_t appRef                    ///< [IN] The application reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * This handler must be called when a SIGCHILD is received for a process that belongs to the
 * specified application.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the procPid was not found for the specified app.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_SigChildHandler
(
    app_Ref_t appRef,                   ///< [IN] The application reference.
    pid_t procPid,                      ///< [IN] The pid of the process that changed state.
    int procExitStatus,                 ///< [IN] The return status of the process given by wait().
    app_FaultAction_t* faultActionPtr   ///< [OUT] The fault action that should be taken.
);


#endif  // LEGATO_SRC_APP_INCLUDE_GUARD
