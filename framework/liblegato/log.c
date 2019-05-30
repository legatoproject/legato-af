/** @file log.c
 *
 * Logging facility functions which are platform-agnostic.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "log.h"

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the logging system.  This must be called VERY early in the process initialization.
 * Anything that is logged prior to this call will be logged with the wrong component name.
 */
//--------------------------------------------------------------------------------------------------
void log_Init
(
    void
)
{
    fa_log_Init();
}

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
)
{
    switch (level)
    {
        case LE_LOG_EMERG:
            return "*EMR*";
        case LE_LOG_CRIT:
            return "*CRT*";
        case LE_LOG_ERR:
            return "=ERR=";
        case LE_LOG_WARN:
            return "-WRN-";
        case LE_LOG_INFO:
            return " INFO";
        case LE_LOG_DEBUG:
            return " DBUG";
    }

    return " ??? ";
}

//--------------------------------------------------------------------------------------------------
/**
 * Translates a severity level string to the severity level value.  These strings are received from
 * the log control tool and are different from the strings that are used in the actual log messages.
 *
 * @return
 *      The severity level if successful.
 *      -1 if the string is an invalid log level.
 */
//--------------------------------------------------------------------------------------------------
le_log_Level_t log_StrToSeverityLevel
(
    const char* levelStr    // The severity level string.
)
{
    if (strcmp(levelStr, LOG_SET_LEVEL_EMERG_STR) == 0)
    {
        return LE_LOG_EMERG;
    }
    else if (strcmp(levelStr, LOG_SET_LEVEL_CRIT_STR) == 0)
    {
        return LE_LOG_CRIT;
    }
    else if (strcmp(levelStr, LOG_SET_LEVEL_ERROR_STR) == 0)
    {
        return LE_LOG_ERR;
    }
    else if (strcmp(levelStr, LOG_SET_LEVEL_WARN_STR) == 0)
    {
        return LE_LOG_WARN;
    }
    else if (strcmp(levelStr, LOG_SET_LEVEL_INFO_STR) == 0)
    {
        return LE_LOG_INFO;
    }
    else if (strcmp(levelStr, LOG_SET_LEVEL_DEBUG_STR) == 0)
    {
        return LE_LOG_DEBUG;
    }

    return -1;
}


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
    le_log_Level_t level    ///< [IN] Severity level.
)
{
    switch (level)
    {
        case LE_LOG_DEBUG:
            return LOG_SET_LEVEL_DEBUG_STR;

        case LE_LOG_INFO:
            return LOG_SET_LEVEL_INFO_STR;

        case LE_LOG_WARN:
            return LOG_SET_LEVEL_WARN_STR;

        case LE_LOG_ERR:
            return LOG_SET_LEVEL_ERROR_STR;

        case LE_LOG_CRIT:
            return LOG_SET_LEVEL_CRIT_STR;

        case LE_LOG_EMERG:
            return LOG_SET_LEVEL_EMERG_STR;
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get a null-terminated, printable string representing an le_result_t value.
 *
 * For example, LE_RESULT_TXT(LE_NOT_PERMITTED) would return a pointer to a string containing
 * "LE_NOT_PERMITTED".
 *
 * "(unknown)" will be returned if the value given is out of range.
 *
 * @return A pointer to a string constant.
 */
//--------------------------------------------------------------------------------------------------
const char* _le_log_GetResultCodeString
(
    le_result_t resultCode  ///< [in] The result code value to be translated.
)
{
    switch (resultCode)
    {
        case LE_OK:
            return "LE_OK";
        case LE_NOT_FOUND:
            return "LE_NOT_FOUND";
        case LE_NOT_POSSIBLE:
            return "LE_NOT_POSSIBLE";
        case LE_OUT_OF_RANGE:
            return "LE_OUT_OF_RANGE";
        case LE_NO_MEMORY:
            return "LE_NO_MEMORY";
        case LE_NOT_PERMITTED:
            return "LE_NOT_PERMITTED";
        case LE_FAULT:
            return "LE_FAULT";
        case LE_COMM_ERROR:
            return "LE_COMM_ERROR";
        case LE_TIMEOUT:
            return "LE_TIMEOUT";
        case LE_OVERFLOW:
            return "LE_OVERFLOW";
        case LE_UNDERFLOW:
            return "LE_UNDERFLOW";
        case LE_WOULD_BLOCK:
            return "LE_WOULD_BLOCK";
        case LE_DEADLOCK:
            return "LE_DEADLOCK";
        case LE_FORMAT_ERROR:
            return "LE_FORMAT_ERROR";
        case LE_DUPLICATE:
            return "LE_DUPLICATE";
        case LE_BAD_PARAMETER:
            return "LE_BAD_PARAMETER";
        case LE_CLOSED:
            return "LE_CLOSED";
        case LE_BUSY:
            return "LE_BUSY";
        case LE_UNSUPPORTED:
            return "LE_UNSUPPORTED";
        case LE_IO_ERROR:
            return "LE_IO_ERROR";
        case LE_NOT_IMPLEMENTED:
            return "LE_NOT_IMPLEMENTED";
        case LE_UNAVAILABLE:
            return "LE_UNAVAILABLE";
        case LE_TERMINATED:
            return "LE_TERMINATED";
        case LE_IN_PROGRESS:
            return "LE_IN_PROGRESS";
    }
    LE_ERROR("Result code %d out of range.", resultCode);
    return "(unknown)";
}

/// Function that exits in a race-free manner -- work around glibc BZ#14333
__attribute__((noreturn))
void _le_log_ExitFatal
(
    void
)
{
    static bool exitCalled = false;

    if (__atomic_test_and_set(&exitCalled, __ATOMIC_SEQ_CST))
    {
        pthread_exit(NULL);
    }

    exit(EXIT_FAILURE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Log data block. Provides a hex dump for debug
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
)
{
    char    buffer[100];    // enough to hold 16 printed bytes, plus ASCII equivalents
    char    c;
    int     i;
    int     j;
    int     numColumns;

    for (i = 0; i < dataLength; i += 16)
    {
        numColumns = dataLength - i;
        if (numColumns > 16)
        {
            numColumns = 16;
        }

        // Print the data as numbers
        for (j = 0; j < numColumns; ++j)
        {
            snprintf(&buffer[j * 3], sizeof(buffer) - j * 3, "%02X ", dataPtr[i + j]);
        }

        // Print extra spaces, if needed, plus separator at column 49
        snprintf(&buffer[numColumns * 3], sizeof(buffer) - numColumns * 3, "%*c: ",
            (16 - numColumns) * 3 + 1, ' ');

        // Print the data as characters, starting at column 51
        for (j = 0; j < numColumns; ++j)
        {
            c = dataPtr[i + j];
            if (!isprint(c))
            {
                c = '.';
            }
            snprintf(&buffer[51 + j], sizeof(buffer) - (51 + j), "%c", c);
        }

        do
        {
            if ((LE_LOG_LEVEL_FILTER_PTR == NULL) || (level >= *LE_LOG_LEVEL_FILTER_PTR))
            {
                _le_log_Send(level, NULL, LE_LOG_SESSION, filenamePtr, functionNamePtr,
                             lineNumber, "%s", buffer);
            }
        } while(0);
    }
}

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
)
{
    return fa_log_GetTraceRef(logSession, keyword);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the log filter level for the calling component.
 **/
//--------------------------------------------------------------------------------------------------
void _le_log_SetFilterLevel
(
    le_log_SessionRef_t logSession, ///< Log session.
    le_log_Level_t      level       ///< Log level to apply.
)
{
    fa_log_SetFilterLevel(logSession, level);
}

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
)
{
    va_list args;
    va_start(args, formatPtr);
    fa_log_Send(level, traceRef, logSession, filenamePtr, functionNamePtr, lineNumber, formatPtr,
        args);
    va_end(args);
}
