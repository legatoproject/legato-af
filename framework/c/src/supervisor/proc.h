//--------------------------------------------------------------------------------------------------
/** @file supervisor/proc.h
 *
 * API for working with child processes.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "watchdogAction.h"

#ifndef LEGATO_SRC_PROC_INCLUDE_GUARD
#define LEGATO_SRC_PROC_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * The process object reference.
 */
//--------------------------------------------------------------------------------------------------
typedef struct proc_Ref* proc_Ref_t;


//--------------------------------------------------------------------------------------------------
/**
 * Process states.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PROC_STATE_STOPPED,     ///< The process object does not reference an actual runing process, ie.
                            /// no valid PID.
    PROC_STATE_RUNNING,     ///< The process object references an actual process with a valid PID.
    PROC_STATE_PAUSED       ///< The process object references a process that has been stopped, ie.
                            ///  it was sent a SIGSTOP signal.
}
proc_State_t;


//--------------------------------------------------------------------------------------------------
/**
 * Process fault actions.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PROC_FAULT_ACTION_NO_FAULT,         ///< There wasn't a fault.
    PROC_FAULT_ACTION_IGNORE,           ///< A fault occured but no further action is required.
    PROC_FAULT_ACTION_RESTART,          ///< The process should be restarted.
    PROC_FAULT_ACTION_RESTART_APP,      ///< The application should be restarted.
    PROC_FAULT_ACTION_STOP_APP,         ///< The application should be terminated.
    PROC_FAULT_ACTION_REBOOT            ///< The system should be rebooted.
}
proc_FaultAction_t;


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the process system.
 */
//--------------------------------------------------------------------------------------------------
void proc_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Create a process object.
 *
 * @note The name of the process is the node name (last part) of the cfgPathRootPtr.
 *
 * @return
 *      A reference to a process object if successful.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
proc_Ref_t proc_Create
(
    const char* cfgPathRootPtr,     ///< [IN] The path in the config tree for this process.
    app_Ref_t appRef                ///< [IN] Reference to the app that we are part of.
);


//--------------------------------------------------------------------------------------------------
/**
 * Delete the process object.  The process must be stopped before it is deleted.
 *
 * @note If this function fails it will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void proc_Delete
(
    proc_Ref_t procRef              ///< [IN] The process to start.
);


//--------------------------------------------------------------------------------------------------
/**
 * Starts a process, running as the root user, in a given working directory.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t proc_Start
(
    proc_Ref_t procRef,             ///< [IN] The process to start.
    const char* workingDirPtr       ///< [IN] The path to the process's working directory, relative
                                    ///       to the sandbox directory.
);


//--------------------------------------------------------------------------------------------------
/**
 * Start the process in a sandbox.
 *
 * The process will chroot to the sandboxDirPtr and assume the workingDirPtr is relative to the
 * sandboxDirPtr.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t proc_StartInSandbox
(
    proc_Ref_t procRef,             ///< [IN] The process to start.
    const char* workingDirPtr,      ///< [IN] The path to the process's working directory, relative
                                    ///       to the sandbox directory.
    uid_t uid,                      ///< [IN] The user ID to start the process as.
    gid_t gid,                      ///< [IN] The primary group ID for this process.
    const gid_t* groupsPtr,         ///< [IN] List of supplementary groups for this process.
    size_t numGroups,               ///< [IN] The number of groups in the supplementary groups list.
    const char* sandboxDirPtr       ///< [IN] The path to the root of the sandbox this process is to
                                    ///       run in.  If NULL then process will be unsandboxed.
);


//--------------------------------------------------------------------------------------------------
/**
 * Used to indicate that the process is intentionally being stopped externally and not due to a
 * fault.  The process state is not updated right away only when the process actually stops.
 */
//--------------------------------------------------------------------------------------------------
void proc_Stopping
(
    proc_Ref_t procRef              ///< [IN] The process to stop.
);


