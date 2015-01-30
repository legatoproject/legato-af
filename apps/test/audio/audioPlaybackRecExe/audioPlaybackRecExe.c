/**
 * This module is for unit of the audio playback/recorder
 *
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include <stdio.h>
#include <pthread.h>


#include "interfaces.h"


static le_audio_StreamRef_t MdmRxAudioRef = NULL;
static le_audio_StreamRef_t MdmTxAudioRef = NULL;
static le_audio_StreamRef_t FeInRef = NULL;
static le_audio_StreamRef_t FeOutRef = NULL;
static le_audio_StreamRef_t FileAudioRef = NULL;

static le_audio_ConnectorRef_t AudioInputConnectorRef = NULL;
static le_audio_ConnectorRef_t AudioOutputConnectorRef = NULL;

static char  AudioTestCase[16];
static char  MainAudioSoundPath[16];
static char  AudioFilePath[129];
static int   AudioFileFd = -1;


//--------------------------------------------------------------------------------------------------
/**
 * Bindings functions.
 *
 */
//--------------------------------------------------------------------------------------------------
#define SERVICE_BASE_BINDINGS_CFG  "/users/root/bindings"

typedef void (*LegatoServiceInit_t)(void);

typedef struct {
    const char * appNamePtr;
    const char * serviceNamePtr;
    LegatoServiceInit_t serviceInitPtr;
} ServiceInitEntry_t;

#define SERVICE_ENTRY(aPP, sVC) { aPP, #sVC, sVC##_ConnectService },

const ServiceInitEntry_t ServiceInitEntries[] = {
    SERVICE_ENTRY("audioService", le_audio)
};

static void SetupBindings(void)
{
    int serviceIndex;
    char cfgPath[512];
    le_cfg_IteratorRef_t iteratorRef;

    /* Setup bindings */
    for(serviceIndex = 0; serviceIndex < NUM_ARRAY_MEMBERS(ServiceInitEntries); serviceIndex++ )
    {
        const ServiceInitEntry_t * entryPtr = &ServiceInitEntries[serviceIndex];

        /* Update binding in config tree */
        LE_INFO("-> Bind %s", entryPtr->serviceNamePtr);

        snprintf(cfgPath, sizeof(cfgPath), SERVICE_BASE_BINDINGS_CFG "/%s", entryPtr->serviceNamePtr);

        iteratorRef = le_cfg_CreateWriteTxn(cfgPath);

        le_cfg_SetString(iteratorRef, "app", entryPtr->appNamePtr);
        le_cfg_SetString(iteratorRef, "interface", entryPtr->serviceNamePtr);

        le_cfg_CommitTxn(iteratorRef);
    }

    /* Tel legato to reload its bindings */
    system("sdir load");
}

static void ConnectServices(void)
{
    int serviceIndex;

    /* Init services */
    for(serviceIndex = 0; serviceIndex < NUM_ARRAY_MEMBERS(ServiceInitEntries); serviceIndex++ )
    {
        const ServiceInitEntry_t * entryPtr = &ServiceInitEntries[serviceIndex];

        LE_INFO("-> Init %s", entryPtr->serviceNamePtr);
        entryPtr->serviceInitPtr();
    }

    LE_INFO("All services bound!");
}

static void ConnectAudioToUsb
(
    le_mcc_call_ObjRef_t callRef
)
{
    le_result_t res;

    // Redirect audio to the USB.
    FeOutRef = le_audio_OpenUsbTx();
    LE_ERROR_IF((FeOutRef==NULL), "OpenUsbTx returns NULL!");
    FeInRef = le_audio_OpenUsbRx();
    LE_ERROR_IF((FeInRef==NULL), "OpenUsbRx returns NULL!");

    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef==NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef==NULL), "AudioOutputConnectorRef is NULL!");
    {
        res = le_audio_Connect(AudioInputConnectorRef, FeInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect USB Rx on Input connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect USB Tx on Output connector!");
    }
}

static void ConnectAudioToFileLocalPlay
(
    le_mcc_call_ObjRef_t callRef
)
{
    le_result_t res;
    if ((AudioFileFd=open(AudioFilePath, O_RDONLY)) == -1)
    {
        LE_ERROR("Open file %s failure: errno.%d (%s)",  AudioFilePath, errno, strerror(errno));
    }
    else
    {
        LE_INFO("Open file %s with AudioFileFd.%d",  AudioFilePath, AudioFileFd);
    }

    // Play local on output connector.
    FileAudioRef = le_audio_OpenFilePlayback(AudioFileFd);
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFilePlayback returns NULL!");

    if (FileAudioRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioOutputConnectorRef, FileAudioRef);
        if(res!=LE_OK)
        {
            LE_ERROR("Failed to connect FilePlayback on output connector!");
        }
        else
        {
            LE_INFO("FilePlayback is now connected.");
        }
    }
}

