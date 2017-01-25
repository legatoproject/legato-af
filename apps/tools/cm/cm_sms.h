//-------------------------------------------------------------------------------------------------
/**
 * @file cm_sms.h
 *
 * Handle SMS related functionality
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#ifndef CMODEM_SMS_INCLUDE_GUARD
#define CMODEM_SMS_INCLUDE_GUARD

#define CMODEM_SMS_DEFAULT_MAX_BIN_SMS  5

//-------------------------------------------------------------------------------------------------
/**
 * Print the SMS help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_sms_PrintSmsHelp
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Process commands for SMS service.
 */
//--------------------------------------------------------------------------------------------------
void cm_sms_ProcessSmsCommand
(
    const char * command,   ///< [IN] Command
    size_t numArgs          ///< [IN] Number of arguments
);

#endif

