/** @file le_audio_local.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_LEAUDIOLOCAL_INCLUDE_GUARD
#define LEGATO_LEAUDIOLOCAL_INCLUDE_GUARD


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
    uint32_t byteRate;              ///< byterate of the played/recorded file
}
le_audio_SamplePcmConfig_t;

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
le_audio_SampleAmrConfig_t;

//--------------------------------------------------------------------------------------------------
/**
 * Audio resource opaque handle declaration
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct pcm_Handle* pcm_Handle_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference type used by Add/Remove functions for EVENT 'le_audio_StreamEvent'
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_audio_DtmfStreamEventHandlerRef* le_audio_DtmfStreamEventHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * The enumeration of all audio interface
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_AUDIO_IF_CODEC_MIC,
    LE_AUDIO_IF_CODEC_SPEAKER,
    LE_AUDIO_IF_DSP_FRONTEND_USB_RX,
    LE_AUDIO_IF_DSP_FRONTEND_USB_TX,
    LE_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX,
    LE_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX,
    LE_AUDIO_IF_DSP_FRONTEND_PCM_RX,
    LE_AUDIO_IF_DSP_FRONTEND_PCM_TX,
    LE_AUDIO_IF_DSP_FRONTEND_I2S_RX,
    LE_AUDIO_IF_DSP_FRONTEND_I2S_TX,
    LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY,
    LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE,
    LE_AUDIO_NUM_INTERFACES
}
le_audio_If_t;


//--------------------------------------------------------------------------------------------------
/**
 * The data parameters structure associated to the playback/capture thread.
 */
//--------------------------------------------------------------------------------------------------

typedef struct {
    pcm_Handle_t                pcmHandle;          ///< audio resource handle
    le_audio_SamplePcmConfig_t  pcmConfig;          ///< PCM paramaeters
    int                         fd;                 ///< File descriptor for file capture/playback
    le_thread_Ref_t             mainThreadRef;      ///< main thread reference
    le_audio_If_t               interface;          ///< audio interface
    bool                        pause;              ///< pause in capture
    le_audio_MediaEvent_t       mediaEvent;         ///< media event to be sent
    int                         framesFuncTimeout;  ///< Timeout for getFramesFunc callback
}
le_audio_PcmContext_t;

