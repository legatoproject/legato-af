/**
 * @page c_logging Logging API
 *
 * @subpage le_log.h "API Reference" <br>
 *
 * The Legato Logging API provides a toolkit allowing code to be instrumented with error, warning,
 * informational, and debugging messages. These messages can be turned on or off remotely and pushed or pulled
 * from the device through a secure shell, cloud services interfaces, e-mail, SMS, etc.
 *
 * @section c_log_logging Logging Basics
 *
 * Legato's logging can be configured through this API, and there's also a command-line target
 * @ref toolsTarget_log tool available.
 *
 * @subsection c_log_levels Levels
 *
 * Log messages are categorized according to the severity of the information being logged.
 * A log message may be purely informational, describing something that is
 * expected to occur from time-to-time during normal operation; or it may be a report of a fault
 * that might have a significant negative impact on the operation of the system.  To
 * differentiate these, each log entry is associated with one of the following log levels:
 *
 *  - @ref LE_LOG_DEBUG "DEBUG":
 *    Handy for troubleshooting.
 *  - @ref LE_LOG_INFO "INFORMATION":
 *    Expected to happen; can be interesting even when not troubleshooting.
 *  - @ref LE_LOG_WARN "WARNING":
 *    Should not normally happen; may not have any real impact on system performance.
 *  - @ref LE_LOG_ERR "ERROR":
 *    Fault that may result in noticeable short-term system misbehaviour. Needs attention.
 *  - @ref LE_LOG_CRIT "CRITICAL":
 *    Fault needs urgent attention. Will likely result in system failure.
 *  - @ref LE_LOG_EMERG "EMERGENCY":
 *    Definite system failure.
 *
 * @subsection c_log_basic_defaultSyslog Standard Out and Standard Error in Syslog
 *
 * By default, app processes will have their @c stdout and @c stderr redirected to the @c syslog. Each
 * process’s stdout will be logged at INFO severity level; it’s stderr will be logged at
 * “ERR” severity level.

 * There are two limitations with this feature:
 * - the PID reported in the logs generally refer to the PID of the process that
 * generates the stdout/stderr message. If a process forks, then both the parent and
 * child processes’ stdout/stderr will share the same connection to the syslog, and the parent’s
 * PID will be reported in the logs for both processes.
 * - stdout is line buffered when connected to a terminal, which means
 * <code>printf(“hello\n”)</code> will be printed to the terminal immediately. If stdout is
 * connected to something like a pipe it's bulk buffered, which means a flush doesn't occur until the buffer is full.
 *
 * To make your process line buffer stdout so that printf will show up in the logs as expected,
 * the @c setlinebuf(stdout) system call can be used.  Alternatively, @c fflush(stdout) can be called \
 * to force a flush of the stdout buffer.
 *
 * This issue doesn't exist with stderr as stderr is never buffered.
 *
 * @subsection c_log_basic_logging Basic Logging
 *
 * A series of macros are available to make logging easy.
 *
 * None of them return anything.
 *
 * All of them accept printf-style arguments, consisting of a format string followed by zero or more
 * parameters to be printed (depending on the contents of the format string).
 *
 * There is a logging macro for each of the log levels:
 *
 *  - @ref LE_DEBUG(formatString, ...)
 *  - @ref LE_INFO(formatString, ...)
 *  - @ref LE_WARN(formatString, ...)
 *  - @ref LE_ERROR(formatString, ...)
 *  - @ref LE_CRIT(formatString, ...)
 *  - @ref LE_EMERG(formatString, ...)
 *
 * For example,
 * @code
 * LE_INFO("Obtained new IP address %s.", ipAddrStr);
 * @endcode
 *
 * @subsection c_log_conditional_logging Conditional Logging
 *
 * Similar to the basic macros, but these contain a conditional expression as their first parameter.  If this expression equals
 * true, then the macro will generate this log output:
 *
 *  - @ref LE_DEBUG_IF(expression, formatString, ...)
 *  - @ref LE_INFO_IF(expression, formatString, ...)
 *  - @ref LE_WARN_IF(expression, formatString, ...)
 *  - @ref LE_ERROR_IF(expression, formatString, ...)
 *  - @ref LE_CRIT_IF(expression, formatString, ...)
 *  - @ref LE_EMERG_IF(expression, formatString, ...)
 *
 * Instead of writing
 * @code
 * if (result == -1)
 * {
 *     LE_WARN("Failed to send message to server.  Errno = %d.", errno);
 * }
 * @endcode
 *
 * you could write this:
 * @code
 * LE_WARN_IF(result == -1, "Failed to send message to server.  Errno = %d.", errno);
 * @endcode
 *
 * @subsection c_log_loging_fatals Fatal Errors
 *
 * There are some special logging macros intended for fatal errors:
 *
 *  - @ref LE_FATAL(formatString, ...) \n
 *    Always kills the calling process after logging the message at EMERGENCY level (never returns).
 *  - @ref LE_FATAL_IF(condition, formatString, ...) \n
 *    If the condition is true, kills the calling process after logging the message at EMERGENCY
 *    level.
 *  - @ref LE_ASSERT(condition) \n
 *    If the condition is true, does nothing. If the condition is false, logs the source
 *    code text of the condition at EMERGENCY level and kills the calling process.
 *  - @ref LE_ASSERT_OK(condition) \n
 *    If the condition is LE_OK (0), does nothing. If the condition is anything else, logs the
 *    a message at EMERGENCY level, containing the source code text of the condition, indicating
 *    that it did not evaluate to LE_OK, and kills the calling process.
 *
 * For example,
 * @code
 * if (NULL == objPtr)
 * {
 *     LE_FATAL("Object pointer is NULL!");
 * }
 *
 * // Now I can go ahead and use objPtr, knowing that if it was NULL then LE_FATAL() would have
 * // been called and LE_FATAL() never returns.
 * @endcode
 *
 * or,
 *
 * @code
 * LE_FATAL_IF(NULL == objPtr, "Object pointer is NULL!");
 *
 * // Now I can go ahead and use objPtr, knowing that if it was NULL then LE_FATAL_IF() would not
 * // have returned.
 * @endcode
 *
 * or,
 *
 * @code
 * LE_ASSERT(NULL != objPtr);
 *
 * // Now I can go ahead and use objPtr, knowing that if it was NULL then LE_ASSERT() would not
 * // have returned.
 * @endcode
 *
 * @subsection c_log_tracing Tracing
 *
 * Finally, a macro is provided for tracing:
 *
 *  - @ref LE_TRACE(traceRef, string, ...)
 *
 * This macro is special because it's independent of log level.  Instead, trace messages are
 * associated with a trace keyword.  Tracing can be enabled and disabled based on these keywords.
 *
 * If a developer wanted to trace the creation of "shape"
 * objects in their GUI package, they could add trace statements like the following:
 *
 * @code
 * LE_TRACE(NewShapeTraceRef, "Created %p with position (%d,%d).", shapePtr, shapePtr->x, shapePtr->y);
 * @endcode
 *
 * The reference to the trace is obtained at start-up as follows:
 *
 * @code
 * NewShapeTraceRef = le_log_GetTraceRef("newShape");
 * @endcode
 *
 * This allows enabling and disabling these LE_TRACE() calls using the "newShape" keyword
 * through configuration settings and runtime log control tools.  See @ref c_log_controlling below.
 *
 * Applications can use @ref LE_IS_TRACE_ENABLED(NewShapeTraceRef) to query whether
 * a trace keyword is enabled.
 *
 * These allow apps to hook into the trace management system to use it to implement
 * sophisticated, app-specific tracing or profiling features.
 *
 * @subsection c_log_resultTxt Result Code Text
 *
 * The @ref le_result_t macro supports printing an error condition in a human-readable text string.
 *
 * @code
 * result = le_foo_DoSomething();
 *
 * if (result != LE_OK)
 * {
 *     LE_ERROR("Failed to do something. Result = %d (%s).", result, LE_RESULT_TXT(result));
 * }
 * @endcode
 *
 *
 * @section c_log_controlling Log Controls
 *
 * Log level filtering and tracing can be controlled at runtime using:
 *  - the command-line Log Control Tool (@ref toolsTarget_log)
 *  - configuration settings
 *  - environment variables
 *  - function calls.
 *
 * @subsection c_log_control_tool Log Control Tool
 *
 * The log control tool is used from the command-line to control the log
 * level filtering, log output location (syslog/stderr), and tracing for different components
 * within a running system.
 *
 * Online documentation can be accessed from the log control tool by running "log help".
 *
 * Here are some code samples.
 *
 * To set the log level to INFO for a component "myComp" running in all processes with the
 * name "myProc":
 * @verbatim
$ log level INFO myProc/myComp
@endverbatim
 *
 * To set the log level to DEBUG for a component "myComp" running in a process with PID 1234:
 * @verbatim
$ log level DEBUG 1234/myComp
@endverbatim
 *
 * To enable all LE_TRACE statements tagged with the keyword "foo" in a component called "myComp"
 * running in all processes called "myProc":
 * @verbatim
$ log trace foo myProc/myComp
@endverbatim
 *
 * To disable the trace statements tagged with "foo" in the component "myComp" in processes
 * called "myProc":
 * @verbatim
$ log stoptrace foo myProc/myComp
@endverbatim
 *
 * With all of the above examples "*" can be used in place of the process name or a component
 * name (or both) to mean "all processes" and/or "all components".
 *
 * @subsection c_log_control_config Log Control Configuration Settings
 *
 * @note The configuration settings haven't been implemented yet.
 *
 * @todo Write @ref c_log_control_config section.
 *
 * @subsection c_log_control_environment_vars Environment Variables
 *
 * Environment variables can be used to control the default log settings, taking effect immediately
 * at process start-up; even before the Log Control Daemon has been connected to.
 *
 * Settings in the Log Control Daemon (applied through configuration and/or the log control tool)
 * will override the environment variable settings when the process connects to the Log
 * Control Daemon.
 *
 * @subsubsection c_log_control_env_level LE_LOG_LEVEL
 *
 * @c LE_LOG_LEVEL can be used to set the default log filter level for all components
 * in the process.  Valid values are:
 *
 * - @c EMERGENCY
 * - @c CRITICAL
 * - @c ERROR
 * - @c WARNING
 * - @c INFO
 * - @c DEBUG
 *
 * For example,
 * @verbatim
$ export LE_LOG_LEVEL=DEBUG
@endverbatim
 *
 * @subsubsection c_log_control_env_trace LE_LOG_TRACE
 *
 * @c LE_LOG_TRACE allows trace keywords to be enabled by default.  The contents of this
 * variable is a colon-separated list of keywords that should be enabled. Each keyword
 * must be prefixed with a component name followed by a slash ('/').
 *
 * For example,
 * @verbatim
$ export LE_LOG_TRACE=framework/fdMonitor:framework/logControl
@endverbatim
 *
 * @subsection c_log_control_functions Programmatic Log Control
 *
 * Normally, configuration settings and the log control tool should suffice for controlling
 * logging functionality.  In some situations, it can be convenient
 * to control logging programmatically in C.
 *
 * le_log_SetFilterLevel() sets the log filter level.
 *
 * le_log_GetFilterLevel() gets the log filter level.
 *
 * Trace keywords can be enabled and disabled programmatically by calling
 * @subpage le_log_EnableTrace() and @ref le_log_DisableTrace().
 *
 *
 * @section c_log_format Log Formats
 *
 * Log entries can also contain any of these:
 *  - timestamp (century, year, month, day, hours, minutes, seconds, milliseconds, microseconds)
 *  - level (debug, info, warning, etc.) @b or trace keyword
 *  - process ID
 *  - component name
 *  - thread name
 *  - source code file name
 *  - function name
 *  - source code line number
 *
 * Log messages have the following format:
 *
 * @verbatim
Jan  3 02:37:56  INFO  | processName[pid]/componentName T=threadName | fileName.c funcName() lineNum | Message
@endverbatim
 *
 * @section c_log_debugFiles App Crash Logs

* When a process within an app faults or exits in error, a copy of the current syslog buffer
* is captured along with a core file of the process crash (if generated).

* The core file maximum size is determined by the process settings @c maxCoreDumpFileBytes and
* @c maxFileBytes found in the processes section of your app's @c .adef file.  By default, the
* @c maxCoreDumpFileBytes is set to 0, do not create a core file.

* To help save the target from flash burnout, the syslog and core files are stored in the RAM
* FS under /tmp.  When a crash occurs, this directory is created:

 @verbatim
 /tmp/legato_logs/
 @endverbatim

* The files in that directory look like this:

 @verbatim
 core-myProc-1418694851
 syslog-myApp-myProc-1418694851
 @endverbatim

* To save on RAM space, only the most recent 4 copies of each file are preserved.

* If the fault action for that app's process is to reboot the target, the output location is changed to
* this (and the most recent files in RAM space are preserved across reboots):

 @verbatim
 /mnt/flash/legato_logs/
 @endverbatim

*
 * @todo May need to support log format configuration through command-line arguments or a
 *       configuration file.
 *
 * @todo Write @ref c_log_retrieving section
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */


