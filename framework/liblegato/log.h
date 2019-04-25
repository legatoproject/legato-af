/** @file log.h
 *
 * Log module's intra-framework header file.  This file exposes type definitions and function
 * interfaces to other modules inside the framework implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LOG_INCLUDE_GUARD
#define LOG_INCLUDE_GUARD

#include "fa/log.h"

//--------------------------------------------------------------------------------------------------
/**
 * Default logging level for sessions when they are first created.
 **/
//--------------------------------------------------------------------------------------------------
#define LOG_DEFAULT_LOG_FILTER      LE_LOG_INFO

// =======================================================
//  LOG LEVELS (CommandData part of SET_LEVEL commands)
// =======================================================

#define LOG_SET_LEVEL_EMERG_STR "EMERGENCY"
#define LOG_SET_LEVEL_CRIT_STR  "CRITICAL"
#define LOG_SET_LEVEL_ERROR_STR "ERROR"
#define LOG_SET_LEVEL_WARN_STR  "WARNING"
#define LOG_SET_LEVEL_INFO_STR  "INFO"
#define LOG_SET_LEVEL_DEBUG_STR "DEBUG"

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the logging system.  This must be called VERY early in the process initialization.
 * Anything that is logged prior to this call will be logged with the wrong component name.
 */
//--------------------------------------------------------------------------------------------------
void log_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Convert log level enum values to strings suitable for message logging.
 *
 * @return Severity string.
 */
//--------------------------------------------------------------------------------------------------
const char *log_GetSeverityStr
(
    le_log_Level_t level    ///< Severity level.
);

//--------------------------------------------------------------------------------------------------
/**
 * Translates a severity level string to a severity level value.
 *
 * @return
 *      The severity level if successful.
 *      -1 if the string is an invalid log level.
 */
//--------------------------------------------------------------------------------------------------
le_log_Level_t log_StrToSeverityLevel
(
    const char *levelStr    ///< The severity level string.
);

//--------------------------------------------------------------------------------------------------
/**
 * Translates a severity level value to a severity level string.
 *
 * @return
 *      Pointer to a string constant containing the severity level string.
 *      NULL if the value is out of range.
 */
//--------------------------------------------------------------------------------------------------
const char* log_SeverityLevelToStr
(
    le_log_Level_t level    ///< Severity level.
);

#endif // LOG_INCLUDE_GUARD
