/**
 * @page c_clock System Clock API
 *
 * @subpage le_clock.h "API Reference"
 *
 * <HR>
 *
 * This module provides an API for getting/setting date and/or time values, and
 * performing conversions between these values.
 *
 * @todo This API is currently incomplete, as it only provides a subset of functions that will
 * eventually be supported.
 *
 *
 * @section clk_time Getting/Setting Time
 *
 * Time values can either be absolute or relative.  Time is expressed in seconds
 * plus microseconds, and does not stop when the system is suspended (i.e., the clock continues to
 * run even when the system is suspended).
 *
 * Absolute time is given as time since the Epoch, 1970-01-01 00:00:00 +0000 (UTC) and is provided
 * by @ref le_clk_GetAbsoluteTime(). By definition, it is UTC time. The absolute time may jump
 * forward or backward if a new value is set for the absolute time. Absolute time can be set by
 * unsandboxed applications using le_clk_SetAbsoluteTime().
 *
 * Relative time is a monotonic time from a fixed but unspecified starting point and is provided
 * by @ref le_clk_GetRelativeTime(). The relative time is independent of the absolute time.  The
 * starting point is fixed during system boot, and cannot be changed, but is reset on each system
 * boot. Since the relative time is monotonic, it is guaranteed to never go backwards. With these
 * characteristics, the relative time is useful for measuring the time between two or more events.
 * For example, at event 1, relative time A is stored, and at some later event 2, relative time B
 * is stored.  The relative time between these two events can always be calculated as B-A, and will
 * always be an accurate measure of the relative time between these two events.
 *
 *
 * @section clk_values Operations on Time Values
 *
 * These operations can be performed on time values:
 *  - @ref le_clk_Add
 *  - @ref le_clk_GreaterThan
 *  - @ref le_clk_Equal
 *  - @ref le_clk_Sub
 *  - @ref le_clk_Multiply
 *
 * The functions use these assumptions:
 *  - All input time values are normalized (i.e., the usec value is less than 1 sec).
 *    All time values returned are normalized.
 *  - All input time values or scale factors are positive; a negative time value will not be
 *    returned.
 *  - All input time values or scale factors are expected to have reasonable values
 *    (i.e., they will not be so large as to cause an overflow of the time value structure).
 *
 *
 * @section clk_convert Converting Time to/from Other Formats
 *
 * The current absolute time can be converted to a formatted string in either UTC time or local
 * time, using @ref le_clk_GetUTCDateTimeString() or @ref le_clk_GetLocalDateTimeString()
 * respectively. These functions use the format specification defined for strftime(), with the
 * following additional conversion specifications:
 *  - %%J : milliseconds, as a 3 digit zero-padded string, e.g. "015"
 *  - %%K : microseconds, as a 6 digit zero-padded string, e.g. "001015"
 *
 * The absolute time can be set with a formatted string in UTC time, using
 * le_clk_SetUTCDateTimeString().
 *
 * @note The additional format specifications %%J and %%K are not supported by
 * le_clk_SetUTCDateTimeString().
 *
 * @todo
 *  - Add new formatting object to allow arbitrary time to be converted to a string, potentially
 *    with additional formatting options.
 *  - Add new objects and/or APIs to allow converting time to other formats, e.g. Linux broken down
 *    time in "struct tm" format.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

//--------------------------------------------------------------------------------------------------
/** @file le_clock.h
 *
 * Legato @ref c_clock include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_CLK_INCLUDE_GUARD
#define LEGATO_CLK_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Represents time in seconds/microseconds.  Can be relative or absolute.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    time_t sec;     ///< seconds
    long   usec;    ///< microseconds
} le_clk_Time_t;


//--------------------------------------------------------------------------------------------------
/** @name String Formats
 *
 * Pre-defined formats for converting time to string format. These pre-defined formats use the
 * conversion specifiers from strftime().
 * @{
 */
//--------------------------------------------------------------------------------------------------
#define LE_CLK_STRING_FORMAT_DATE_TIME "%c"
    ///< Preferred date and time format for current locale, e.g. "Mon Jan 21 13:37:05 2013"
