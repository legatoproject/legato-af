 /**
  * This module implements the le_ecall's test with a local voice prompt.
  *
 * You must issue the following commands:
 * @verbatim
   $ app start eCallWPrompt
   $ app runProc eCallWPrompt --exe=eCallWPrompt -- <PSAP number>
   @endverbatim

  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include "interfaces.h"

static const char *                     PsapNumber = NULL;
static le_ecall_CallRef_t               LastTestECallRef = NULL;
static le_audio_StreamRef_t             FeOutRef = NULL;
static le_audio_StreamRef_t             FileAudioRef = NULL;
static le_audio_ConnectorRef_t          AudioOutputConnectorRef = NULL;
static le_audio_MediaHandlerRef_t       MediaHandlerRef = NULL;
static const char                       AudioFilePath[] = "/male.wav";
static int                              AudioFileFd = -1;

//--------------------------------------------------------------------------------------------------
/**
 * Function to disconnect audio streams.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DisconnectAudio
(
    void
)
{
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

    if(AudioOutputConnectorRef)
    {
        le_audio_DeleteConnector(AudioOutputConnectorRef);
        AudioOutputConnectorRef = NULL;
    }

    if(FileAudioRef)
    {
        le_audio_Close(FileAudioRef);
        FileAudioRef = NULL;
    }

    if(FeOutRef)
    {
        le_audio_Close(FeOutRef);
        FeOutRef = NULL;
    }

    // Closing AudioFileFd is unnecessary since the messaging infrastructure underneath
    // le_audio_xxx APIs that use it would close it.
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Audio Stream Event Notifications.
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
 * Function to connect audio streams.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudio
(
    void
)
{
    // Open and connect an output stream.
    // Redirect audio to the in-built Speaker.
    FeOutRef = le_audio_OpenSpeaker();
    if (FeOutRef == NULL)
    {
        LE_ERROR("OpenSpeaker returns NULL!");
        LE_INFO("Switching to I2S interface...");
        // Redirect audio to the I2S interface.
        FeOutRef = le_audio_OpenI2sTx(LE_AUDIO_I2S_STEREO);
        if (FeOutRef==NULL)
        {
            LE_ERROR("OpenI2sTx returns NULL!");
        }
        else
        {
            LE_INFO("Open I2S: FeOutRef.%p", FeOutRef);
        }
    }
    else
    {
        LE_INFO("Open Speaker: FeOutRef.%p", FeOutRef);
    }

    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef==NULL), "AudioOutputConnectorRef is NULL!");
    {
        LE_ERROR_IF((le_audio_Connect(AudioOutputConnectorRef, FeOutRef) != LE_OK),
                    "Failed to connect I2S TX on Output connector!");
    }

    FileAudioRef = le_audio_OpenPlayer();
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFilePlayback returns NULL!");

    MediaHandlerRef = le_audio_AddMediaHandler(FileAudioRef,
                                                MyMediaEventHandler,
                                                NULL);

    if(le_audio_Connect(AudioOutputConnectorRef, FileAudioRef) != LE_OK)
    {
        LE_ERROR("Failed to connect FilePlayback on output connector!");
        return;
    }

    if ((AudioFileFd=open(AudioFilePath, O_RDONLY)) == -1)
    {
        LE_ERROR("Open file %s failure: errno.%d (%s)",  AudioFilePath, errno, strerror(errno));
        DisconnectAudio();
        exit(0);
    }
    else
    {
        LE_INFO("Open file %s with AudioFileFd.%d",  AudioFilePath, AudioFileFd);
    }

    if (le_audio_PlayFile(FileAudioRef, AudioFileFd) != LE_OK)
    {
        LE_ERROR("Failed to play the file");
        return;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for eCall state Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyECallEventHandler
(
    le_ecall_CallRef_t  eCallRef,
    le_ecall_State_t    state,
    void*               contextPtr
)
{
    LE_INFO("eCall TEST: New eCall state: %d for eCall ref.%p", state, eCallRef);

    switch(state)
    {
        case LE_ECALL_STATE_STARTED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_STARTED.");
            break;
        }
        case LE_ECALL_STATE_CONNECTED:
        {
            LE_INFO("Mute audio interface and voice prompt.");
            le_audio_Mute(FeOutRef);
            le_audio_Mute(FileAudioRef);
            LE_INFO("eCall state is LE_ECALL_STATE_CONNECTED.");
            break;
        }
        case LE_ECALL_STATE_DISCONNECTED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_DISCONNECTED.");
            break;
        }
        case LE_ECALL_STATE_WAITING_PSAP_START_IND:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_WAITING_PSAP_START_IND.");
            break;
        }
        case LE_ECALL_STATE_PSAP_START_IND_RECEIVED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_PSAP_START_IND_RECEIVED.");
            if (le_ecall_SendMsd(eCallRef) != LE_OK)
            {
                LE_ERROR("Could not send the MSD");
            }

            break;
        }
        case LE_ECALL_STATE_MSD_TX_STARTED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_MSD_TX_STARTED.");
            break;
        }
        case LE_ECALL_STATE_LLNACK_RECEIVED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_LLNACK_RECEIVED.");
            break;
        }
        case LE_ECALL_STATE_LLACK_RECEIVED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_LLACK_RECEIVED.");
            break;
        }
        case LE_ECALL_STATE_MSD_TX_COMPLETED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_MSD_TX_COMPLETED.");
            break;
        }
        case LE_ECALL_STATE_MSD_TX_FAILED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_MSD_TX_FAILED.");
            break;
        }
        case LE_ECALL_STATE_ALACK_RECEIVED_POSITIVE:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_ALACK_RECEIVED_POSITIVE.");
            break;
        }
        case LE_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN.");
            break;
        }
        case LE_ECALL_STATE_STOPPED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_STOPPED.");
            LE_INFO("Unmute audio interface and voice prompt.");
            le_audio_Unmute(FeOutRef);
            le_audio_Unmute(FileAudioRef);
            break;
        }
        case LE_ECALL_STATE_RESET:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_RESET.");
            LE_INFO("Unmute audio interface and voice prompt.");
            le_audio_Unmute(FeOutRef);
            le_audio_Unmute(FileAudioRef);
            break;
        }
        case LE_ECALL_STATE_COMPLETED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_COMPLETED.");
            break;
        }
        case LE_ECALL_STATE_FAILED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_FAILED.");
            LE_INFO("Unmute audio interface and voice prompt.");
            le_audio_Unmute(FeOutRef);
            le_audio_Unmute(FileAudioRef);
            break;
        }
        case LE_ECALL_STATE_END_OF_REDIAL_PERIOD:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_END_OF_REDIAL_PERIOD.");
            break;
        }
        case LE_ECALL_STATE_UNKNOWN:
        default:
        {
            LE_INFO("Unknown eCall state.");
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Create and start a test eCall.
 *
 */
