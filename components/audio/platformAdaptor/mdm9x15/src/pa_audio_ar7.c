/**
 * @file pa_audio_qmi.c
 *
 * QMI (AR7) implementation of pa_audio API.
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
 * IN\OUT         |          MODEM_VOICE_TX             |               USB_TX               |            SPEAKER                   |                PCM_TX               |                I2S_TX               |
 * -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * MODEM_VOICE_RX |                N/A                  |  AFE_PCM_RX_Voice Mixer CSVoice    |   SLIM_0_RX_Voice Mixer CSVoice      |  SEC_AUX_PCM_RX_Voice Mixer CSVoice |   SEC_RX_Voice Mixer CSVoice
 * -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * USB_RX         |   Voice_Tx Mixer AFE_PCM_TX_Voice   |               N/A                  |              N/A                     |                  N/A                |                  N/A                |
 * -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * PCM_RX         | Voice_Tx Mixer SEC_AUX_PCM_TX_Voice |               N/A                  |              N/A                     |                  N/A                |                  N/A                |
 * -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * I2S_RX         |    Voice_Tx Mixer SEC_TX_Voice      |               N/A                  |              N/A                     |                  N/A                |                  N/A                |
 * -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * MIC            |   Voice_Tx Mixer SLIM_0_TX_Voice    |               N/A                  |              N/A                     |                  N/A                |                  N/A                |
 * -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * FILE_PLAYING   |                N/A                  | AFE_PCM_RX Audio Mixer MultiMedia1 | SLIMBUS_0_RX Audio Mixer MultiMedia1 |                  N/A                |                  N/A                |
 *
 */
