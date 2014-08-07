//--------------------------------------------------------------------------------------------------
/**
 * Definitions needed by the parser's internals.  Not to be shared outside the parser.
 *
 * @warning THIS FILE IS INCLUDED FROM C CODE.
 *
 * Copyright (C) 2013-2014 Sierra Wireless Inc., Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef APPLICATION_PARSER_H_INCLUDE_GUARD
#define APPLICATION_PARSER_H_INCLUDE_GUARD


// Maximum number of errors that will be reported before stopping the parsing.
// Note: this is an arbitrary number.
#define AYY_MAX_ERROR_COUNT 5


//--------------------------------------------------------------------------------------------------
/**
 * Application parsing function, implemented by the Bison-generated parser.
 *
 * @note    ayy_parse() may return a non-zero value before parsing has finished, because an error
 *          was encountered.  To aide in troubleshooting, ayy_parse() can be called again to look
 *          for further errors.  However, there's no point in doing this if the end-of-file has
 *          been reached.  ayy_EndOfFile can be used to check for end of file.
 *
 * @return 0 when parsing successfully completes.  Non-zero when an error is encountered.
 */
//--------------------------------------------------------------------------------------------------
int ayy_parse(void);


//--------------------------------------------------------------------------------------------------
/**
 * End of file flag.  This is set when cyy_parse() returns due to an end of file condition.
 */
//--------------------------------------------------------------------------------------------------
extern int ayy_EndOfFile;


//--------------------------------------------------------------------------------------------------
/**
 * Count of the number of errors that have been reported during scanning.
 */
//--------------------------------------------------------------------------------------------------
extern size_t ayy_ErrorCount;


//--------------------------------------------------------------------------------------------------
/**
 * Name of the file that is currently being parsed.
 */
//--------------------------------------------------------------------------------------------------
extern const char* ayy_FileName;


//--------------------------------------------------------------------------------------------------
/**
 * Non-zero if verbose operation is requested.
 */
//--------------------------------------------------------------------------------------------------
extern int ayy_IsVerbose;


//--------------------------------------------------------------------------------------------------
/**
 * The standard error reporting function that will be called by both the
 * lexer and the parser when they detect errors or when the YYERROR macro is invoked.
 */
//--------------------------------------------------------------------------------------------------
void ayy_error(const char* errorString);


//--------------------------------------------------------------------------------------------------
/**
 * Set the application version.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetVersion
(
    const char* version         ///< The version string.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the application sandboxed or unsandboxed ("true" if sandboxed or "false" if unsandboxed).
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetSandboxed
(
    const char* mode    ///< The mode string.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the application start-up mode ("manual" or "auto"; default is "auto").
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetStartMode
(
    const char* mode    ///< The mode string.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a component to the list of components used by this application.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddComponent
(
    const char* name,   ///< Name of the component (or "" if the name should be derived
                        ///   from the file system path).
    const char* path    ///< File system path of the component.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a group name to the list of groups that this application's user should be a member of.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddGroup
(
    const char* name    ///< Name of the group.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a component to the list of components used by this application.
 */
//--------------------------------------------------------------------------------------------------
void ayy_FinishProcessesSection
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a file or directory from the build host file system to the application.
 *
 * If the sourcePath ends in a '/', then it is treated as a directory.
 * Otherwise, it is treated as a file.
 *
 * @warning This function is deprecated.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddFile
