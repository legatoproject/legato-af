//--------------------------------------------------------------------------------------------------
/**
 * @file audioCallPbRecApp.c
 *
 * This file contains the source code of the audioCallPbRecApp app.
 *
 * This app take as an argument the destination phone number to call. To set the destination
 * number, you must issue the folllowing command:
 * @verbatim
    $ app start audioCallPbRecApp
    $ app runProc audioCallPbRecApp --exe=audioCallPbRecApp -- <tel number>
 *  @endverbatim
 *
 * Once you have started the app with 'app start audioCallPbRecApp', the app will automatically
 * calls the destination number. When the remote party answers the call, the app starts the audio
 * file recording.
 * When the call is disconnected, the recorded audio is played on local interface (Speaker).
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"



//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Objects references (call, audio stream, audio connector)
 */
//--------------------------------------------------------------------------------------------------
static le_mcc_CallRef_t        CallRef = NULL;
static le_audio_StreamRef_t    MdmRxAudioRef = NULL;
static le_audio_StreamRef_t    MdmTxAudioRef = NULL;
static le_audio_StreamRef_t    FeInRef = NULL;
static le_audio_StreamRef_t    FeOutRef = NULL;
static le_audio_StreamRef_t    FileAudioRef = NULL;
static le_audio_ConnectorRef_t AudioInputConnectorRef = NULL;
static le_audio_ConnectorRef_t AudioOutputConnectorRef = NULL;
static le_audio_MediaHandlerRef_t MediaHandlerRef = NULL;

static const char  AudioFileRecPath[] = "/record/remote.wav";

