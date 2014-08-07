
// -------------------------------------------------------------------------------------------------
/**
 *  @file cm_data.h
 *
 *  Handle data connection control related functionality
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#ifndef CMODEM_DATA_INCLUDE_GUARD
#define CMODEM_DATA_INCLUDE_GUARD


// -------------------------------------------------------------------------------------------------
/**
*  Print the data help text to stdout.
*/
// -------------------------------------------------------------------------------------------------
void cm_data_PrintDataHelp
(
    void
);


// -------------------------------------------------------------------------------------------------
/**
*  Set the profile in use in configDB
*/
// -------------------------------------------------------------------------------------------------
int cm_data_SetProfileInUse
(
    int profileInUse
);


//--------------------------------------------------------------------------------------------------
/**
* Start a data connection.
*/
//--------------------------------------------------------------------------------------------------
void cm_data_StartDataConnection
(
    const char * timeout        ///< [IN] Data connection timeout timer
);


//--------------------------------------------------------------------------------------------------
/**
* Monitor a data connection.
*/
//--------------------------------------------------------------------------------------------------
void cm_data_MonitorDataConnection
(
    void
);


// -------------------------------------------------------------------------------------------------
/**
*  This function will attempt to set the APN name.
*
*  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
*/
// -------------------------------------------------------------------------------------------------
int cm_data_SetApnName
(
    const char * apn    ///< [IN] Access point name
);


// -------------------------------------------------------------------------------------------------
/**
*  This function will attempt to set the PDP type.
*
*  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
*/
// -------------------------------------------------------------------------------------------------
int cm_data_SetPdpType
(
    const char * pdpType    ///< [IN] Packet data protocol
);


// -------------------------------------------------------------------------------------------------
/**
*  This function will attempt to set the authentication information.
*
*  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
*/
// -------------------------------------------------------------------------------------------------
int cm_data_SetAuthentication
(
    const char * type,      ///< [IN] Authentication type
    const char * userName,  ///< [IN] Authentication username
    const char * password   ///< [IN] Authentication password
);


// -------------------------------------------------------------------------------------------------
/**
*  This function will return profile information for profile that it will be using.
*
*  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
*/
// -------------------------------------------------------------------------------------------------
int cm_data_GetProfileInfo
(
    void
);


// -------------------------------------------------------------------------------------------------
/**
 *  This function will list all profiles with their pdp and apn information.
 *
 *  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
int cm_data_ListProfileName
(
    void
);

#endif

