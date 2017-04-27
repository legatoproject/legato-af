/**
 * @file avcControl.c
 *
 * AirVantage Control application.
 *
 * Provide an AirVantage control application with default behaviours.
 * This includes:
 * - Automatic download/install of OTA packages
 * - Receive incoming SMS wake up messages
 * - Polling timer
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"


// -------------------------------------------------------------------------------------------------
/**
 *  Polling timer reference. Time interval to automatically start an AVC session.
 */
// ------------------------------------------------------------------------------------------------
static le_timer_Ref_t PollingTimerRef = NULL;

// -------------------------------------------------------------------------------------------------
/**
 *  Default polling timer interval in minutes, in case the polling timer config cannot be retrieved.
 *  The default is set to 1 day.
 */
// ------------------------------------------------------------------------------------------------
#define DEFAULT_POLLING_TIMER_MIN 24*60

//-------------------------------------------------------------------------------------------------
/**
 * Fetch a string describing the type of update underway over Air Vantage.
 *
 * @return Pointer to a null-terminated string constant.
 */
//-------------------------------------------------------------------------------------------------
static const char* GetUpdateType
(
    void
)
{
    const char* updateTypePtr = NULL;
    le_avc_UpdateType_t type;
    le_result_t res = le_avc_GetUpdateType(&type);
    if (res != LE_OK)
    {
        LE_CRIT("Unable to get update type (%s)", LE_RESULT_TXT(res));
        updateTypePtr = "UNKNOWN";
    }
    else
    {
        switch (type)
        {
            case LE_AVC_FIRMWARE_UPDATE:
                updateTypePtr = "FIRMWARE";
                break;

            case LE_AVC_APPLICATION_UPDATE:
                updateTypePtr = "APPLICATION";
                break;

            case LE_AVC_FRAMEWORK_UPDATE:
                updateTypePtr = "FRAMEWORK";
                break;

            case LE_AVC_UNKNOWN_UPDATE:
                updateTypePtr = "UNKNOWN";
                break;

            default:
                LE_CRIT("Unexpected update type %d", type);
                updateTypePtr = "UNKNOWN";
                break;
        }
    }

    return updateTypePtr;
}


//-------------------------------------------------------------------------------------------------
/**
 * Status handler for avcService updates
 */
//-------------------------------------------------------------------------------------------------
static void StatusHandler
(
    le_avc_Status_t updateStatus,
    int32_t totalNumBytes,
    int32_t downloadProgress,
    void* contextPtr
)
{
    le_result_t res;
    const char* statusPtr = NULL;

    switch (updateStatus)
    {
        case LE_AVC_NO_UPDATE:
            statusPtr = "NO_UPDATE";
            break;

        case LE_AVC_DOWNLOAD_PENDING:

            LE_INFO("Accepting %s update.", GetUpdateType());
            res = le_avc_AcceptDownload();
            if (res != LE_OK)
            {
                LE_ERROR("Failed to accept download from Air Vantage (%s)", LE_RESULT_TXT(res));
            }

            statusPtr = "DOWNLOAD_PENDING";
            break;

        case LE_AVC_DOWNLOAD_IN_PROGRESS:
            statusPtr = "DOWNLOAD_IN_PROGRESS";
            break;

        case LE_AVC_DOWNLOAD_COMPLETE:
            statusPtr = "DOWNLOAD_COMPLETE";
            break;

        case LE_AVC_DOWNLOAD_FAILED:
            statusPtr = "DOWNLOAD_FAILED";
            break;

        case LE_AVC_INSTALL_PENDING:

            LE_INFO("Accepting %s installation.", GetUpdateType());
            res = le_avc_AcceptInstall();
            if (res != LE_OK)
            {
                LE_ERROR("Failed to accept install from Air Vantage (%s)", LE_RESULT_TXT(res));
            }

            statusPtr = "INSTALL_PENDING";
            break;

        case LE_AVC_INSTALL_IN_PROGRESS:
            statusPtr = "INSTALL_IN_PROGRESS";
            break;

        case LE_AVC_INSTALL_COMPLETE:
            statusPtr = "INSTALL_COMPLETE";
            break;

        case LE_AVC_INSTALL_FAILED:
            statusPtr = "INSTALL_FAILED";
            break;

        case LE_AVC_UNINSTALL_PENDING:

            LE_INFO("Accepting %s uninstall.", GetUpdateType());
            res = le_avc_AcceptUninstall();
            if (res != LE_OK)
            {
                LE_ERROR("Failed to accept uninstall from Air Vantage (%s)", LE_RESULT_TXT(res));
            }

            statusPtr = "UNINSTALL_PENDING";
            break;

        case LE_AVC_UNINSTALL_IN_PROGRESS:
            statusPtr = "UNINSTALL_IN_PROGRESS";
            break;

        case LE_AVC_UNINSTALL_COMPLETE:
            statusPtr = "UNINSTALL_COMPLETE";
            break;

        case LE_AVC_UNINSTALL_FAILED:
            statusPtr = "UNINSTALL_FAILED";
            break;

        case LE_AVC_SESSION_STARTED:
            statusPtr = "SESSION_STARTED";
            break;

        case LE_AVC_SESSION_STOPPED:
            statusPtr = "SESSION_STOPPED";
            break;

        default:
            break;
    }

    if (NULL == statusPtr)
    {
        LE_ERROR("Air Vantage agent reported unexpected update status: %d", updateStatus);
    }
    else
    {
        LE_INFO("Air Vantage agent reported update status: %s", statusPtr);
    }
}


