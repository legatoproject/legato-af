/**
 *
 * This file contains the source code of the rtpLoopback executable from the streamMediaTest
 * application.
 *
 * It is shows how to use the streamMedia interface by creating a RTP session connected to Mic
 * and Speaker.
 *
 * This executable sends audio from the microphone to a local RTP loop, and plays it to the speaker.
 * Usage :
 *     app runProc streamMediaTest --exe=rtpLoopback
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

// -------------------------------------------------------------------------------------------------
/**
 *  Local RTP port.
 */
// ------------------------------------------------------------------------------------------------
#define LOCAL_PORT 4000

// -------------------------------------------------------------------------------------------------
/**
 *  Local IP address.
 */
// ------------------------------------------------------------------------------------------------
#define LOCAL_ADDRESS "127.0.0.1"

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

static le_audio_StreamRef_t            RtpInRef;
static le_audio_StreamRef_t            RtpOutRef;
static le_audio_StreamRef_t            SpeakerAudioRef;
static le_audio_StreamRef_t            MicAudioRef;
static le_audio_ConnectorRef_t         AudioInputConnectorRef;
static le_audio_ConnectorRef_t         AudioOutputConnectorRef;

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
    LE_DEBUG("Disconnecting audio");

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

    exit(EXIT_SUCCESS);
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
    DisconnectAllAudio();
    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the rtpLoopback component.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    int         localPort = LOCAL_PORT;
    const char* remoteAddress = LOCAL_ADDRESS;
    le_result_t res;

    // Register a signal event handler for SIGTERM when the app stops
    le_sig_Block(SIGTERM);
    le_sig_SetEventHandler(SIGTERM, SigHandler);

    SpeakerAudioRef = le_audio_OpenSpeaker();
    LE_ERROR_IF((SpeakerAudioRef==NULL), "OpenSpeaker returns NULL!");
    MicAudioRef = le_audio_OpenMic();
    LE_ERROR_IF((MicAudioRef==NULL), "OpenMic returns NULL!");

    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef==NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef==NULL), "AudioOutputConnectorRef is NULL!");

    res = le_audio_Connect(AudioInputConnectorRef, MicAudioRef);
    LE_ERROR_IF((res!=LE_OK), "Failed to connect Mic on Input connector!");
    res = le_audio_Connect(AudioOutputConnectorRef, SpeakerAudioRef);
    LE_ERROR_IF((res!=LE_OK), "Failed to connect Speaker on Output connector!");

    RtpInRef = streamMedia_OpenAudioRtpRx(localPort);
    LE_ERROR_IF((RtpInRef==NULL), "RtpInRef returns NULL!");
    res = le_audio_Connect(AudioOutputConnectorRef, RtpInRef);
    LE_ERROR_IF((res!=LE_OK), "Failed to connect RtpInRef on Output connector!");
    res = streamMedia_Start(RtpInRef);
    LE_ERROR_IF((res!=LE_OK), "Failed to start RTP reception");

    RtpOutRef = streamMedia_OpenAudioRtpTx(localPort, remoteAddress, localPort);
    LE_ERROR_IF((RtpOutRef==NULL), "RtpOutRef returns NULL!");
    res = le_audio_Connect(AudioInputConnectorRef, RtpOutRef);
    LE_ERROR_IF((res!=LE_OK), "Failed to connect RtpOutRef on Input connector!");
    res = streamMedia_Start(RtpOutRef);
    LE_ERROR_IF((res!=LE_OK), "Failed to start RTP transmission");
}
