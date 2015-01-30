//--------------------------------------------------------------------------------------------------
/**
 * Definitions needed by the parser's internals.  Not to be shared outside the parser.
 *
 * @warning THIS FILE IS INCLUDED FROM C CODE.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef SYSTEM_PARSER_H_INCLUDE_GUARD
#define SYSTEM_PARSER_H_INCLUDE_GUARD


// Maximum number of errors that will be reported before stopping the parsing.
// Note: this is an arbitrary number.
#define SYY_MAX_ERROR_COUNT 5


//--------------------------------------------------------------------------------------------------
/**
 * System parsing function, implemented by the Bison-generated parser.
 *
 * @note    syy_parse() may return a non-zero value before parsing has finished, because an error
 *          was encountered.  To aide in troubleshooting, syy_parse() can be called again to look
 *          for further errors.  However, there's no point in doing this if the end-of-file has
 *          been reached.  syy_EndOfFile can be used to check for end of file.
 *
 * @return 0 when parsing successfully completes.  Non-zero when an error is encountered.
 */
//--------------------------------------------------------------------------------------------------
int syy_parse(void);


//--------------------------------------------------------------------------------------------------
/**
 * End of file flag.  This is set when syy_parse() returns due to an end of file condition.
 */
//--------------------------------------------------------------------------------------------------
extern int syy_EndOfFile;


//--------------------------------------------------------------------------------------------------
/**
 * Count of the number of errors that have been reported during scanning.
 */
//--------------------------------------------------------------------------------------------------
extern size_t syy_ErrorCount;


//--------------------------------------------------------------------------------------------------
/**
 * Name of the file that is currently being parsed.
 */
//--------------------------------------------------------------------------------------------------
extern const char* syy_FileName;


//--------------------------------------------------------------------------------------------------
/**
 * Non-zero if verbose operation is requested.
 */
//--------------------------------------------------------------------------------------------------
extern int syy_IsVerbose;


//--------------------------------------------------------------------------------------------------
/**
 * The standard error reporting function that will be called by both the
 * lexer and the parser when they detect errors or when the YYERROR macro is invoked.
 */
//--------------------------------------------------------------------------------------------------
void syy_error(const char* errorString);


//--------------------------------------------------------------------------------------------------
/**
 * Set the system version.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetVersion
(
    const char* version         ///< The version string.
);


//--------------------------------------------------------------------------------------------------
/**
 * Adds an application to the system, making it the "current application".
 */
//--------------------------------------------------------------------------------------------------
void syy_AddApp
(
    const char* adefPath       ///< File system path to the application's .adef file.
);


//--------------------------------------------------------------------------------------------------
/**
 * Closes processing of an "app:" section.
 */
//--------------------------------------------------------------------------------------------------
void syy_FinalizeApp
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the current application sandboxed or unsandboxed ("true" if sandboxed or "false" if
 * unsandboxed).
 */
