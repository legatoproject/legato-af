//-------------------------------------------------------------------------------------------------
/**
 * @file cm_radio.h
 *
 * Handle radio control related functionality
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#ifndef CMODEM_RADIO_INCLUDE_GUARD
#define CMODEM_RADIO_INCLUDE_GUARD


//-------------------------------------------------------------------------------------------------
/**
 * Print the radio help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_mrc_PrintRadioHelp
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Process commands for radio service.
 */
//--------------------------------------------------------------------------------------------------
void cm_mrc_ProcessRadioCommand
(
    const char * command,   ///< [IN] Radio command
    size_t numArgs          ///< [IN] Number of arguments
);

//-------------------------------------------------------------------------------------------------
/**
 * This function sets the radio power.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_mrc_SetRadioPower
(
    le_onoff_t power    ///< [IN] Radio power switch
);


//-------------------------------------------------------------------------------------------------
/**
 * This function returns modem status information to the user.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_mrc_GetModemStatus
(
    void
);



//-------------------------------------------------------------------------------------------------
/**
 * This function sets the radio access technology to use.
 *
 * @return
 * - LE_OK    If the call was successful
 * - LE_FAULT Otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_mrc_SetRat
(
    le_mrc_RatBitMask_t rat ///< [IN] Radio access technology
);

#endif

