//--------------------------------------------------------------------------------------------------
/**
 * @file le_media.c
 *
 * This file contains the source code of the high level Audio API for playback / capture
 *
 * Copyright (C) Sierra Wireless Inc.
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
 * Legato object string length.
 */
//--------------------------------------------------------------------------------------------------
#define STRING_LEN    30

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
/**
 * For PlaySamples wait indefinitely until more samples are available or playback is stopped
*/
//--------------------------------------------------------------------------------------------------
#define NO_MORE_SAMPLES_INFINITE_TIMEOUT -1

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
    uint32_t sampleRate;         ///< Sample frequency in Hertz
    uint32_t duration;           ///< The DTMF duration in milliseconds.
    uint32_t pause;              ///< The pause duration between tones in milliseconds.
    bool     playPause;          ///< Play the pause
    char     dtmf[LE_AUDIO_DTMF_MAX_BYTES];    ///< The DTMFs to play.
    uint32_t currentDtmf;        ///< Index of the play dtmf
    uint32_t currentSampleCount; ///< Current sample count for the current DTMF
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
 * Wake Lock for audio streams
 *
 */
//--------------------------------------------------------------------------------------------------
static le_pm_WakeupSourceRef_t MediaWakeLock;

//--------------------------------------------------------------------------------------------------
/**
 * Reads a specified number of bytes from the provided file descriptor into the provided buffer.
 * This function will block until the specified number of bytes is read or an EOF is reached.
 *
 * @return
 *      Number of bytes read.
 *      LE_FAULT if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static ssize_t ReadFd
(
    int fd,                               ///<[IN] File to read.
    void* bufPtr,                         ///<[OUT] Buffer to store the read bytes in.
    size_t bufSize                        ///<[IN] Size of the buffer.
)
{
    LE_FATAL_IF(bufPtr == NULL, "Supplied NULL string pointer");
    LE_FATAL_IF(fd < 0, "Supplied invalid file descriptor");

    int bytesRd = 0, tempBufSize = 0, rdReq = bufSize;
    char *tempStr;

    // Requested zero bytes to read, return immediately
    if (bufSize == 0)
    {
        return tempBufSize;
    }

    do
    {
        tempStr = (char *)(bufPtr);
        tempStr = tempStr + tempBufSize;

        bytesRd = read(fd, tempStr, rdReq);

        if ((bytesRd < 0) && (errno != EINTR) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
        {
            LE_ERROR("Error while reading file, errno: %d (%m)", errno);
            return bytesRd;
        }
        else
        {
            //Reached End of file, so return what it reads upto EOF
            if (bytesRd == 0)
            {
                return tempBufSize;
            }

            tempBufSize += bytesRd;

            if (tempBufSize < bufSize)
            {
                rdReq = bufSize - tempBufSize;
            }
        }
    }
    while (tempBufSize < bufSize);

    return tempBufSize;
}


//--------------------------------------------------------------------------------------------------
/**
 * Writes a specified number of bytes from the provided buffer to the provided file descriptor.
 * This function will block until the specified number of bytes is written.
 *
 * @return
 *      Number of bytes written.
 *      LE_FAULT if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static ssize_t WriteFd
(
    int fd,                               ///<[IN] File to write.
    void* bufPtr,                         ///<[IN] Buffer which will be written to file.
    size_t bufSize                        ///<[IN] Size of the buffer.
)
{
    LE_FATAL_IF(bufPtr == NULL, "Supplied NULL String Pointer");
    LE_FATAL_IF(fd < 0, "Supplied invalid file descriptor");

    int bytesWr = 0, tempBufSize = 0, wrReq = bufSize;
    char *tempStr;

    // Requested zero bytes to write, returns immediately
    if (bufSize == 0)
    {
        return tempBufSize;
    }

    do
    {
        tempStr = (char *)(bufPtr);
        tempStr = tempStr + tempBufSize;

        bytesWr= write(fd, tempStr, wrReq);

        if ((bytesWr < 0) && (errno != EINTR))
        {
            LE_ERROR("Error while writing file, errno: %d (%m)", errno);
            return bytesWr;
        }
        else
        {
            tempBufSize += bytesWr;

            if(tempBufSize < bufSize)
            {
                wrReq = bufSize - tempBufSize;
            }
        }
    }
    while (tempBufSize < bufSize);

    return tempBufSize;
}

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
 *  Play Tone function. This function split into samples of 1s. To play a DTMF or a PAUSE for a
 *  duration greater than 1s, several calls are mandatory to get the whole duration sample.
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
    // Max samples on the whole duration
    uint32_t samplesCount;
    // Sample count until the next second
    uint32_t sampleOneSecond = dtmfParamsPtr->sampleRate + dtmfParamsPtr->currentSampleCount;
    uint32_t freq1;
    uint32_t freq2;
    int32_t  amp1;
    int32_t  amp2;
    int16_t* dataPtr = (int16_t*) bufferOutPtr;
    // Length of the current sample: max 1 second, i.e, sampleRate
    uint32_t sampleLength;

    if ( (dtmfParamsPtr->sampleRate * sizeof(int16_t)) > mediaCtxPtr->bufferSize )
    {
        LE_ERROR("%s buffer too small, sampleRate %d, bufferSize %d",
                 dtmfParamsPtr->playPause ? "Pause" : "DTMF",
                 dtmfParamsPtr->sampleRate, mediaCtxPtr->bufferSize);
        return LE_FAULT;
    }

    if (dtmfParamsPtr->playPause)
    {
        samplesCount = dtmfParamsPtr->sampleRate*dtmfParamsPtr->pause / 1000;
        // If the remaining duration is greater than sampleRate, produce only 1s sample, else
        // produce the remaining duration
        sampleLength =
            ((samplesCount - dtmfParamsPtr->currentSampleCount) > dtmfParamsPtr->sampleRate)
                ? dtmfParamsPtr->sampleRate
                : (samplesCount - dtmfParamsPtr->currentSampleCount);

        LE_DEBUG("Play PAUSE sampleOneSecond %u, currentSampleCount %u, sampleLength %u",
                 sampleOneSecond, dtmfParamsPtr->currentSampleCount, sampleLength);

        memset( dataPtr, 0, sampleLength * sizeof(int16_t) );

        dtmfParamsPtr->currentSampleCount += sampleLength;
        if (dtmfParamsPtr->currentSampleCount >= samplesCount)
        {
            dtmfParamsPtr->currentSampleCount = 0;
        }
    }
    else
    {
        if (dtmfParamsPtr->currentDtmf == strlen(dtmfParamsPtr->dtmf))
        {
            LE_DEBUG("All DTMF played");
            *bufferLenPtr = 0;
            return LE_UNDERFLOW;
        }

        samplesCount = dtmfParamsPtr->sampleRate*dtmfParamsPtr->duration / 1000;
        // If the remaining duration is greater than sampleRate, produce only 1s sample, else
        // produce the remaining duration
        sampleLength =
            ((samplesCount - dtmfParamsPtr->currentSampleCount) > dtmfParamsPtr->sampleRate)
                ? dtmfParamsPtr->sampleRate
                : (samplesCount - dtmfParamsPtr->currentSampleCount);
        LE_DEBUG("Play DtMF '%c' sampleOneSecond %u, currentSampleCount %u, sampleLength %u",
                 dtmfParamsPtr->dtmf[dtmfParamsPtr->currentDtmf],
                 sampleOneSecond, dtmfParamsPtr->currentSampleCount, sampleLength);

        freq1 = Digit2LowFreq(dtmfParamsPtr->dtmf[dtmfParamsPtr->currentDtmf]);
        freq2 = Digit2HighFreq(dtmfParamsPtr->dtmf[dtmfParamsPtr->currentDtmf]);
        amp1 = DTMF_AMPLITUDE;
        amp2 = DTMF_AMPLITUDE;

        d1 = 1.0f * freq1 / dtmfParamsPtr->sampleRate;
        d2 = 1.0f * freq2 / dtmfParamsPtr->sampleRate;

        for (i = dtmfParamsPtr->currentSampleCount;
             // Play max sampleRate (1s) of DTMF and continue at next call
             (i < sampleOneSecond) && (i < samplesCount);
             i++)
        {
            int16_t s1, s2;

            s1 = (int16_t)(SAMPLE_SCALE * amp1 / 100.0f * sin(2 * PI * d1 * i));
            s2 = (int16_t)(SAMPLE_SCALE * amp2 / 100.0f * sin(2 * PI * d2 * i));
            *(dataPtr++) = SaturateAdd16(s1, s2);
        }

        // Save the current sample count. If the whole DTMF is played, reset to 0
        dtmfParamsPtr->currentSampleCount = (i == samplesCount ? 0 : i);
        if (0 == dtmfParamsPtr->currentSampleCount)
        {
            // Update the index of DTMF if the current sample count is reset to 0
            dtmfParamsPtr->currentDtmf++;
        }
    }

    *bufferLenPtr = sampleLength * sizeof(int16_t);

    if (0 == dtmfParamsPtr->currentSampleCount)
    {
        dtmfParamsPtr->playPause = (dtmfParamsPtr->playPause
                                       ? false
                                       : (dtmfParamsPtr->pause ? true : false));
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
    if (WriteFd(fd, &hdr, sizeof(hdr)) != sizeof(hdr))
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
static le_result_t MediaReadFd
(
    le_audio_MediaThreadContext_t* mediaCtxPtr,  ///< [IN] Media thread context
    uint8_t*                       bufferOutPtr, ///< [OUT] Decoding samples buffer output
    uint32_t*                      readLenPtr    ///< [OUT] Length of the read data
)
{
    ssize_t size = ReadFd(mediaCtxPtr->fd_in, bufferOutPtr, mediaCtxPtr->bufferSize);

    if (size < 0)
    {
        LE_ERROR("Read error fd=%d", mediaCtxPtr->fd_in);
        return LE_FAULT;
    }

    (*readLenPtr) = size;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write on a file descriptor.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t MediaWriteFd
(
    le_audio_MediaThreadContext_t* mediaCtxPtr,     ///< [IN] Media thread context
    uint8_t*                       bufferInPtr,     ///< [IN] Decoding samples buffer input
    uint32_t                       bufferLen        ///< [IN] Buffer length
)
{
    if (WriteFd(mediaCtxPtr->fd_out, bufferInPtr, bufferLen) < 0)
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
        int32_t writeLen = WriteFd( mediaCtxPtr->fd_out, outputBuf, outputBufLen );

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

    int32_t len = WriteFd(mediaCtxPtr->fd_out, bufferInPtr, bufferLen);

    if (len != bufferLen)
    {
        LE_ERROR("write error: %d written, expected %d, errno %d", len, bufferLen, errno);
        pthread_setcancelstate(oldstate, &dummy);
        return LE_FAULT;
    }

    wavParamPtr->recordingSize += len;

    lseek(mediaCtxPtr->fd_out, ((uint8_t*)&hdr.dataSize - (uint8_t*)&hdr), SEEK_SET);

    len = WriteFd(mediaCtxPtr->fd_out,
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

    len = WriteFd(mediaCtxPtr->fd_out, &riffSize, sizeof(riffSize));

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

    // Buffer size to sample of 1s x 16-bits
    mediaCtxPtr->bufferSize = dtmfParamsPtr->sampleRate * sizeof(int16_t);

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

        if (mediaCtxPtr->threadSemaphore)
        {
            le_sem_Delete(mediaCtxPtr->threadSemaphore);
        }

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
    bool semPost = false;

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
        if ( ( mediaCtxPtr->readFunc( mediaCtxPtr,
                                      outBuffer,
                                      &readLen ) == LE_OK ) && readLen )
        {
            if ( mediaCtxPtr->writeFunc( mediaCtxPtr,
                                         outBuffer,
                                         readLen ) != LE_OK )
            {
                break;
            }
            else
            {
                if (mediaCtxPtr->threadSemaphore && !semPost)
                {
                    le_sem_Post(mediaCtxPtr->threadSemaphore);
                    semPost = true;
                }
            }
        }
        else
        {
            break;
        }
    }

    LE_DEBUG("MediaThread end");

    // Run the event loop to wait the end of the thread
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
    le_audio_MediaThreadContext_t* mediaCtxPtr;

    if (streamPtr == NULL)
    {
        return LE_FAULT;
    }

    mediaCtxPtr = streamPtr->mediaThreadContextPtr;

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

    char name[STRING_LEN];

    snprintf(name, sizeof(name), "MediaThread-%p", streamPtr->streamRef);

    streamPtr->mediaThreadRef = le_thread_Create(name,
                                                 MediaThread,
                                                 streamPtr->mediaThreadContextPtr);

    snprintf(name, sizeof(name), "MediaSem-%p", streamPtr->streamRef);

    // semaphore only needs for playback. For recording, we are waiting for data on the pipe
    if (streamPtr->audioInterface == LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY)
    {
        mediaCtxPtr->threadSemaphore = le_sem_Create(name,0);
    }
    else
    {
        mediaCtxPtr->threadSemaphore = NULL;
    }

    // Increase thread priority for file playback to avoid underflow
    if ( LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY == streamPtr->audioInterface )
    {
        le_thread_SetPriority(streamPtr->mediaThreadRef, LE_THREAD_PRIORITY_RT_3);
    }

    le_thread_SetJoinable(streamPtr->mediaThreadRef);

    le_thread_AddChildDestructor(streamPtr->mediaThreadRef,
                                 DestroyMediaThread,
                                 streamPtr);

    le_thread_Start(streamPtr->mediaThreadRef);

    if (mediaCtxPtr->threadSemaphore)
    {
        le_clk_Time_t  timeToWait = {1,0};
        le_sem_WaitWithTimeOut(mediaCtxPtr->threadSemaphore, timeToWait);
    }

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

    if (ReadFd(streamPtr->fd, &hdr, sizeof(hdr)) != sizeof(hdr))
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
    mediaContextPtr->readFunc = MediaReadFd;
    mediaContextPtr->writeFunc = MediaWriteFd;
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
        mediaContextPtr->writeFunc = MediaWriteFd;
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
    mediaCtxPtr->readFunc = MediaReadFd;
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
    mediaCtxPtr->readFunc = MediaReadFd;
    mediaCtxPtr->writeFunc = WavWriteFd;
    mediaCtxPtr->closeFunc = ReleaseCodecParams;
    *formatPtr = LE_AUDIO_FILE_WAVE;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Control the playback thread (pause/resume)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t PlayCaptControl
(
    le_audio_PcmContext_t*  pcmContextPtr,
    ControlOperation_t      operation
)
{
    LE_DEBUG("operation: %d", operation);
    le_result_t res = LE_FAULT;

    switch (operation)
    {
        case PAUSE:
            // stop the timer to make a pause
            if (pcmContextPtr && !pcmContextPtr->pause)
            {
                pcmContextPtr->pause = true;
                res = LE_OK;
            }
            else
            {
                LE_ERROR("stream already in pause");
                res = LE_FAULT;
            }
        break;

        case RESUME:
            // start the timer to resume the playback
            if (pcmContextPtr && pcmContextPtr->pause)
            {
                pcmContextPtr->pause = false;
                res = LE_OK;
            }
            else
            {
                LE_ERROR("Resume the stream, but not paused");
                res = LE_FAULT;
            }
        break;

        case FLUSH:
            // flush the audio stream
            if (pcmContextPtr)
            {
                pcmContextPtr->pause = true;

                char data[4096];
                int mask;

                if ((mask = fcntl(pcmContextPtr->fd, F_GETFL, 0)) == -1)
                {
                    LE_ERROR("fcntl error, errno.%d (%s)", errno, strerror(errno));
                    res = LE_FAULT;
                    break;
                }
                if (fcntl(pcmContextPtr->fd, F_SETFL, mask | O_NONBLOCK) == -1)
                {
                    LE_ERROR("fcntl error, errno.%d (%s)", errno, strerror(errno));
                    res = LE_FAULT;
                    break;
                }

                ssize_t len = 1;

                while (len > 0)
                {
                    len = ReadFd(pcmContextPtr->fd, data, sizeof(data));
                }

                if (fcntl(pcmContextPtr->fd, F_SETFL, mask) == -1)
                {
                    LE_ERROR("fcntl error, errno.%d (%s)", errno, strerror(errno));
                    res = LE_FAULT;
                    break;
                }

                pcmContextPtr->pause = false;

                res = LE_OK;
                LE_INFO("Flush audio!");
            }
            else
            {
                LE_ERROR("pcmContextPtr not exist");
                res = LE_FAULT;
            }
        break;

        default:
            // This shouldn't occur
            LE_ERROR("Bad asked operation %d", operation);
        break;
    }

    LE_DEBUG("end operation: %d res: %d", operation, res);

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * treat events sent by playback thread
 *
 */
