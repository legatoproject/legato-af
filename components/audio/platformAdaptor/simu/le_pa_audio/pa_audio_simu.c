/**
 * @file pa_audio_simu.c
 *
 * Simulation implementation of @ref c_pa_audio API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_audio.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * DTMF event ID used to report DTMFs to the registered event handlers.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t DtmfEvent;

//--------------------------------------------------------------------------------------------------
/**
 * The DTMF user's event handler reference.
 */
//--------------------------------------------------------------------------------------------------
static le_event_HandlerRef_t DtmfHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer DTMF Event Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerDtmfRxHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    char*                      dtmfPtr = reportPtr;
    pa_audio_DtmfHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    LE_DEBUG("[%c] DTMF detected!", *dtmfPtr);
    clientHandlerFunc(*dtmfPtr);
}

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
    // Create the event for DTMF handlers.
    DtmfEvent = le_event_CreateId("DtmfEvent", sizeof(char));
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
void pa_audio_ResetDspAudioPath
(
    void
)
{
    return ;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the interface gain
 *
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
    le_audio_I2SChannel_t   mode
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to play audio samples.
 *
 * @return LE_OK            The thread is started
 * @return LE_BAD_PARAMETER The interface is not valid
 * @return LE_DUPLICATE     The thread is already started
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_PlaySamples
(
    pa_audio_If_t interface,                        ///< [IN] audio interface
    int32_t       fd,                               ///< [IN] audio file descriptor
    pa_audio_SamplePcmConfig_t* samplePcmConfigPtr  ///< [IN] Sample configuration
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to pause the playback/capture thread.
 *
 * @return LE_OK            The function is succeeded
 * @return LE_FAULT         The function is failed
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_Pause
(
    pa_audio_If_t interface    ///< [IN] audio interface
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to resume the playback/capture thread.
 *
 * @return LE_OK            The function is succeeded
 * @return LE_FAULT         The function is failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_Resume
(
    pa_audio_If_t interface    ///< [IN] audio interface
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop an interface.
 *
 * @return LE_OK            The function is succeeded
 * @return LE_FAULT         The function is failed
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_Stop
(
    pa_audio_If_t interface    ///< [IN] audio interface
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to capture an audio stream.
 *
 * @return LE_OK            The thread is started
 * @return LE_BAD_PARAMETER The interface is not valid
 * @return LE_DUPLICATE     The thread is already started
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_Capture
(
    pa_audio_If_t interface,                        ///< [IN] audio interface
    int32_t       fd,                               ///< [IN] audio file descriptor
    pa_audio_SamplePcmConfig_t* samplePcmConfigPtr  ///< [IN] Sample configuration
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start the DTMF Decoder.
 *
 * @return LE_OK            The decoder is started
 * @return LE_BAD_PARAMETER The interface is not valid
 * @return LE_NOT_POSSIBLE  On other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_StartDtmfDecoder
(
    pa_audio_If_t interface    ///< [IN] audio interface
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop the DTMF Decoder.
 *
 * @return LE_OK            The decoder is started
 * @return LE_BAD_PARAMETER The interface is not valid
 * @return LE_NOT_POSSIBLE  On other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_StopDtmfDecoder
(
    pa_audio_If_t interface    ///< [IN] audio interface
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for DTMF notifications.
 *
 * @return LE_NOT_POSSIBLE  The function failed to register the handler.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetDtmfDetectorHandler
(
    pa_audio_DtmfHandlerFunc_t   handlerFuncPtr ///< [IN] The event handler function.
)
{
    LE_DEBUG("Set new Call Event handler.");

    LE_FATAL_IF(handlerFuncPtr == NULL, "The DTMF handler function is NULL.");

    DtmfHandlerRef = le_event_AddLayeredHandler("DtmfRxHandler",
                                                DtmfEvent,
                                                FirstLayerDtmfRxHandler,
                                                (le_event_HandlerFunc_t)handlerFuncPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable or disable the Noise Suppressor.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_NoiseSuppressorSwitch
(
    pa_audio_If_t interface,    ///< [IN] audio interface
    le_onoff_t    switchOnOff   ///< [IN] switch ON or OFF
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable or disable the Echo Canceller.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_EchoCancellerSwitch
(
    pa_audio_If_t interface,    ///< [IN] audio interface
    le_onoff_t    switchOnOff   ///< [IN] switch ON or OFF
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable or disable the FIR (Finite Impulse Response) filter on the
 * downlink or uplink audio path.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_FirFilterSwitch
(
    pa_audio_If_t interface,    ///< [IN] audio interface
    le_onoff_t    switchOnOff   ///< [IN] switch ON or OFF
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable or disable the IIR (Infinite Impulse Response) filter on
 * the downlink or uplink audio path.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_IirFilterSwitch
(
    pa_audio_If_t interface,    ///< [IN] audio interface
    le_onoff_t    switchOnOff   ///< [IN] switch ON or OFF
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable or disable the automatic gain control on the selected
 * audio stream.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_AutomaticGainControlSwitch
(
    pa_audio_If_t interface,    ///< [IN] audio interface
    le_onoff_t    switchOnOff   ///< [IN] switch ON or OFF
)
{
    return LE_FAULT;
}

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
)
{
    return LE_FAULT;
}

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
)
{
    return LE_FAULT;
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
    return LE_AUDIO_I2S_STEREO;
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
    // TODO: implement this one
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
    // TODO: implement this one
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
    // TODO: implement this one
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
    // TODO: implement this one
    return 16000;
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
    // TODO: implement this one
    return 1;
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
    // TODO: implement this one
    return LE_AUDIO_COMPANDING_NONE;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for stream events notifications.
 *
 * @return an handler reference.
 */
//--------------------------------------------------------------------------------------------------
pa_audio_StreamEventHandlerRef_t pa_audio_AddStreamEventHandler
(
    pa_audio_StreamEventHandlerFunc_t handlerFuncPtr, ///< [IN] The event handler function.
    void*                             contextPtr      ///< [IN] The handler's context.
)
{
    // TODO: implement this one
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for audio stream events.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_audio_RemoveStreamEventHandler
(
    pa_audio_StreamEventHandlerRef_t addHandlerRef ///< [IN]
)
{
    // TODO: implement this one
    return;
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
    // TODO: implement this one
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Return true if an in-built Codec is present.
 *
 * @return true  if an in-built Codec is present, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool pa_audio_IsCodecPresent
(
    void
)
{
    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to mute or unmute the interface
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_Mute
(
    pa_audio_If_t interface, ///< [IN] audio interface
    bool          mute       ///< [IN] true to mute the interface, false to unmute
)
{
    // TODO: implement this one
    return LE_OK;
}