static void ConnectAudioToFileLocalRec
(
    le_mcc_call_ObjRef_t callRef
)
{
    le_result_t res;
    if ((AudioFileFd=open(AudioFilePath, O_WRONLY | O_CREAT | O_TRUNC)) == -1)
    {
        LE_ERROR("Open file %s failure: errno.%d (%s)",  AudioFilePath, errno, strerror(errno));
    }
    else
    {
        LE_INFO("Open file %s with AudioFileFd.%d",  AudioFilePath, AudioFileFd);
    }

    // Capture local on input connector.
    FileAudioRef = le_audio_OpenFileRecording(AudioFileFd);
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFileRecording returns NULL!");

    if (FileAudioRef && AudioInputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FileAudioRef);
        if(res!=LE_OK)
        {
            LE_ERROR("Failed to connect FileRecording on input connector!");
        }
        else
        {
            LE_INFO("FileRecording is now connected.");
        }
    }
}

#ifdef ENABLE_CODEC
static void ConnectAudioToCodec
(
    le_mcc_call_ObjRef_t callRef
)
{
    le_result_t res;

    // Redirect audio to the in-built Microphone and Speaker.
    FeOutRef = le_audio_OpenSpeaker();
    LE_ERROR_IF((FeOutRef==NULL), "OpenSpeaker returns NULL!");
    FeInRef = le_audio_OpenMic();
    LE_ERROR_IF((FeInRef==NULL), "OpenMic returns NULL!");

    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef==NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef==NULL), "AudioOutputConnectorRef is NULL!");

    {
        res = le_audio_Connect(AudioInputConnectorRef, FeInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect Mic on Input connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect Speaker on Output connector!");
    }
}
#endif

static void ConnectAudioToPcm
(
    le_mcc_call_ObjRef_t callRef
)
{
    le_result_t res;


    // Redirect audio to the PCM interface.
    FeOutRef = le_audio_OpenPcmTx(0);
    LE_ERROR_IF((FeOutRef==NULL), "OpenPcmTx returns NULL!");
    FeInRef = le_audio_OpenPcmRx(0);
    LE_ERROR_IF((FeInRef==NULL), "OpenPcmRx returns NULL!");

    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef==NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef==NULL), "AudioOutputConnectorRef is NULL!");
    {
        res = le_audio_Connect(AudioInputConnectorRef, FeInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect PCM RX on Input connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect PCM TX on Output connector!");
    }
}

