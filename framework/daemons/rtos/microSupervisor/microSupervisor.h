//--------------------------------------------------------------------------------------------------
/** @file microSupervisor/microSupervisor.h
 *
 * This is the Micro Supervisor control structure definitions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_SRC_MICROSUPERVISOR_INCLUDE_GUARD
#define LEGATO_SRC_MICROSUPERVISOR_INCLUDE_GUARD

/// Maximum allowed number of command line arguments
#define MAX_ARGC (1 + 2 * LE_CONFIG_MAX_ARG_OPTIONS + LE_CONFIG_MAX_ARG_POSITIONAL_CALLBACKS)

/**
 * Definitions for tasks managed by the microsupervisor.
 *
 * Each task corresponds to a single process in an app definition.
 */
typedef struct
{
    const char               *nameStr;      /// Task name -- derived from the process name
    le_thread_Priority_t      priority;     /// Task default priority
    size_t                    stackSize;    /// Task stack size.
    void                     *stackPtr;
    le_thread_MainFunc_t      entryPoint;   /// Task entry point function.
    int                       defaultArgc;  /// Default number of arguments
    const char              **defaultArgv;  /// Default argument list
}
Task_t;

/**
 * Define runtime data for a task
 */
typedef struct
{
    le_thread_Ref_t  threadRef;             ///< Thread reference
    int              argc;                  ///< Number of arguments
    const char      *argv[MAX_ARGC + 1];    ///< Argument list
    char            *cmdlinePtr;            ///< Pointer to buffer of strings for command line
                                            ///< arguments.
}
TaskInfo_t;

/**
 * Definitions for apps managed by the microsupervisor.
 */
typedef struct
{
    const char      *appNameStr;    ///< Application name
    bool             manualStart;   ///< If this app should not be started on system start.
    uint32_t         taskCount;     ///< Number of tasks in this application
    const Task_t    *taskList;      ///< Pointer to array of task definitions for this application.
    TaskInfo_t      *threadList;    ///< Pointer to array of task threads for this application.
                                    ///< For running applications this array is the same size as
                                    ///< taskList.  For non-running applications it may be NULL.
}
App_t;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Internal functions
//
// For use only in microSupervisor or related components (e.g. appInfo app).
//

//--------------------------------------------------------------------------------------------------
/**
 * Find an app, given an app name.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED const App_t* microSupervisor_FindApp
(
    const char* appNameStr       ///< Application name
);


//--------------------------------------------------------------------------------------------------
/**
 * Find a task in an app, given the app reference and and the task name
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED const Task_t* microSupervisor_FindTask
(
    const App_t* appPtr,                 ///< [IN] pointer to the app
    const char* procNameStr              ///< [IN] task name
);


//--------------------------------------------------------------------------------------------------
/**
 * Check if an app is running.
 *
 * An app is running if at least one process in the app is running.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED bool microSupervisor_IsAppRunning
(
    const App_t* appPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Check if a task is running.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED bool microSupervisor_IsTaskRunning
(
    const App_t* appPtr,
    const Task_t* taskPtr
);


////////////////////////////////////////////////////////////////////////////////////////////////////
// External functions
//

//--------------------------------------------------------------------------------------------------
/**
 * Entry point for micro supervisor
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_microSupervisor_Main
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Start a specific app (by name)
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_microSupervisor_StartApp
(
    const char* appNameStr
);


//--------------------------------------------------------------------------------------------------
/**
 * Start a specific process (by name)
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_microSupervisor_RunProc
(
    const char* appNameStr,
    const char* procNameStr,
    int argc,
    const char* argv[]
);

//--------------------------------------------------------------------------------------------------
/**
 * Start a specific process (by name).  The command line is passed as a single string.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_microSupervisor_RunProcStr
(
    const char *appNameStr,     ///< App name.
    const char *procNameStr,    ///< Process name.
    const char *cmdlineStr      ///< Command line arguments.
);

//--------------------------------------------------------------------------------------------------
/**
 * Print app status on serial port (using printf).
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_microSupervisor_DebugAppStatus
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the log level filter
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_microSupervisor_SetLogLevel
(
    le_log_Level_t level     ///< [IN] the filter level to set
);

#endif /* LEGATO_SRC_MICROSUPERVISOR_INCLUDE_GUARD */
