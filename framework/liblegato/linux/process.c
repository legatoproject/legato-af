/** @file process.c
 *
 * Legato process control functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

/**
 * Write to stderr in an async-thread-safe manner.
 *
 * @param   str String to write.
 */
#define WRITE_ERR(str)                                                                      \
    do                                                                                      \
    {                                                                                       \
        const char  *c;                                                                     \
        int          unused __attribute__((unused));                                        \
        for (c = (str); *c != '\0'; ++c)                                                    \
        {                                                                                   \
            /* Ignore return value, as we can't really do anything useful with it in this   \
             * context.                                                                     \
             */                                                                             \
            unused = write(STDERR_FILENO, c, 1);                                            \
        }                                                                                   \
    } while (0)

/**
 * Close all file descriptors in the given range.
 */
static inline void CloseFrom
(
    int fd,     ///< First file descriptor to close.
    int maxFds  ///< One greater than the last file descriptor to close.
)
{
    int i;

    for (i = fd; i < maxFds; ++i)
    {
        close(i);
    }
}

/**
 * Spawn a new process from a given executable.
 *
 * @return
 *      -   0 if the process was spawned in detached mode.
 *      -   Greater than 0 if the process was spawned but not detached.  The returned value is the
 *          process ID in this case.
 *      -   Less than zero if an error occurred.
 */
pid_t le_proc_Execute
(
    const le_proc_Parameters_t *paramPtr    ///< Properties of the process to spawn.
)
{
    char        *argArray[2] = { NULL, NULL };
    char        *envArray[1] = { NULL };
    char *const *argumentsPtr;
    char *const *environmentPtr;
    int          maxFds = LE_PROC_NO_FDS;
    int          status = 0;
    pid_t        pid;

    LE_ASSERT(paramPtr != NULL);
    LE_ASSERT(paramPtr->executableStr != NULL);

    // Initialize defaults for parameters.
    if (paramPtr->argumentsPtr == NULL)
    {
        argArray[0] = (char *) paramPtr->executableStr;
        argumentsPtr = argArray;
    }
    else
    {
        argumentsPtr = paramPtr->argumentsPtr;
    }

    if (paramPtr->environmentPtr == NULL)
    {
        environmentPtr = envArray;
    }
    else
    {
        environmentPtr = paramPtr->environmentPtr;
    }

    if (paramPtr->closeFds > LE_PROC_NO_FDS)
    {
        maxFds = sysconf(_SC_OPEN_MAX);
    }

    // NOTE: We can only use async-signal-safe functions from fork() until execve() is called.  Here
    //       we use fork(), write(), and _exit() which are safe according to the POSIX.1-2001
    //       standard.

    pid = fork();
    if (pid < 0)
    {
        LE_ERROR("Failed to fork child process for '%s': error %d",
            paramPtr->executableStr, errno);
        return -1;
    }

    if (paramPtr->detach)
    {
        if (pid > 0)
        {
            // Ensure transitional process doesn't leave a zombie.  We can wait because it is very
            // short-lived.
            LE_INFO("Executing '%s' [detached]", paramPtr->executableStr);
            waitpid(pid, &status, 0);
            return 0;
        }

        CloseFrom(paramPtr->closeFds, maxFds);

        // Double fork to avoid creating a zombie.
        pid = fork();
        if (pid < 0)
        {
            WRITE_ERR("le_proc_Execute(): Failed to double fork for '");
            WRITE_ERR(paramPtr->executableStr);
            WRITE_ERR("'\n");
            _exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (pid > 0)
        {
            LE_INFO("Executing '%s'", paramPtr->executableStr);
            return pid;
        }

        CloseFrom(paramPtr->closeFds, maxFds);
    }

    // Execute initialization function provided by the caller, if any.
    if (paramPtr->init != NULL)
    {
        paramPtr->init(paramPtr);
    }

    // Exec the new process.
    if (execve(paramPtr->executableStr, argumentsPtr, environmentPtr) != 0)
    {
        WRITE_ERR("le_proc_Execute(): Failed to execute '");
        WRITE_ERR(paramPtr->executableStr);
        WRITE_ERR("'\n");
        _exit(EXIT_FAILURE);
    }

    // Should never get here.
    WRITE_ERR("le_proc_Execute(): execve() returned success?!\n");
    _exit(EXIT_FAILURE);
}
