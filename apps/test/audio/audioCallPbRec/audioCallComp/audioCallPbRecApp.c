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
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
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
static le_mcc_call_ObjRef_t    CallRef = NULL;
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
static char  DestinationNumber[LE_MDMDEFS_PHONE_NUM_MAX_LEN];

//--------------------------------------------------------------------------------------------------
/**
 * Audio file path used for audio playback
 */
//--------------------------------------------------------------------------------------------------
static const char  AudioFilePbPath[] = "/opt/legato/apps/audioCallPbRecApp/usr/share/sounds/male.wav";
// static const char  AudioFilePbPath[] = "/usr/share/sounds/male.wav";

//--------------------------------------------------------------------------------------------------
/**
 * Audio file path used for audio recording
 */
//--------------------------------------------------------------------------------------------------
// static const char  AudioFileRecPath[] = "/record/remote.wav";
static const char  AudioFileRecPath[] = "/opt/legato/apps/audioCallPbRecApp/record/remote.wav";

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
    FileAudioRef = le_audio_OpenFileRecording(AudioFileFd);
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFileRecording returns NULL!");

    if (FileAudioRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioOutputConnectorRef, FileAudioRef);
        if(res!=LE_OK)
        {
            LE_ERROR("Failed to connect FileRecording on output connector!");
        }
        else
        {
            LE_INFO("FileRecording is now connected.");
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
    FileAudioRef = le_audio_OpenFilePlayback(AudioFileFd);
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFilePlayback returns NULL!");

    if (FileAudioRef && AudioInputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FileAudioRef);
        if(res!=LE_OK)
        {
            LE_ERROR("Failed to connect FilePlayback on input connector!");
        }
        else
        {
            LE_INFO("FilePlayback is now connected.");
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
    le_mcc_call_ObjRef_t callRef
)
{
    le_result_t res;

    MdmRxAudioRef = le_mcc_call_GetRxAudioStream(callRef);
    LE_ERROR_IF((MdmRxAudioRef==NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_mcc_call_GetTxAudioStream(callRef);
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
    le_mcc_call_ObjRef_t callRef
)
{
    LE_INFO("Connect I2S");
    ConnectAudioToI2s(callRef);

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
    le_mcc_call_ObjRef_t callRef
)
{
    LE_INFO("Connect I2S");
    ConnectAudioToI2s(callRef);

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
    le_mcc_call_ObjRef_t callRef
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
    le_mcc_call_ObjRef_t   callRef,
    le_mcc_call_Event_t callEvent,
    void*               contextPtr
)
{
    if (callEvent == LE_MCC_CALL_EVENT_ALERTING)
    {
        LE_INFO("Call event is LE_MCC_CALL_EVENT_ALERTING.");
        IsAnOutgoingCall =true;
    }
    else if (callEvent == LE_MCC_CALL_EVENT_CONNECTED)
    {
        LE_INFO("Call event is LE_MCC_CALL_EVENT_CONNECTED.");
        if (IsAnOutgoingCall)
        {
            ConnectAudioWithPlayback(callRef);
        }
        else
        {
            ConnectAudioWithRecording(callRef);
        }
    }
    else if (callEvent == LE_MCC_CALL_EVENT_TERMINATED)
    {
        LE_INFO("Call event is LE_MCC_CALL_EVENT_TERMINATED.");
        le_mcc_call_TerminationReason_t term = le_mcc_call_GetTerminationReason(callRef);
        switch(term)
        {
            case LE_MCC_CALL_TERM_NETWORK_FAIL:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_NETWORK_FAIL");
                break;

            case LE_MCC_CALL_TERM_BAD_ADDRESS:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_BAD_ADDRESS");
                break;

            case LE_MCC_CALL_TERM_BUSY:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_BUSY");
                break;

            case LE_MCC_CALL_TERM_LOCAL_ENDED:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_LOCAL_ENDED");
                break;

            case LE_MCC_CALL_TERM_REMOTE_ENDED:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_REMOTE_ENDED");
                break;

            case LE_MCC_CALL_TERM_NOT_DEFINED:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_NOT_DEFINED");
                break;

            default:
                LE_INFO("Termination reason is %d", term);
                break;
        }

        DisconnectAllAudio(callRef);

        le_mcc_call_Delete(callRef);

        IsAnOutgoingCall = false;
    }
    else if (callEvent == LE_MCC_CALL_EVENT_INCOMING)
    {
        LE_INFO("Call event is LE_MCC_CALL_EVENT_INCOMING.");
        le_mcc_call_Answer(callRef);
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
 * Thread that install the Call Event handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void* HandlerInstallerThread(void* contextPtr)
{
    le_mcc_profile_ObjRef_t profileRef = contextPtr;

    le_mcc_profile_AddCallEventHandler(profileRef, MyCallEventHandler, NULL);

    le_event_RunLoop();
    return NULL;
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
            "   audioCallPbRecApp <tel number>",
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

    if (le_arg_NumArgs() == 1)
    {
        le_arg_GetArg(0, DestinationNumber, LE_MDMDEFS_PHONE_NUM_MAX_LEN);
    }
    else
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    le_mcc_profile_ObjRef_t profileRef=le_mcc_profile_GetByName("Modem-Sim1");
    if ( profileRef == NULL )
    {
        LE_INFO("Unable to get the Call profile reference");
        exit(EXIT_FAILURE);
    }

    // Start the handler thread to monitor the call for the just created profile.
    le_thread_Start(le_thread_Create("HandlerInstallerThread", HandlerInstallerThread, profileRef));

    IsAnOutgoingCall = false;
    CallRef=le_mcc_profile_CreateCall(profileRef, DestinationNumber);
    le_mcc_call_Start(CallRef);
}
