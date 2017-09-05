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
 * Sets the resource limits for the specified process.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t resLim_SetProcLimits
(
    proc_Ref_t procRef              ///< [IN] The process to set resource limits for.
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
