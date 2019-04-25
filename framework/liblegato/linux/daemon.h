//--------------------------------------------------------------------------------------------------
/** @file daemon.h
 *
 * Process daemonization function.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DAEMON_H_INCLUDE_GUARD
#define LEGATO_DAEMON_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Daemonizes the calling process.
 *
 * This function only returns in the child process. In the parent, it waits until the
 * child process its stdin, then terminates itself with a 0 (EXIT_SUCCESS) exit code.
 *
 * The child process should use code like the following to close stdin and reopen it to /dev/null
 * when it wants the parent process to exit.
 *
 * @code
 *
 * LE_FATAL_IF(freopen("/dev/null", "r", stdin) == NULL,
 *             "Failed to redirect stdin to /dev/null.  %m.");
 *
 * @endcode
 *
 * If anything goes wrong, a fatal error is logged and the process exits with EXIT_FAILURE.
 *
 * If the parent times out, it logs a warning message and exits with EXIT_SUCCESS.
 */
//--------------------------------------------------------------------------------------------------
void daemon_Daemonize
(
    uint timeoutMs  ///< Max time to wait for child to close its stdin (in millisecs). -1 = forever.
);


#endif // LEGATO_DAEMON_H_INCLUDE_GUARD
