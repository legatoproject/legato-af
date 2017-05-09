/**
 * @file avcCompat.c
 *
 * AirVantage Compatibility application.
 *
 * The purpose of this application is to ensure the compatibility of a system that uses the
 * 'lwm2mCore'-based AVC on a product that also supports the modem-based AVC.
 *
 * Failure to disable the modem-based AVC indicates that it is still busy completing an operation.
 * Only start avcService when we can successfully disable the modem-based AVC.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "lwm2m.h"
#include "assetData.h"
#include "avcObject.h"
#include "avcShared.h"


// -------------------------------------------------------------------------------------------------
/**
 *  AVC Service config tree path
 */
// ------------------------------------------------------------------------------------------------
#define AVC_SERVICE_CFG "/apps/avcService"


//--------------------------------------------------------------------------------------------------
/**
 * Current internal state.
 *
 * Used mainly to ensure that API functions don't do anything if in the wrong state.
 *
 * TODO: May need to revisit some of the state transitions here.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    AVC_IDLE,                    ///< No updates pending or in progress
    AVC_DOWNLOAD_PENDING,        ///< Received pending download; no response sent yet
    AVC_DOWNLOAD_IN_PROGRESS,    ///< Accepted download, and in progress
    AVC_INSTALL_PENDING,         ///< Received pending install; no response sent yet
    AVC_INSTALL_IN_PROGRESS,     ///< Accepted install, and in progress
    AVC_UNINSTALL_PENDING,         ///< Received pending uninstall; no response sent yet
    AVC_UNINSTALL_IN_PROGRESS      ///< Accepted uninstall, and in progress
}
AvcState_t;


//--------------------------------------------------------------------------------------------------
/**
 * The current state of any update.
 *
 * Although this variable is accessed both in API functions and in UpdateHandler(), access locks
 * are not needed.  This is because this is running as a daemon, and so everything runs in the
 * main thread.
 */
//--------------------------------------------------------------------------------------------------
static AvcState_t CurrentState = AVC_IDLE;


//--------------------------------------------------------------------------------------------------
/**
 * Handler to receive update status notifications from PA
 */
//--------------------------------------------------------------------------------------------------
static void UpdateHandler
(
    le_avc_Status_t updateStatus,
    le_avc_UpdateType_t updateType,
    int32_t totalNumBytes,
    int32_t dloadProgress,
    le_avc_ErrorCode_t errorCode
)
{
    LE_INFO("Received update status: %d", updateStatus);

    // Keep track of the state of any pending downloads or installs.
    switch ( updateStatus )
    {
        case LE_AVC_SESSION_STARTED:
            if (CurrentState == AVC_IDLE)
            {
                pa_avc_StartModemActivityTimer();
            }

            assetData_SessionStatus(ASSET_DATA_SESSION_AVAILABLE);
            break;

        case LE_AVC_SESSION_STOPPED:
            assetData_SessionStatus(ASSET_DATA_SESSION_UNAVAILABLE);
            break;

        default:
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialization function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    if (pa_avc_Disable() == LE_OK)
    {
        LE_INFO("Modem-based AVC disabled.");
        ImportConfig();
    }
    else
    {
        LE_INFO("Modem-based has not been AVC disabled. Allow modem to complete the update.");

        // TODO: Not needed once LE-6719 is merged.
        le_appCtrl_Stop(AVC_APP_NAME);

        // Initialize the sub-components
        assetData_Init();
        lwm2m_Init();
        avcObject_Init();

        // Read the user defined timeout from config tree @ /apps/avcService/modemActivityTimeout
        le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(AVC_SERVICE_CFG);
        int timeout = le_cfg_GetInt(iterRef, "modemActivityTimeout", 20);
        le_cfg_CancelTxn(iterRef);

        pa_avc_SetModemActivityTimeout(timeout);

        pa_avc_SetAVMSMessageHandler(UpdateHandler);
    }
}