//--------------------------------------------------------------------------------------------------
static void StartTestECall
(
    void
)
{
    le_ecall_State_t                   state = LE_ECALL_STATE_UNKNOWN;
    le_ecall_StateChangeHandlerRef_t   stateChangeHandlerRef = 0x00;

    LE_INFO("Start StartTestECall");

    LE_ASSERT((stateChangeHandlerRef = le_ecall_AddStateChangeHandler(MyECallEventHandler, NULL)) != NULL);

    LE_ASSERT(le_ecall_SetPsapNumber(PsapNumber) == LE_OK);

    LE_ASSERT(le_ecall_SetMsdTxMode(LE_ECALL_TX_MODE_PUSH) == LE_OK);

    LE_ASSERT((LastTestECallRef=le_ecall_Create()) != NULL);

    LE_ASSERT(le_ecall_SetMsdPosition(LastTestECallRef, true, +48898064, +2218092, 0) == LE_OK);

    LE_ASSERT(le_ecall_SetMsdPositionN1(LastTestECallRef, 0, 11) == LE_OK);

    LE_ASSERT(le_ecall_SetMsdPositionN2(LastTestECallRef, -22, -33) == LE_OK);

    LE_ASSERT(le_ecall_SetMsdPassengersCount(LastTestECallRef, 3) == LE_OK);

    ConnectAudio();

    LE_ASSERT(le_ecall_StartTest(LastTestECallRef) == LE_OK);

    state=le_ecall_GetState(LastTestECallRef);
    LE_ASSERT(((state>=LE_ECALL_STATE_STARTED) && (state<=LE_ECALL_STATE_FAILED)));
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
    LE_INFO("End and delete last test eCall");
    le_ecall_End(LastTestECallRef);
    le_ecall_Delete(LastTestECallRef);
    DisconnectAudio();
    exit(EXIT_SUCCESS);
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
            "Usage of the eCallWPrompt is:",
            "   eCallWPrompt <PSAP number>",
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
    if (le_arg_NumArgs() == 1)
    {
        // Register a signal event handler for SIGINT when user interrupts/terminates process
        signal(SIGINT, SigHandler);

        PsapNumber = le_arg_GetArg(0);
        LE_INFO("======== Start eCallWPrompt Test with PSAP.%s========",
                PsapNumber);

        StartTestECall();
        LE_INFO("======== eCallWPrompt Test SUCCESS ========");
    }
    else
    {
        PrintUsage();
        LE_INFO("EXIT eCallWPrompt");
        exit(EXIT_FAILURE);
    }
}
