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
 * Symbols used to populate wave header file.
 */
//--------------------------------------------------------------------------------------------------
#define ID_RIFF    0x46464952
#define ID_WAVE    0x45564157
#define ID_FMT     0x20746d66
#define ID_DATA    0x61746164
#define FORMAT_PCM 1

//--------------------------------------------------------------------------------------------------
/**
 * The enumeration of all PA audio interface
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_AUDIO_IF_CODEC_MIC,
    PA_AUDIO_IF_CODEC_SPEAKER,
    PA_AUDIO_IF_DSP_FRONTEND_USB_RX,
    PA_AUDIO_IF_DSP_FRONTEND_USB_TX,
    PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX,
    PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX,
    PA_AUDIO_IF_DSP_FRONTEND_PCM_RX,
    PA_AUDIO_IF_DSP_FRONTEND_PCM_TX,
    PA_AUDIO_IF_DSP_FRONTEND_I2S_RX,
    PA_AUDIO_IF_DSP_FRONTEND_I2S_TX,
    PA_AUDIO_IF_DSP_FRONTEND_FILE_PLAY,
    PA_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE,
    PA_AUDIO_NUM_INTERFACES
}
pa_audio_If_t;

//--------------------------------------------------------------------------------------------------
/**
 * The wave header file structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint32_t riffId;         ///< "RIFF" constant. Marks the file as a riff file.
    uint32_t riffSize;       ///< Size of the overall file - 8 bytes
    uint32_t riffFmt;        ///< File Type Header. For our purposes, it always equals "WAVE".
    uint32_t fmtId;          ///< Equals "fmt ". Format chunk marker. Includes trailing null
    uint32_t fmtSize;        ///< Length of format data as listed above
    uint16_t audioFormat;    ///< Audio format (PCM)
    uint16_t channelsCount;  ///< Number of channels
    uint32_t sampleRate;     ///< Sample frequency in Hertz
    uint32_t byteRate;       ///< sampleRate * channelsCount * bps / 8
    uint16_t blockAlign;     ///< channelsCount * bps / 8
    uint16_t bitsPerSample;  ///< Bits per sample
    uint32_t dataId;         ///< "data" chunk header. Marks the beginning of the data section.
    uint32_t dataSize;       ///< Data size
} WavHeader_t;



//--------------------------------------------------------------------------------------------------
/**
 * Stream Events Bit Mask
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_AUDIO_BITMASK_MEDIA_EVENT = 0x1,      ///< event related to audio file's event
    PA_AUDIO_BITMASK_DTMF_DETECTION = 0x02   ///< event related to DTMF detection's event
}
pa_audio_StreamEventBitMask_t;

//--------------------------------------------------------------------------------------------------
/**
 * Stream event structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    pa_audio_If_t                   interface;        ///< Interface event
    pa_audio_StreamEventBitMask_t   streamEvent;
    union
    {
        le_audio_MediaEvent_t       mediaEvent;        ///< media event (plaback/capture interface)
        char                        dtmf;             ///< dtmf (dtmf detection interface)
    } event;
}
pa_audio_StreamEvent_t;

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration for recording PCM format.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PCM_RAW,
    PCM_WAVE
}
pa_audio_PcmFormat_t;

//--------------------------------------------------------------------------------------------------
/**
 * Configuration of PCM samples.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t sampleRate;            ///< Sample frequency in Hertz
    uint16_t channelsCount;         ///< Number of channels
    uint16_t bitsPerSample;         ///< Sampling resolution
    uint32_t fileSize;              ///< file size (Play file ohnly)
    pa_audio_PcmFormat_t pcmFormat;

}
pa_audio_SamplePcmConfig_t;

//--------------------------------------------------------------------------------------------------
/**
 * Configuration of AMR samples.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_audio_AmrMode_t amrMode;      ///< AMR mode
    bool dtx;                        ///< AMR discontinuous transmission
}
pa_audio_SampleAmrConfig_t;

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
typedef void (*pa_audio_StreamEventHandlerFunc_t)
(
    pa_audio_StreamEvent_t*         streamEventPtr,    ///< stream Event context
    void*                           contextPtr        ///< handler's context
);

//--------------------------------------------------------------------------------------------------
/**
 * Reference type used by Add/Remove functions for EVENT 'pa_audio_StreamEvent'
 */
