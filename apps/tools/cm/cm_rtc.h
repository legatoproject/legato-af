//-------------------------------------------------------------------------------------------------
/**
 * @file cm_rtc.h
 *
 * Handle RTC related functionality.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#ifndef CMODEM_RTC_INCLUDE_GUARD
#define CMODEM_RTC_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 *  Delta in seconds between Posix time epoch (01 Jan 1970) and GPS time epoch (06 Jan 1980)
 *  wihtout counting the leap seconds.
 *  leap seconds definition:
 *  UTC and GPS time deviate (on average) every 18 months by one additional second. This is called
 *  a leap second, introduced in UTC time base, necessary to adjust for changes in the
 *  earth's rotation. The International Atomic Time (TAI) adds the leap seconds to UTC time.
 *  To have the Delta in seconds between Posix time epoch TAI (01 Jan 1970) and GPS time epoch
 *  (06 Jan 1980), do: CM_DELTA_POSIX_TIME_EPOCH_GPS_TIME_EPOCH_IN_SEC + 19
 */
//--------------------------------------------------------------------------------------------------
#define  CM_DELTA_POSIX_TIME_EPOCH_GPS_TIME_EPOCH_IN_SEC   315964800

//--------------------------------------------------------------------------------------------------
/**
 *  Number of arguments for the CM tool RTC usage.
 */
//--------------------------------------------------------------------------------------------------
#define  CM_NUM_PARAMETERS_FOR_RTC_HELP   0
#define  CM_NUM_PARAMETERS_FOR_RTC_READ   0
#define  CM_NUM_PARAMETERS_FOR_RTC_SET    1

//-------------------------------------------------------------------------------------------------
/**
 * Print the RTC help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_rtc_PrintRtcHelp
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Process commands for RTC service.
 */
//--------------------------------------------------------------------------------------------------
void cm_rtc_ProcessRtcCommand
(
    const char * command,   ///< [IN] Command
    size_t numArgs          ///< [IN] Number of arguments
);

#endif

