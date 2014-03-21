/**
 * This module is for unit testing of the MCC service component as a client of MS daemon.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013.  All rights reserved. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include <stdio.h>
#include <pthread.h>


#include "le_mcc.h"
#include "le_audio.h"

static char  DEST_TEST_PATTERN[LE_TEL_NMBR_MAX_LEN];
static char  TYPE_TEST_PATTERN[16];

static le_mcc_call_Ref_t       testCallRef;

static le_audio_StreamRef_t mdmRxAudioRef = NULL;
static le_audio_StreamRef_t mdmTxAudioRef = NULL;
static le_audio_StreamRef_t codecOutRef = NULL;
static le_audio_StreamRef_t codecInRef = NULL;
static le_audio_StreamRef_t usbRxAudioRef = NULL;
static le_audio_StreamRef_t usbTxAudioRef = NULL;

static le_audio_ConnectorRef_t audioInputConnectorRef = NULL;
static le_audio_ConnectorRef_t audioOutputConnectorRef = NULL;



static void ConnectAudioToUsb
(
    le_mcc_call_Ref_t callRef
)
{
    le_result_t res;

    mdmRxAudioRef = le_mcc_call_GetRxAudioStream(callRef);
    LE_ERROR_IF((mdmRxAudioRef==NULL), "GetRxAudioStream returns NULL!");
    mdmTxAudioRef = le_mcc_call_GetTxAudioStream(callRef);
    LE_ERROR_IF((mdmTxAudioRef==NULL), "GetTxAudioStream returns NULL!");

    // Redirect audio to the USB.
    usbTxAudioRef = le_audio_OpenUsbTx();
    LE_ERROR_IF((usbTxAudioRef==NULL), "OpenUsbTx returns NULL!");
    usbRxAudioRef = le_audio_OpenUsbRx();
    LE_ERROR_IF((usbRxAudioRef==NULL), "OpenUsbRx returns NULL!");

    audioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((audioInputConnectorRef==NULL), "audioInputConnectorRef is NULL!");
    audioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((audioOutputConnectorRef==NULL), "audioOutputConnectorRef is NULL!");

    if (mdmRxAudioRef && mdmTxAudioRef && usbTxAudioRef && usbRxAudioRef &&
        audioInputConnectorRef && audioOutputConnectorRef)
    {
        res = le_audio_Connect(audioInputConnectorRef, usbRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect USB Rx on Input connector!");
        res = le_audio_Connect(audioInputConnectorRef, mdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(audioOutputConnectorRef, usbTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect USB Tx on Output connector!");
        res = le_audio_Connect(audioOutputConnectorRef, mdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
}


static void ConnectAudioToCodec
(
    le_mcc_call_Ref_t callRef
)
{
    le_result_t res;

    mdmRxAudioRef = le_mcc_call_GetRxAudioStream(callRef);
    LE_ERROR_IF((mdmRxAudioRef==NULL), "GetRxAudioStream returns NULL!");
    mdmTxAudioRef = le_mcc_call_GetTxAudioStream(callRef);
    LE_ERROR_IF((mdmTxAudioRef==NULL), "GetTxAudioStream returns NULL!");

#ifdef ENABLE_CODEC
    // Redirect audio to the in-built Microphone and Speaker.
    codecOutRef = le_audio_OpenSpeaker();
    LE_ERROR_IF((codecOutRef==NULL), "OpenSpeaker returns NULL!");
    codecInRef = le_audio_OpenMic();
    LE_ERROR_IF((codecInRef==NULL), "OpenMic returns NULL!");
#else
    // Redirect audio to the PCM interface.
    codecOutRef = le_audio_OpenPcmTx(0);
    LE_ERROR_IF((codecOutRef==NULL), "OpenSecondaryPcmTx returns NULL!");
    codecInRef = le_audio_OpenPcmRx(0);
    LE_ERROR_IF((codecInRef==NULL), "OpenSecondaryPcmRx returns NULL!");
#endif

    audioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((audioInputConnectorRef==NULL), "audioInputConnectorRef is NULL!");
    audioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((audioOutputConnectorRef==NULL), "audioOutputConnectorRef is NULL!");

    if (mdmRxAudioRef && mdmTxAudioRef && codecOutRef && codecInRef &&
        audioInputConnectorRef && audioOutputConnectorRef)
    {
        res = le_audio_Connect(audioInputConnectorRef, codecInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect Mic on Input connector!");
        res = le_audio_Connect(audioInputConnectorRef, mdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(audioOutputConnectorRef, codecOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect Speaker on Output connector!");
        res = le_audio_Connect(audioOutputConnectorRef, mdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
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
        if (codecInRef)
        {
            le_audio_Disconnect(audioInputConnectorRef, codecInRef);
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
        if(codecOutRef)
        {
            le_audio_Disconnect(audioOutputConnectorRef, codecOutRef);
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
    if(codecOutRef)
    {
        le_audio_Close(codecOutRef);
        codecOutRef = NULL;
    }
    if(codecInRef)
    {
        le_audio_Close(codecInRef);
        codecInRef = NULL;
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


//--------------------------------------------------------------------------------------------------
/**
 * This function gets the output for sound from the User.
 *
 */
