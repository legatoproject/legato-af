//-------------------------------------------------------------------------------------------------
/**
 * @file cm_adc.h
 *
 * Handle Analog to Digital Conversion related functionality
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#ifndef CMODEM_ADC_INCLUDE_GUARD
#define CMODEM_ADC_INCLUDE_GUARD

//-------------------------------------------------------------------------------------------------
/**
 * Print the adc help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_adc_PrintAdcHelp
(
    void
);

///--------------------------------------------------------------------------------------------------
/**
 * Process commands for ADC service.
 */
//--------------------------------------------------------------------------------------------------
void cm_adc_ProcessAdcCommand
(
    const char * command,   ///< [IN] Command
    size_t numArgs          ///< [IN] Number of arguments
);

#endif

