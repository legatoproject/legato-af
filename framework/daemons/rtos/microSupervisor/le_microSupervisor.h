//--------------------------------------------------------------------------------------------------
/** @file microSupervisor/le_microSupervisor.h
 *
 * This is the Micro Supervisor external control structure definitions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_SRC_MICROSUPERVISOR_EXTERN_INCLUDE_GUARD
#define LEGATO_SRC_MICROSUPERVISOR_EXTERN_INCLUDE_GUARD

////////////////////////////////////////////////////////////////////////////////////////////////////
// External functions
//

//--------------------------------------------------------------------------------------------------
/**
 * Get the active run group.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED uint8_t le_microSupervisor_GetActiveRunGroup
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the active run group.
 *
 * If used, must be called before calling le_microSupervisor_Main()
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_microSupervisor_SetActiveRunGroup
(
    uint8_t runGroup
);

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
 * Run a specific command (by name)
 */
//--------------------------------------------------------------------------------------------------
le_result_t LE_SHARED le_microSupervisor_RunCommand
(
    const char* appNameStr,            ///< [IN] App name
    const char* procNameStr,           ///< [IN] Process name
    int argc,                          ///< [IN] Process argument count
    const char* argv[]                 ///< [IN] Process argument list
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
* Get legato version string
*/
//--------------------------------------------------------------------------------------------------
LE_SHARED bool le_microSupervisor_GetLegatoVersion
(
    char*  bufferPtr,        ///< [OUT] output buffer to contain version string
    size_t size              ///< [IN] buffer length
);

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the configured max watchdog timeout if one exists
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int32_t le_microSupervisor_GetMaxWatchdogTimeout
(
    pthread_t threadId ///< [IN] thread to find in the app list
);

//--------------------------------------------------------------------------------------------------
/**
* Returns the configured default watchdog timeout if one exists
*/
//--------------------------------------------------------------------------------------------------
LE_SHARED int32_t le_microSupervisor_GetWatchdogTimeout
(
    pthread_t threadId ///< [IN] thread to find in the app list
);

//--------------------------------------------------------------------------------------------------
/**
* Returns the configured manual start configuration
*/
//--------------------------------------------------------------------------------------------------
LE_SHARED bool le_microSupervisor_GetManualStart
(
    pthread_t threadId ///< [IN] thread to find in the app list
);

#endif /* LEGATO_SRC_MICROSUPERVISOR_EXTERN_INCLUDE_GUARD */