//--------------------------------------------------------------------------------------------------
static int32_t getOutputSound(void)
{
    char *strPtr;

    do
    {
#ifdef ENABLE_CODEC
        fprintf(stderr, "AR7 platform, please enter the sound path or 'stop' to exit: \n");
        fprintf(stderr, " - MIC (for mic/speaker) \n");
#else
        fprintf(stderr, "WP7 platform, please enter the sound path or 'stop' to exit: \n");
        fprintf(stderr, " - PCM (for devkit's codec use, execute 'wm8940_demo --pcm' command) \n");
#endif
        fprintf(stderr, " - USB (for USB) \n");
        strPtr=fgets ((char*)TYPE_TEST_PATTERN, 16, stdin);
    }while (strlen(strPtr) == 0);


    TYPE_TEST_PATTERN[strlen(TYPE_TEST_PATTERN)-1]='\0';

    if (!strcmp(TYPE_TEST_PATTERN, "stop"))
    {
        return -1;
    }
    else
    {
        return 0;
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
        LE_INFO("Call event is LE_MCC_EVENT_CONNECTED.");

#ifdef ENABLE_CODEC
        if (strcmp(TYPE_TEST_PATTERN,"MIC")==0)
        {
            LE_INFO("Connect MIC and SPEAKER ");
#else
        if (strcmp(TYPE_TEST_PATTERN,"PCM")==0)
        {
            LE_INFO("Connect PCM ");
#endif
            ConnectAudioToCodec(callRef);
        }
        else if (strcmp(TYPE_TEST_PATTERN,"USB")==0)
        {
            LE_INFO("Connect USB ");
            ConnectAudioToUsb(callRef);
        }
        else
        {
            LE_INFO("Error in format could not connect audio");
        }
    }
    else if (callEvent == LE_MCC_EVENT_TERMINATED)
    {
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

        DisconnectAllAudio(callRef);

        le_mcc_call_Delete(callRef);
    }
    else if (callEvent == LE_MCC_EVENT_INCOMING)
    {
        LE_INFO("Call event is LE_MCC_EVENT_INCOMING.");
        res = le_mcc_call_Answer(callRef);
        if (res == LE_OK)
        {
#ifdef ENABLE_CODEC
            if (strcmp(TYPE_TEST_PATTERN,"MIC")==0)
            {
                LE_INFO("Connect MIC and SPEAKER ");
#else
            if (strcmp(TYPE_TEST_PATTERN,"PCM")==0)
            {
                LE_INFO("Connect PCM ");
#endif
                ConnectAudioToCodec(callRef);
            }
            else if (strcmp(TYPE_TEST_PATTERN,"USB")==0)
            {
                LE_INFO("Connect USB ");
                ConnectAudioToUsb(callRef);
            }
            else
            {
                LE_INFO("Error in format could not connect audio");
            }
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


static void* TestAudioMccClientService(void* contextPtr)
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

            stopTest = getOutputSound();
            if (!stopTest)
            {
                le_mcc_call_Start(testCallRef);
            }
            else
            {
                LE_INFO("Exit Audio Test!");
                exit(0);
            }
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
    le_thread_Start(le_thread_Create("TestAudioMccClient", TestAudioMccClientService, NULL));
}

