//--------------------------------------------------------------------------------------------------
/** @file supervisor/wait.h
 *
 * API for waiting/reaping children processes.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#ifndef LEGATO_SRC_WAIT_INCLUDE_GUARD
#define LEGATO_SRC_WAIT_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Gets the pid of any child that is in a waitable state without reaping the child process.
 *
 * @return
 *      The pid of the waitable process if successful.
 *      0 if there are currently no waitable children.
 */
//--------------------------------------------------------------------------------------------------
pid_t wait_Peek
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Reap a specific child.  The child must be in a waitable state.
 *
 * @note This function does not return on error.
 *
 * @return
 *      The status of the reaped child.
 */
//--------------------------------------------------------------------------------------------------
int wait_ReapChild
(
    pid_t pid                       ///< [IN] Pid of the child to reap.
);


#endif  // LEGATO_SRC_WAIT_INCLUDE_GUARD
