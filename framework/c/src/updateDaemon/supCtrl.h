//--------------------------------------------------------------------------------------------------
/**
 * @file supCtrl.h
 *
 * Copyright (C), Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef __UPDATE_DAEMON_SUP_CTRL_H_INCLUDE_GUARD
#define __UPDATE_DAEMON_SUP_CTRL_H_INCLUDE_GUARD




//--------------------------------------------------------------------------------------------------
/**
 * Start the named application.
 *
 * @return LE_OK if the application was started, otherwise, LE_FAULT.
 */
//--------------------------------------------------------------------------------------------------
le_result_t supCtrl_StartApp
(
    const char* appNamePtr  ///< [IN] The name of the app to start.
);


//--------------------------------------------------------------------------------------------------
/**
 * Stop the named application.
 */
//--------------------------------------------------------------------------------------------------
void supCtrl_StopApp
(
    const char* appNamePtr  ///< [IN] The name of the application to stop.
);


//--------------------------------------------------------------------------------------------------
/**
 * Restart the Legato framework.
 */
//--------------------------------------------------------------------------------------------------
void supCtrl_RestartLegato
(
    void
);




//--------------------------------------------------------------------------------------------------
/**
 * Gets the application's SMACK label.
 *
 * @note
 *      The app doesn't need to be installed for this function to succeed.
 *
 * @warning
 *      This function will kill the client if there is an error.
 */
//--------------------------------------------------------------------------------------------------
void supCtrl_GetLabel
(
    const char* appName,  ///< [IN]  Application name.
    char* label,          ///< [OUT] SMACK label for the application.
    size_t labelSize      ///< [IN]  Size of the SMACK label string.
);


#endif // __UPDATE_DAEMON_SUP_CTRL_H_INCLUDE_GUARD
