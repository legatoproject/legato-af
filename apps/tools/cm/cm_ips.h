//-------------------------------------------------------------------------------------------------
/**
 * @file cm_ips.h
 *
 * Handle IPS (input power supply) related functionality.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#ifndef CMODEM_IPS_INCLUDE_GUARD
#define CMODEM_IPS_INCLUDE_GUARD

//-------------------------------------------------------------------------------------------------
/**
 * Print the IPS help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_ips_PrintIpsHelp
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Process commands for IPS service.
 */
//--------------------------------------------------------------------------------------------------
void cm_ips_ProcessIpsCommand
(
    const char * command,   ///< [IN] Command
    size_t numArgs          ///< [IN] Number of arguments
);

#endif

