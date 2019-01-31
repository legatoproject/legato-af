/**
 * @file clock.c
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "timer.h"


// Microseconds should be less than this value.
// If greater than or equal, this indicates an overflow.
#define LIMIT_USEC (1000000)

// =============================================
//  PRIVATE FUNCTIONS
// =============================================


//--------------------------------------------------------------------------------------------------
/**
 * Get relative time since some fixed but unspecified starting point. However, retrieve the time
 * based on the desired wakeup behaviour.
   IsWakeup: False - Use non-waking clock. Otherwise not.
 *
 * @return
 *      Relative time in seconds/microseconds
 *
 */
//--------------------------------------------------------------------------------------------------
le_clk_Time_t clk_GetRelativeTime(bool IsWakeup)
{
    struct timespec systemTime;
    le_clk_Time_t relativeTime;

    if (!IsWakeup)
    {
        // Use a clock coherent with timerfd functions.
        if (0 > clock_gettime(CLOCK_MONOTONIC, &systemTime))
        {
            LE_FATAL("clock_gettime() failed. errno = %d", errno);
        }
    }
    else
    {
        // Use a clock coherent with timerfd functions.
        if (0 > clock_gettime(timer_GetClockType(), &systemTime))
        {
            LE_FATAL("clock_gettime() failed. errno = %d", errno);
        }
    }

    relativeTime.sec = systemTime.tv_sec;
    relativeTime.usec = systemTime.tv_nsec/1000;

    return relativeTime;
}

// =============================================
//  PUBLIC API FUNCTIONS
// =============================================


//--------------------------------------------------------------------------------------------------
/**
 * Get relative time since some fixed but unspecified starting point.
 *
 * @return
 *      The relative time in seconds/microseconds
 *
 * @note
 *      - The relative time includes any time that the processor is suspended.
 *      - It is a fatal error if the relative time cannot be returned
 */
