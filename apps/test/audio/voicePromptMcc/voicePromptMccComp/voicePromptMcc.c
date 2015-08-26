/**
 * This app plays voice prompts during a voice call as follows:
 *
 * - Play VOICE_PROMPT_START voice prompt (just once);
 * - Play DIALING audio file (in loop);
 * - Initiate the voice call;
 * - As soon as the call event is ALERTING, stop DIALING audio file playback and play RINGTONE audio
 *   file (in loop);
 * - As soon as the call event is CONNECTED, stop RINGTONE audio file playback and speak/listen to
 *   the phone;
 * - Hangup the call;
 * - As soon as the call event is TERMINATED, play VOICE_PROMPT_END voice prompt (just once).
 *
 * Audio interfaces are the analog ones (microphone and speaker).
 *
 * You must issue the following commands:
 * @verbatim
   $ app start voicePromptMcc
   $ execInApp voicePromptMcc voicePromptMcc <phone number>
   @endverbatim
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include <pthread.h>
#include <unistd.h>
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------
#define VOICE_PROMPT_START  "/usr/share/sounds/SwiECallStartMono.wav"
#define VOICE_PROMPT_END    "/usr/share/sounds/SwiECallCanceledMono.wav"
#define DIALING             "/usr/share/sounds/SwiDialingMono.wav"
#define RINGTONE            "/usr/share/sounds/SwiRingBackToneFrMono.wav"

//--------------------------------------------------------------------------------------------------
// Data structures.
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

typedef struct
{
    le_thread_Ref_t mainThreadRef;
    bool            playInLoop;
    bool            playDone;
}
SamplesThreadCtx_t;

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

static le_mcc_CallRef_t TestCallRef;

static le_audio_StreamRef_t MdmRxAudioRef = NULL;
static le_audio_StreamRef_t MdmTxAudioRef = NULL;
static le_audio_StreamRef_t FeInRef = NULL;
static le_audio_StreamRef_t FeOutRef = NULL;
static le_audio_StreamRef_t PlayerRef = NULL;

static le_audio_ConnectorRef_t AudioInputConnectorRef = NULL;
static le_audio_ConnectorRef_t AudioOutputConnectorRef = NULL;

static int   AudioFileFd = -1;
static int   Pipefd[2];

static SamplesThreadCtx_t SamplesThreadCtx;
static le_thread_Ref_t PlaySamplesRef = NULL;

static le_sem_Ref_t MediaSem;


static void DisconnectAllAudio(void);
static void PlaySamples(void* param1Ptr,void* param2Ptr);




//--------------------------------------------------------------------------------------------------
/**
 *  Stop playback.
 *
 */
//--------------------------------------------------------------------------------------------------
static void StopFilePlayback
(
    void
)
{
    if (PlaySamplesRef)
    {
        le_thread_Cancel(PlaySamplesRef);
    }

    SamplesThreadCtx.playDone = false;
    SamplesThreadCtx.playInLoop = false;
    memset(&SamplesThreadCtx, 0, sizeof(SamplesThreadCtx_t));
    close(AudioFileFd);
    le_sem_Post(MediaSem);
}


//--------------------------------------------------------------------------------------------------
/**
 *  Play Samples thread destructor.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DestroyPlayThread
(
    void *contextPtr
)
{
    LE_INFO("DestroyPlayThread playDone %d PlayInLoop %d",
            SamplesThreadCtx.playDone,
            SamplesThreadCtx.playInLoop);

    if( SamplesThreadCtx.playDone && SamplesThreadCtx.playInLoop )
    {
        LE_INFO("Play in loop activated, restart PlaySamples thread");
        le_event_QueueFunctionToThread(SamplesThreadCtx.mainThreadRef,
                                       PlaySamples,
                                       contextPtr,
                                       NULL);
    }
    else
    {
        LE_INFO("End playback");
        le_sem_Post(MediaSem);
    }

    PlaySamplesRef = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Play Samples thread.
 *
 */
