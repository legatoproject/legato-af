//--------------------------------------------------------------------------------------------------
/** @file supervisor/proc.h
 *
 * API for working with child processes.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

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
 * Confines the calling process into the sandbox.  The current working directory will be set to "/"
 * relative to the sandbox.
 *
 * @note Kills the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
void proc_ConfineProcInSandbox
(
    const char* sandboxRootPtr, ///< [IN] Path to the sandbox root.
    uid_t uid,                  ///< [IN] The user ID the process should be set to.
    gid_t gid,                  ///< [IN] The group ID the process should be set to.
    const gid_t* groupsPtr,     ///< [IN] List of supplementary groups for this process.
    size_t numGroups            ///< [IN] The number of groups in the supplementary groups list.
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
