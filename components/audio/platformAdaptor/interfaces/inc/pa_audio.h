/**
 * @page c_pa_audio Audio Platform Adapter API
 *
 * @ref pa_audio.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * @section pa_audio_toc Table of Contents
 *
 *  - @ref pa_audio_intro
 *  - @ref pa_audio_rational
 *
 *
 * @section pa_audio_intro Introduction
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developped upon these APIs.
 *
 *
 * @section pa_audio_rational Rational
 * These functions are all blocking functions, so that they return when the modem has answered or
 * when a timeout has occured due to an interrupted communication with the modem.
 *
 * They all verify the validity and the range of the input parameters before performing the modem
 * operation.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


/** @file pa_audio.h
 *
 * Legato @ref c_pa_audio include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_PAAUDIO_INCLUDE_GUARD
#define LEGATO_PAAUDIO_INCLUDE_GUARD


#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * The enumeration of all PA audio interface
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_AUDIO_IF_CODEC_MIC                   = 0,
    PA_AUDIO_IF_CODEC_SPEAKER               = 1,
    PA_AUDIO_IF_DSP_FRONTEND_USB_RX         = 2,
    PA_AUDIO_IF_DSP_FRONTEND_USB_TX         = 3,
    PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX  = 4,
    PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX  = 5,
    PA_AUDIO_IF_FILE_PLAYING                = 6,
    PA_AUDIO_IF_DSP_FRONTEND_PCM_RX         = 7,
    PA_AUDIO_IF_DSP_FRONTEND_PCM_TX         = 8,
    PA_AUDIO_IF_END                         = 9
}
pa_audio_If_t;



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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start a playing thread
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
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop a playing thread
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_audio_StopPlayback
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start a recording thread
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
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop a recording thread
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_audio_StopCapture
(
    void
);

#endif // LEGATO_PAAUDIO_INCLUDE_GUARD
