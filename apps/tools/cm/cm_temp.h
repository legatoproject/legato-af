//-------------------------------------------------------------------------------------------------
/**
 * @file cm_temp.h
 *
 * Handle temperature related functionality
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#ifndef CMODEM_TEMP_INCLUDE_GUARD
#define CMODEM_TEMP_INCLUDE_GUARD

//-------------------------------------------------------------------------------------------------
/**
 * Print the temp help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_temp_PrintTempHelp
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Process commands for temp service.
 */
//--------------------------------------------------------------------------------------------------
void cm_temp_ProcessTempCommand
(
    const char * command,   ///< [IN] Command
    size_t numArgs          ///< [IN] Number of arguments
);

#endif

