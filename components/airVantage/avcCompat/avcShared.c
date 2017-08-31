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
 * Is configuration imported from modem?
 */
//--------------------------------------------------------------------------------------------------
#define AVC_CONFIG_IS_IMPORTED "/avc/config/isImported"

//--------------------------------------------------------------------------------------------------
/**
 * Write to file using Legato le_fs API
 *
 * @return
 *  - LE_OK             The function succeeded
 *  - LE_BAD_PARAMETER  Incorrect parameter provided
 *  - LE_OVERFLOW       The file path is too long
 *  - LE_FAULT          The function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WriteFs
(
    const char  *pathPtr,   ///< File path
    uint8_t     *bufPtr,    ///< Data buffer
    size_t      size        ///< Buffer size
)
{
    le_fs_FileRef_t fileRef;
    le_result_t result;

    result = le_fs_Open(pathPtr, LE_FS_WRONLY | LE_FS_CREAT, &fileRef);
    if (LE_OK != result)
    {
        LE_ERROR("failed to open %s: %s", pathPtr, LE_RESULT_TXT(result));
        return result;
    }

    result = le_fs_Write(fileRef, bufPtr, size);
    if (LE_OK != result)
    {
        LE_ERROR("failed to write %s: %s", pathPtr, LE_RESULT_TXT(result));
        if (LE_OK != le_fs_Close(fileRef))
        {
            LE_ERROR("failed to close %s", pathPtr);
        }
        return result;
    }

    result = le_fs_Close(fileRef);
    if (LE_OK != result)
    {
        LE_ERROR("failed to close %s: %s", pathPtr, LE_RESULT_TXT(result));
        return result;
    }

    return LE_OK;
}

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
    le_result_t result;

    LE_INFO("Connect avcService");
    le_avc_ConnectService();

    // Don't import config from the modem if it was done before.
    if (le_fs_Exists(AVC_CONFIG_IS_IMPORTED))
    {
        LE_INFO("NOT importing AVMS config from modem to Legato since it was done before.");
    }
    else
    {
        LE_INFO("Importing AVMS config from modem to Legato.");

        /* Get the PDP profile */
        char apnName[LE_AVC_APN_NAME_MAX_LEN_BYTES] = {0};
        char userName[LE_AVC_USERNAME_MAX_LEN_BYTES] = {0};
        char userPassword[LE_AVC_PASSWORD_MAX_LEN_BYTES] = {0};

        result = pa_avc_GetApnConfig(apnName, LE_AVC_APN_NAME_MAX_LEN_BYTES,
                                     userName, LE_AVC_USERNAME_MAX_LEN_BYTES,
                                     userPassword, LE_AVC_PASSWORD_MAX_LEN_BYTES);

        if (result == LE_OK)
        {
            le_avc_SetApnConfig(apnName, userName, userPassword);
        }
        else
        {
            LE_WARN("Failed to get APN config from the modem.");
        }

        /* Get the polling timer */
        uint32_t pollingTimer = 0;

        result = pa_avc_GetPollingTimer(&pollingTimer);

        if (result == LE_OK)
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

        result = pa_avc_GetRetryTimers(timerValue, &numTimers);

        LE_ASSERT(numTimers <= LE_AVC_NUM_RETRY_TIMERS);

        if (result == LE_OK)
        {
            le_avc_SetRetryTimers(timerValue, numTimers);
        }
        else
        {
            LE_WARN("Failed to get the retry timers from the modem.");
        }

        /* Get the user agreement configuration */
        pa_avc_UserAgreement_t userAgreementConfig;
        result = pa_avc_GetUserAgreement(&userAgreementConfig);

        if (result == LE_OK)
        {
            le_avc_SetUserAgreement(LE_AVC_USER_AGREEMENT_CONNECTION,
                                    userAgreementConfig.isAutoConnect);
            le_avc_SetUserAgreement(LE_AVC_USER_AGREEMENT_DOWNLOAD,
                                    userAgreementConfig.isAutoDownload);
            le_avc_SetUserAgreement(LE_AVC_USER_AGREEMENT_INSTALL,
                                    userAgreementConfig.isAutoUpdate);
        }
        else
        {
            LE_WARN("Failed to get user agreement configuration from the modem.");
        }

        // Save status of import
        uint8_t isImported = true;
        WriteFs(AVC_CONFIG_IS_IMPORTED, &isImported, sizeof(int));
    }
}
