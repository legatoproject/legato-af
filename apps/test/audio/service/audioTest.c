/**
 * This module is for unit testing of the Audio service component.
 *
 * This test opens the Audio on USB for outgoing calls, it opens the Audio on Microphone and Speaker
 * for incoming calls.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013.  All rights reserved. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include <stdio.h>
#include <pthread.h>

#include "le_ms.h"
#include "le_mcc.h"
#include "le_audio.h"
#include "le_audio_local.h"


static le_audio_StreamRef_t mdmRxAudioRef = NULL;
static le_audio_StreamRef_t mdmTxAudioRef = NULL;
static le_audio_StreamRef_t speakerRef = NULL;
static le_audio_StreamRef_t micRef = NULL;
static le_audio_StreamRef_t usbRxAudioRef = NULL;
static le_audio_StreamRef_t usbTxAudioRef = NULL;

static le_audio_ConnectorRef_t audioInputConnectorRef = NULL;
static le_audio_ConnectorRef_t audioOutputConnectorRef = NULL;

static char  DEST_TEST_PATTERN[LE_TEL_NMBR_MAX_LEN];

static le_mcc_call_Ref_t       testCallRef;

static bool                    isAudioAlreadyConnected = false;


//--------------------------------------------------------------------------------------------------
/**
 * This function gets the telephone number from the User.
 *
 */
//--------------------------------------------------------------------------------------------------
static int32_t getTel(void)
{
    char *strPtr;

    do
    {
        fprintf(stderr, "Please enter the destination's telephone number to start a call or 'stop' to exit: \n");
        strPtr=fgets ((char*)DEST_TEST_PATTERN, 16, stdin);
    }while (strlen(strPtr) == 0);


    DEST_TEST_PATTERN[strlen(DEST_TEST_PATTERN)-1]='\0';

    if (!strcmp(DEST_TEST_PATTERN, "stop"))
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

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
    le_mcc_call_Ref_t callRef
)
{
    // mdmRxAudioRef and mdmTxAudioRef become null
    mdmRxAudioRef = le_mcc_call_GetRxAudioStream(callRef);
    mdmTxAudioRef = le_mcc_call_GetTxAudioStream(callRef);
    
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
static void MyCallEventHandler(le_mcc_call_Ref_t  callRef, le_mcc_call_Event_t callEvent, void* contextPtr)
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
        le_mcc_call_TerminationReason_t term = le_mcc_call_GetTerminationReason(callRef);
        switch(term)
        {
            case LE_MCC_TERM_NETWORK_FAIL:
                LE_INFO("Termination reason is LE_MCC_TERM_NETWORK_FAIL");
                break;

            case LE_MCC_TERM_BAD_ADDRESS:
                LE_INFO("Termination reason is LE_MCC_TERM_BAD_ADDRESS");
                break;

            case LE_MCC_TERM_BUSY:
                LE_INFO("Termination reason is LE_MCC_TERM_BUSY");
                break;

            case LE_MCC_TERM_LOCAL_ENDED:
                LE_INFO("Termination reason is LE_MCC_TERM_LOCAL_ENDED");
                break;

            case LE_MCC_TERM_REMOTE_ENDED:
                LE_INFO("Termination reason is LE_MCC_TERM_REMOTE_ENDED");
                break;

            case LE_MCC_TERM_NOT_DEFINED:
                LE_INFO("Termination reason is LE_MCC_TERM_NOT_DEFINED");
                break;

            default:
                LE_INFO("Termination reason is %d", term);
                break;
        }
        le_mcc_call_Delete(callRef);
    }
    else if (callEvent == LE_MCC_EVENT_INCOMING)
    {
        ConnectAudioToMicAndSpeaker();
        isAudioAlreadyConnected = true;

        LE_INFO("Call event is LE_MCC_EVENT_INCOMING.");
        res = le_mcc_call_Answer(callRef);
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


static void* HandlerThread(void* contextPtr)
{
    le_mcc_profile_Ref_t profileRef = contextPtr;

    le_mcc_profile_AddCallEventHandler(profileRef, MyCallEventHandler, NULL);

    le_event_RunLoop();
    return NULL;
}


static void* TestAudioService(void* contextPtr)
{
    le_mcc_profile_Ref_t profileRef;
    int32_t              stopTest = 0;


    profileRef=le_mcc_profile_GetByName("Modem-Sim1");
    if ( profileRef == NULL )
    {
        LE_INFO("Unable to get the Call profile reference");
        return NULL;
    }

    // Start the handler thread to monitor the call for the just created profile.
    le_thread_Start(le_thread_Create("MCC", HandlerThread, profileRef));


    while(!stopTest)
    {
        stopTest = getTel();
        if (!stopTest)
        {
            testCallRef=le_mcc_profile_CreateCall(profileRef, DEST_TEST_PATTERN);
            le_mcc_call_Start(testCallRef);
        }
        else
        {
            LE_INFO("Exit Audio Test!");
            exit(0);
        }
    }

    return NULL;
}


LE_EVENT_INIT_HANDLER
{
    // Note that this init should be done in the main thread, and in particular, should not be done
    // in the same thread as the tests.
    le_ms_Init();
    le_audio_Init();

    le_thread_Start(le_thread_Create("TestAudio", TestAudioService, NULL));
}

