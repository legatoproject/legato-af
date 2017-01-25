//--------------------------------------------------------------------------------------------------
/** @file supervisor/wait.c
 *
 * API wrapper for wait() system calls.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "wait.h"

//--------------------------------------------------------------------------------------------------
/**
 * Gets the pid of any child that is in a waitable state without reaping the child process.
 *
 * @note
 *      This function does not block.
 *
 * @return
 *      The pid of the waitable process if successful.
 *      0 if there are currently no waitable children.
 */
//--------------------------------------------------------------------------------------------------
pid_t wait_Peek
(
    void
)
{
    siginfo_t childInfo = {.si_pid = 0};

    int result;

    do
    {
        result = waitid(P_ALL, 0, &childInfo, WEXITED | WNOHANG | WNOWAIT);
    }
    while ( (result == -1) && (errno == EINTR) );

    LE_FATAL_IF(result == -1, "%m.")

    return childInfo.si_pid;
}


//--------------------------------------------------------------------------------------------------
/**
 * Reap a specific child.  The child must be in a waitable state.
 *
 * @note
 *      This function does not return on error.
 *      This function does not block.
 *
 * @return
 *      The status of the reaped child.
 */
//--------------------------------------------------------------------------------------------------
int wait_ReapChild
(
    pid_t pid                       ///< [IN] Pid of the child to reap.
)
{
    pid_t resultPid;
    int status;

    do
    {
        resultPid = waitpid(pid, &status, WNOHANG);
    }
    while ( (resultPid == -1) && (errno == EINTR) );

    LE_FATAL_IF(resultPid == -1, "%m.");

    LE_FATAL_IF(resultPid == 0, "Could not reap child %d.", pid);

    return status;
}
