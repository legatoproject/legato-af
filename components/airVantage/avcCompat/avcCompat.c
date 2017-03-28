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
    }
    else
    {
        StartRetryTimer();
        le_appCtrl_Stop(AVC_APP_NAME);
    }
}

