//-------------------------------------------------------------------------------------------------
/**
 * @file cm_mdmCfg.h
 *
 * Handle MDM configuration related functionality
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#ifndef CMODEM_MDMCFG_INCLUDE_GUARD
#define CMODEM_MDMCFG_INCLUDE_GUARD

//-------------------------------------------------------------------------------------------------
/**
 * Print the mdmCfg help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_mdmCfg_PrintHelp
(
    void
);

///--------------------------------------------------------------------------------------------------
/**
 * Process commands for mdmCfg service.
 */
//--------------------------------------------------------------------------------------------------
void cm_mdmCfg_ProcessCommand
(
    const char * command,   ///< [IN] Command
    size_t numArgs          ///< [IN] Number of arguments
);

#endif