#define LE_CLK_STRING_FORMAT_DATE     "%x"
    ///< Preferred date format for current locale, e.g. "01/21/13"
#define LE_CLK_STRING_FORMAT_TIME     "%X"
    ///< Preferred time format for current locale, e.g. "13:37:05"
// @}


//--------------------------------------------------------------------------------------------------
/**
 * Get relative time since a fixed but unspecified starting point.
 *
 * @return
 *      Relative time in seconds/microseconds
 *
 * @note
 *      Relative time includes any time that the processor is suspended.
 */
//--------------------------------------------------------------------------------------------------
le_clk_Time_t le_clk_GetRelativeTime(void);


//--------------------------------------------------------------------------------------------------
/**
 * Get absolute time since the Epoch, 1970-01-01 00:00:00 +0000 (UTC).
 *
 * @return
 *      Absolute time in seconds/microseconds
 *
 * @note
 *      Absolute time includes any time that the processor is suspended.
 */
//--------------------------------------------------------------------------------------------------
le_clk_Time_t le_clk_GetAbsoluteTime(void);


//--------------------------------------------------------------------------------------------------
/**
 * Add two time values together, and return the result.
 *
 * @return
 *      Sum of the two time values
 */
//--------------------------------------------------------------------------------------------------
le_clk_Time_t le_clk_Add
(
    le_clk_Time_t timeA,
    le_clk_Time_t timeB
);


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
);


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
);


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
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the UTC date/time as a formatted string.
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
le_result_t le_clk_GetUTCDateTimeString
(
    const char* formatSpecStrPtr,  ///< [IN]  Format specifier string, using conversion
                                   ///        specifiers defined for strftime().
    char*   destStrPtr,            ///< [OUT] Destination for the formatted date/time string
    size_t  destSize,              ///< [IN]  Size of the destination buffer in bytes.
    size_t* numBytesPtr            ///< [OUT] Number of bytes copied, not including NULL-terminator.
                                   ///        Parameter can be set to NULL if the number of
                                   ///        bytes copied is not needed.

);


//--------------------------------------------------------------------------------------------------
/**
 * Get the Local date/time as a formatted string.
 *
 * The formatted date/time string, including NULL-terminator, will be copied to the destination
 * buffer, provided it fits, and the number of bytes copied (not including the NULL-terminator)
 * will be returned in numBytesPtr.
 *
 * If the formatted date/time string does not fit in the destination buffer, then the contents of
 * the destination buffer will be undefined, and the value returned in numBytesPtr will be zero.
 *
 * @return
 *      - LE_OK if the formatted string was copied to destStrPtr.
 *      - LE_OVERFLOW if the formatted string would not fit in destStrPtr.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_clk_GetLocalDateTimeString
(
    const char* formatSpecStrPtr,  ///< [IN]  Format specifier string, using conversion
                                   ///        specifiers defined for strftime().
    char*   destStrPtr,            ///< [OUT] Destination for the formatted date/time string
    size_t  destSize,              ///< [IN]  Size of the destination buffer in bytes.
    size_t* numBytesPtr            ///< [OUT] Number of bytes copied, not including NULL-terminator.
                                   ///        Parameter can be set to NULL if the number of
                                   ///        bytes copied is not needed.
);


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

);


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

);


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
);


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
LE_FULL_API le_result_t le_clk_ConvertToTime
(
    const char*    formatSpecStrPtr,    ///< [IN]  Format specifier string, using conversion
                                        ///        specifiers defined for strptime().
    const char*    srcStrPtr,           ///< [IN]  Formatted date/time string.
    le_clk_Time_t* timePtr              ///< [OUT] Converted date/time.
);


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
LE_FULL_API le_result_t le_clk_SetUTCDateTimeString
(
    const char* formatSpecStrPtr,   ///< [IN] Format specifier string, using conversion
                                    ///       specifiers defined for strptime().
    const char* srcStrPtr           ///< [IN] Formatted date/time string.
);


#endif // LEGATO_CLK_INCLUDE_GUARD