static int  AudioFileFd = -1;


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Media Event Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyMediaEventHandler
(
    le_audio_StreamRef_t          streamRef,
    le_audio_MediaEvent_t          event,
    void*                         contextPtr
)
{
    switch(event)
    {
        case LE_AUDIO_MEDIA_ENDED:
            LE_INFO("File event is LE_AUDIO_MEDIA_ENDED.");
            break;

        case LE_AUDIO_MEDIA_ERROR:
            LE_INFO("File event is LE_AUDIO_MEDIA_ERROR.");
            break;
        case LE_AUDIO_MEDIA_NO_MORE_SAMPLES:
            LE_INFO("File event is LE_AUDIO_MEDIA_NO_MORE_SAMPLES.");
            break;
        default:
            LE_INFO("File event is %d", event);
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable File local playback.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToFileLocalPlay
(
    void
)
{
    le_result_t res;
    if ((AudioFileFd=open(AudioFileRecPath, O_RDWR)) == -1)
    {
        LE_ERROR("Open file %s failure: errno.%d (%s)",  AudioFileRecPath, errno, strerror(errno));
    }
    else
    {
        LE_INFO("Open file %s with AudioFileFd.%d",  AudioFileRecPath, AudioFileFd);
    }

    // Play local on output connector.
    FileAudioRef = le_audio_OpenPlayer();
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFilePlayback returns NULL!");

    MediaHandlerRef = le_audio_AddMediaHandler(FileAudioRef, MyMediaEventHandler, NULL);
    LE_ERROR_IF((MediaHandlerRef==NULL), "AddMediaHandler returns NULL!");

    if (FileAudioRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioOutputConnectorRef, FileAudioRef);
        if(res!=LE_OK)
        {
            LE_ERROR("Failed to connect FilePlayback on output connector!");
            return;
        }

        LE_INFO("FilePlayback is now connected.");
        res = le_audio_PlayFile(FileAudioRef, AudioFileFd);

        if(res != LE_OK)
        {
            LE_ERROR("Failed to play the file!");
        }
        else
        {
            LE_INFO("File is now playing");
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect the audio for audio file recording of the local and remote end.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToFileRec
(
    void
)
{
    le_result_t res;
    if ((AudioFileFd=open(AudioFileRecPath, O_RDWR | O_CREAT | O_TRUNC)) == -1)
    {
        LE_ERROR("Open file %s failure: errno.%d (%s)",  AudioFileRecPath, errno, strerror(errno));
    }
    else
    {
        LE_INFO("Open file %s with AudioFileFd.%d",  AudioFileRecPath, AudioFileFd);
    }

    // Capture Remote on output connector.
    FileAudioRef = le_audio_OpenRecorder();
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFileRecording returns NULL!");

    if (FileAudioRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioOutputConnectorRef, FileAudioRef);
        if(res!=LE_OK)
        {
            LE_ERROR("Failed to connect FileRecording on output connector!");
            return;
        }
        res = le_audio_Connect(AudioInputConnectorRef, FileAudioRef);
        if(res!=LE_OK)
        {
            LE_ERROR("Failed to connect FileRecording on input connector!");
            return;
        }

        LE_INFO("Recorder is now connected.");
        res = le_audio_RecordFile(FileAudioRef, AudioFileFd);

        if(res!=LE_OK)
        {
            LE_ERROR("Failed to record the file");
        }
        else
        {
            LE_INFO("File is now recording.");
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect the main Audio path (Analog).
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToAnalog
(
    void
)
{
    le_result_t res;

    MdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((MdmRxAudioRef==NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_audio_OpenModemVoiceTx();
    LE_ERROR_IF((MdmTxAudioRef==NULL), "GetTxAudioStream returns NULL!");

    // Redirect audio to the in-built Microphone and Speaker.
    FeOutRef = le_audio_OpenSpeaker();
    LE_ERROR_IF((FeOutRef==NULL), "OpenSpeaker returns NULL!");
    FeInRef = le_audio_OpenMic();
    LE_ERROR_IF((FeInRef==NULL), "OpenMic returns NULL!");

    LE_INFO("Open Analog: FeInRef.%p FeOutRef.%p", FeInRef, FeOutRef);

    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef==NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef==NULL), "AudioOutputConnectorRef is NULL!");

    if (MdmRxAudioRef && MdmTxAudioRef && FeOutRef && FeInRef &&
        AudioInputConnectorRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FeInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect Mic on Input connector!");
        res = le_audio_Connect(AudioInputConnectorRef, MdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect Speaker on Output connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, MdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
    LE_INFO("Open Analog: FeInRef.%p FeOutRef.%p", FeInRef, FeOutRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disconnect all the opened audio connections.
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
        if(FileAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", FileAudioRef, AudioInputConnectorRef);
            le_audio_Disconnect(AudioInputConnectorRef, FileAudioRef);
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
        if(FileAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", FileAudioRef, AudioOutputConnectorRef);
            le_audio_Disconnect(AudioOutputConnectorRef, FileAudioRef);
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

    if(FileAudioRef)
    {
        le_audio_Close(FileAudioRef);
        FeOutRef = NULL;
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
        FeOutRef = NULL;
    }
    if(MdmTxAudioRef)
    {
        le_audio_Close(MdmTxAudioRef);
        FeOutRef = NULL;
    }

    if(MediaHandlerRef)
    {
        le_audio_RemoveMediaHandler(MediaHandlerRef);
        MediaHandlerRef = NULL;
    }

    // Closing AudioFileFd is unnecessary since the messaging infrastructure underneath
    // le_audio_xxx APIs that use it would close it.
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Call Event Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyCallEventHandler
(
    le_mcc_CallRef_t   callRef,
    le_mcc_Event_t callEvent,
    void*               contextPtr
)
{
    if (callEvent == LE_MCC_EVENT_ALERTING)
    {
        LE_INFO("Call event is LE_MCC_EVENT_ALERTING.");
    }
    else if (callEvent == LE_MCC_EVENT_CONNECTED)
    {
        LE_INFO("Call event is LE_MCC_EVENT_CONNECTED.");
        LE_INFO("Connect Remote Rec");
        ConnectAudioToFileRec();
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

        le_audio_Stop(FileAudioRef);
        le_audio_Disconnect(AudioOutputConnectorRef, FileAudioRef);
        le_audio_Disconnect(AudioInputConnectorRef, FileAudioRef);

        le_audio_Close(FileAudioRef);

        // 2-second pause: workaround to step over possible pcm_open error on AR8 platforms
        sleep(2);

        ConnectAudioToFileLocalPlay();

        le_mcc_Delete(callRef);
    }
    else if (callEvent == LE_MCC_EVENT_INCOMING)
    {
        LE_INFO("Call event is LE_MCC_EVENT_INCOMING.");
        le_mcc_Answer(callRef);
    }
    else
    {
        LE_INFO("Other Call event.%d", callEvent);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Help.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage()
{
    int idx;
    bool sandboxed = (getuid() != 0);
    const char * usagePtr[] = {
            "Usage of the 'audioCallPbRecApp' tool is:",
            "   app runProc audioCallPbRecApp --exe=audioCallPbRecApp -- <tel number>",
            "",
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
 * The signal event handler function for SIGINT when process dies.
 */
//--------------------------------------------------------------------------------------------------
static void SigHandler
(
    int sigNum
)
{
    LE_INFO("End audioCallPbRecApp test");
    DisconnectAllAudio();
    le_mcc_HangUpAll();

    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Start audioCallPbRecApp app.");

    // Register a signal event handler for SIGINT when user interrupts/terminates process
    signal(SIGINT, SigHandler);

    if (le_arg_NumArgs() == 1)
    {
        ConnectAudioToAnalog();

        const char* destinationNumber = le_arg_GetArg(0);

        le_mcc_AddCallEventHandler(MyCallEventHandler, NULL);
        CallRef=le_mcc_Create(destinationNumber);
        le_mcc_Start(CallRef);
    }
    else
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }
}
