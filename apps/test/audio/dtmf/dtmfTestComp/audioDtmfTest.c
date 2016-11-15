/**
 * This module is for testing of the DTMF Audio service.
 *
 * On the target, you must issue the following commands:
 * $ app start dtmfTest
 * $ app runProc dtmfTest --exe=dtmfTest -- <loc/rem> <dtmfs> <duration in ms> <pause in ms>
 *   [<tel number> <inband/outband>]
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include <stdio.h>
#include <pthread.h>


#include "interfaces.h"

static bool isIncoming=false;
static le_mcc_CallRef_t TestCallRef;

static le_audio_StreamRef_t MdmRxAudioRef = NULL;
static le_audio_StreamRef_t MdmTxAudioRef = NULL;
static le_audio_StreamRef_t FeInRef = NULL;
static le_audio_StreamRef_t FeOutRef = NULL;
static le_audio_StreamRef_t PlayerAudioRef = NULL;

static le_audio_ConnectorRef_t AudioInputConnectorRef = NULL;
static le_audio_ConnectorRef_t AudioOutputConnectorRef = NULL;

static const char*  DestinationNumber;
static const char*  DtmfSendingCase;
static const char*  DtmfString;
static uint32_t     Duration;
static uint32_t     Pause;

static le_audio_DtmfDetectorHandlerRef_t DtmfHandlerRef1 = NULL;
static le_audio_DtmfDetectorHandlerRef_t DtmfHandlerRef2 = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Handler functions for DTMF Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyDtmfDetectorHandler1
(
    le_audio_StreamRef_t streamRef,
    char  dtmf,
    void* contextPtr
)
{
    LE_INFO("MyDtmfDetectorHandler1 detects %c", dtmf);
}

static void MyDtmfDetectorHandler2
(
    le_audio_StreamRef_t streamRef,
    char  dtmf,
    void* contextPtr
)
{
    LE_INFO("MyDtmfDetectorHandler2 detects %c", dtmf);
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect audio to PCM.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToPcm
(
    void
)
{
    le_result_t res;

    MdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((MdmRxAudioRef==NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_audio_OpenModemVoiceTx();
    LE_ERROR_IF((MdmTxAudioRef==NULL), "GetTxAudioStream returns NULL!");

    // Redirect audio to the PCM interface.
    FeOutRef = le_audio_OpenPcmTx(0);
    LE_ERROR_IF((FeOutRef==NULL), "OpenPcmTx returns NULL!");
    FeInRef = le_audio_OpenPcmRx(0);
    LE_ERROR_IF((FeInRef==NULL), "OpenPcmRx returns NULL!");

    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef==NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef==NULL), "AudioOutputConnectorRef is NULL!");

    if (MdmRxAudioRef && MdmTxAudioRef && FeOutRef && FeInRef &&
        AudioInputConnectorRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FeInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect PCM RX on Input connector!");
        res = le_audio_Connect(AudioInputConnectorRef, MdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect PCM TX on Output connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, MdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Disconnect audio.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DisconnectAllAudio
(
    void
)
{
    if (DtmfHandlerRef1)
    {
        LE_INFO("delete DTMF handler 1\n");
        le_audio_RemoveDtmfDetectorHandler(DtmfHandlerRef1);
        DtmfHandlerRef1 = NULL;
        sleep(1);
    }
    if (DtmfHandlerRef2)
    {
        LE_INFO("delete DTMF handler 2\n");
        le_audio_RemoveDtmfDetectorHandler(DtmfHandlerRef2);
        DtmfHandlerRef2 = NULL;
    }

    if (AudioInputConnectorRef)
    {
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
        if(PlayerAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", PlayerAudioRef, AudioInputConnectorRef);
            le_audio_Disconnect(AudioInputConnectorRef, PlayerAudioRef);
        }
    }
    if(AudioOutputConnectorRef)
    {
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
        if(PlayerAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", PlayerAudioRef, AudioOutputConnectorRef);
            le_audio_Disconnect(AudioOutputConnectorRef, PlayerAudioRef);
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
        FeOutRef = NULL;
    }
    if(MdmTxAudioRef)
    {
        le_audio_Close(MdmTxAudioRef);
        FeOutRef = NULL;
    }
    if(PlayerAudioRef)
    {
        le_audio_Close(PlayerAudioRef);
        PlayerAudioRef = NULL;
    }
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
    le_mcc_Event_t    callEvent,
    void*                  contextPtr
)
{
    le_result_t         res;

    if (callEvent == LE_MCC_EVENT_ALERTING)
    {
        LE_INFO("Call event is LE_MCC_EVENT_ALERTING.");
    }
    else if (callEvent == LE_MCC_EVENT_CONNECTED)
    {
        LE_INFO("Call event is LE_MCC_EVENT_CONNECTED.");

        if (strcmp(DtmfSendingCase,"inband")==0)
        {
            PlayerAudioRef = le_audio_OpenPlayer();
            LE_ERROR_IF((PlayerAudioRef==NULL), "OpenPlayer returns NULL!");

            if (PlayerAudioRef && AudioInputConnectorRef)
            {
                res = le_audio_Connect(AudioInputConnectorRef, PlayerAudioRef);
                if(res != LE_OK)
                {
                    LE_ERROR("Failed to connect Player on input connector!");
                    return;
                }
                res = le_audio_PlayDtmf(PlayerAudioRef, DtmfString, Duration, Pause);
                if(res != LE_OK)
                {
                    LE_ERROR("Failed to play DTMF!");
                    return;
                }
            }
        }
        else if (strcmp(DtmfSendingCase,"outband")==0)
        {
            res = le_audio_PlaySignallingDtmf(DtmfString, Duration, Pause);
            if(res != LE_OK)
            {
                LE_ERROR("Failed to play signalling DTMF!");
                return;
            }
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
        exit(0);
    }
    else if (callEvent == LE_MCC_EVENT_INCOMING)
    {
        LE_INFO("Call event is LE_MCC_EVENT_INCOMING.");
        isIncoming=true;
        res = le_mcc_Answer(callRef);
        if (res != LE_OK)
        {
            LE_INFO("Failed to answer the call.");
        }
    }
    else
    {
        LE_INFO("Unknowm Call event.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Play DTMF function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PlayLocalDtmf
(
    void
)
{
    le_result_t res;
#if (ENABLE_CODEC == 1)
    LE_INFO("Play DTMF on Speaker");
    // Redirect audio to the in-built Microphone and Speaker.
    FeOutRef = le_audio_OpenSpeaker();
    LE_ERROR_IF((FeOutRef==NULL), "OpenSpeaker returns NULL!");
#else
    LE_INFO("Play DTMF on PCM output interface");
    // Redirect audio to the PCM Tx.
    FeOutRef = le_audio_OpenPcmTx(0);
    LE_ERROR_IF((FeOutRef==NULL), "OpenPcmTx returns NULL!");
#endif

    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef==NULL), "AudioOutputConnectorRef is NULL!");
    {
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect Speaker on Output connector!");
    }

    // Play DTMF on output connector.
    PlayerAudioRef = le_audio_OpenPlayer();
    LE_ERROR_IF((PlayerAudioRef==NULL), "OpenPlayer returns NULL!");

    if (PlayerAudioRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioOutputConnectorRef, PlayerAudioRef);
        if(res != LE_OK)
        {
            LE_ERROR("Failed to connect Player on output connector!");
            return;
        }
        LE_INFO("Play DTMF on PlayerAudioRef.%p", PlayerAudioRef);
        res = le_audio_PlayDtmf(PlayerAudioRef, DtmfString, Duration, Pause);
        if(res != LE_OK)
        {
            LE_ERROR("Failed to play DTMF!");
            return;
        }
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
            "Usage of the 'dtmfTest' app is:",
            "   app runProc dtmfTest --exe=dtmfTest -- <loc/rem> <dtmfs> <duration in ms> <pause in ms> "
            "[<tel number> <inband/outband>] ",
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
 * The signal event handler function for SIGINT/SIGTERM when process dies.
 */