//--------------------------------------------------------------------------------------------------
void syy_SetSandboxed
(
    const char* mode    ///< The mode string.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the current application start-up mode ("manual" or "auto"; default is "auto").
 */
//--------------------------------------------------------------------------------------------------
void syy_SetStartMode
(
    const char* mode    ///< The mode string.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a group name to the list of groups that the current application's user should be a member of.
 */
//--------------------------------------------------------------------------------------------------
void syy_AddGroup
(
    const char* name    ///< Name of the group.
);



//--------------------------------------------------------------------------------------------------
/**
 * Clear the list of groups that the current application's user should be a member of.
 */
//--------------------------------------------------------------------------------------------------
void syy_ClearGroups
(
    void
);



//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of processes that the current application is allowed to have running at
 * Set the maximum number of threads that the current application is allowed to have running at
 * any given time.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxThreads
(
    int limit   ///< Must be a positive integer.
);



//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of bytes that can be allocated for POSIX MQueues for all processes
 * in the current application at any given time.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxMQueueBytes
(
    int limit   ///< Must be a positive integer or zero.
);



//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of signals that are allowed to be queued-up by sigqueue() waiting for
 * processes in the current application at any given time.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxQueuedSignals
(
    int limit   ///< Must be a positive integer.
);



//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum amount of memory (in bytes) the current application is allowed to use for
 * all of its processes.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxMemoryBytes
(
    int limit   ///< Must be a positive integer.
);



//--------------------------------------------------------------------------------------------------
/**
 * Sets the cpu share to be shared by all processes in the current application.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetCpuShare
(
    int limit   ///< Must be a positive integer.
);



//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum amount of RAM (in bytes) that the current application is allowed to consume
 * through usage of its temporary sandbox file system.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxFileSystemBytes
(
    int limit   ///< Must be a positive integer or zero.
);



//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum priority level of processes in the current application.
 * Does not set the starting priority, unless the application's .adef file is trying to start
 * a process at a priority higher than the one specified here, in which case the process's
 * starting priority will be lowered to this level.
 *
 * Allowable values are:
 *   "idle" - intended for very low priority processes that will only get CPU time if there are
 *            no other processes waiting for the CPU.
 *
 *   "low",
 *   "medium",
 *   "high" - intended for normal processes that contend for the CPU.  Processes with these
 *            priorities do not preempt each other but their priorities affect how they are
 *            inserted into the scheduling queue. ie. "high" will get higher priority than
 *            "medium" when inserted into the queue.
 *
 *   "rt1"
 *    ...
 *   "rt32" - intended for (soft) realtime processes. A higher realtime priority will pre-empt
 *            a lower realtime priority (ie. "rt2" would pre-empt "rt1"). Processes with any
 *            realtime priority will pre-empt processes with "high", "medium", "low" and "idle"
 *            (non-real-time) priorities.  Also, note that processes with these realtime priorities
 *            will pre-empt the Legato framework processes so take care to design realtime
 *            processes that relinguishes the CPU appropriately.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxPriority
(
    const char* priority
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum size (in bytes) of the core dump file that any process in the current
 * application can generate.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxCoreDumpFileBytes
(
    int limit   ///< Must be a positive integer or zero.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum size (in bytes) that any process in the current application can make files.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxFileBytes
(
    int limit   ///< Must be a positive integer or zero.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum size (in bytes) that any process in the current application is allowed to lock
 * into physical memory.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxLockedMemoryBytes
(
    int limit   ///< Must be a positive integer or zero.
                ///  Will be rounded down to the nearest system memory page size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of file descriptors that each process in the current application are
 * allowed to have open at one time.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxFileDescriptors
(
    int limit   ///< Must be a positive integer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the action that should be taken if any process in the current application
 * terminates with a non-zero exit code (i.e., any error code other than EXIT_SUCCESS).
 *
 * Accepted actions are:
 *  "ignore"        - Leave the process dead.
 *  "restart"       - Restart the process.
 *  "restartApp"    - Terminate and restart the whole application.
 *  "stopApp"       - Terminate the application and leave it stopped.
 *  "reboot"        - Reboot the device.
 *  "pauseApp"      - Send a SIGSTOP to all processes in the application, halting them in their
 *                    tracks, but not killing them.  This allows the processes to be inspected
 *                    for debugging purposes.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetFaultAction
(
    const char* action
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the action that should be taken if any process in the current application
 * terminates due to a watchdog time-out.
 *
 * Accepted actions are:
 *  "ignore"        - Leave the process dead.
 *  "restart"       - Restart the process.
 *  "stop"          - Terminate the process if it is still running.
 *  "restartApp"    - Terminate and restart the whole application.
 *  "stopApp"       - Terminate the application and leave it stopped.
 *  "reboot"        - Reboot the device.
 *  "pauseApp"      - Send a SIGSTOP to all processes in the application, halting them in their
 *                    tracks, but not killing them.  This allows the processes to be inspected
 *                    for debugging purposes.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetWatchdogAction
(
    const char* action
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the timeout for the watchdogs in the current application.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetWatchdogTimeout
(
    const int timeout  ///< The time in milliseconds after which the watchdog expires if not
                       ///< kicked again before then. Only positive values are allowed.
);


//--------------------------------------------------------------------------------------------------
/**
 * Disables the watchdog timeout in the current application.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetWatchdogDisabled
(
    const char* never  ///< The only acceptable string is "never".
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the size of a pool in the current application.
 *
 * The pool name is expected to be of the form "process.component.pool".
 */
//--------------------------------------------------------------------------------------------------
void syy_SetPoolSize
(
    const char* poolName,
    int numBlocks           ///< [in] Must be non-negative (>= 0).
);


//--------------------------------------------------------------------------------------------------
/**
 * Create an IPC API binding between two apps.
 *
 * The both interface specifications are always expected to take the form "app.interface".
 */
//--------------------------------------------------------------------------------------------------
void syy_AddAppToAppBind
(
    const char* clientInterfaceSpec,    ///< [in] Client-side interface.
    const char* serverInterfaceSpec     ///< [in] Server-side interface.
);


//--------------------------------------------------------------------------------------------------
/**
 * Create an IPC API binding from an application's client-side interface to a service offered
 * by a specific user account.
 *
 * The client interface specification is expected to be of the form "app.interface".
 */
//--------------------------------------------------------------------------------------------------
void syy_AddAppToUserBind
(
    const char* clientInterfaceSpec,    ///< [in] Client-side interface.
    const char* serverUserName,         ///< [in] Server-side user account name.
    const char* serverInterfaceName     ///< [in] Server-side interface name.
);


//--------------------------------------------------------------------------------------------------
/**
 * Create an IPC API binding from a specific user account's client-side interface to a service
 * offered by an application.
 *
 * The server interface specification is expected to be of the form "app.interface".
 */
//--------------------------------------------------------------------------------------------------
void syy_AddUserToAppBind
(
    const char* clientUserName,         ///< [in] Client-side user account name.
    const char* clientInterfaceName,    ///< [in] Client-side interface name.
    const char* serverInterfaceSpec     ///< [in] Server-side interface.
);


//--------------------------------------------------------------------------------------------------
/**
 * Create an IPC API binding from a specific user account's client-side interface to a specific
 * user account's server-side interface.
 */
//--------------------------------------------------------------------------------------------------
void syy_AddUserToUserBind
(
    const char* clientUserName,         ///< [in] Client-side user account name.
    const char* clientInterfaceName,    ///< [in] Client-side interface name.
    const char* serverUserName,         ///< [in] Server-side user account name.
    const char* serverInterfaceName     ///< [in] Server-side interface name.
);



#endif // SYSTEM_PARSER_H_INCLUDE_GUARD
