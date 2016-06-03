//--------------------------------------------------------------------------------------------------
/**
 * @file supCtrl.c
 *
 * Copyright (C), Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "appCfg.h"


// Have I already connected the supervisor service?
static bool ConnectedToSupervisor = false;


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
)
//--------------------------------------------------------------------------------------------------
{
    // Read the application's info from the config, and check if the app is marked as auto start.
    // If not, there's not much else to do here.
    appCfg_Iter_t appIterRef = appCfg_FindApp(appNamePtr);

    if (appIterRef == NULL)
    {
        // The app was not found, so is basically not startable.
        LE_CRIT("Can't find app '%s' to start it.", appNamePtr);
        return false;
    }

    appCfg_StartMode_t startMode = appCfg_GetStartMode(appIterRef);
    appCfg_DeleteIter(appIterRef);

    if (startMode != APPCFG_START_MODE_AUTO)
    {
        LE_INFO("App '%s' is not marked for auto-start.", appNamePtr);

        return true;
    }

    // Connect to the supervisor and start the application.
    if (!ConnectedToSupervisor)
    {
        le_sup_ctrl_ConnectService();
        ConnectedToSupervisor = true;
    }

    LE_INFO("Starting app '%s'.", appNamePtr);

    le_result_t result = le_sup_ctrl_StartApp(appNamePtr);

    // The app is auto start, so attempt to start it now.
    if (result == LE_DUPLICATE)
    {
        // This means the app was previously installed and running, so we need to stop and start it
        // again to get the new version.
        le_sup_ctrl_StopApp(appNamePtr);
        result = le_sup_ctrl_StartApp(appNamePtr);
    }
    else if (result == LE_NOT_FOUND)
    {
        LE_CRIT("Attempt to start '%s' failed because its config could not be found.",
                appNamePtr);
        return LE_FAULT;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stop the named application.
 */
//--------------------------------------------------------------------------------------------------
void supCtrl_StopApp
(
    const char* appNamePtr  ///< [IN] The name of the application to stop.
)
//--------------------------------------------------------------------------------------------------
{
    // Connect to the supervisor and start the application.
    if (!ConnectedToSupervisor)
    {
        le_sup_ctrl_ConnectService();
        ConnectedToSupervisor = true;
    }

    LE_INFO("Stopping app '%s'.", appNamePtr);

    le_sup_ctrl_StopApp(appNamePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Restart the Legato framework.
 */
//--------------------------------------------------------------------------------------------------
void supCtrl_RestartLegato
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Connect to the supervisor.
    if (!ConnectedToSupervisor)
    {
        le_sup_ctrl_ConnectService();
        ConnectedToSupervisor = true;
    }

    LE_INFO("Requesting Legato restart.");

    if (le_sup_ctrl_RestartLegato(false) != LE_OK)
    {
        LE_INFO("Legato restart request rejected.  Shutdown must be underway already.");
    }
    else
    {
        LE_INFO("Legato restart request accepted.");
    }
}