(
    const char* permissions,///< String representing the permissions.
    const char* sourcePath, ///< The file path in the build host's file system.
    const char* destPath    ///< The file path in the target file system, inside sandbox.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a file from the build host file system to the application.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddBundledFile
(
    const char* permissions,///< String representing the permissions.
    const char* sourcePath, ///< The file path in the build host's file system.
    const char* destPath    ///< The file path in the target file system, inside sandbox.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a directory from the build host file system to the application.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddBundledDir
(
    const char* sourcePath, ///< The path in the build host's file system.
    const char* destPath    ///< The path in the target file system, inside sandbox.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a new executable to the app.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddExecutable
(
    const char* exePath     ///< File system path relative to executables output directory.
);



//--------------------------------------------------------------------------------------------------
/**
 * Add an item of content to the executable that is currently being parsed.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddExeContent
(
    const char* contentName
);


//--------------------------------------------------------------------------------------------------
/**
 * Called when parsing of an executable specification finishes.
 */
//--------------------------------------------------------------------------------------------------
void ayy_FinalizeExecutable
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the path to the executable that is to be used to start the process.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_SetProcessExe
(
    const char* path
);



//--------------------------------------------------------------------------------------------------
/**
 * Wrap-up the processing of a "run:" subsection in the "processes:" section.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_FinalizeProcess
(
    const char* name    ///< The process name, or NULL if the exe name should be used.
);



//--------------------------------------------------------------------------------------------------
/**
 * Adds a command-line argument to a process.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_AddProcessArg
(
    const char* arg
);



//--------------------------------------------------------------------------------------------------
/**
 * Adds an environment variable to the Process Environment object associated with the processes:
 * section that is currently being parsed.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_AddEnvVar
(
    const char* name,
    const char* value
);



//--------------------------------------------------------------------------------------------------
/**
 * Add a file import mapping, which is the mapping of a non-directory object from the file system
 * somewhere outside the application sandbox to somewhere in the file system inside the application
 * sandbox.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_AddRequiredFile
(
    const char* permissions,///< String representing the permissions. (e.g., "[r]")
    const char* sourcePath, ///< The path in the target file system., outside the sandbox.
    const char* destPath    ///< The path in the target file system, inside the sandbox.
);



//--------------------------------------------------------------------------------------------------
/**
 * Add a directory import mapping, which is the mapping of a directory object from the file system
 * somewhere outside the application sandbox to somewhere in the file system inside the application
 * sandbox.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_AddRequiredDir
(
    const char* permissions,///< String representing the permissions. (e.g., "[r]")
    const char* sourcePath, ///< The path in the target file system., outside the sandbox.
    const char* destPath    ///< The path in the target file system, inside the sandbox.
);



//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of processes that this application is allowed to have running at any
 * given time.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetNumProcsLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
);



//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of bytes that can be allocated for POSIX MQueues for all processes
 * in this application at any given time.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetMqueueSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
);



//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of real-time signals that are allowed to be queued-up waiting for
 * processes in this application at any given time.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetRTSignalQueueSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
);



//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum amount of memory (in kilobytes) the application is allowed to use for all of it
 * processes.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetMemoryLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
);



//--------------------------------------------------------------------------------------------------
/**
 * Sets the cpu share for all processes in the application.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetCpuShare
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
);



//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum amount of RAM (in bytes) that the application is allowed to consume through
 * usage of its temporary sandbox file system.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetFileSystemSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
);



//--------------------------------------------------------------------------------------------------
/**
 * Sets the starting (and maximum) priority level of processes in the current processes section.
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
void ayy_SetPriority
(
    const char* priority
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum size (in bytes) of the core dump file that a process in the current processes
 * section can generate.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetCoreFileSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum size (in bytes) that a process in the current processes section can make files.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetMaxFileSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum size (in bytes) that a process in this processes section is allowed to lock
 * into physical memory.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetMemLockSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
                ///  Will be rounded down to the nearest system memory page size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of file descriptor that each process in the processes section are
 * allowed to have open at one time.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetNumFdsLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the action that should be taken if a process in the process group currently being parsed
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
void ayy_SetFaultAction
(
    const char* action
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the action that should be taken if a process in the process group currently being parsed
 * terminates due to a watchdog time-out.
 *
 * @todo Implement watchdog actions.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetWatchdogAction
(
    const char* action  ///< Accepted actions are TBD.
);

//--------------------------------------------------------------------------------------------------
/**
 * Sets the timeout for the watchdogs in the application.
 *
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetWatchdogTimeout
(
    const int timeout  ///< The time in milliseconds after which the watchdog expires if not
                       ///< kicked again before then. Only positive values are allowed.
);

//--------------------------------------------------------------------------------------------------
/**
 * Disables the watchdog timeout in the application if the timeout value is "never".
 *
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetWatchdogDisabled
(
    const char* never  ///< The only acceptable string in "never"
                       ///< Any other will leave the watchdog enabled with default timeout
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the size of a pool.
 *
 * The pool name is expected to be of the form "process.component.pool".
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetPoolSize
(
    const char* poolName,
    int numBlocks
);


//--------------------------------------------------------------------------------------------------
/**
 * Mark a client-side IPC API interface as an external interface that can be bound to other
 * apps or users using a given interface name.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddRequiredApi
(
    const char* externalAlias, ///< [in] Name to use outside app (NULL = use client interface name).
    const char* clientInterfaceSpec  ///< [in] Client-side interface (exe.component.interface).
);


//--------------------------------------------------------------------------------------------------
/**
 * Mark a server-side IPC API interface as an external interface that other apps or users
 * can bind to using a given interface name.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddProvidedApi
(
    const char* externalAlias, ///< [in] Name to use outside app (NULL = use server interface name).
    const char* clientInterfaceSpec  ///< [in] Client-side interface (exe.component.interface).
);


//--------------------------------------------------------------------------------------------------
/**
 * Create an IPC API binding.
 *
 * The client interface specification is always expected to be of the form
 * "exe.component.interface".
 *
 * The server interface specification can be one of the following:
 * - "exe.component.interface" = an internal bind (within the app).
 * - "app.service" = an external bind to a service provided by an application.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddBind
(
    const char* clientInterfaceSpec,    ///< [in] Client-side interface.
    const char* serverInterfaceSpec     ///< [in] Server-side interface.
);


//--------------------------------------------------------------------------------------------------
/**
 * Create an IPC API binding to a service offered by a user.
 *
 * The client interface specification is expected to be of the form
 * "exe.component.interface".
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddBindOutToUser
(
    const char* clientInterfaceSpec,    ///< [in] Client-side interface.
    const char* serverUserName,         ///< [in] Server-side user name.
    const char* serverServiceName       ///< [in] Server-side service name.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add read access permission for a particular configuration tree.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_AddConfigTreeAccess
(
    const char* permissions,    ///< [in] String representing the permissions (e.g., "[r]", "[w]").
    const char* treeName        ///< [in] The name of the configuration tree.
);



#endif // APPLICATION_PARSER_H_INCLUDE_GUARD
