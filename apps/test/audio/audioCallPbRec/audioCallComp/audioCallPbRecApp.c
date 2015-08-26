//--------------------------------------------------------------------------------------------------
/**
 * @file audioCallPbRecApp.c
 *
 * This file contains the source code of the audioCallPbRecApp app.
 *
 * This app take as an argument the destination phone number to call. To set the destination
 * number, you must issue the folllowing command:
 * config set /apps/audioCallPbRecApp/procs/audioCallPbRecApp/args/1 <destination number>
 *
 * Once you have started the app with 'app start audioCallPbRecApp', the app will automatically
 * calls the destination number. When the remote party answers the call, the app starts the audio
 * file playback. The remote party will be able to hear the audio file.
 *
 * After the initial outgoing call, the app simply waits for an incoming call. You can now call
 * your target. The target will automatically answer and start the recording of the remote end
 * (actually your voice!).
 * The resulted audio file can be found here:
 * /tmp/legato/sandboxes/audioCallPbRecApp/record/remote.wav
 *
 * Note that this app uses the I2S interface.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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

//--------------------------------------------------------------------------------------------------
/**
 * Destination number for outgoing calls
 */
//--------------------------------------------------------------------------------------------------
static const char* DestinationNumber;

//--------------------------------------------------------------------------------------------------
/**
 * Audio file path used for audio playback
 */
//--------------------------------------------------------------------------------------------------
static const char  AudioFilePbPath[] = "/usr/share/sounds/male.wav";

//--------------------------------------------------------------------------------------------------
/**
 * Audio file path used for audio recording
 */
//--------------------------------------------------------------------------------------------------
static const char  AudioFileRecPath[] = "/record/remote.wav";

//--------------------------------------------------------------------------------------------------
/**
 * Audio file descriptor
 */
//--------------------------------------------------------------------------------------------------
static int  AudioFileFd = -1;

//--------------------------------------------------------------------------------------------------
/**
 * Flag specifying the call direction
 */
//--------------------------------------------------------------------------------------------------
static bool IsAnOutgoingCall = false;

//--------------------------------------------------------------------------------------------------
/**
 * Connect the audio for audio file recording of the remote end.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToFileRemoteRec
(
    void
)
{
    le_result_t res;
    if ((AudioFileFd=open(AudioFileRecPath, O_WRONLY | O_CREAT | O_TRUNC)) == -1)
    {
        LE_ERROR("Open file %s failure: errno.%d (%s)",  AudioFileRecPath, errno, strerror(errno));
    }
    else
    {
        LE_INFO("Open file %s with AudioFileFd.%d",  AudioFileRecPath, AudioFileFd);
    }

    if (fchmod(AudioFileFd, 0666) == -1)
    {
        LE_WARN("Cannot set rights to file %s: errno.%d (%s)",  AudioFileRecPath, errno, strerror(errno));
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
 * Connect the audio for audio file playback on remote party.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToFileRemotePlay
(
    void
)
{
    le_result_t res;

    if ((AudioFileFd=open(AudioFilePbPath, O_RDONLY)) == -1)
    {
        LE_ERROR("Open file %s failure: errno.%d (%s)",  AudioFilePbPath, errno, strerror(errno));
    }
    else
    {
        LE_INFO("Open file %s with AudioFileFd.%d",  AudioFilePbPath, AudioFileFd);
    }

    // Play Remote on input connector.
    FileAudioRef = le_audio_OpenPlayer();
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFilePlayback returns NULL!");

    if (FileAudioRef && AudioInputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FileAudioRef);
        if(res!=LE_OK)
        {
            LE_ERROR("Failed to connect FilePlayback on input connector!");
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
 * Connect the main Audio path (I2S).
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToI2s
(
    void
)
{
    le_result_t res;

    MdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((MdmRxAudioRef==NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_audio_OpenModemVoiceTx();
    LE_ERROR_IF((MdmTxAudioRef==NULL), "GetTxAudioStream returns NULL!");

    // Redirect audio to the I2S interface.
    FeOutRef = le_audio_OpenI2sTx(LE_AUDIO_I2S_STEREO);
    LE_ERROR_IF((FeOutRef==NULL), "OpenI2sTx returns NULL!");
    FeInRef = le_audio_OpenI2sRx(LE_AUDIO_I2S_STEREO);
    LE_ERROR_IF((FeInRef==NULL), "OpenI2sRx returns NULL!");

    LE_INFO("Open I2s: FeInRef.%p FeOutRef.%p", FeInRef, FeOutRef);

    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef==NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef==NULL), "AudioOutputConnectorRef is NULL!");

    if (MdmRxAudioRef && MdmTxAudioRef && FeOutRef && FeInRef &&
        AudioInputConnectorRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FeInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect I2S RX on Input connector!");
        res = le_audio_Connect(AudioInputConnectorRef, MdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect I2S TX on Output connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, MdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
    LE_INFO("Open I2s: FeInRef.%p FeOutRef.%p", FeInRef, FeOutRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect the main Audio path and the audio file playback.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioWithPlayback
(
    void
)
{
    LE_INFO("Connect I2S");
    ConnectAudioToI2s();

    LE_INFO("Connect Remote Play");
    ConnectAudioToFileRemotePlay();
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect the main Audio path and the audio file recording.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioWithRecording
(
    void
)
{
    LE_INFO("Connect I2S");
    ConnectAudioToI2s();

    LE_INFO("Connect Remote Rec");
    ConnectAudioToFileRemoteRec();
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

    close(AudioFileFd);
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
        IsAnOutgoingCall =true;
    }
    else if (callEvent == LE_MCC_EVENT_CONNECTED)
    {
        LE_INFO("Call event is LE_MCC_EVENT_CONNECTED.");
        if (IsAnOutgoingCall)
        {
            ConnectAudioWithPlayback();
        }
        else
        {
            ConnectAudioWithRecording();
        }
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

        DisconnectAllAudio();

        le_mcc_Delete(callRef);

        IsAnOutgoingCall = false;
    }
    else if (callEvent == LE_MCC_EVENT_INCOMING)
    {
        LE_INFO("Call event is LE_MCC_EVENT_INCOMING.");
        le_mcc_Answer(callRef);
    }
    else
    {
        LE_ERROR("Unknowm Call event.");
        close(AudioFileFd);
        exit(EXIT_FAILURE);
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
            "   execInApp audioCallPbRecApp audioCallPbRecApp <tel number>",
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
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Start audioCallPbRecApp app.");
    IsAnOutgoingCall = false;

    if (le_arg_NumArgs() == 1)
    {
        DestinationNumber = le_arg_GetArg(0);

        le_mcc_AddCallEventHandler(MyCallEventHandler, NULL);
        CallRef=le_mcc_Create(DestinationNumber);
        le_mcc_Start(CallRef);
    }
    else
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }
}
