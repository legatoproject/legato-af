/**
 * This module is for unit testing of the Audio service component.
 *
 * This test opens the Audio on USB for outgoing calls, it opens the Audio on Microphone and Speaker
 * for incoming calls.
 *
 * On the target, you must issue the following commands:
 * app runProc audioTest --exe=audioTest -- <tel number>
 *     <MIC/USB/USBTXI2SRX/USBTXPCMRX/USBRXI2STX/USBRXPCMTX>
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include <pthread.h>

#include "interfaces.h"


static le_audio_StreamRef_t mdmRxAudioRef = NULL;
static le_audio_StreamRef_t mdmTxAudioRef = NULL;
static le_audio_StreamRef_t speakerRef = NULL;
static le_audio_StreamRef_t micRef = NULL;
static le_audio_StreamRef_t feInRef = NULL;
static le_audio_StreamRef_t feOutRef = NULL;

static le_audio_ConnectorRef_t audioInputConnectorRef = NULL;
static le_audio_ConnectorRef_t audioOutputConnectorRef = NULL;

static const char* DestinationNumber;
static const char* AudioTestCase;

static bool isAudioAlreadyConnected = false;


//--------------------------------------------------------------------------------------------------
/**
 * Connect audio to USB.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToUsb
(
    void
)
{
    le_result_t res;

    mdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((mdmRxAudioRef == NULL), "GetRxAudioStream returns NULL!");
    mdmTxAudioRef = le_audio_OpenModemVoiceTx();
    LE_ERROR_IF((mdmTxAudioRef == NULL), "GetTxAudioStream returns NULL!");

    // Redirect audio to the USB.
    feOutRef = le_audio_OpenUsbTx();
    LE_ERROR_IF((feOutRef == NULL), "OpenUsbTx returns NULL!");
    feInRef = le_audio_OpenUsbRx();
    LE_ERROR_IF((feInRef == NULL), "OpenUsbRx returns NULL!");

    audioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((audioInputConnectorRef == NULL), "AudioInputConnectorRef is NULL!");
    audioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((audioOutputConnectorRef == NULL), "AudioOutputConnectorRef is NULL!");

    if (mdmRxAudioRef && mdmTxAudioRef && feOutRef && feInRef &&
        audioInputConnectorRef && audioOutputConnectorRef)
    {
        res = le_audio_Connect(audioInputConnectorRef, feInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect Usb Rx on Input connector!");
        res = le_audio_Connect(audioInputConnectorRef, mdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect Usb Rx on Input connector!");
        res = le_audio_Connect(audioOutputConnectorRef, feOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect Usb Tx on Input connector!");
        res = le_audio_Connect(audioOutputConnectorRef, mdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect audio to Mic and Speaker.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToMicAndSpeaker
(
    void
)
{
    le_result_t res;

    mdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((mdmRxAudioRef == NULL), "GetRxAudioStream returns NULL!");
    mdmTxAudioRef = le_audio_OpenModemVoiceTx();
    LE_ERROR_IF((mdmTxAudioRef == NULL), "GetTxAudioStream returns NULL!");

    // Redirect audio to the in-built Microphone and Speaker.
    speakerRef = le_audio_OpenSpeaker();
    LE_ERROR_IF((speakerRef == NULL), "OpenSpeaker returns NULL!");
    micRef = le_audio_OpenMic();
    LE_ERROR_IF((micRef == NULL), "OpenMic returns NULL!");

    audioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((audioInputConnectorRef == NULL), "AudioInputConnectorRef is NULL!");
    audioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((audioOutputConnectorRef == NULL), "AudioOutputConnectorRef is NULL!");

    if (mdmRxAudioRef && mdmTxAudioRef && speakerRef && micRef &&
        audioInputConnectorRef && audioOutputConnectorRef)
    {
        res = le_audio_Connect(audioInputConnectorRef, micRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect Mic on Input connector!");
        res = le_audio_Connect(audioInputConnectorRef, mdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(audioOutputConnectorRef, speakerRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect Speaker on Output connector!");
        res = le_audio_Connect(audioOutputConnectorRef, mdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect audio to USB-TX & I2S-RX.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToUsbTxI2sRx
(
    void
)
{
    le_result_t res;

    mdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((mdmRxAudioRef == NULL), "GetRxAudioStream returns NULL!");
    mdmTxAudioRef = le_audio_OpenModemVoiceTx();
    LE_ERROR_IF((mdmTxAudioRef == NULL), "GetTxAudioStream returns NULL!");

    // Redirect audio to the USB.
    feOutRef = le_audio_OpenUsbTx();
    LE_ERROR_IF((feOutRef == NULL), "OpenUsbTx returns NULL!");
    // Redirect audio to the I2S.
    feInRef = le_audio_OpenI2sRx(LE_AUDIO_I2S_STEREO);
    LE_ERROR_IF((feInRef == NULL), "OpenI2sRx returns NULL!");

    audioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((audioInputConnectorRef == NULL), "AudioInputConnectorRef is NULL!");
    audioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((audioOutputConnectorRef == NULL), "AudioOutputConnectorRef is NULL!");

    if (mdmRxAudioRef && mdmTxAudioRef && feOutRef && feInRef &&
        audioInputConnectorRef && audioOutputConnectorRef)
    {
        res = le_audio_Connect(audioInputConnectorRef, feInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect I2S Rx on Input connector!");
        res = le_audio_Connect(audioInputConnectorRef, mdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(audioOutputConnectorRef, feOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect USB Tx on Output connector!");
        res = le_audio_Connect(audioOutputConnectorRef, mdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect audio to USB-TX & PCM-RX.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToUsbTxPcmRx
(
    void
)
{
    le_result_t res;

    mdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((mdmRxAudioRef == NULL), "GetRxAudioStream returns NULL!");
    mdmTxAudioRef = le_audio_OpenModemVoiceTx();
    LE_ERROR_IF((mdmTxAudioRef == NULL), "GetTxAudioStream returns NULL!");

    // Redirect audio to the USB.
    feOutRef = le_audio_OpenUsbTx();
    LE_ERROR_IF((feOutRef == NULL), "OpenUsbTx returns NULL!");
    // Redirect audio to the PCM.
    feInRef = le_audio_OpenPcmRx(0);
    LE_ERROR_IF((feInRef == NULL), "OpenPcmRx returns NULL!");

    audioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((audioInputConnectorRef == NULL), "AudioInputConnectorRef is NULL!");
    audioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((audioOutputConnectorRef == NULL), "AudioOutputConnectorRef is NULL!");

    if (mdmRxAudioRef && mdmTxAudioRef && feOutRef && feInRef &&
        audioInputConnectorRef && audioOutputConnectorRef)
    {
        res = le_audio_Connect(audioInputConnectorRef, feInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect PCM Rx on Input connector!");
        res = le_audio_Connect(audioInputConnectorRef, mdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(audioOutputConnectorRef, feOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect USB Tx on Output connector!");
        res = le_audio_Connect(audioOutputConnectorRef, mdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect audio to USB-RX & I2S-TX.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToUsbRxI2sTx
(
    void
)
{
    le_result_t res;

    mdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((mdmRxAudioRef == NULL), "GetRxAudioStream returns NULL!");
    mdmTxAudioRef = le_audio_OpenModemVoiceTx();
    LE_ERROR_IF((mdmTxAudioRef == NULL), "GetTxAudioStream returns NULL!");

    // Redirect audio to the I2S.
    feOutRef = le_audio_OpenI2sTx(LE_AUDIO_I2S_STEREO);
    LE_ERROR_IF((feOutRef == NULL), "OpenI2sTx returns NULL!");
    // Redirect audio to the USB.
    feInRef = le_audio_OpenUsbRx();
    LE_ERROR_IF((feInRef == NULL), "OpenUsbRx returns NULL!");

    audioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((audioInputConnectorRef == NULL), "AudioInputConnectorRef is NULL!");
    audioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((audioOutputConnectorRef == NULL), "AudioOutputConnectorRef is NULL!");

    if (mdmRxAudioRef && mdmTxAudioRef && feOutRef && feInRef &&
        audioInputConnectorRef && audioOutputConnectorRef)
    {
        res = le_audio_Connect(audioInputConnectorRef, feInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect USB Rx on Input connector!");
        res = le_audio_Connect(audioInputConnectorRef, mdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(audioOutputConnectorRef, feOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect I2S Tx on Output connector!");
        res = le_audio_Connect(audioOutputConnectorRef, mdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect audio to USB-RX & PCM-TX.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToUsbRxPcmTx
(
    void
)
{
    le_result_t res;

    mdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((mdmRxAudioRef == NULL), "GetRxAudioStream returns NULL!");
    mdmTxAudioRef = le_audio_OpenModemVoiceTx();
    LE_ERROR_IF((mdmTxAudioRef == NULL), "GetTxAudioStream returns NULL!");

    // Redirect audio to the PCM.
    feOutRef = le_audio_OpenPcmTx(0);
    LE_ERROR_IF((feOutRef == NULL), "OpenPcmTx returns NULL!");
    // Redirect audio to the USB.
    feInRef = le_audio_OpenUsbRx();
    LE_ERROR_IF((feInRef == NULL), "OpenUsbRx returns NULL!");

    audioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((audioInputConnectorRef == NULL), "AudioInputConnectorRef is NULL!");
    audioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((audioOutputConnectorRef == NULL), "AudioOutputConnectorRef is NULL!");

    if (mdmRxAudioRef && mdmTxAudioRef && feOutRef && feInRef &&
        audioInputConnectorRef && audioOutputConnectorRef)
    {
        res = le_audio_Connect(audioInputConnectorRef, feInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect USB Rx on Input connector!");
        res = le_audio_Connect(audioInputConnectorRef, mdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(audioOutputConnectorRef, feOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect PCM Tx on Output connector!");
        res = le_audio_Connect(audioOutputConnectorRef, mdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Disconnection function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DisconnectAllAudio
(
    le_mcc_CallRef_t callRef
)
{
    if (audioInputConnectorRef)
    {
        if (micRef)
        {
            le_audio_Disconnect(audioInputConnectorRef, micRef);
        }
        if(feInRef)
        {
            le_audio_Disconnect(audioInputConnectorRef, feInRef);
        }
        if(mdmTxAudioRef)
        {
            le_audio_Disconnect(audioInputConnectorRef, mdmTxAudioRef);
        }
    }
    if(audioOutputConnectorRef)
    {
        if(speakerRef)
        {
            le_audio_Disconnect(audioOutputConnectorRef, speakerRef);
        }
        if(feOutRef)
        {
            le_audio_Disconnect(audioOutputConnectorRef, feOutRef);
        }
        if(mdmRxAudioRef)
        {
            le_audio_Disconnect(audioOutputConnectorRef, mdmRxAudioRef);
        }
    }

    if(audioInputConnectorRef)
    {
        le_audio_DeleteConnector(audioInputConnectorRef);
        audioInputConnectorRef = NULL;
    }
    if(audioOutputConnectorRef)
    {
        le_audio_DeleteConnector(audioOutputConnectorRef);
        audioOutputConnectorRef = NULL;
    }
    if(mdmRxAudioRef)
    {
        le_audio_Close(mdmRxAudioRef);
        feOutRef = NULL;
    }
    if(mdmTxAudioRef)
    {
        le_audio_Close(mdmTxAudioRef);
        feOutRef = NULL;
    }
    if(speakerRef)
    {
        le_audio_Close(speakerRef);
        speakerRef = NULL;
    }
    if(micRef)
    {
        le_audio_Close(micRef);
        micRef = NULL;
    }
    if(feInRef)
    {
        le_audio_Close(feInRef);
        feInRef = NULL;
    }
    if(feOutRef)
    {
        le_audio_Close(feOutRef);
        feOutRef = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Call Event Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyCallEventHandler(le_mcc_CallRef_t callRef, le_mcc_Event_t callEvent, void* contextPtr)
{
    le_result_t         res;

    if (callEvent == LE_MCC_EVENT_ALERTING)
    {
        LE_INFO("Call event is LE_MCC_EVENT_ALERTING.");
    }
    else if (callEvent == LE_MCC_EVENT_CONNECTED)
    {
        // Outgoing call case
        if (!isAudioAlreadyConnected)
        {
            if (strcmp(AudioTestCase,"MIC")==0)
            {
                LE_INFO("Connect MIC and SPEAKER ");
                ConnectAudioToMicAndSpeaker();
            }
            else if (strcmp(AudioTestCase,"USB")==0)
            {
                LE_INFO("Connect USB ");
                ConnectAudioToUsb();
            }
            else if (strcmp(AudioTestCase,"USBTXI2SRX")==0)
            {
                LE_INFO("Connect USBTXI2SRX ");
                ConnectAudioToUsbTxI2sRx();
            }
            else if (strcmp(AudioTestCase,"USBTXPCMRX")==0)
            {
                LE_INFO("Connect USBTXPCMRX ");
                ConnectAudioToUsbTxPcmRx();
            }
            else if (strcmp(AudioTestCase,"USBRXI2STX")==0)
            {
                LE_INFO("Connect USBRXI2STX ");
                ConnectAudioToUsbRxI2sTx();
            }
            else if (strcmp(AudioTestCase,"USBRXPCMTX")==0)
            {
                LE_INFO("Connect USBRXPCMTX ");
                ConnectAudioToUsbRxPcmTx();
            }
            else
            {
                LE_INFO("Bad test case");
            }
        }
        isAudioAlreadyConnected = true;

        LE_INFO("Call event is LE_MCC_EVENT_CONNECTED.");
    }
    else if (callEvent == LE_MCC_EVENT_TERMINATED)
    {
        DisconnectAllAudio(callRef);

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
        le_mcc_Delete(callRef);
    }
    else if (callEvent == LE_MCC_EVENT_INCOMING)
    {
        ConnectAudioToMicAndSpeaker();
        isAudioAlreadyConnected = true;

        LE_INFO("Call event is LE_MCC_EVENT_INCOMING.");
        res = le_mcc_Answer(callRef);
        if (res == LE_OK)
        {
            LE_INFO("Call I answered the call");
        }
        else
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
 * Help.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage()
{
    int idx;
    bool sandboxed = (getuid() != 0);
    const char * usagePtr[] = {
            "Usage of the 'audioTest' tool is:",
            "app runProc audioTest --exe=audioTest -- <tel number>"
            " <MIC/USB/USBTXI2SRX/USBTXPCMRX/USBRXI2STX/USBRXPCMTX>",
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
    static le_mcc_CallRef_t testCallRef;

    if (le_arg_NumArgs() == 2)
    {
        DestinationNumber = le_arg_GetArg(0);
        AudioTestCase = le_arg_GetArg(1);
        LE_INFO("   Phone number.%s", DestinationNumber);
        LE_INFO("   Test case.%s", AudioTestCase);
    }
    else
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    le_mcc_AddCallEventHandler(MyCallEventHandler, NULL);

    testCallRef=le_mcc_Create(DestinationNumber);
    le_mcc_Start(testCallRef);
}

