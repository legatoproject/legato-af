/**
 * @file avcControl.c
 *
 * Sample AirVantage Control application.
 *
 * Provide an AirVantage control application with the following behaviours:
 * - Automatic download/install of OTA packages
 * - Receive incoming SMS wake up messages
 * - Polling timer
 * - Retry timers
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
 *  Polling timer interval in minutes. Set to 1 day.
 */
// ------------------------------------------------------------------------------------------------
#define POLLING_TIMER_MIN 24*60

//--------------------------------------------------------------------------------------------------
/**
 * Denoting a session is established to the DM serevr.
 */
//--------------------------------------------------------------------------------------------------
static bool SessionStarted = false;

//--------------------------------------------------------------------------------------------------
/**
 * Retry timers related data. RetryTimersIndex is index to the array of RetryTimers.
 * A timer of value 0 means it's disabled. The timers values are in minutes.
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t RetryTimerRef = NULL;
static uint16_t RetryTimersIndex = 0;
#define NUM_RETRY_TIMERS 4
static uint16_t RetryTimers[NUM_RETRY_TIMERS] = {15, 60, 240, 480};

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
            SessionStarted = true;
            break;

        case LE_AVC_SESSION_STOPPED:
            statusPtr = "SESSION_STOPPED";
            SessionStarted = false;
            break;

        case LE_AVC_REBOOT_PENDING:
            statusPtr = "REBOOT_PENDING";
            break;

        case LE_AVC_CONNECTION_REQUIRED:
            statusPtr = "CONNECTION_REQUIRED";
            break;

        case LE_AVC_AUTH_STARTED:
            statusPtr = "AUTHENTICATION_STARTED";
            break;

        case LE_AVC_AUTH_FAILED:
            statusPtr = "AUTHENTICATION_FAILED";
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

        // Pending operation has been notified, it can now be accepted
        switch (updateStatus)
        {
            case LE_AVC_DOWNLOAD_PENDING:
                LE_INFO("Accepting %s update.", GetUpdateType());
                res = le_avc_AcceptDownload();
                if (res != LE_OK)
                {
                    LE_ERROR("Failed to accept download from AirVantage (%s)", LE_RESULT_TXT(res));
                }
                break;

            case LE_AVC_INSTALL_PENDING:
                LE_INFO("Accepting %s installation.", GetUpdateType());
                res = le_avc_AcceptInstall();
                if (res != LE_OK)
                {
                    LE_ERROR("Failed to accept install from AirVantage (%s)", LE_RESULT_TXT(res));
                }
                break;

            case LE_AVC_UNINSTALL_PENDING:
                LE_INFO("Accepting %s uninstall.", GetUpdateType());
                res = le_avc_AcceptUninstall();
                if (res != LE_OK)
                {
                    LE_ERROR("Failed to accept uninstall from AirVantage (%s)", LE_RESULT_TXT(res));
                }
                break;

            case LE_AVC_REBOOT_PENDING:
                LE_INFO("Accepting device reboot.");
                res = le_avc_AcceptReboot();
                if (res != LE_OK)
                {
                    LE_ERROR("Failed to accept reboot from AirVantage (%s)", LE_RESULT_TXT(res));
                }
                break;

            default:
                // No action required
                break;
        }
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


//--------------------------------------------------------------------------------------------------
/**
 * Reset the retry timers by resetting the RetryTimersIndex, and stopping the current retry timer.
 */
//--------------------------------------------------------------------------------------------------
static void ResetRetryTimers
(
    void
)
{
    RetryTimersIndex = 0;
    le_timer_Stop(RetryTimerRef);
}


//-------------------------------------------------------------------------------------------------
/**
 * Start an AVC session. Retry if session doesn't start.
 */
//-------------------------------------------------------------------------------------------------
static void StartSession
(
    le_timer_Ref_t timerRef
)
{
    if (SessionStarted)
    {
        // No need to start a retry timer. Perform reset/cleanup.
        ResetRetryTimers();
    }
    else
    {
        // Retrying. LE_FAULT shouldn't happen because this app is the control app.
        LE_ASSERT(LE_FAULT != le_avc_StopSession());
        LE_ASSERT(LE_FAULT != le_avc_StartSession());

        // Increment the index except for the first try.
        if (RetryTimersIndex != 0)
        {
            RetryTimersIndex++;
        }

        // Attempt to start a retry timer.
        // See which timer we are at by looking at RetryTimersIndex
        // if the timer is 0, get the next one. (0 means disabled / not used)
        // if we run out of timers, do nothing.  Perform reset/cleanup
        while((RetryTimersIndex < NUM_RETRY_TIMERS) &&
              (0 == RetryTimers[RetryTimersIndex]))
        {
                RetryTimersIndex++;
        }

        // This is the case when we've run out of timers. Reset/cleanup, and don't start the next
        // retry timer (since there aren't any left).
        if (RetryTimersIndex >= NUM_RETRY_TIMERS)
        {
            ResetRetryTimers();
        }
        // Start the next retry timer.
        else
        {
            le_clk_Time_t interval = {RetryTimers[RetryTimersIndex] * 60, 0};

            LE_ASSERT(LE_OK == le_timer_SetInterval(RetryTimerRef, interval));
            LE_ASSERT(LE_OK == le_timer_SetHandler(RetryTimerRef, StartSession));
            LE_ASSERT(LE_OK == le_timer_Start(RetryTimerRef));
        }
    }
}


//-------------------------------------------------------------------------------------------------
/**
 * Start an AVC session periodically.
 */
//-------------------------------------------------------------------------------------------------
static void StartPollingTimer
(
    void
)
{
    LE_INFO("Polling timer is set to start AVC session every %d minutes.", POLLING_TIMER_MIN);

    le_clk_Time_t interval = {POLLING_TIMER_MIN * 60, 0};
    PollingTimerRef = le_timer_Create("avcControl PollingTimer");

    LE_ASSERT(LE_OK == le_timer_SetInterval(PollingTimerRef, interval));
    LE_ASSERT(LE_OK == le_timer_SetRepeat(PollingTimerRef, 0));
    LE_ASSERT(LE_OK == le_timer_SetHandler(PollingTimerRef, StartSession));
    LE_ASSERT(LE_OK == le_timer_Start(PollingTimerRef));
}


COMPONENT_INIT
{
    // Register AirVantage status report handler. This makes this app "control app".
    le_avc_AddStatusEventHandler(StatusHandler, NULL);

    RetryTimerRef = le_timer_Create("avcControl RetryTimer");

    // Start an AVC session at least once.
    StartSession(NULL);

    // Start an AVC session periodically.
    StartPollingTimer();

    // Register sms handler for SMS wakeup
    le_sms_AddRxMessageHandler(SmsReceivedHandler, NULL);
}