/** @file le_log.h
 *
 * Legato @ref c_logging include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_LOG_INCLUDE_GUARD
#define LEGATO_LOG_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Local definitions that should not be used directly.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_LOG_DEBUG,   ///< Debug message.
    LE_LOG_INFO,    ///< Informational message.  Normally expected.
    LE_LOG_WARN,    ///< Warning.  Possibly indicates a problem.  Should be addressed.
    LE_LOG_ERR,     ///< Error.  Definitely indicates a  fault that needs to be addressed.
                    ///  Possibly resulted in a system failure.
    LE_LOG_CRIT,    ///< Critical error.  Fault that almost certainly has or will result in a
                    ///  system failure.
    LE_LOG_EMERG    ///< Emergency.  A fatal error has occurred.  A process is being terminated.
}
le_log_Level_t;

//--------------------------------------------------------------------------------------------------
/**
 * Compile-time filtering level.
 *
 * @note Logs below this filter level will be removed at compile-time and cannot be enabled
 *       at runtime.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_LOG_STATIC_FILTER_EMERG
#   define LE_LOG_LEVEL_STATIC_FILTER   LE_LOG_EMERG
#elif LE_CONFIG_LOG_STATIC_FILTER_CRIT
#   define LE_LOG_LEVEL_STATIC_FILTER   LE_LOG_CRIT
#elif LE_CONFIG_LOG_STATIC_FILTER_ERR
#   define LE_LOG_LEVEL_STATIC_FILTER   LE_LOG_ERR
#elif LE_CONFIG_LOG_STATIC_FILTER_WARN
#   define LE_LOG_LEVEL_STATIC_FILTER   LE_LOG_WARN
#elif LE_CONFIG_LOG_STATIC_FILTER_INFO
#   define LE_LOG_LEVEL_STATIC_FILTER   LE_LOG_INFO
#else /* default to LE_DEBUG */
#   define LE_LOG_LEVEL_STATIC_FILTER   LE_LOG_DEBUG
#endif