//--------------------------------------------------------------------------------------------------
/**
 * Audio format.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_AUDIO_FILE_WAVE,
    LE_AUDIO_FILE_AMR_NB,
    LE_AUDIO_FILE_AMR_WB,
    LE_AUDIO_FILE_MAX
}
le_audio_FileFormat_t;


//--------------------------------------------------------------------------------------------------
/**
 * Prototypes for encoding/decoding functions: initialisation, read, write, close
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_audio_Stream* le_audio_StreamPtr_t;
typedef struct le_audio_MediaThreadContext* le_audio_MediaThreadContextPtr_t;

typedef le_result_t (*InitMediaFunc_t)
(
    le_audio_StreamPtr_t                streamPtr,    ///< [IN] Decoding format
    le_audio_MediaThreadContextPtr_t    mediaCtxPtr   ///< [IN] Media thread context
);

typedef le_result_t (*ReadMediaFunc_t)
(
    le_audio_MediaThreadContextPtr_t mediaCtxPtr,    ///< [IN] Media thread context
    uint8_t                          *bufferOutPtr,  ///< [OUT] Decoding samples buffer output,
    uint32_t*                        readLenPtr      ///< [OUT] Length of the read data
);

typedef le_result_t (*WriteMediaFunc_t)
(
    le_audio_MediaThreadContextPtr_t mediaCtxPtr,    ///< [IN] Media thread context,
    uint8_t*                         bufferInPtr,    ///< [IN] Decoding samples buffer input,
    uint32_t                         bufferLen       ///< [IN] Buffer length
);

typedef le_result_t (*CloseMediaFunc_t)
(
    le_audio_MediaThreadContextPtr_t
);


//--------------------------------------------------------------------------------------------------
/**
 * Opaque type.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_audio_Opaque* le_audio_Codec_t;
typedef struct le_audio_Opaque* pa_audio_Params_t;

//--------------------------------------------------------------------------------------------------
/**
 * The data parameters structure associated to the Media thread.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_audio_MediaThreadContext
{
    le_audio_FileFormat_t            format;             ///< Format of the file (decoding)
    int32_t                          fd_arg;             ///< Application fd
    int32_t                          fd_pipe_input;      ///< pipe input
    int32_t                          fd_pipe_output;     ///< pipe output
    uint32_t                         fd_in;              ///< file descriptor to read
    uint32_t                         fd_out;             ///< file descriptor to write
    uint32_t                         bufferSize;         ///< Size of the required buffer
    le_sem_Ref_t                     threadSemaphore;    ///< semaphore to wait starting
    InitMediaFunc_t                  initFunc;           ///< Init function for play/capture
                                                         ///< in WAV/AMR format
    ReadMediaFunc_t                  readFunc;           ///< Read function for play/capture
                                                         ///< in WAV/AMR format
    WriteMediaFunc_t                 writeFunc;          ///< Write function for play/capture
                                                         ///< in WAV/AMR format
    CloseMediaFunc_t                 closeFunc;          ///< Close function for play/capture
                                                         ///< in WAV/AMR format
    le_audio_Codec_t                 codecParams;        ///< Codec parameters
}
le_audio_MediaThreadContext_t;

//--------------------------------------------------------------------------------------------------
/**
 * Audio stream structure.
 * Objects of this type are used to define an audio stream.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_audio_Stream {
    bool             isInput;                          ///< Flag to specify if the stream is an input
                                                       ///  or output.
    le_audio_If_t    audioInterface;                   ///< Audio interface identifier.
    uint32_t         gain;                             ///< Gain
    int32_t          fd;                               ///< Audio file descriptor for playback or
    le_hashmap_Ref_t connectorList;                    ///< capture list of connectors to which the
                                                       ///  audio stream is tied to.
    le_dls_List_t    streamRefWithEventHdlrList;       ///< List containing information related to
                                                       ///  stream event handlers
    le_event_Id_t    streamEventId;                    ///< Event ID to report stream events
    le_audio_StreamRef_t streamRef;                    ///< Stream reference
    le_audio_SamplePcmConfig_t  samplePcmConfig;       ///< Sample PCM configuration
    le_dls_List_t    sessionRefList;                   ///< Clients sessionRef list
    le_audio_SampleAmrConfig_t  sampleAmrConfig;       ///< Sample AMR configuration
    le_audio_Format_t   encodingFormat;                ///< Audio encoding format
    le_audio_PcmContext_t* pcmContextPtr;              ///< PCM playback/capture context
    le_audio_MediaThreadContext_t* mediaThreadContextPtr;///< Read Media thread
    le_audio_DtmfStreamEventHandlerRef_t dtmfEventHandler; ///< Dtmf stream event handler
    le_thread_Ref_t     mediaThreadRef;                 ///< Media thread reference
    bool                playFile;                      ///< Stream plays a file
    int8_t              deviceIdentifier;              ///< Device identifier
    int8_t              hwDeviceId;                    ///< Hardware Device identifier
    pa_audio_Params_t   PaParams;                      ///< PA Parameters
    bool echoCancellerEnabled;                         ///< Store the status of echo canceller
    bool noiseSuppressorEnabled;                       ///< Store the status of noise suppressor
}
le_audio_Stream_t;

//--------------------------------------------------------------------------------------------------
/**
 * Stream Events Bit Mask
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_AUDIO_BITMASK_MEDIA_EVENT = 0x1,      ///< event related to audio file's event
    LE_AUDIO_BITMASK_DTMF_DETECTION = 0x02   ///< event related to DTMF detection's event
}
le_audio_StreamEventBitMask_t;

//--------------------------------------------------------------------------------------------------
/**
 * Stream event structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_audio_Stream_t*              streamPtr;        ///< Stream object
    le_audio_StreamEventBitMask_t   streamEvent;
    union
    {
        le_audio_MediaEvent_t       mediaEvent;        ///< media event (plaback/capture interface)
        char                        dtmf;             ///< dtmf (dtmf detection interface)
    } event;
}
le_audio_StreamEvent_t;

//--------------------------------------------------------------------------------------------------
/**
 * A handler that is called whenever a dtmf event is notified.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_audio_DtmfStreamEventHandlerFunc_t)
(
    le_audio_StreamEvent_t*         streamEventPtr,    ///< stream Event context
    void*                           contextPtr        ///< handler's context
);


#endif // LEGATO_LEAUDIOLOCAL_INCLUDE_GUARD
