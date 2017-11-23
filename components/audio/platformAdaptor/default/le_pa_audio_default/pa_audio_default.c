/**
 * @file pa_audio_default.c
 *
 * Default implementation of @ref pa_audio interface.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_audio_local.h"
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
 * Component initializer.  Called automatically by the application framework at process start.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the timeslot number of a PCM interface.
 *
 * @return LE_BAD_PARAMETER The audio stream is not valid.
 * @return LE_FAULT         The function failed to set the timeslot number.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetPcmTimeSlot
(
    le_audio_Stream_t* streamPtr,    ///< [IN] input audio stream
    uint32_t           timeslot
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the channel mode of an I2S interface.
 *
 * @return LE_BAD_PARAMETER The audio stream or interface is not valid.
 * @return LE_FAULT         The function failed to set the channel mode.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetI2sChannelMode
(
    le_audio_Stream_t*     streamPtr,    ///< [IN] input audio stream
    le_audio_I2SChannel_t  mode
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
    le_audio_Stream_t* streamPtr    ///< [IN] input audio stream
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
    le_audio_Stream_t* streamPtr    ///< [IN] input audio stream
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the DSP Audio path
 *
 * @return LE_BAD_PARAMETER The input or output audio stream is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetDspAudioPath
(
    le_audio_Stream_t* inputStreamPtr,   ///< [IN] input audio stream
    le_audio_Stream_t* outputStreamPtr   ///< [IN] output audio stream
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to reset the DSP Audio path
 *
 * @return LE_BAD_PARAMETER The input or output audio stream is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_ResetDspAudioPath
(
    le_audio_Stream_t* inputStreamPtr,   ///< [IN] input audio stream
    le_audio_Stream_t* outputStreamPtr   ///< [IN] output audio stream
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the interface gain
 *
 * @return LE_BAD_PARAMETER The input audio stream is invalid.
 * @return LE_OUT_OF_RANGE  The gain parameter is out of range.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetGain
(
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    int32_t            gain         ///< [IN] gain value
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the interface gain
 *
 * @return LE_BAD_PARAMETER The input audio stream or gain value is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_GetGain
(
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    int32_t*           gainPtr      ///< [OUT] gain value
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start the DTMF Decoder.
 *
 * @return LE_OK            The decoder is started
 * @return LE_BAD_PARAMETER The audio stream or interface is not valid
 * @return LE_FAULT         On other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_StartDtmfDecoder
(
    le_audio_Stream_t* streamPtr     ///< [IN] input audio stream
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop the DTMF Decoder.
 *
 * @return LE_OK            The decoder is started
 * @return LE_BAD_PARAMETER The interface is not valid
 * @return LE_FAULT         On other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_StopDtmfDecoder
(
    le_audio_Stream_t* streamPtr   ///< [IN] input audio stream
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable or disable the Noise Suppressor.
 *
 * @return LE_BAD_PARAMETER The audio interface is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_NoiseSuppressorSwitch
(
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    le_onoff_t         switchOnOff  ///< [IN] switch ON or OFF
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable or disable the Echo Canceller.
 *
 * @return LE_BAD_PARAMETER The audio interface is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_EchoCancellerSwitch
(
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    le_onoff_t         switchOnOff  ///< [IN] switch ON or OFF
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable or disable the FIR (Finite Impulse Response) filter on the
 * downlink or uplink audio path.
 *
 * @return LE_BAD_PARAMETER The audio stream or interface is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_FirFilterSwitch
(
    le_audio_Stream_t* streamPtr,    ///< [IN] input audio stream
    le_onoff_t         switchOnOff   ///< [IN] switch ON or OFF
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable or disable the IIR (Infinite Impulse Response) filter on
 * the downlink or uplink audio path.
 *
 * @return LE_BAD_PARAMETER The audio interface is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_IirFilterSwitch
(
    le_audio_Stream_t* streamPtr,    ///< [IN] input audio stream
    le_onoff_t         switchOnOff   ///< [IN] switch ON or OFF
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable or disable the automatic gain control on the selected
 * audio stream.
 *
 * @return LE_BAD_PARAMETER The audio stream is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_AutomaticGainControlSwitch
(
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    le_onoff_t         switchOnOff  ///< [IN] switch ON or OFF
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the audio profile.
 *
 * @return LE_UNAVAILABLE   On audio service initialization failure.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetProfile
(
    uint32_t profile   ///< [IN] The audio profile.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the audio profile in use.
 *
 * @return LE_UNAVAILABLE   On audio service initialization failure.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_GetProfile
(
    uint32_t* profilePtr  ///< [OUT] The audio profile.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

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
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

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
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

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
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

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
)
{
    LE_ERROR("Unsupported function called");
    return 0;
}

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
)
{
    LE_ERROR("Unsupported function called");
    return 0;
}

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
)
{
    LE_ERROR("Unsupported function called");
    return LE_AUDIO_COMPANDING_NONE;
}

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
)
{
    LE_ERROR("Unsupported function called");
    return 0;
}

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
)
{
    LE_ERROR("Unsupported function called");
    return LE_AUDIO_I2S_LEFT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for stream events notifications.
 *
 * @return an handler reference.
 */
