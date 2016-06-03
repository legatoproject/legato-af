/**
 * @file pa_audio_simu.c
 *
 * Simulation implementation of @ref c_pa_audio API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_audio_local.h"
#include "pa_audio.h"
#include "pa_audio_simu.h"
#include "pa_pcm_simu.h"
#include "pa_amr_simu.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

#define IS_INPUT_STREAM(X) (X == LE_AUDIO_IF_CODEC_MIC) ? true :\
                           ((X == LE_AUDIO_IF_DSP_FRONTEND_USB_RX) ? true :\
                           ((X == LE_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX) ? true :\
                           ((X == LE_AUDIO_IF_DSP_FRONTEND_PCM_RX) ? true :\
                           ((X == LE_AUDIO_IF_DSP_FRONTEND_I2S_RX) ? true :\
                           ((X == LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY) ? true : false)))))

#define IS_OUTPUT_STREAM(X) (X == LE_AUDIO_IF_CODEC_SPEAKER) ? true :\
                           ((X == LE_AUDIO_IF_DSP_FRONTEND_USB_TX) ? true :\
                           ((X == LE_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX) ? true :\
                           ((X == LE_AUDIO_IF_DSP_FRONTEND_PCM_TX) ? true :\
                           ((X == LE_AUDIO_IF_DSP_FRONTEND_I2S_TX) ? true :\
                           ((X == LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE) ? true : false)))))


//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

static le_event_Id_t DtmfEvent;
static char* DtmfPtr = NULL;
static uint32_t DtmfDuration = 0;
static uint32_t DtmfPause = 0;
static int8_t BuildAudioPath[LE_AUDIO_NUM_INTERFACES][LE_AUDIO_NUM_INTERFACES];


//--------------------------------------------------------------------------------------------------
/**
 * The first-layer File Event Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstDtmfLayeredHandler
(
    void*   reportPtr,
    void*   secondLayerFunc
)
{
    le_audio_StreamEvent_t*               streamEventPtr = reportPtr;
    le_audio_DtmfStreamEventHandlerFunc_t dtmfStreamEventHandler =
                                        (le_audio_DtmfStreamEventHandlerFunc_t) secondLayerFunc;

    dtmfStreamEventHandler( streamEventPtr, le_event_GetContextPtr() );
}

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Simulate a reception of a dtmf. Send the dtmf report.
 */
