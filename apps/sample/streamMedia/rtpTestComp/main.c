/**
 *
 * This file contains the source code of the rtpTest executable from the streamMediaTest
 * application.
 * The purpose of this executable is to show how to redirect audio from a Modem Call to a RTP
 * session.
 *
 * Usage is :
 *     app runProc streamMediaTest --exe=rtpTest -- <test case> <remote ipv4 addr> <tel number>
 * Here are some example commands :
 *     app runProc streamMediaTest --exe=rtpTest -- AUDIO_PEER 192.168.10.2
 *     app runProc streamMediaTest --exe=rtpTest -- MODEM_PEER 192.168.2.5 0123456789
 *
 * Note that the application must be terminated using "app stop streamMediaTest".
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include <legato.h>
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

#define LOCAL_PORT (4000)

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

static le_mcc_CallRef_t                CallRef = NULL;
static le_audio_StreamRef_t            RtpInRef = NULL;
static le_audio_StreamRef_t            RtpOutRef = NULL;
static le_audio_StreamRef_t            SpeakerAudioRef = NULL;
static le_audio_StreamRef_t            MicAudioRef = NULL;
static le_audio_StreamRef_t            MdmRxAudioRef = NULL;
static le_audio_StreamRef_t            MdmTxAudioRef = NULL;
static le_audio_StreamRef_t            UsbRxAudioRef = NULL;
static le_audio_StreamRef_t            UsbTxAudioRef = NULL;
static le_audio_ConnectorRef_t         AudioInputConnectorRef = NULL;
static le_audio_ConnectorRef_t         AudioOutputConnectorRef = NULL;
static streamMedia_RtcpHandlerRef_t    RtcpHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Disconnect all audio interface from the connectors and exit.
 *
 */