//-------------------------------------------------------------------------------------------------
/**
 * Sms handler for incoming SMS wakeup messages
 */
//-------------------------------------------------------------------------------------------------
static void SmsReceivedHandler
(
    le_sms_MsgRef_t messagePtr,   ///< [IN] Message object received from the modem.
    void*           contextPtr    ///< [IN] The handler's context.
)
{
    char text[LE_SMS_TEXT_MAX_BYTES] = {0};

    if (le_sms_GetFormat(messagePtr) != LE_SMS_FORMAT_TEXT)
    {
        LE_INFO("Non-text message received!");
        return;
    }

    le_sms_GetText(messagePtr, text, LE_SMS_TEXT_MAX_BYTES);

    if (0 == strncmp(text, "LWM2MWAKEUP", sizeof(text)))
    {
        LE_DEBUG("SMS Wakeup received.");
        // Do something...
    }

    le_sms_DeleteFromStorage(messagePtr);
}


//-------------------------------------------------------------------------------------------------
/**
 * Start an AVC session.
 */
//-------------------------------------------------------------------------------------------------
static void StartSession
(
    le_timer_Ref_t timerRef
)
{
    // Start an AV session.
    le_result_t res = le_avc_StartSession();
    if (res != LE_OK)
    {
        LE_ERROR("Failed to connect to AirVantage: %s", LE_RESULT_TXT(res));

        LE_INFO("Attempting to stop previous session, in case one is still active...");
        res = le_avc_StopSession();
        if (res != LE_OK)
        {
            LE_ERROR("Failed to stop session: %s", LE_RESULT_TXT(res));
        }
        else
        {
            LE_INFO("Successfully stopped session.  Attempting to start a new one.");
            res = le_avc_StartSession();
            if (res != LE_OK)
            {
                LE_FATAL("Failed to connect to AirVantage: %s", LE_RESULT_TXT(res));
            }
        }
    }
}


//-------------------------------------------------------------------------------------------------
/**
 * Start an AVC session periodically according the polling timer config.
 */
//-------------------------------------------------------------------------------------------------
static void StartPollingTimer
(
    void
)
{
    // Polling timer, in minutes.
    uint32_t pollingTimer = 0;
    le_result_t result = le_avc_GetPollingTimer(&pollingTimer);

    if (LE_OK != result)
    {
        LE_WARN("Failed to retrieve polling timer config. Default to %d minutes.",
                DEFAULT_POLLING_TIMER_MIN);
        pollingTimer = DEFAULT_POLLING_TIMER_MIN;
    }

    if (0 == pollingTimer)
    {
        LE_INFO("Polling timer disabled. AVC session will not be started periodically.");
    }
    else
    {
        LE_INFO("Polling timer is set to start AVC session every %d minutes.",
                DEFAULT_POLLING_TIMER_MIN);

        le_clk_Time_t interval = {pollingTimer * 60, 0};
        PollingTimerRef = le_timer_Create("PollingTimer");

        LE_ASSERT(LE_OK == le_timer_SetInterval(PollingTimerRef, interval));
        LE_ASSERT(LE_OK == le_timer_SetRepeat(PollingTimerRef, 0));
        LE_ASSERT(LE_OK == le_timer_SetHandler(PollingTimerRef, StartSession));
        LE_ASSERT(LE_OK == le_timer_Start(PollingTimerRef));
    }
}


COMPONENT_INIT
{
    // Register AirVantage status report handler.
    le_avc_AddStatusEventHandler(StatusHandler, NULL);

    // Start an AVC session at least once.
    StartSession(NULL);

    // Start an AVC session periodically accoridng to the polling timer config.
    StartPollingTimer();

    // Register sms handler for SMS wakeup
    le_sms_AddRxMessageHandler(SmsReceivedHandler, NULL);
}
