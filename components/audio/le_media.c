//--------------------------------------------------------------------------------------------------
/**
 * @file le_media.c
 *
 * This file contains the source code of the high level Audio API for playback / capture
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "le_audio_local.h"
#include "le_media_local.h"
#include "pa_audio.h"
#include "pa_amr.h"
#include "pa_pcm.h"
#include <math.h>


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Values used for DTMF sampling.
 */
//--------------------------------------------------------------------------------------------------
#define SAMPLE_SCALE    (32767)
#define DTMF_AMPLITUDE  (40)
#if !defined (PI)
#define PI 3.14159265358979323846264338327
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Symbols used to populate wave header file.
 */
//--------------------------------------------------------------------------------------------------
#define ID_RIFF    0x46464952
#define ID_WAVE    0x45564157
#define ID_FMT     0x20746d66
#define ID_DATA    0x61746164
#define FORMAT_PCM 1

//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * The wave header file structure.
 */
//--------------------------------------------------------------------------------------------------
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

//--------------------------------------------------------------------------------------------------
/**
 * DTMF resource structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t       sampleRate; ///< Sample frequency in Hertz
    uint32_t       duration;   ///< The DTMF duration in milliseconds.
    uint32_t       pause;      ///< The pause duration between tones in milliseconds.
    bool           playPause;  ///< Play the pause
    char           dtmf[LE_AUDIO_DTMF_MAX_BYTES];    ///< The DTMFs to play.
    uint32_t       currentDtmf;///< Index of the play dtmf
}
DtmfParams_t;

//--------------------------------------------------------------------------------------------------
/**
 * WAV resource structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t recordingSize;      ///< recording Wav file size
}
WavParams_t;

//--------------------------------------------------------------------------------------------------
/**
 * Playback/Capture Control enumeration.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    STOP,    // Stop playback/capture
    PLAY,    // Play playback/capture
    PAUSE,   // Pause playback/capture
    RESUME,  // Resume playback/capture
    FLUSH    // Flush playback stream
}
ControlOperation_t;

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for the DTMF parameters
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t DtmfParamsPool;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for the WAV parameters
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t WavParamsPool;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for the Media thread context
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t MediaThreadContextPool;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for the playback/capture threads data parameters objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PcmThreadContextPool;

//--------------------------------------------------------------------------------------------------
/**
 *  Return the low frequency component of a DTMF character.
 *
 */
