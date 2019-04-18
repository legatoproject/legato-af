/** @page c_process Process API
 *
 * @subpage le_process.h "API Reference"
 *
 * <HR>
 * This API provides a means to spawn external processes.  The function le_proc_Execute() takes a
 * structure which is populated with the execution parameters and handles the heavy lifting of (on
 * Linux) forking and execing as necessary.
 *
 *
 * @code{.c}
 * int status;
 * le_proc_Parameters_t proc =
 * {
 *     .executableStr   = "/bin/ls",
 *     .argumentsPtr    = { "/bin/ls", "/tmp", NULL },
 *     .environmentPtr  = NULL,
 *     .detach          = false,
 *     .closeFds        = LE_PROC_NO_FDS,
 *     .init            = NULL,
 *     .userPtr         = NULL
 * };
 *
 * pid_t pid = le_proc_Execute(&proc);
 * if (pid < 0)
 * {
 *     LE_FATAL("Oh no!  Something went wrong (error %d).", errno);
 * }
 * if (waitpid(pid, &status, 0) > 0)
 * {
 *     LE_INFO("%s[%d] returned %d", proc.executableStr, (int) pid, status);
 * }
 * @endcode
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

/** @file le_process.h
 *
 * Legato @ref c_process include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PROCESS_INCLUDE_GUARD
#define LEGATO_PROCESS_INCLUDE_GUARD

/** Value representing no file descriptors. */
#define LE_PROC_NO_FDS  -1

/** Parameters specifying how to spawn an external process. */
typedef struct le_proc_Parameters
{
    const char  *executableStr;     ///< Path to the file to execute.
    char *const *argumentsPtr;      ///< NULL-terminated array of arguments to pass to the process.
                                    ///< May be NULL.
    char *const *environmentPtr;    ///< NULL-terminated array of "var=value" strings to set in the
                                    ///< environment of the process.  May be NULL.
    bool         detach;            ///< "Detach" the process so that a call to le_proc_Wait() is
                                    ///< not required to prevent it from becoming a zombie.
    int          closeFds;          ///< Close all open file descriptors in the child at or above
                                    ///< the given value.  Set to LE_PROC_NO_FDS to avoid closing
                                    ///< any.
    void         (*init)(const struct le_proc_Parameters *paramPtr);    ///< Custom function to run
                                    ///< right before the executable is exec'd.  Note that only
                                    ///< async-signal-safe functions may be called from this
                                    ///< callback.
    void        *userPtr;           ///< Arbitrary data to include with the parameters.  Only useful
                                    ///< if init() is also set.
} le_proc_Parameters_t;

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
);

#endif /* end LEGATO_PROCESS_INCLUDE_GUARD */