static void ConnectAudioToI2s
(
    le_mcc_call_ObjRef_t callRef
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
    {
        res = le_audio_Connect(AudioInputConnectorRef, FeInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect I2S RX on Input connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect I2S TX on Output connector!");
    }
    LE_INFO("Open I2s: FeInRef.%p FeOutRef.%p", FeInRef, FeOutRef);
}


static void ConnectAudio
(
    le_mcc_call_ObjRef_t callRef
)
{

#ifdef ENABLE_CODEC
    if (strcmp(AudioTestCase,"MIC")==0)
    {
        LE_INFO("Connect MIC and SPEAKER ");
        ConnectAudioToCodec(callRef);
    }
    else
#endif
    if (strcmp(AudioTestCase,"PCM")==0)
    {
        LE_INFO("Connect PCM ");
        ConnectAudioToPcm(callRef);
    }
    else if (strcmp(AudioTestCase,"I2S")==0)
    {
        LE_INFO("Connect I2S");
        ConnectAudioToI2s(callRef);
    }
    else if (strcmp(AudioTestCase,"USB")==0)
    {
        LE_INFO("Connect USB ");
        ConnectAudioToUsb(callRef);
    }
    else if ((strncmp(AudioTestCase,"PB",2)==0) || (strncmp(AudioTestCase,"REC",3)==0))
    {
#ifdef ENABLE_CODEC
        if (strcmp(MainAudioSoundPath,"MIC")==0)
        {
            LE_INFO("Connect MIC and SPEAKER ");
            ConnectAudioToCodec(callRef);
        }
        else
#endif
        if (strcmp(MainAudioSoundPath,"PCM")==0)
        {
            LE_INFO("Connect PCM ");
            ConnectAudioToPcm(callRef);
        }
        else if (strcmp(MainAudioSoundPath,"I2S")==0)
        {
            LE_INFO("Connect I2S");
            ConnectAudioToI2s(callRef);
        }
        else if (strcmp(MainAudioSoundPath,"USB")==0)
        {
            LE_INFO("Connect USB ");
            ConnectAudioToUsb(callRef);
        }
        else
        {
            LE_INFO("Error in format could not connect audio");
        }

        // Connect SW-PCM
        if (strcmp(AudioTestCase,"PB")==0)
        {
            LE_INFO("Connect Local Play");
            ConnectAudioToFileLocalPlay(callRef);
        }
        else if (strcmp(AudioTestCase,"REC")==0)
        {
            LE_INFO("Connect Local Rec ");
            ConnectAudioToFileLocalRec(callRef);
        }
        else
        {
            LE_INFO("Error in format could not connect audio");
        }
    }
    else if (strcmp(AudioTestCase,"NONE")==0)
    {
        LE_INFO("NO audio connection ");
    }
    else
    {
        LE_INFO("Error in format could not connect audio");
    }
}

static void DisconnectAllAudio
(
    le_mcc_call_ObjRef_t callRef
)
{
    if (AudioInputConnectorRef)
    {
        if(FileAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", FileAudioRef, AudioInputConnectorRef);
            le_audio_Disconnect(AudioInputConnectorRef, FileAudioRef);
        }
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
        if(FileAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", FileAudioRef, AudioOutputConnectorRef);
            le_audio_Disconnect(AudioOutputConnectorRef, FileAudioRef);
        }
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

    if(FileAudioRef)
    {
        le_audio_Close(FileAudioRef);
        FeOutRef = NULL;
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

    close(AudioFileFd);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the audio file's name from the User.
 *
 */
//--------------------------------------------------------------------------------------------------
static int32_t GetAudioFileName(void)
{
    char *strPtr;

    memset(AudioFilePath, 0, 129);

    do
    {
        fprintf(stderr, "Please enter the file's name for audio playback/recording or 'stop' to exit: \n");
        strPtr=fgets ((char*)AudioFilePath, 129, stdin);
    }while (strlen(strPtr) == 0);

    AudioFilePath[strlen(AudioFilePath)-1]='\0';

    if (!strcmp(AudioFilePath, "stop"))
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
 * This function gets the audio Interface choice from the User.
 *
 */
//--------------------------------------------------------------------------------------------------
static int32_t GetAudioTestCaseChoice(void)
{
    char *strPtr;

    memset(AudioTestCase, 0, 16);

    do
    {
        fprintf(stderr, " - PB (for playback) \n");
        fprintf(stderr, " - REC (for recording) \n");
        strPtr=fgets ((char*)AudioTestCase, 16, stdin);
    } while (strlen(strPtr) == 0);
    AudioTestCase[strlen(AudioTestCase)-1]='\0';

    if (!strcmp(AudioTestCase, "stop"))
    {
        return -1;
    }
    else if ((!strncmp(AudioTestCase, "PB", 2)) || (!strncmp(AudioTestCase, "REC", 3)))
    {
        if (GetAudioFileName())
        {
            return -1;
        }
        memset(MainAudioSoundPath, 0, 16);

        do
        {
#ifdef ENABLE_CODEC
            fprintf(stderr, "AR7 platform, please choose the main audio path or digit 'stop' to exit: \n");
            fprintf(stderr, " - MIC (for mic/speaker) \n");
#else
            fprintf(stderr, "WP7 platform, please choose the main audio path or digit 'stop' to exit: \n");
#endif
            fprintf(stderr, " - PCM (for devkit's codec use, execute 'wm8940_demo --pcm' command) \n");
            fprintf(stderr, " - I2S (for devkit's codec use, execute 'wm8940_demo --i2s' command) \n");
            fprintf(stderr, " - USB (for USB) \n");
            strPtr=fgets ((char*)MainAudioSoundPath, 16, stdin);
        } while (strlen(strPtr) == 0);
        MainAudioSoundPath[strlen(MainAudioSoundPath)-1]='\0';

        if (!strcmp(MainAudioSoundPath, "stop"))
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return -1;
    }
}


COMPONENT_INIT
{
    int32_t                 stopTest = 0;

    LE_INFO("Init");


    SetupBindings();
    ConnectServices();


    while(stopTest <= 0)
    {
        stopTest = GetAudioTestCaseChoice();

        if (!stopTest)
        {
            ConnectAudio(NULL);
        }
        else
        {
            LE_INFO("Exit Audio Test!");

            // ADO
            DisconnectAllAudio(NULL);

            exit(0);
        }
    }
}