//--------------------------------------------------------------------------------------------------
le_clk_Time_t le_clk_GetRelativeTime(void)
{
    struct timespec systemTime;
    le_clk_Time_t relativeTime;

    // Use a clock coherent with timerfd functions.
    if (0 > clock_gettime(timer_GetClockType(), &systemTime))
    {
        LE_FATAL("clock_gettime() failed. errno = %d", errno);
    }

    relativeTime.sec = systemTime.tv_sec;
    relativeTime.usec = systemTime.tv_nsec/1000;

    return relativeTime;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get absolute time since the Epoch, 1970-01-01 00:00:00 +0000 (UTC).
 *
 * @return
 *      The absolute time in seconds/microseconds
 *
 * @note
 *      - The absolute time includes any time that the processor is suspended.
 *      - It is a fatal error if the absolute time cannot be returned
 */
//--------------------------------------------------------------------------------------------------
le_clk_Time_t le_clk_GetAbsoluteTime(void)
{
    struct timespec systemTime;
    le_clk_Time_t absoluteTime;

    if ( clock_gettime(CLOCK_REALTIME, &systemTime) < 0 )
    {
        LE_FATAL("CLOCK_REALTIME is not supported for Absolute time");
    }

    absoluteTime.sec = systemTime.tv_sec;
    absoluteTime.usec = systemTime.tv_nsec/1000;

    return absoluteTime;
}


//--------------------------------------------------------------------------------------------------
/**
 * Add two time values together, and return the result.
 *
 * @return
 *      The sum of the two time values
 */
//--------------------------------------------------------------------------------------------------
le_clk_Time_t le_clk_Add
(
    le_clk_Time_t timeA,
    le_clk_Time_t timeB
)
{
    le_clk_Time_t result;

    result.sec = timeA.sec + timeB.sec;
    result.usec = timeA.usec + timeB.usec;

    // Handle overflow of usec
    if (result.usec >= LIMIT_USEC)
    {
        result.usec -= LIMIT_USEC;
        result.sec++;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Compare two time values.
 *
 * @return
 *      - TRUE if TimeA > TimeB
 *      - FALSE otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_clk_GreaterThan
(
    le_clk_Time_t timeA,
    le_clk_Time_t timeB
)
{
    // Only compare usec if the sec are the same
    if (timeA.sec == timeB.sec)
    {
        return (timeA.usec > timeB.usec);
    }

    return (timeA.sec > timeB.sec);
}


//--------------------------------------------------------------------------------------------------
/**
 * Compare two time values.
 *
 * @return
 *      - TRUE if TimeA == TimeB
 *      - FALSE otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_clk_Equal
(
    le_clk_Time_t timeA,
    le_clk_Time_t timeB
)
{
    return (timeA.sec == timeB.sec) &&
        (timeA.usec == timeB.usec);
}


//--------------------------------------------------------------------------------------------------
/**
 * Subtract two time values, and return the result.
 *
 * @return
 *      Result of (timeA - timeB)
 */
//--------------------------------------------------------------------------------------------------
le_clk_Time_t le_clk_Sub
(
    le_clk_Time_t timeA,
    le_clk_Time_t timeB
)
{
    le_clk_Time_t result;

    result.sec = timeA.sec - timeB.sec;
    if ( timeA.usec < timeB.usec )
    {
        // borrow from the seconds
        result.sec--;
        timeA.usec += LIMIT_USEC;
    }
    result.usec = timeA.usec - timeB.usec;

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Multiply the time by a scale factor, and return the result
 *
 * @return
 *      Time multiplied by scale factor
 */
//--------------------------------------------------------------------------------------------------
le_clk_Time_t le_clk_Multiply
(
    le_clk_Time_t timeA,
    int scaleFactor
)
{
    le_clk_Time_t result;

    result.sec = scaleFactor * timeA.sec;
    result.usec = scaleFactor * timeA.usec;

    // Handle overflow of usec
    if (result.usec >= LIMIT_USEC)
    {
        result.usec = result.usec % LIMIT_USEC;
        result.sec += result.usec / LIMIT_USEC;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Convert broken down time into a formatted string.
 *
 * The formatted date/time string, including NULL-terminator, will be copied to the destination
 * buffer, provided it fits, and the number of bytes copied (not including the NULL-terminator)
 * will be returned in numBytesPtr.
 *
 * If the formatted date/time string does not fit in the destination buffer, then the contents of
 * the destination buffer are undefined, and the value returned in numBytesPtr is zero.
 *
 * @return
 *      - LE_OK if the formatted string was copied to destStrPtr
 *      - LE_OVERFLOW if the formatted string would not fit in destStrPtr
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FormatBrokenTime
(
    le_clk_Time_t absoluteTime,    ///< [IN]  Absolute time.
    struct tm brokenTime,          ///< [IN]  The broken-down time based on the absolute time.
    const char* formatSpecStrPtr,  ///< [IN]  Format specifier string, using conversion
                                   ///        specifiers defined for strftime().
    char*   destStrPtr,            ///< [OUT] Destination for the formatted date/time string
    size_t  destSize,              ///< [IN]  Size of the destination buffer in bytes.
    size_t* numBytesPtr            ///< [OUT] Number of bytes copied, not including NULL-terminator.
                                   ///        This parameter can be set to NULL if the number of
                                   ///        bytes copied is not needed.
)
{
    // Temporary buffer to hold formatSpec after processing %J and %K.  Since the formatted output
    // should fit in destStr, then destSize is a reasonable upper limit for the temp buffer size.
    size_t copySize = destSize;
    char copyFormatSpecStr[copySize];
    size_t origIdx = 0;
    size_t copyIdx = 0;
    size_t numChars;
    int charsLeft;
    int charsPrinted;

    // Set to the default error value of 0.  This will get overwritten in the following code if
    // there are no errors.
    if (numBytesPtr != NULL)
    {
        *numBytesPtr = 0;
    }

    // Handle extra conversion specifications %J and %K for ms and us, respectively.
    while ((formatSpecStrPtr[origIdx] != '\0') && (copyIdx < copySize))
    {
        if ('%' == formatSpecStrPtr[origIdx])
        {
            charsLeft = copySize-copyIdx;

            // Check if this is a conversion specification that we need to handle
            switch (formatSpecStrPtr[origIdx+1])
            {
                case 'J':
                    // fill in ms
                    charsPrinted = snprintf(&copyFormatSpecStr[copyIdx],
                                            charsLeft, "%03li", absoluteTime.usec/1000);

                    // Check if output was truncated, or some other error occurred.
                    if ( (charsPrinted >= charsLeft) || (charsPrinted < 0) )
                    {
                        return LE_OVERFLOW;
                    }

                    origIdx += 2;
                    copyIdx += charsPrinted;
                    break;

                case 'K':
                    // fill in us
                    charsPrinted = snprintf(&copyFormatSpecStr[copyIdx],
                                            charsLeft, "%06li", absoluteTime.usec);

                    // Check if output was truncated, or some other error occurred.
                    if ((charsPrinted >= charsLeft) || (charsPrinted < 0))
                    {
                        return LE_OVERFLOW;
                    }

                    origIdx += 2;
                    copyIdx += charsPrinted;
                    break;

                case '%':
                    // Preserve double %.  This needs to be handled here because it could precede
                    // a J or K. I.e., we don't want to interpret "%%J" as "%J" or "%%K" as "%K".
                    copyFormatSpecStr[copyIdx++] = formatSpecStrPtr[origIdx++];
                    copyFormatSpecStr[copyIdx++] = formatSpecStrPtr[origIdx++];
                    break;

                default:
                    copyFormatSpecStr[copyIdx++] = formatSpecStrPtr[origIdx++];
                    break;
            }
        }
        else
        {
            copyFormatSpecStr[copyIdx++] = formatSpecStrPtr[origIdx++];
        }
    }

    // Check for overflow while copying the format specifier string.
    if (copyIdx >= copySize)
    {
        return LE_OVERFLOW;
    }

    // Ensure the copy is null-terminated, since the above while loop will not do it.  Note that we
    // are guaranteed that copyIdx is valid.
    copyFormatSpecStr[copyIdx] = '\0';

    // Process the standard conversion specifiers.
    numChars = strftime(destStrPtr, destSize, copyFormatSpecStr, &brokenTime);

    // Assume that numChars==0 always indicates an error.  Note that according to the documentation
    // for strftime(), this may not always be the case. However, it is very unlikely that a format
    // specifier string giving a zero length result will be used here, and so this uncommon case
    // will be ignored.
    if (0 == numChars)
    {
        return LE_OVERFLOW;
    }

    if (numBytesPtr != NULL)
    {
        *numBytesPtr = numChars;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the UTC date/time as a formatted string.
 *
 * The formatted date/time string, including NULL-terminator, will be copied to the destination
 * buffer, provided it fits, and the number of bytes copied (not including the NULL-terminator)
 * will be returned in numBytesPtr.
 *
 * If the formatted date/time string does not fit in the destination buffer, then the contents of
 * the destination buffer are undefined, and the value returned in numBytesPtr is zero.
 *
 * @return
 *      - LE_OK if the formatted string was copied to destStr
 *      - LE_OVERFLOW if the formatted string would not fit in destStr
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_clk_GetUTCDateTimeString
(
    const char* formatSpecStrPtr,  ///< [IN]  Format specifier string, using conversion
                                   ///        specifiers defined for strftime().
    char*   destStrPtr,            ///< [OUT] Destination for the formatted date/time string
    size_t  destSize,              ///< [IN]  Size of the destination buffer in bytes.
    size_t* numBytesPtr            ///< [OUT] Number of bytes copied, not including NULL-terminator.
                                   ///        This parameter can be set to NULL if the number of
                                   ///        bytes copied is not needed.

)
{
    le_clk_Time_t absTime;

    // Get the time broken down into UTC year, month, day, and so on.
    absTime = le_clk_GetAbsoluteTime();

    return le_clk_ConvertToUTCString(absTime,formatSpecStrPtr,destStrPtr,destSize,numBytesPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Local date/time as a formatted string.
 *
 * The formatted date/time string, including NULL-terminator, will be copied to the destination
 * buffer, provided it fits, and the number of bytes copied (not including the NULL-terminator)
 * will be returned in numBytesPtr.
 *
 * If the formatted date/time string does not fit in the destination buffer, then the contents of
 * the destination buffer are undefined, and the value returned in numBytesPtr is zero.
 *
 * @return
 *      - LE_OK if the formatted string was copied to destStrPtr
 *      - LE_OVERFLOW if the formatted string would not fit in destStrPtr
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_clk_GetLocalDateTimeString
(
    const char* formatSpecStrPtr,  ///< [IN]  Format specifier string, using conversion
                                   ///        specifiers defined for strftime().
    char*   destStrPtr,            ///< [OUT] Destination for the formatted date/time string
    size_t  destSize,              ///< [IN]  Size of the destination buffer in bytes.
    size_t* numBytesPtr            ///< [OUT] Number of bytes copied, not including NULL-terminator.
                                   ///        This parameter can be set to NULL if the number of
                                   ///        bytes copied is not needed.
)
{
    le_clk_Time_t absTime;

    // Get the time broken down into local year, month, day, and so on.
    absTime = le_clk_GetAbsoluteTime();

    return le_clk_ConvertToLocalTimeString(absTime,
                                           formatSpecStrPtr,
                                           destStrPtr,
                                           destSize,
                                           numBytesPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate a printable string representation of a given absolute date/time value as UTC time
 * (no timezone offset applied).
 *
 * The formatted date/time string, including NULL-terminator, will be copied to the destination
 * buffer, provided it fits, and the number of bytes copied (not including the NULL-terminator)
 * will be returned in numBytesPtr.
 *
 * If the formatted date/time string does not fit in the destination buffer, the contents of
 * the destination buffer will be undefined and the value returned in numBytesPtr will be zero.
 *
 * @return
 *      - LE_OK if the formatted string was copied to destStrPtr
 *      - LE_OVERFLOW if the formatted string would not fit in destStrPtr
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_clk_ConvertToUTCString
(
    le_clk_Time_t time,            ///< [IN]  date/time to convert
    const char* formatSpecStrPtr,  ///< [IN]  Format specifier string, using conversion
                                   ///        specifiers defined for strftime().
    char*   destStrPtr,            ///< [OUT] Destination for the formatted date/time string
    size_t  destSize,              ///< [IN]  Size of the destination buffer in bytes.
    size_t* numBytesPtr            ///< [OUT] Number of bytes copied, not including NULL-terminator.
                                   ///        Parameter can be set to NULL if the number of
                                   ///        bytes copied is not needed.

)
{
    struct tm brokenTime;
    le_result_t result;

    if (gmtime_r(&time.sec, &brokenTime) == NULL)
    {
        LE_FATAL("Cannot convert time into UTC broken down time.");
    }

    result = FormatBrokenTime(time,
                              brokenTime,
                              formatSpecStrPtr,
                              destStrPtr,
                              destSize,
                              numBytesPtr);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate a printable string representation of a given absolute date/time value as a local time
 * (with timezone offset applied).
 *
 * The formatted date/time string, including NULL-terminator, will be copied to the destination
 * buffer, provided it fits, and the number of bytes copied (not including the NULL-terminator)
 * will be returned in numBytesPtr.
 *
 * If the formatted date/time string does not fit in the destination buffer, then the contents of
 * the destination buffer are undefined, and the value returned in numBytesPtr is zero.
 *
 * @return
 *      - LE_OK if the formatted string was copied to destStrPtr
 *      - LE_OVERFLOW if the formatted string would not fit in destStrPtr
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_clk_ConvertToLocalTimeString
(
    le_clk_Time_t time,            ///< [IN]  date/time to convert
    const char* formatSpecStrPtr,  ///< [IN]  Format specifier string, using conversion
                                   ///        specifiers defined for strftime().
    char*   destStrPtr,            ///< [OUT] Destination for the formatted date/time string
    size_t  destSize,              ///< [IN]  Size of the destination buffer in bytes.
    size_t* numBytesPtr            ///< [OUT] Number of bytes copied, not including NULL-terminator.
                                   ///        Parameter can be set to NULL if the number of
                                   ///        bytes copied is not needed.

)
{
    struct tm brokenTime;
    le_result_t result;
    // According to the documentation for localtime_r(), for portable code, tzset() should be
    // called before localtime_r().
    tzset();

    if (localtime_r(&time.sec, &brokenTime) == NULL)
    {
        LE_FATAL("Cannot convert Absolute time into local broken down time.");
    }

    result = FormatBrokenTime(time,
                              brokenTime,
                              formatSpecStrPtr,
                              destStrPtr,
                              destSize,
                              numBytesPtr);

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set absolute time since the Epoch, 1970-01-01 00:00:00 +0000 (UTC).
 *
 * @note Only unsandboxed application can set the date/time.
 *
 * @return
 *      - LE_OK if the function succeeded
 *      - LE_BAD_PARAMETER if an invalid parameter is provided
 *      - LE_NOT_PERMITTED if the operation is not permitted
 *      - LE_FAULT if an error occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_clk_SetAbsoluteTime
(
    le_clk_Time_t absoluteTime  ///< [IN] Absolute time in seconds/microseconds
)
{
    struct timespec systemTime;
    systemTime.tv_sec = absoluteTime.sec;
    systemTime.tv_nsec = absoluteTime.usec * 1000;

    if (clock_settime(CLOCK_REALTIME, &systemTime) < 0)
    {
        switch (errno)
        {
            case EPERM:
                LE_ERROR("Setting CLOCK_REALTIME for Absolute time is not permitted");
                return LE_NOT_PERMITTED;

            case EINVAL:
                LE_ERROR("Invalid parameter to set CLOCK_REALTIME for Absolute time");
                return LE_BAD_PARAMETER;

            default:
                LE_ERROR("Unable to set CLOCK_REALTIME for Absolute time (errno = %d)", errno);
                return LE_FAULT;
        }
    }

    return LE_OK;
}


#if LE_CONFIG_LINUX
//--------------------------------------------------------------------------------------------------
/**
 * Generate an absolute date/time value as UTC time representation of a given printable string
 * representation (no timezone offset applied).
 *
 * @return
 *      - LE_OK if the conversion was successful
 *      - LE_BAD_PARAMETER if an invalid parameter is provided
 *      - LE_FAULT if an error occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_clk_ConvertToTime
(
    const char*    formatSpecStrPtr,    ///< [IN]  Format specifier string, using conversion
                                        ///        specifiers defined for strptime().
    const char*    srcStrPtr,           ///< [IN]  Formatted date/time string.
    le_clk_Time_t* timePtr              ///< [OUT] Converted date/time.
)
{
    struct tm brokenTime = {0};


    if (   (!formatSpecStrPtr) || ('\0' == formatSpecStrPtr[0])
        || (!srcStrPtr) || ('\0' == srcStrPtr[0]))
    {
        LE_ERROR("Incorrect input parameter");
        return LE_BAD_PARAMETER;
    }

    if (!timePtr)
    {
        LE_ERROR("Incorrect output parameter");
        return LE_BAD_PARAMETER;
    }

    // Convert the string into a broken time structure
    if (!strptime(srcStrPtr, formatSpecStrPtr, &brokenTime))
    {
        LE_ERROR("strptime error");
        return LE_FAULT;
    }

    // If date is not set, set it to the Epoch date for mktime()
    if ((!brokenTime.tm_year) && (!brokenTime.tm_mon) && (!brokenTime.tm_mday))
    {
        brokenTime.tm_year = 70;
        brokenTime.tm_mon  = 1;
        brokenTime.tm_mday = 1;
    }

    // Convert to a simple time representation
    timePtr->sec = mktime(&brokenTime);
    if (-1 == timePtr->sec)
    {
        LE_ERROR("mktime error");
        return LE_FAULT;
    }
    timePtr->usec = 0;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the UTC date/time as a formatted string.
 *
 * @note Only unsandboxed application can set the date/time.
 *
 * @return
 *      - LE_OK if the time is correctly set
 *      - LE_BAD_PARAMETER if an invalid parameter is provided
 *      - LE_NOT_PERMITTED if the operation is not permitted
 *      - LE_FAULT if an error occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_clk_SetUTCDateTimeString
(
    const char* formatSpecStrPtr,   ///< [IN] Format specifier string, using conversion
                                    ///       specifiers defined for strptime().
    const char* srcStrPtr           ///< [IN] Formatted date/time string.
)
{
    le_clk_Time_t newAbsTime;

    if (   (!formatSpecStrPtr) || ('\0' == formatSpecStrPtr[0])
        || (!srcStrPtr) || ('\0' == srcStrPtr[0]))
    {
        LE_ERROR("Incorrect input parameter");
        return LE_BAD_PARAMETER;
    }

    newAbsTime.sec = 0;
    newAbsTime.usec = 0;

    if (LE_OK != le_clk_ConvertToTime(formatSpecStrPtr, srcStrPtr, &newAbsTime))
    {
        return LE_FAULT;
    }

    return le_clk_SetAbsoluteTime(newAbsTime);
}
#endif
