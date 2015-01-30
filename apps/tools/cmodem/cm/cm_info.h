// -------------------------------------------------------------------------------------------------
/**
 *  @file cm_info.h
 *
 *  Handle info related functionality
 *
 *  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#ifndef CMODEM_INFO_INCLUDE_GUARD
#define CMODEM_INFO_INCLUDE_GUARD

//-------------------------------------------------------------------------------------------------
/**
 *  Print the info help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_info_PrintInfoHelp
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Process commands for SMS service.
 */
//--------------------------------------------------------------------------------------------------
void cm_info_ProcessInfoCommand
(
    const char * command,   ///< [IN] Command
    size_t numArgs          ///< [IN] Number of arguments
);

#endif

