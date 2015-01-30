/**
 * This module is for unit testing of the voice Call service component as a client of voiceCallService.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include <stdio.h>
#include <pthread.h>

#include "interfaces.h"

#define     DESTINATION_LEN_MAX     50


#define     TEST_STOP               0
#define     TEST_EXIT               1
#define     TEST_REQ                2
#define     TEST_ANSWER             3


static le_voicecall_StateHandlerRef_t VoiceCallHandlerRef;

static le_voicecall_CallRef_t RequestCallRef;

static char Destination[DESTINATION_LEN_MAX];

#define EVENT_STRING_LEN        150
#define REASON_STRING_LEN       50



//--------------------------------------------------------------------------------------------------
/**
 * Audio safe references
 */
//--------------------------------------------------------------------------------------------------
static le_audio_StreamRef_t MdmRxAudioRef = NULL;
static le_audio_StreamRef_t MdmTxAudioRef = NULL;
static le_audio_StreamRef_t FeInRef = NULL;
static le_audio_StreamRef_t FeOutRef = NULL;

static le_audio_ConnectorRef_t AudioInputConnectorRef = NULL;
static le_audio_ConnectorRef_t AudioOutputConnectorRef = NULL;




static void ConnectAudioToI2s
(
)
{
    le_result_t res;

    // Redirect audio to the I2S interface.
    FeOutRef = le_audio_OpenI2sTx(LE_AUDIO_I2S_STEREO);
    LE_ERROR_IF((FeOutRef==NULL), "OpenI2sTx returns NULL!");
    FeInRef = le_audio_OpenI2sRx(LE_AUDIO_I2S_STEREO);
    LE_ERROR_IF((FeInRef==NULL), "OpenI2sRx returns NULL!");

    LE_INFO("Open I2s: FeInRef.%p FeOutRef.%p", FeInRef, FeOutRef);

    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef==NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef==NULL), "AudioOutputConnectorRef is NULL!");

    if (MdmRxAudioRef && MdmTxAudioRef && FeOutRef && FeInRef &&
        AudioInputConnectorRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FeInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect I2S RX on Input connector!");
        res = le_audio_Connect(AudioInputConnectorRef, MdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect I2S TX on Output connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, MdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
    LE_INFO("Open I2s: FeInRef.%p FeOutRef.%p", FeInRef, FeOutRef);
}

static void DisconnectAllAudio
(
    le_voicecall_CallRef_t reference
)
{
    LE_DEBUG("DisconnectAllAudio");

    MdmRxAudioRef = le_voicecall_GetRxAudioStream(reference);
    LE_ERROR_IF((MdmRxAudioRef==NULL), "le_voicecall_GetRxAudioStream returns NULL!");

    MdmTxAudioRef = le_voicecall_GetTxAudioStream(reference);
    LE_ERROR_IF((MdmTxAudioRef==NULL), "le_voicecall_GetRxAudioStream returns NULL!");

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



static le_result_t OpenAudio
(
    le_voicecall_CallRef_t reference
)
{
    MdmRxAudioRef = le_voicecall_GetRxAudioStream(reference);
    LE_ERROR_IF((MdmRxAudioRef==NULL), "le_voicecall_GetRxAudioStream returns NULL!");

    MdmTxAudioRef = le_voicecall_GetTxAudioStream(reference);
    LE_ERROR_IF((MdmTxAudioRef==NULL), "le_voicecall_GetRxAudioStream returns NULL!");

    LE_DEBUG("OpenAudio MdmRxAudioRef %p, MdmTxAudioRef %p", MdmRxAudioRef, MdmTxAudioRef);

    LE_INFO("Connect I2S");
    ConnectAudioToI2s();

    return LE_OK;
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
    le_voicecall_Event_t event,
    void* contextPtr
)
{
    LE_INFO("MyCallEventHandler DestNumber=> %s", identifier);
    char eventString[EVENT_STRING_LEN];

    switch(event)
    {
        case LE_VOICECALL_EVENT_CONNECTED:
        {
            snprintf(eventString, EVENT_STRING_LEN, "%s", "LE_VOICECALL_EVENT_CONNECTED");
            OpenAudio(reference);
        }
        break;
        case LE_VOICECALL_EVENT_ALERTING:
        {
            snprintf(eventString, EVENT_STRING_LEN, "%s", "LE_VOICECALL_EVENT_ALERTING");
        }
        break;
        case LE_VOICECALL_EVENT_BUSY:
        {
            snprintf(eventString, EVENT_STRING_LEN, "%s", "LE_VOICECALL_EVENT_BUSY");
        }
        break;
        case LE_VOICECALL_EVENT_INCOMING:
        {
            snprintf(eventString, EVENT_STRING_LEN, "%s", "LE_VOICECALL_EVENT_INCOMING");
            RequestCallRef = reference;
        }
        break;
        case LE_VOICECALL_EVENT_OFFLINE:
        {
            snprintf(eventString, EVENT_STRING_LEN, "%s", "LE_VOICECALL_EVENT_OFFLINE");
        }
        break;
        case LE_VOICECALL_EVENT_RESOURCE_BUSY:
        {
            snprintf(eventString, EVENT_STRING_LEN, "%s", "LE_VOICECALL_EVENT_RESOURCE_BUSY");
        }
        break;
        case LE_VOICECALL_EVENT_TERMINATED:
        {
            le_voicecall_TerminationReason_t reason;
            le_result_t result;

            LE_DEBUG("LE_VOICECALL_EVENT_TERMINATED audio Disconnecting");
            DisconnectAllAudio(reference);

            result = le_voicecall_GetTerminationReason(reference, &reason);
            if (result == LE_OK)
            {
                char reasonstring[REASON_STRING_LEN];
                switch(reason)
                {
                    case LE_VOICECALL_TERM_BAD_ADDRESS:
                    {
                        snprintf(reasonstring, REASON_STRING_LEN, "%s", "LE_VOICECALL_TERM_BAD_ADDRESS");
                    }
                    break;
                    case LE_VOICECALL_TERM_BUSY:
                    {
                        snprintf(reasonstring, REASON_STRING_LEN, "%s", "LE_VOICECALL_TERM_BUSY");
                    }
                    break;
                    case LE_VOICECALL_TERM_LOCAL_ENDED:
                    {
                        snprintf(reasonstring, REASON_STRING_LEN, "%s", "LE_VOICECALL_TERM_LOCAL_ENDED");
                    }
                    break;
                    case LE_VOICECALL_TERM_NETWORK_FAIL:
                    {
                        snprintf(reasonstring, REASON_STRING_LEN, "%s", "LE_VOICECALL_TERM_NETWORK_FAIL");
                    }
                    break;
                    case LE_VOICECALL_TERM_REMOTE_ENDED:
                    {
                        snprintf(reasonstring, REASON_STRING_LEN, "%s", "LE_VOICECALL_TERM_REMOTE_ENDED");
                    }
                    break;
                    case LE_VOICECALL_TERM_NOT_DEFINED:
                    {
                        snprintf(reasonstring, REASON_STRING_LEN, "%s", "LE_VOICECALL_TERM_NOT_DEFINED");
                    }
                    break;
                    default:
                    {
                        snprintf(reasonstring, REASON_STRING_LEN, "%s", "reason not defined");
                    }
                    break;

                        break;
                }
                snprintf(eventString, EVENT_STRING_LEN, "%s reason =%d => %s", "LE_VOICECALL_EVENT_TERMINATED", reason, reasonstring);
            }
            else
            {
                snprintf(eventString, EVENT_STRING_LEN, "%s reason not found", "LE_VOICECALL_EVENT_TERMINATED");
            }
            le_voicecall_Delete(reference);
        }
        break;
        default:
        {
            snprintf(eventString,EVENT_STRING_LEN,"udefined");
            LE_INFO("Unknown event");
        }
        break;
    }

    fprintf(stderr,"\n=>Destination %s, Event %d => %s\n\n", identifier ,event, eventString);
    LE_INFO("MyCallEventHandler Event state %d, %s", event,eventString);

    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function gets destination number.
 *
 */
//--------------------------------------------------------------------------------------------------
static int32_t GetTel(void)
{
    char *strPtr;

    fprintf(stderr, "Audio path is sent to I2S (for devkit's codec use, execute 'wm8940_demo --i2s' command) \n");

    do
    {
        // Get command or destination number.
        fprintf(stderr, "Set Destination Number or command: stop (hang-up), answer (pick-up) or exit to exit of application\n");
        strPtr=fgets ((char*)Destination, DESTINATION_LEN_MAX, stdin);
    } while (strlen(strPtr) == 0);

    Destination[strlen(Destination)-1]='\0';
    LE_INFO("Get string => %s",Destination);

    if (!strcmp(Destination, "stop"))
    {
        return TEST_STOP;
    }
    else if (!strcmp(Destination, "exit"))
    {
        return TEST_EXIT;
    }
    else if (!strcmp(Destination, "answer"))
    {
        return TEST_ANSWER;
    }
    else
    {
        return TEST_REQ;
    }
}

static void* HandlerThread(void* contextPtr)
{

    // Connect to the services required by this thread
    le_voicecall_ConnectService();
    le_audio_ConnectService();

    // Add voice call event handler function.
    VoiceCallHandlerRef = le_voicecall_AddStateHandler(MyCallEventHandler, contextPtr );

    le_event_RunLoop();
    return NULL;
}


COMPONENT_INIT
{
    int32_t stopTest = TEST_REQ;

    LE_INFO("VoicecallTest Started");

    // Start the handler thread to monitor the voice call.
    le_thread_Start(le_thread_Create("VoiceCallTest", HandlerThread, NULL));

    while(TEST_EXIT != stopTest)
    {
        stopTest = GetTel();

        switch (stopTest)
        {
            case TEST_STOP:
            {
                LE_INFO("Stop in progress....");
                le_voicecall_End(RequestCallRef);
            }
            break;

            case TEST_ANSWER:
            {
                LE_INFO("Answer to incoming call...");
                le_voicecall_Answer(RequestCallRef);

            }
            break;

            case TEST_REQ:
            {
                LE_INFO("Start a new voice call...");
                RequestCallRef = le_voicecall_Start(Destination);
            }
            break;

            default:
            case TEST_EXIT:
            {
                LE_INFO("Exit in progress....");
                le_voicecall_RemoveStateHandler(VoiceCallHandlerRef);
            }
            break;
        }
    }

    LE_INFO("Exit VoiceCallTest Test!");
    exit(0);
}

