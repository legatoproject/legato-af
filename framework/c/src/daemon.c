//--------------------------------------------------------------------------------------------------
/** @file daemon.c
 *
 * Implementation of process daemonization.
 *
 * Copyright (C), Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "daemon.h"
#include "fileDescriptor.h"

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
)
{
    // Create a pipe to use to synchronize the parent and the child.
    int syncPipeFd[2];
    LE_FATAL_IF(pipe(syncPipeFd) != 0, "Could not create synchronization pipe.  %m.");

    // Fork off the parent process.
    pid_t pid = fork();

    LE_FATAL_IF(pid < 0, "Failed to fork when daemonizing the supervisor.  %m.");

    // If we got a non-zero PID, we are the parent process.
    if (pid > 0)
    {
        // The parent does not need the write end of the pipe so close it.
        fd_Close(syncPipeFd[1]);

        // Block until the pipe closes (with timeout).
        struct pollfd waitList;
        waitList.fd = syncPipeFd[0];
        waitList.events = 0;
        waitList.revents = 0;
        int pollResult = poll(&waitList, 1, timeoutMs);
        switch (pollResult)
        {
            case -1:    // Failed.

                LE_FATAL("poll() failed: %m");

            case 0:     // Timeout

                LE_WARN("Timed out after waiting %u ms for indication from child.",
                        timeoutMs);
                exit(EXIT_SUCCESS);

            case 1:     // Child closed their end of the pipe.

                exit(EXIT_SUCCESS);

            default:

                LE_FATAL("Unexpected result (%d) from poll().", pollResult);
        }
    }

    // Only the child gets here.

    // The child does not need the read end of the pipe so close it.
    fd_Close(syncPipeFd[0]);

    // Move the write end of the pipe to stdin, to be closed when the framework is ready for use.
    // Note: This closes stdin and replaces it with the pipe fd.
    while (dup2(syncPipeFd[1], 0) == -1)
    {
        if (errno != EINTR)
        {
            LE_FATAL("dup2(%d, %d) failed: %m", syncPipeFd[1], 0);
        }
    }
    fd_Close(syncPipeFd[1]);

    // Start a new session and become the session leader, the process group leader which will free
    // us from any controlling terminals.
    LE_FATAL_IF(setsid() == -1, "Could not start a new session.  %m.");

    // Reset the file mode mask.
    umask(0);

    // Change the current working directory to the root filesystem, to ensure that it doesn't tie
    // up another filesystem and prevent it from being unmounted.
    LE_FATAL_IF(chdir("/") < 0, "Failed to set working directory to root.  %m.");

    // Redirect stderr to /dev/console.
    if (freopen("/dev/console", "w", stderr) == NULL)
    {
        LE_WARN("Could not redirect stderr to /dev/console, redirecting it to /dev/null instead.");

        LE_FATAL_IF(freopen("/dev/null", "w", stderr) == NULL,
                    "Failed to redirect stderr to /dev/null.  %m.");
    }

    // Redirect stdout to /dev/null.
    LE_FATAL_IF( (freopen("/dev/null", "w", stdout) == NULL),
                "Failed to redirect stdout to /dev/null.  %m.");
}