//--------------------------------------------------------------------------------------------------
static inline uint32_t Digit2LowFreq
(
    int digit
)
{
    switch (digit)
    {

        case '1':
        case '2':
        case '3':
        case 'a':
        case 'A':
            return 697;

        case '4':
        case '5':
        case '6':
        case 'b':
        case 'B':
            return 770;

        case '7':
        case '8':
        case '9':
        case 'c':
        case 'C':
            return 852;

        case '*':
        case '0':
        case '#':
        case 'd':
        case 'D':
            return 941;

        default:
            return 0;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Return the high frequency component of a DTMF character.
 *
 */
//--------------------------------------------------------------------------------------------------
static inline uint32_t Digit2HighFreq
(
    int digit
)
{
    switch (digit)
    {

        case '1':
        case '4':
        case '7':
        case '*':
            return 1209;

        case '2':
        case '5':
        case '8':
        case '0':
            return 1336;

        case '3':
        case '6':
        case '9':
        case '#':
            return 1477;

        case 'a':
        case 'A':
        case 'b':
        case 'B':
        case 'c':
        case 'C':
        case 'd':
        case 'D':
            return 1633;

        default:
            return 0;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Add two 16-bit values.
 *
 */
//--------------------------------------------------------------------------------------------------
static inline int16_t SaturateAdd16
(
    int32_t a,
    int32_t b
)
{
    int32_t tot=a+b;

    if (tot > 32767)
    {
        return 32767;
    }
    else if (tot < -32768)
    {
        return -32768;
    }
    else
    {
        return (tot & 0xFFFF);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Play Tone function.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t PlayTone
(
    le_audio_MediaThreadContext_t* mediaCtxPtr,  ///< [IN] Media thread context
    uint8_t*                       bufferOutPtr, ///< [OUT] dtmf samples buffer output
    uint32_t*                      bufferLenPtr  ///< [OUT] Length of the buffer
)
{
    double   d1, d2;
    uint32_t i;

    DtmfParams_t*  dtmfParamsPtr = (DtmfParams_t*) mediaCtxPtr->codecParams;
    uint32_t samplesCount;
    uint32_t freq1;
    uint32_t freq2;
    int32_t  amp1;
    int32_t  amp2;
    uint16_t* dataPtr = (uint16_t*) bufferOutPtr;

    if (dtmfParamsPtr->playPause)
    {
        freq1 = 0;
        freq2 = 0;
        amp1 = 0;
        amp2 = 0;
        samplesCount = dtmfParamsPtr->sampleRate*dtmfParamsPtr->pause*2/1000;
    }
    else
    {
        if (dtmfParamsPtr->currentDtmf == strlen(dtmfParamsPtr->dtmf))
        {
            LE_DEBUG("All dtmf played");
            *bufferLenPtr = 0;
            return LE_UNDERFLOW;
        }

        LE_DEBUG("Play %c", dtmfParamsPtr->dtmf[dtmfParamsPtr->currentDtmf]);

        freq1 = Digit2LowFreq(dtmfParamsPtr->dtmf[dtmfParamsPtr->currentDtmf]);
        freq2 = Digit2HighFreq(dtmfParamsPtr->dtmf[dtmfParamsPtr->currentDtmf]);
        amp1 = DTMF_AMPLITUDE;
        amp2 = DTMF_AMPLITUDE;
        samplesCount = dtmfParamsPtr->sampleRate*dtmfParamsPtr->duration*2/1000;
        dtmfParamsPtr->currentDtmf++;
    }

    *bufferLenPtr = samplesCount;
    d1 = 1.0f * freq1 / dtmfParamsPtr->sampleRate;
    d2 = 1.0f * freq2 / dtmfParamsPtr->sampleRate;

    for (i=0; i<samplesCount; i++)
    {
        int16_t s1, s2;

        s1 = (int16_t)(SAMPLE_SCALE * amp1 / 100.0f * sin(2 * PI * d1 * i));
        s2 = (int16_t)(SAMPLE_SCALE * amp2 / 100.0f * sin(2 * PI * d2 * i));
        dataPtr[i] = SaturateAdd16(s1, s2);
    }

    *bufferLenPtr = samplesCount;

    if (dtmfParamsPtr->playPause)
    {
        dtmfParamsPtr->playPause = false;
    }
    else
    {
        if (dtmfParamsPtr->pause)
        {
            dtmfParamsPtr->playPause = true;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Populate wave header file
 *
 * @return LE_OK    on success
 * @return LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetWavHeader
(
    int32_t      fd,
    le_audio_SamplePcmConfig_t* pcmConfigPtr
)
{
    WavHeader_t hdr;

    memset(&hdr, 0, sizeof(WavHeader_t));
    hdr.riffId = ID_RIFF;
    hdr.riffFmt = ID_WAVE;
    hdr.fmtId = ID_FMT;
    hdr.fmtSize = 16;
    hdr.audioFormat = FORMAT_PCM;
    hdr.channelsCount = pcmConfigPtr->channelsCount;
    hdr.sampleRate = pcmConfigPtr->sampleRate;
    hdr.bitsPerSample = pcmConfigPtr->bitsPerSample;
    hdr.byteRate = pcmConfigPtr->byteRate;
    hdr.blockAlign = ( hdr.bitsPerSample * pcmConfigPtr->channelsCount ) / 8;
    hdr.dataId = ID_DATA;
    hdr.dataSize = 0;
    hdr.riffSize = hdr.dataSize + 44 - 8;
    if (write(fd, &hdr, sizeof(hdr)) != sizeof(hdr))
    {
        LE_ERROR("Cannot write wave header");
        return LE_FAULT;
    }
    else
    {
        LE_INFO("Wav header set with %d ch, %d Hz, %d bit, %s",
                hdr.channelsCount, hdr.sampleRate, hdr.bitsPerSample,
                hdr.audioFormat == FORMAT_PCM ? "PCM" : "unknown");
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a file descriptor.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReadFd
(
    le_audio_MediaThreadContext_t* mediaCtxPtr,  ///< [IN] Media thread context
    uint8_t*                       bufferOutPtr, ///< [OUT] Decoding samples buffer output
    uint32_t*                      readLenPtr    ///< [OUT] Length of the read data
)
{
    int len = 1;
    *readLenPtr = 0;

    while ((len != 0) && (*readLenPtr != mediaCtxPtr->bufferSize))
    {
        len = read( mediaCtxPtr->fd_in,
                    bufferOutPtr+(*readLenPtr),
                    mediaCtxPtr->bufferSize-(*readLenPtr));

        if (len < 0)
        {
            LE_ERROR("Read error fd=%d, errno %d", mediaCtxPtr->fd_in, errno);
            return LE_FAULT;
        }

        (*readLenPtr) += len;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a file descriptor for capture.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReadFdForCapture
(
    le_audio_MediaThreadContext_t* mediaCtxPtr,  ///< [IN] Media thread context
    uint8_t*                       bufferOutPtr, ///< [OUT] Decoding samples buffer output
    uint32_t*                      readLenPtr    ///< [OUT] Length of the read data
)
{
    le_result_t result = LE_OK;
    *readLenPtr = 0;

    while ( (result == LE_OK) && (*readLenPtr != mediaCtxPtr->bufferSize) )
    {
        int32_t len = read(mediaCtxPtr->fd_in,
                           bufferOutPtr+*readLenPtr,
                           mediaCtxPtr->bufferSize-*readLenPtr);

        if (len < 0)
        {
            LE_DEBUG("read error %d", len);
            result = LE_FAULT;
        }
        else if (len == 0)
        {
            // nothing else to read
            return LE_OK;
        }
        else
        {
            *readLenPtr += len;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write on a file descriptor.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WriteFd
(
    le_audio_MediaThreadContext_t* mediaCtxPtr,     ///< [IN] Media thread context
    uint8_t*                       bufferInPtr,     ///< [IN] Decoding samples buffer input
    uint32_t                       bufferLen        ///< [IN] Buffer length
)
{
    int len = write(mediaCtxPtr->fd_out, bufferInPtr, bufferLen);

    if (len <= 0)
    {
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write on a file descriptor with AMR encoding.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AmrWriteFd
(
    le_audio_MediaThreadContext_t* mediaCtxPtr,     ///< [IN] Media thread context
    uint8_t*                       bufferInPtr,     ///< [IN] Decoding samples buffer input
    uint32_t                       bufferLen        ///< [IN] Buffer length
)
{
    uint32_t outputBufLen = 500;
    uint8_t outputBuf[outputBufLen];
    le_result_t result = LE_FAULT;

    if (pa_amr_EncodeFrames(mediaCtxPtr,
                            bufferInPtr,
                            bufferLen,
                            outputBuf,
                            &outputBufLen) == LE_OK)
    {
        int32_t writeLen = write( mediaCtxPtr->fd_out, outputBuf, outputBufLen );

        if (writeLen != outputBufLen)
        {
            LE_ERROR("write error %d", writeLen);
            result = LE_FAULT;
        }
        else
        {
            result = LE_OK;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write on a file descriptor a WAV audio file.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WavWriteFd
(
    le_audio_MediaThreadContext_t* mediaCtxPtr,  ///< [IN] Media thread context
    uint8_t*                       bufferInPtr,  ///< [IN] Decoding samples buffer input
    uint32_t                       bufferLen     ///< [IN] Buffer length
)
{
    WavHeader_t hdr;
    WavParams_t* wavParamPtr =  (WavParams_t*) mediaCtxPtr->codecParams;
    int oldstate = PTHREAD_CANCEL_ENABLE, dummy = PTHREAD_CANCEL_ENABLE;

    // This function is set to no cancelable to avoid desynchronisation between the data and the
    // header
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);

    int32_t len = write(mediaCtxPtr->fd_out, bufferInPtr, bufferLen);

    if (len != bufferLen)
    {
        LE_ERROR("write error: %d written, expected %d, errno %d", len, bufferLen, errno);
        pthread_setcancelstate(oldstate, &dummy);
        return LE_FAULT;
    }

    wavParamPtr->recordingSize += len;

    lseek(mediaCtxPtr->fd_out, ((uint8_t*)&hdr.dataSize - (uint8_t*)&hdr), SEEK_SET);

    len = write(mediaCtxPtr->fd_out,
                &wavParamPtr->recordingSize,
                sizeof(wavParamPtr->recordingSize));

    if (len != sizeof(wavParamPtr->recordingSize))
    {
        LE_ERROR("read error: %d written, errno %d", len, errno);
        pthread_setcancelstate(oldstate, &dummy);
        return LE_FAULT;
    }

    lseek(mediaCtxPtr->fd_out, ((uint8_t*)&hdr.riffSize - (uint8_t*)&hdr), SEEK_SET);

    uint32_t riffSize = wavParamPtr->recordingSize + 44 - 8;

    len = write(mediaCtxPtr->fd_out, &riffSize, sizeof(riffSize));

    if (len != sizeof(riffSize))
    {
        LE_ERROR("read error: %d written, errno %d", len, errno);
        pthread_setcancelstate(oldstate, &dummy);
        return LE_FAULT;
    }

    lseek(mediaCtxPtr->fd_out, sizeof(WavHeader_t)+wavParamPtr->recordingSize, SEEK_SET);

    pthread_setcancelstate(oldstate, &dummy);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Release a codec context.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReleaseCodecParams
(
    le_audio_MediaThreadContext_t* mediaCtxPtr   ///< [IN] Media thread context
)
{
    if (mediaCtxPtr && mediaCtxPtr->codecParams)
    {
        le_mem_Release(mediaCtxPtr->codecParams);
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization for WAVE file reading.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t InitPlayWavFile
(
    le_audio_Stream_t*               streamPtr,    ///< [IN] Stream object
    le_audio_MediaThreadContext_t*   mediaCtxPtr   ///< [IN] Media thread context
)
{
    if ( mediaCtxPtr->format != LE_AUDIO_FILE_WAVE)
    {
        return LE_FAULT;
    }

    mediaCtxPtr->bufferSize = PIPE_BUF/4;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization for WAVE file recording.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t InitRecWavFile
(
    le_audio_Stream_t*               streamPtr,    ///< [IN] Stream object
    le_audio_MediaThreadContext_t*   mediaCtxPtr   ///< [IN] Media thread context
)
{
    SetWavHeader(mediaCtxPtr->fd_out, &(streamPtr->samplePcmConfig));

    mediaCtxPtr->format = LE_AUDIO_FILE_WAVE;
    mediaCtxPtr->bufferSize = 1024;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization for DTMF playing.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t InitPlayDtmf
(
    le_audio_Stream_t*               streamPtr,   ///< [IN] Stream object
    le_audio_MediaThreadContext_t*   mediaCtxPtr  ///< [IN] Media thread context
)
{
    DtmfParams_t*  dtmfParamsPtr = (DtmfParams_t*) mediaCtxPtr->codecParams;


    uint32_t duration = (dtmfParamsPtr->duration > dtmfParamsPtr->pause) ?
                        dtmfParamsPtr->duration : dtmfParamsPtr->pause;

    mediaCtxPtr->bufferSize = dtmfParamsPtr->sampleRate*duration*4/1000;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 *  Media threads destructor.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DestroyMediaThread
(
    void *contextPtr
)
{
    le_audio_Stream_t* streamPtr = (le_audio_Stream_t *) contextPtr;
    le_audio_MediaThreadContext_t * mediaCtxPtr = NULL;

    if (streamPtr)
    {
        mediaCtxPtr = streamPtr->mediaThreadContextPtr;
    }

    LE_DEBUG("DestroyMediaThread running");

    if (mediaCtxPtr)
    {
        mediaCtxPtr->closeFunc(mediaCtxPtr);

        close(mediaCtxPtr->fd_pipe_input);
        close(mediaCtxPtr->fd_pipe_output);
        streamPtr->fd = mediaCtxPtr->fd_arg;

        le_mem_Release(mediaCtxPtr);
        streamPtr->mediaThreadContextPtr = NULL;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Media thread.
 *
 */
//--------------------------------------------------------------------------------------------------
static void* MediaThread
(
    void* contextPtr
)
{
    le_audio_MediaThreadContext_t * mediaCtxPtr = (le_audio_MediaThreadContext_t *) contextPtr;
    uint8_t outBuffer[mediaCtxPtr->bufferSize];
    uint32_t readLen = 0;

    LE_DEBUG("MediaThread");

    if (!mediaCtxPtr->readFunc || !mediaCtxPtr->closeFunc || !mediaCtxPtr->writeFunc)
    {
        LE_ERROR("functions not set !!!");
        return NULL;
    }

    while (1)
    {
        memset(outBuffer,0,mediaCtxPtr->bufferSize);

        /* read/decode the packet */
        if (( mediaCtxPtr->readFunc( mediaCtxPtr,
                                   outBuffer,
                                   &readLen ) == LE_OK ) && readLen )
        {
            if ( mediaCtxPtr->writeFunc( mediaCtxPtr,
                                    outBuffer,
                                    readLen ) != LE_OK )
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    mediaCtxPtr->allDataSent = true;
    LE_DEBUG("MediaThread end");

    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Media thread
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t InitMediaThread
(
    le_audio_Stream_t*          streamPtr,
    le_audio_FileFormat_t       format,
    uint32_t                    fd_in,
    uint32_t                    fd_out
)
{
    le_audio_MediaThreadContext_t * mediaCtxPtr = NULL;

    if (streamPtr)
    {
        mediaCtxPtr = streamPtr->mediaThreadContextPtr;
    }

    if (mediaCtxPtr == NULL)
    {
        return LE_FAULT;
    }

    mediaCtxPtr->fd_in = fd_in;
    mediaCtxPtr->fd_out = fd_out;
    mediaCtxPtr->format = format;

    if ( mediaCtxPtr->initFunc &&
        (mediaCtxPtr->initFunc ( streamPtr,
                                mediaCtxPtr ) != LE_OK))
    {
        LE_ERROR("Failed to init decoder");
        return LE_FAULT;
    }

    char threadName[30]="\0";

    snprintf(threadName, 30, "MediaThread-%p", streamPtr->streamRef);

    streamPtr->mediaThreadRef = le_thread_Create(threadName,
                                                MediaThread,
                                                streamPtr->mediaThreadContextPtr);

    le_thread_SetJoinable(streamPtr->mediaThreadRef);

    le_thread_AddChildDestructor(streamPtr->mediaThreadRef,
                                 DestroyMediaThread,
                                 streamPtr);

    // set the task in real time prioprity
    // [TODO] This resets the audio dameon. To be reactivated when it works.
    // le_thread_SetPriority(streamPtr->mediaThreadRef, LE_THREAD_PRIORITY_RT_1);

    le_thread_Start(streamPtr->mediaThreadRef);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function check the file header to detect a WAV file, and get the PCM configuration.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t PlayWavFile
(
    le_audio_Stream_t*             streamPtr,
    le_audio_SamplePcmConfig_t*    samplePcmConfigPtr,
    le_audio_MediaThreadContext_t* mediaContextPtr,
    le_audio_FileFormat_t*         formatPtr
)
{
    WavHeader_t      hdr;

    if (read(streamPtr->fd, &hdr, sizeof(hdr)) != sizeof(hdr))
    {
        LE_WARN("WAV detection: cannot read header");
        return LE_FAULT;
    }

    if ((hdr.riffId != ID_RIFF)  ||
        (hdr.riffFmt != ID_WAVE) ||
        (hdr.fmtId != ID_FMT)    ||
        (hdr.audioFormat != FORMAT_PCM) ||
        (hdr.fmtSize != 16))
    {
        LE_WARN("WAV detection: unrecognized wav format");
        lseek(streamPtr->fd, -sizeof(hdr), SEEK_CUR);
        return LE_FAULT;
    }

    samplePcmConfigPtr->sampleRate = hdr.sampleRate;
    samplePcmConfigPtr->channelsCount = hdr.channelsCount;
    samplePcmConfigPtr->bitsPerSample = hdr.bitsPerSample;

    mediaContextPtr->initFunc = InitPlayWavFile;
    mediaContextPtr->readFunc = ReadFd;
    mediaContextPtr->writeFunc = WriteFd;
    mediaContextPtr->closeFunc = ReleaseCodecParams;

    *formatPtr = LE_AUDIO_FILE_WAVE;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function check the file header to detect an AMR file, and get the PCM configuration.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t PlayAmrFile
(
    le_audio_Stream_t*             streamPtr,
    le_audio_SamplePcmConfig_t*    samplePcmConfigPtr,
    le_audio_MediaThreadContext_t* mediaContextPtr,
    le_audio_FileFormat_t*         formatPtr
)
{
    char header[10] = {0};

    // Try to detect the 5th first characters
    if ( read(streamPtr->fd, header, 9) != 9 )
    {
        LE_WARN("AMR detection: cannot read header");
        return LE_FAULT;
    }

    if ( strncmp(header, "#!AMR", 5) == 0 )
    {
        *formatPtr = LE_AUDIO_FILE_MAX;

        if ( strncmp(header+5, "-WB\n", 4) == 0 )
        {
            LE_DEBUG("AMR-WB found");
            *formatPtr = LE_AUDIO_FILE_AMR_WB;
        }
        else if ( strncmp(header+5, "-NB\n", 4) == 0 )
        {
            LE_DEBUG("AMR-NB found");
            *formatPtr = LE_AUDIO_FILE_AMR_NB;
        }
        else if ( strncmp(header+5, "\n", 1) == 0 )
        {
            LE_DEBUG("AMR-NB found");
            *formatPtr = LE_AUDIO_FILE_AMR_NB;
            lseek(streamPtr->fd, -3, SEEK_CUR);
        }
        else
        {
            LE_ERROR("Not an AMR file");
            return LE_FAULT;
        }

        if (*formatPtr == LE_AUDIO_FILE_AMR_WB)
        {
            samplePcmConfigPtr->sampleRate = 16000;
        }
        else
        {
            samplePcmConfigPtr->sampleRate = 8000;
        }

        samplePcmConfigPtr->channelsCount = 1;
        samplePcmConfigPtr->bitsPerSample = 16;

        mediaContextPtr->initFunc = pa_amr_StartDecoder;
        mediaContextPtr->readFunc = pa_amr_DecodeFrames;
        mediaContextPtr->writeFunc = WriteFd;
        mediaContextPtr->closeFunc = pa_amr_StopDecoder;

        return LE_OK;
    }

    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function gets the format regarding to the AMR mode.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_audio_FileFormat_t GetFormat
(
    le_audio_AmrMode_t amrMode
)
{
    uint8_t i=0;

    struct
    {
        le_audio_AmrMode_t amrMode;
        le_audio_FileFormat_t format;
    }   EncodingFormat[] = {
                                { LE_AUDIO_AMR_NB_4_75_KBPS,        LE_AUDIO_FILE_AMR_NB     },
                                { LE_AUDIO_AMR_NB_5_15_KBPS,        LE_AUDIO_FILE_AMR_NB     },
                                { LE_AUDIO_AMR_NB_5_9_KBPS,         LE_AUDIO_FILE_AMR_NB     },
                                { LE_AUDIO_AMR_NB_6_7_KBPS,         LE_AUDIO_FILE_AMR_NB     },
                                { LE_AUDIO_AMR_NB_7_4_KBPS,         LE_AUDIO_FILE_AMR_NB     },
                                { LE_AUDIO_AMR_NB_7_95_KBPS,        LE_AUDIO_FILE_AMR_NB     },
                                { LE_AUDIO_AMR_NB_10_2_KBPS,        LE_AUDIO_FILE_AMR_NB     },
                                { LE_AUDIO_AMR_NB_12_2_KBPS,        LE_AUDIO_FILE_AMR_NB     },
                                { LE_AUDIO_AMR_WB_6_6_KBPS,         LE_AUDIO_FILE_AMR_WB     },
                                { LE_AUDIO_AMR_WB_8_85_KBPS,        LE_AUDIO_FILE_AMR_WB     },
                                { LE_AUDIO_AMR_WB_12_65_KBPS,       LE_AUDIO_FILE_AMR_WB     },
                                { LE_AUDIO_AMR_WB_14_25_KBPS,       LE_AUDIO_FILE_AMR_WB     },
                                { LE_AUDIO_AMR_WB_15_85_KBPS,       LE_AUDIO_FILE_AMR_WB     },
                                { LE_AUDIO_AMR_WB_18_25_KBPS,       LE_AUDIO_FILE_AMR_WB     },
                                { LE_AUDIO_AMR_WB_19_85_KBPS,       LE_AUDIO_FILE_AMR_WB     },
                                { LE_AUDIO_AMR_WB_23_05_KBPS,       LE_AUDIO_FILE_AMR_WB     },
                                { LE_AUDIO_AMR_WB_23_85_KBPS,       LE_AUDIO_FILE_AMR_WB     },
                                { 0,                                LE_AUDIO_FILE_MAX        }
                            };

    while (EncodingFormat[i].format != LE_AUDIO_FILE_MAX)
    {
        if (EncodingFormat[i].amrMode == amrMode)
        {
            return EncodingFormat[i].format;
        }

        i++;
    }

    return LE_AUDIO_FILE_MAX;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function starts the file recording in AMR format.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RecordAmrFile
(
    le_audio_Stream_t*             streamPtr,
    le_audio_SamplePcmConfig_t*    samplePcmConfigPtr,
    le_audio_MediaThreadContext_t* mediaCtxPtr,
    le_audio_FileFormat_t*         formatPtr
)
{
    *formatPtr = GetFormat(streamPtr->sampleAmrConfig.amrMode);

    if ((*formatPtr != LE_AUDIO_FILE_AMR_NB) && (*formatPtr != LE_AUDIO_FILE_AMR_WB))
    {
        LE_ERROR("Bad AMR mode");
        return LE_FAULT;
    }

    samplePcmConfigPtr->channelsCount = 1;
    samplePcmConfigPtr->bitsPerSample = 16;

    if (*formatPtr == LE_AUDIO_FILE_AMR_WB)
    {
        samplePcmConfigPtr->sampleRate = 16000;
    }
    else
    {
        samplePcmConfigPtr->sampleRate = 8000;
    }

    mediaCtxPtr->initFunc = pa_amr_StartEncoder;
    mediaCtxPtr->readFunc = ReadFdForCapture;
    mediaCtxPtr->writeFunc = AmrWriteFd;
    mediaCtxPtr->closeFunc = pa_amr_StopEncoder;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function starts the file recording in WAV format.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RecordWavFile
(
    le_audio_Stream_t*             streamPtr,
    le_audio_SamplePcmConfig_t*    samplePcmConfigPtr,
    le_audio_MediaThreadContext_t* mediaCtxPtr,
    le_audio_FileFormat_t*         formatPtr
)
{
    streamPtr->samplePcmConfig.byteRate = ( streamPtr->samplePcmConfig.sampleRate *
                                            streamPtr->samplePcmConfig.channelsCount *
                                            streamPtr->samplePcmConfig.bitsPerSample ) /8;

    memcpy( samplePcmConfigPtr,
            &streamPtr->samplePcmConfig,
            sizeof(le_audio_SamplePcmConfig_t));

    WavParams_t*  WavParamsPtr = le_mem_ForceAlloc(WavParamsPool);

    memset(WavParamsPtr,0,sizeof(WavParams_t));

    mediaCtxPtr->codecParams = (le_audio_Codec_t) WavParamsPtr;
    mediaCtxPtr->initFunc = InitRecWavFile;
    mediaCtxPtr->readFunc = ReadFdForCapture;
    mediaCtxPtr->writeFunc = WavWriteFd;
    mediaCtxPtr->closeFunc = ReleaseCodecParams;
    *formatPtr = LE_AUDIO_FILE_WAVE;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Playback/Capture thread destructor
 *
 */
//--------------------------------------------------------------------------------------------------
static void DestroyPlayCaptThread
(
    void *contextPtr
)
{
    le_audio_Stream_t* streamPtr = (le_audio_Stream_t *) contextPtr;
    le_audio_PcmThreadContext_t* resourcePtr = NULL;

    LE_DEBUG("DestroyPlayCaptThread running");

    if (streamPtr)
    {
        resourcePtr = streamPtr->pcmThreadContextPtr;
    }

    if (resourcePtr)
    {
        if (NULL != (void*) resourcePtr->pcmHandle)
        {
            if (resourcePtr->interface == LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY)
            {
                // Avoid starvation issue on driver side
                uint32_t bufsize = pa_pcm_GetPeriodSize(resourcePtr->pcmHandle);
                char     data[bufsize];
                memset(data, 0, bufsize);

                if (pa_pcm_Write(resourcePtr->pcmHandle, data, bufsize) != LE_OK)
                {
                    LE_ERROR("Could not write %d void bytes!", bufsize);
                }
            }

            pa_pcm_Close(resourcePtr->pcmHandle);
            resourcePtr->pcmHandle = NULL;
        }

        if (resourcePtr->timerRef)
        {
            le_timer_Delete(resourcePtr->timerRef);
            resourcePtr->timerRef = NULL;
        }

        le_sem_Delete(resourcePtr->threadSemaphore);

        le_mem_Release(resourcePtr);
        streamPtr->pcmThreadContextPtr = NULL;
    }

    LE_DEBUG("Playback/Capture Thread stopped");
}



//--------------------------------------------------------------------------------------------------
/**
 * Control the playback thread (pause/resume)
 *
 */
//--------------------------------------------------------------------------------------------------
static void PlayThreadControl
(
    void *param1Ptr,
    void *param2Ptr
)
{
    le_audio_PcmThreadContext_t* threadContextPtr = (le_audio_PcmThreadContext_t*) param1Ptr;
    ControlOperation_t operation = (ControlOperation_t) param2Ptr;

    LE_DEBUG("operation: %d", operation);

    switch (operation)
    {
        case PAUSE:
            // stop the timer to make a pause
            if (le_timer_IsRunning(threadContextPtr->timerRef) && !threadContextPtr->pause)
            {
                threadContextPtr->pause = true;
                threadContextPtr->operationResult = LE_OK;
            }
            else
            {
                LE_ERROR("Timer is not running");
                threadContextPtr->operationResult = LE_FAULT;
            }
        break;

        case RESUME:
            // start the timer to resume the playback
            if (le_timer_IsRunning(threadContextPtr->timerRef) && threadContextPtr->pause)
            {
                threadContextPtr->pause = false;
                threadContextPtr->operationResult = LE_OK;
            }
            else
            {
                LE_ERROR("Timer is running");
                threadContextPtr->operationResult = LE_FAULT;
            }
        break;

        case FLUSH:
            // flush the audio stream
            if (le_timer_IsRunning(threadContextPtr->timerRef))
            {
                char data[4096];
                int mask;
                if (le_timer_Stop(threadContextPtr->timerRef) != LE_OK)
                {
                    LE_ERROR("le_timer_Stop error");
                    threadContextPtr->operationResult = LE_FAULT;
                    break;
                }
                if ((mask = fcntl(threadContextPtr->fd, F_GETFL, 0)) == -1)
                {
                    LE_ERROR("fcntl error, errno.%d (%s)", errno, strerror(errno));
                    threadContextPtr->operationResult = LE_FAULT;
                    break;
                }
                if (fcntl(threadContextPtr->fd, F_SETFL, mask | O_NONBLOCK) == -1)
                {
                    LE_ERROR("fcntl error, errno.%d (%s)", errno, strerror(errno));
                    threadContextPtr->operationResult = LE_FAULT;
                    break;
                }

                ssize_t len = 1;

                while ((len != -1) && (len != 0))
                {
                    len = read(threadContextPtr->fd, data, 4096);
                }

                if (fcntl(threadContextPtr->fd, F_SETFL, mask) == -1)
                {
                    LE_ERROR("fcntl error, errno.%d (%s)", errno, strerror(errno));
                    threadContextPtr->operationResult = LE_FAULT;
                    break;
                }
                if (le_timer_Start(threadContextPtr->timerRef) != LE_OK)
                {
                    LE_ERROR("le_timer_Start error");
                    threadContextPtr->operationResult = LE_FAULT;
                    break;
                }
                threadContextPtr->operationResult = LE_OK;
                LE_INFO("Flush audio!");
            }
            else
            {
                LE_ERROR("Timer is not running");
                threadContextPtr->operationResult = LE_FAULT;
            }
        break;

        default:
            // This shouldn't occur
            LE_ERROR("Bad asked operation %d", operation);
        break;
    }

    LE_DEBUG("end operation: %d res: %d", operation, threadContextPtr->operationResult);

    le_sem_Post(threadContextPtr->threadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * treat events sent by playback thread
 *
 */
//--------------------------------------------------------------------------------------------------
void PlayCaptTreatEvent
(
    void* param1Ptr,
    void* param2Ptr
)
{
    le_audio_Stream_t* streamPtr = param1Ptr;
    le_audio_PcmThreadContext_t* pcmThreadContextPtr = streamPtr->pcmThreadContextPtr;
    le_audio_MediaThreadContext_t* mediaThreadContextPtr = streamPtr->mediaThreadContextPtr;
    bool sendEvent = true;

    if (!streamPtr || !pcmThreadContextPtr)
    {
        LE_ERROR("streamPtr or pcmThreadContextPtr is null !!!");
        return;
    }

    LE_DEBUG("mediaEvent %d playFile %d", pcmThreadContextPtr->mediaEvent, streamPtr->playFile);

    if (pcmThreadContextPtr->mediaEvent != LE_AUDIO_MEDIA_ERROR)
    {
        if (streamPtr->playFile)
        {
            if (!mediaThreadContextPtr)
            {
                LE_ERROR("mediaThreadContextPtr null !!!");
            }
            else
            {
                LE_DEBUG("allDataSent %d", mediaThreadContextPtr->allDataSent);
                if (mediaThreadContextPtr->allDataSent)
                {
                    le_media_Stop(streamPtr);
                }
                else
                {
                    sendEvent = false;
                }
            }
        }
    }
    else
    {
        le_media_Stop(streamPtr);
    }

    if (sendEvent)
    {
        le_audio_StreamEvent_t streamEvent;
        streamEvent.streamPtr = streamPtr;
        streamEvent.streamEvent = LE_AUDIO_BITMASK_MEDIA_EVENT;
        streamEvent.event.mediaEvent = pcmThreadContextPtr->mediaEvent;

        le_event_Report(streamPtr->streamEventId,
                            &streamEvent,
                            sizeof(le_audio_StreamEvent_t));
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Capture thread
 *
 */
//--------------------------------------------------------------------------------------------------
static void* CaptureThread
(
    void* contextPtr
)
{
    le_audio_Stream_t*          streamPtr = contextPtr;
    le_audio_PcmThreadContext_t* threadContextPtr = streamPtr->pcmThreadContextPtr;
    int32_t fd;
    uint32_t bufsize;
    le_audio_MediaEvent_t mediaEvent = LE_AUDIO_MEDIA_MAX;

    threadContextPtr->operationResult = LE_OK;

    fd = threadContextPtr->fd;

    LE_DEBUG("Open capture on fd.%d device id %d", fd, streamPtr->deviceIdentifier);

    pcm_Handle_t pcmHandle = NULL;

    char deviceString[10]="\0";
    snprintf(deviceString,10,"hw:0,%d", streamPtr->hwDeviceId);
    LE_DEBUG("Hardware interface: %s", deviceString);

    if ((pa_pcm_InitCapture(&pcmHandle, deviceString, &(threadContextPtr->pcmConfig)) != LE_OK) ||
        (pcmHandle == NULL))
    {
        LE_ERROR("PCM cannot be open");
        threadContextPtr->operationResult = LE_FAULT;
    }

    le_sem_Post(threadContextPtr->threadSemaphore);

    if (threadContextPtr->operationResult == LE_OK)
    {
        threadContextPtr->pcmHandle = pcmHandle;
        bufsize = pa_pcm_GetPeriodSize(pcmHandle);

        LE_DEBUG("bufsize.%d", bufsize);

        char data[bufsize];
        memset(data,0,bufsize);

        // Start acquisition
        while (pa_pcm_Read(pcmHandle, data, bufsize) == LE_OK)
        {
            if ( !threadContextPtr->pause )
            {
                if (write(fd, data, bufsize) != bufsize)
                {
                    LE_ERROR("Could not write %d bytes!", bufsize);
                    break;
                }
            }
        }

        /* error treatment */
        LE_ERROR("Error notification and kill");

       mediaEvent = LE_AUDIO_MEDIA_ERROR;

        threadContextPtr->mediaEvent = mediaEvent;

        le_event_QueueFunctionToThread( threadContextPtr->mainThreadRef,
                                        PlayCaptTreatEvent,
                                        streamPtr,
                                        NULL );
    }

    le_event_RunLoop();
    // Should never happened
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Playback timer handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void PlaybackThreadTimer
(
    le_timer_Ref_t timerRef
)
{
    le_audio_Stream_t* streamPtr = le_timer_GetContextPtr(timerRef);
    le_audio_PcmThreadContext_t* threadContextPtr = streamPtr->pcmThreadContextPtr;
    le_audio_MediaEvent_t mediaEvent = LE_AUDIO_MEDIA_MAX;
    uint32_t bufsize = pa_pcm_GetPeriodSize(threadContextPtr->pcmHandle);
    char data[bufsize];

    memset(data,0,bufsize);

    if (bufsize)
    {
        int32_t len;
        fd_set rfds;
        struct timeval tv;
        int ret;

        // Select on fd to check when there is no more samples to read
        do
        {
            FD_ZERO(&rfds);
            FD_SET(threadContextPtr->fd, &rfds);
            tv.tv_sec = 0;
            tv.tv_usec =  threadContextPtr->timerUsec/2;
            ret = select(threadContextPtr->fd+1, &rfds, NULL, NULL, &tv);
        }
        while ((ret == -1) && (errno == EINTR));

        if ((ret == 1) && (FD_ISSET(threadContextPtr->fd, &rfds)))
        {
            threadContextPtr->isNoMoreSamplesEventSent = false;

            // reading file descriptor
            if (threadContextPtr->pause)
            {
                memset(data, 0, bufsize);
                len = bufsize;
            }
            else
            {
                len = read(threadContextPtr->fd, data, bufsize);
            }

            if (len == bufsize)
            {
                if (pa_pcm_Write(threadContextPtr->pcmHandle, data, len) != LE_OK)
                {
                    LE_ERROR("Could not write %d bytes!", len);
                    mediaEvent = LE_AUDIO_MEDIA_ERROR;
                }
            }
            else if (len < 0)
            {
                LE_ERROR("Could not read %d bytes, errno.%d, %s", bufsize, errno, strerror(errno));
                mediaEvent = LE_AUDIO_MEDIA_ERROR;
            }
            else
            {
                // underrun error
                LE_DEBUG("Not enough data for pcm_write!");
            }
        }
        else if (ret == 0)
        {
            if(!threadContextPtr->isNoMoreSamplesEventSent)
            {
                // send no more samples event
                LE_WARN("No more samples to read!");
                mediaEvent = LE_AUDIO_MEDIA_NO_MORE_SAMPLES;
                threadContextPtr->isNoMoreSamplesEventSent = true;
            }
        }
        else
        {
            LE_ERROR("Select error, errno.%d, %s", errno, strerror(errno));
            mediaEvent = LE_AUDIO_MEDIA_ERROR;
        }
    }
    else
    {
        LE_ERROR("Null bufsize");
        mediaEvent = LE_AUDIO_MEDIA_ERROR;
    }

    if (mediaEvent != LE_AUDIO_MEDIA_MAX)
    {
        if (mediaEvent == LE_AUDIO_MEDIA_ERROR)
        {
            LE_DEBUG("PlaybackThreadTimer end");
            // Stop the timer
            le_timer_Stop(timerRef);
            lseek(threadContextPtr->fd, 0, SEEK_SET);
        }

        threadContextPtr->mediaEvent = mediaEvent;

        le_event_QueueFunctionToThread( threadContextPtr->mainThreadRef,
                                        PlayCaptTreatEvent,
                                        streamPtr,
                                        NULL );
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Playback thread
 *
 */
//--------------------------------------------------------------------------------------------------
static void* PlaybackThread
(
    void* contextPtr
)
{
    le_audio_Stream_t*          streamPtr = contextPtr;
    le_audio_PcmThreadContext_t* threadContextPtr = streamPtr->pcmThreadContextPtr;
    le_clk_Time_t interval = {0};

    threadContextPtr->operationResult = LE_OK;

    LE_DEBUG("Open playback");

    pcm_Handle_t pcmHandle = NULL;
    LE_DEBUG("streamPtr->deviceIdentifier %d",streamPtr->deviceIdentifier);

    char deviceString[10]="\0";
    snprintf(deviceString,10,"hw:0,%d", streamPtr->hwDeviceId);
    LE_DEBUG("Hardware interface: %s", deviceString);

    if ((pa_pcm_InitPlayback(&pcmHandle, deviceString, &(threadContextPtr->pcmConfig)) != LE_OK)
                            || (pcmHandle == NULL))
    {
        LE_ERROR("PCM cannot be open");
        threadContextPtr->operationResult = LE_FAULT;
    }
    else
    {
        threadContextPtr->pcmHandle = pcmHandle;

        // Set the timer
        threadContextPtr->timerRef = le_timer_Create ("PlaybackThreadTimer");
        le_timer_SetHandler(threadContextPtr->timerRef, PlaybackThreadTimer);
        le_timer_SetContextPtr(threadContextPtr->timerRef, contextPtr);
        threadContextPtr->timerUsec = (pa_pcm_GetPeriodSize(threadContextPtr->pcmHandle) * 1000000)/
                                    (threadContextPtr->pcmConfig.byteRate);
        interval.usec = threadContextPtr->timerUsec;
        LE_INFO("Play timer = %ld usec", interval.usec);

        le_timer_SetInterval(threadContextPtr->timerRef,interval);
        le_timer_SetRepeat(threadContextPtr->timerRef,0);
        le_timer_Start(threadContextPtr->timerRef);
    }

    le_sem_Post(threadContextPtr->threadSemaphore);

    le_event_RunLoop();

    return NULL;
}


//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to play a DTMF on a specific audio stream.
 *
 * @return LE_FORMAT_ERROR  The DTMF characters are invalid.
 * @return LE_BUSY          A DTMF playback is already in progress on the playback stream.
 * @return LE_FAULT         The function failed to play the DTMFs.
 * @return LE_OK            The funtion succeeded.
 *
 * @note The process exits, if an invalid audio stream reference is given.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_media_PlayDtmf
(
    le_audio_Stream_t*   streamPtr, ///< [IN] Stream object
    const char*          dtmfPtr,   ///< [IN] The DTMFs to play.
    uint32_t             duration,  ///< [IN] The DTMF duration in milliseconds.
    uint32_t             pause      ///< [IN] The pause duration between tones in milliseconds.
)
{
    le_result_t       res;

    if (streamPtr->audioInterface != LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY)
    {
        LE_ERROR("Invalid interface");
        return LE_FAULT;
    }

    if (streamPtr->mediaThreadContextPtr)
    {
        LE_ERROR("Media thread is already started");
        return LE_BUSY;
    }

    if (streamPtr->pcmThreadContextPtr)
    {
        LE_ERROR("Playback thread is already started");
        return LE_BUSY;
    }

    le_audio_MediaThreadContext_t* mediaCtxPtr = le_mem_ForceAlloc(MediaThreadContextPool);
    memset(mediaCtxPtr, 0, sizeof(le_audio_MediaThreadContext_t));

    DtmfParams_t*  dtmfParamsPtr = le_mem_ForceAlloc(DtmfParamsPool);
    int            pipefd[2];

    memset(dtmfParamsPtr, 0, sizeof(DtmfParams_t));

    streamPtr->samplePcmConfig.sampleRate = 16000;
    streamPtr->samplePcmConfig.bitsPerSample = 16;
    streamPtr->samplePcmConfig.channelsCount = 1;
    streamPtr->mediaThreadContextPtr = mediaCtxPtr;
    streamPtr->playFile = true;

    dtmfParamsPtr->duration = duration;
    dtmfParamsPtr->pause = pause;
    dtmfParamsPtr->sampleRate = 16000;

    strncpy(dtmfParamsPtr->dtmf, dtmfPtr, LE_AUDIO_DTMF_MAX_LEN);
    dtmfParamsPtr->dtmf[LE_AUDIO_DTMF_MAX_LEN] = '\0';

    mediaCtxPtr->initFunc = InitPlayDtmf;
    mediaCtxPtr->readFunc = PlayTone;
    mediaCtxPtr->writeFunc = WriteFd;
    mediaCtxPtr->closeFunc = ReleaseCodecParams;
    mediaCtxPtr->codecParams = (le_audio_Codec_t) dtmfParamsPtr;

    if (pipe(pipefd) == -1)
    {
        LE_ERROR("Failed to create the pipe");
        le_mem_Release(dtmfParamsPtr);
        le_mem_Release(mediaCtxPtr);
        streamPtr->mediaThreadContextPtr = NULL;
        return LE_FAULT;
    }

    mediaCtxPtr->fd_arg = streamPtr->fd;
    mediaCtxPtr->fd_pipe_input = pipefd[1];
    mediaCtxPtr->fd_pipe_output = pipefd[0];
    streamPtr->fd = pipefd[0];

    if ( (res=InitMediaThread( streamPtr,
                            LE_AUDIO_FILE_MAX,
                            -1,
                            pipefd[1] )) == LE_OK)
    {
        res=le_media_PlaySamples(streamPtr, &streamPtr->samplePcmConfig);
    }
    else
    {
        le_mem_Release(dtmfParamsPtr);
        le_mem_Release(mediaCtxPtr);
        streamPtr->mediaThreadContextPtr = NULL;
        LE_ERROR("Cannot spawn DTMF thread!");
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start media service: check the header, start decoder if needed.
 *
 *
 * @return LE_BUSY          Media is already started
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_media_Open
(
    le_audio_Stream_t*          streamPtr,          ///< [IN] Stream object
    le_audio_SamplePcmConfig_t* samplePcmConfigPtr  ///< [IN] Sample configuration
)
{
    le_result_t res = LE_FAULT;

    switch (streamPtr->audioInterface)
    {
        case LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY:
        {
            if (streamPtr->pcmThreadRef)
            {
                LE_ERROR("Play aleady in progress");
                return LE_BUSY;
            }

            le_audio_MediaThreadContext_t* mediaCtxPtr = le_mem_ForceAlloc(MediaThreadContextPool);
            le_audio_FileFormat_t format = LE_AUDIO_FILE_MAX;
            memset(mediaCtxPtr, 0, sizeof(le_audio_MediaThreadContext_t));

            // Check wav format
            res = PlayWavFile(streamPtr, samplePcmConfigPtr, mediaCtxPtr, &format);

            if (res != LE_OK)
            {
                // Check amr format
                res = PlayAmrFile(streamPtr, samplePcmConfigPtr, mediaCtxPtr, &format);
            }

            if (res == LE_OK)
            {
                int pipefd[2];
                if (pipe(pipefd) == -1)
                {
                    LE_ERROR("Failed to create the pipe");
                    le_mem_Release(mediaCtxPtr);
                    return LE_FAULT;
                }

                mediaCtxPtr->fd_arg = streamPtr->fd;
                mediaCtxPtr->fd_pipe_input = pipefd[1];
                mediaCtxPtr->fd_pipe_output = pipefd[0];
                streamPtr->mediaThreadContextPtr = mediaCtxPtr;
                streamPtr->fd =mediaCtxPtr->fd_pipe_output;

                LE_DEBUG("Pipe created, fd_pipe_input.%d fd_pipe_output.%d fd_arg.%d",
                            mediaCtxPtr->fd_pipe_input,
                            mediaCtxPtr->fd_pipe_output,
                            mediaCtxPtr->fd_arg);

                res = InitMediaThread( streamPtr,
                                       format,
                                       mediaCtxPtr->fd_arg,
                                       pipefd[1] );
            }

            if (res != LE_OK)
            {
                le_mem_Release(mediaCtxPtr);
            }
        }
        break;
        case LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE:
        {
            le_audio_MediaThreadContext_t* mediaCtxPtr = le_mem_ForceAlloc(MediaThreadContextPool);
            memset(mediaCtxPtr, 0, sizeof(le_audio_MediaThreadContext_t));
            le_audio_FileFormat_t format = LE_AUDIO_FILE_MAX;

            if (streamPtr->pcmThreadRef)
            {
                LE_ERROR("Recording aleady in progress");
                return LE_BUSY;
            }

            switch (streamPtr->encodingFormat)
            {
                case LE_AUDIO_WAVE:
                {
                    res = RecordWavFile(streamPtr, samplePcmConfigPtr, mediaCtxPtr, &format);
                }
                break;
                case LE_AUDIO_AMR:
                    res = RecordAmrFile(streamPtr, samplePcmConfigPtr, mediaCtxPtr, &format);
                break;
                default:
                    res = LE_FAULT;
                break;
            }

            if ( res == LE_OK )
            {
                int pipefd[2];
                if (pipe(pipefd) == -1)
                {
                    LE_ERROR("Failed to create the pipe");
                    le_mem_Release(mediaCtxPtr);
                    return LE_FAULT;
                }

                mediaCtxPtr->fd_arg = streamPtr->fd;
                mediaCtxPtr->fd_pipe_input = pipefd[1];
                mediaCtxPtr->fd_pipe_output = pipefd[0];
                streamPtr->fd = mediaCtxPtr->fd_pipe_input;
                streamPtr->mediaThreadContextPtr = mediaCtxPtr;
                streamPtr->fd = mediaCtxPtr->fd_pipe_input;

                LE_DEBUG("Pipe created, fd_pipe_input.%d fd_pipe_output.%d fd_arg.%d",
                            mediaCtxPtr->fd_pipe_input,
                            mediaCtxPtr->fd_pipe_output,
                            mediaCtxPtr->fd_arg);

                res = InitMediaThread(  streamPtr,
                                        format,
                                        pipefd[0],
                                        mediaCtxPtr->fd_arg );
            }

            if (res != LE_OK)
            {
                le_mem_Release(mediaCtxPtr);
            }
        }
        break;
        default:
        break;
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to play audio samples.
 *
 * @return LE_OK            The thread is started
 * @return LE_BAD_PARAMETER The interface is not valid
 * @return LE_BUSY          The thread is already started
 * @return LE_FAULT         The function is failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_media_PlaySamples
(
    le_audio_Stream_t*          streamPtr,          ///< [IN] Stream object
    le_audio_SamplePcmConfig_t* samplePcmConfigPtr  ///< [IN] Sample configuration
)
{
    le_audio_PcmThreadContext_t*  threadContextPtr;

    LE_DEBUG("Create Playback thread for interface %d fd %d", streamPtr->audioInterface,
                                                              streamPtr->fd);

    if (streamPtr->pcmThreadContextPtr)
    {
        LE_ERROR("Playback thread is already started");
        return LE_BUSY;
    }

    if (streamPtr->audioInterface != LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY)
    {
        LE_ERROR("Invalid interface");
        return LE_BAD_PARAMETER;
    }

    threadContextPtr = le_mem_ForceAlloc(PcmThreadContextPool);
    memset(threadContextPtr, 0, sizeof(le_audio_PcmThreadContext_t));

    threadContextPtr->fd = streamPtr->fd;

    samplePcmConfigPtr->byteRate = (  samplePcmConfigPtr->sampleRate *
                                      samplePcmConfigPtr->channelsCount *
                                      samplePcmConfigPtr->bitsPerSample  ) / 8;

    memcpy(&(threadContextPtr->pcmConfig), samplePcmConfigPtr, sizeof(le_audio_SamplePcmConfig_t));

    threadContextPtr->threadSemaphore = le_sem_Create("PlaybackSem",0);

    threadContextPtr->mainThreadRef = le_thread_GetCurrent();
    threadContextPtr->interface = streamPtr->audioInterface;
    threadContextPtr->operationResult = LE_FAULT;
    threadContextPtr->isNoMoreSamplesEventSent = false;
    threadContextPtr->mediaEvent = LE_AUDIO_MEDIA_MAX;

    streamPtr->pcmThreadContextPtr = threadContextPtr;

    LE_DEBUG("nbChannel.%d, rate.%d, bitsPerSample.%d, byteRate.%d",
             threadContextPtr->pcmConfig.channelsCount, threadContextPtr->pcmConfig.sampleRate,
             threadContextPtr->pcmConfig.bitsPerSample, threadContextPtr->pcmConfig.byteRate);

    char threadName[30]="\0";

    snprintf(threadName, 30, "Playback-%p", streamPtr->streamRef);

    streamPtr->pcmThreadRef = le_thread_Create(threadName,
                                                PlaybackThread,
                                                streamPtr);

    le_thread_SetJoinable(streamPtr->pcmThreadRef);

    le_thread_AddChildDestructor(streamPtr->pcmThreadRef,
                                 DestroyPlayCaptThread,
                                 streamPtr);

    // set the task in real time prioprity
    // [TODO] This resets the audio dameon. To be reactivated when it works.
    // le_thread_SetPriority(streamPtr->pcmThreadRef, LE_THREAD_PRIORITY_RT_2);

    le_thread_SetJoinable(streamPtr->pcmThreadRef);
    le_thread_Start(streamPtr->pcmThreadRef);

    le_sem_Wait(threadContextPtr->threadSemaphore);

    if (threadContextPtr->operationResult != LE_OK)
    {
        le_media_Stop(streamPtr);
    }

    return threadContextPtr->operationResult;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to pause the playback/capture thread.
 *
 * @return LE_OK            The function is succeeded
 * @return LE_FAULT         The function is failed
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_media_Pause
(
    le_audio_Stream_t*          streamPtr          ///< [IN] Stream object
)
{
    if ((streamPtr == NULL) || (streamPtr->pcmThreadContextPtr == NULL))
    {
        LE_ERROR("Bad stream objet or stream can't be paused");
        return LE_FAULT;
    }

    le_audio_PcmThreadContext_t* pcmThreadContextPtr = streamPtr->pcmThreadContextPtr;

    switch (streamPtr->audioInterface)
    {
        case LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE:
        {
            if(!pcmThreadContextPtr->pause)
            {
                pcmThreadContextPtr->pause = true;
                return LE_OK;
            }
            else
            {
                return LE_FAULT;
            }
        }
        break;

        case LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY:
        {
            if (streamPtr->pcmThreadRef)
            {
                pcmThreadContextPtr->operationResult = LE_FAULT;

                le_event_QueueFunctionToThread(streamPtr->pcmThreadRef,
                                                PlayThreadControl,
                                                (void*) pcmThreadContextPtr,
                                                (void*) PAUSE);

                le_sem_Wait(pcmThreadContextPtr->threadSemaphore);

                return pcmThreadContextPtr->operationResult;
            }
        }
        break;
        default:
        break;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to resume the playback/capture thread.
 *
 * @return LE_OK            The function is succeeded
 * @return LE_FAULT         The function is failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_media_Resume
(
    le_audio_Stream_t*          streamPtr          ///< [IN] Stream object
)
{
    if ((streamPtr == NULL) || (streamPtr->pcmThreadContextPtr == NULL))
    {
        LE_ERROR("Bad stream objet or stream can't be resumed");
        return LE_FAULT;
    }

    le_audio_PcmThreadContext_t* pcmThreadContextPtr = streamPtr->pcmThreadContextPtr;

    switch (streamPtr->audioInterface)
    {
        case LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE:
        {
            if(pcmThreadContextPtr->pause)
            {
                pcmThreadContextPtr->pause = false;
                return LE_OK;
            }
            else
            {
                return LE_FAULT;
            }
        }
        break;
        case LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY:
        {
            if (streamPtr->pcmThreadRef)
            {
                pcmThreadContextPtr->operationResult = LE_FAULT;

                le_event_QueueFunctionToThread( streamPtr->pcmThreadRef,
                                                PlayThreadControl,
                                                (void*) pcmThreadContextPtr,
                                                (void*) RESUME);

                le_sem_Wait(pcmThreadContextPtr->threadSemaphore);

                return pcmThreadContextPtr->operationResult;
            }
        }
        default:
        break;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to flush the remaining audio samples.
 *
 * @return LE_OK            The function is succeeded
 * @return LE_FAULT         The function is failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_media_Flush
(
    le_audio_Stream_t*          streamPtr          ///< [IN] Stream object
)
{
    if ((streamPtr == NULL) || (streamPtr->pcmThreadContextPtr == NULL))
    {
        LE_ERROR("Bad stream objet or stream can't be flushed");
        return LE_FAULT;
    }

    le_audio_PcmThreadContext_t* pcmThreadContextPtr = streamPtr->pcmThreadContextPtr;

    switch (streamPtr->audioInterface)
    {
        case LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE:
        {
            LE_ERROR("Cannot flush a capture stream!");
            return LE_FAULT;
        }
        break;
        case LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY:
        {
            if ( streamPtr->pcmThreadRef )
            {
                pcmThreadContextPtr->operationResult = LE_FAULT;

                le_event_QueueFunctionToThread( streamPtr->pcmThreadRef,
                                                PlayThreadControl,
                                                (void*) pcmThreadContextPtr,
                                                (void*) FLUSH);

                le_sem_Wait(pcmThreadContextPtr->threadSemaphore);

                return pcmThreadContextPtr->operationResult;
            }
        }
        default:
        break;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop an interface.
 *
 * @return LE_OK            The function is succeeded
 * @return LE_FAULT         The function is failed
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_media_Stop
(
    le_audio_Stream_t*          streamPtr          ///< [IN] Stream object
)
{
    if (streamPtr == NULL)
    {
        LE_ERROR("Bad stream objet or stream can't be stopped");
        return LE_FAULT;
    }

    switch (streamPtr->audioInterface)
    {
        case LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY:
        case LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE:
        {
            // first close pcmThread, then Media thread because of file descriptor closure order

            LE_DEBUG("le_media_Stop requested");

            if (streamPtr->pcmThreadRef)
            {
                le_thread_Cancel(streamPtr->pcmThreadRef);
                le_thread_Join(streamPtr->pcmThreadRef,NULL);
                streamPtr->pcmThreadRef = NULL;
            }

            if (streamPtr->mediaThreadRef)
            {
                le_thread_Cancel(streamPtr->mediaThreadRef);
                le_thread_Join(streamPtr->mediaThreadRef,NULL);
                streamPtr->mediaThreadRef = NULL;
            }

            return LE_OK;
        }
        break;

        default:
            LE_DEBUG("stream %d can't be stopped", streamPtr->audioInterface);
        break;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to capture an audio stream.
 *
 * @return LE_OK            The thread is started
 * @return LE_BAD_PARAMETER The interface is not valid
 * @return LE_BUSY          The thread is already started
 * @return LE_FAULT         The function is failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_media_Capture
(
    le_audio_Stream_t*          streamPtr,          ///< [IN] Stream object
    le_audio_SamplePcmConfig_t* samplePcmConfigPtr  ///< [IN] Sample configuration
)
{
    LE_DEBUG("Create capture thread for interface %d", streamPtr->audioInterface);

    if (streamPtr->pcmThreadContextPtr)
    {
        LE_ERROR("capture thread is already started");
        return LE_BUSY;
    }

    if (streamPtr->audioInterface != LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE)
    {
        LE_ERROR("Invalid interface");
        return LE_BAD_PARAMETER;
    }

    le_audio_PcmThreadContext_t* threadContextPtr = le_mem_ForceAlloc(PcmThreadContextPool);

    memset(threadContextPtr, 0, sizeof(le_audio_PcmThreadContext_t));

    threadContextPtr->fd = streamPtr->fd;

    samplePcmConfigPtr->byteRate = (samplePcmConfigPtr->sampleRate *
                                    samplePcmConfigPtr->channelsCount *
                                    samplePcmConfigPtr->bitsPerSample) / 8;

    memcpy(&(threadContextPtr->pcmConfig), samplePcmConfigPtr, sizeof(le_audio_SamplePcmConfig_t));

    threadContextPtr->threadSemaphore = le_sem_Create("CaptureSem",0);
    threadContextPtr->timerRef = NULL;
    threadContextPtr->mainThreadRef = le_thread_GetCurrent();
    threadContextPtr->interface = streamPtr->audioInterface;
    threadContextPtr->pause = false;
    threadContextPtr->operationResult = LE_FAULT;

    streamPtr->pcmThreadContextPtr = threadContextPtr;

    char threadName[30]="\0";

    snprintf(threadName, 30, "AudioCapture-%p", streamPtr->streamRef);

    streamPtr->pcmThreadRef = le_thread_Create(threadName,
                                            CaptureThread,
                                            streamPtr);

    le_thread_AddChildDestructor(streamPtr->pcmThreadRef,
                                 DestroyPlayCaptThread,
                                 streamPtr);

    // set the task in real time prioprity
    // [TODO] This resets the audio dameon. To be reactivated when it works.
    // le_thread_SetPriority(streamPtr->pcmThreadRef, LE_THREAD_PRIORITY_RT_2);

    le_thread_SetJoinable(streamPtr->pcmThreadRef);
    le_thread_Start(streamPtr->pcmThreadRef);

    le_sem_Wait(threadContextPtr->threadSemaphore);

    return threadContextPtr->operationResult;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function check if a stream is busy.
 *
 * @return false            The stream is unused
 * @return true             The stream is busy
 */
//--------------------------------------------------------------------------------------------------
bool le_media_IsStreamBusy
(
    le_audio_Stream_t*          streamPtr         ///< [IN] Stream object
)
{
    if ((streamPtr->audioInterface == LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE) ||
        (streamPtr->audioInterface == LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY))
    {
        if (streamPtr->pcmThreadContextPtr)
        {
            LE_DEBUG("Stream in use pcmThreadContextPtr %p", streamPtr->pcmThreadContextPtr);
            return true;
        }
        else
        {
            return false;
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the media service.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_media_Init
(
    void
)
{
    // Allocate the Media thread context pool.
    MediaThreadContextPool = le_mem_CreatePool("MediaThreadContextPool",
                                                sizeof(le_audio_MediaThreadContext_t));

    // Allocate the DTMF parameters pool.
    DtmfParamsPool = le_mem_CreatePool("DtmfParamsPool", sizeof(DtmfParams_t));

    // Allocate the WAV parameters pool.
    WavParamsPool = le_mem_CreatePool("WavParamsPool", sizeof(WavParams_t));

    // Allocate the audio threads params pool.
    PcmThreadContextPool = le_mem_CreatePool("PcmThreadContextPool",
                                                               sizeof(le_audio_PcmThreadContext_t));
}
