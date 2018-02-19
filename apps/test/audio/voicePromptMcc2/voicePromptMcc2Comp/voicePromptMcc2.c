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
   $ app start voicePromptMcc2
   $ app runProc voicePromptMcc2 --exe=voicePromptMcc2 -- <phone number> [AMR]
   @endverbatim
 *
 * @note If Ctrl-C is issued while the call is connected, the last voice prompt (VOICE_PROMPT_END)
 *       won't be played since we exit before the playback starts.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include <pthread.h>
#include <unistd.h>
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------
#define VOICE_PROMPT_START  "/usr/share/sounds/SwiECallStartMono"
#define VOICE_PROMPT_END    "/usr/share/sounds/SwiECallCanceledMono"
#define DIALING             "/usr/share/sounds/SwiDialingMono"
#define RINGTONE            "/usr/share/sounds/SwiRingBackToneFrMono"

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

static le_audio_Format_t AudioFileFormat = LE_AUDIO_WAVE;

static le_mcc_CallRef_t TestCallRef;

static le_audio_StreamRef_t MdmRxAudioRef = NULL;
static le_audio_StreamRef_t MdmTxAudioRef = NULL;
static le_audio_StreamRef_t FeInRef = NULL;
static le_audio_StreamRef_t FeOutRef = NULL;
static le_audio_StreamRef_t PlayerRef = NULL;

static le_audio_ConnectorRef_t AudioInputConnectorRef = NULL;
static le_audio_ConnectorRef_t AudioOutputConnectorRef = NULL;

static le_audio_MediaHandlerRef_t MediaHandlerRef = NULL;

static int   AudioFileFd = -1;

