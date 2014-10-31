/**
 * This module is for unit testing of the MCC service component as a client of MS daemon.
 *
 * New commands added to manage Calling Line Identification Restriction (CLIR) status on calls.
 *  - clir_on : Set CLIR status to LE_ON for next calls.
 *      Disable presentation of your target's phone number to remote.
 *  - clir_off : Set CLIR status to LE_OFF for next calls.
 *      Enable presentation of your target's phone number to remote.
 *  - clir_default : No CLIR status set (Default value).
 *
 * Copyright (C) Sierra Wireless, Inc. 2013-2014. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include <stdio.h>
#include <pthread.h>


#include "interfaces.h"



static le_mcc_call_ObjRef_t TestCallRef;

static le_audio_StreamRef_t MdmRxAudioRef = NULL;
static le_audio_StreamRef_t MdmTxAudioRef = NULL;
static le_audio_StreamRef_t FeInRef = NULL;
static le_audio_StreamRef_t FeOutRef = NULL;
static le_audio_StreamRef_t FileAudioRef = NULL;

static le_audio_ConnectorRef_t AudioInputConnectorRef = NULL;
static le_audio_ConnectorRef_t AudioOutputConnectorRef = NULL;

static char  DestinationNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
static char  AudioTestCase[16];
static char  MainAudioSoundPath[16];
static char  AudioFilePath[129];
static int   AudioFileFd = -1;

typedef enum
{
    AUDIO_MCC_TEST_CLIR_DEFAULT,   ///< No CLIR status set (Default value).
    AUDIO_MCC_TEST_CLIR_ON,        ///< Set CLIR status to LE_ON.
    AUDIO_MCC_TEST_CLIR_OFF        ///< Set CLIR status to LE_OFF.
}
ClirStatus_t;

static ClirStatus_t CurrentCLIRStatus;

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
    SERVICE_ENTRY("modemService", le_mcc_profile)
    SERVICE_ENTRY("modemService", le_mcc_call)
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

    MdmRxAudioRef = le_mcc_call_GetRxAudioStream(callRef);
    LE_ERROR_IF((MdmRxAudioRef==NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_mcc_call_GetTxAudioStream(callRef);
    LE_ERROR_IF((MdmTxAudioRef==NULL), "GetTxAudioStream returns NULL!");

    // Redirect audio to the USB.
    FeOutRef = le_audio_OpenUsbTx();
    LE_ERROR_IF((FeOutRef==NULL), "OpenUsbTx returns NULL!");
    FeInRef = le_audio_OpenUsbRx();
    LE_ERROR_IF((FeInRef==NULL), "OpenUsbRx returns NULL!");

    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef==NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef==NULL), "AudioOutputConnectorRef is NULL!");

    if (MdmRxAudioRef && MdmTxAudioRef && FeOutRef && FeInRef &&
        AudioInputConnectorRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FeInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect USB Rx on Input connector!");
        res = le_audio_Connect(AudioInputConnectorRef, MdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect USB Tx on Output connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, MdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
}


static void ConnectAudioToFileRemotePlay
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

    // Play Remote on input connector.
    FileAudioRef = le_audio_OpenFilePlayback(AudioFileFd);
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFilePlayback returns NULL!");

    if (FileAudioRef && AudioInputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FileAudioRef);
        if(res!=LE_OK)
        {
            LE_ERROR("Failed to connect FilePlayback on input connector!");
        }
        else
        {
            LE_INFO("FilePlayback is now connected.");
        }
    }
}

static void ConnectAudioToFileRemoteRec
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

    // Capture Remote on output connector.
    FileAudioRef = le_audio_OpenFileRecording(AudioFileFd);
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFileRecording returns NULL!");

    if (FileAudioRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioOutputConnectorRef, FileAudioRef);
        if(res!=LE_OK)
        {
            LE_ERROR("Failed to connect FileRecording on output connector!");
        }
        else
        {
            LE_INFO("FileRecording is now connected.");
        }
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

    MdmRxAudioRef = le_mcc_call_GetRxAudioStream(callRef);
    LE_ERROR_IF((MdmRxAudioRef==NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_mcc_call_GetTxAudioStream(callRef);
    LE_ERROR_IF((MdmTxAudioRef==NULL), "GetTxAudioStream returns NULL!");

    // Redirect audio to the in-built Microphone and Speaker.
    FeOutRef = le_audio_OpenSpeaker();
    LE_ERROR_IF((FeOutRef==NULL), "OpenSpeaker returns NULL!");
    FeInRef = le_audio_OpenMic();
    LE_ERROR_IF((FeInRef==NULL), "OpenMic returns NULL!");

    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef==NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef==NULL), "AudioOutputConnectorRef is NULL!");

    if (MdmRxAudioRef && MdmTxAudioRef && FeOutRef && FeInRef &&
        AudioInputConnectorRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FeInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect Mic on Input connector!");
        res = le_audio_Connect(AudioInputConnectorRef, MdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect Speaker on Output connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, MdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
}
#endif

static void ConnectAudioToPcm
(
    le_mcc_call_ObjRef_t callRef
)
{
    le_result_t res;

    MdmRxAudioRef = le_mcc_call_GetRxAudioStream(callRef);
    LE_ERROR_IF((MdmRxAudioRef==NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_mcc_call_GetTxAudioStream(callRef);
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

static void ConnectAudioToI2s
(
    le_mcc_call_ObjRef_t callRef
)
{
    le_result_t res;

    MdmRxAudioRef = le_mcc_call_GetRxAudioStream(callRef);
    LE_ERROR_IF((MdmRxAudioRef==NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_mcc_call_GetTxAudioStream(callRef);
    LE_ERROR_IF((MdmTxAudioRef==NULL), "GetTxAudioStream returns NULL!");

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
    else if ((strncmp(AudioTestCase,"R-",2)==0) || (strncmp(AudioTestCase,"L-",2)==0))
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
        if (strcmp(AudioTestCase,"R-PB")==0)
        {
            LE_INFO("Connect Remote Play");
            ConnectAudioToFileRemotePlay(callRef);
        }
        else
        if (strcmp(AudioTestCase,"R-REC")==0)
        {
            LE_INFO("Connect Remote Rec ");
            ConnectAudioToFileRemoteRec(callRef);
        }
        else if (strcmp(AudioTestCase,"L-PB")==0)
        {
            LE_INFO("Connect Local Play");
            ConnectAudioToFileLocalPlay(callRef);
        }
        else if (strcmp(AudioTestCase,"L-REC")==0)
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
 * This function gets the telephone number from the User.
 *
 */