//--------------------------------------------------------------------------------------------------
/// @cond HIDDEN_IN_USER_DOCS

typedef struct le_log_Session* le_log_SessionRef_t;

typedef struct le_log_Trace* le_log_TraceRef_t;

#if !defined(LE_DEBUG) && \
    !defined(LE_DUMP) && \
    !defined(LE_LOG_DUMP) && \
    !defined(LE_INFO) && \
    !defined(LE_WARN) && \
    !defined(LE_ERROR) && \
    !defined(LE_CRIT) && \
    !defined(LE_EMERG)

//--------------------------------------------------------------------------------------------------
/**
 * Send a message to the logging target.
 **/
//--------------------------------------------------------------------------------------------------
void _le_log_Send
(
    const le_log_Level_t     level,             ///< Log level.
    const le_log_TraceRef_t  traceRef,          ///< Trace reference.  May be NULL.
    le_log_SessionRef_t      logSession,        ///< Log session.  May be NULL.
    const char              *filenamePtr,       ///< File name.
    const char              *functionNamePtr,   ///< Function name.  May be NULL.
    const unsigned int       lineNumber,        ///< Line number.
    const char              *formatPtr,         ///< Format string.
    ...                                         ///< Positional parameters.
) __attribute__((format (printf, 7, 8)));