//--------------------------------------------------------------------------------------------------
static void PlayTreatEvent
(
    void* param1Ptr,
    void* param2Ptr
)
{
    le_audio_Stream_t* streamPtr = param1Ptr;
    le_audio_PcmContext_t* pcmContextPtr = NULL;
    bool mediaClose = false;

    if (!streamPtr)
    {
        LE_ERROR("streamPtr is null !!!");
        return;
    }

    pcmContextPtr = streamPtr->pcmContextPtr;
    if (!pcmContextPtr)
    {
        LE_ERROR("pcmContextPtr is null !!!");
        return;
    }

    if (LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY != streamPtr->audioInterface)
    {
        LE_ERROR("Bad function called");
        return;
    }

    LE_DEBUG("mediaEvent %d playFile %d", pcmContextPtr->mediaEvent, streamPtr->playFile);

    if (pcmContextPtr->mediaEvent != LE_AUDIO_MEDIA_ERROR)
    {
        if (streamPtr->playFile)
        {
            mediaClose = true;
        }
    }
    else
    {
        mediaClose = true;
    }

    le_audio_StreamEvent_t streamEvent;
    streamEvent.streamPtr = streamPtr;
    streamEvent.streamEvent = LE_AUDIO_BITMASK_MEDIA_EVENT;
    streamEvent.event.mediaEvent = pcmContextPtr->mediaEvent;

    le_event_Report(streamPtr->streamEventId,
                        &streamEvent,
                        sizeof(le_audio_StreamEvent_t));

    if (mediaClose)
    {
        le_media_Stop(streamPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get Playback frames
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetPlaybackFrames
(
    uint8_t* bufferPtr,
    uint32_t* bufsizePtr,
    void* contextPtr
)
{
    le_audio_Stream_t* streamPtr = contextPtr;
    le_audio_PcmContext_t* pcmContextPtr = streamPtr->pcmContextPtr;
    int32_t len = 0;
    int ret;
    uint32_t size = *bufsizePtr;
    uint32_t amount = 0;
    nfds_t nfds = 1;
    struct pollfd pfd;

    pfd.fd = pcmContextPtr->fd;
    pfd.events = POLLIN;

    while (size)
    {
        ret = poll(&pfd, nfds, pcmContextPtr->framesFuncTimeout);

        switch (ret)
        {
            case -1:
                if ((errno == EINTR) || (errno == EAGAIN))
                {
                    // read again
                    LE_WARN("Failed in poll: %m");
                    continue;
                }
                else
                {
                    LE_ERROR("Failed in poll: %m");
                    return LE_FAULT;
                }
                break;
            case 0:
                // timeout: no data read
                LE_DEBUG("No data read");
                if (!amount)
                {
                    // no more samples available at this point:
                    // send silence frames to avoid xrun
                    memset(bufferPtr, 0, size);
                    size = 0;
                }
                *bufsizePtr = amount;
                return LE_OK;
            default:
                // playback is paused: return without reading samples
                if (pcmContextPtr->pause)
                {
                    amount = *bufsizePtr;
                    memset(bufferPtr, 0, amount);
                    return LE_OK;
                }
                else if (pfd.revents & POLLIN)
                {
                    len = read(pcmContextPtr->fd, bufferPtr + amount, size);

                    if (len == 0)
                    {
                        LE_ERROR("Failed to read on fd %d, writing end of pipe was closed",
                                 pcmContextPtr->fd);
                        return LE_CLOSED;
                    }
                    else if (len > 0)
                    {
                        size -= len;
                        amount += len;
                        *bufsizePtr = amount;
                        // update timeout value for remaining data to be read
                        long msec = (pa_pcm_GetPeriodSize(pcmContextPtr->pcmHandle) *
                                     (1000000 / (pcmContextPtr->pcmConfig.byteRate))) / 1000;
                        pcmContextPtr->framesFuncTimeout = msec;
                        continue;
                    }
                    else if (len < 0)
                    {
                        LE_ERROR("Failed in read: %m");
                        return LE_FAULT;
                    }
                }
                else if ((pfd.revents & POLLHUP) || (pfd.revents & POLLRDHUP))
                {
                    LE_ERROR("Write-end of pipe was closed (%m)");
                    return LE_CLOSED;
                }
                else if (pfd.revents & POLLERR)
                {
                    LE_ERROR("Failed in poll: %m");
                    return LE_FAULT;
                }
                break;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set capture frames
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetCaptureFrames
(
    uint8_t* bufferPtr,
    uint32_t* bufsizePtr,
    void* contextPtr
)
{
    le_audio_Stream_t*     streamPtr = contextPtr;
    le_audio_PcmContext_t* pcmContextPtr = streamPtr->pcmContextPtr;

    if ( !pcmContextPtr->pause )
    {
        if (WriteFd(pcmContextPtr->fd, bufferPtr, *bufsizePtr) < 0)
        {
            LE_ERROR("Cannot write on pipe");
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Result of the playback or capture
 *
 */
//--------------------------------------------------------------------------------------------------
static void PlayCaptResult
(
    le_result_t res,
    void* contextPtr
)
{
    le_audio_Stream_t* streamPtr = contextPtr;
    le_audio_PcmContext_t* pcmContextPtr = NULL;

    if (!streamPtr)
    {
        LE_ERROR("streamPtr %p is null !!!", streamPtr);
        return;
    }

    pcmContextPtr = streamPtr->pcmContextPtr;
    if (!pcmContextPtr)
    {
        LE_ERROR("pcmContextPtr %p is null !!!", pcmContextPtr);
        return;
    }
    if (LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY == streamPtr->audioInterface)
    {
        LE_DEBUG("Playback result: res %d mainThreadRef %p", res, pcmContextPtr->mainThreadRef);

        if (res == LE_OK)
        {
            pcmContextPtr->mediaEvent = LE_AUDIO_MEDIA_NO_MORE_SAMPLES;
        }
        else if (res == LE_UNDERFLOW)
        {
            // No data were sent to the driver. Nothing to do
            return;
        }
        else
        {
            pcmContextPtr->mediaEvent = LE_AUDIO_MEDIA_ERROR;
        }

        le_event_QueueFunctionToThread( pcmContextPtr->mainThreadRef,
                                        PlayTreatEvent,
                                        streamPtr,
                                        NULL );
    }
    else
    {
        LE_DEBUG("capture result: res %d mainThreadRef %p", res, pcmContextPtr->mainThreadRef);
    }
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

    if (streamPtr->pcmContextPtr)
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
    dtmfParamsPtr->currentSampleCount = 0;

    strncpy(dtmfParamsPtr->dtmf, dtmfPtr, LE_AUDIO_DTMF_MAX_LEN);
    dtmfParamsPtr->dtmf[LE_AUDIO_DTMF_MAX_LEN] = '\0';

    mediaCtxPtr->initFunc = InitPlayDtmf;
    mediaCtxPtr->readFunc = PlayTone;
    mediaCtxPtr->writeFunc = MediaWriteFd;
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
            if (streamPtr->pcmContextPtr)
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

            if (streamPtr->pcmContextPtr)
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
    le_audio_PcmContext_t*  pcmContextPtr;

    LE_DEBUG("Create Playback thread for interface %d fd %d", streamPtr->audioInterface,
                                                              streamPtr->fd);

    if ((streamPtr == NULL) || (streamPtr->pcmContextPtr))
    {
        LE_ERROR("Playback thread is already started");
        return LE_BUSY;
    }

    if (streamPtr->audioInterface != LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY)
    {
        LE_ERROR("Invalid interface");
        return LE_BAD_PARAMETER;
    }

    pcmContextPtr = le_mem_ForceAlloc(PcmThreadContextPool);
    memset(pcmContextPtr, 0, sizeof(le_audio_PcmContext_t));

    pcmContextPtr->fd = streamPtr->fd;

    samplePcmConfigPtr->byteRate = (  samplePcmConfigPtr->sampleRate *
                                      samplePcmConfigPtr->channelsCount *
                                      samplePcmConfigPtr->bitsPerSample  ) / 8;

    memcpy(&(pcmContextPtr->pcmConfig), samplePcmConfigPtr, sizeof(le_audio_SamplePcmConfig_t));

    pcmContextPtr->mainThreadRef = le_thread_GetCurrent();
    pcmContextPtr->interface = streamPtr->audioInterface;
    pcmContextPtr->mediaEvent = LE_AUDIO_MEDIA_MAX;

    streamPtr->pcmContextPtr = pcmContextPtr;

    LE_DEBUG("nbChannel.%d, rate.%d, bitsPerSample.%d, byteRate.%d",
             pcmContextPtr->pcmConfig.channelsCount, pcmContextPtr->pcmConfig.sampleRate,
             pcmContextPtr->pcmConfig.bitsPerSample, pcmContextPtr->pcmConfig.byteRate);

    pcm_Handle_t pcmHandle = NULL;

    LE_DEBUG("streamPtr->deviceIdentifier %d",streamPtr->deviceIdentifier);

    char deviceString[STRING_LEN];
    snprintf(deviceString,sizeof(deviceString),"hw:0,%d", streamPtr->hwDeviceId);
    LE_DEBUG("Hardware interface: %s", deviceString);

    // Request a wakeup source for media streams
    le_pm_StayAwake(MediaWakeLock);

    if ((pa_pcm_InitPlayback(&pcmHandle, deviceString, &(pcmContextPtr->pcmConfig)) != LE_OK)
                            || (pcmHandle == NULL))
    {
        LE_ERROR("PCM cannot be open");
        le_media_Stop(streamPtr);
        return LE_FAULT;
    }

    pcmContextPtr->pcmHandle = pcmHandle;
    pcmContextPtr->framesFuncTimeout = 0;

    pa_pcm_SetCallbackHandlers( pcmHandle,
                                GetPlaybackFrames,
                                PlayCaptResult,
                                streamPtr );

    if (pa_pcm_Play(pcmHandle) != LE_OK)
    {
        LE_ERROR("Error in pa_pcm_Write");
        le_media_Stop(streamPtr);
        return LE_FAULT;
    }

    LE_DEBUG("Playback started");

    return LE_OK;
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
    if ((streamPtr == NULL) || (streamPtr->pcmContextPtr == NULL))
    {
        LE_ERROR("Bad stream objet or stream can't be paused");
        return LE_FAULT;
    }

    le_audio_PcmContext_t* pcmContextPtr = streamPtr->pcmContextPtr;

    switch (streamPtr->audioInterface)
    {
        case LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE:
        {
            if(pcmContextPtr && !pcmContextPtr->pause)
            {
                pcmContextPtr->pause = true;
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
            if (pcmContextPtr)
            {
                return PlayCaptControl(pcmContextPtr, PAUSE);
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
    if ((streamPtr == NULL) || (streamPtr->pcmContextPtr == NULL))
    {
        LE_ERROR("Bad stream objet or stream can't be resumed");
        return LE_FAULT;
    }

    le_audio_PcmContext_t* pcmContextPtr = streamPtr->pcmContextPtr;

    switch (streamPtr->audioInterface)
    {
        case LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE:
        {
            if(pcmContextPtr->pause)
            {
                pcmContextPtr->pause = false;
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
            if (pcmContextPtr)
            {
                return PlayCaptControl(pcmContextPtr, RESUME);
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
    if ((streamPtr == NULL) || (streamPtr->pcmContextPtr == NULL))
    {
        LE_ERROR("Bad stream objet or stream can't be flushed");
        return LE_FAULT;
    }

    le_audio_PcmContext_t* pcmContextPtr = streamPtr->pcmContextPtr;

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
            if ( pcmContextPtr )
            {
                return PlayCaptControl(pcmContextPtr, FLUSH);
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
        case LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE:
        case LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY:
        {
            if (streamPtr->pcmContextPtr)
            {
                LE_DEBUG("Close pa_pcm");
                pa_pcm_Close(streamPtr->pcmContextPtr->pcmHandle);
                le_mem_Release(streamPtr->pcmContextPtr);
                streamPtr->pcmContextPtr = NULL;
            }

            if (streamPtr->mediaThreadRef)
            {
                LE_DEBUG("Stop media thread");
                le_thread_Cancel(streamPtr->mediaThreadRef);
                le_thread_Join(streamPtr->mediaThreadRef,NULL);
                streamPtr->mediaThreadRef = NULL;
            }

            // Release the wakeup source for media streams
            le_pm_Relax(MediaWakeLock);

            LE_DEBUG("Interface %d Stopped", streamPtr->audioInterface);
        }
        break;

        default:
            LE_DEBUG("stream %d can't be stopped", streamPtr->audioInterface);
        break;
    }

    return LE_OK;
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

    if (streamPtr->pcmContextPtr)
    {
        LE_ERROR("capture thread is already started");
        return LE_BUSY;
    }

    if (streamPtr->audioInterface != LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE)
    {
        LE_ERROR("Invalid interface");
        return LE_BAD_PARAMETER;
    }

    le_audio_PcmContext_t* pcmContextPtr = le_mem_ForceAlloc(PcmThreadContextPool);
    pcm_Handle_t pcmHandle = NULL;

    memset(pcmContextPtr, 0, sizeof(le_audio_PcmContext_t));

    pcmContextPtr->fd = streamPtr->fd;

    samplePcmConfigPtr->byteRate = (samplePcmConfigPtr->sampleRate *
                                    samplePcmConfigPtr->channelsCount *
                                    samplePcmConfigPtr->bitsPerSample) / 8;

    memcpy(&(pcmContextPtr->pcmConfig), samplePcmConfigPtr, sizeof(le_audio_SamplePcmConfig_t));

    pcmContextPtr->mainThreadRef = le_thread_GetCurrent();
    pcmContextPtr->interface = streamPtr->audioInterface;
    pcmContextPtr->pause = false;
    streamPtr->pcmContextPtr = pcmContextPtr;

    char deviceString[STRING_LEN];
    snprintf(deviceString,sizeof(deviceString),"hw:0,%d", streamPtr->hwDeviceId);
    LE_DEBUG("Hardware interface: %s", deviceString);

    // Request a wakeup source for media streams
    le_pm_StayAwake(MediaWakeLock);

    if ((pa_pcm_InitCapture(&pcmHandle, deviceString, &(pcmContextPtr->pcmConfig)) != LE_OK) ||
        (pcmHandle == NULL))
    {
        LE_ERROR("PCM cannot be open");
        le_media_Stop(streamPtr);
        return LE_FAULT;
    }

    pcmContextPtr->pcmHandle = pcmHandle;

    pa_pcm_SetCallbackHandlers( pcmHandle,
                                SetCaptureFrames,
                                PlayCaptResult,
                                streamPtr );
    if (pa_pcm_Capture(pcmHandle) != LE_OK)
    {
        LE_ERROR("PCM cannot be open");
        le_media_Stop(streamPtr);
        return LE_FAULT;
    }

    return LE_OK;
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
        if (streamPtr->pcmContextPtr)
        {
            LE_DEBUG("Stream in use pcmContextPtr %p", streamPtr->pcmContextPtr);
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
                                                               sizeof(le_audio_PcmContext_t));

    // Create a Wakeup source for Media
    MediaWakeLock = le_pm_NewWakeupSource( LE_PM_REF_COUNT, "MediaStream" );
}
