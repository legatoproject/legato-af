/**
 * @file avcCompat.c
 *
 * AirVantage Compatibility application.
 *
 * The purpose of this application is to ensure the compatibility of a system that uses the
 * 'lwm2mCore'-based AVC on a product that also supports the modem-based AVC.
 *
 * Failure to disable the modem-based AVC indicates that it is still busy completing an operation.
 * When modem-based AVC is successfully disabled, we will start the avcService application.
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
 * Retry timer for disabling the modem-based AVC
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t RetryTimerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Flag to indicate whether modem-based AVC is disabled or not
 */
//--------------------------------------------------------------------------------------------------
static bool IsAvcDisabled = false;

// -------------------------------------------------------------------------------------------------
/**
 *  Timer interval to retry disabling the modem-based AVC.
 */
// ------------------------------------------------------------------------------------------------
#define DISABLE_RETRY_TIMER_INTERVAL 30

// -------------------------------------------------------------------------------------------------
/**
 * Number of retry timer events to wait for read notification.
 * After that, attempt to disable modem-based AVC will be made even if
 * "read" notification is not received.
 */
// ------------------------------------------------------------------------------------------------
#define DISABLE_RETRY_TIMER_REPEAT_LIMIT 10

//--------------------------------------------------------------------------------------------------
/**
 * Number of repeats of the Retry Timer
 */
//--------------------------------------------------------------------------------------------------
static int RetryTimerRepeatCount = 0;

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

            // keep session alive until modem-based AVC is disabled
            if (!IsAvcDisabled)
            {
                pa_avc_StartSession();
            }
            break;

        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start AVC, AT and QMI AirVantage services
 */
//--------------------------------------------------------------------------------------------------
static void StartApplications
(
    void
)
{
    le_appCtrl_Start(AVC_APP_NAME);
    le_appCtrl_Start(AT_APP_NAME);
    le_appCtrl_Start(QMI_APP_NAME);
}

//--------------------------------------------------------------------------------------------------
/**
 * Expiry handler function that will retry disabling the modem-based AVC.
 */
//--------------------------------------------------------------------------------------------------
void RetryDisable
(
    le_timer_Ref_t timerRef
)
{
    RetryTimerRepeatCount++;

    // If read operation is not received yet, skip the retry. Do this only for
    // a limited number of timer repeats. After that, retry regardless.
    if (!lwm2m_IsReadEventReceived() &&
        RetryTimerRepeatCount < DISABLE_RETRY_TIMER_REPEAT_LIMIT)
    {
        LE_INFO("Read event not received yet: skipping retry %d to disable modem-based AVC.",
                RetryTimerRepeatCount);
        return;
    }

    if (pa_avc_Disable() == LE_OK)
    {
        LE_INFO("Retry %d: modem-based AVC disabled.", RetryTimerRepeatCount);
        IsAvcDisabled = true;
        // If successful, we can stop retrying
        le_timer_Stop(RetryTimerRef);
        StartApplications();
        ImportConfig();
    }
    else
    {
        LE_INFO("Retry %d: modem-based AVC not disabled. "
                "Allow modem to complete the update.", RetryTimerRepeatCount);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize and start retry timer.
 */
//--------------------------------------------------------------------------------------------------
static void StartRetryTimer
(
    void
)
{
    le_result_t res = LE_NOT_POSSIBLE;
    le_clk_Time_t interval = { DISABLE_RETRY_TIMER_INTERVAL, 0 };

    RetryTimerRef = le_timer_Create("RetryDisableTimer");

    res = le_timer_SetInterval(RetryTimerRef, interval);
    LE_FATAL_IF(res != LE_OK, "Unable to set timer interval.");

    res = le_timer_SetRepeat(RetryTimerRef, 0);
    LE_FATAL_IF(res != LE_OK, "Unable to set repeat for timer.");

    res = le_timer_SetHandler(RetryTimerRef, RetryDisable);
    LE_FATAL_IF(res != LE_OK, "Unable to set timer handler.");

    res = le_timer_Start(RetryTimerRef);
    LE_FATAL_IF(res != LE_OK, "Unable to start timer.");
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
        IsAvcDisabled = true;
        StartApplications();
        ImportConfig();
    }
    else
    {
        LE_INFO("Modem-based AVC has not been disabled. Allow modem to complete the update.");

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

        // Start timer to periodically retry disabling modem-based AVC
        StartRetryTimer();
    }
}

