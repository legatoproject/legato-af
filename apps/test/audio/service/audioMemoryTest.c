/**
 * This module is for memory unit testing of the Audio service component.
 *
 *
 * Copyright (C) Sierra Wireless, Inc. 2013.  All rights reserved. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include <stdio.h>
#include <pthread.h>

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

static void ConnectAudioToMicAndSpeaker
(
    void
)
{
    mdmRxAudioRef = le_audio_OpenModemVoiceRx();
    mdmTxAudioRef = le_audio_OpenModemVoiceTx();

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

static void ConnectAudioToUsbInOut
(
    void
)
{
    mdmRxAudioRef = le_audio_OpenModemVoiceRx();
    mdmTxAudioRef = le_audio_OpenModemVoiceTx();

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

static void ConnectAudioToMicAndSpeakerAndUsbOut
(
    void
)
{
    mdmRxAudioRef = le_audio_OpenModemVoiceRx();
    mdmTxAudioRef = le_audio_OpenModemVoiceTx();

    micRef = le_audio_OpenMic();
    speakerRef = le_audio_OpenSpeaker();
    usbTxAudioRef = le_audio_OpenUsbTx();

    audioInputConnectorRef = le_audio_CreateConnector();
    audioOutputConnectorRef = le_audio_CreateConnector();

    if (mdmRxAudioRef && mdmTxAudioRef && speakerRef && micRef && usbTxAudioRef &&
        audioInputConnectorRef && audioOutputConnectorRef)
    {
        le_audio_Connect(audioInputConnectorRef, micRef);
        le_audio_Connect(audioInputConnectorRef, mdmTxAudioRef);
        le_audio_Connect(audioOutputConnectorRef, mdmRxAudioRef);
        le_audio_Connect(audioOutputConnectorRef, speakerRef);
        le_audio_Connect(audioOutputConnectorRef, usbTxAudioRef);
    }
}

static void ConnectAudioToMicAndUsbInAndSpeaker
(
    void
)
{
    mdmRxAudioRef = le_audio_OpenModemVoiceRx();
    mdmTxAudioRef = le_audio_OpenModemVoiceTx();

    micRef = le_audio_OpenMic();
    speakerRef = le_audio_OpenSpeaker();
    usbRxAudioRef = le_audio_OpenUsbRx();

    audioInputConnectorRef = le_audio_CreateConnector();
    audioOutputConnectorRef = le_audio_CreateConnector();

    if (mdmRxAudioRef && mdmTxAudioRef && speakerRef && micRef && usbRxAudioRef &&
        audioInputConnectorRef && audioOutputConnectorRef)
    {
        le_audio_Connect(audioInputConnectorRef, mdmTxAudioRef);
        le_audio_Connect(audioInputConnectorRef, micRef);
        le_audio_Connect(audioInputConnectorRef, usbRxAudioRef);
        le_audio_Connect(audioOutputConnectorRef, mdmRxAudioRef);
        le_audio_Connect(audioOutputConnectorRef, speakerRef);
    }
}

static void ConnectAudioToAll
(
    void
)
{
    mdmRxAudioRef = le_audio_OpenModemVoiceRx();
    mdmTxAudioRef = le_audio_OpenModemVoiceTx();

    micRef = le_audio_OpenMic();
    speakerRef = le_audio_OpenSpeaker();
    usbTxAudioRef = le_audio_OpenUsbTx();
    usbRxAudioRef = le_audio_OpenUsbRx();

    audioInputConnectorRef = le_audio_CreateConnector();
    audioOutputConnectorRef = le_audio_CreateConnector();

    if (mdmRxAudioRef && mdmTxAudioRef && speakerRef && micRef &&
        usbRxAudioRef && usbTxAudioRef &&
        audioInputConnectorRef && audioOutputConnectorRef)
    {
        le_audio_Connect(audioInputConnectorRef, mdmTxAudioRef);
        le_audio_Connect(audioInputConnectorRef, micRef);
        le_audio_Connect(audioInputConnectorRef, usbRxAudioRef);
        le_audio_Connect(audioOutputConnectorRef, mdmRxAudioRef);
        le_audio_Connect(audioOutputConnectorRef, speakerRef);
        le_audio_Connect(audioOutputConnectorRef, usbTxAudioRef);
    }
}

static void DisconnectAllAudio
(
    void
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


static void TestMemoryAudioService
(
    void
)
{
    ConnectAudioToMicAndSpeaker();
    DisconnectAllAudio();

    ConnectAudioToUsbInOut();
    DisconnectAllAudio();

    ConnectAudioToMicAndSpeakerAndUsbOut();
    DisconnectAllAudio();

    ConnectAudioToMicAndUsbInAndSpeaker();
    DisconnectAllAudio();

    ConnectAudioToAll();
    DisconnectAllAudio();
}


COMPONENT_INIT
{
    if (le_arg_NumArgs() == 1) {

        char valuePtr[10];

        le_arg_GetArg(0,valuePtr,10);

        int counter,i;

        le_audio_Init();

        counter = atoi(valuePtr);

        if ( counter < 0)
        {
            exit(EXIT_FAILURE);
        }

        for(i=0;i<counter;i++) {
            fprintf(stderr, "Test [%d] START\n",i+1);
            TestMemoryAudioService();
            fprintf(stderr, "Test [%d] DONE\n",i+1);
        }

        exit(EXIT_SUCCESS);
    } else {

        char progName[64];

        le_arg_GetProgramName(progName,64,NULL);

        fprintf(stderr,"%s Usage:\n",progName);
        fprintf(stderr,"\t %s NUMBER\n\n",progName);
        fprintf(stderr,"NUMBER corresponds to the number of time the memory test will be run.\n");

        exit(EXIT_FAILURE);
    }
}

