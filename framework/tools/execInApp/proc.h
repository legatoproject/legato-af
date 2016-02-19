//--------------------------------------------------------------------------------------------------
/** @file execInApp/proc.h
 *
 * Temporary solution until command apps are available.
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
