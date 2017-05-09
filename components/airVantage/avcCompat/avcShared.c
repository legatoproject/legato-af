/**
 * @file avcShared.c
 *
 * Implementation of shared definitions for AVC compatibility app
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "avcShared.h"
#include "pa_avc.h"


//--------------------------------------------------------------------------------------------------
/**
 * Import AVMS config from the modem to Legato.
 */
//--------------------------------------------------------------------------------------------------
void ImportConfig
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
            if (LE_OK == le_avc_TryConnectService())
            {
                le_avc_SetApnConfig(apnName, userName, userPassword);
            }
            else
            {
                LE_ERROR("Unable to set apn config.");
            }
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
            if (LE_OK == le_avc_TryConnectService())
            {
                le_avc_SetPollingTimer(pollingTimer);
            }
            else
            {
                LE_ERROR("Unable to set polling timer.");
            }
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
            if (LE_OK == le_avc_TryConnectService())
            {
                le_avc_SetRetryTimers(timerValue, numTimers);
            }
            else
            {
                LE_ERROR("Unable to set retry timer.");
            }
        }
        else
        {
            LE_WARN("Failed to get the retry timers from the modem.");
        }

        // Set the "imported" dirty bit, so that config isn't imported next time.
        le_cfg_QuickSetBool(CFG_AVC_CONFIG_PATH "/imported", true);
    }
}