//--------------------------------------------------------------------------------------------------
static int32_t GetTel(void)
{
    char *strPtr;

    memset(DestinationNumber, 0, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);

    /**
     * New commands added to manage Calling Line Identification Restriction (CLIR) status on calls.
     *  - clir_on : Set CLIR status to LE_OFF for next calls.
     *  - clir_off : Set CLIR status to LE_ON for next calls.
     *  - clir_default : No CLIR status set (Default value).
     */
    do
    {
        fprintf(stderr, "Please enter a command:\n"
                        " - 'clir_on' : CLIR option will be set to LE_ON for next calls\n"
                        " - 'clir_off': CLIR option will be set to LE_OFF for next calls\n"
                        " - 'clir_default' : CLIR option will not be set for next calls\n"
                        " - 'stop' to exit\n"
                        " or enter the destination's telephone number to start a call:\n");
        strPtr=fgets ((char*)DestinationNumber, 16, stdin);
    }while (strlen(strPtr) == 0);

    DestinationNumber[strlen(DestinationNumber)-1]='\0';

    /* Command  'Stop': exit */
    if (!strcmp(DestinationNumber, "stop"))
    {
        return 1;
    }
    /**
     * Command 'clir_on' : Set CLIR status to LE_ON
     *  with le_mcc_call_SetCallerIdRestrict() API.
     */
    else if (!strcmp(DestinationNumber, "clir_on"))
    {
        CurrentCLIRStatus = AUDIO_MCC_TEST_CLIR_ON;
        fprintf(stderr, "CLIR will be set to activated on next calls\n\n");
        return -1;

    }
    /**
     * Command 'clir_off' : Set CLIR status to LE_OFF
     *  with le_mcc_call_SetCallerIdRestrict() API.
     */
    else if (!strcmp(DestinationNumber, "clir_off"))
    {
        CurrentCLIRStatus = AUDIO_MCC_TEST_CLIR_OFF;
        fprintf(stderr, "CLIR will be set to deactivated on next calls\n\n");
        return -1;
    }
    /**
     * Command 'clir_default' : Doesn't set CLIR status
     *  with le_mcc_call_SetCallerIdRestrict() API.
     *  Mcc default value is used.
     */
    else if (!strcmp(DestinationNumber, "clir_default"))
    {
        CurrentCLIRStatus = AUDIO_MCC_TEST_CLIR_DEFAULT;
        fprintf(stderr, "CLIR will not be set on next calls\n\n");
        return -1;
    }
    else
    {
        return 0;
    }
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
#ifdef ENABLE_CODEC
        fprintf(stderr, "AR7 platform, please choose the test case or digit 'stop' to exit: \n");
        fprintf(stderr, " - MIC (for mic/speaker) \n");
#else
        fprintf(stderr, "WP7 platform, please choose the test case or digit 'stop' to exit: \n");
#endif
        fprintf(stderr, " - PCM (for devkit's codec use, execute 'wm8940_demo --pcm' command) \n");
        fprintf(stderr, " - I2S (for devkit's codec use, execute 'wm8940_demo --i2s' command) \n");
        fprintf(stderr, " - USB (for USB) \n");
        fprintf(stderr, " - R-PB (for Remote playback) \n");
        fprintf(stderr, " - R-REC (for Remote recording) \n");
        fprintf(stderr, " - L-PB (for Local playback) \n");
        fprintf(stderr, " - L-REC (for Local recording) \n");
        fprintf(stderr, " - NONE (No pre-configured path, you must use 'amix' commands) \n");
        strPtr=fgets ((char*)AudioTestCase, 16, stdin);
    } while (strlen(strPtr) == 0);
    AudioTestCase[strlen(AudioTestCase)-1]='\0';

    if (!strcmp(AudioTestCase, "stop"))
    {
        return -1;
    }
    else if ((!strncmp(AudioTestCase, "R-", 2)) || (!strncmp(AudioTestCase, "L-", 2)))
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
        return 0;
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
    le_mcc_call_ObjRef_t   callRef,
    le_mcc_call_Event_t callEvent,
    void*               contextPtr
)
{
    le_result_t         res;

    if (callEvent == LE_MCC_CALL_EVENT_ALERTING)
    {
        LE_INFO("Call event is LE_MCC_CALL_EVENT_ALERTING.");
    }
    else if (callEvent == LE_MCC_CALL_EVENT_CONNECTED)
    {
        LE_INFO("Call event is LE_MCC_CALL_EVENT_CONNECTED.");
        ConnectAudio(callRef);
    }
    else if (callEvent == LE_MCC_CALL_EVENT_TERMINATED)
    {
        LE_INFO("Call event is LE_MCC_CALL_EVENT_TERMINATED.");
        le_mcc_call_TerminationReason_t term = le_mcc_call_GetTerminationReason(callRef);
        switch(term)
        {
            case LE_MCC_CALL_TERM_NETWORK_FAIL:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_NETWORK_FAIL");
                break;

            case LE_MCC_CALL_TERM_BAD_ADDRESS:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_BAD_ADDRESS");
                break;

            case LE_MCC_CALL_TERM_BUSY:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_BUSY");
                break;

            case LE_MCC_CALL_TERM_LOCAL_ENDED:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_LOCAL_ENDED");
                break;

            case LE_MCC_CALL_TERM_REMOTE_ENDED:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_REMOTE_ENDED");
                break;

            case LE_MCC_CALL_TERM_NOT_DEFINED:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_NOT_DEFINED");
                break;

            default:
                LE_INFO("Termination reason is %d", term);
                break;
        }

        DisconnectAllAudio(callRef);

        le_mcc_call_Delete(callRef);
    }
    else if (callEvent == LE_MCC_CALL_EVENT_INCOMING)
    {
        LE_INFO("Call event is LE_MCC_CALL_EVENT_INCOMING.");
        res = le_mcc_call_Answer(callRef);
        if (res == LE_OK)
        {
            ConnectAudio(callRef);
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
    le_mcc_profile_ObjRef_t profileRef = contextPtr;

    le_mcc_profile_AddCallEventHandler(profileRef, MyCallEventHandler, NULL);

    le_event_RunLoop();
    return NULL;
}

COMPONENT_INIT
{
    le_mcc_profile_ObjRef_t profileRef;
    int32_t                 stopTest = 0;

    LE_INFO("Init");

    CurrentCLIRStatus = AUDIO_MCC_TEST_CLIR_DEFAULT;

    SetupBindings();
    ConnectServices();

    profileRef=le_mcc_profile_GetByName("Modem-Sim1");
    if ( profileRef == NULL )
    {
        LE_INFO("Unable to get the Call profile reference");
        exit(1);
    }

    // Start the handler thread to monitor the call for the just created profile.
    le_thread_Start(le_thread_Create("MCC", HandlerThread, profileRef));

    while(stopTest <= 0)
    {
        stopTest = GetTel();
        if (stopTest == 0)
        {
            le_result_t res;
            le_onoff_t  clirState;
            TestCallRef = le_mcc_profile_CreateCall(profileRef, DestinationNumber);

            if (CurrentCLIRStatus == AUDIO_MCC_TEST_CLIR_ON)
            {
                res = le_mcc_call_SetCallerIdRestrict(TestCallRef,LE_ON);
                if(res != LE_OK)
                {
                    LE_ERROR("le_mcc_call_SetCallerIdRestrict() return LE_NOT_FOUND" );
                }
            }
            if (CurrentCLIRStatus == AUDIO_MCC_TEST_CLIR_OFF)
            {
                res = le_mcc_call_SetCallerIdRestrict(TestCallRef,LE_OFF);
                if(res != LE_OK)
                {
                    LE_ERROR("le_mcc_call_SetCallerIdRestrict() return LE_NOT_FOUND" );
                }
            }

            res = le_mcc_call_GetCallerIdRestrict(TestCallRef,&clirState);
            if(res != LE_OK)
            {
                if (res ==  LE_NOT_FOUND)
                {
                    LE_ERROR("le_mcc_call_GetCallerIdRestrict() return LE_NOT_FOUND" );
                }
                else
                {
                    LE_ERROR("le_mcc_call_GetCallerIdRestrict() return ERROR" );
                }
            }
            else
            {
                fprintf(stderr, "\nCurrent CLIR status on the call is %s\n",
                                (clirState == LE_ON ? "LE_ON" : "LE_OFF" ));
            }

            stopTest = GetAudioTestCaseChoice();

            if (!stopTest)
            {
                le_mcc_call_Start(TestCallRef);
            }
            else
            {
                LE_INFO("Exit Audio Test!");
                exit(0);
            }
        }
        else if (stopTest > 0)
        {
            LE_INFO("Exit Audio Test!");
            exit(0);
        }
    }


}

