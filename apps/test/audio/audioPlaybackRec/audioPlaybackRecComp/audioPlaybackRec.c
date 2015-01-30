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

static le_audio_StreamEventHandlerRef_t StreamHandlerRef = NULL;

static const char* AudioTestCase;
static const char* MainAudioSoundPath;
static const char* AudioFilePath;
static const char* Option;
static int   AudioFileFd = -1;

static void DisconnectAllAudio(void);

le_timer_Ref_t  GainTimerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Timer handler function for volume playback test.
 *
 */
//--------------------------------------------------------------------------------------------------

static void GainTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    static uint32_t vol = 0, getVol = 0;
    static bool increase = true;
    le_result_t result;

    if ( increase )
    {
        vol+=20;

        if (vol == 100)
        {
            increase = false;
        }
    }
    else
    {
        vol-=20;

        if (vol == 0)
        {
            increase = true;
        }
    }

    LE_INFO("Playback volume: vol %d", vol);
    result = le_audio_SetGain(FileAudioRef, vol);

    if (result != LE_OK)
    {
        LE_ERROR("le_audio_SetGain error : %d", result);
    }

    result = le_audio_GetGain(FileAudioRef, &getVol);

    if ((result != LE_OK) || (vol != getVol))
    {
        LE_ERROR("le_audio_GetGain error : %d read volume: %d", result, getVol);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Stream Event Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyStreamEventHandler
(
    le_audio_StreamRef_t          streamRef,
    le_audio_StreamEventBitMask_t streamEventMask,
    void*                         contextPtr
)
{
    le_audio_FileEvent_t event;

    if (streamEventMask & LE_AUDIO_BITMASK_FILE_EVENT)
    {
        if(le_audio_GetFileEvent(streamRef, &event) == LE_OK)
        {
            switch(event)
            {
                case LE_AUDIO_FILE_ENDED:
                    LE_INFO("File event is LE_AUDIO_FILE_ENDED.");
                    break;

                case LE_AUDIO_FILE_ERROR:
                    LE_INFO("File event is LE_AUDIO_FILE_ERROR.");
                    break;
            }
        }
    }

    if (GainTimerRef)
    {
        le_timer_Stop (GainTimerRef);
        le_timer_Delete (GainTimerRef);

    }

    DisconnectAllAudio();

    exit(0);
}


static void ConnectAudioToUsb
(
    void
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
    void
)
{
    le_result_t res;
    if ((AudioFileFd=open(AudioFilePath, O_RDONLY)) == -1)
    {
        LE_ERROR("Open file %s failure: errno.%d (%s)",  AudioFilePath, errno, strerror(errno));
        DisconnectAllAudio();
        exit(0);
    }
    else
    {
        LE_INFO("Open file %s with AudioFileFd.%d",  AudioFilePath, AudioFileFd);
    }

    // Play local on output connector.
    FileAudioRef = le_audio_OpenFilePlayback(AudioFileFd);
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFilePlayback returns NULL!");

    // Check for the test of the playback volume
    if ( (Option!= NULL) && strncmp(Option,"GAIN",4) == 0 )
    {
        le_clk_Time_t interval;
        interval.sec = 1;
        interval.usec = 0;

        LE_INFO("Test the playback volume");

        GainTimerRef = le_timer_Create  ( "Gain timer" );
        le_timer_SetHandler(GainTimerRef, GainTimerHandler);

        le_audio_SetGain(FileAudioRef, 0);
        le_timer_SetInterval(GainTimerRef, interval);
        le_timer_SetRepeat(GainTimerRef,0);
        le_timer_Start(GainTimerRef);
    }
    else
    {
        LE_INFO("playback volume set to default value");
        le_audio_SetGain(FileAudioRef, 50);
    }

    StreamHandlerRef = le_audio_AddStreamEventHandler(FileAudioRef, LE_AUDIO_BITMASK_FILE_EVENT, MyStreamEventHandler, NULL);

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
    void
)
{
    le_result_t res;
    if ((AudioFileFd=open(AudioFilePath, O_WRONLY | O_CREAT | O_TRUNC)) == -1)
    {
        LE_ERROR("Open file %s failure: errno.%d (%s)",  AudioFilePath, errno, strerror(errno));
        DisconnectAllAudio();
        exit(0);
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
    void
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
    void
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
    void
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
    void
)
{
    if ((strncmp(AudioTestCase,"PB",2)==0) || (strncmp(AudioTestCase,"REC",3)==0))
    {
#ifdef ENABLE_CODEC
        if (strcmp(MainAudioSoundPath,"MIC")==0)
        {
            LE_INFO("Connect MIC and SPEAKER ");
            ConnectAudioToCodec();
        }
        else
#endif
        if (strcmp(MainAudioSoundPath,"PCM")==0)
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
        else
        {
            LE_INFO("Error in format could not connect audio");
        }

        // Connect SW-PCM
        if (strcmp(AudioTestCase,"PB")==0)
        {
            LE_INFO("Connect Local Play");
            ConnectAudioToFileLocalPlay();
        }
        else if (strcmp(AudioTestCase,"REC")==0)
        {
            LE_INFO("Connect Local Rec ");
            ConnectAudioToFileLocalRec();
        }
        else
        {
            LE_INFO("Error in format could not connect audio");
        }
    }
    else
    {
        LE_INFO("Error in format could not connect audio");
    }
}

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

    close(AudioFileFd);
}

static void PrintUsage()
{
    int idx;
    bool sandboxed = (getuid() != 0);
    const char * usagePtr[] = {
            "Usage of the audioPlaybackRec test is:",
            "   audioMccTest <test case> [main audio path] [file's name] [option]",
            "",
            "Test cases are:",
            " - PB (for Local playback)",
            " - REC (for Local recording)",
            "",
            "Main audio paths are: (for file playback/recording only)",
#if (ENABLE_CODEC == 1)
            " - MIC (for mic/speaker)",
#endif
            " - PCM (for devkit's codec use, execute 'wm8940_demo --pcm' command)",
            " - I2S (for devkit's codec use, execute 'wm8940_demo --i2s' command)",
            " - USB (for USB)",
            "",
            "File's name can be the complete file's path",
            "",
            "Options are:",
            " - GAIN (for playback gain testing)"
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
    LE_INFO("Init");

    if (le_arg_NumArgs() >= 1)
    {
        LE_INFO("======== Start Audio implementation Test (audioPlaybackRecTest) ========");

        AudioTestCase = le_arg_GetArg(0);
        LE_INFO("   Test case.%s", AudioTestCase);

        if(le_arg_NumArgs() >= 3)
        {
            MainAudioSoundPath = le_arg_GetArg(1);
            AudioFilePath = le_arg_GetArg(2);
            LE_INFO("   Main audio path.%s", MainAudioSoundPath);
            LE_INFO("   Audio file [%s]", AudioFilePath);
        }

        if(le_arg_NumArgs() >= 4)
        {
            Option = le_arg_GetArg(3);
        }

        ConnectAudio();

        LE_INFO("======== Audio implementation Test (audioMccTest) started successfully ========");
    }
    else
    {
        PrintUsage();
        LE_INFO("EXIT audioMccTest");
        exit(EXIT_FAILURE);
    }
}

