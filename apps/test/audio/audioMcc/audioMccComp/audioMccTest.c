/**
 * This module is for testing of the Audio service component.
 *
 * On the target, you must issue the following commands:
 * $ app start audioMccTest
 * $ app runProc audioMccTest --exe=audioMccTest -- <Phone number> <test case> [main audio path] [file's name]
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"


static le_mcc_CallRef_t TestCallRef;

static le_audio_StreamRef_t MdmRxAudioRef = NULL;
static le_audio_StreamRef_t MdmTxAudioRef = NULL;
static le_audio_StreamRef_t FeInRef = NULL;
static le_audio_StreamRef_t FeOutRef = NULL;
static le_audio_StreamRef_t FileAudioRef = NULL;

static le_audio_ConnectorRef_t AudioInputConnectorRef = NULL;
static le_audio_ConnectorRef_t AudioOutputConnectorRef = NULL;

static le_audio_MediaHandlerRef_t MediaHandlerRef = NULL;

static const char* DestinationNumber;
static const char* AudioTestCase;
static const char* MainAudioSoundPath;
static const char* AudioFilePath;
static int   AudioFileFd = -1;

#define GAIN_VALUE  0x3000


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Media Event Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyMediaEventHandler
(
    le_audio_StreamRef_t          streamRef,
    le_audio_MediaEvent_t          event,
    void*                         contextPtr
)
{
    switch(event)
    {
        case LE_AUDIO_MEDIA_ENDED:
            LE_INFO("File event is LE_AUDIO_MEDIA_ENDED.");
            break;

        case LE_AUDIO_MEDIA_ERROR:
            LE_INFO("File event is LE_AUDIO_MEDIA_ERROR.");
            break;
        case LE_AUDIO_MEDIA_NO_MORE_SAMPLES:
            LE_INFO("File event is LE_AUDIO_MEDIA_NO_MORE_SAMPLES.");
            break;
        default:
            LE_INFO("File event is %d", event);
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Enable File remote playback.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToFileRemotePlay
(
    void
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
    FileAudioRef = le_audio_OpenPlayer();
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFilePlayback returns NULL!");

    LE_ERROR_IF((le_audio_SetGain(FileAudioRef, GAIN_VALUE) != LE_OK), "Cannot set multimedia gain");

    MediaHandlerRef = le_audio_AddMediaHandler(FileAudioRef, MyMediaEventHandler, NULL);
    LE_ERROR_IF((MediaHandlerRef==NULL), "AddMediaHandler returns NULL!");

    if (FileAudioRef && AudioInputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FileAudioRef);
        if(res!=LE_OK)
        {
            LE_ERROR("Failed to connect FilePlayback on input connector!");
            return;
        }

        LE_INFO("FilePlayback is now connected.");
        res = le_audio_PlayFile(FileAudioRef, AudioFileFd);

        if(res != LE_OK)
        {
            LE_ERROR("Failed to play the file!");
        }
        else
        {
            LE_INFO("File is now playing");
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable File remote recording.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToFileRemoteRec
(
    void
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
    FileAudioRef = le_audio_OpenRecorder();
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFileRecording returns NULL!");

    LE_ERROR_IF((le_audio_SetGain(FileAudioRef, GAIN_VALUE) != LE_OK), "Cannot set multimedia gain");

    if (FileAudioRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioOutputConnectorRef, FileAudioRef);
        if(res!=LE_OK)
        {
            LE_ERROR("Failed to connect Recorder on output connector!");
            return;
        }

        LE_INFO("Recorder is now connected.");
        res = le_audio_RecordFile(FileAudioRef, AudioFileFd);

        if(res!=LE_OK)
        {
            LE_ERROR("Failed to record the file");
        }
        else
        {
            LE_INFO("File is now recording.");
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable File local playback.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToFileLocalPlay
(
    void
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
    FileAudioRef = le_audio_OpenPlayer();
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFilePlayback returns NULL!");

    LE_ERROR_IF((le_audio_SetGain(FileAudioRef, GAIN_VALUE) != LE_OK), "Cannot set multimedia gain");

    MediaHandlerRef = le_audio_AddMediaHandler(FileAudioRef, MyMediaEventHandler, NULL);
    LE_ERROR_IF((MediaHandlerRef==NULL), "AddMediaHandler returns NULL!");

    if (FileAudioRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioOutputConnectorRef, FileAudioRef);
        if(res!=LE_OK)
        {
            LE_ERROR("Failed to connect FilePlayback on output connector!");
            return;
        }

        LE_INFO("FilePlayback is now connected.");
        res = le_audio_PlayFile(FileAudioRef, AudioFileFd);

        if(res != LE_OK)
        {
            LE_ERROR("Failed to play the file!");
        }
        else
        {
            LE_INFO("File is now playing");
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable File remote recording.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToFileLocalRec
(
    void
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
    FileAudioRef = le_audio_OpenRecorder();
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFileRecording returns NULL!");

    LE_ERROR_IF((le_audio_SetGain(FileAudioRef, GAIN_VALUE) != LE_OK), "Cannot set multimedia gain");

    if (FileAudioRef && AudioInputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FileAudioRef);
        if(res!=LE_OK)
        {
            LE_ERROR("Failed to connect FileRecording on input connector!");
            return;
        }

        LE_INFO("Recorder is now connected.");
        res = le_audio_RecordFile(FileAudioRef, AudioFileFd);

        if(res!=LE_OK)
        {
            LE_ERROR("Failed to record the file");
        }
        else
        {
            LE_INFO("File is now recording.");
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect audio to analogic input/output.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToCodec
(
    void
)
{
    le_result_t res;

    MdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((MdmRxAudioRef==NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_audio_OpenModemVoiceTx();
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

//--------------------------------------------------------------------------------------------------
/**
 * Connect audio to PCM.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToPcm
(
    void
)
{
    le_result_t res;

    MdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((MdmRxAudioRef==NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_audio_OpenModemVoiceTx();
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

//--------------------------------------------------------------------------------------------------
/**
 * Connect audio to I2S.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudioToI2s
(
    void
)
{
    le_result_t res;

    MdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((MdmRxAudioRef==NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_audio_OpenModemVoiceTx();
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

    MdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((MdmRxAudioRef==NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_audio_OpenModemVoiceTx();
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

    MdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((MdmRxAudioRef == NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_audio_OpenModemVoiceTx();
    LE_ERROR_IF((MdmTxAudioRef == NULL), "GetTxAudioStream returns NULL!");

    // Redirect audio to the USB.
    FeOutRef = le_audio_OpenUsbTx();
    LE_ERROR_IF((FeOutRef == NULL), "OpenUsbTx returns NULL!");
    // Redirect audio to the I2S.
    FeInRef = le_audio_OpenI2sRx(LE_AUDIO_I2S_STEREO);
    LE_ERROR_IF((FeInRef == NULL), "OpenI2sRx returns NULL!");

    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef == NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef == NULL), "AudioOutputConnectorRef is NULL!");

    if (MdmRxAudioRef && MdmTxAudioRef && FeOutRef && FeInRef &&
        AudioInputConnectorRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FeInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect I2S Rx on Input connector!");
        res = le_audio_Connect(AudioInputConnectorRef, MdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect USB Tx on Output connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, MdmRxAudioRef);
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

    MdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((MdmRxAudioRef == NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_audio_OpenModemVoiceTx();
    LE_ERROR_IF((MdmTxAudioRef == NULL), "GetTxAudioStream returns NULL!");

    // Redirect audio to the USB.
    FeOutRef = le_audio_OpenUsbTx();
    LE_ERROR_IF((FeOutRef == NULL), "OpenUsbTx returns NULL!");
    // Redirect audio to the PCM.
    FeInRef = le_audio_OpenPcmRx(0);
    LE_ERROR_IF((FeInRef == NULL), "OpenPcmRx returns NULL!");

    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef == NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef == NULL), "AudioOutputConnectorRef is NULL!");

    if (MdmRxAudioRef && MdmTxAudioRef && FeOutRef && FeInRef &&
        AudioInputConnectorRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FeInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect PCM Rx on Input connector!");
        res = le_audio_Connect(AudioInputConnectorRef, MdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect USB Tx on Output connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, MdmRxAudioRef);
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

    MdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((MdmRxAudioRef == NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_audio_OpenModemVoiceTx();
    LE_ERROR_IF((MdmTxAudioRef == NULL), "GetTxAudioStream returns NULL!");

    // Redirect audio to the I2S.
    FeOutRef = le_audio_OpenI2sTx(LE_AUDIO_I2S_STEREO);
    LE_ERROR_IF((FeOutRef == NULL), "OpenI2sTx returns NULL!");
    // Redirect audio to the USB.
    FeInRef = le_audio_OpenUsbRx();
    LE_ERROR_IF((FeInRef == NULL), "OpenUsbRx returns NULL!");

    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef == NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef == NULL), "AudioOutputConnectorRef is NULL!");

    if (MdmRxAudioRef && MdmTxAudioRef && FeOutRef && FeInRef &&
        AudioInputConnectorRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FeInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect USB Rx on Input connector!");
        res = le_audio_Connect(AudioInputConnectorRef, MdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect I2S Tx on Output connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, MdmRxAudioRef);
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

    MdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_ERROR_IF((MdmRxAudioRef == NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_audio_OpenModemVoiceTx();
    LE_ERROR_IF((MdmTxAudioRef == NULL), "GetTxAudioStream returns NULL!");

    // Redirect audio to the PCM.
    FeOutRef = le_audio_OpenPcmTx(0);
    LE_ERROR_IF((FeOutRef == NULL), "OpenPcmTx returns NULL!");
    // Redirect audio to the USB.
    FeInRef = le_audio_OpenUsbRx();
    LE_ERROR_IF((FeInRef == NULL), "OpenUsbRx returns NULL!");

    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef == NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef == NULL), "AudioOutputConnectorRef is NULL!");

    if (MdmRxAudioRef && MdmTxAudioRef && FeOutRef && FeInRef &&
        AudioInputConnectorRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FeInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect USB Rx on Input connector!");
        res = le_audio_Connect(AudioInputConnectorRef, MdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect PCM Tx on Output connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, MdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Main audio connection function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudio
(
    void
)
{

    if (strcmp(AudioTestCase,"MIC")==0)
    {
        LE_INFO("Connect MIC and SPEAKER ");
        ConnectAudioToCodec();
    }
    else if (strcmp(AudioTestCase,"PCM")==0)
    {
        LE_INFO("Connect PCM ");
        ConnectAudioToPcm();
    }
    else if (strcmp(AudioTestCase,"I2S")==0)
    {
        LE_INFO("Connect I2S");
        ConnectAudioToI2s();
    }
    else if (strcmp(AudioTestCase,"USB")==0)
    {
        LE_INFO("Connect USB ");
        ConnectAudioToUsb();
    }
    else if (strcmp(MainAudioSoundPath,"USBTXI2SRX")==0)
    {
        LE_INFO("Connect USBTXI2SRX ");
        ConnectAudioToUsbTxI2sRx();
    }
    else if (strcmp(MainAudioSoundPath,"USBTXPCMRX")==0)
    {
        LE_INFO("Connect USBTXPCMRX ");
        ConnectAudioToUsbTxPcmRx();
    }
    else if (strcmp(MainAudioSoundPath,"USBRXI2STX")==0)
    {
        LE_INFO("Connect USBRXI2STX ");
        ConnectAudioToUsbRxI2sTx();
    }
    else if (strcmp(MainAudioSoundPath,"USBRXPCMTX")==0)
    {
        LE_INFO("Connect USBRXPCMTX ");
        ConnectAudioToUsbRxPcmTx();
    }
    else if ((strncmp(AudioTestCase,"R-",2)==0) || (strncmp(AudioTestCase,"L-",2)==0))
    {
        if (strcmp(MainAudioSoundPath,"MIC")==0)
        {
            LE_INFO("Connect MIC and SPEAKER ");
            ConnectAudioToCodec();
        }
        else if (strcmp(MainAudioSoundPath,"PCM")==0)
        {
            LE_INFO("Connect PCM ");
            ConnectAudioToPcm();
        }
        else if (strcmp(MainAudioSoundPath,"I2S")==0)
        {
            LE_INFO("Connect I2S");
            ConnectAudioToI2s();
        }
        else if (strcmp(MainAudioSoundPath,"USB")==0)
        {
            LE_INFO("Connect USB ");
            ConnectAudioToUsb();
        }
        else if (strcmp(MainAudioSoundPath,"USBTXI2SRX")==0)
        {
            LE_INFO("Connect USBTXI2SRX ");
            ConnectAudioToUsbTxI2sRx();
        }
        else if (strcmp(MainAudioSoundPath,"USBTXPCMRX")==0)
        {
            LE_INFO("Connect USBTXPCMRX ");
            ConnectAudioToUsbTxPcmRx();
        }
        else if (strcmp(MainAudioSoundPath,"USBRXI2STX")==0)
        {
            LE_INFO("Connect USBRXI2STX ");
            ConnectAudioToUsbRxI2sTx();
        }
        else if (strcmp(MainAudioSoundPath,"USBRXPCMTX")==0)
        {
            LE_INFO("Connect USBRXPCMTX ");
            ConnectAudioToUsbRxPcmTx();
        }
        else
        {
            LE_INFO("Bad test case");
        }
    }
    else
    {
        LE_INFO("Bad test case");
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
    void
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

    if(MediaHandlerRef)
    {
        le_audio_RemoveMediaHandler(MediaHandlerRef);
        MediaHandlerRef = NULL;
    }

    // Closing AudioFileFd is unnecessary since the messaging infrastructure underneath
    // le_audio_xxx APIs that use it would close it.
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Call Event Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyCallEventHandler
(
    le_mcc_CallRef_t   callRef,
    le_mcc_Event_t callEvent,
    void*               contextPtr
)
{
    le_result_t         res;

    if (callEvent == LE_MCC_EVENT_ALERTING)
    {
        LE_INFO("Call event is LE_MCC_EVENT_ALERTING.");
    }
    else if (callEvent == LE_MCC_EVENT_CONNECTED)
    {
        LE_INFO("Call event is LE_MCC_EVENT_CONNECTED.");

        // Connect SW-PCM
        if (strcmp(AudioTestCase,"R-PB")==0)
        {
            LE_INFO("Connect Remote Play");
            ConnectAudioToFileRemotePlay();
        }
        else
        if (strcmp(AudioTestCase,"R-REC")==0)
        {
            LE_INFO("Connect Remote Rec ");
            ConnectAudioToFileRemoteRec();
        }
        else if (strcmp(AudioTestCase,"L-PB")==0)
        {
            LE_INFO("Connect Local Play");
            ConnectAudioToFileLocalPlay();
        }
        else if (strcmp(AudioTestCase,"L-REC")==0)
        {
            LE_INFO("Connect Local Rec ");
            ConnectAudioToFileLocalRec();
        }
        else
        {
            LE_INFO("Bad test case");
        }
    }
    else if (callEvent == LE_MCC_EVENT_TERMINATED)
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

        DisconnectAllAudio();

        le_mcc_Delete(callRef);
    }
    else if (callEvent == LE_MCC_EVENT_INCOMING)
    {
        LE_INFO("Call event is LE_MCC_EVENT_INCOMING.");

        res = le_mcc_Answer(callRef);
        if (res != LE_OK)
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
 * Helper.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage()
{
    int idx;
    bool sandboxed = (getuid() != 0);
    const char * usagePtr[] = {
            "Usage of the audioMccTest  is:",
            "   audioMccTest <Phone number> <test case> [main audio path] [file's name]",
            "",
            "Test cases are:",
            " - MIC (for mic/speaker)",
            " - PCM (not supported on mangOH board - for AR755x, AR8652 devkit's codec use, "
                "execute 'wm8940_demo --pcm' command)",
            " - I2S (not supported on mangOH board - for AR755x, AR8652 devkit's codec use, "
                "execute 'wm8940_demo --i2s' command)",
            " - USB (for USB)",
            " - R-PB (for Remote playback)",
            " - R-REC (for Remote recording)",
            " - L-PB (for Local playback)",
            " - L-REC (for Local recording)",
            "",
            "Main audio paths are: (for file playback/recording only)",
            " - MIC (for mic/speaker)",
            " - PCM (not supported on mangOH board - for AR755x, AR8652 devkit's codec use, "
                "execute 'wm8940_demo --pcm' command)",
            " - I2S (not supported on mangOH board - for AR755x, AR8652 devkit's codec use, "
                "execute 'wm8940_demo --i2s' command)",
            " - USB (for USB)",
            "",
            "File's name can be the complete file's path (for file playback/recording only).",
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
    if (le_arg_NumArgs() >= 2)
    {
        LE_INFO("======== Start Audio implementation Test (audioMccTest) ========");

        DestinationNumber = le_arg_GetArg(0);
        if (NULL == DestinationNumber)
        {
            LE_ERROR("DestinationNumber is NULL");
            exit(EXIT_FAILURE);
        }
        AudioTestCase = le_arg_GetArg(1);
        LE_INFO("   Phone number.%s", DestinationNumber);
        LE_INFO("   Test case.%s", AudioTestCase);

        if(le_arg_NumArgs() == 4)
        {
            MainAudioSoundPath = le_arg_GetArg(2);
            AudioFilePath = le_arg_GetArg(3);
            LE_INFO("   Main audio path.%s", MainAudioSoundPath);
            LE_INFO("   Audio file [%s]", AudioFilePath);
        }

        le_mcc_AddCallEventHandler( MyCallEventHandler, NULL);

        // Configure the audio
        ConnectAudio();

        TestCallRef = le_mcc_Create(DestinationNumber);
        le_mcc_Start(TestCallRef);

        LE_INFO("======== Audio implementation Test (audioMccTest) started successfully ========");
    }
    else
    {
        PrintUsage();
        LE_INFO("EXIT audioMccTest");
        exit(EXIT_FAILURE);
    }
}
