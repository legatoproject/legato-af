//--------------------------------------------------------------------------------------------------
/**
 * @file supCtrl.c
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "appCfg.h"
#include "smack.h"

// Have I already connected the supervisor services?
static bool ConnectedToAppsService = false;
static bool ConnectedToFrameworkService = false;
static bool ConnectedToImaService = false;


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
    if (!ConnectedToAppsService)
    {
        le_appCtrl_ConnectService();
        ConnectedToAppsService = true;
    }

    LE_INFO("Starting app '%s'.", appNamePtr);

    le_result_t result = le_appCtrl_Start(appNamePtr);

    // The app is auto start, so attempt to start it now.
    if (result == LE_DUPLICATE)
    {
        // This means the app was previously installed and running, so we need to stop and start it
        // again to get the new version.
        le_appCtrl_Stop(appNamePtr);
        result = le_appCtrl_Start(appNamePtr);
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
    if (!ConnectedToAppsService)
    {
        le_appCtrl_ConnectService();
        ConnectedToAppsService = true;
    }

    LE_INFO("Stopping app '%s'.", appNamePtr);

    le_appCtrl_Stop(appNamePtr);
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
    if (!ConnectedToFrameworkService)
    {
        le_framework_ConnectService();
        ConnectedToFrameworkService = true;
    }

    LE_INFO("Requesting Legato restart.");

    if (le_framework_Restart(false) != LE_OK)
    {
        LE_INFO("Legato restart request rejected.  Shutdown must be underway already.");
    }
    else
    {
        LE_INFO("Legato restart request accepted.");
    }
}


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
)
//--------------------------------------------------------------------------------------------------
{
    // Connect to the supervisor and start the application.

    if (!ConnectedToImaService)
    {
        le_ima_ConnectService();
        ConnectedToImaService = true;
    }

    LE_INFO("Requesting to import certificate '%s'.", certPath);

    // Change the label of the application certificate file so that the supervisor child process
    // running as '_' can import the key.
    smack_SetLabel(certPath, "_");

    return le_ima_ImportCert(certPath);
}