//--------------------------------------------------------------------------------------------------
void InitializeConnectionMatrix
(
    void
)
{
    ConnectionMatrix[PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX][PA_AUDIO_IF_CODEC_SPEAKER] = "SLIM_0_RX_Voice Mixer CSVoice";
    ConnectionMatrix[PA_AUDIO_IF_CODEC_MIC][PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX] = "Voice_Tx Mixer SLIM_0_TX_Voice";
    ConnectionMatrix[PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX][PA_AUDIO_IF_DSP_FRONTEND_USB_TX] = "AFE_PCM_RX_Voice Mixer CSVoice";
    ConnectionMatrix[PA_AUDIO_IF_DSP_FRONTEND_USB_RX][PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX] = "Voice_Tx Mixer AFE_PCM_TX_Voice";
    ConnectionMatrix[PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX][PA_AUDIO_IF_DSP_FRONTEND_PCM_TX] = "SEC_AUX_PCM_RX_Voice Mixer CSVoice";
    ConnectionMatrix[PA_AUDIO_IF_DSP_FRONTEND_PCM_RX][PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX] = "Voice_Tx Mixer SEC_AUX_PCM_TX_Voice";
    ConnectionMatrix[PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX][PA_AUDIO_IF_DSP_FRONTEND_I2S_TX] = "SEC_RX_Voice Mixer CSVoice";
    ConnectionMatrix[PA_AUDIO_IF_DSP_FRONTEND_I2S_RX][PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX] = "Voice_Tx Mixer SEC_TX_Voice";
    ConnectionMatrix[PA_AUDIO_IF_FILE_PLAYING][PA_AUDIO_IF_DSP_FRONTEND_USB_TX] = "AFE_PCM_RX Audio Mixer MultiMedia1";
    ConnectionMatrix[PA_AUDIO_IF_FILE_PLAYING][PA_AUDIO_IF_CODEC_SPEAKER] = "SLIMBUS_0_RX Audio Mixer MultiMedia1";
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

    pa_audio_StopPlayback();
    pa_audio_StopCapture();

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
    LE_DEBUG("Enable Codec input of %d", interface);

    switch (interface)
    {
        case PA_AUDIO_IF_CODEC_MIC:
        {
            SetMixerParameter("SLIM_0_TX Channels","One");
            SetMixerParameter("SLIM TX1 MUX","DEC1");
            SetMixerParameter("DEC1 MUX","ADC1");

            SetMixerParameter("ADC1 Volume","2");
            return LE_OK;
        }
        case PA_AUDIO_IF_CODEC_SPEAKER:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_TX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX:
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_TX:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_TX:
        case PA_AUDIO_IF_FILE_PLAYING:
        case PA_AUDIO_IF_END:
        {
            break;
        }
    }
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
    LE_DEBUG("Disable Codec input of %d", interface);

    switch (interface)
    {
        case PA_AUDIO_IF_CODEC_MIC:
        {
            SetMixerParameter("SLIM_0_TX Channels","One");
            SetMixerParameter("SLIM TX1 MUX","ZERO");
            SetMixerParameter("DEC1 MUX","ZERO");

            SetMixerParameter("ADC1 Volume","0");
            return LE_OK;
        }
        case PA_AUDIO_IF_CODEC_SPEAKER:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_TX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX:
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_TX:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_TX:
        case PA_AUDIO_IF_FILE_PLAYING:
        case PA_AUDIO_IF_END:
        {
            break;
        }
    }
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
    LE_DEBUG("Enable Codec output of %d", interface);

    switch (interface)
    {
        case PA_AUDIO_IF_CODEC_SPEAKER:
        {
            SetMixerParameter("SLIM_0_RX Channels","One");
            SetMixerParameter("DAC3 MUX","INV_RX1");
            SetMixerParameter("DAC2 MUX","RX1");
            SetMixerParameter("RX1 MIX1 INP1","RX1");
            SetMixerParameter("Speaker Function","On");

            // TODO change it
            SetMixerParameter("RX1 Digital Volume","100");
            return LE_OK;
        }
        case PA_AUDIO_IF_CODEC_MIC:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_TX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX:
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_TX:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_TX:
        case PA_AUDIO_IF_FILE_PLAYING:
        case PA_AUDIO_IF_END:
        {
            break;
        }
    }
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
    LE_DEBUG("Disable Codec output of %d", interface);

    switch (interface)
    {
        case PA_AUDIO_IF_CODEC_SPEAKER:
        {
            SetMixerParameter("SLIM_0_RX Channels","One");
            SetMixerParameter("DAC3 MUX","ZERO");
            SetMixerParameter("DAC2 MUX","ZERO");
            SetMixerParameter("RX1 MIX1 INP1","ZERO");
            SetMixerParameter("Speaker Function","Off");

            // TODO change it
            SetMixerParameter("RX1 Digital Volume","0");
            return LE_OK;
        }
        case PA_AUDIO_IF_CODEC_MIC:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_TX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX:
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_TX:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_TX:
        case PA_AUDIO_IF_FILE_PLAYING:
        case PA_AUDIO_IF_END:
        {
            break;
        }
    }
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
    uint32_t      gain          ///< [IN] gain value [0..100] (0 means 'muted', 100 is the
                                ///       maximum gain value)
)
{
    LE_DEBUG("Set gain for [%d] to %d",interface,gain);

    if ((gain < 0) || (gain > 100))
    {
        return LE_OUT_OF_RANGE;
    }

    switch (interface)
    {
        case PA_AUDIO_IF_CODEC_MIC:
        {
            if (gain > 66)
            {
                SetMixerParameter("ADC1 Volume","3");
            }
            else if (gain > 33)
            {
                SetMixerParameter("ADC1 Volume","2");
            }
            else if (gain > 0)
            {
                SetMixerParameter("ADC1 Volume","1");
            }
            else
            {
                SetMixerParameter("ADC1 Volume","0");
            }
            return LE_OK;
        }
        case PA_AUDIO_IF_CODEC_SPEAKER:
        {
            char gainPtr[10];
            sprintf(gainPtr,"%d",(124*gain)/100);
            SetMixerParameter("RX1 Digital Volume", gainPtr);
            return LE_OK;
        }
        case PA_AUDIO_IF_DSP_FRONTEND_USB_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_TX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX:
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_TX:
        case PA_AUDIO_IF_FILE_PLAYING:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_TX:
        case PA_AUDIO_IF_END:
        {
            break;
        }
    }
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
    uint32_t     *gainPtr       ///< [OUT] gain value [0..100] (0 means 'muted', 100 is the
                                ///        maximum gain value)
)
{
    LE_ASSERT(gainPtr);

    LE_DEBUG("Get gain for [%d]",interface);
    switch (interface)
    {
        case PA_AUDIO_IF_CODEC_MIC:
        {
            unsigned value;
            GetMixerParameter("ADC1 Volume",&value);
            if (value == 3)
            {
                *gainPtr = 100;
            }
            else if (value == 2)
            {
                *gainPtr = 66;
            }
            else if (value == 1)
            {
                *gainPtr = 33;
            }
            else
            {
                *gainPtr = 0;
            }
            return LE_OK;
        }
        case PA_AUDIO_IF_CODEC_SPEAKER:
        {
            unsigned value;
            GetMixerParameter("RX1 Digital Volume",&value);
            *gainPtr = (100*value)/124;
            return LE_OK;
        }
        case PA_AUDIO_IF_DSP_FRONTEND_USB_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_TX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX:
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_TX:
        case PA_AUDIO_IF_FILE_PLAYING:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_TX:
        case PA_AUDIO_IF_END:
        {
            break;
        }
    }
    LE_ERROR("This interface (%d) is not supported",interface);
    return LE_FAULT;
}
