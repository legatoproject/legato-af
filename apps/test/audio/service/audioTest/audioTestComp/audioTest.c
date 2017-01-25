/**
 * This module is for unit testing of the Audio service component.
 *
 * This test opens the Audio on USB for outgoing calls, it opens the Audio on Microphone and Speaker
 * for incoming calls.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include <stdio.h>
#include <pthread.h>

#include "interfaces.h"


static le_audio_StreamRef_t mdmRxAudioRef = NULL;
static le_audio_StreamRef_t mdmTxAudioRef = NULL;
static le_audio_StreamRef_t speakerRef = NULL;
static le_audio_StreamRef_t micRef = NULL;
static le_audio_StreamRef_t usbRxAudioRef = NULL;
static le_audio_StreamRef_t usbTxAudioRef = NULL;

static le_audio_ConnectorRef_t audioInputConnectorRef = NULL;
static le_audio_ConnectorRef_t audioOutputConnectorRef = NULL;

static const char* DestinationNumber;


static bool                 isAudioAlreadyConnected = false;



static void ConnectAudioToUsb
(
    void
)
{
    mdmRxAudioRef = le_audio_OpenModemVoiceRx();
    mdmTxAudioRef = le_audio_OpenModemVoiceTx();

    // Redirect audio to the USB.
    usbTxAudioRef = le_audio_OpenUsbTx();
    usbRxAudioRef = le_audio_OpenUsbRx();

    audioInputConnectorRef = le_audio_CreateConnector();
    audioOutputConnectorRef = le_audio_CreateConnector();

    if (mdmRxAudioRef && mdmTxAudioRef && usbTxAudioRef && usbRxAudioRef &&
        audioInputConnectorRef && audioOutputConnectorRef)
    {
        le_audio_Connect(audioInputConnectorRef, usbRxAudioRef);
        le_audio_Connect(audioInputConnectorRef, mdmTxAudioRef);
        le_audio_Connect(audioOutputConnectorRef, usbTxAudioRef);
        le_audio_Connect(audioOutputConnectorRef, mdmRxAudioRef);
    }
}


static void ConnectAudioToMicAndSpeaker
(
    void
)
{
    mdmRxAudioRef = le_audio_OpenModemVoiceRx();
    mdmTxAudioRef = le_audio_OpenModemVoiceTx();

    // Redirect audio to the in-built Microphone and Speaker.
    speakerRef = le_audio_OpenSpeaker();
    micRef = le_audio_OpenMic();

    audioInputConnectorRef = le_audio_CreateConnector();
    audioOutputConnectorRef = le_audio_CreateConnector();

    if (mdmRxAudioRef && mdmTxAudioRef && speakerRef && micRef &&
        audioInputConnectorRef && audioOutputConnectorRef)
    {
        le_audio_Connect(audioInputConnectorRef, micRef);
        le_audio_Connect(audioInputConnectorRef, mdmTxAudioRef);
        le_audio_Connect(audioOutputConnectorRef, speakerRef);
        le_audio_Connect(audioOutputConnectorRef, mdmRxAudioRef);
    }
}

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
        if(usbRxAudioRef)
        {
            le_audio_Disconnect(audioInputConnectorRef, usbRxAudioRef);
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
        if(usbTxAudioRef)
        {
            le_audio_Disconnect(audioOutputConnectorRef, usbTxAudioRef);
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
    if(usbRxAudioRef)
    {
        le_audio_Close(usbRxAudioRef);
        usbRxAudioRef = NULL;
    }
    if(usbTxAudioRef)
    {
        le_audio_Close(usbTxAudioRef);
        usbTxAudioRef = NULL;
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
            ConnectAudioToUsb();
        }
        isAudioAlreadyConnected = false;

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
            "   audioTest <tel number>",
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

COMPONENT_INIT
{
    static le_mcc_CallRef_t testCallRef;

    if (le_arg_NumArgs() == 1)
    {
        DestinationNumber = le_arg_GetArg(0);
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

