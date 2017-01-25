//--------------------------------------------------------------------------------------------------
/** @file killProc.h
 *
 * API for killing processes.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#ifndef LEGATO_SRC_KILLER_INCLUDE_GUARD
#define LEGATO_SRC_KILLER_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the kill API.
 */
//--------------------------------------------------------------------------------------------------
void kill_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Initiates a soft kill of a process.  This sends the process a SIGTERM signal allowing the process
 * to catch the signal and perform a graceful shutdown.  If the process fails to shut down within
 * the timeout period a hard kill (SIGKILL) will be performed.  If the calling process knows that
 * the target process has already died it can call kill_Died() to cancel the hard kill timeout.  The
 * calling process must have privileges to send signals to the process specified by pid.
 *
 * If the process does not exist this function simply returns.
 *
 * @note
 *      Does not return on error.
 */
//--------------------------------------------------------------------------------------------------
void kill_Soft
(
    pid_t pid,                  ///< [IN] Process to kill.
    size_t timeoutMsecs         ///< [IN] Number of milliseconds until a hard kill is sent.
);


//--------------------------------------------------------------------------------------------------
/**
 * Called when a process actually dies.  This should be called when the process actually dies so
 * that an additional hard kill is not attempted.
 */
//--------------------------------------------------------------------------------------------------
void kill_Died
(
    pid_t pid                   ///< [IN] Process that died.
);


//--------------------------------------------------------------------------------------------------
/**
 * Initiates a hard kill to kill the process immediately.  The calling process must have privileges
 * to send signals to the process specified by pid.
 *
 * If the process does not exist this function simply returns.
 *
 * @note
 *      Does not return on error.
 */
//--------------------------------------------------------------------------------------------------
void kill_Hard
(
    pid_t pid                   ///< [IN] Process to kill.
);


//--------------------------------------------------------------------------------------------------
/**
 * Kills processes by name.  Kills all occurrences of a process with the specified name.
 */
//--------------------------------------------------------------------------------------------------
void kill_ByName
(
    const char* procNamePtr     ///< [IN] Name of the processes to kill.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sends a signal to a process.  The calling process must have privileges to send signals to the
 * process specified by pid.
 *
 * If the process does not exist this function simply returns.
 *
 * @note
 *      Does not return on error.
 */
//--------------------------------------------------------------------------------------------------
void kill_SendSig
(
    pid_t pid,                  ///< [IN] Process to send the signal to.
    int sig                     ///< [IN] The signal to send.
);


#endif // LEGATO_SRC_KILLER_INCLUDE_GUARD
