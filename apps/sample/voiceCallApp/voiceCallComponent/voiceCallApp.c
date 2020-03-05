#include "legato.h"
#include "interfaces.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

//--------------------------------------------------------------------------------------------------
/**
* Call Reference.
*/
//--------------------------------------------------------------------------------------------------
static le_voicecall_CallRef_t myCallRef;

//--------------------------------------------------------------------------------------------------
/**
* handler voice call Reference.
*/
//--------------------------------------------------------------------------------------------------
static le_voicecall_StateHandlerRef_t VoiceCallHandlerRef;

//--------------------------------------------------------------------------------------------------
/**
* Destination Phone number.
*/
//--------------------------------------------------------------------------------------------------
static char DestinationNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];

//--------------------------------------------------------------------------------------------------
/**
* Voice call state flags.
*/
//--------------------------------------------------------------------------------------------------
static bool callFlag        = false; //set to true when dialing a number
static bool incomingFlag    = false; //set to true when there is an incoming call
static bool onHoldFlag      = false; //set to true when call is on hold
static bool redialPossible  = false; //set to true if there is at least 1 number available for redial
static bool callInProgress  = false; //set to true when there is an active call in progress

//--------------------------------------------------------------------------------------------------
/**
* The path specified on the command line.
*/
//--------------------------------------------------------------------------------------------------
static char Path[PATH_MAX] = "/";

//--------------------------------------------------------------------------------------------------
/**
* Audio safe references
*/
//--------------------------------------------------------------------------------------------------
static le_audio_StreamRef_t             MdmRxAudioRef;
static le_audio_StreamRef_t             MdmTxAudioRef;
static le_audio_StreamRef_t             FeInRef;
static le_audio_StreamRef_t             FeOutRef;
static le_audio_ConnectorRef_t          AudioInputConnectorRef;
static le_audio_ConnectorRef_t          AudioOutputConnectorRef;
static le_audio_MediaHandlerRef_t       MediaHandlerRef = NULL;
static le_audio_StreamRef_t             FileAudioRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
* Audio file path and descriptor
*/
//--------------------------------------------------------------------------------------------------
static const char                       AudioFilePathDefault[] = "/legato/systems/current/appsWriteable/voiceCallApp/piano.wav";
static char                             AudioFilePath[] = "/legato/systems/current/appsWriteable/voiceCallApp/piano.wav"; //Default audio file, can be changed via command line
static int                              AudioFileFd = -1;

