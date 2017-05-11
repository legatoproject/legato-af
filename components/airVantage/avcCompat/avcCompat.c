/**
 * @file avcCompat.c
 *
 * AirVantage Compatibility application.
 *
 * The purpose of this application is to ensure the compatibility of a system that uses the
 * 'lwm2mCore'-based AVC on a product that also supports the modem-based AVC.
 *
 * Failure to disable the modem-based AVC indicates that it is still busy completing an operation.
 * In the case of failure, we will stop the avcService and only start it when the retry mechanism
 * is able to successfully disable the modem-based AVC.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "pa_avc.h"

static le_timer_Ref_t RetryTimerRef = NULL;


// -------------------------------------------------------------------------------------------------
/**
 *  Name of the AVC application running in legato.
 */
// ------------------------------------------------------------------------------------------------
#define AVC_APP_NAME "avcService"


// -------------------------------------------------------------------------------------------------
/**
 *  Timer to retry disabling the modem-based AVC every 30 seconds.
 */
// ------------------------------------------------------------------------------------------------
#define DISABLE_RETRY_TIMER 30 // seconds


//--------------------------------------------------------------------------------------------------
/**
 *  Path to the lwm2m configurations in the Config Tree.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_AVC_CONFIG_PATH "system:/apps/avcService/config"


//--------------------------------------------------------------------------------------------------
/**
 *  Max number of bytes of a retry timer name.
 */
//--------------------------------------------------------------------------------------------------
#define TIMER_NAME_BYTES 10


//--------------------------------------------------------------------------------------------------
/**
 * Import AVMS config from the modem to Legato.
 */
//--------------------------------------------------------------------------------------------------
static void ImportConfig
(
    void
)
{
    // Don't import config from the modem if it was done before. Also if we can't read the dirty
    // bit, assume it's false and proceed with import.
    if (le_cfg_QuickGetBool(CFG_AVC_CONFIG_PATH "/imported", false))
    {
        LE_INFO("NOT importing AVMS config from modem to Legato since it was done before.");
    }
    else
    {
        LE_INFO("Importing AVMS config from modem to Legato.");

        le_result_t getConfigRes;

        /* Get the PDP profile */
        char apnName[LE_AVC_APN_NAME_MAX_LEN_BYTES] = {0};
        char userName[LE_AVC_USERNAME_MAX_LEN_BYTES] = {0};
        char userPassword[LE_AVC_PASSWORD_MAX_LEN_BYTES] = {0};

        getConfigRes = pa_avc_GetApnConfig(apnName, LE_AVC_APN_NAME_MAX_LEN_BYTES,
                                           userName, LE_AVC_USERNAME_MAX_LEN_BYTES,
                                           userPassword, LE_AVC_PASSWORD_MAX_LEN_BYTES);

        if (getConfigRes == LE_OK)
        {
            le_avc_SetApnConfig(apnName, userName, userPassword);
        }
        else
        {
            LE_WARN("Failed to get APN config from the modem.");
        }

        /* Get the polling timer */
        uint32_t pollingTimer = 0;

        getConfigRes = pa_avc_GetPollingTimer(&pollingTimer);

        if (getConfigRes == LE_OK)
        {
            le_avc_SetPollingTimer(pollingTimer);
        }
        else
        {
            LE_WARN("Failed to get the polling timer from the modem.");
        }

        /* Get the retry timers */
        uint16_t timerValue[LE_AVC_NUM_RETRY_TIMERS];
        size_t numTimers = 0;

        getConfigRes = pa_avc_GetRetryTimers(timerValue, &numTimers);

        LE_ASSERT(numTimers <= LE_AVC_NUM_RETRY_TIMERS);

        if (getConfigRes == LE_OK)
        {
            le_avc_SetRetryTimers(timerValue, numTimers);
        }
        else
        {
            LE_WARN("Failed to get the retry timers from the modem.");
        }

        // Set the "imported" dirty bit, so that config isn't imported next time.
        le_cfg_QuickSetBool(CFG_AVC_CONFIG_PATH "/imported", true);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Expiry handler function that will retry disabling the modem-based AVC.
 */
//--------------------------------------------------------------------------------------------------
static void RetryDisable
(
    le_timer_Ref_t timerRef
)
{
    LE_INFO("Retry disabling modem-based AVC.");

    // If successful, we can stop retrying
    if (pa_avc_Disable() == LE_OK)
    {
        LE_INFO("Modem-based AVC disabled.");
        le_timer_Stop(RetryTimerRef);
        le_appCtrl_Start(AVC_APP_NAME);
        ImportConfig();
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
    le_clk_Time_t interval = { DISABLE_RETRY_TIMER, 0 };

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
        ImportConfig();
    }
    else
    {
        StartRetryTimer();
        le_appCtrl_Stop(AVC_APP_NAME);
    }
}

