/**
 * @page c_logging Logging API
 *
 *
 * @ref le_log.h "Click here for the API Reference documentation."
 *
 * <HR>
 *
 * @ref c_log_logging <br>
 * @ref c_log_controlling <br>
 * @ref c_log_format <br>
 *
 *
 * Logging is a critical part of any embedded development framework. Most devices can't
 * display error or warning messages and don't have a human user monitoring them.
 * Even when a device does have a display and a user watching it, the log
 * messages often don't help the device's primary user.
 * And displaying messages on a screen doesn't support 
 * remote troubleshooting; especially when the device is hidden from view inside
 * a piece of equipment or located in remote geographic regions.
 *
 * The Legato Logging API provides a toolkit allowing code to be instrumented with error, warning,
 * informational, and debugging messages. These messages can be turned on or off remotely and pushed or pulled
 * from the device through a secure shell, cloud services interfaces, e-mail, SMS, etc.
 *
 * @section c_log_logging Logging Basics
 *
 * @ref c_log_levels <br>
 * @ref c_log_basic_logging <br>
 * @ref c_log_conditional_logging <br>
 * @ref c_log_loging_fatals <br>
 * @ref c_log_tracing <br>
 * @ref c_log_resultTxt
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
 *    Handy during troubleshooting, but is normally just log clutter.
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
 *     LE_WARN("Failed to send message to server.  Errno = %m.");
 * }
 * @endcode
 *
 * you could write this:
 * @code
 * LE_WARN_IF(result == -1, "Failed to send message to server.  Errno = %m.");
 * @endcode
 *
 * @subsection c_log_loging_fatals Fatal Errors
 *
 * There are some special logging macros that are intended for logging fatal errors:
 *
 *  - @ref LE_FATAL(formatString, ...) \n
 *    Always kills the calling process after logging the message at EMERGENCY level (never returns).
 *  - @ref LE_FATAL_IF(condition, formatString, ...) \n
 *    If the condition is true, kills the calling process after logging the message at EMERGENCY
 *    level.
 *  - @ref LE_ASSERT(condition) \n
 *    If the condition is true, does nothing. If the condition is false, logs the source
 *    code text of the condition at EMERGENCY level and kills the calling process.
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
 * These allow applications to hook into the trace management system to use it to implement
 * sophisticated, application-specific tracing or profiling features.
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
 * @ref c_log_control_tool <br>
 * @ref c_log_control_config <br>
 * @ref c_log_control_environment_vars <br>
 * @ref c_log_control_env_level <br>
 * @ref c_log_control_env_trace <br>
 * @ref c_log_control_functions <br>
 * 
 * Log level filtering and tracing can be controlled at runtime using:
 *  - the command-line Log Control Tool ("log")
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
 * To set the log filter level, @ref le_log_SetFilterLevel() is provided.
 *
 * Trace keywords can be enabled and disabled programmatically by calling
 * @ref le_log_EnableTrace() and @ref le_log_DisableTrace().
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
 * @todo May need to support log format configuration through command-line arguments or a
 *       configuration file.
 *
 * @todo Write @ref c_log_retrieving section
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */


/** @file le_log.h
 *
 * Legato @ref c_logging include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
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

/* @cond PrivateFunctions */

typedef struct le_log_Session* le_log_SessionRef_t;

typedef struct le_log_Trace* le_log_TraceRef_t;

void _le_log_Send
(
    const le_log_Level_t level,
    const le_log_TraceRef_t traceRef,
    le_log_SessionRef_t logSession,
    const char* filenamePtr,
    const char* functionNamePtr,
    const unsigned int lineNumber,
    const char* formatPtr,
    ...
) __attribute__ ((format (printf, 7, 8)));

le_log_TraceRef_t _le_log_GetTraceRef
(
    le_log_SessionRef_t logSession,
    const char* keyword
);

void _le_log_SetFilterLevel
(
    le_log_SessionRef_t logSession,
    le_log_Level_t level
);


//--------------------------------------------------------------------------------------------------
/**
 * Filtering session reference for the current source file.
 *
 * @note The real name of this is passed in by the build system. This way it can be a unique
 *       variable for each component.
 */
//--------------------------------------------------------------------------------------------------
le_log_SessionRef_t LE_LOG_SESSION;

//--------------------------------------------------------------------------------------------------
/**
 * Filtering level for the current source file.
 *
 * @note The real name of this is passed in by the build system. This way it can be a unique
 *       variable for each component.
 */
//--------------------------------------------------------------------------------------------------
le_log_Level_t* LE_LOG_LEVEL_FILTER_PTR;

/* @endcond */

//--------------------------------------------------------------------------------------------------
/**
 * Internal macro to filter out messages that do not meet the current filtering level.
 */
