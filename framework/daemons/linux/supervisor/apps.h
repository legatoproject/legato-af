//--------------------------------------------------------------------------------------------------
/** @file supervisor/apps.h
 *
 * API for managing all Legato applications.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#ifndef LEGATO_SRC_APPS_INCLUDE_GUARD
#define LEGATO_SRC_APPS_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for applications shutdown handler.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*apps_ShutdownHandler_t)
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the applications system.
 */
//--------------------------------------------------------------------------------------------------
void apps_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Start all applications marked as 'auto' start.
 */
//--------------------------------------------------------------------------------------------------
void apps_AutoStart
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Initiates the shut down of all the applications.  The shut down sequence happens asynchronously.
 * A shut down handler should be set using apps_SetShutdownHandler() to be
 * notified when all applications actually shut down.
 */
//--------------------------------------------------------------------------------------------------
void apps_Shutdown
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the shutdown handler to be called when all the applications have shutdown.
 */
//--------------------------------------------------------------------------------------------------
void apps_SetShutdownHandler
(
    apps_ShutdownHandler_t shutdownHandler      ///< [IN] Shut down handler.  Can be NULL.
);


//--------------------------------------------------------------------------------------------------
/**
 * The SIGCHLD handler for the applications.  This should be called from the Supervisor's SIGCHILD
 * handler.
 *
 * @note
 *      This function will reap the child if the child is an application process, otherwise the
 *      child will remain unreaped.
 *
 * @return
 *      LE_OK if the signal was handled without incident.
 *      LE_NOT_FOUND if the pid is not an application process.  The child will not be reaped.
 *      LE_FAULT if the signal indicates a failure of one of the applications which requires a
 *               system restart.
 */
//--------------------------------------------------------------------------------------------------
le_result_t apps_SigChildHandler
(
    pid_t pid               ///< [IN] Pid of the process that produced the SIGCHLD.
);


//--------------------------------------------------------------------------------------------------
/**
 * Verify that all devices in our sandboxed applications match with the device outside the sandbox.
 * Remove devices and allow supervisor to recreate them.
 */
//--------------------------------------------------------------------------------------------------
void apps_VerifyAppWriteableDeviceFiles
(
    void
);


#endif  // LEGATO_SRC_APPS_INCLUDE_GUARD
