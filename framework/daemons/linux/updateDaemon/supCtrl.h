//--------------------------------------------------------------------------------------------------
/**
 * @file supCtrl.h
 *
 * Copyright (C) Sierra Wireless Inc.
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
 * Import the IMA public certificate
 *
 * @return LE_OK if the certificate was imported, otherwise, LE_FAULT.
 */
//--------------------------------------------------------------------------------------------------
le_result_t supCtrl_ImportImaCert
(
    const char* certPath  ///< [IN] Path to IMA public certificate
);


#endif // __UPDATE_DAEMON_SUP_CTRL_H_INCLUDE_GUARD
