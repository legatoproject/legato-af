 /**
  * This module implements the le_voicecall's integration tests.
  *
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Call Reference.
 */
//--------------------------------------------------------------------------------------------------
static le_voicecall_CallRef_t TestCallRef;


//--------------------------------------------------------------------------------------------------
/**
 * handler voice call Reference.
 */
//--------------------------------------------------------------------------------------------------
static le_voicecall_StateHandlerRef_t VoiceCallHandlerRef;


//--------------------------------------------------------------------------------------------------
/**
 * Hang Up Timer Reference.
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t HangUpTimer;


//--------------------------------------------------------------------------------------------------
/**
 * Destination Phone number.
 */
//--------------------------------------------------------------------------------------------------
static char  DestinationNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];


//--------------------------------------------------------------------------------------------------
/**
 * Audio safe references
 */
//--------------------------------------------------------------------------------------------------
static le_audio_StreamRef_t MdmRxAudioRef;
static le_audio_StreamRef_t MdmTxAudioRef;
static le_audio_StreamRef_t FeInRef;
static le_audio_StreamRef_t FeOutRef;

static le_audio_ConnectorRef_t AudioInputConnectorRef;
static le_audio_ConnectorRef_t AudioOutputConnectorRef;


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
}


//--------------------------------------------------------------------------------------------------
/**
 * Open Audio Stream.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t OpenAudio
(
    le_voicecall_CallRef_t reference
)
{
    le_result_t res;

    MdmRxAudioRef = le_voicecall_GetRxAudioStream(reference);
    LE_ERROR_IF((MdmRxAudioRef==NULL), "le_voicecall_GetRxAudioStream returns NULL!");

    MdmTxAudioRef = le_voicecall_GetTxAudioStream(reference);
    LE_ERROR_IF((MdmTxAudioRef==NULL), "le_voicecall_GetRxAudioStream returns NULL!");

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

    if (MdmRxAudioRef && MdmTxAudioRef && FeOutRef && FeInRef &&
                    AudioInputConnectorRef && AudioOutputConnectorRef)
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

//--------------------------------------------------------------------------------------------------
/**
 * HangUp Timer handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void HangUpTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    LE_INFO("Hanging up all calls!");
    LE_ERROR_IF(le_voicecall_End(TestCallRef) != LE_OK, "Could not Hang UP.");
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
    le_result_t  res;
    le_voicecall_TerminationReason_t term = LE_VOICECALL_TERM_UNDEFINED;
    static bool firstConnectCall = true;

    LE_INFO("Voice Call TEST: New Call event: %d for Call %p, from %s",
        callEvent, reference, identifier);

    if (callEvent == LE_VOICECALL_EVENT_ALERTING)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_VOICECALL_EVENT_ALERTING.");
        if (firstConnectCall)
        {
            LE_INFO("---!!!! PLEASE PICk UP ON THE REMOTE SIDE !!!!---");
        }
    }
    else if (callEvent == LE_VOICECALL_EVENT_CONNECTED)
    {
        OpenAudio(reference);
        LE_INFO("Check MyCallEventHandler passed, event is LE_VOICECALL_EVENT_CONNECTED.");
        if (firstConnectCall)
        {
            LE_INFO("---!!!! PLEASE TERMINATE THE CALL on THE REMOTE SIDE !!!!---");
            firstConnectCall = false;
        }
        else
        {
            LE_INFO("All calls will be hung-up in 10 seconds");
            LE_ASSERT(le_timer_Start(HangUpTimer) == LE_OK);
        }
    }
    else if (callEvent == LE_VOICECALL_EVENT_TERMINATED)
    {
        DisconnectAllAudio(reference);
        LE_INFO("Check MyCallEventHandler passed, event is LE_VOICECALL_EVENT_TERMINATED.");
        le_voicecall_GetTerminationReason(reference, &term);
        switch(term)
        {
            case LE_VOICECALL_TERM_NETWORK_FAIL:
            {
                LE_ERROR("Termination reason is LE_VOICECALL_TERM_NETWORK_FAIL");
            }
            break;

            case LE_VOICECALL_TERM_BUSY:
            {
                LE_ERROR("Termination reason is LE_VOICECALL_TERM_BUSY");
            }
            break;

            case LE_VOICECALL_TERM_LOCAL_ENDED:
            {
                LE_INFO("LE_VOICECALL_TERM_LOCAL_ENDED");
                le_voicecall_RemoveStateHandler(VoiceCallHandlerRef);
                if(firstConnectCall)
                {
                    LE_ERROR("voiceCallTest Sequence FAILED ");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    LE_INFO("voiceCallTest Sequence SUCCESS");
                    exit(EXIT_SUCCESS);
                }
            }
            break;

            case LE_VOICECALL_TERM_REMOTE_ENDED:
            {
                LE_INFO("Termination reason is LE_VOICECALL_TERM_REMOTE_ENDED");
                LE_INFO("---!!!! PLEASE CREATE AN INCOMING CALL !!!!---");
            }
            break;

            case LE_VOICECALL_TERM_UNDEFINED:
            {
                LE_INFO("Termination reason is LE_VOICECALL_TERM_UNDEFINED");
                LE_ERROR("---!!!! PLEASE CREATE AN INCOMING CALL !!!!---");
            }
            break;

            default:
            {
                LE_ERROR("Termination reason is %d", term);
            }
            break;
        }

        if (le_timer_IsRunning(HangUpTimer))
        {
            LE_INFO("STOP Timer");
            le_timer_Stop(HangUpTimer);
            HangUpTimer = NULL;
        }

        le_voicecall_Delete(reference);
    }
    else if (callEvent == LE_VOICECALL_EVENT_INCOMING)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_VOICECALL_EVENT_INCOMING.");
        res = le_voicecall_Answer(reference);
        if (res == LE_OK)
        {
            TestCallRef = reference;
            LE_INFO("Check MyCallEventHandler passed, I answered the call");
        }
        else
        {
            LE_ERROR("Check MyCallEventHandler failed to answer the call.");
        }
    }
    if (callEvent == LE_VOICECALL_EVENT_CALL_END_FAILED)
    {
        LE_INFO("Event is LE_VOICECALL_EVENT_CALL_END_FAILED.");
    }
    if (callEvent == LE_VOICECALL_EVENT_CALL_ANSWER_FAILED)
    {
        LE_INFO("Event is LE_VOICECALL_EVENT_CALL_ANSWER_FAILED.");
    }
    else
    {
        LE_ERROR("Check MyCallEventHandler failed, unknowm event %d.", callEvent);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: Create and start a voice call.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Testle_voicecall_start
(
    void
)
{
    le_result_t  res = LE_FAULT;
    le_voicecall_TerminationReason_t reason = LE_VOICECALL_TERM_UNDEFINED;

    TestCallRef = le_voicecall_Start(DestinationNumber);
    if (!TestCallRef)
    {
       res = le_voicecall_GetTerminationReason(TestCallRef, &reason);
       LE_ASSERT(res == LE_OK);
       LE_INFO("le_voicecall_GetTerminationReason %d", reason);
       return LE_FAULT;
    }

    return LE_OK;
}



//--------------------------------------------------------------------------------------------------
/*
 * ME must be registered on Network with the SIM in ready state.
 * Check "logread -f | grep voice" log
 * Start app : app start voiceCallTest
 * Execute app : app runProc voiceCallTest --exe=voiceCallTest -- <Destination phone number>
 *               <Initiate the call>
 * Follow INFO instruction in traces.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    int nbArgument = le_arg_NumArgs();
    bool initiateCall = true;
    LE_INFO("Starting the application");

    if (nbArgument >= 1)
    {
        // Get the telephone number from the User.
        const char* phoneNumber = le_arg_GetArg(0);
        if (NULL == phoneNumber)
        {
            LE_ERROR("phoneNumber is NULL");
            exit(EXIT_FAILURE);
        }

        // Check whether to initiate a call or wait for an incoming call
        if (nbArgument >= 2)
        {
            const char* initiateCallPtr = le_arg_GetArg(1);
            if (NULL == initiateCallPtr)
            {
                LE_ERROR("Options missing");
                exit(EXIT_FAILURE);
            }
            initiateCall = strtol(initiateCallPtr, NULL, 0);
        }

        strncpy(DestinationNumber, phoneNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);

        LE_INFO("Phone number %s", DestinationNumber);
        HangUpTimer = le_timer_Create("MyHangUp");
        LE_ASSERT(HangUpTimer);
        le_clk_Time_t interval = {10, 0};
        LE_ASSERT(le_timer_SetInterval(HangUpTimer, interval) == LE_OK);
        LE_ASSERT(le_timer_SetHandler(HangUpTimer, HangUpTimerHandler) == LE_OK);

        VoiceCallHandlerRef = le_voicecall_AddStateHandler(MyCallEventHandler, NULL);
        if (initiateCall)
        {
            LE_ASSERT(Testle_voicecall_start() == LE_OK);
        }
    }
    else
    {
        LE_ERROR("PRINT USAGE => app runProc voiceCallTest --exe=voiceCallTest -- <Destination phone number>");
        exit(EXIT_SUCCESS);
    }
}