//--------------------------------------------------------------------------------------------------
void DisconnectAllAudio
(
    void
)
{
    streamMedia_Stop(RtpOutRef);
    streamMedia_Stop(RtpInRef);

    if (AudioInputConnectorRef)
    {
        if (RtpOutRef)
        {
            le_audio_Disconnect(AudioInputConnectorRef, RtpOutRef);
        }
        if (MicAudioRef)
        {
            le_audio_Disconnect(AudioInputConnectorRef, MicAudioRef);
        }
        if (MdmRxAudioRef)
        {
            le_audio_Disconnect(AudioInputConnectorRef, MdmRxAudioRef);
        }
        if (UsbRxAudioRef)
        {
            le_audio_Disconnect(AudioInputConnectorRef, UsbRxAudioRef);
        }
    }
    if (AudioOutputConnectorRef)
    {
        if (RtpInRef)
        {
            le_audio_Disconnect(AudioOutputConnectorRef, RtpInRef);
        }
        if (SpeakerAudioRef)
        {
            le_audio_Disconnect(AudioOutputConnectorRef, SpeakerAudioRef);
        }
        if (MdmTxAudioRef)
        {
            le_audio_Disconnect(AudioOutputConnectorRef, MdmTxAudioRef);
        }
        if (UsbTxAudioRef)
        {
            le_audio_Disconnect(AudioOutputConnectorRef, UsbTxAudioRef);
        }
    }

    if (AudioInputConnectorRef)
    {
        le_audio_DeleteConnector(AudioInputConnectorRef);
        AudioInputConnectorRef = NULL;
    }
    if (AudioOutputConnectorRef)
    {
        le_audio_DeleteConnector(AudioOutputConnectorRef);
        AudioOutputConnectorRef = NULL;
    }

    if (RtpOutRef)
    {
        streamMedia_Close(RtpOutRef);
    }
    if (RtpInRef)
    {
        streamMedia_Close(RtpInRef);
        RtpInRef = NULL;
    }

    if (MdmRxAudioRef)
    {
        le_audio_Close(MdmRxAudioRef);
        MdmRxAudioRef = NULL;
    }
    if (MdmTxAudioRef)
    {
        le_audio_Close(MdmTxAudioRef);
        MdmTxAudioRef = NULL;
    }

    if (MicAudioRef)
    {
        le_audio_Close(MicAudioRef);
        MicAudioRef = NULL;
    }
    if (SpeakerAudioRef)
    {
        le_audio_Close(SpeakerAudioRef);
        SpeakerAudioRef = NULL;
    }

    if (UsbRxAudioRef)
    {
        le_audio_Close(UsbRxAudioRef);
        UsbRxAudioRef = NULL;
    }
    if (UsbTxAudioRef)
    {
        le_audio_Close(UsbTxAudioRef);
        UsbTxAudioRef = NULL;
    }

    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Call Event Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyCallEventHandler
(
    le_mcc_CallRef_t    callRef,
    le_mcc_Event_t      callEvent,
    void*               contextPtr
)
{
    if (LE_MCC_EVENT_ALERTING == callEvent)
    {
        LE_INFO("Call event is LE_MCC_EVENT_ALERTING.");
    }
    else if (LE_MCC_EVENT_CONNECTED == callEvent)
    {
        LE_INFO("Call event is LE_MCC_EVENT_CONNECTED.");
    }
    else if (LE_MCC_EVENT_TERMINATED == callEvent)
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

        streamMedia_SendRtcpBye(RtpOutRef, "Call terminated");
        if (RtcpHandlerRef)
        {
            streamMedia_RemoveRtcpHandler(RtcpHandlerRef);
        }

        // 2-second pause: workaround to step over possible pcm_open error on AR8 platforms
        sleep(2);

        le_mcc_Delete(callRef);
        DisconnectAllAudio();
    }
    else if (LE_MCC_EVENT_INCOMING == callEvent)
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
 * Handler function for RTCP Event Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyRtcpEventHandler
(
    le_audio_StreamRef_t          streamRef,
    streamMedia_RtcpEvent_t       event,
    void*                         contextPtr
)
{
    switch (event)
    {
        case STREAMMEDIA_RTCP_BYE:
            if (CallRef)
            {
                le_mcc_HangUp(CallRef);
            }
            if (RtcpHandlerRef)
            {
                streamMedia_RemoveRtcpHandler(RtcpHandlerRef);
            }
            DisconnectAllAudio();
            break;
        default:
            LE_INFO("Other event");
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Create the RTP session and connect it to the audio connector
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToRtp
(
    const char* remoteAddress
)
{
    le_result_t res;

    LE_INFO("RTP remote address : %s", remoteAddress);

    RtpInRef = streamMedia_OpenAudioRtpRx(LOCAL_PORT);
    LE_ERROR_IF((RtpInRef==NULL), "RtpInRef returns NULL!");
    res = le_audio_Connect(AudioOutputConnectorRef, RtpInRef);
    LE_ERROR_IF((res!=LE_OK), "Failed to connect RtpInRef on RtpMdm connector!");
    res = streamMedia_Start(RtpInRef);
    LE_ERROR_IF((res!=LE_OK), "Failed to start RTP reception");

    RtpOutRef = streamMedia_OpenAudioRtpTx(LOCAL_PORT, remoteAddress, LOCAL_PORT);
    LE_ERROR_IF((RtpOutRef==NULL), "RtpOutRef returns NULL!");
    res = le_audio_Connect(AudioInputConnectorRef, RtpOutRef);
    LE_ERROR_IF((res!=LE_OK), "Failed to connect RtpInRef on RtpMdm connector!");
    res = streamMedia_Start(RtpOutRef);
    LE_ERROR_IF((res!=LE_OK), "Failed to start RTP reception");

}

//--------------------------------------------------------------------------------------------------
/**
 * Connect Mic and Speaker to the audio connector
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToMicAndSpeaker
(
    void
)
{
    le_result_t res;

    SpeakerAudioRef = le_audio_OpenSpeaker();
    LE_ERROR_IF((SpeakerAudioRef==NULL), "OpenSpeaker returns NULL!");
    MicAudioRef = le_audio_OpenMic();
    LE_ERROR_IF((MicAudioRef==NULL), "OpenMic returns NULL!");

    res = le_audio_Connect(AudioOutputConnectorRef, SpeakerAudioRef);
    LE_ERROR_IF((res!=LE_OK), "Failed to connect Speaker on Output connector!");
    res = le_audio_Connect(AudioInputConnectorRef, MicAudioRef);
    LE_ERROR_IF((res!=LE_OK), "Failed to connect Speaker on Output connector!");
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect Modem to the audio connector
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToModem
(
    void
)
{
    le_result_t res;

    MdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((MdmRxAudioRef==NULL), "MdmRxAudioRef returns NULL!");
    MdmTxAudioRef = le_audio_OpenModemVoiceTx();
    LE_ERROR_IF((MdmTxAudioRef==NULL), "MdmTxAudioRef returns NULL!");

    res = le_audio_Connect(AudioOutputConnectorRef, MdmTxAudioRef);
    LE_ERROR_IF((res!=LE_OK), "Failed to connect ModemRx on Output connector!");
    res = le_audio_Connect(AudioInputConnectorRef, MdmRxAudioRef);
    LE_ERROR_IF((res!=LE_OK), "Failed to connect ModemRx on Output connector!");
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect USB to the audio connector
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToUsb
(
    void
)
{
    le_result_t res;

    UsbRxAudioRef = le_audio_OpenUsbRx();
    LE_ERROR_IF((UsbRxAudioRef==NULL), "UsbRxAudioRef returns NULL!");
    UsbTxAudioRef = le_audio_OpenUsbTx();
    LE_ERROR_IF((UsbTxAudioRef==NULL), "UsbTxAudioRef returns NULL!");

    res = le_audio_Connect(AudioInputConnectorRef, UsbRxAudioRef);
    LE_ERROR_IF((res!=LE_OK), "Failed to connect UsbRx on Input connector!");
    res = le_audio_Connect(AudioOutputConnectorRef, UsbTxAudioRef);
    LE_ERROR_IF((res!=LE_OK), "Failed to connect UsbTx on Output connector!");
}

//--------------------------------------------------------------------------------------------------
/**
 * The signal event handler function for SIGTERM when process dies.
 */
//--------------------------------------------------------------------------------------------------
static void SigHandler
(
    int sigNum
)
{
    LE_INFO("End streamMedia test");
    if (CallRef)
    {
        le_mcc_HangUp(CallRef);
    }
    if (RtcpHandlerRef)
    {
        streamMedia_RemoveRtcpHandler(RtcpHandlerRef);
    }
    streamMedia_SendRtcpBye(RtpOutRef, "Application terminated");
    DisconnectAllAudio();
    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Helper.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage
(
    void
)
{
    int idx;
    bool sandboxed = (getuid() != 0);
    const char * usagePtr[] = {
            "Usage of the rtpModem test is:",
            "   app runProc streamMediaTest --exe=rtpModem -- <test case> <remote ipv4 addr> <tel number>",
            "",
            "Test cases are:",
            " - AUDIO_PEER (Connect RTP to Mic and Speaker)",
            " - MODEM_PEER (Connect RTP to Modem Call)",
            " - USB_PEER (Connect RTP to USB)",
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
 * Initialize the sample application.
 *
 * Execute application with 'app runProc streamMediaTest --exe=rtpTest -- [options]
 * (see PrintUsage())
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    const char* audioTestCase;

    // Register a signal event handler for SIGTERM when the app stops
    le_sig_Block(SIGTERM);
    le_sig_SetEventHandler(SIGTERM, SigHandler);

    const char* remoteAddress;

    if (le_arg_NumArgs() < 2)
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    audioTestCase = le_arg_GetArg(0);

    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef==NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef==NULL), "AudioOutputConnectorRef is NULL!");

    remoteAddress = le_arg_GetArg(1);

    if (strncmp(audioTestCase, "AUDIO_PEER", 10)==0)
    {
        ConnectAudioToMicAndSpeaker();
        ConnectAudioToRtp(remoteAddress);
    }
    else if (strncmp(audioTestCase, "MODEM_PEER", 10)==0)
    {
        if (le_arg_NumArgs() < 3)
        {
            PrintUsage();
            exit(EXIT_FAILURE);
        }
        ConnectAudioToModem();
        ConnectAudioToRtp(remoteAddress);

        const char* destinationNumber = le_arg_GetArg(2);
        le_mcc_AddCallEventHandler(MyCallEventHandler, NULL);
        CallRef=le_mcc_Create(destinationNumber);
        le_mcc_Start(CallRef);
    }
    else if (strncmp(audioTestCase, "USB_PEER", 8)==0)
    {
        ConnectAudioToUsb();
        ConnectAudioToRtp(remoteAddress);
    }

    RtcpHandlerRef = streamMedia_AddRtcpHandler(RtpInRef, MyRtcpEventHandler, NULL);
}
