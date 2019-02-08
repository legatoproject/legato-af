//--------------------------------------------------------------------------------------------------
/** @file resourceLimits.h
 *
 * API for setting resource limits.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_SRC_RESOURCE_LIMITS_INCLUDE_GUARD
#define LEGATO_SRC_RESOURCE_LIMITS_INCLUDE_GUARD


#include "app.h"
#include "proc.h"


//--------------------------------------------------------------------------------------------------
/**
 * Process limits controlled by Legato
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int maxCoreDumpFileBytes;
    int maxFileBytes;
    int maxLockedMemoryBytes;
    int maxFileDescriptors;
    int maxStackBytes;
    int maxMQueueBytes;
    int maxThreads;
    int maxQueuedSignals;
} resLim_ProcLimits_t;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the sandboxed application's tmpfs file system limit.
 *
 * @return
 *      The file system limit for the specified application.
 */
//--------------------------------------------------------------------------------------------------
rlim_t resLim_GetSandboxedAppTmpfsLimit
(
    app_Ref_t appRef                ///< [IN] The application to set resource limits for.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the resource limits for the specified application.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t resLim_SetAppLimits
(
    app_Ref_t appRef                ///< [IN] The application to set resource limits for.
);


//--------------------------------------------------------------------------------------------------
/**
 * Read the resource limits from the config tree.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
void resLim_GetProcLimits
(
    proc_Ref_t procRef,             ///< [IN] The process to get resource limits for.
    resLim_ProcLimits_t* limitPtr   ///< [OUT] The limits for the process
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the resource limits for the specified process.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
void resLim_SetProcLimits
(
    resLim_ProcLimits_t* limitPtr ///< [IN] The limits for the process
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a process to the app's cgroups.
 */
//--------------------------------------------------------------------------------------------------
void resLim_SetCGroups
(
    proc_Ref_t procRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Cleans up any resources used to set the resource limits for an application.  This should be
 * called when an app is completely stopped, meaning all processes in the application have been
 * killed.
 */
//--------------------------------------------------------------------------------------------------
void resLim_CleanupApp
(
    app_Ref_t appRef                ///< [IN] Application to clean up resources for.
);


#endif  // LEGATO_SRC_RESOURCE_LIMITS_INCLUDE_GUARD
