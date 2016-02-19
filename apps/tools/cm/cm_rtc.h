//-------------------------------------------------------------------------------------------------
/**
 * @file cm_rtc.h
 *
 * Handle RTC related functionality.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//-------------------------------------------------------------------------------------------------

#ifndef CMODEM_RTC_INCLUDE_GUARD
#define CMODEM_RTC_INCLUDE_GUARD

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

