/**
 * @file pa_audio_x86.c
 *
 * localhost (x86) implementation of pa_audio API.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "pa_audio.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

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
    return LE_OK;
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
    return LE_OK;
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
    return LE_OK;
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
    return LE_OK;
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
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the interface gain
 *
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
    return LE_OK;
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
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the timeslot number of a PCM interface.
 *
 * @return LE_FAULT         The function failed to set the timeslot number.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetPcmTimeSlot
(
    pa_audio_If_t interface,
    uint32_t      timeslot
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to configure an interface as a Master.
 *
 * @return LE_FAULT         The function failed to configure the interface as a Master.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetMasterMode
(
    pa_audio_If_t interface
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to configure an interface as a Slave.
 *
 * @return LE_FAULT         The function failed to configure the interface as a Slave.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetSlaveMode
(
    pa_audio_If_t interface
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the channel mode of an I2S interface.
 *
 * @return LE_FAULT         The function failed to set the channel mode.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetI2sChannelMode
(
    pa_audio_If_t           interface,
    le_audio_I2SChannel_t  mode
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to ask for a playback thread starting
 *
 * @return LE_OK            The thread is started
 * @return LE_BAD_PARAMETER The playback format is not valid
 * @return LE_DUPLICATE     The thread is already started
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_StartPlayback
(
    const char* formatPtr,      ///< [IN] Playback format
    uint32_t    channelCount    ///< [IN] Number of channel
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop a playback thread
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_audio_StopPlayback
(
    void
)
{

}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to ask for a capture thread starting
 *
 * @return LE_OK            The thread is started
 * @return LE_BAD_PARAMETER The capture format is not valid
 * @return LE_DUPLICATE     The thread is already started
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_StartCapture
(
    const char* formatPtr,      ///< [IN] Capture format
    uint32_t    channelCount    ///< [IN] Number of channel
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop a capture thread
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_audio_StopCapture
(
    void
)
{

}