//--------------------------------------------------------------------------------------------------
#define _LE_LOG_MSG(level, formatString, ...)  \
    if ((LE_LOG_LEVEL_FILTER_PTR == NULL) || (level >= *LE_LOG_LEVEL_FILTER_PTR))     \
    {                                          \
        _le_log_Send(level, NULL, LE_LOG_SESSION, __FILE__, __func__, __LINE__, \
                formatString, ##__VA_ARGS__);  \
    }


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
#define LE_DEBUG_IF(condition, formatString, ...)                                                   \
        if (condition) { _LE_LOG_MSG(LE_LOG_DEBUG, formatString, ##__VA_ARGS__) }
/** @ref LE_INFO if condition is met. */
#define LE_INFO_IF(condition, formatString, ...)                                                    \
        if (condition) { _LE_LOG_MSG(LE_LOG_INFO, formatString, ##__VA_ARGS__) }
/** @ref LE_WARN if condition is met. */
#define LE_WARN_IF(condition, formatString, ...)                                                    \
        if (condition) { _LE_LOG_MSG(LE_LOG_WARN, formatString, ##__VA_ARGS__) }
/** @ref LE_ERROR if condition is met. */
#define LE_ERROR_IF(condition, formatString, ...)                                                   \
        if (condition) { _LE_LOG_MSG(LE_LOG_ERR, formatString, ##__VA_ARGS__) }
/** @ref LE_CRIT if condition is met. */
#define LE_CRIT_IF(condition, formatString, ...)                                                    \
        if (condition) { _LE_LOG_MSG(LE_LOG_CRIT, formatString, ##__VA_ARGS__) }
/** @ref LE_EMERG if condition is met. */
#define LE_EMERG_IF(condition, formatString, ...)                                                   \
        if (condition) { _LE_LOG_MSG(LE_LOG_EMERG, formatString, ##__VA_ARGS__) }


//--------------------------------------------------------------------------------------------------
/**
 * Log fatal errors by killing the calling process after logging the message at EMERGENCY
 * level.  This macro never returns.
 *
 * Accepts printf-style arguments, consisting of a format string followed by zero or more parameters
 * to be printed (depending on the contents of the format string).
 */
//--------------------------------------------------------------------------------------------------
#define LE_FATAL(formatString, ...)                                                                 \
        { LE_EMERG(formatString, ##__VA_ARGS__); exit(EXIT_FAILURE); }


//--------------------------------------------------------------------------------------------------
/**
 * This macro does nothing if the condition is false, otherwise it logs the message at EMERGENCY
 * level and then kills the calling process.
 *
 * Accepts printf-style arguments, consisting of a format string followed by zero or more parameters
 * to be printed (depending on the contents of the format string).
 */
//--------------------------------------------------------------------------------------------------
#define LE_FATAL_IF(condition, formatString, ...)                                                   \
        if (condition) { LE_FATAL(formatString, ##__VA_ARGS__) }


//--------------------------------------------------------------------------------------------------
/**
 * This macro does nothing if the condition is true, otherwise it logs the condition expression as
 * a message at EMERGENCY level and then kills the calling process.
 */
//--------------------------------------------------------------------------------------------------
#define LE_ASSERT(condition)                                                                        \
        if (!(condition)) { LE_FATAL("Assert Failed: '%s'", #condition) }


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

/// Function that does the real work of translating result codes.  See @ref LE_RESULT_TXT.
const char* _le_log_GetResultCodeString
(
    le_result_t resultCode  ///< [in] Result code value to be translated.
);


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
#define LE_TRACE(traceRef, string, ...) \
        if (le_log_IsTraceEnabled(traceRef)) \
        { \
            _le_log_Send(-1, traceRef, LE_LOG_SESSION, __FILE__, __func__, __LINE__, \
                    string, ##__VA_ARGS__); \
        }


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to a trace keyword's settings.
 *
 * @return  Trace reference.
 **/
//--------------------------------------------------------------------------------------------------
static inline le_log_TraceRef_t le_log_GetTraceRef
(
    const char* keywordPtr      ///< [IN] Pointer to the keyword string.
)
{
    return _le_log_GetTraceRef(LE_LOG_SESSION, keywordPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Determines if a trace is currently enabled.
 *
 * @return  true if enabled, false if not.
 **/
//--------------------------------------------------------------------------------------------------
static inline bool le_log_IsTraceEnabled
(
    const le_log_TraceRef_t traceRef    ///< [IN] Trace reference obtained from le_log_GetTraceRef().
)
{
    return *((bool*)traceRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the log filter level for the calling component.
 *
 * @note    Normally not necessary as the log filter level can be controlled at
 *          runtime using the log control tool, and can be persistently configured.
 *          See @ref c_log_controlling.
 **/
//--------------------------------------------------------------------------------------------------
static inline void le_log_SetFilterLevel
(
    le_log_Level_t level    ///< [IN] Log filter level to apply to the current log session.
)
{
    _le_log_SetFilterLevel(LE_LOG_SESSION, level);
}


//--------------------------------------------------------------------------------------------------
/**
 * Enables a trace.
 *
 * @note    Normally, this is not necessary, since traces can be enabled at runtime using the
 *          log control tool and can be persistently configured.  See @ref c_log_controlling.
 **/
//--------------------------------------------------------------------------------------------------
static inline void le_log_EnableTrace
(
    const le_log_TraceRef_t traceRef    ///< [IN] Trace reference obtained from le_log_GetTraceRef().
)
{
    *((bool*)traceRef) = true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Disables a trace.
 *
 * @note    Normally, this is not necessary, since traces can be enabled at runtime using the
 *          log control tool and can be persistently configured.  See @ref c_log_controlling.
 **/
//--------------------------------------------------------------------------------------------------
static inline void le_log_DisableTrace
(
    const le_log_TraceRef_t traceRef    ///< [IN] Trace reference obtained from le_log_GetTraceRef()
)
{
    *((bool*)traceRef) = false;
}



#endif // LEGATO_LOG_INCLUDE_GUARD