//--------------------------------------------------------------------------------------------------
static void SigHandler
(
    int sigNum
)
{
    LE_INFO("End DTMF test");
    DisconnectAllAudio();
    if(TestCallRef)
    {
        le_mcc_HangUp(TestCallRef);
        le_mcc_Delete(TestCallRef);
    }

    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    bool isLocalTest = false;

    // Register a signal event handler for SIGINT when user interrupts/terminates process
    signal(SIGINT, SigHandler);

    if (le_arg_NumArgs() == 6)
    {
        DtmfString = le_arg_GetArg(1);
        Duration = atoi(le_arg_GetArg(2));
        Pause = atoi(le_arg_GetArg(3));
        DestinationNumber = le_arg_GetArg(4);
        DtmfSendingCase = le_arg_GetArg(5);
        LE_INFO("   DTMF to play.\"%s\"", DtmfString);
        LE_INFO("   Duration.%dms", Duration);
        LE_INFO("   Pause.%dms", Pause);
        LE_INFO("   Phone number.%s", DestinationNumber);
        LE_INFO("   DTMF Sending case.%s", DtmfSendingCase);
    }
    else if (le_arg_NumArgs() == 4)
    {
        if(strncmp(le_arg_GetArg(0), "loc", strlen("loc"))==0)
        {
            LE_INFO("   Play DTMF on local interface");
            DtmfString = le_arg_GetArg(1);
            Duration = atoi(le_arg_GetArg(2));
            Pause = atoi(le_arg_GetArg(3));
            LE_INFO("   DTMF to play.\"%s\"", DtmfString);
            LE_INFO("   Duration.%dms", Duration);
            LE_INFO("   Pause.%dms", Pause);
            isLocalTest = true;
        }
        else
        {
            PrintUsage();
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    if (!isLocalTest)
    {
        ConnectAudioToPcm();
        DtmfHandlerRef1 = le_audio_AddDtmfDetectorHandler(MdmRxAudioRef,
                                                          MyDtmfDetectorHandler1,
                                                          NULL);
        DtmfHandlerRef2 = le_audio_AddDtmfDetectorHandler(MdmRxAudioRef,
                                                          MyDtmfDetectorHandler2,
                                                          NULL);

        le_mcc_AddCallEventHandler(MyCallEventHandler, NULL);
        TestCallRef=le_mcc_Create(DestinationNumber);
        le_mcc_Start(TestCallRef);
    }
    else
    {
        PlayLocalDtmf();
    }
}