//--------------------------------------------------------------------------------------------------
le_audio_DtmfStreamEventHandlerRef_t pa_audio_AddDtmfStreamEventHandler
(
    le_audio_DtmfStreamEventHandlerFunc_t  handlerFuncPtr, ///< [IN] The event handler function.
    void*                                  contextPtr      ///< [IN] The handler's context.
)
{
    LE_ERROR("Unsupported function called");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for audio stream events.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_audio_RemoveDtmfStreamEventHandler
(
    le_audio_DtmfStreamEventHandlerRef_t addHandlerRef ///< [IN]
)
{
    LE_ERROR("Unsupported function called");
}

//--------------------------------------------------------------------------------------------------
/**
 * Play signalling DTMFs
 *
 * @return LE_OK            on success
 * @return LE_DUPLICATE     The thread is already started
 * @return LE_FAULT         on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_PlaySignallingDtmf
(
    const char*          dtmfPtr,   ///< [IN] The DTMFs to play.
    uint32_t             duration,  ///< [IN] The DTMF duration in milliseconds.
    uint32_t             pause      ///< [IN] The pause duration between tones in milliseconds.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to mute or unmute the interface
 *
 * @return LE_BAD_PARAMETER The input audio stream is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_Mute
(
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    bool          mute              ///< [IN] true to mute the interface, false to unmute
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the value of a platform specific gain in the audio subsystem.
 *
 * @return LE_BAD_PARAMETER The pointer to name of the platform specific gain is invalid.
 * @return LE_NOT_FOUND     The specified gain's name is not recognized in your audio subsystem.
 * @return LE_OUT_OF_RANGE  The gain parameter is out of range.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetPlatformSpecificGain
(
    const char*    gainNamePtr, ///< [IN] Name of the platform specific gain.
    int32_t        gain         ///< [IN] The gain value
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the value of a platform specific gain in the audio subsystem.
 *
 * @return LE_BAD_PARAMETER The pointer to name of the platform specific gain is invalid.
 * @return LE_NOT_FOUND     The specified gain's name is not recognized in your audio subsystem.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_GetPlatformSpecificGain
(
    const char* gainNamePtr, ///< [IN] Name of the platform specific gain.
    int32_t*    gainPtr      ///< [OUT] gain value [0..100]
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Mute/Unmute the Call Waiting Tone.
 *
 * @return LE_UNAVAILABLE   On audio service initialization failure.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_MuteCallWaitingTone
(
    bool    mute    ///< [IN] true to mute the Call Waiting tone, false otherwise.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Release pa internal parameters.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_audio_ReleasePaParameters
(
    le_audio_Stream_t* streamPtr    ///< [IN] input audio stream
)
{
    LE_ERROR("Unsupported function called");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Noise Suppressor status
 *
 * @return LE_BAD_PARAMETER The input audio stream or noise suppressor status is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_GetNoiseSuppressorStatus
(
    le_audio_Stream_t* streamPtr,                   ///< [IN] input audio stream
    bool*              noiseSuppressorStatusPtr     ///< [OUT] Noise Suppressor status
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Echo Canceller status
 *
 * @return LE_BAD_PARAMETER The input audio stream or echo canceller status is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_GetEchoCancellerStatus
(
    le_audio_Stream_t* streamPtr,                   ///< [IN] input audio stream
    bool*              echoCancellerStatusPtr       ///< [OUT] Echo Canceller status
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}
