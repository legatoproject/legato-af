//--------------------------------------------------------------------------------------------------
/** @file supervisor/app.h
 *
 * API for working with Legato user applications.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
#ifndef LEGATO_SRC_APP_INCLUDE_GUARD
#define LEGATO_SRC_APP_INCLUDE_GUARD

#include "watchdogAction.h"


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
 * Process state.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    APP_PROC_STATE_STOPPED, ///< The application process is not running.
    APP_PROC_STATE_RUNNING, ///< The application process is running.
    APP_PROC_STATE_PAUSED   ///< The application process has been paused.
}
app_ProcState_t;


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
 * the application may not stop right away.  Check the application's state with app_GetState() to
 * see when the application actually stops.
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
 * Gets the state of a process belonging to an application.
 *
 * @return
 *      The process's state.
 */
//--------------------------------------------------------------------------------------------------
app_ProcState_t app_GetProcState
(
    app_Ref_t appRef,                   ///< [IN] Reference to the application to stop.
    const char* procName                ///< [IN] Name of the process.
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
 * Check to see if the application is sandboxed or not.
 *
 * @return
 *      True if the app is sandboxed, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool app_GetIsSandboxed
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
 */
//--------------------------------------------------------------------------------------------------
void app_SigChildHandler
(
    app_Ref_t appRef,                   ///< [IN] The application reference.
    pid_t procPid,                      ///< [IN] The pid of the process that changed state.
    int procExitStatus,                 ///< [IN] The return status of the process given by wait().
    app_FaultAction_t* faultActionPtr   ///< [OUT] The fault action that should be taken.
);


//--------------------------------------------------------------------------------------------------
/**
 * This handler must be called when the watchdog expires for a process that belongs to the
 * specified application.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the procPid was not found for the specified app.
 *
 *      The watchdog action passed in will be set to the action that should be taken for
 *      this process or one of the following:
 *          WATCHDOG_ACTION_NOT_FOUND - no action was configured for this process
 *          WATCHDOG_ACTION_ERROR     - the action could not be read or is unknown
 *          WATCHDOG_ACTION_HANDLED   - no further action is required, it is already handled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_WatchdogTimeoutHandler
(
    app_Ref_t appRef,                                ///< [IN] The application reference.
    pid_t procPid,                                   ///< [IN] The pid of the process that changed
                                                     ///< state.
    wdog_action_WatchdogAction_t* watchdogActionPtr  ///< [OUT] The watchdog action that should be
                                                     ///< taken.
);

#endif  // LEGATO_SRC_APP_INCLUDE_GUARD