//--------------------------------------------------------------------------------------------------
typedef struct pa_audio_StreamEventHandlerRef* pa_audio_StreamEventHandlerRef_t;

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
LE_SHARED le_result_t pa_audio_SetI2sChannelMode
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
LE_SHARED le_result_t pa_audio_SetMasterMode
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
LE_SHARED le_result_t pa_audio_SetSlaveMode
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
LE_SHARED le_result_t pa_audio_SetDspAudioPath
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
LE_SHARED le_result_t pa_audio_FlagForResetDspAudioPath
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
LE_SHARED void pa_audio_ResetDspAudioPath
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
LE_SHARED le_result_t pa_audio_SetGain
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
LE_SHARED le_result_t pa_audio_GetGain
(
    pa_audio_If_t interface,    ///< [IN] audio interface
    uint32_t     *gainPtr       ///< [OUT] gain value [0..100] (0 means 'muted', 100 is the
                                ///        maximum gain value)
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to play audio samples.
 *
 * @return LE_OK            The thread is started
 * @return LE_BAD_PARAMETER The interface is not valid
 * @return LE_DUPLICATE     The thread is already started
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_PlaySamples
(
    pa_audio_If_t interface,                        ///< [IN] audio interface
    int32_t       fd,                               ///< [IN] audio file descriptor
    pa_audio_SamplePcmConfig_t* samplePcmConfigPtr  ///< [IN] Sample configuration
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to pause the playback/capture thread.
 *
 * @return LE_OK            The function is succeeded
 * @return LE_FAULT         The function is failed
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_Pause
(
    pa_audio_If_t interface    ///< [IN] audio interface
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to resume the playback/capture thread.
 *
 * @return LE_OK            The function is succeeded
 * @return LE_FAULT         The function is failed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_Resume
(
    pa_audio_If_t interface    ///< [IN] audio interface
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop an interface.
 *
 * @return LE_OK            The function is succeeded
 * @return LE_FAULT         The function is failed
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_Stop
(
    pa_audio_If_t interface    ///< [IN] audio interface
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to capture an audio stream.
 *
 * @return LE_OK            The thread is started
 * @return LE_BAD_PARAMETER The interface is not valid
 * @return LE_DUPLICATE     The thread is already started
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_audio_Capture
(
    pa_audio_If_t interface,                        ///< [IN] audio interface
    int32_t       fd,                               ///< [IN] audio file descriptor
    pa_audio_SamplePcmConfig_t* samplePcmConfigPtr  ///< [IN] Sample configuration
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
    pa_audio_If_t interface    ///< [IN] audio interface
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
LE_SHARED LE_SHARED le_result_t pa_audio_StopDtmfDecoder
(
    pa_audio_If_t interface    ///< [IN] audio interface
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
LE_SHARED le_result_t pa_audio_EchoCancellerSwitch
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
LE_SHARED le_result_t pa_audio_FirFilterSwitch
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
LE_SHARED le_result_t pa_audio_IirFilterSwitch
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
LE_SHARED le_result_t pa_audio_AutomaticGainControlSwitch
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
LE_SHARED le_result_t pa_audio_SetProfile
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
LE_SHARED le_result_t pa_audio_GetProfile
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
LE_SHARED pa_audio_StreamEventHandlerRef_t pa_audio_AddStreamEventHandler
(
    pa_audio_StreamEventHandlerFunc_t  handlerFuncPtr, ///< [IN] The event handler function.
    void*                              contextPtr      ///< [IN] The handler's context.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for audio stream events.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_audio_RemoveStreamEventHandler
(
    pa_audio_StreamEventHandlerRef_t addHandlerRef ///< [IN]
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
 * Return true if an in-built Codec is present.
 *
 * @return true  if an in-built Codec is present, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED bool pa_audio_IsCodecPresent
(
    void
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
    pa_audio_If_t interface, ///< [IN] audio interface
    bool          mute       ///< [IN] true to mute the interface, false to unmute
);

#endif // LEGATO_PAAUDIO_INCLUDE_GUARD