//--------------------------------------------------------------------------------------------------
static void* PlaySamplesThread
(
    void* contextPtr
)
{
    char data[1024];
    int32_t len = 1;

    lseek(AudioFileFd, 0+sizeof(WavHeader_t), SEEK_SET);

    LE_INFO("Read audio file...");

    while ((len = read(AudioFileFd, data, 1024)) > 0)
    {
        int32_t wroteLen = write(Pipefd[1], data, len);
        if (wroteLen <= 0)
        {
            LE_ERROR("write error %d", wroteLen);
            return NULL;
        }
    }

    LE_INFO("End of audio file reached");
    SamplesThreadCtx.playDone = true;

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Play Samples.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PlaySamples
(
    void* param1Ptr,
    void* param2Ptr
)
{
    if (PlaySamplesRef == NULL)
    {
        PlaySamplesRef = le_thread_Create("PlaySamples", PlaySamplesThread, NULL);

        le_thread_AddChildDestructor(PlaySamplesRef,
                                     DestroyPlayThread,
                                     NULL);

        le_thread_Start(PlaySamplesRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Play a file
 *
 */
//--------------------------------------------------------------------------------------------------
static void PlayFile
(
    const char* audioFilePathPtr,
    bool        isInLoop
)
{
    le_sem_Wait(MediaSem);
    if ((AudioFileFd=open(audioFilePathPtr, O_RDONLY)) == -1)
    {
        LE_ERROR("Open file %s failure: errno.%d (%s)",  audioFilePathPtr, errno, strerror(errno));
        DisconnectAllAudio();
        exit(0);
    }
    else
    {
        LE_INFO("Open file %s with AudioFileFd.%d",  audioFilePathPtr, AudioFileFd);
    }

    memset(&SamplesThreadCtx,0,sizeof(SamplesThreadCtx_t));
    SamplesThreadCtx.mainThreadRef = le_thread_GetCurrent();
    SamplesThreadCtx.playDone = false;
    SamplesThreadCtx.playInLoop = isInLoop;

    PlaySamples(NULL, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect audio
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudio
(
    void
)
{
    // Redirect audio to the in-built Microphone and Speaker.
    FeOutRef = le_audio_OpenSpeaker();
    LE_ERROR_IF((FeOutRef==NULL), "OpenSpeaker returns NULL!");
    FeInRef = le_audio_OpenMic();
    LE_ERROR_IF((FeInRef==NULL), "OpenMic returns NULL!");

    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef==NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef==NULL), "AudioOutputConnectorRef is NULL!");

    LE_ERROR_IF((le_audio_Connect(AudioInputConnectorRef, FeInRef)!=LE_OK),
                "Failed to connect Mic on Input connector!");
    LE_ERROR_IF((le_audio_Connect(AudioOutputConnectorRef, FeOutRef)!=LE_OK),
                "Failed to connect Speaker on Output connector!");

    // Play local on output connector.
    PlayerRef = le_audio_OpenPlayer();
    LE_ERROR_IF((PlayerRef==NULL), "OpenFilePlayback returns NULL!");

    LE_ERROR_IF((le_audio_Connect(AudioOutputConnectorRef, PlayerRef) != LE_OK),
                "Failed to connect FilePlayback on output connector!");

    LE_ERROR_IF((pipe(Pipefd) == -1), "Failed to create the pipe");

    LE_ERROR_IF((le_audio_SetSamplePcmChannelNumber(PlayerRef, 1) != LE_OK),
                "Cannot set the channel number");
    LE_ERROR_IF((le_audio_SetSamplePcmSamplingRate(PlayerRef, 44100) != LE_OK),
                "Cannot set the sampling rate");
    LE_ERROR_IF((le_audio_SetSamplePcmSamplingResolution(PlayerRef, 16) != LE_OK),
                "Cannot set the sampling resolution");

    LE_ERROR_IF((le_audio_PlaySamples(PlayerRef, Pipefd[0]) != LE_OK), "Cannot play samples");

    LE_ERROR_IF((le_audio_SetGain(PlayerRef, 60) != LE_OK), "Cannot set multimedia gain");
    LE_ERROR_IF((le_audio_SetGain(FeOutRef, 60) != LE_OK), "Cannot set speaker gain");
}


//--------------------------------------------------------------------------------------------------
/**
 * Disconnect all streams and connectors
 *
 */
//--------------------------------------------------------------------------------------------------
static void DisconnectAllAudio
(
    void
)
{
    if (AudioInputConnectorRef)
    {
        if(PlayerRef)
        {
            LE_INFO("Disconnect %p from connector.%p", PlayerRef, AudioInputConnectorRef);
            le_audio_Disconnect(AudioInputConnectorRef, PlayerRef);
        }
        if (FeInRef)
        {
            LE_INFO("Disconnect %p from connector.%p", FeInRef, AudioInputConnectorRef);
            le_audio_Disconnect(AudioInputConnectorRef, FeInRef);
        }
        if(MdmTxAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", MdmTxAudioRef, AudioInputConnectorRef);
            le_audio_Disconnect(AudioInputConnectorRef, MdmTxAudioRef);
        }
    }
    if(AudioOutputConnectorRef)
    {
        if(PlayerRef)
        {
            LE_INFO("Disconnect %p from connector.%p", PlayerRef, AudioOutputConnectorRef);
            le_audio_Disconnect(AudioOutputConnectorRef, PlayerRef);
        }
        if(FeOutRef)
        {
            LE_INFO("Disconnect %p from connector.%p", FeOutRef, AudioOutputConnectorRef);
            le_audio_Disconnect(AudioOutputConnectorRef, FeOutRef);
        }
        if(MdmRxAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", MdmRxAudioRef, AudioOutputConnectorRef);
            le_audio_Disconnect(AudioOutputConnectorRef, MdmRxAudioRef);
        }
    }

    if(AudioInputConnectorRef)
    {
        le_audio_DeleteConnector(AudioInputConnectorRef);
        AudioInputConnectorRef = NULL;
    }
    if(AudioOutputConnectorRef)
    {
        le_audio_DeleteConnector(AudioOutputConnectorRef);
        AudioOutputConnectorRef = NULL;
    }

    if(PlayerRef)
    {
        le_audio_Close(PlayerRef);
        PlayerRef = NULL;
    }
    if(FeInRef)
    {
        le_audio_Close(FeInRef);
        FeInRef = NULL;
    }
    if(FeOutRef)
    {
        le_audio_Close(FeOutRef);
        FeOutRef = NULL;
    }
    if(MdmRxAudioRef)
    {
        le_audio_Close(MdmRxAudioRef);
        MdmRxAudioRef = NULL;
    }
    if(MdmTxAudioRef)
    {
        le_audio_Close(MdmTxAudioRef);
        MdmTxAudioRef = NULL;
    }

    close(AudioFileFd);
    close(Pipefd[0]);
    close(Pipefd[1]);

    exit(0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Call Event Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyCallEventHandler
(
    le_mcc_CallRef_t  callRef,
    le_mcc_Event_t    callEvent,
    void*             contextPtr
)
{
    le_result_t         res;

    if (callEvent == LE_MCC_EVENT_ALERTING)
    {
        LE_INFO("Call event is LE_MCC_EVENT_ALERTING.");
        StopFilePlayback();
        PlayFile(RINGTONE, true);
    }
    else if (callEvent == LE_MCC_EVENT_CONNECTED)
    {
        LE_INFO("Call event is LE_MCC_EVENT_CONNECTED.");

        StopFilePlayback();

        // Connect voice call to audio
        MdmRxAudioRef = le_audio_OpenModemVoiceRx();
        LE_ERROR_IF((MdmRxAudioRef==NULL), "GetRxAudioStream returns NULL!");
        MdmTxAudioRef = le_audio_OpenModemVoiceTx();
        LE_ERROR_IF((MdmTxAudioRef==NULL), "GetTxAudioStream returns NULL!");
        res = le_audio_Connect(AudioInputConnectorRef, MdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, MdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
    else if (callEvent == LE_MCC_EVENT_TERMINATED)
    {
        LE_INFO("Call event is LE_MCC_EVENT_TERMINATED.");
        le_mcc_TerminationReason_t term = le_mcc_GetTerminationReason(callRef);
        switch(term)
        {
            case LE_MCC_TERM_NETWORK_FAIL:
                LE_INFO("Termination reason is LE_MCC_TERM_NETWORK_FAIL");
                break;

            case LE_MCC_TERM_UNASSIGNED_NUMBER:
                LE_INFO("Termination reason is LE_MCC_TERM_UNASSIGNED_NUMBER");
                break;

            case LE_MCC_TERM_USER_BUSY:
                LE_INFO("Termination reason is LE_MCC_TERM_USER_BUSY");
                break;

            case LE_MCC_TERM_LOCAL_ENDED:
                LE_INFO("Termination reason is LE_MCC_TERM_LOCAL_ENDED");
                break;

            case LE_MCC_TERM_REMOTE_ENDED:
                LE_INFO("Termination reason is LE_MCC_TERM_REMOTE_ENDED");
                break;

            case LE_MCC_TERM_UNDEFINED:
                LE_INFO("Termination reason is LE_MCC_TERM_UNDEFINED");
                break;

            default:
                LE_INFO("Termination reason is %d", term);
                break;
        }

        StopFilePlayback();
        PlayFile(VOICE_PROMPT_END, false);

        le_mcc_Delete(callRef);
    }
    else if (callEvent == LE_MCC_EVENT_INCOMING)
    {
        LE_INFO("Call event is LE_MCC_EVENT_INCOMING.");
    }
    else
    {
        LE_INFO("Unknowm Call event.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Helper.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage()
{
    int idx;
    bool sandboxed = (getuid() != 0);
    const char * usagePtr[] = {
            "Usage of the voicePromptMccTest test is:",
            "   execInApp voicePromptMcc voicePromptMcc <phone number>",
    };

    for(idx = 0; idx < NUM_ARRAY_MEMBERS(usagePtr); idx++)
    {
        if(sandboxed)
        {
            LE_INFO("%s", usagePtr[idx]);
        }
        else
        {
            fprintf(stderr, "%s\n", usagePtr[idx]);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The signal event handler function for SIGINT/SIGTERM when process dies.
 */
//--------------------------------------------------------------------------------------------------
static void SigHandler
(
    int sigNum
)
{
    LE_INFO("Disconnect All Audio and end call");

    DisconnectAllAudio();
    le_mcc_HangUp(TestCallRef);

    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the test component.
 * Start application with 'app start voicePromptMccTest' command
 * Execute application with 'execInApp voicePromptMcc voicePromptMcc (see PrintUsage())'
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    const char* destinationNumber;

    LE_INFO("Init");

    if (le_arg_NumArgs() == 1)
    {
        // Register a signal event handler for SIGINT when user interrupts/terminates process
        signal(SIGINT, SigHandler);

        LE_INFO("======== Start voicePromptMcc Test ========");

        destinationNumber = le_arg_GetArg(0);

        MediaSem = le_sem_Create("MediaSem", 1);

        // Connect audio stuff
        ConnectAudio();

        // Start playing voice prompt once
        PlayFile(VOICE_PROMPT_START, false);

        // Start playing audio file in loop
        PlayFile(DIALING, true);

        // Initiate the call
        le_mcc_AddCallEventHandler(MyCallEventHandler, NULL);
        TestCallRef = le_mcc_Create(destinationNumber);
        le_mcc_Start(TestCallRef);

        LE_INFO("======== voicePromptMcc Test started successfully ========");
    }
    else
    {
        PrintUsage();
        LE_INFO("EXIT voicePromptMcc Test");
        exit(EXIT_FAILURE);
    }
}

