/**
 * @file clock.c
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 *
 */

#include "legato.h"


// Microseconds should be less than this value.
// If greater than or equal, this indicates an overflow.
#define LIMIT_USEC (1000000)


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

    // todo: ARM compiler doesn't like CLOCK_BOOTTIME
    //if ( clock_gettime(CLOCK_BOOTTIME, &systemTime) < 0 )
    if ( clock_gettime(CLOCK_MONOTONIC, &systemTime) < 0 )
    {
        LE_FATAL("CLOCK_BOOTTIME is not supported for Relative time");
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
        return (timeA.usec > timeB.usec);

    return (timeA.sec > timeB.sec);
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
 *      - LE_OK if the formatted string was copied to destStr
 *      - LE_OVERFLOW if the formatted string would not fit in destStr
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FormatBrokenTime
(
    le_clk_Time_t absoluteTime,    ///< [IN]  Absolute time.
    struct tm brokenTime,          ///< [IN}  The broken-down time based on the absolute time.
    const char* formatSpecStr,     ///< [IN]  Format specifier string, using conversion
                                   ///        specifiers defined for strftime().
    char*   destStr,               ///< [OUT] Destination for the formatted date/time string
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
    size_t origIdx;
    size_t copyIdx;
    size_t numChars;
    int charsLeft;
    int charsPrinted;

    // Set to the default error value of 0.  This will get overwritten in the following code if
    // there are no errors.
    if (numBytesPtr != NULL)
        *numBytesPtr = 0;

    // Handle extra conversion specifications %J and %K for ms and us, respectively.
    origIdx = copyIdx = 0;
    while ( (formatSpecStr[origIdx] != '\0') && (copyIdx < copySize) )
    {
        if (formatSpecStr[origIdx] == '%')
        {
            charsLeft = copySize-copyIdx;

            // Check if this is a conversion specification that we need to handle
            switch ( formatSpecStr[origIdx+1] )
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
                    if ( (charsPrinted >= charsLeft) || (charsPrinted < 0) )
                    {
                        return LE_OVERFLOW;
                    }

                    origIdx += 2;
                    copyIdx += charsPrinted;
                    break;

                case '%':
                    // Preserve double %.  This needs to be handled here because it could precede
                    // a J or K. I.e., we don't want to interpret "%%J" as "%J" or "%%K" as "%K".
                    copyFormatSpecStr[copyIdx++] = formatSpecStr[origIdx++];
                    copyFormatSpecStr[copyIdx++] = formatSpecStr[origIdx++];
                    break;

                default:
                    copyFormatSpecStr[copyIdx++] = formatSpecStr[origIdx++];
                    break;
            }
        }
        else
        {
            copyFormatSpecStr[copyIdx++] = formatSpecStr[origIdx++];
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
    numChars = strftime(destStr, destSize, copyFormatSpecStr, &brokenTime);

    // Assume that numChars==0 always indicates an error.  Note that according to the documentation
    // for strftime(), this may not always be the case. However, it is very unlikely that a format
    // specifier string giving a zero length result will be used here, and so this uncommon case
    // will be ignored.
    if (numChars == 0)
        return LE_OVERFLOW;
    else
    {
        if (numBytesPtr != NULL)
            *numBytesPtr = numChars;

        return LE_OK;
    }

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
    const char* formatSpecStr,     ///< [IN]  Format specifier string, using conversion
                                   ///        specifiers defined for strftime().
    char*   destStr,               ///< [OUT] Destination for the formatted date/time string
    size_t  destSize,              ///< [IN]  Size of the destination buffer in bytes.
    size_t* numBytesPtr            ///< [OUT] Number of bytes copied, not including NULL-terminator.
                                   ///        This parameter can be set to NULL if the number of
                                   ///        bytes copied is not needed.

)
{
    struct tm brokenTime;
    le_clk_Time_t absTime;
    le_result_t result;

    // Get the time broken down into UTC year, month, day, and so on.
    absTime = le_clk_GetAbsoluteTime();
    if (gmtime_r(&absTime.sec, &brokenTime) == NULL)
    {
        LE_FATAL("Cannot convert Absolute time into UTC broken down time.");
    }

    result = FormatBrokenTime(absTime, brokenTime, formatSpecStr, destStr, destSize, numBytesPtr);

    return result;
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
 *      - LE_OK if the formatted string was copied to destStr
 *      - LE_OVERFLOW if the formatted string would not fit in destStr
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_clk_GetLocalDateTimeString
(
    const char* formatSpecStr,     ///< [IN]  Format specifier string, using conversion
                                   ///        specifiers defined for strftime().
    char*   destStr,               ///< [OUT] Destination for the formatted date/time string
    size_t  destSize,              ///< [IN]  Size of the destination buffer in bytes.
    size_t* numBytesPtr            ///< [OUT] Number of bytes copied, not including NULL-terminator.
                                   ///        This parameter can be set to NULL if the number of
                                   ///        bytes copied is not needed.
)
{
    struct tm brokenTime;
    le_clk_Time_t absTime;
    le_result_t result;

    // According to the documentation for localtime_r(), for portable code, tzset() should be
    // called before localtime_r().
    tzset();

    // Get the time broken down into local year, month, day, and so on.
    absTime = le_clk_GetAbsoluteTime();
    if (localtime_r(&absTime.sec, &brokenTime) == NULL)
    {
        LE_FATAL("Cannot convert Absolute time into local broken down time.");
    }

    result = FormatBrokenTime(absTime, brokenTime, formatSpecStr, destStr, destSize, numBytesPtr);

    return result;
}


