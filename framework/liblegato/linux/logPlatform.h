/** @file logPlatform.h
 *
 * Linux-specific intra-framework header file for log system.  This file exposes type definitions
 * and function interfaces to other modules inside the framework implementation.
 *
 * The Log Control Daemon is a server process that everyone else connects to.  The log control
 * tool sends commands to the Log Control Daemon who validates them and keeps track of log
 * settings that last longer than the lifetime of a given process.  When another process opens
 * a log session with the Log Control Daemon, the Daemon updates that process with any settings
 * that were previously set for processes that have that name.
 *
 * In the current implementation, the Log Control Daemon sends settings to processes using
 * the IPC session.  These get applied by a message receive handler running in the process's
 * main thread.
 *
 * @section log_future Future Enhancement
 *
 * In the future, the Log Control Daemon will write log settings (filter level and keyword
 * enable/disable) directly into the client process's address space using shared memory.
 * The shared memory file is created by the log client who passes the shared memory file
 * fd to the Log Control Daemon over the @ref c_messaging when the client starts up.
 *
 * When a process starts, it must create its shared memory file and define the layout of the
 * settings within it before it talks to the Log Control Daemon.  Once it has notified the
 * Log Control Daemon of the file's location and layout, the size and layout of the file must
 * not change for the lifetime of the process.
 *
 * The shared memory file layout is a list of log sessions, identified by component name.
 * For each session, there exists a level, a set of output location flags, and a list of
 * trace keywords, each of which has an "is enabled" flag.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LINUX_LOGPLATFORM_INCLUDE_GUARD
#define LINUX_LOGPLATFORM_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Re-Initialize the logging system.
 * This must be called in case the underlying log fd has been closed.
 */
//--------------------------------------------------------------------------------------------------
void log_ReInit
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Connects to the Log Control Daemon.  This must not be done until after the Messaging system
 * is initialized, but must be done before the main thread's Event Loop starts to ensure that
 * all log settings are received from the Log Control Daemon and applied to sessions in the local
 * process before any component initialization functions are run.
 */
//--------------------------------------------------------------------------------------------------
void log_ConnectToControlDaemon
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Registers a named component with the logging system.
 *
 * @return
 *      A log session reference.  This reference must be kept by the component and accessible
 *      through a local macro with the name LE_LOG_SESSION.
 */
//--------------------------------------------------------------------------------------------------
le_log_SessionRef_t log_RegComponent
(
    const char* componentNamePtr,       ///< [IN] A pointer to the component's name.
    le_log_Level_t** levelFilterPtrPtr  ///< [OUT] Set to point to the component's level filter.
);


//--------------------------------------------------------------------------------------------------
/**
 * Logs a generic message with the given information.
 */
//--------------------------------------------------------------------------------------------------
void log_LogGenericMsg
(
    le_log_Level_t level,       ///< [IN] Severity level.
    const char* procNamePtr,    ///< [IN] Process name.
    pid_t pid,                  ///< [IN] PID of the process.
    const char* msgPtr          ///< [IN] Message.
);

#endif /* end LINUX_LOGPLATFORM_INCLUDE_GUARD */
