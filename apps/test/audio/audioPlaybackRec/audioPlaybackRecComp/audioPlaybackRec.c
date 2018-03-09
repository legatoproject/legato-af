/**
 * This module is for unit of the audio playback/recorder
 *
* You must issue the following commands:
* @verbatim
  $ app runProc audioPlaybackRec --exe=audioPlaybackRecTest --
  $     <test case> [main audio path] [file's name] [option]

  Example:
  $ wm8940_demo --i2s
  $ app runProc audioPlaybackRec --exe=audioPlaybackRecTest --
  $     REC I2S /record/remote.wav WAV STOP=10
  $ app runProc audioPlaybackRec --exe=audioPlaybackRecTest -- PB I2S /usr/share/sounds/0-to-9.wav
  $ app runProc audioPlaybackRec --exe=audioPlaybackRecTest -- PB I2S /usr/share/sounds/0-to-9.wav
  $     PAUSE=2 RESUME=3
 @endverbatim
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------
#define ID_RIFF    0x46464952
#define ID_WAVE    0x45564157
#define ID_FMT     0x20746d66
#define ID_DATA    0x61746164
#define FORMAT_PCM 1

//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

struct
{
    enum
    {
        STOP,
        PLAY,
        PAUSE,
        RESUME,
        RECORD,
        DISCONNECT
    } typeOption;
} OptionContext;

typedef struct {
    uint32_t riffId;         ///< "RIFF" constant. Marks the file as a riff file.
    uint32_t riffSize;       ///< Size of the overall file - 8 bytes
    uint32_t riffFmt;        ///< File Type Header. For our purposes, it always equals "WAVE".
    uint32_t fmtId;          ///< Equals "fmt ". Format chunk marker. Includes trailing null
    uint32_t fmtSize;        ///< Length of format data as listed above
    uint16_t audioFormat;    ///< Audio format (PCM)
    uint16_t channelsCount;  ///< Number of channels
    uint32_t sampleRate;     ///< Sample frequency in Hertz
    uint32_t byteRate;       ///< sampleRate * channelsCount * bps / 8
    uint16_t blockAlign;     ///< channelsCount * bps / 8
    uint16_t bitsPerSample;  ///< Bits per sample
    uint32_t dataId;         ///< "data" chunk header. Marks the beginning of the data section.
    uint32_t dataSize;       ///< Data size
} WavHeader_t;

typedef struct
{
    WavHeader_t hd;
    uint32_t wroteLen;
    int pipefd[2];
    le_thread_Ref_t mainThreadRef;
    bool playDone;
}
PbRecSamplesThreadCtx_t;

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

static le_audio_StreamRef_t MdmRxAudioRef = NULL;
static le_audio_StreamRef_t MdmTxAudioRef = NULL;
static le_audio_StreamRef_t FeInRef = NULL;
static le_audio_StreamRef_t FeOutRef = NULL;
static le_audio_StreamRef_t FileAudioRef = NULL;

static le_audio_ConnectorRef_t AudioInputConnectorRef = NULL;
static le_audio_ConnectorRef_t AudioOutputConnectorRef = NULL;

static le_audio_MediaHandlerRef_t MediaHandlerRef = NULL;

static const char* AudioTestCase;
static const char* MainAudioSoundPath;
static const char* AudioFilePath;

static int   AudioFileFd = -1;
static bool PlayInLoop = false;

le_timer_Ref_t OptionTimerRef;
le_timer_Ref_t  GainTimerRef = NULL;
le_timer_Ref_t  MuteTimerRef = NULL;

static PbRecSamplesThreadCtx_t PbRecSamplesThreadCtx;
static le_thread_Ref_t RecPbThreadRef = NULL;

static uint32_t ChannelsCount;
static uint32_t SampleRate;
static uint32_t BitsPerSample;

static void DisconnectAllAudio(void);
static void PlaySamples(void* param1Ptr,void* param2Ptr);
static uint8_t NextOptionArg;
static le_audio_Format_t AudioFormat;
static bool DtxActivation;
static le_audio_AmrMode_t AmrMode;


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
    static int32_t vol = 0, getVol = 0;
    static bool increase = true;
    le_result_t result;

    if ( increase )
    {
        vol+=1;

        if (vol == 100)
        {
            increase = false;
        }
    }
    else
    {
        vol-=1;

        if (vol == 0)
        {
            increase = true;
        }
    }

    LE_INFO("Playback volume: vol %d", vol);
    result = le_audio_SetGain(FileAudioRef, vol);

    if (result != LE_OK)
    {
        LE_FATAL("le_audio_SetGain error : %d", result);
    }

    result = le_audio_GetGain(FileAudioRef, &getVol);

    if ((result != LE_OK) || (vol != getVol))
    {
        LE_FATAL("le_audio_GetGain error : %d read volume: %d", result, getVol);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Timer handler function for volume playback MUTE test.
 *
 */
