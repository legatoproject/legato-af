//--------------------------------------------------------------------------------------------------
/** @file supervisor/proc.h
 *
 * API for working with child processes.
 *
 * Copyright (C) Sierra Wireless Inc.
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
    PROC_STATE_RUNNING      ///< The process object references an actual process with a valid PID.
}
proc_State_t;


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for callback functions that can be used to indicate when a process has blocked after it
 * forks and initializes but before it has called exec().
 */
//--------------------------------------------------------------------------------------------------
typedef void (*proc_BlockCallback_t)
(
    pid_t pid,                  ///< [IN] The process that has blocked.
    const char* namePtr,        ///< [IN] Name of the blocked process.
    void* contextPtr            ///< [IN] Context pointer.
);


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
 * @note If the config path is given, the last node in the path must be the name of the process.
 *
 * @return
 *      A reference to a process object if successful.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
proc_Ref_t proc_Create
(
    const char* namePtr,            ///< [IN] Name of the process.
    app_Ref_t appRef,               ///< [IN] Reference to the app that we are part of.
    const char* cfgPathRootPtr      ///< [IN] The path in the config tree for this process.  Can be
                                    ///       NULL if there is no config entry for this process.
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
 * Starts a process.  If the process belongs to a sandboxed app the process will run in its sandbox,
 * otherwise the process will run in its working directory as root.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t proc_Start
(
    proc_Ref_t procRef              ///< [IN] The process to start.
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
 * Get the process's config path.
 *
 * @return
 *      The process's config path.
 *      NULL if the process does not have a config.
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
 * Set the process's file descriptors to use as its standard in.
 *
 * By default the standard in is directed to /dev/null.
 */
//--------------------------------------------------------------------------------------------------
void proc_SetStdIn
(
    proc_Ref_t procRef,         ///< [IN] The process reference.
    int stdInFd                 ///< [IN] File descriptor to use as the app proc's standard in.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the process's file descriptors to use as its standard out.
 *
 * By default the standard out is directed to the logs.
 */
//--------------------------------------------------------------------------------------------------
void proc_SetStdOut
(
    proc_Ref_t procRef,         ///< [IN] The process reference.
    int stdOutFd                ///< [IN] File descriptor to use as the app proc's standard out.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the process's file descriptors to use as its standard error.
 *
 * By default the standard out is directed to the logs.
 */
//--------------------------------------------------------------------------------------------------
void proc_SetStdErr
(
    proc_Ref_t procRef,         ///< [IN] The process reference.
    int stdErrFd                ///< [IN] File descriptor to use as the app proc's standard error.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the process's executable path.
 *
 * This overrides the configured executable path if available.  If the configuration for the process
 * is unavailable this function must be called to set the executable path.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the executable path is too long.
 */
//--------------------------------------------------------------------------------------------------
le_result_t proc_SetExecPath
(
    proc_Ref_t procRef,         ///< [IN] The process reference.
    const char* execPathPtr     ///< [IN] Executable path to use for this process.  NULL to clear
                                ///       the executable path.
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
le_result_t proc_SetPriority
(
    proc_Ref_t procRef,         ///< [IN] The process reference.
    const char* priorityPtr     ///< [IN] Priority string.  NULL to clear the priority.
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
le_result_t proc_AddArgs
(
    proc_Ref_t procRef,         ///< [IN] The process reference.
    const char* argPtr          ///< [IN] Argument string.
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes and invalidates the cmd-line arguments to a process.  This means the process will only
 * use arguments from the config if available.
 */
//--------------------------------------------------------------------------------------------------
void proc_ClearArgs
(
    proc_Ref_t procRef          ///< [IN] The process reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set fault action.
 *
 * This overrides the configured fault action if available.
 *
 * The fault action can be set to FAULT_ACTION_NONE to indicate that the configured fault action
 * should be used if available.
 */
//--------------------------------------------------------------------------------------------------
void proc_SetFaultAction
(
    proc_Ref_t procRef,                 ///< [IN] The process reference.
    FaultAction_t faultAction           ///< [IN] Fault action.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the fault action for a given process.
 */
//--------------------------------------------------------------------------------------------------
FaultAction_t proc_GetFaultAction
(
    proc_Ref_t procRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the run flag.
 */
//--------------------------------------------------------------------------------------------------
void proc_SetRun
(
    proc_Ref_t procRef, ///< [IN] The process reference.
    bool run            ///< [IN] Run flag.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the debug flag.
 */
//--------------------------------------------------------------------------------------------------
void proc_SetDebug
(
    proc_Ref_t procRef, ///< [IN] The process reference.
    bool debug          ///< [IN] Debug flag.
);


//--------------------------------------------------------------------------------------------------
/**
 * Blocks the process on startup, after it is forked and initialized but before it has execed.  The
 * specified callback function will be called when the process has blocked.  Clearing the callback
 * function means the process should not block on startup.
 */
//--------------------------------------------------------------------------------------------------
void proc_SetBlockCallback
(
    proc_Ref_t procRef,                     ///< [IN] The process reference.
    proc_BlockCallback_t blockCallback,     ///< [IN] Callback to set.  NULL to clear.
    void* contextPtr                        ///< [IN] Context pointer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Unblocks a process that was blocked on startup.
 */
//--------------------------------------------------------------------------------------------------
void proc_Unblock
(
    proc_Ref_t procRef                      ///< [IN] The process reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * This handler must be called when a SIGCHILD is received for the specified process.
 *
 * @return
 *      The fault action that should be taken for this process.
 */
//--------------------------------------------------------------------------------------------------
FaultAction_t proc_SigChildHandler
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


#endif //LEGATO_SRC_PROC_INCLUDE_GUARD