//--------------------------------------------------------------------------------------------------
/**
 * Log data block.  Provides a hex dump for debug.
 */
//--------------------------------------------------------------------------------------------------
void _le_LogData
(
    le_log_Level_t       level,             ///< Log level.
    const uint8_t       *dataPtr,           ///< The buffer address to be dumped.
    int                  dataLength,        ///< The data length of buffer.
    const char          *filenamePtr,       ///< The name of the source file that logged the
                                            ///< message.
    const char          *functionNamePtr,   ///< The name of the function that logged the message.
    const unsigned int   lineNumber         ///< The line number in the source file that logged the
                                            ///< message.
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to a trace keyword's settings.
 *
 * @return  Trace reference.
 **/
//--------------------------------------------------------------------------------------------------
le_log_TraceRef_t _le_log_GetTraceRef
(
    le_log_SessionRef_t  logSession,    ///< Log session.
    const char          *keyword        ///< Trace keyword.
);

//--------------------------------------------------------------------------------------------------
/**
 * Sets the log filter level for the calling component.
 **/
//--------------------------------------------------------------------------------------------------
void _le_log_SetFilterLevel
(
    le_log_SessionRef_t logSession, ///< Log session.
    le_log_Level_t      level       ///< Log level to apply.
);

//--------------------------------------------------------------------------------------------------
/**
 * Filtering session reference for the current source file.
 *
 * @note The real name of this is passed in by the build system. This way it can be a unique
 *       variable for each component.
 */
//--------------------------------------------------------------------------------------------------
extern LE_SHARED le_log_SessionRef_t LE_LOG_SESSION;

//--------------------------------------------------------------------------------------------------
/**
 * Filtering level for the current source file.
 *
 * @note The real name of this is passed in by the build system. This way it can be a unique
 *       variable for each component.
 */
//--------------------------------------------------------------------------------------------------
extern LE_SHARED le_log_Level_t* LE_LOG_LEVEL_FILTER_PTR;

//--------------------------------------------------------------------------------------------------
/**
 * Internal macro to set whether the function name is displayed with log messages.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_LOG_FUNCTION_NAMES
#   define _LE_LOG_FUNCTION_NAME __func__
#else
#   define _LE_LOG_FUNCTION_NAME NULL
#endif

/// @endcond
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Internal macro to filter out messages that do not meet the current filtering level.
 */
//--------------------------------------------------------------------------------------------------
#define _LE_LOG_MSG(level, formatString, ...) \
    do { \
        if ((level >= LE_LOG_LEVEL_STATIC_FILTER) &&                        \
            ((LE_LOG_LEVEL_FILTER_PTR == NULL) || (level >= *LE_LOG_LEVEL_FILTER_PTR))) \
            _le_log_Send(level, NULL, LE_LOG_SESSION, __FILE__, _LE_LOG_FUNCTION_NAME, __LINE__, \
                    formatString, ##__VA_ARGS__); \
    } while(0)


//--------------------------------------------------------------------------------------------------
/** @internal
 * The following macros are used to send log messages at different severity levels:
 *
 * Accepts printf-style arguments, consisting of a format string followed by zero or more parameters
 * to be printed (depending on the contents of the format string).
 */
//--------------------------------------------------------------------------------------------------

/** @copydoc LE_LOG_DEBUG */
#define LE_DEBUG(formatString, ...)     _LE_LOG_MSG(LE_LOG_DEBUG, formatString, ##__VA_ARGS__)

//--------------------------------------------------------------------------------------------------
/**
 * Dump a buffer of data as hexadecimal to the log at debug level.
 *
 *  @param  dataPtr     Binary data to dump.
 *  @param  dataLength  Length og the buffer.
 */
//--------------------------------------------------------------------------------------------------
#define LE_DUMP(dataPtr, dataLength)                                                              \
    _le_LogData(LE_LOG_DEBUG, dataPtr, dataLength, STRINGIZE(LE_FILENAME), _LE_LOG_FUNCTION_NAME, \
        __LINE__)

//--------------------------------------------------------------------------------------------------
/**
 * Dump a buffer of data as hexadecimal to the log at the specified level.
 *
 *  @param  level       Log level.
 *  @param  dataPtr     Binary data to dump.
 *  @param  dataLength  Length of the buffer.
 */
//--------------------------------------------------------------------------------------------------
#define LE_LOG_DUMP(level, dataPtr, dataLength) \
    _le_LogData(level, dataPtr, dataLength, STRINGIZE(LE_FILENAME), _LE_LOG_FUNCTION_NAME, __LINE__)
/** @copydoc LE_LOG_INFO */
#define LE_INFO(formatString, ...)      _LE_LOG_MSG(LE_LOG_INFO, formatString, ##__VA_ARGS__)
/** @copydoc LE_LOG_WARN */
#define LE_WARN(formatString, ...)      _LE_LOG_MSG(LE_LOG_WARN, formatString, ##__VA_ARGS__)
/** @copydoc LE_LOG_ERR */
#define LE_ERROR(formatString, ...)     _LE_LOG_MSG(LE_LOG_ERR, formatString, ##__VA_ARGS__)
/** @copydoc LE_LOG_CRIT */
#define LE_CRIT(formatString, ...)      _LE_LOG_MSG(LE_LOG_CRIT, formatString, ##__VA_ARGS__)
/** @copydoc LE_LOG_EMERG */
#define LE_EMERG(formatString, ...)     _LE_LOG_MSG(LE_LOG_EMERG, formatString, ##__VA_ARGS__)


//--------------------------------------------------------------------------------------------------
/**
 * Queries whether or not a trace keyword is enabled.
 *
 * @return
 *      true if the keyword is enabled.
 *      false otherwise.
 */
//--------------------------------------------------------------------------------------------------
#define LE_IS_TRACE_ENABLED(traceRef)  (le_log_IsTraceEnabled(traceRef))


//--------------------------------------------------------------------------------------------------
/**
 * Logs the string if the keyword has been enabled by a runtime tool or configuration setting.
 */
//--------------------------------------------------------------------------------------------------
#define LE_TRACE(traceRef, string, ...)         \
        if (le_log_IsTraceEnabled(traceRef))    \
        {                                       \
            _le_log_Send((le_log_Level_t)-1,    \
                    traceRef,                   \
                    LE_LOG_SESSION,             \
                    STRINGIZE(LE_FILENAME),     \
                    _LE_LOG_FUNCTION_NAME,      \
                    __LINE__,                   \
                    string,                     \
                    ##__VA_ARGS__);             \
        }


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to a trace keyword's settings.
 *
 * @param keywordPtr   [IN] Pointer to the keyword string.
 *
 * @return  Trace reference.
 **/
//--------------------------------------------------------------------------------------------------
#define le_log_GetTraceRef(keywordPtr)                  \
    _le_log_GetTraceRef(LE_LOG_SESSION, (keywordPtr))


//--------------------------------------------------------------------------------------------------
/**
 * Determines if a trace is currently enabled.
 *
 * @param traceRef    [IN] Trace reference obtained from le_log_GetTraceRef()
 *
 * @return  true if enabled, false if not.
 **/
//--------------------------------------------------------------------------------------------------
#define le_log_IsTraceEnabled(traceRef) ((traceRef) == NULL ? false : *((bool *) (traceRef)))


//--------------------------------------------------------------------------------------------------
/**
 * Sets the log filter level for the calling component.
 *
 * @note    Normally not necessary as the log filter level can be controlled at
 *          runtime using the log control tool, and can be persistently configured.
 *          See @ref c_log_controlling.
 *
 * @param level   [IN] Log filter level to apply to the current log session.
 **/
//--------------------------------------------------------------------------------------------------
#define le_log_SetFilterLevel(level)                    \
    _le_log_SetFilterLevel(LE_LOG_SESSION, (level))


//--------------------------------------------------------------------------------------------------
/**
 * Gets the log filter level for the calling component.
 **/
//--------------------------------------------------------------------------------------------------
#define le_log_GetFilterLevel()                 \
    ((LE_LOG_LEVEL_FILTER_PTR != NULL)?(*LE_LOG_LEVEL_FILTER_PTR):(LE_LOG_INFO))


//--------------------------------------------------------------------------------------------------
/**
 * Enables a trace.
 *
 * @note    Normally, this is not necessary, since traces can be enabled at runtime using the
 *          log control tool and can be persistently configured.  See @ref c_log_controlling.
 *
 * @param traceRef    [IN] Trace reference obtained from le_log_GetTraceRef()
 *
 **/
//--------------------------------------------------------------------------------------------------
#define le_log_EnableTrace(traceRef)            \
    ((void)(*((bool*)(traceRef)) = true))


//--------------------------------------------------------------------------------------------------
/**
 * Disables a trace.
 *
 * @note    Normally, this is not necessary, since traces can be enabled at runtime using the
 *          log control tool and can be persistently configured.  See @ref c_log_controlling.
 *
 * @param traceRef    [IN] Trace reference obtained from le_log_GetTraceRef()
 *
 **/
//--------------------------------------------------------------------------------------------------
#define le_log_DisableTrace(traceRef)           \
    ((void)(*((bool*)(traceRef)) = false))

#else

// If any logging macro is overridden, all logging macros must be overridden.
#if !defined(LE_DEBUG)
#  error "Logging are overridden, but LE_DEBUG not defined.  Define LE_DEBUG."
#endif

#if !defined(LE_DUMP)
#  error "Logging are overridden, but LE_DUMP not defined.  Define LE_DUMP."
#endif

#if !defined(LE_LOG_DUMP)
#  error "Logging are overridden, but LE_LOG_DUMP not defined.  Define LE_LOG_DUMP."
#endif

#if !defined(LE_INFO)
#  error "Logging are overridden, but LE_INFO not defined.  Define LE_INFO."
#endif

#if !defined(LE_WARN)
#  error "Logging are overridden, but LE_WARN not defined.  Define LE_WARN."
#endif

#if !defined(LE_ERROR)
#  error "Logging are overridden, but LE_ERROR not defined.  Define LE_ERROR."
#endif

#if !defined(LE_CRIT)
#  error "Logging are overridden, but LE_CRIT not defined.  Define LE_CRIT."
#endif

#if !defined(LE_EMERG)
#  error "Logging are overridden, but LE_EMERG not defined.  Define LE_EMERG."
#endif

// If SetFilterLevel is not defined when logging is overridden,
// assume it cannot be set programatically
#ifndef le_log_SetFilterLevel
//--------------------------------------------------------------------------------------------------
/**
 * Sets the log filter level for the calling component.
 *
 * @note    Normally not necessary as the log filter level can be controlled at
 *          runtime using the log control tool, and can be persistently configured.
 *          See @ref c_log_controlling.
 *
 * @param level   [IN] Log filter level to apply to the current log session.
 **/
//--------------------------------------------------------------------------------------------------
#define le_log_SetFilterLevel(level) ((void)(level))
#endif


// If le_log_GetFilterLevel is not defined when logging is overridden, assume it
// cannot be obtained programatically.  In this case code should assume filter level
// is equal to static filter level
#ifndef le_log_GetFilterLevel
//--------------------------------------------------------------------------------------------------
/**
 * Gets the log filter level for the calling component.
 **/
//--------------------------------------------------------------------------------------------------
#define le_log_GetFilterLevel()  (LE_LOG_LEVEL_STATIC_FILTER)
#endif

//--------------------------------------------------------------------------------------------------
/*
 * If logging is overridden, disable Legato tracing mechanism.  Tracing is noisy and should
 * not be put into general logs.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Queries whether or not a trace keyword is enabled.
 *
 * @return
 *      true if the keyword is enabled.
 *      false otherwise.
 */
//--------------------------------------------------------------------------------------------------
#define LE_IS_TRACE_ENABLED(traceRef)  (false)


//--------------------------------------------------------------------------------------------------
/**
 * Logs the string if the keyword has been enabled by a runtime tool or configuration setting.
 */
//--------------------------------------------------------------------------------------------------
#define LE_TRACE(traceRef, string, ...) ((void)0)


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to a trace keyword's settings.
 *
 * @param keywordPtr   [IN] Pointer to the keyword string.
 *
 * @return  Trace reference.
 **/
//--------------------------------------------------------------------------------------------------
#define le_log_GetTraceRef(keywordPtr) ((le_log_TraceRef_t)NULL)


//--------------------------------------------------------------------------------------------------
/**
 * Determines if a trace is currently enabled.
 *
 * @param traceRef    [IN] Trace reference obtained from le_log_GetTraceRef()
 *
 * @return  true if enabled, false if not.
 **/
//--------------------------------------------------------------------------------------------------
#define le_log_IsTraceEnabled(traceRef) (false)


//--------------------------------------------------------------------------------------------------
/**
 * Enables a trace.
 *
 * @note    Normally, this is not necessary, since traces can be enabled at runtime using the
 *          log control tool and can be persistently configured.  See @ref c_log_controlling.
 *
 * @param traceRef    [IN] Trace reference obtained from le_log_GetTraceRef()
 *
 **/
//--------------------------------------------------------------------------------------------------
#define le_log_EnableTrace(traceRef) ((void)(0))


//--------------------------------------------------------------------------------------------------
/**
 * Disables a trace.
 *
 * @note    Normally, this is not necessary, since traces can be enabled at runtime using the
 *          log control tool and can be persistently configured.  See @ref c_log_controlling.
 *
 * @param traceRef    [IN] Trace reference obtained from le_log_GetTraceRef()
 *
 **/
//--------------------------------------------------------------------------------------------------
#define le_log_DisableTrace(traceRef) ((void)(0))

#endif /* Logging macro override */

/// Function that does the real work of translating result codes.  See @ref LE_RESULT_TXT.
const char* _le_log_GetResultCodeString
(
    le_result_t resultCode  ///< [in] Result code value to be translated.
);

/// Function that exits in a race-free manner -- work around glibc BZ#14333
__attribute__((noreturn))
void _le_log_ExitFatal
(
    void
);

//--------------------------------------------------------------------------------------------------
/** @internal
 * The following macros are used to send log messages at different severity levels conditionally.
 * If the condition is true the message is logged.  If the condition is false the message is not
 * logged.
 *
 * Accepts printf-style arguments, consisting of a format string followed by zero or more parameters
 * to be printed (depending on the contents of the format string).
 */
//--------------------------------------------------------------------------------------------------

/** @ref LE_DEBUG if condition is met. */
#define LE_DEBUG_IF(condition, formatString, ...) \
        if (condition) { LE_DEBUG(formatString, ##__VA_ARGS__); }
/** @ref LE_INFO if condition is met. */
#define LE_INFO_IF(condition, formatString, ...) \
        if (condition) { LE_INFO(formatString, ##__VA_ARGS__); }
/** @ref LE_WARN if condition is met. */
#define LE_WARN_IF(condition, formatString, ...) \
        if (condition) { LE_WARN(formatString, ##__VA_ARGS__); }
/** @ref LE_ERROR if condition is met. */
#define LE_ERROR_IF(condition, formatString, ...) \
        if (condition) { LE_ERROR(formatString, ##__VA_ARGS__); }
/** @ref LE_CRIT if condition is met. */
#define LE_CRIT_IF(condition, formatString, ...) \
        if (condition) { LE_CRIT(formatString, ##__VA_ARGS__); }
/** @ref LE_EMERG if condition is met. */
#define LE_EMERG_IF(condition, formatString, ...) \
        if (condition) { LE_EMERG(formatString, ##__VA_ARGS__); }


//--------------------------------------------------------------------------------------------------
/**
 * Log fatal errors by killing the calling process after logging the message at EMERGENCY
 * level.  This macro never returns.
 *
 * Accepts printf-style arguments, consisting of a format string followed by zero or more parameters
 * to be printed (depending on the contents of the format string).
 */
//--------------------------------------------------------------------------------------------------
#if !LE_CONFIG_DEBUG
#define LE_FATAL(formatString, ...) \
        { LE_EMERG(formatString, ##__VA_ARGS__); _le_log_ExitFatal(); }
#else
#define LE_FATAL(formatString, ...) \
        { LE_EMERG(formatString, ##__VA_ARGS__); abort(); }
#endif


//--------------------------------------------------------------------------------------------------
/**
 * This macro does nothing if the condition is false, otherwise it logs the message at EMERGENCY
 * level and then kills the calling process.
 *
 * Accepts printf-style arguments, consisting of a format string followed by zero or more parameters
 * to be printed (depending on the contents of the format string).
 */
//--------------------------------------------------------------------------------------------------
#define LE_FATAL_IF(condition, formatString, ...) \
        if (condition) { LE_FATAL(formatString, ##__VA_ARGS__) }


//--------------------------------------------------------------------------------------------------
/**
 * This macro does nothing if the condition is true, otherwise it logs the condition expression as
 * a message at EMERGENCY level and then kills the calling process.
 */
//--------------------------------------------------------------------------------------------------
#define LE_ASSERT(condition)                            \
    LE_FATAL_IF(!(condition), "Assert Failed: '%s'", #condition)


//--------------------------------------------------------------------------------------------------
/**
 * This macro does nothing if the condition is LE_OK (0), otherwise it logs that the expression did
 * not evaluate to LE_OK (0) in a message at EMERGENCY level and then kills the calling process.
 */
//--------------------------------------------------------------------------------------------------
#define LE_ASSERT_OK(condition)                                         \
    LE_FATAL_IF((condition) != LE_OK, "Assert Failed: '%s' is not LE_OK (0)", #condition)


//--------------------------------------------------------------------------------------------------
/**
 * Get a null-terminated, printable string representing an le_result_t value.
 *
 * For example, LE_RESULT_TXT(LE_NOT_PERMITTED) would return a pointer to a string containing
 * "LE_NOT_PERMITTED".
 *
 * "(unknown)" will be returned if the value given is out of range.
 *
 * @return Pointer to a string constant.
 */
//--------------------------------------------------------------------------------------------------
#define LE_RESULT_TXT(v) _le_log_GetResultCodeString(v)


#endif // LEGATO_LOG_INCLUDE_GUARD
