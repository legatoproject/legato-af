/**
 * @page c_pa_audio Audio Platform Adapter API
 *
 * @ref pa_audio.h "API Reference"
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
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


/** @file pa_audio.h
 *
 * Legato @ref c_pa_audio include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_PAAUDIO_INCLUDE_GUARD
#define LEGATO_PAAUDIO_INCLUDE_GUARD



//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum size of File path's name related field.
 */
//--------------------------------------------------------------------------------------------------
#define FILE_NAME_MAX_LEN             128
#define FILE_NAME_MAX_BYTES           (FILE_NAME_MAX_LEN+1)

//--------------------------------------------------------------------------------------------------
/**
 * The enumeration of all PA audio interface
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_AUDIO_IF_CODEC_MIC                     = 0,
    PA_AUDIO_IF_CODEC_SPEAKER                 = 1,
    PA_AUDIO_IF_DSP_FRONTEND_USB_RX           = 2,
    PA_AUDIO_IF_DSP_FRONTEND_USB_TX           = 3,
    PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX    = 4,
    PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX    = 5,
    PA_AUDIO_IF_DSP_FRONTEND_PCM_RX           = 6,
    PA_AUDIO_IF_DSP_FRONTEND_PCM_TX           = 7,
    PA_AUDIO_IF_DSP_FRONTEND_I2S_RX           = 8,
    PA_AUDIO_IF_DSP_FRONTEND_I2S_TX           = 9,
    PA_AUDIO_IF_DSP_FRONTEND_FILE_PLAY        = 10,
    PA_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE     = 11,
    PA_AUDIO_IF_DSP_BACKEND_DTMF_RX           = 12,
    PA_AUDIO_NUM_INTERFACES                   = 13
}
pa_audio_If_t;


//--------------------------------------------------------------------------------------------------
/**
 * A handler that is called whenever a DTMF is received by the modem.
 *
 * @param dtmf       DTMF character received with the handler.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_audio_DtmfHandlerFunc_t)
(
    char  dtmf  ///< DTMFs received with the handler
);

//--------------------------------------------------------------------------------------------------
/**
 * A handler that is called whenever a file event is notified.
 *
 * @param event       file's event notified with the handler.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_audio_FileEventHandlerFunc_t)
(
    le_audio_FileEvent_t  event,     ///< file's event notified with the handler
    void*                 contextPtr ///< handler's context
);

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
 * This function must be called to set the channel mode of an I2S interface.
 *
 * @return LE_FAULT         The function failed to set the channel mode.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetI2sChannelMode
(
    pa_audio_If_t          interface,
    le_audio_I2SChannel_t  mode
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
 * This function must be called to flag for reset the DSP Audio path
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_FlagForResetDspAudioPath
(
    pa_audio_If_t inputInterface,    ///< [IN] input audio interface
    pa_audio_If_t outputInterface    ///< [IN] output audio interface
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to reset the DSP Audio path
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_audio_ResetDspAudioPath
(
    void
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
 * This function must be called to start the playback thread.
 *
 * @return LE_OK            The thread is started
 * @return LE_BAD_PARAMETER The interface is not valid
 * @return LE_DUPLICATE     The thread is already started
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_StartPlayback
(
    pa_audio_If_t interface,    ///< [IN] audio interface
    int32_t       fd            ///< [IN] audio file descriptor
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop the playback thread.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_audio_StopPlayback
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start the SW capture thread.
 *
 * @return LE_OK            The thread is started
 * @return LE_BAD_PARAMETER The interface is not valid
 * @return LE_DUPLICATE     The thread is already started
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_StartCapture
(
    pa_audio_If_t interface,    ///< [IN] audio interface
    int32_t       fd            ///< [IN] audio file descriptor
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop the SW capture thread.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_audio_StopCapture
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start the DTMF Decoder.
 *
 * @return LE_OK            The decoder is started
 * @return LE_BAD_PARAMETER The interface is not valid
 * @return LE_FAULT         On other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_StartDtmfDecoder
(
    pa_audio_If_t interface    ///< [IN] audio interface
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop the DTMF Decoder.
 *
 * @return LE_OK            The decoder is stopped
 * @return LE_BAD_PARAMETER The interface is not valid
 * @return LE_FAULT         On other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_StopDtmfDecoder
(
    pa_audio_If_t interface    ///< [IN] audio interface
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for DTMF notifications.
 *
 * @return LE_FAULT         The function failed to register the handler.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetDtmfDetectorHandler
(
    pa_audio_DtmfHandlerFunc_t   handlerFuncPtr ///< [IN] The event handler function.
);


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable or disable the Noise Suppressor.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BAD_PARAMETER The audio interface is invalid.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_NoiseSuppressorSwitch
(
    pa_audio_If_t interface,    ///< [IN] audio interface
    le_onoff_t    switchOnOff   ///< [IN] switch ON or OFF
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable or disable the Echo Canceller.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BAD_PARAMETER The audio interface is invalid.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_EchoCancellerSwitch
(
    pa_audio_If_t interface,    ///< [IN] audio interface
    le_onoff_t    switchOnOff   ///< [IN] switch ON or OFF
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable or disable the FIR (Finite Impulse Response) filter on the
 * downlink or uplink audio path.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BAD_PARAMETER The audio interface is invalid.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_FirFilterSwitch
(
    pa_audio_If_t interface,    ///< [IN] audio interface
    le_onoff_t    switchOnOff   ///< [IN] switch ON or OFF
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable or disable the IIR (Infinite Impulse Response) filter on
 * the downlink or uplink audio path.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BAD_PARAMETER The audio interface is invalid.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_IirFilterSwitch
(
    pa_audio_If_t interface,    ///< [IN] audio interface
    le_onoff_t    switchOnOff   ///< [IN] switch ON or OFF
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable or disable the automatic gain control on the selected
 * audio stream.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BAD_PARAMETER The audio interface is invalid.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_AutomaticGainControlSwitch
(
    pa_audio_If_t interface,    ///< [IN] audio interface
    le_onoff_t    switchOnOff   ///< [IN] switch ON or OFF
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the audio profile.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetProfile
(
    le_audio_Profile_t profile   ///< [IN] The audio profile.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the audio profile in use.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_GetProfile
(
    le_audio_Profile_t* profilePtr  ///< [OUT] The audio profile.
);

//--------------------------------------------------------------------------------------------------
/**
 * Configure the PCM Sampling Rate.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OUT_OF_RANGE  Your platform does not support the setting's value.
 * @return LE_BUSY          The PCM interface is already active.
 * @return LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetPcmSamplingRate
(
    uint32_t    rate         ///< [IN] Sampling rate in Hz.
);

//--------------------------------------------------------------------------------------------------
/**
 * Configure the PCM Sampling Resolution.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OUT_OF_RANGE  Your platform does not support the setting's value.
 * @return LE_BUSY          The PCM interface is already active.
 * @return LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetPcmSamplingResolution
(
    uint32_t  bitsPerSample   ///< [IN] Sampling resolution (bits/sample).
);

//--------------------------------------------------------------------------------------------------
/**
 * Configure the PCM Companding.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OUT_OF_RANGE  Your platform does not support the setting's value.
 * @return LE_BUSY          The PCM interface is already active.
 * @return LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetPcmCompanding
(
    le_audio_Companding_t companding   ///< [IN] Companding.
);

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the PCM Sampling Rate.
 *
 * @return The sampling rate in Hz.
 */
