//--------------------------------------------------------------------------------------------------
/** @file supervisor/app.h
 *
 * API for working with Legato user applications.
 *
 * Copyright (C) Sierra Wireless Inc.
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
 * The application process object reference.
 */
//--------------------------------------------------------------------------------------------------
typedef struct app_ProcRef* app_Proc_Ref_t;


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for a handler that is called when an application process exits.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*app_Proc_StopHandlerFunc_t)
(
    int exitCode,               ///< [IN] Exit code of the process.
    void* contextPtr            ///< [IN] Context pointer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for a handler that is called when an application's process is blocked just before it
 * has called exec().
 */
//--------------------------------------------------------------------------------------------------
typedef void (*app_BlockFunc_t)
(
    pid_t pid,                      ///< [IN] PID of the blocked process.
    const char* procNamePtr,        ///< [IN] Name of the blocked process.
    void* contextPtr                ///< [IN] Context pointer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Fault actions to take when a process experiences a fault (terminated abnormally).
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    FAULT_ACTION_NONE = 0,          ///< No fault action.
    FAULT_ACTION_IGNORE,            ///< Just ignore the fault.
    FAULT_ACTION_RESTART_PROC,      ///< The application should be restarted.
    FAULT_ACTION_RESTART_APP,       ///< The application should be restarted.
    FAULT_ACTION_STOP_APP,          ///< The application should be stopped.
    FAULT_ACTION_REBOOT             ///< The system should be rebooted.
}
FaultAction_t;


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
    APP_PROC_STATE_RUNNING  ///< The application process is running.
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
 * Gets a process container for the app by name.
 *
 * @return
 *      The pointer to a process container if successful.
 *      NULL if the process container could not be found.
 */
//--------------------------------------------------------------------------------------------------
app_Proc_Ref_t app_GetProcContainer
(
    app_Ref_t appRef,               ///< [IN] The application to search in.
    const char* procNamePtr         ///< [IN] The process name to get for.
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
    app_Ref_t appRef                    ///< [IN] Reference to the application.
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
    app_Ref_t appRef,                   ///< [IN] Reference to the application.
    const char* procName                ///< [IN] Name of the process.
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks if a given app is running a top-level process with given PID.
 *
 * An app's top-level processes are those that are started by the Supervisor directly.
 * If the Supervisor starts a process and that process starts another process, this function
 * will not find that second process.
 *
 * @return
 *      true if the process is one of this app's top-level processes, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool app_HasTopLevelProc
(
    app_Ref_t appRef,
    pid_t pid
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
 * Gets the directory path for an app's installation directory in the current running system.
 *
 * @return
 *      The absolute directory path.
 */
//--------------------------------------------------------------------------------------------------
const char* app_GetInstallDirPath
(
    app_Ref_t appRef                    ///< [IN] The application reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's working directory.
 *
 * @return
 *      The application's sandbox path.
 */
//--------------------------------------------------------------------------------------------------
const char* app_GetWorkingDir
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
 * Gets an application's supplementary groups list.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer was too small to hold all the gids.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_GetSupplementaryGroups
(
    app_Ref_t appRef,                   ///< [IN] The application reference.
    gid_t* groupsPtr,                   ///< [OUT] List of group IDs.
    size_t* numGroupsPtr                ///< [IN/OUT] Size of groupsPtr buffer on input.  Number of
                                        ///           groups in groupsPtr on output.
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
    FaultAction_t* faultActionPtr       ///< [OUT] The fault action that should be taken.
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


//--------------------------------------------------------------------------------------------------
/**
 * Creates a reference to an application process.
 *
 * If the process name refers to an existing configured application process then a reference to that
 * process is simply returned.  In this case an executable path may be specified to override the
 * configured executable.
 *
 * If the process name does not match any configured application processes then a new process
 * is created.  In this case an executable path must be specified.
 *
 * Configured processes take their runtime parameters, such as environment variables, priority, etc.
 * from the configuration database while non-configured processes use default parameters.
 *
 * Parameters can be overridden by the other functions in this API such as app_AddProcArg(),
 * app_SetProcPriority(), etc.
 *
 * It is an error to call this function on a configured process that is already running.
 *
 * @return
 *      Reference to the application process if successful.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
app_Proc_Ref_t app_CreateProc
(
    app_Ref_t appRef,                   ///< [IN] The application reference.
    const char* procNamePtr,            ///< [IN] Name of the process.
    const char* execPathPtr             ///< [IN] Path to the executable file.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the standard in of an application process.
 */
//--------------------------------------------------------------------------------------------------
void app_SetProcStdIn
(
    app_Proc_Ref_t appProcRef,  ///< [IN] Process reference.
    int stdInFd                 ///< [IN] File descriptor to use as the app proc's standard in.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the standard out of an application process.
 */
//--------------------------------------------------------------------------------------------------
void app_SetProcStdOut
(
    app_Proc_Ref_t appProcRef,  ///< [IN] Process reference.
    int stdOutFd                ///< [IN] File descriptor to use as the app proc's standard out.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the standard error of an application process.
 */
//--------------------------------------------------------------------------------------------------
void app_SetProcStdErr
(
    app_Proc_Ref_t appProcRef,  ///< [IN] Process reference.
    int stdErrFd                ///< [IN] File descriptor to use as the app proc's standard error.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets a stop handler to be called when the specified process stops.
 */
//--------------------------------------------------------------------------------------------------
void app_SetProcStopHandler
(
    app_Proc_Ref_t appProcRef,                  ///< [IN] Process reference.
    app_Proc_StopHandlerFunc_t stopHandler,     ///< [IN] Handler to call when the process exits.
    void* stopHandlerContextPtr                 ///< [IN] Context data for the stop handler.
);


//--------------------------------------------------------------------------------------------------
/**
 * Adds a cmd-line argument to a process.  Adding a NULL argPtr is valid and can be used to validate
 * the args list without actually adding an argument.  This is useful for overriding the configured
 * arguments with an empty list.
 *
 * This overrides the configured arguments if available.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the argument string is too long.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_AddArgs
(
    app_Proc_Ref_t appProcRef,  ///< [IN] Process reference.
    const char* argPtr          ///< [IN] Argument string.
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes and invalidates the cmd-line arguments to a process.  This means the process will only
 * use arguments from the config if available.
 */
//--------------------------------------------------------------------------------------------------
void app_ClearArgs
(
    app_Proc_Ref_t appProcRef   ///< [IN] Process reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the process's priority.
 *
 * This overrides the configured priority if available.
 *
 * The priority level string can be either "idle", "low", "medium", "high", "rt1" ... "rt32".
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the priority string is too long.
 *      LE_FAULT if the priority string is not valid.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_SetProcPriority
(
    app_Proc_Ref_t appProcRef,  ///< [IN] Process reference.
    const char* priorityPtr     ///< [IN] Priority string.  NULL to clear the priority.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets fault action for a process.
 *
 * This overrides the configured fault action if available.
 *
 * The fault action can be set to FAULT_ACTION_NONE to indicate that the configured fault action
 * should be used if available.
 */
//--------------------------------------------------------------------------------------------------
void app_SetFaultAction
(
    app_Proc_Ref_t appProcRef,          ///< [IN] Process reference.
    FaultAction_t faultAction           ///< [IN] Fault action.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the run flag of a process.
 */
//--------------------------------------------------------------------------------------------------
void app_SetRun
(
    app_Proc_Ref_t appProcRef,  ///< [IN] Process reference.
    bool run                    ///< [IN] Run flag.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the run flag of all processes in an app.
 */
//--------------------------------------------------------------------------------------------------
void app_SetRunForAllProcs
(
    app_Ref_t appRef,  ///< [IN] App reference.
    bool run           ///< [IN] Run flag.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the debug flag of a process.
 */
//--------------------------------------------------------------------------------------------------
void app_SetDebug
(
    app_Proc_Ref_t appProcRef,  ///< [IN] Process reference.
    bool debug                  ///< [IN] Debug flag.
);


//--------------------------------------------------------------------------------------------------
/**
 * Starts an application process.  This function assumes that the app has already started.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_StartProc
(
    app_Proc_Ref_t appProcRef                   ///< [IN] Process reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes an application process from an app.
 *
 * If the process is a configured process the overriden parameters are cleared but the process is
 * not actually deleted.
 */
//--------------------------------------------------------------------------------------------------
void app_DeleteProc
(
    app_Ref_t appRef,                           ///< [IN] App reference.
    app_Proc_Ref_t appProcRef                   ///< [IN] Process reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Adds a new link to a file for the app.  A link to the file will be created in the app's working
 * directory under the same path.  For example, if the path is /bin/ls then a link to the file will
 * be created at appsSandboxRoot/bin/ls.
 *
 * @return
 *      LE_OK if successful.
 *      LE_DUPLICATE if the link could not be created because the path conflicts with an existing
 *                   item already in the app.
 *      LE_NOT_FOUND if the source path does not point to a valid file.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_AddLink
(
    app_Ref_t appRef,                           ///< [IN] App reference.
    const char* pathPtr                         ///< [IN] Absolute path to the file.
);


//--------------------------------------------------------------------------------------------------
/**
 * Remove all links added using app_AddLink().
 */
//--------------------------------------------------------------------------------------------------
void app_RemoveAllLinks
(
    app_Ref_t appRef                            ///< [IN] App reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the permissions for a device file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the source path does not point to a valid device.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_SetDevPerm
(
    app_Ref_t appRef,                           ///< [IN] App reference.
    const char* pathPtr,                        ///< [IN] Absolute path to the device.
    const char* permissionPtr                   ///< [IN] Permission string, "r", "w", "rw".
);


//--------------------------------------------------------------------------------------------------
/**
 * Blocks each app process on startup, after the process is forked and initialized but before it has
 * execed.  The specified callback function will be called when the process has blocked.  Clearing
 * the callback function means processes should not block on startup.
 */
//--------------------------------------------------------------------------------------------------
void app_SetBlockCallback
(
    app_Ref_t appRef,                           ///< [IN] App reference.
    app_BlockFunc_t blockCallback,              ///< [IN] Callback function.  NULL to clear.
    void* contextPtr                            ///< [IN] Context pointer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Unblocks a process that was blocked on startup.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the process could not be found in the app.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_Unblock
(
    app_Ref_t appRef,                           ///< [IN] App reference.
    pid_t pid                                   ///< [IN] Process to unblock.
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the application has any configured processes running.
 *
 * @return
 *      true if there is at least one configured running process for the application.
 *      false if there are no configured running processes for the application.
 */
//--------------------------------------------------------------------------------------------------
bool app_HasConfRunningProc
(
    app_Ref_t appRef                    ///< [IN] The application reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Performs tasks after an app has been stopped.
 */
//--------------------------------------------------------------------------------------------------
void app_StopComplete
(
    app_Ref_t appRef                    ///< [IN] The application reference.
);


#endif  // LEGATO_SRC_APP_INCLUDE_GUARD