//--------------------------------------------------------------------------------------------------
/**
 * Pause the running process.  This is an asynchronous function call that returns immediately but
 * the process state may not be updated right away.  Set a state change handler using
 * proc_SetStateChangeHandler to get notified when the process actually pauses.
 */
//--------------------------------------------------------------------------------------------------
void proc_Pause
(
    proc_Ref_t procRef              ///< [IN] The process to pause.
);


//--------------------------------------------------------------------------------------------------
/**
 * Resume the running process.  This is an asynchronous function call that returns immediately but
 * the process state may not be updated right away.  Set a state change handler using
 * proc_SetStateChangeHandler to get notified when the process actually resumes.
 */
//--------------------------------------------------------------------------------------------------
void proc_Resume
(
    proc_Ref_t procRef              ///< [IN] The process to resume.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the process state.
 *
 * @return
 *      The process state.
 */
//--------------------------------------------------------------------------------------------------
proc_State_t proc_GetState
(
    proc_Ref_t procRef             ///< [IN] The process reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the process's PID.
 *
 * @return
 *      The process's PID if the state is not PROC_STATE_DEAD.
 *      -1 if the state is PROC_STATE_DEAD.
 */
//--------------------------------------------------------------------------------------------------
pid_t proc_GetPID
(
    proc_Ref_t procRef             ///< [IN] The process reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the process's name.
 *
 * @return
 *      The process's name.
 */
//--------------------------------------------------------------------------------------------------
const char* proc_GetName
(
    proc_Ref_t procRef             ///< [IN] The process reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the name of the application that this process belongs to.
 *
 * @return
 *      The application name.
 */
//--------------------------------------------------------------------------------------------------
const char* proc_GetAppName
(
    proc_Ref_t procRef             ///< [IN] The process reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the process's previous fault time.
 *
 * @return
 *      The process's name.
 */
//--------------------------------------------------------------------------------------------------
time_t proc_GetFaultTime
(
    proc_Ref_t procRef             ///< [IN] The process reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the process's config path.
 *
 * @return
 *      The process's config path.
 */
//--------------------------------------------------------------------------------------------------
const char* proc_GetConfigPath
(
    proc_Ref_t procRef             ///< [IN] The process reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Determines if the process is a realtime process.
 *
 * @return
 *      true if the process has realtime priority.
 *      false if the process does not have realtime priority.
 */
//--------------------------------------------------------------------------------------------------
bool proc_IsRealtime
(
    proc_Ref_t procRef             ///< [IN] The process reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * This handler must be called when a SIGCHILD is received for the specified process.
 *
 * @return
 *      The fault action that should be taken for this process.
 */
//--------------------------------------------------------------------------------------------------
proc_FaultAction_t proc_SigChildHandler
(
    proc_Ref_t procRef,             ///< [IN] The process reference.
    int procExitStatus              ///< [IN] The status of the process given by wait().
);

//--------------------------------------------------------------------------------------------------
/**
 * This handler must be called when the watchdog expires for the specified process.
 *
 * @return
 *      The watchdog action that should be taken for this process or one of the following:
 *          WATCHDOG_ACTION_NOT_FOUND - no action was configured for this process
 *          WATCHDOG_ACTION_ERROR     - the action could not be read or is unknown
 *          WATCHDOG_ACTION_HANDLED   - no further action is required, it is already handled.
 */
//--------------------------------------------------------------------------------------------------
wdog_action_WatchdogAction_t proc_GetWatchdogAction
(
    proc_Ref_t procRef             ///< [IN] The process reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Sets the priority level for the specified process.
 *
 * The priority level string can be either "idle", "low", "medium", "high", "rt1" ... "rt32".
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t proc_SetPriority
(
    const char* priorStr,   ///< [IN] Priority level string.
    pid_t pid               ///< [IN] PID of the process to set the priority for.
);


#endif //LEGATO_SRC_PROC_INCLUDE_GUARD