//--------------------------------------------------------------------------------------------------
/**
* Handler function for Audio Stream Event Notifications.
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
        case LE_AUDIO_MEDIA_ENDED:
        LE_INFO("File event is LE_AUDIO_MEDIA_ENDED.");
        if(FileAudioRef)
        {
            if (le_audio_PlayFile(FileAudioRef, LE_AUDIO_NO_FD) != LE_OK)
            {
                LE_ERROR("Failed to play the file");
                return;
            }
            else
            {
                LE_INFO("file is now playing.");
            }
            break;
        }
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
* Close Audio Stream.
*
*/
//--------------------------------------------------------------------------------------------------
static void DisconnectAllAudio
(
    le_voicecall_CallRef_t reference
)
{
    LE_INFO("DisconnectAllAudio");

    if (AudioInputConnectorRef)
    {
        LE_INFO("Disconnect %p from connector.%p", FeInRef, AudioInputConnectorRef);
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
        LE_INFO("le_audio_Disconnect %p from connector.%p", MdmTxAudioRef, AudioOutputConnectorRef);
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
    }
    if(FeOutRef)
    {
        le_audio_Close(FeOutRef);
        FeOutRef = NULL;
    }
    if(FeInRef)
    {
        le_audio_Close(FeInRef);
        FeInRef = NULL;
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

    if(FileAudioRef)
    {
        le_audio_Close(FileAudioRef);
        FileAudioRef = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
* Open Audio Stream for microphone.
* This function is to be used when the we want to send voice through the mic.
* This function will still open the speaker and allow received audio stream to be heard through the speaker.
*
*/
//--------------------------------------------------------------------------------------------------
//! [setup audio path]
static le_result_t OpenAudioMic
(
    le_voicecall_CallRef_t reference
)
{
    le_result_t res;

    MdmRxAudioRef = le_voicecall_GetRxAudioStream(reference);
    LE_ERROR_IF((MdmRxAudioRef==NULL), "le_voicecall_GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_voicecall_GetTxAudioStream(reference);
    LE_ERROR_IF((MdmTxAudioRef==NULL), "le_voicecall_GetTxAudioStream returns NULL!");
    LE_DEBUG("OpenAudio MdmRxAudioRef %p, MdmTxAudioRef %p", MdmRxAudioRef, MdmTxAudioRef);
    LE_INFO("Connect to Mic and Speaker");

    // Redirect audio to the in-built Microphone and Speaker.
    FeOutRef = le_audio_OpenSpeaker();
    LE_ERROR_IF((FeOutRef==NULL), "le_audio_OpenSpeaker returns NULL!");
    FeInRef = le_audio_OpenMic();
    LE_ERROR_IF((FeInRef==NULL), "le_audio_OpenMic returns NULL!");
    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef==NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef==NULL), "AudioOutputConnectorRef is NULL!");

    if (MdmRxAudioRef && MdmTxAudioRef && FeOutRef && FeInRef && AudioInputConnectorRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FeInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect RX on Input connector!");
        res = le_audio_Connect(AudioInputConnectorRef, MdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect TX on Output connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, MdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }

    return LE_OK;
}
//! [setup audio path]
//--------------------------------------------------------------------------------------------------
/**
* Open Audio Stream for file playback.
* This function is to be used when a .wav file is to be played. Mic will not be opened.
* This function will still open the speaker and allow received audio stream to be heard through the speaker.
*
*/
//--------------------------------------------------------------------------------------------------
static le_result_t OpenAudioFile
(
    le_voicecall_CallRef_t reference
    )
{
    le_result_t res;

    MdmTxAudioRef = le_voicecall_GetTxAudioStream(reference);
    LE_ERROR_IF((MdmTxAudioRef==NULL), "le_voicecall_GetTxAudioStream returns NULL!");
    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef==NULL), "AudioInputConnectorRef is NULL!");
    FileAudioRef = le_audio_OpenPlayer();
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFilePlayback returns NULL!");

    MediaHandlerRef = le_audio_AddMediaHandler(FileAudioRef, MyMediaEventHandler, NULL);

    if (MdmTxAudioRef && FileAudioRef && AudioInputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, MdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect TX on Input connector!");
        res = le_audio_Connect(AudioInputConnectorRef, FileAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect FilePlayback on input connector!");

        if ((AudioFileFd=open(AudioFilePath, O_RDONLY)) == -1)
        {
            LE_ERROR("Open file %s failure: errno.%d (%s)",
                     AudioFilePath, errno, LE_ERRNO_TXT(errno));
            DisconnectAllAudio(reference);
            return LE_FAULT;
        }
        else
        {
            LE_INFO("Open file %s with AudioFileFd.%d",  AudioFilePath, AudioFileFd);
        }

        res = le_audio_PlayFile(FileAudioRef, AudioFileFd);
        LE_ERROR_IF((res!=LE_OK), "Failed to play the file!");
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
* Handler function for Call Event Notifications.
*
*/
//--------------------------------------------------------------------------------------------------
static void MyCallEventHandler
(
    le_voicecall_CallRef_t reference,
    const char* identifier,
    le_voicecall_Event_t callEvent,
    void* contextPtr
    )
{
    le_voicecall_TerminationReason_t term = LE_VOICECALL_TERM_UNDEFINED;

    LE_INFO("New Call event: %d for Call %p, from %s", callEvent, reference, identifier);

    if (callEvent == LE_VOICECALL_EVENT_ALERTING)
    {
        LE_INFO("LE_VOICECALL_EVENT_ALERTING");
        LE_INFO("Destination phone is ringing...");
    }

    else if (callEvent == LE_VOICECALL_EVENT_CONNECTED)
    {
        incomingFlag = false;
        callInProgress = true;
        OpenAudioMic(reference);
        LE_INFO("LE_VOICECALL_EVENT_CONNECTED");
        LE_INFO("You are now connected to %s", DestinationNumber);

    }
    else if (callEvent == LE_VOICECALL_EVENT_TERMINATED)
    {
        callFlag        = false;
        incomingFlag    = false;
        onHoldFlag      = false;
        callInProgress  = false;
        DisconnectAllAudio(reference);
        LE_INFO("LE_VOICECALL_EVENT_TERMINATED");
        le_voicecall_GetTerminationReason(reference, &term);
        switch(term)
        {
            case LE_VOICECALL_TERM_NETWORK_FAIL:
            {
                LE_ERROR("LE_VOICECALL_TERM_NETWORK_FAIL");
            }
            break;

            case LE_VOICECALL_TERM_BAD_ADDRESS:
            {
                LE_ERROR("LE_VOICECALL_TERM_BAD_ADDRESS");
            }
            break;

            case LE_VOICECALL_TERM_BUSY:
            {
                LE_ERROR("LE_VOICECALL_TERM_BUSY");
            }
            break;

            case LE_VOICECALL_TERM_LOCAL_ENDED:
            {
                LE_INFO("LE_VOICECALL_TERM_LOCAL_ENDED");
            }
            break;

            case LE_VOICECALL_TERM_REMOTE_ENDED:
            {
                LE_INFO("LE_VOICECALL_TERM_REMOTE_ENDED");
            }
            break;

            case LE_VOICECALL_TERM_UNDEFINED:
            {
                LE_INFO("LE_VOICECALL_TERM_UNDEFINED");
            }
            break;

            default:
            {
                LE_ERROR("Termination reason is %d", term);
            }
            break;
        }

        le_voicecall_Delete(reference);
    }
    else if (callEvent == LE_VOICECALL_EVENT_INCOMING)
    {
        LE_INFO("LE_VOICECALL_EVENT_INCOMING");
        incomingFlag = true;
        myCallRef = reference;
    }
    else if (callEvent == LE_VOICECALL_EVENT_CALL_END_FAILED)
    {
        LE_INFO("LE_VOICECALL_EVENT_CALL_END_FAILED");
    }
    else if (callEvent == LE_VOICECALL_EVENT_CALL_ANSWER_FAILED)
    {
        LE_INFO("LE_VOICECALL_EVENT_CALL_ANSWER_FAILED");
    }
    else if (callEvent == LE_VOICECALL_EVENT_OFFLINE)
    {
        LE_INFO("LE_VOICECALL_EVENT_OFFLINE");
    }
    else if (callEvent == LE_VOICECALL_EVENT_BUSY)
    {
        LE_INFO("LE_VOICECALL_EVENT_BUSY");
    }
    else if (callEvent == LE_VOICECALL_EVENT_RESOURCE_BUSY)
    {
        LE_INFO("LE_VOICECALL_EVENT_RESOURCE_BUSY");
    }
    else
    {
        LE_ERROR("MyCallEventHandler failed, unknowm event %d.", callEvent);
    }
}

//--------------------------------------------------------------------------------------------------
/**
* Create and start a voice call.
*
* @return
*      - LE_OK             If initiate call is successful.
*      - LE_FAULT          If initiate call cannot be proccessed.
*/
//--------------------------------------------------------------------------------------------------
static le_result_t voicecall_start
(
    void
)
{
    le_result_t  res = LE_FAULT;
    le_voicecall_TerminationReason_t reason = LE_VOICECALL_TERM_UNDEFINED;

    //! [Starting a voicecall]
    myCallRef = le_voicecall_Start(DestinationNumber);
    if (!myCallRef)
    {
        res = le_voicecall_GetTerminationReason(myCallRef, &reason);
        LE_ASSERT(res == LE_OK);
        LE_INFO("Termination reason is: %d", reason);
        return LE_FAULT;
    }
    //! [Starting a voicecall]
    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
* Check whether the user provided number is valid!
*
* @return
*      - LE_OK             If destination number is valid.
*      - LE_FAULT          If destination number is not valid.
*/
//-------------------------------------------------------------------------------------------------
static le_result_t isNumValid
(
    const char* phoneNumber
)
{
    if (NULL == phoneNumber)
    {
        LE_ERROR("Phone number NULL");
        return LE_FAULT;
    }
    int i = 0;
    int numLength = strlen(phoneNumber);
    if (numLength+1 > LE_MDMDEFS_PHONE_NUM_MAX_BYTES)
    {
        LE_INFO("The number is too long!");
        return LE_FAULT;
    }
    for (i = 0; i <= numLength-1; i++)
    {
        char dig = *phoneNumber;
        if(!isdigit(dig))
        {
            LE_INFO("The input contains non-digit symbol %c", dig);
            return LE_FAULT;
        }
        phoneNumber++;
    }
    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
* Get the phone number specified on the command line and start voice call to the destination number.
*
* @return
*      - LE_OK             If voice call can be successfully made to the destination.
*      - LE_BUSY           If there is already an active voice call.
*      - LE_FAULT          If not able to initiate voice call.
*/
//-------------------------------------------------------------------------------------------------
le_result_t ctrlVC_MakeCall
(
    const char * argPtr
)
{
    if (callFlag)
    {
        LE_INFO("Cannot make voice call while there is already an active voice call. Please hang up and try again.");
        return LE_BUSY;
    }
    else
    {
        const char* phoneNumber = argPtr;
        if(isNumValid(phoneNumber) != LE_OK)
        {
            LE_INFO("Phone number is not valid!");
            return LE_FAULT;
        }
        else
        {
            callFlag = true;
            redialPossible = true;
            le_utf8_Copy(DestinationNumber, phoneNumber, sizeof(DestinationNumber), NULL);
            LE_INFO("Phone number %s is valid.", DestinationNumber);
            LE_ASSERT(voicecall_start() == LE_OK);
        }
    }
    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
* This function changes the path to the .wav file to the specified path on the command line.
* If user specifies "default", path is reset to the default piano.wav included with the app.
*
* @return
*      - LE_OK     If wav file exists and path to audio file is successfully changed.
*      - LE_FAULT  If path was not changed.
*/
//-------------------------------------------------------------------------------------------------
le_result_t ctrlVC_SetWav
(
    const char * argPtr,
    bool setAudioToDefaultFlag
)
{
    if (onHoldFlag)
    {
        LE_INFO("Cannot change audio file while it is being played. Please unhold the call and try again.");
        return LE_FAULT;
    }
    else if (setAudioToDefaultFlag)
    {
        memset(AudioFilePath, '\0', sizeof(AudioFilePath));
        le_utf8_Copy(AudioFilePath, AudioFilePathDefault, sizeof(AudioFilePath), NULL);
        LE_INFO("Audio file has been reset to default!");
        return LE_OK;
    }
    else
    {
        Path[0] = '/';

        if (le_path_Concat("/", Path, sizeof(Path), argPtr, NULL) != LE_OK)
        {
            fprintf(stderr, "Path is too long.\n");
            return LE_FAULT;
        }
        if(access(Path, F_OK) != -1)
        {
            memset(AudioFilePath, '\0', sizeof(AudioFilePath));
            le_utf8_Copy(AudioFilePath, Path, sizeof(AudioFilePath), NULL);
            memset(Path, 0, sizeof(Path));
            LE_INFO("Path to audio file changed to %s", AudioFilePath);
        }
        else
        {
            LE_INFO("File does not exist!");
            memset(Path, 0, sizeof(Path));
            return LE_FAULT;
        }
    }
    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
* Call the last dialed number.
*
* @return
*      - LE_OK             If redial is processed successfully.
*      - LE_FAULT          If not able to perform redial.
*/
//-------------------------------------------------------------------------------------------------
le_result_t ctrlVC_Redial
(
    void
    )
{
    le_result_t res = LE_OK;
    if (callFlag)
    {
        LE_INFO("Call in progress. Please hangup and try redialing again.");
        return LE_FAULT;
    }
    else if (redialPossible)
    {
        LE_INFO("Redialing %s", DestinationNumber);
        res = voicecall_start();
        callFlag = true;
    }
    else
    {
        LE_INFO("No number is available. Please make at least one call before redialing.");
        return LE_FAULT;
    }
    return res;
}

//-------------------------------------------------------------------------------------------------
/**
* End any active voice call. Also ends incoming or outgoing calls which are not yet connected.
*
* @return
*      - LE_OK         If call is successfully ended.
*      - LE_NOT_FOUND  If the voice call object reference is not found.
*      - LE_FAULT      If end call cannot be processed.
*/
//-------------------------------------------------------------------------------------------------
le_result_t ctrlVC_HangupCall
(
    void
    )
{
    le_result_t res = LE_OK;
    if (!callFlag && !callInProgress && !incomingFlag)
    {
        LE_INFO("There is no voice call to end. You may hangup if a number is being dialed, there is an incoming call, or an active call is in progress.");
        return LE_FAULT;
    }
    //Due to issue in LE-12130 we cannot directly end an incoming call to reject it
    //Therefore this is a work-around to be able to reject incoming calls in the meantime
    //We first answer the call and immediately end it
    else if (incomingFlag)
    {
        callFlag = false;
        onHoldFlag = false;
        incomingFlag = false;
        LE_INFO("Rejecting the incoming call!");
        le_voicecall_Answer(myCallRef);
        res = le_voicecall_End(myCallRef);
        if (res != LE_OK)
        {
            LE_INFO("Failed to end call.");
        }
    }
    else
    {
        callFlag = false;
        onHoldFlag = false;
        LE_INFO("Hanging up all calls!");
        //! [Ending a voicecall]
        res = le_voicecall_End(myCallRef);
        if (res != LE_OK)
        {
            LE_INFO("Failed to end call.");
        }
        //! [Ending a voicecall]
    }
    return res;
}

//-------------------------------------------------------------------------------------------------
/**
* Answer an incoming call.
*
* @return
*      - LE_OK             If incoming call is successfully connected.
*      - LE_NOT_FOUND      Incoming voice call object reference is not found.
*/
//-------------------------------------------------------------------------------------------------
le_result_t ctrlVC_AnswerCall
(
    void
)
{
    le_result_t res = LE_OK;
    //! [answer]
    res = le_voicecall_Answer(myCallRef);
    if (res == LE_OK)
    {
        LE_INFO("Incoming call has been answered, you may now talk with %s", DestinationNumber);
    }
    //! [answer]
    else
    {
        LE_ERROR("No incoming call!");
    }
    return res;
}

//-------------------------------------------------------------------------------------------------
/**
* Place the active call on hold by disabling all audio input from mic and connecting input stream
* to the file pointed to by AudioFilePath
*
* @return
*      - LE_OK             If hold is processed successfully.
*      - LE_FAULT          If hold is not possible.
*/
//-------------------------------------------------------------------------------------------------
le_result_t ctrlVC_HoldCall
(
    void
    )
{
    le_result_t res = LE_OK;
    if (!callInProgress)
    {
        LE_INFO("There is no active voice call to place on hold.");
        return LE_FAULT;
    }
    else if (onHoldFlag)
    {
        LE_INFO("Call is already on hold. To unhold, type unhold instead.");
        return LE_FAULT;
    }
    else
    {
        onHoldFlag = true;
        LE_INFO("Placing call on hold!");
        DisconnectAllAudio(myCallRef);
        res = OpenAudioFile(myCallRef);
    }
    return res;
}

//-------------------------------------------------------------------------------------------------
/**
* Unhold the active call which is currently on hold by disconnecting the .wav file from the
input stream and connecting the mic to the input stream.
*
* @return
*      - LE_OK             If unhold is processed successfully.
*      - LE_FAULT          If unhold is not possible.
*/
//-------------------------------------------------------------------------------------------------
le_result_t ctrlVC_UnholdCall
(
    void
    )
{
    le_result_t res = LE_OK;
    if (!callInProgress)
    {
        LE_INFO("There is no active voice call to unhold.");
        return LE_FAULT;
    }
    else if (!onHoldFlag)
    {
        LE_INFO("Call is not on hold. Place it on hold by typing hold instead.");
        return LE_FAULT;
    }
    else
    {
        onHoldFlag = false;
        LE_INFO("Taking call off hold!");
        DisconnectAllAudio(myCallRef);
        res = OpenAudioMic(myCallRef);
    }
    return res;
}

//--------------------------------------------------------------------------------------------------
/*
* Must be registered on Network with the SIM in ready state.
* Start voiceCallService: app start voiceCallService
*
*/
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("voiceCallApp started!");
    LE_INFO("Make sure voiceCallService is running and SIM is registered on Network and is in ready state.");
    //! [AddStateHandler]
    VoiceCallHandlerRef = le_voicecall_AddStateHandler(MyCallEventHandler, NULL);
    //! [AddStateHandler]
}
