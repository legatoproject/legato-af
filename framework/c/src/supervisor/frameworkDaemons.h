//--------------------------------------------------------------------------------------------------
/** @file frameworkDaemons.h
 *
 * API for managing Legato framework daemons.  The framework daemons include the Service Directory,
 * Log Control, Configuration Tree and Watchdog.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#ifndef LEGATO_SRC_FRAMEWORK_DAEMON_INCLUDE_GUARD
#define LEGATO_SRC_FRAMEWORK_DAEMON_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for framework daemon shutdown handler.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*fwDaemons_ShutdownHandler_t)
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Start all the framework daemons.
 */
//--------------------------------------------------------------------------------------------------
void fwDaemons_Start
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Initiates the shut down of all the framework daemons.  The shut down sequence happens
 * asynchronously.  A shut down handler should be set using fwDaemons_SetShutdownHandler() to be
 * notified when all framework daemons actually shut down.
 */
//--------------------------------------------------------------------------------------------------
void fwDaemons_Shutdown
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the shutdown handler to be called when all the framework daemons shutdown.
 */
//--------------------------------------------------------------------------------------------------
void fwDaemons_SetShutdownHandler
(
    fwDaemons_ShutdownHandler_t shutdownHandler
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the intermediate shutdown handler to be called when all the framework daemons shutdown
 * except for the Service Directory.  This gives the caller a chance to do some message handling
 * before the Service Directory is shutdown as well.
 *
 * @note The Service Directory is the last framework daemon to shutdown.
 */
//--------------------------------------------------------------------------------------------------
void fwDaemons_SetIntermediateShutdownHandler
(
    fwDaemons_ShutdownHandler_t shutdownHandler
);


//--------------------------------------------------------------------------------------------------
/**
 * The SIGCHLD handler for the framework daemons.  This should be called from the Supervisor's
 * SIGCHILD handler.
 *
 * @note
 *      This function will reap the child if the child is a framework daemon, otherwise the child
 *      will remain unreaped.
 *
 * @return
 *      LE_OK if the signal was handled without incident.
 *      LE_NOT_FOUND if the pid is not a framework daemon.  The child will not be reaped.
 *      LE_FAULT if the signal indicates the failure of one of the framework daemons.
 */
//--------------------------------------------------------------------------------------------------
le_result_t fwDaemons_SigChildHandler
(
    pid_t pid               ///< [IN] Pid of the process that produced the SIGCHLD.
);


#endif // LEGATO_SRC_FRAMEWORK_DAEMON_INCLUDE_GUARD