//--------------------------------------------------------------------------------------------------
uint32_t pa_audio_GetPcmSamplingRate
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the PCM Sampling Resolution.
 *
 * @return The sampling resolution (bits/sample).
 */
//--------------------------------------------------------------------------------------------------
uint32_t pa_audio_GetPcmSamplingResolution
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the PCM Companding.
 *
 * @return The PCM companding.
 */
//--------------------------------------------------------------------------------------------------
le_audio_Companding_t pa_audio_GetPcmCompanding
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the default PCM time slot used on the current platform.
 *
 * @return the time slot number.
 */
//--------------------------------------------------------------------------------------------------
uint32_t pa_audio_GetDefaultPcmTimeSlot
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the default I2S channel mode used on the current platform.
 *
 * @return the I2S channel mode.
 */
//--------------------------------------------------------------------------------------------------
le_audio_I2SChannel_t pa_audio_GetDefaultI2sMode
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for  for audio file events notifications.
 *
 * @return an handler reference.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamEventHandlerRef_t pa_audio_AddFileEventHandler
(
    pa_audio_FileEventHandlerFunc_t handlerFuncPtr, ///< [IN] The event handler function.
    void*                           contextPtr      ///< [IN] The handler's context.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for audio file events.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_audio_RemoveFileEventHandler
(
    le_audio_StreamEventHandlerRef_t addHandlerRef ///< [IN]
);

#endif // LEGATO_PAAUDIO_INCLUDE_GUARD