//--------------------------------------------------------------------------------------------------
void pa_audioSimu_ReceiveDtmf
(
    char dtmf
)
{
    le_audio_StreamEvent_t streamEvent;

    // Only one interface support the DTMF detection for now
    streamEvent.streamEvent = LE_AUDIO_BITMASK_DTMF_DETECTION;
    streamEvent.event.dtmf = dtmf;

    le_event_Report(DtmfEvent, &streamEvent, sizeof(le_audio_StreamEvent_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initializer.  Called automatically by the application framework at process start.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    memset(BuildAudioPath,0,LE_AUDIO_NUM_INTERFACES*LE_AUDIO_NUM_INTERFACES*sizeof(uint8_t));
    pa_amrSimu_Init();
    pa_pcmSimu_Init();

    DtmfEvent = le_event_CreateId("DtmfEventId", sizeof(le_audio_StreamEvent_t));
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
    le_audio_Stream_t* inputStreamPtr,   ///< [IN] input audio stream
    le_audio_Stream_t* outputStreamPtr   ///< [IN] output audio stream
)
{
    le_audio_If_t inputInterface = inputStreamPtr->audioInterface;
    le_audio_If_t outputInterface = outputStreamPtr->audioInterface;

    LE_ASSERT( IS_INPUT_STREAM(inputInterface) == true );
    LE_ASSERT( IS_OUTPUT_STREAM(outputInterface) == true );

    BuildAudioPath[inputInterface][outputInterface] += 1;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check the audio path set.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audioSimu_CheckAudioPathSet
(
    void
)
{
    le_audio_If_t inItf;
    le_audio_If_t outItf;

    for (inItf = 0; inItf < LE_AUDIO_NUM_INTERFACES; inItf++)
    {
        for (outItf = 0; outItf < LE_AUDIO_NUM_INTERFACES; outItf++)
        {
            if ( inItf == outItf )
            {
                // audio path can't be built between a stream and the same stream
                LE_ASSERT( BuildAudioPath[inItf][outItf] == 0 );
            }
            else if ( IS_OUTPUT_STREAM(inItf) )
            {
                // if the input stream is an output, it is not a valuable path
                LE_ASSERT( BuildAudioPath[inItf][outItf] == 0 );
            }
            else if ( IS_INPUT_STREAM(inItf) )
            {
                if ( IS_OUTPUT_STREAM(outItf) )
                {
                    // this is an expected audio path. It should be set only once
                    LE_ASSERT( BuildAudioPath[inItf][outItf] == 1 );
                }
                else if (IS_INPUT_STREAM(outItf))
                {
                    // if the output stream is an input, it is not a valuable path
                    LE_ASSERT( BuildAudioPath[inItf][outItf] == 0 );
                }
                else
                {
                    LE_FATAL("Unknown audio path");
                }
            }
            else
            {
                LE_FATAL("Unknown audio path");
            }
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check the reseted audio path.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audioSimu_CheckAudioPathReseted
(
    void
)
{
    le_audio_If_t inItf;
    le_audio_If_t outItf;

    for (inItf = 0; inItf < LE_AUDIO_NUM_INTERFACES; inItf++)
    {
        for (outItf = 0; outItf < LE_AUDIO_NUM_INTERFACES; outItf++)
        {
            LE_ASSERT( BuildAudioPath[inItf][outItf] == 0 );
        }
    }

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
le_result_t pa_audio_ResetDspAudioPath
(
    le_audio_Stream_t* inputStreamPtr,   ///< [IN] input audio stream
    le_audio_Stream_t* outputStreamPtr   ///< [IN] output audio stream
)
{
    le_audio_If_t inputInterface = inputStreamPtr->audioInterface;
    le_audio_If_t outputInterface = outputStreamPtr->audioInterface;

    LE_ASSERT( IS_INPUT_STREAM(inputInterface) == true );
    LE_ASSERT( IS_OUTPUT_STREAM(outputInterface) == true );

    BuildAudioPath[inputInterface][outputInterface] -= 1;

    LE_ASSERT( BuildAudioPath[inputInterface][outputInterface] >= 0 );

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the interface gain
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetGain
(
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    int32_t            gain         ///< [IN] gain value
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
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    int32_t*           gainPtr      ///< [OUT] gain value
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
    le_audio_Stream_t* streamPtr,
    uint32_t           timeslot
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
    le_audio_Stream_t* streamPtr
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
    le_audio_Stream_t* streamPtr    ///< [IN] input audio stream
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
    le_audio_Stream_t*     streamPtr,    ///< [IN] input audio stream
    le_audio_I2SChannel_t  mode
)
{
    return LE_OK;
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
    le_audio_Stream_t* streamPtr     ///< [IN] input audio stream
)
{
    LE_ASSERT( streamPtr->audioInterface == LE_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX );

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
    le_audio_Stream_t* streamPtr   ///< [IN] input audio stream
)
{
    LE_ASSERT( streamPtr->audioInterface == LE_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX );

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
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    le_onoff_t         switchOnOff  ///< [IN] switch ON or OFF
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
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    le_onoff_t         switchOnOff  ///< [IN] switch ON or OFF
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
    le_audio_Stream_t* streamPtr,    ///< [IN] input audio stream
    le_onoff_t         switchOnOff   ///< [IN] switch ON or OFF
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
    le_audio_Stream_t* streamPtr,    ///< [IN] input audio stream
    le_onoff_t         switchOnOff   ///< [IN] switch ON or OFF
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
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    le_onoff_t         switchOnOff  ///< [IN] switch ON or OFF
)
{
    return LE_FAULT;
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
    le_audio_DtmfStreamEventHandlerFunc_t handlerFuncPtr, ///< [IN] The event handler function.
    void*                                 contextPtr      ///< [IN] The handler's context.
)
{
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler( "DtmfHander",
                                                                    DtmfEvent,
                                                                    FirstDtmfLayeredHandler,
                                                                    handlerFuncPtr );

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_audio_DtmfStreamEventHandlerRef_t) handlerRef;
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
    uint32_t profile   ///< [IN] The audio profile.
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
    uint32_t* profilePtr  ///< [OUT] The audio profile.
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
 * Set dtmf configuration.
 */
//--------------------------------------------------------------------------------------------------
void pa_audioSimu_PlaySignallingDtmf
(
    char*          dtmfPtr,
    uint32_t       duration,
    uint32_t       pause
)
{
    DtmfPtr = dtmfPtr;
    DtmfDuration = duration;
    DtmfPause = pause;
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
    LE_ASSERT(strncmp(dtmfPtr, DtmfPtr, strlen(DtmfPtr))==0);
    LE_ASSERT(duration == DtmfDuration);
    LE_ASSERT(DtmfPause == pause);
    return LE_OK;
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
    le_audio_Stream_t* streamPtr,   ///< [IN] input audio stream
    bool          mute              ///< [IN] true to mute the interface, false to unmute
)
{
    // TODO: implement this one
    return LE_OK;
}

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
le_result_t pa_audio_SetPlatformSpecificGain
(
    const char*    gainNamePtr, ///< [IN] Name of the platform specific gain.
    int32_t        gain         ///< [IN] The gain value
)
{
    // TODO: implement this one
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the value of a platform specific gain in the audio subsystem.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_NOT_FOUND     The specified gain's name is not recognized in your audio subsystem.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_GetPlatformSpecificGain
(
    const char* gainNamePtr, ///< [IN] Name of the platform specific gain.
    int32_t*    gainPtr      ///< [OUT] gain value
)
{
    // TODO: implement this one
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Release pa internal parameters.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_audio_ReleasePaParameters
(
    le_audio_Stream_t* streamPtr
)
{
    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * Mute/Unmute the Call Waiting Tone.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_MuteCallWaitingTone
(
    bool    mute    ///< [IN] true to mute the Call Waiting tone, false otherwise.
)
{
    return LE_OK;
}