//--------------------------------------------------------------------------------------------------

static void MuteTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    static bool mute = false;
    le_result_t result;

    if ( mute )
    {
        result = le_audio_Unmute(FileAudioRef);
        mute = false;
        LE_INFO("Unmute audio Playback");
    }
    else
    {
        result = le_audio_Mute(FileAudioRef);
        mute = true;
        LE_INFO("Mute audio Playback");
    }

    if (result != LE_OK)
    {
        LE_FATAL("le_audio_Mute/le_audio_Unmute Failed %d", result);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Rec Samples thread destructor.
 *
 */
//--------------------------------------------------------------------------------------------------

static void DestroyRecThread
(
    void *contextPtr
)
{
    PbRecSamplesThreadCtx_t *threadCtxPtr = (PbRecSamplesThreadCtx_t*) contextPtr;

    LE_INFO("wroteLen %d", threadCtxPtr->wroteLen);

    // Closing AudioFileFd is unnecessary since the messaging infrastructure underneath
    // le_audio_xxx APIs that use it would close it.
}

//--------------------------------------------------------------------------------------------------
/**
 * Rec Samples thread.
 *
 */
//--------------------------------------------------------------------------------------------------

static void* RecSamplesThread
(
    void* contextPtr
)
{
    int pipefd[2];
    int32_t len = 1024;
    char data[len];
    uint32_t channelsCount;
    uint32_t sampleRate;
    uint32_t bitsPerSample;
    PbRecSamplesThreadCtx_t *threadCtxPtr = (PbRecSamplesThreadCtx_t*) contextPtr;

    le_audio_ConnectService();

    lseek(AudioFileFd, 0, SEEK_SET);

    if (pipe(pipefd) == -1)
    {
        LE_ERROR("Failed to create the pipe");
        return NULL;
    }

    LE_ASSERT(le_audio_SetSamplePcmChannelNumber(FileAudioRef, ChannelsCount) == LE_OK);
    LE_ASSERT(le_audio_SetSamplePcmSamplingRate(FileAudioRef, SampleRate) == LE_OK);
    LE_ASSERT(le_audio_SetSamplePcmSamplingResolution(FileAudioRef, BitsPerSample) == LE_OK);

    LE_ASSERT(le_audio_GetSamplePcmChannelNumber(FileAudioRef, &channelsCount) == LE_OK);
    LE_ASSERT(channelsCount == ChannelsCount);
    LE_ASSERT(le_audio_GetSamplePcmSamplingRate(FileAudioRef, &sampleRate) == LE_OK);
    LE_ASSERT(sampleRate == SampleRate);
    LE_ASSERT(le_audio_GetSamplePcmSamplingResolution(FileAudioRef, &bitsPerSample) == LE_OK);
    LE_ASSERT(bitsPerSample == BitsPerSample);

    LE_ASSERT(le_audio_GetSamples(FileAudioRef, pipefd[1]) == LE_OK);
    LE_INFO("Start getting samples...");

    int32_t readLen;
    int32_t writeLen;

    while ((readLen = read( pipefd[0], data, len )))
    {
        if (readLen < 0)
        {
            LE_ERROR("read error %d %d", len, readLen);
            break;
        }

        writeLen = write( AudioFileFd, data, readLen );

        if (writeLen < 0)
        {
            LE_ERROR("write error %d %d", readLen, writeLen);
            break;
        }

        threadCtxPtr->wroteLen += writeLen;
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Rec Samples.
 *
 */
//--------------------------------------------------------------------------------------------------

static void RecSamples
(
    void
)
{
    memset(&PbRecSamplesThreadCtx,0,sizeof(PbRecSamplesThreadCtx_t));

    RecPbThreadRef = le_thread_Create("RecSamples", RecSamplesThread, &PbRecSamplesThreadCtx);

    le_thread_AddChildDestructor(RecPbThreadRef,
                                DestroyRecThread,
                                &PbRecSamplesThreadCtx);

    le_thread_Start(RecPbThreadRef);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Play Samples thread destructor.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DestroyPlayThread
(
    void *contextPtr
)
{
    PbRecSamplesThreadCtx_t *threadCtxPtr = (PbRecSamplesThreadCtx_t*) contextPtr;

    LE_INFO("DestroyPlayThread playDone %d PlayInLoop %d", threadCtxPtr->playDone, PlayInLoop);

    if (RecPbThreadRef != NULL)
    {
        le_thread_Cancel(RecPbThreadRef);
        RecPbThreadRef = NULL;

        // Restart in case of looping playback
        if (threadCtxPtr->playDone && PlayInLoop)
        {
            le_event_QueueFunctionToThread(threadCtxPtr->mainThreadRef,
                                           PlaySamples,
                                           contextPtr,
                                           NULL);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Play Samples thread.
 *
 */
//--------------------------------------------------------------------------------------------------
static void* PlaySamplesThread
(
    void* contextPtr
)
{
    char data[1024];
    int32_t len = 1;
    uint32_t channelsCount;
    uint32_t sampleRate;
    uint32_t bitsPerSample;
    PbRecSamplesThreadCtx_t *threadCtxPtr = (PbRecSamplesThreadCtx_t*) contextPtr;

    le_audio_ConnectService();

    lseek(AudioFileFd, 0, SEEK_SET);

    if ( ( threadCtxPtr->pipefd[0] == -1 ) && ( threadCtxPtr->pipefd[1] == -1 ) )
    {
        if (pipe(threadCtxPtr->pipefd) == -1)
        {
            LE_ERROR("Failed to create the pipe");
            return NULL;
        }

        LE_ASSERT(le_audio_SetSamplePcmChannelNumber(FileAudioRef, ChannelsCount) == LE_OK);
        LE_ASSERT(le_audio_SetSamplePcmSamplingRate(FileAudioRef, SampleRate) == LE_OK);
        LE_ASSERT(le_audio_SetSamplePcmSamplingResolution(FileAudioRef, BitsPerSample) == LE_OK);

        LE_ASSERT(le_audio_GetSamplePcmChannelNumber(FileAudioRef, &channelsCount) == LE_OK);
        LE_ASSERT(channelsCount == ChannelsCount);
        LE_ASSERT(le_audio_GetSamplePcmSamplingRate(FileAudioRef, &sampleRate) == LE_OK);
        LE_ASSERT(sampleRate == SampleRate);
        LE_ASSERT(le_audio_GetSamplePcmSamplingResolution(FileAudioRef, &bitsPerSample) == LE_OK);
        LE_ASSERT(bitsPerSample == BitsPerSample);

        LE_ASSERT(le_audio_PlaySamples(FileAudioRef, threadCtxPtr->pipefd[0]) == LE_OK);
        LE_INFO("Start playing samples...");
    }

    while ((len = read(AudioFileFd, data, 1024)) > 0)
    {
        int32_t wroteLen = write( threadCtxPtr->pipefd[1], data, len );

        if (wroteLen <= 0)
        {
            LE_ERROR("write error %d", wroteLen);
            return NULL;
        }
    }

    threadCtxPtr->playDone = true;

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Play Samples thread.
 *
 */
//--------------------------------------------------------------------------------------------------

static void PlaySamples
(
    void* param1Ptr,
    void* param2Ptr
)
{
    if (RecPbThreadRef == NULL)
    {
        RecPbThreadRef = le_thread_Create("PlaySamples", PlaySamplesThread, param1Ptr);

        le_thread_AddChildDestructor(RecPbThreadRef,
                                    DestroyPlayThread,
                                    param1Ptr);

        le_thread_Start(RecPbThreadRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Execute next optional parameters
 *
 */
//--------------------------------------------------------------------------------------------------
static void ExecuteNextOption
(
    void
)
{
    bool nextOption = false;

    if ( NextOptionArg < le_arg_NumArgs() )
    {
        const char* nextOptionArgPtr = le_arg_GetArg(NextOptionArg);
        if (NULL == nextOptionArgPtr)
        {
            LE_ERROR("nextOptionArgPtr is NULL");
            exit(EXIT_FAILURE);
        }
        if ( strncmp(nextOptionArgPtr, "STOP", 4) == 0 )
        {
            le_clk_Time_t interval={0};
            const char* stop = nextOptionArgPtr;

            if (strlen(stop) > 5) // higher than "STOP="
            {
                LE_INFO("STOP will be done in %d seconds", atoi(stop+5));
                interval.sec = atoi(stop+5);
                le_timer_SetInterval(OptionTimerRef, interval);
                OptionContext.typeOption = STOP;
                le_timer_Start(OptionTimerRef);
            }
        }
        else if ( strncmp(nextOptionArgPtr, "PLAY", 4) == 0 )
        {
            le_clk_Time_t interval={0};
            const char* play = nextOptionArgPtr;
            if (strlen(play) > 5) // higher than "PLAY="
            {
                LE_INFO("PLAY will be done in %d seconds", atoi(play+5));
                interval.sec = atoi(play+5);
                le_timer_SetInterval(OptionTimerRef, interval);
                OptionContext.typeOption = PLAY;
                le_timer_Start(OptionTimerRef);
            }
        }
        else if ( strncmp(nextOptionArgPtr, "RECORD", 6) == 0 )
        {
            le_clk_Time_t interval={0};
            const char* play = nextOptionArgPtr;
            if (strlen(play) > 7) // higher than "RECORD="
            {
                LE_INFO("RECORD will be done in %d seconds", atoi(play+7));
                interval.sec = atoi(play+7);
                le_timer_SetInterval(OptionTimerRef, interval);
                OptionContext.typeOption = RECORD;
                le_timer_Start(OptionTimerRef);
            }
        }
        else if ( strncmp(nextOptionArgPtr, "PAUSE", 5) == 0 )
        {
            le_clk_Time_t interval={0};
            const char* pause = nextOptionArgPtr;

            if (strlen(pause) > 6) // higher than "PAUSE="
            {
                LE_INFO("PAUSE will be done in %d seconds", atoi(pause+6));
                interval.sec = atoi(pause+6);
                le_timer_SetInterval(OptionTimerRef, interval);
                OptionContext.typeOption = PAUSE;
                le_timer_Start(OptionTimerRef);
            }
        }
        else if ( strncmp(nextOptionArgPtr, "RESUME", 6) == 0 )
        {
            le_clk_Time_t interval={0};
            const char* resume = nextOptionArgPtr;

            if (strlen(resume) > 7) // higher than "RESUME="
            {
                LE_INFO("RESUME will be done in %d seconds", atoi(resume+7));
                interval.sec = atoi(resume+7);
                le_timer_SetInterval(OptionTimerRef, interval);
                OptionContext.typeOption = RESUME;
                le_timer_Start(OptionTimerRef);
            }
        }
        else if ( strncmp(nextOptionArgPtr, "DISCONNECT", 10) == 0 )
        {
            le_clk_Time_t interval={0};
            const char* resume = nextOptionArgPtr;

            if (strlen(resume) > 11) // higher than "DISCONNECT="
            {
                LE_INFO("DISCONNECT will be done in %d seconds", atoi(resume+11));
                interval.sec = atoi(resume+11);
                le_timer_SetInterval(OptionTimerRef, interval);
                OptionContext.typeOption = DISCONNECT;
                le_timer_Start(OptionTimerRef);
            }
        }
        else if ( strncmp(nextOptionArgPtr, "LOOP", 4) == 0 )
        {
            PlayInLoop = true;
            nextOption = true;
        }
        else if ( strncmp(nextOptionArgPtr, "MUTE", 4) == 0 )
        {

            le_clk_Time_t interval;
            interval.sec = 1;
            interval.usec = 0;

            LE_INFO("Test the MUTE function");

            MuteTimerRef = le_timer_Create  ( "Mute timer" );
            le_timer_SetHandler(MuteTimerRef, MuteTimerHandler);

            le_timer_SetInterval(MuteTimerRef, interval);
            le_timer_SetRepeat(MuteTimerRef,0);
            le_timer_Start(MuteTimerRef);
        }
        else if ( strncmp(nextOptionArgPtr,"GAIN",4) == 0 )
        {
            le_clk_Time_t interval;
            interval.sec = 0;
            interval.usec = 100000;

            LE_INFO("Test the playback volume");

            GainTimerRef = le_timer_Create  ( "Gain timer" );
            le_timer_SetHandler(GainTimerRef, GainTimerHandler);

            le_audio_SetGain(FileAudioRef, 0);
            le_timer_SetInterval(GainTimerRef, interval);
            le_timer_SetRepeat(GainTimerRef,0);
            le_timer_Start(GainTimerRef);
        }

        NextOptionArg++;
    }

    if (nextOption)
    {
        ExecuteNextOption();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Timer handler for optional parameters
 *
 */
//--------------------------------------------------------------------------------------------------
void OptionTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    le_result_t result;

    LE_INFO("timeout for %d", OptionContext.typeOption);

    switch (OptionContext.typeOption)
    {
        case STOP:
            result = le_audio_Stop(FileAudioRef);
            LE_INFO("Stop result %d", result);

            if (RecPbThreadRef)
            {
                le_thread_Cancel(RecPbThreadRef);
                RecPbThreadRef = NULL;
            }

            if ( strncmp(AudioTestCase,"PB_SAMPLES",10)==0 )
            {
                // Closing PbRecSamplesThreadCtx.pipefd[] is unnecessary since the messaging
                // infrastructure underneath le_audio_xxx APIs that use it would close it.

                PbRecSamplesThreadCtx.pipefd[0] = -1;
                PbRecSamplesThreadCtx.pipefd[1] = -1;
                PbRecSamplesThreadCtx.playDone = 0;
            }
        break;
        case PLAY:
            if ( strncmp(AudioTestCase,"PB_SAMPLES",10)==0 )
            {
                PlaySamples(&PbRecSamplesThreadCtx, NULL);
            }
            else
            {
                result = le_audio_PlayFile(FileAudioRef,LE_AUDIO_NO_FD);
                LE_INFO("Play result %d", result);
            }
        break;
        case RECORD:
        if ( strncmp(AudioTestCase,"REC_SAMPLES",11)==0 )
        {
            RecSamples();
        }
        else
        {
            result = le_audio_RecordFile(FileAudioRef,LE_AUDIO_NO_FD);

            LE_INFO("Record result %d", result);
        }
        break;
        case PAUSE:
            result = le_audio_Pause(FileAudioRef);
            LE_INFO("Pause result %d", result);
        break;
        case RESUME:
            result = le_audio_Resume(FileAudioRef);
            LE_INFO("Resume result %d", result);
        break;
        case DISCONNECT:
            LE_INFO("disconnect all audio");
            DisconnectAllAudio();
        break;
        default:
        break;
    }

    ExecuteNextOption();
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Stream Event Notifications.
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
    if (!streamRef)
    {
        LE_ERROR("Bad streamRef");
        return;
    }


    switch(event)
    {
        case LE_AUDIO_MEDIA_ENDED:
            LE_INFO("File event is LE_AUDIO_MEDIA_ENDED.");
            if (PlayInLoop)
            {
                le_audio_PlayFile(streamRef, LE_AUDIO_NO_FD);
            }
            break;

        case LE_AUDIO_MEDIA_ERROR:
            LE_INFO("File event is LE_AUDIO_MEDIA_ERROR.");
        break;

        case LE_AUDIO_MEDIA_NO_MORE_SAMPLES:
            LE_INFO("File event is LE_AUDIO_MEDIA_NO_MORE_SAMPLES.");
        break;
        default:
            LE_INFO("File event is %d.", event);
        break;
    }

    if (GainTimerRef)
    {
        le_timer_Stop (GainTimerRef);
        le_timer_Delete (GainTimerRef);
        GainTimerRef = NULL;
    }

    if (MuteTimerRef)
    {
        le_timer_Stop (MuteTimerRef);
        le_timer_Delete (MuteTimerRef);
        le_audio_Unmute(FileAudioRef);
        MuteTimerRef = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect USB audio class to connectors
 *
 */
//--------------------------------------------------------------------------------------------------
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
        LE_ERROR_IF((res!=LE_OK), "Failed to connect USB Rx on Input connector (%s)!",
                    LE_RESULT_TXT(res));
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect USB Tx on Output connector (%s)!",
                    LE_RESULT_TXT(res));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect player to connector
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
        DisconnectAllAudio();
        exit(0);
    }
    else
    {
        LE_INFO("Open file %s with AudioFileFd.%d",  AudioFilePath, AudioFileFd);
    }

    // Play local on output connector.
    FileAudioRef = le_audio_OpenPlayer();
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFilePlayback returns NULL!");

    LE_ERROR_IF((le_audio_SetGain(FileAudioRef, 0x3000) != LE_OK), "Cannot set multimedia gain");


    le_audio_Unmute(FileAudioRef);

    MediaHandlerRef = le_audio_AddMediaHandler(FileAudioRef, MyMediaEventHandler, NULL);

    if (FileAudioRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioOutputConnectorRef, FileAudioRef);
        if(res != LE_OK)
        {
            LE_ERROR("Failed to connect FilePlayback on output connector (%s)!",
                     LE_RESULT_TXT(res));
            return;
        }

        if (strncmp(AudioTestCase,"PB_SAMPLES",10)==0)
        {
            memset(&PbRecSamplesThreadCtx,0,sizeof(PbRecSamplesThreadCtx_t));
            PbRecSamplesThreadCtx.pipefd[0] = -1;
            PbRecSamplesThreadCtx.pipefd[1] = -1;
            PbRecSamplesThreadCtx.mainThreadRef = le_thread_GetCurrent();

            PlaySamples(&PbRecSamplesThreadCtx, NULL);
        }
        else
        {
            LE_INFO("FilePlayback is now connected.");
            res = le_audio_PlayFile(FileAudioRef, AudioFileFd);

            if(res != LE_OK)
            {
                LE_ERROR("Failed to play the file!");
                return;
            }
            else
            {
                LE_INFO("File is now playing");
            }
        }

        ExecuteNextOption();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect recorder to connector
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
        DisconnectAllAudio();
        exit(0);
    }
    else
    {
        LE_INFO("Open file %s with AudioFileFd.%d",  AudioFilePath, AudioFileFd);
    }

    // Capture local on input connector.
    FileAudioRef = le_audio_OpenRecorder();
    LE_ERROR_IF((FileAudioRef==NULL), "OpenFileRecording returns NULL!");

    LE_ERROR_IF((le_audio_SetGain(FileAudioRef, 0x3000) != LE_OK), "Cannot set multimedia gain");


    MediaHandlerRef = le_audio_AddMediaHandler(FileAudioRef, MyMediaEventHandler, NULL);

    if (FileAudioRef && AudioInputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FileAudioRef);
        if(res!=LE_OK)
        {
            LE_ERROR("Failed to connect FileRecording on input connector (%s)!",
                     LE_RESULT_TXT(res));
            return;
        }

        LE_INFO("Recorder is now connected.");

        if ( strncmp(AudioTestCase,"REC_SAMPLES",11)==0 )
        {
            memset(&PbRecSamplesThreadCtx,0,sizeof(PbRecSamplesThreadCtx_t));

            RecSamples();

            ExecuteNextOption();
        }
        else
        {
            res = le_audio_SetEncodingFormat(FileAudioRef, AudioFormat);

            if(res!=LE_OK)
            {
                LE_ERROR("Failed to set audio format");
                return;
            }

            if (AudioFormat == LE_AUDIO_AMR)
            {
                LE_INFO("Set AMR mode %d", AmrMode);
                res = le_audio_SetSampleAmrMode(FileAudioRef, AmrMode);

                if(res!=LE_OK)
                {
                    LE_ERROR("Failed to set AMR bitrate");
                    return;
                }

                LE_INFO("Set AMR DTX %d", DtxActivation);
                res = le_audio_SetSampleAmrDtx(FileAudioRef, DtxActivation);

                if(res!=LE_OK)
                {
                    LE_ERROR("Failed to set DTX");
                    return;
                }
            }

            res = le_audio_RecordFile(FileAudioRef, AudioFileFd);

            if(res!=LE_OK)
            {
                LE_ERROR("Failed to record the file");
                return;
            }
            else
            {
                LE_INFO("File is now recording.");
            }

            sleep(1);

            LE_INFO("Try again to record");

            LE_ASSERT(le_audio_RecordFile(FileAudioRef, LE_AUDIO_NO_FD) != LE_OK);

            ExecuteNextOption();
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect speaker and MIC to connectors
 *
 */
//--------------------------------------------------------------------------------------------------
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
        LE_ERROR_IF((res!=LE_OK), "Failed to connect Mic on Input connector (%s)!",
                    LE_RESULT_TXT(res));
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect Speaker on Output connector (res %s)!",
                    LE_RESULT_TXT(res));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect PCM to connectors
 *
 */
//--------------------------------------------------------------------------------------------------
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
        LE_ERROR_IF((res!=LE_OK), "Failed to connect PCM RX on Input connector (%s)!",
                    LE_RESULT_TXT(res));
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect PCM TX on Output connector (%s)!",
                    LE_RESULT_TXT(res));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect I2S to the connectors
 *
 */
//--------------------------------------------------------------------------------------------------
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
        LE_ERROR_IF((res!=LE_OK), "Failed to connect I2S RX on Input connector (%s)!",
                    LE_RESULT_TXT(res));
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect I2S TX on Output connector (%s)!",
                    LE_RESULT_TXT(res));
    }
    LE_INFO("Open I2s: FeInRef.%p FeOutRef.%p", FeInRef, FeOutRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect audio
 *
 */
//--------------------------------------------------------------------------------------------------
static void ConnectAudio
(
    void
)
{
    if ((strncmp(AudioTestCase,"PB",2)==0) || (strncmp(AudioTestCase,"REC",3)==0))
    {
        if (strcmp(MainAudioSoundPath,"MIC")==0)
        {
            LE_INFO("Connect MIC and SPEAKER ");
            ConnectAudioToCodec();
        }
        else
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
        if (strncmp(AudioTestCase,"PB",2)==0)
        {
            LE_INFO("Connect Local Play");
            ConnectAudioToFileLocalPlay();
        }
        else if (strncmp(AudioTestCase,"REC",3)==0)
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

//--------------------------------------------------------------------------------------------------
/**
 * Disconnect all streams and connectors
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

    // Closing AudioFileFd is unnecessary since the messaging infrastructure underneath
    // le_audio_xxx APIs that use it would close it.

    exit(0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Helper.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage
(
    void
)
{
    int idx;
    bool sandboxed = (getuid() != 0);
    const char * usagePtr[] = {
            "Usage of the audioPlaybackRec test is:",
            "   app runProc audioPlaybackRec --exe=audioPlaybackRecTest -- <test case>"
              "[main audio path] [file's name] [option]",
            "",
            "Test cases are:",
            " - PB (for Local playback)",
            " - REC (for Local recording)",
            " - PB_SAMPLES (for Local samples play)",
            " - REC_SAMPLES (for Local samples recording) [option]",
            "",
            "Main audio paths are: (for file playback/recording only)",
            " - MIC (for mic/speaker)",
            " - PCM (not supported on mangOH board - for AR755x, AR8652 devkit's codec use, "
                "execute 'wm8940_demo --pcm' command)",
            " - I2S (not supported on mangOH board - for AR755x, AR8652 devkit's codec use, "
                "execute 'wm8940_demo --i2s' command)",
            " - USB (for USB)",
            "",
            "Options are:",
            " - ChannelNmbr SampleRate BitsPerSample (for REC_SAMPLES)",
            " - AMR AmrMode DTX (for REC in AMR Narrowband format)",
            " - WAV (for REC in WAV format)",
            " - GAIN (for playback gain testing)",
            " - LOOP (to replay a file in loop) (optional)",
            " - PLAY=<timer value> (to replay a file after a delay) (optional)",
            " - RECORD=<timer value> (to record a file after a delay) (optional)",
            " - STOP=<timer value> (to stop a file playback/capture after a delay) (optional)",
            " - PAUSE=<timer value> (to pause a file playback/capture after a delay) (optional)",
            " - RESUME=<timer value> (to resume a file playback/capture after a delay) (optional)",
            " - DISCONNECT=<timer value> (to disconnect connectors and streams"
              " after a delay) (optional)",
            " - MUTE (for playback MUTE testing)",
            "",
            "File's name can be the complete file's path.",
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
 * The signal event handler function for SIGINT/SIGTERM when process dies.
 */
//--------------------------------------------------------------------------------------------------
static void SigHandler
(
    int sigNum
)
{
    LE_INFO("Disconnect All Audio and end test");
    if (RecPbThreadRef)
    {
        le_thread_Cancel(RecPbThreadRef);
        RecPbThreadRef = NULL;
    }
    DisconnectAllAudio();
    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the test component.
 *
 * Execute application with 'app runProc audioPlaybackRec --exe=audioPlaybackRecTest -- [options]
 * (see PrintUsage())'
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Init");

    if (le_arg_NumArgs() >= 1)
    {
        // Register a signal event handler for SIGINT when user interrupts/terminates process
        signal(SIGINT, SigHandler);

        LE_INFO("======== Start Audio implementation Test (audioPlaybackRecTest) ========");

        AudioTestCase = le_arg_GetArg(0);
        if (NULL == AudioTestCase)
        {
            LE_INFO("AudioTestCase is NULL");
            exit(EXIT_FAILURE);
        }
        LE_INFO("   Test case.%s", AudioTestCase);

        if(le_arg_NumArgs() >= 3)
        {
            MainAudioSoundPath = le_arg_GetArg(1);
            AudioFilePath = le_arg_GetArg(2);
            LE_INFO("   Main audio path.%s", MainAudioSoundPath);
            LE_INFO("   Audio file [%s]", AudioFilePath);
        }

        if ( (strncmp(AudioTestCase,"REC_SAMPLES",11)==0) ||
             (strncmp(AudioTestCase,"PB_SAMPLES",10)==0) )
        {
            if(le_arg_NumArgs() < 6)
            {
                PrintUsage();
                LE_INFO("EXIT audioPlaybackRec");
                exit(EXIT_FAILURE);
            }
            const char* channelsCountPtr  = le_arg_GetArg(3);
            const char* sampleRatePtr    = le_arg_GetArg(4);
            const char* bitsPerSamplePtr = le_arg_GetArg(5);

            if (NULL == channelsCountPtr)
            {
                LE_ERROR("channelsCountPtr is NULL");
                exit(EXIT_FAILURE);
            }
            if (NULL == sampleRatePtr)
            {
                LE_ERROR("sampleRatePtr is NULL");
                exit(EXIT_FAILURE);
            }
            if (NULL == bitsPerSamplePtr)
            {
                LE_ERROR("bitsPerSamplePtr is NULL");
                exit(EXIT_FAILURE);
            }
            ChannelsCount = atoi(channelsCountPtr);
            SampleRate = atoi(sampleRatePtr);
            BitsPerSample = atoi(bitsPerSamplePtr);
            LE_INFO("   Get/Play PCM samples with ChannelsCount.%d SampleRate.%d BitsPerSample.%d",
                    ChannelsCount, SampleRate, BitsPerSample);
            NextOptionArg = 6;
        }
        else if (strncmp(AudioTestCase,"REC", 3)==0)
        {
            const char* recFormat = le_arg_GetArg(3);

            if (NULL == recFormat)
            {
                LE_ERROR("recFormat is NULL");
                exit(EXIT_FAILURE);
            }
            if (strncmp(recFormat,"WAV", 3)==0)
            {
                AudioFormat = LE_AUDIO_WAVE;
            }
            else if (strncmp(recFormat,"AMR", 3)==0)
            {
                AudioFormat = LE_AUDIO_AMR;
            }
            else
            {
                PrintUsage();
                LE_INFO("EXIT audioPlaybackRec");
                exit(EXIT_FAILURE);
            }

            if (AudioFormat == LE_AUDIO_WAVE)
            {
                NextOptionArg = 4;
            }
            else
            {
                const char* amrModePtr       = le_arg_GetArg(4);
                const char* dtxActivationPtr = le_arg_GetArg(5);
                if (NULL == amrModePtr)
                {
                    LE_ERROR("amrModePtr is NULL");
                    exit(EXIT_FAILURE);
                }
                if (NULL == dtxActivationPtr)
                {
                    LE_ERROR("dtxActivationPtr is NULL");
                    exit(EXIT_FAILURE);
                }

                AmrMode = atoi(amrModePtr);
                DtxActivation = atoi(dtxActivationPtr);
                NextOptionArg = 6;
            }
        }
        else
        {
            NextOptionArg = 3;
        }


        OptionTimerRef = le_timer_Create("OptionTimer");
        le_timer_SetHandler(OptionTimerRef, OptionTimerHandler);

        ConnectAudio();

        LE_INFO("======== Audio implementation Test (audioPlaybackRec) started successfully ========");
    }
    else
    {
        PrintUsage();
        LE_INFO("EXIT audioPlaybackRec");
        exit(EXIT_FAILURE);
    }
}

