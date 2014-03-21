/**
 * @file pa_audio_wp7.c
 *
 * QMI (WP7) implementation of pa_audio API.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "pa_audio.h"
#include "pa_audio_local.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * The DSP audio path matrix.
 */
//--------------------------------------------------------------------------------------------------
static const char*          ConnectionMatrix[PA_AUDIO_IF_END+1][PA_AUDIO_IF_END+1] = { { NULL } };

//--------------------------------------------------------------------------------------------------
/**
 * This function initializes the Connection Matric as follows:
 *
 *
 * IN\OUT         |          MODEM_VOICE_TX             |               USB_TX               |  SPEAKER  |               SEC_PCM_TX
 * ---------------------------------------------------------------------------------------------------------------------------------------------
 * MODEM_VOICE_RX |                N/A                  |  AFE_PCM_RX_Voice Mixer CSVoice    |    N/A    |  SEC_AUX_PCM_RX_Voice Mixer CSVoice
 * ---------------------------------------------------------------------------------------------------------------------------------------------
 * USB_RX         |   Voice_Tx Mixer AFE_PCM_TX_Voice   |               N/A                  |    N/A    |                  N/A
 * ---------------------------------------------------------------------------------------------------------------------------------------------
 * SEC_PCM_RX     | Voice_Tx Mixer SEC_AUX_PCM_TX_Voice |               N/A                  |    N/A    |                  N/A
 * ---------------------------------------------------------------------------------------------------------------------------------------------
 * MIC            |                N/A                  |               N/A                  |    N/A    |                  N/A    
 * ---------------------------------------------------------------------------------------------------------------------------------------------
 * FILE_PLAYING   |                N/A                  | AFE_PCM_RX Audio Mixer MultiMedia1 |    N/A    |                  N/A    
 *
 */
//--------------------------------------------------------------------------------------------------
void InitializeConnectionMatrix
(
    void
)
{
    ConnectionMatrix[PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX][PA_AUDIO_IF_DSP_FRONTEND_USB_TX] = "AFE_PCM_RX_Voice Mixer CSVoice";
    ConnectionMatrix[PA_AUDIO_IF_DSP_FRONTEND_USB_RX][PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX] = "Voice_Tx Mixer AFE_PCM_TX_Voice";
    ConnectionMatrix[PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX][PA_AUDIO_IF_DSP_FRONTEND_PCM_TX] = "SEC_AUX_PCM_RX_Voice Mixer CSVoice";
    ConnectionMatrix[PA_AUDIO_IF_DSP_FRONTEND_PCM_RX][PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX] = "Voice_Tx Mixer SEC_AUX_PCM_TX_Voice";
    ConnectionMatrix[PA_AUDIO_IF_FILE_PLAYING][PA_AUDIO_IF_DSP_FRONTEND_USB_TX] = "AFE_PCM_RX Audio Mixer MultiMedia1";
}

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the PA Audio module.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if unsuccessful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_Init
(
    void
)
{
    InitializeConnectionMatrix();

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable codec input
 *
 * @return LE_FAULT         The function failed to enable codec input
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_EnableCodecInput
(
    pa_audio_If_t interface
)
{
    LE_ERROR("This interface (%d) is not supported",interface);
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to disable codec input
 *
 * @return LE_FAULT         The function failed to disable codec input
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_DisableCodecInput
(
    pa_audio_If_t interface
)
{
    LE_ERROR("This interface (%d) is not supported", interface);
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable codec output
 *
 * @return LE_FAULT         The function failed to enable codec output
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_EnableCodecOutput
(
    pa_audio_If_t interface
)
{
    LE_ERROR("This interface (%d) is not supported",interface);
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to disable codec output
 *
 * @return LE_FAULT         The function failed to disable codec output
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_DisableCodecOutput
(
    pa_audio_If_t interface
)
{
    LE_ERROR("This interface (%d) is not supported",interface);
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the DSP Audio path
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetDspAudioPath
(
    pa_audio_If_t inputInterface,    ///< [IN] input audio interface
    pa_audio_If_t outputInterface    ///< [IN] output audio interface
)
{
    if (ConnectionMatrix[inputInterface][outputInterface])
    {
        LE_DEBUG("Set the following path: %s", ConnectionMatrix[inputInterface][outputInterface]);
        SetMixerParameter(ConnectionMatrix[inputInterface][outputInterface], "1");
    }
    else
    {
        LE_DEBUG("DSP audio path not found in connexion matrix.");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to reset the DSP Audio path
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_ResetDspAudioPath
(
    pa_audio_If_t inputInterface,    ///< [IN] input audio interface
    pa_audio_If_t outputInterface    ///< [IN] output audio interface
)
{
    if (ConnectionMatrix[inputInterface][outputInterface] != NULL)
    {
        LE_DEBUG("Reset the following path: %s", ConnectionMatrix[inputInterface][outputInterface]);
        SetMixerParameter(ConnectionMatrix[inputInterface][outputInterface], "0");
    }
    else
    {
        LE_DEBUG("DSP audio path not found in connexion matrix.");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the interface gain
 *
 * @return LE_OUT_OF_RANGE  The gain parameter is not between 0 and 100
 * @return LE_FAULT         The function failed to set the interface gain
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetGain
(
    pa_audio_If_t interface,    ///< [IN] audio interface
    uint32_t      gain          ///< [IN] gain value [0..100]
)
{
    // TODO: verify which interfaces can support gain setting modification
    LE_ERROR("This interface (%d) is not supported",interface);
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the interface gain
 *
 * @return LE_FAULT         The function failed to get the interface gain
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_GetGain
(
    pa_audio_If_t interface,    ///< [IN] audio interface
    uint32_t     *gainPtr       ///< [OUT] gain value [0..100]
)
{
    LE_ASSERT(gainPtr);

    // TODO: verify which interfaces can support gain setting modification
    LE_ERROR("This interface (%d) is not supported",interface);
    return LE_FAULT;
}
