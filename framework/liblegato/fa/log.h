/** @file log.h
 *
 * Logging subsystem's framework adaptor API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef FA_LOG_H_INCLUDE_GUARD
#define FA_LOG_H_INCLUDE_GUARD

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the logging system.  This must be called VERY early in the process initialization.
 * Anything that is logged prior to this call will be logged with the wrong component name.
 */
//--------------------------------------------------------------------------------------------------
void fa_log_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to a trace keyword's settings.
 *
 * @return  Trace reference.
 **/
//--------------------------------------------------------------------------------------------------
le_log_TraceRef_t fa_log_GetTraceRef
(
    le_log_SessionRef_t  logSession,    ///< Log session.
    const char          *keyword        ///< Trace keyword.
);

//--------------------------------------------------------------------------------------------------
/**
 * Sets the log filter level for a given log session in the calling process.
 *
 * @note This does not affect other processes and does not update the Log Control Daemon.
 **/
//--------------------------------------------------------------------------------------------------
void fa_log_SetFilterLevel
(
    le_log_SessionRef_t logSession, ///< Log session.
    le_log_Level_t      level       ///< Log level.
);

//--------------------------------------------------------------------------------------------------
/**
 * Builds the log message and sends it to the logging system.
 */
//--------------------------------------------------------------------------------------------------
void fa_log_Send
(
    const le_log_Level_t     level,             // The severity level. Set to -1 if this is a Trace
                                                // log.
    const le_log_TraceRef_t  traceRef,          // The Trace reference. Set to NULL if this is not a
                                                // Trace log.
    le_log_SessionRef_t      logSession,        // The log session.
    const char              *filenamePtr,       // The name of the source file that logged the
                                                // message.
    const char              *functionNamePtr,   // The name of the function that logged the message.
    const unsigned int       lineNumber,        // The line number in the source file that logged
                                                // the message.
    const char              *formatPtr,         // The user message format.
    va_list                  args               // Positional parameters.
);

#endif /* end FA_LOG_H_INCLUDE_GUARD */
