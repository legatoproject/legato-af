//--------------------------------------------------------------------------------------------------
/**
 * Definitions needed by the parser's internals.  Not to be shared outside the parser.
 *
 * @warning THIS FILE IS INCLUDED FROM C CODE.
 *
 * Copyright (C) 2013 Sierra Wireless Inc., all rights reserved.
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
 * Restarts the application parsing with a new file stream to parse.
 */
//--------------------------------------------------------------------------------------------------
void ayy_restart(FILE *new_file);


//--------------------------------------------------------------------------------------------------
/**
 * The standard error reporting function that will be called by both the
 * lexer and the parser when they detect errors or when the YYERROR macro is invoked.
 */
//--------------------------------------------------------------------------------------------------
void ayy_error(const char* errorString);


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
 * Add a file to the application.
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
 * Add a file import mapping, which is the mapping of an object from the file system somewhere
 * outside the application sandbox to somewhere in the file system inside the application sandbox.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_AddFileImport
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
 * Set the maximum number of messages that are allowed to be queued-up in POSIX MQueues waiting for
 * processes in this application at any given time.
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
 * Sets the maximum size (in bytes) that a process in the current processes section can make
 * its virtual address space.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetVMemSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
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
 * Create an IPC binding between one interface and another.
 *
 * Each interface definition is expected to be of the form "proc.interface".  One of the
 * two interfaces must be an imported interface, while the other must be an exported interface.
 */
//--------------------------------------------------------------------------------------------------
void ayy_CreateBind
(
    const char* importSpecifier,
    const char* exportSpecifier
);


#endif // APPLICATION_PARSER_H_INCLUDE_GUARD