static bool IsVoicePromptStart = false;
static bool IsDialing = false;
static bool IsInLoop = false;



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
    char filePathPtr[100]={0};

    IsInLoop = isInLoop;

    if (AudioFileFormat == LE_AUDIO_AMR)
    {
        snprintf(filePathPtr, 100, "%s.amr", audioFilePathPtr);
    }
    else
    {
        snprintf(filePathPtr, 100, "%s.wav", audioFilePathPtr);
    }

    if ((AudioFileFd=open(filePathPtr, O_RDONLY)) == -1)
    {
        LE_ERROR("Open file %s failure: errno.%d (%s)",  filePathPtr, errno, strerror(errno));
        exit(0);
    }
    else
    {
        LE_INFO("Play file %s %s with AudioFileFd.%d",
                ((IsInLoop) ? "IN LOOP" : "ONCE"),
                filePathPtr,
                AudioFileFd);
    }

    LE_ERROR_IF((le_audio_PlayFile(PlayerRef, AudioFileFd) != LE_OK), "Cannot play file");
}

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
    LE_INFO("Stop file playback on fd.%d", AudioFileFd);
    LE_FATAL_IF((le_audio_Stop(PlayerRef) != LE_OK), "Cannot stop file");

    // Closing AudioFileFd is unnecessary since the messaging infrastructure underneath
    // le_audio_xxx APIs that use it would close it.
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Stream Event Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyMediaEventHandler
(
    le_audio_StreamRef_t          streamRef,
    le_audio_MediaEvent_t         event,
    void*                         contextPtr
)
{
    switch(event)
    {
        case LE_AUDIO_MEDIA_NO_MORE_SAMPLES:
            LE_INFO("Media event is LE_AUDIO_MEDIA_NO_MORE_SAMPLES.");
            break;

        case LE_AUDIO_MEDIA_ENDED:
            LE_INFO("Media event is LE_AUDIO_MEDIA_ENDED.");
            if (IsInLoop)
            {
                // Start playing audio file in loop
                LE_INFO("Play in loop...");
                LE_ERROR_IF((le_audio_PlayFile(streamRef, LE_AUDIO_NO_FD) != LE_OK),
                            "Cannot play file");
            }

            if(IsVoicePromptStart)
            {
                PlayFile(DIALING, true);
                IsVoicePromptStart = false;
                IsDialing = true;
            }
            else if(IsDialing)
            {
                // Initiate the call
                LE_INFO("Start call");
                le_mcc_Start(TestCallRef);
                IsDialing = false;
            }
            break;

        case LE_AUDIO_MEDIA_ERROR:
            LE_INFO("Media event is LE_AUDIO_MEDIA_ERROR.");
        break;
        default:
            LE_INFO("Media event is %d.", event);
        break;
    }
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

    MdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((MdmRxAudioRef==NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_audio_OpenModemVoiceTx();
    LE_ERROR_IF((MdmTxAudioRef==NULL), "GetTxAudioStream returns NULL!");

    // Play local on output connector.
    PlayerRef = le_audio_OpenPlayer();
    LE_ERROR_IF((PlayerRef==NULL), "OpenFilePlayback returns NULL!");
    LE_ERROR_IF((le_audio_Connect(AudioOutputConnectorRef, PlayerRef) != LE_OK),
                "Failed to connect FilePlayback on output connector!");

    MediaHandlerRef = le_audio_AddMediaHandler(PlayerRef, MyMediaEventHandler, NULL);
    LE_ERROR_IF((MediaHandlerRef==NULL), "AddMediaHandler returns NULL!");

    // Set profile and specific gains for AR7/AR8 (won't work on other platforms)
    LE_ERROR_IF((le_audio_SetProfile(1) != LE_OK), "Cannot set profile 1");
    LE_ERROR_IF((le_audio_SetGain(PlayerRef, 0x300) != LE_OK), "Cannot set multimedia gain");
    LE_ERROR_IF((le_audio_SetGain(MdmRxAudioRef, 5) != LE_OK), "Cannot set MdmRxAudioRef gain");
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
            case LE_MCC_TERM_LOCAL_ENDED:
            case LE_MCC_TERM_REMOTE_ENDED:
                LE_INFO("Termination reason is LE_MCC_TERM_REMOTE_ENDED or LE_MCC_TERM_LOCAL_ENDED");
                break;

            case LE_MCC_TERM_NETWORK_FAIL:
            case LE_MCC_TERM_UNASSIGNED_NUMBER:
            case LE_MCC_TERM_USER_BUSY:
            case LE_MCC_TERM_UNDEFINED:
            default:
                LE_INFO("Termination reason is %d", term);
                StopFilePlayback();
                break;
        }

        PlayFile(VOICE_PROMPT_END, false);

        le_mcc_Delete(callRef);
        if (callRef == TestCallRef)
        {
            TestCallRef = NULL;
        }
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
            "Usage of the voicePromptMcc2 test is:",
            "   \"app runProc voicePromptMcc2 --exe=voicePromptMcc2 -- <phone number>\" with .wav file",
            "   \"app runProc voicePromptMcc2 --exe=voicePromptMcc2 -- <phone number> AMR\" with .amr file"
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
    if (TestCallRef)
    {
        LE_INFO("HangUp call");
        le_mcc_HangUp(TestCallRef);
    }

    // Closing AudioFileFd is unnecessary since the messaging infrastructure underneath
    // le_audio_xxx APIs that use it would close it.

    // If Ctrl-C is issued while the call is connected, the last voice prompt (VOICE_PROMPT_END)
    // won't be played since we exit before the playback starts.
    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the test component.
 * Start application with 'app start voicePromptMcc2' command
 * Execute application with 'app runProc voicePromptMcc2 --exe=voicePromptMcc2 (see PrintUsage())'
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    const char* destinationNumber;

    if ((le_arg_NumArgs() >= 1) && (le_arg_NumArgs() <= 2))
    {
        // Register a signal event handler for SIGINT when user interrupts/terminates process
        signal(SIGINT, SigHandler);

        LE_INFO("======== Start voicePromptMcc2 Test ========");
        destinationNumber = le_arg_GetArg(0);
        if (NULL == destinationNumber)
        {
            LE_ERROR("destinationNumber is NULL");
            exit(EXIT_FAILURE);
        }

        if ( le_arg_NumArgs() == 2 )
        {
            const char* mode = le_arg_GetArg(1);
            if (NULL == mode)
            {
                LE_ERROR("mode is NULL");
                exit(EXIT_FAILURE);
            }

            if ( strcmp(mode, "AMR") == 0 )
            {
                AudioFileFormat = LE_AUDIO_AMR;
                LE_INFO("         Use .amr audio files");
            }
        }
        else
        {
            LE_INFO("         Use .wav audio files");
        }

        // Connect audio stuff
        ConnectAudio();

        // Prepare call handling
        le_mcc_AddCallEventHandler(MyCallEventHandler, NULL);
        TestCallRef = le_mcc_Create(destinationNumber);

        // Start playing voice prompt once
        PlayFile(VOICE_PROMPT_START, false);
        IsVoicePromptStart = true;

        LE_INFO("======== voicePromptMcc2 Test started successfully ========");
    }
    else
    {
        PrintUsage();
        LE_INFO("EXIT voicePromptMcc2 Test");
        exit(EXIT_FAILURE);
    }
}

