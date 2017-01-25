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
 * Copyright (C) Sierra Wireless Inc.
 */


/** @file pa_audio.h
 *
 * Legato @ref c_pa_audio include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PAAUDIO_INCLUDE_GUARD
#define LEGATO_PAAUDIO_INCLUDE_GUARD



//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the timeslot number of a PCM interface.
 *
 * @return LE_FAULT         The function failed to set the timeslot number.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_SetPcmTimeSlot
(
    le_audio_Stream_t* streamPtr,    ///< [IN] input audio stream
    uint32_t           timeslot
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the channel mode of an I2S interface.
 *
 * @return LE_FAULT         The function failed to set the channel mode.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_SetI2sChannelMode
(
    le_audio_Stream_t*     streamPtr,    ///< [IN] input audio stream
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
LE_SHARED le_result_t pa_audio_SetMasterMode
(
    le_audio_Stream_t* streamPtr    ///< [IN] input audio stream
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to configure an interface as a Slave.
 *
 * @return LE_FAULT         The function failed to configure the interface as a Slave.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_SetSlaveMode
(
    le_audio_Stream_t* streamPtr    ///< [IN] input audio stream
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the DSP Audio path
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_SetDspAudioPath
(
    le_audio_Stream_t* inputStreamPtr,   ///< [IN] input audio stream
    le_audio_Stream_t* outputStreamPtr   ///< [IN] output audio stream
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to reset the DSP Audio path
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_ResetDspAudioPath
(
    le_audio_Stream_t* inputStreamPtr,   ///< [IN] input audio stream
    le_audio_Stream_t* outputStreamPtr   ///< [IN] output audio stream
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the interface gain
 *
 * @return LE_OUT_OF_RANGE  The gain parameter is out of range
 * @return LE_FAULT         The function failed to set the interface gain
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_SetGain
(
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    int32_t            gain         ///< [IN] gain value
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the interface gain
 *
 * @return LE_FAULT         The function failed to get the interface gain
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_GetGain
(
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    int32_t*           gainPtr      ///< [OUT] gain value
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Noise Suppressor status
 *
 * @return LE_FAULT         The function failed to get the interface NS status
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_GetNoiseSuppressorStatus
(
    le_audio_Stream_t* streamPtr,                   ///< [IN] input audio stream
    bool*              noiseSuppressorStatusPtr     ///< [OUT] Noise Suppressor status
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Echo Canceller status
 *
 * @return LE_FAULT         The function failed to get the interface EC status
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_GetEchoCancellerStatus
(
    le_audio_Stream_t* streamPtr,                   ///< [IN] input audio stream
    bool*              echoCancellerStatusPtr       ///< [OUT] Echo Canceller status
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
LE_SHARED le_result_t pa_audio_StartDtmfDecoder
(
    le_audio_Stream_t* streamPtr     ///< [IN] input audio stream
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop the DTMF Decoder.
 *
 * @return LE_OK            The decoder is started
 * @return LE_BAD_PARAMETER The interface is not valid
 * @return LE_FAULT         On other errors
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_StopDtmfDecoder
(
    le_audio_Stream_t* streamPtr   ///< [IN] input audio stream
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
LE_SHARED le_result_t pa_audio_NoiseSuppressorSwitch
(
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    le_onoff_t         switchOnOff  ///< [IN] switch ON or OFF
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
LE_SHARED le_result_t pa_audio_EchoCancellerSwitch
(
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    le_onoff_t         switchOnOff  ///< [IN] switch ON or OFF
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
LE_SHARED le_result_t pa_audio_FirFilterSwitch
(
    le_audio_Stream_t* streamPtr,    ///< [IN] input audio stream
    le_onoff_t         switchOnOff   ///< [IN] switch ON or OFF
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
LE_SHARED le_result_t pa_audio_IirFilterSwitch
(
    le_audio_Stream_t* streamPtr,    ///< [IN] input audio stream
    le_onoff_t         switchOnOff   ///< [IN] switch ON or OFF
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
LE_SHARED le_result_t pa_audio_AutomaticGainControlSwitch
(
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    le_onoff_t         switchOnOff  ///< [IN] switch ON or OFF
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
LE_SHARED le_result_t pa_audio_SetProfile
(
    uint32_t profile   ///< [IN] The audio profile.
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
LE_SHARED le_result_t pa_audio_GetProfile
(
    uint32_t* profilePtr  ///< [OUT] The audio profile.
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
LE_SHARED le_result_t pa_audio_SetPcmSamplingRate
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
LE_SHARED le_result_t pa_audio_SetPcmSamplingResolution
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
LE_SHARED le_result_t pa_audio_SetPcmCompanding
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
LE_SHARED uint32_t pa_audio_GetPcmSamplingRate
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
LE_SHARED uint32_t pa_audio_GetPcmSamplingResolution
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
LE_SHARED le_audio_Companding_t pa_audio_GetPcmCompanding
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
LE_SHARED uint32_t pa_audio_GetDefaultPcmTimeSlot
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
LE_SHARED le_audio_I2SChannel_t pa_audio_GetDefaultI2sMode
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for stream events notifications.
 *
 * @return an handler reference.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_audio_DtmfStreamEventHandlerRef_t pa_audio_AddDtmfStreamEventHandler
(
    le_audio_DtmfStreamEventHandlerFunc_t  handlerFuncPtr, ///< [IN] The event handler function.
    void*                                  contextPtr      ///< [IN] The handler's context.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for audio stream events.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_audio_RemoveDtmfStreamEventHandler
(
    le_audio_DtmfStreamEventHandlerRef_t addHandlerRef ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Play signalling DTMFs
 *
 * @return LE_OK            on success
 * @return LE_DUPLICATE     The thread is already started
 * @return LE_FAULT         on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_PlaySignallingDtmf
(
    const char*          dtmfPtr,   ///< [IN] The DTMFs to play.
    uint32_t             duration,  ///< [IN] The DTMF duration in milliseconds.
    uint32_t             pause      ///< [IN] The pause duration between tones in milliseconds.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to mute or unmute the interface
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_Mute
(
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    bool          mute              ///< [IN] true to mute the interface, false to unmute
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the value of a platform specific gain in the audio subsystem.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_NOT_FOUND     The specified gain's name is not recognized in your audio subsystem.
 * @return LE_OUT_OF_RANGE  The gain parameter is not between 0 and 100
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_SetPlatformSpecificGain
(
    const char*    gainNamePtr, ///< [IN] Name of the platform specific gain.
    int32_t        gain         ///< [IN] The gain value
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the value of a platform specific gain in the audio subsystem.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_NOT_FOUND     The specified gain's name is not recognized in your audio subsystem.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_GetPlatformSpecificGain
(
    const char* gainNamePtr, ///< [IN] Name of the platform specific gain.
    int32_t*    gainPtr      ///< [OUT] gain value [0..100]
);

//--------------------------------------------------------------------------------------------------
/**
 * Mute/Unmute the Call Waiting Tone.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_MuteCallWaitingTone
(
    bool    mute    ///< [IN] true to mute the Call Waiting tone, false otherwise.
);

//--------------------------------------------------------------------------------------------------
/**
 * Release pa internal parameters.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_audio_ReleasePaParameters
(
    le_audio_Stream_t* streamPtr    ///< [IN] input audio stream
);

#endif // LEGATO_PAAUDIO_INCLUDE_GUARD
