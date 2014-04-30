/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 * API for communicating with the Supervisor.  This API is only available for users on the
 * Supervisor's access control list.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LE_SUP_INTERFACE_H_INCLUDE_GUARD
#define LE_SUP_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

// User customizable include file
#include "le_supervisor_defs.h"


//--------------------------------------------------------------------------------------------------
/**
 * Start the service for the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_sup_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_sup_StopClient
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Starts an application.
 *
 * @return
 *      LE_OK if the application is successfully started.
 *      LE_DUPLICATE if the application is already running.
 *      LE_NOT_FOUND if the application is not installed.
 *      LE_FAULT if there was an error and the application could not be launched.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sup_StartApp
(
    const char* appName
        ///< [IN]
        ///< The name of the application to start.
);

//--------------------------------------------------------------------------------------------------
/**
 * Stops an application.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the application could not be found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sup_StopApp
(
    const char* appName
        ///< [IN]
        ///< The name of the application to stop.
);

//--------------------------------------------------------------------------------------------------
/**
 * Stops the Legato framework.
 *
 * @return
 *      LE_OK if successful.
 *      LE_DUPLICATE if the framework is in the process of shutting down because someone else has
                     already requested that the framework be stopped.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sup_StopLegato
(
    void
);


#endif // LE_SUP_INTERFACE_H_INCLUDE_GUARD

