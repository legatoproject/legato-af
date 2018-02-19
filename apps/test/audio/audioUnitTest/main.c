/**
 * This module implements the unit tests for AUDIO API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "le_audio_local.h"
#include "pa_audio.h"
#include "log.h"
#include "pa_pcm_simu.h"
#include "pa_audio_simu.h"
#include <string.h>

#define BUFFER_LEN  5000

static le_sem_Ref_t    ThreadSemaphore;
static le_thread_Ref_t TestThreadRef;
static int Pipefd[2];
static int FileFd;
static le_audio_MediaHandlerRef_t MediaHandlerRef;
static le_audio_MediaHandlerRef_t FakeHandlerRef;
static le_audio_DtmfDetectorHandlerRef_t DtmfDetectorHandlerRef;
static uint8_t Buffer[BUFFER_LEN];
static const char AmrWbStr[]="#!AMR-WB\n";
static char Dtmf;
static char DtmfList[]="0123456789ABCD*#";
static uint32_t DtmfDuration = 10;
static uint32_t DtmfPause = 20;
static le_audio_StreamRef_t FakeStreamRef;
static le_audio_StreamRef_t StreamRef[LE_AUDIO_NUM_INTERFACES];


enum
{
    TEST_PLAY_SAMPLES,
    TEST_PLAY_SAMPLES_IN_PROGRESS,
    TEST_PLAY_FILES,
    TEST_PLAY_FILES_IN_PROGRESS,
    TEST_REC_SAMPLES,
    TEST_REC_FILES,
    TEST_DTMF_DECODING,
    TEST_PLAY_DTMF,
    TEST_PLAY_DTMF_IN_PROGRESS,
    TEST_LAST
} TestCase;

typedef struct {
    uint32_t riffId;
    uint32_t riffSize;
    uint32_t riffFmt;
    uint32_t fmtId;
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t channelsCount;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    uint32_t dataId;
    uint32_t dataSize;
} WavHeader_t;

//--------------------------------------------------------------------------------------------------
/**
 * Redefinition of le_audio COMPONENT_INIT
 */
//--------------------------------------------------------------------------------------------------
void le_audio_Init(void);

//--------------------------------------------------------------------------------------------------
/**
 * Redefinition of pa_audio COMPONENT_INIT
 */
//--------------------------------------------------------------------------------------------------
void pa_audio_Init(void);

//--------------------------------------------------------------------------------------------------
/**
 * Connect the current client thread to the service providing this API (STUBBED FUNCTION)
 *
 */
//--------------------------------------------------------------------------------------------------
void le_pm_ConnectService
(
    void
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Acquire a wakeup source (STUBBED FUNCTION)
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pm_StayAwake
(
    le_pm_WakeupSourceRef_t w
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Release a wakeup source (STUBBED FUNCTION)
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pm_Relax
(
    le_pm_WakeupSourceRef_t w
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a new wakeup source (STUBBED FUNCTION)
 *
 * @return Reference to wakeup source
 */
//--------------------------------------------------------------------------------------------------
le_pm_WakeupSourceRef_t le_pm_NewWakeupSource
(
    uint32_t    opts,
    const char *tag
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test thread destructor.
 */
//--------------------------------------------------------------------------------------------------
static void DestroyTestThread
(
    void *contextPtr
)
{
    if (TestCase == TEST_DTMF_DECODING)
    {
        le_audio_RemoveDtmfDetectorHandler(DtmfDetectorHandlerRef);
    }
    else
    {
        le_audio_RemoveMediaHandler(MediaHandlerRef);
    }

    // Don't remove this handler: it should be automatically removed when the le_audio_Close is
    // called. The goal is to test RemoveAllHandlersFromHdlrLists() function
    // le_audio_RemoveMediaHandler(FakeHandlerRef);

    le_audio_Close(FakeStreamRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Second media handler (shouldn't be called).
 */
//--------------------------------------------------------------------------------------------------
static void FakeHandler
(
    le_audio_StreamRef_t streamRef,
    le_audio_MediaEvent_t event,
    void* contextPtr
)
{
    // fatal as it is not supposed to be called
    LE_FATAL("Unused stream handler called");
}

//--------------------------------------------------------------------------------------------------
/**
 * media handler.
 */
//--------------------------------------------------------------------------------------------------
static void MyMediaHandler
(
    le_audio_StreamRef_t streamRef,
    le_audio_MediaEvent_t event,
    void* contextPtr
)
{
    // Ensure that the contextPtr is correctly received
    le_audio_StreamRef_t myStreamRef = (le_audio_StreamRef_t) contextPtr;
    LE_ASSERT(streamRef == myStreamRef);

    LE_INFO("event %d", event);

    // Test the event depending on the test case
    switch(TestCase)
    {
        case TEST_PLAY_SAMPLES_IN_PROGRESS:
            LE_ASSERT(event == LE_AUDIO_MEDIA_NO_MORE_SAMPLES);
            TestCase = TEST_LAST;
        break;
        case TEST_PLAY_FILES_IN_PROGRESS:
        case TEST_PLAY_DTMF_IN_PROGRESS:
            LE_ASSERT(event == LE_AUDIO_MEDIA_ENDED);
            TestCase = TEST_LAST;
        break;
        default:
            LE_FATAL("Unexpected event %d", TestCase);
        break;
    }

    // Unlock the test function
    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Dtmf decoding handler.
 */
//--------------------------------------------------------------------------------------------------
static void DtmfDecodingHandler
(
    le_audio_StreamRef_t streamRef,
    char dtmf,
    void* contextPtr
)
{
    // Ensure that the contextPtr is correctly received
    le_audio_StreamRef_t myStreamRef = (le_audio_StreamRef_t) contextPtr;
    LE_ASSERT(streamRef == myStreamRef);

    // Test dtmf
    LE_ASSERT(Dtmf == dtmf);

    // Unlock the test function
    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test thread.
 */
//--------------------------------------------------------------------------------------------------
static void* TestThread
(
    void* contextPtr
)
{
    le_audio_StreamRef_t myStreamRef = (le_audio_StreamRef_t) contextPtr;

    if (TestCase == TEST_DTMF_DECODING)
    {
        // Add a dtmf decoding handler for this test
        DtmfDetectorHandlerRef = le_audio_AddDtmfDetectorHandler(myStreamRef,
                                                                 DtmfDecodingHandler,
                                                                 myStreamRef);
        LE_ASSERT(DtmfDetectorHandlerRef != NULL);
    }
    else
    {
        // Add a media handler
        MediaHandlerRef = le_audio_AddMediaHandler(myStreamRef, MyMediaHandler, myStreamRef);
        LE_ASSERT(MediaHandlerRef != NULL);
    }

    // Try to subscribe another handler on a different stream. This handler shouldn't be called
    if ((TestCase == TEST_PLAY_SAMPLES) ||
        (TestCase == TEST_PLAY_FILES) ||
        (TestCase == TEST_PLAY_DTMF))
    {
        // For play tests, subscribe to recorder stream
        FakeStreamRef = le_audio_OpenRecorder();
    }
    else
    {
        // For recording test, subscribe to player stream
        FakeStreamRef = le_audio_OpenPlayer();
    }

    // Add a handler on the other stream, and check that it is never called.
    LE_ASSERT(FakeStreamRef != NULL);
    FakeHandlerRef = le_audio_AddMediaHandler(FakeStreamRef, FakeHandler, NULL);
    LE_ASSERT(FakeHandlerRef != NULL);

    // Execute APIs according to the test case
    switch(TestCase)
    {
        case TEST_PLAY_SAMPLES:
            TestCase = TEST_PLAY_SAMPLES_IN_PROGRESS;
            LE_ASSERT(le_audio_PlaySamples(myStreamRef, Pipefd[0]) == LE_OK);
        break;
        case TEST_PLAY_FILES:
            TestCase = TEST_PLAY_FILES_IN_PROGRESS;
            LE_ASSERT(le_audio_PlayFile(myStreamRef, FileFd) == LE_OK);
            LE_ASSERT(le_audio_Resume(myStreamRef) == LE_FAULT);
        break;
        case TEST_REC_SAMPLES:
            LE_ASSERT(le_audio_GetSamples(myStreamRef, Pipefd[1]) == LE_OK);
        break;
        case TEST_REC_FILES:
            LE_ASSERT(le_audio_RecordFile(myStreamRef, FileFd) == LE_OK);
        break;
        case TEST_PLAY_DTMF:
            TestCase = TEST_PLAY_DTMF_IN_PROGRESS;
            LE_ASSERT(le_audio_PlayDtmf(myStreamRef, DtmfList, DtmfDuration, DtmfPause) == LE_OK);
        break;
        default:
        break;
    }

    // Unlock CreateTestThread() function
    le_sem_Post(ThreadSemaphore);

    // Run the event loop
    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a thread to launch play/record/dtmf APIs.
 * These APIs need an event loop to run.
 */
//--------------------------------------------------------------------------------------------------
static void CreateTestThread
(
    le_audio_StreamRef_t streamRef
)
{
    /* Create the thread to subcribe and call the handlers */
    TestThreadRef = le_thread_Create("Threadhandler", TestThread, streamRef);
    le_thread_AddChildDestructor(TestThreadRef,
                                 DestroyTestThread,
                                 NULL);

    le_thread_SetJoinable(TestThreadRef);

    le_thread_Start(TestThreadRef);

    /* Wait the thread to be ready */
    le_sem_Wait(ThreadSemaphore);
}


//--------------------------------------------------------------------------------------------------
// Test functions.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Try to open all possible stream.
 * Check if the returned references are not null.
 *
 * API tested:
 * - le_audio_OpenMic
 * - le_audio_OpenSpeaker
 * - le_audio_OpenUsbRx
 * - le_audio_OpenUsbTx
 * - le_audio_OpenModemVoiceRx
 * - le_audio_OpenModemVoiceTx
 * - le_audio_OpenPcmRx
 * - le_audio_OpenPcmTx
 * - le_audio_OpenI2sRx
 * - le_audio_OpenI2sTx
 * - le_audio_OpenPlayer
 * - le_audio_OpenRecorder
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_audio_OpenStream
(
    void
)
{
    le_audio_If_t audioIf = 0;

    memset(StreamRef,0,LE_AUDIO_NUM_INTERFACES*sizeof(le_audio_StreamRef_t));

    // Open all streams
    for (; audioIf < LE_AUDIO_NUM_INTERFACES; audioIf++)
    {
        switch (audioIf)
        {
            case LE_AUDIO_IF_CODEC_MIC:
                StreamRef[audioIf] = le_audio_OpenMic();
            break;
            case LE_AUDIO_IF_CODEC_SPEAKER:
                StreamRef[audioIf] = le_audio_OpenSpeaker();
            break;
            case LE_AUDIO_IF_DSP_FRONTEND_USB_RX:
                StreamRef[audioIf] = le_audio_OpenUsbRx();
            break;
            case LE_AUDIO_IF_DSP_FRONTEND_USB_TX:
                StreamRef[audioIf] = le_audio_OpenUsbTx();
            break;
            case LE_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX:
                StreamRef[audioIf] = le_audio_OpenModemVoiceRx();
            break;
            case LE_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX:
                StreamRef[audioIf] = le_audio_OpenModemVoiceTx();
            break;
            case LE_AUDIO_IF_DSP_FRONTEND_PCM_RX:
                StreamRef[audioIf] = le_audio_OpenPcmRx(1);
            break;
            case LE_AUDIO_IF_DSP_FRONTEND_PCM_TX:
                StreamRef[audioIf] = le_audio_OpenPcmTx(1);
            break;
            case LE_AUDIO_IF_DSP_FRONTEND_I2S_RX:
                StreamRef[audioIf] = le_audio_OpenI2sRx(LE_AUDIO_I2S_STEREO);
            break;
            case LE_AUDIO_IF_DSP_FRONTEND_I2S_TX:
                StreamRef[audioIf] = le_audio_OpenI2sTx(LE_AUDIO_I2S_STEREO);
            break;
            case LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY:
                StreamRef[audioIf] = le_audio_OpenPlayer();
            break;
            case LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE:
                StreamRef[audioIf] = le_audio_OpenRecorder();
            break;
            default:
                LE_FATAL("Unknown stream" );
            break;
        }
    }

    for (audioIf=0; audioIf < LE_AUDIO_NUM_INTERFACES; audioIf++)
    {
        LE_ASSERT(StreamRef[audioIf] != NULL);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Close all opened streams
 *
 * API tested:
 * - le_audio_Close
 *
 * No error can be checked (no returned error code, no external functions call)
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_audio_CloseStream
(
    void
)
{
    le_audio_If_t audioIf = 0;

    for (; audioIf < LE_AUDIO_NUM_INTERFACES; audioIf++)
    {
        le_audio_Close(StreamRef[audioIf]);
        StreamRef[audioIf] = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test connector creation/deletion
 * Test stream connection to the connector. Check audio paths requested.
 *
 * Sub-test 1: Connect all streams to a created connector. Check audio paths set.
 * Disconnect all streams. Check that all audio paths are reseted.
 *
 * Sub-test 2: Connect again all streams to the creator.
 * Delete the connector. Check that all audio paths are reseted.
 *
 * Sub-test 3 : Try to connect a stream to a deleted connector (error expected)
 *
 * Sub-test 4 : Connect all streams to a created connector. Check audio paths set.
 * Deleted all streams. Check that all audio paths are reseted.
 *
 * API tested:
 * - le_audio_CreateConnector
 * - le_audio_DeleteConnector
 * - le_audio_Connect
 * - le_audio_Disconnect
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_audio_Connector
(
    void
)
{
    le_audio_ConnectorRef_t connectorRef = le_audio_CreateConnector();

    LE_ASSERT(connectorRef != NULL);

    // Open all streams
    Testle_audio_OpenStream();

    //------------
    // Sub-test 1
    //------------

    // Connect all streams to the connector
    le_audio_If_t audioIf = 0;

    for (; audioIf < LE_AUDIO_NUM_INTERFACES; audioIf++)
    {
        LE_ASSERT(le_audio_Connect(connectorRef, StreamRef[audioIf])==LE_OK);
    }

    // Try to connect again: returned error code expected
    for (audioIf = 0; audioIf < LE_AUDIO_NUM_INTERFACES; audioIf++)
    {
        LE_ASSERT(le_audio_Connect(connectorRef, StreamRef[audioIf])==LE_BUSY);
    }

    // Check the audio path
    LE_ASSERT(pa_audioSimu_CheckAudioPathSet() == LE_OK);

    // Disconnect all streams to the connector
    for (audioIf = 0; audioIf < LE_AUDIO_NUM_INTERFACES; audioIf++)
    {
        le_audio_Disconnect(connectorRef, StreamRef[audioIf]);
    }

    LE_ASSERT(pa_audioSimu_CheckAudioPathReseted() == LE_OK);

    //------------
    // Sub-test 2
    //------------

    // Connect again, and check that, when the connector is deleted, all audio path have been
    // reseted
    for (audioIf=0; audioIf < LE_AUDIO_NUM_INTERFACES; audioIf++)
    {
        LE_ASSERT(le_audio_Connect(connectorRef, StreamRef[audioIf])==LE_OK);
    }

    // Check the audio path
    LE_ASSERT(pa_audioSimu_CheckAudioPathSet() == LE_OK);

    // Delete the connector
    le_audio_DeleteConnector(connectorRef);

    LE_ASSERT(pa_audioSimu_CheckAudioPathReseted() == LE_OK);

    //------------
    // Sub-test 3
    //------------

    // Try to connect a stream to a deleted connector: error expected
    LE_ASSERT(le_audio_Connect(connectorRef, StreamRef[0])==LE_BAD_PARAMETER);

    //------------
    // Sub-test 4
    //------------

    // Create a new connector
    connectorRef = le_audio_CreateConnector();

    // Connect all streams
    for (audioIf = 0; audioIf < LE_AUDIO_NUM_INTERFACES; audioIf++)
    {
        LE_ASSERT(le_audio_Connect(connectorRef, StreamRef[audioIf])==LE_OK);
    }

    // Check the audio path
    LE_ASSERT(pa_audioSimu_CheckAudioPathSet() == LE_OK);

    // Close all streams
    Testle_audio_CloseStream();

    // Check that all audio paths have been reseted
    LE_ASSERT(pa_audioSimu_CheckAudioPathReseted() == LE_OK);

    // Delete the connector
    le_audio_DeleteConnector(connectorRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test the play samples functionality.
 * Audio samples (fake data) are sent in the pipe and recorded into the pa_pcm_simu.
 * When the event "LE_AUDIO_MEDIA_NO_MORE_SAMPLES" is received, the test check the received data.
 *
 * API tested:
 * - le_audio_PlaySamples
 * - le_audio_AddMediaHandler
 * - le_audio_Stop
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_audio_PlaySamples
(
    void
)
{
    int i;
    le_audio_StreamRef_t playbackStreamRef = NULL;

    LE_ASSERT(pipe(Pipefd) == 0);

    // Init the pcm buffer in pa_pcm_simu side.
    pa_pcmSimu_InitData(BUFFER_LEN);

    // Open the player stream
    playbackStreamRef = le_audio_OpenPlayer();
    LE_ASSERT(playbackStreamRef != NULL);

    // Set the test case
    TestCase = TEST_PLAY_SAMPLES;

    for (i=0; i < BUFFER_LEN; i++)
    {
        LE_ASSERT(write(Pipefd[1],&Buffer[i],1) == 1);
    }

    // Create the test thread which will execute le_audio_PlaySamples and le_audio_AddMediaHandler
    CreateTestThread(playbackStreamRef);

    // Wait the event LE_AUDIO_MEDIA_NO_MORE_SAMPLES
    le_sem_Wait(ThreadSemaphore);

    // Get the buffer address of the received data in the pa_pcm_simu
    uint8_t* sentPcmPtr = pa_pcmSimu_GetDataPtr();

    // check data
    LE_ASSERT(memcmp(Buffer, sentPcmPtr, BUFFER_LEN) == 0);

    // Release buffer in pa_pcm_simu
    pa_pcmSimu_ReleaseData();

    // Stop
    LE_ASSERT(le_audio_Stop(playbackStreamRef) == LE_OK);

    // Closing the input pipe is unncessary since the messaging infrastructure underneath
    // le_audio_PlaySamples would close it.

    // Stop the test thread
    le_thread_Cancel(TestThreadRef);
    le_thread_Join(TestThreadRef,NULL);

    // Close the player stream
    le_audio_Close(playbackStreamRef);

    // Check that no more call of the semaphore
    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test the play file functionality.
 * A (fake) audio file is built and play. The sent pcm are captured into the pa_pcm_simu.
 * When the event "LE_AUDIO_MEDIA_ENDED" is received, the test check the received data.
 *
 * API tested:
 * - le_audio_PlayFile
 * - le_audio_Resume
 * - le_audio_Pause
 * - le_audio_AddMediaHandler
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_audio_PlayFile
(
    void
)
{
    unlink("test.amrwb");

    le_audio_StreamRef_t playbackStreamRef = NULL;

    // Create an AMR_WB file
    int fd = open("./test.amrwb", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR );

    write (fd, AmrWbStr, strlen(AmrWbStr));
    write (fd, Buffer, BUFFER_LEN);

    close(fd);

    // Try to play the file
    FileFd = open("./test.amrwb", O_RDONLY);
    LE_ASSERT(FileFd != -1);

    // Init the pcm buffer in pa_pcm_simu side.
    pa_pcmSimu_InitData(BUFFER_LEN);

    // Open the player stream
    playbackStreamRef = le_audio_OpenPlayer();
    LE_ASSERT(playbackStreamRef != NULL);

    // Set the test case
    TestCase = TEST_PLAY_FILES;

    // Create the test thread which will execute le_audio_PlayFile and le_audio_AddMediaHandler
    CreateTestThread(playbackStreamRef);

    // Wait the event LE_AUDIO_MEDIA_ENDED
    le_sem_Wait(ThreadSemaphore);

    // Closing the fd is unncessary since the messaging infrastructure underneath
    // le_audio_PlayFile would close it.

    // Get the buffer address of the received data in the pa_pcm_simu
    uint8_t* sentPcmPtr = pa_pcmSimu_GetDataPtr();

    // Check data
    LE_ASSERT(memcmp(Buffer, sentPcmPtr, BUFFER_LEN) == 0);

    // Release buffer in pa_pcm_simu
    pa_pcmSimu_ReleaseData();

    // Stop the test thread
    le_thread_Cancel(TestThreadRef);
    le_thread_Join(TestThreadRef,NULL);

    // Close the player stream
    le_audio_Close(playbackStreamRef);

    // Delete the created file
    unlink("test.amrwb");

    // Check that no more call of the semaphore
    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test the capture samples functionality.
 * Audio samples (fake data) are sent in the pipe by the pa_pcm_simu.
 * When all the data are received, the test checks the received data.
 *
 * API tested:
 * - le_audio_GetSamples
 * - le_audio_AddMediaHandler
 * - le_audio_Stop
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_audio_CaptureSamples
(
    void
)
{
    int i;
    le_audio_StreamRef_t captureStreamRef = NULL;

    LE_ASSERT(pipe(Pipefd) == 0);

    // Init the pcm buffer in pa_pcm_simu side.
    pa_pcmSimu_InitData(BUFFER_LEN);

    // Init the data in the pa_pcm_simu
    uint8_t* dataToReceivePtr = pa_pcmSimu_GetDataPtr();

    memcpy(dataToReceivePtr, Buffer, BUFFER_LEN);

    // Open the recorder stream
    captureStreamRef = le_audio_OpenRecorder();
    LE_ASSERT(captureStreamRef != NULL);

    // Set the test case
    TestCase = TEST_REC_SAMPLES;

    // Create the test thread which will execute le_audio_GetSamples
    CreateTestThread(captureStreamRef);

    uint8_t* receivedPcmFramesPtr = malloc(BUFFER_LEN);
    LE_ASSERT(NULL != receivedPcmFramesPtr);

    // Read the data on the pipe
    for (i=0; i < BUFFER_LEN; i+=10)
    {
        LE_ASSERT(read(Pipefd[0],receivedPcmFramesPtr+i,10) == 10);
    }

    // All the expected data are received: stop the recoding
    LE_ASSERT(le_audio_Stop(captureStreamRef) == LE_OK);

    // Closing the output pipe is unncessary since the messaging infrastructure underneath
    // le_audio_CaptureSamples would close it.

    // Check data
    LE_ASSERT(memcmp(Buffer, receivedPcmFramesPtr, BUFFER_LEN) == 0);

    // Release buffer in pa_pcm_simu
    pa_pcmSimu_ReleaseData();

    // Stop the test thread
    le_thread_Cancel(TestThreadRef);
    le_thread_Join(TestThreadRef,NULL);

    // Close the recorder stream
    le_audio_Close(captureStreamRef);

    // Check that no more call of the semaphore
    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);

    free(receivedPcmFramesPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test play to invalid destination.
 *
 * Should return an error.
 *
 * API tested:
 * - le_audio_AddMediaHandler
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_audio_PlayInvalid
(
    void
)
{
    le_audio_StreamRef_t playbackStreamRef = NULL;

    // Open the player stream
    playbackStreamRef = le_audio_OpenUsbTx();
    LE_ASSERT(playbackStreamRef != NULL);

    // Try to attach a media handler
    MediaHandlerRef = le_audio_AddMediaHandler(playbackStreamRef,
                                               MyMediaHandler, playbackStreamRef);
    LE_ASSERT(MediaHandlerRef == NULL);

    // Close the player stream
    le_audio_Close(playbackStreamRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test the file capture functionality.
 * Audio samples (fake data) are sent in the pipe by the pa_pcm_simu.
 * When all the data are sent, the recorded file is checked.
 *
 * API tested:
 * - le_audio_RecordFile
 * - le_audio_AddMediaHandler
 * - le_audio_Stop
 * - le_audio_SetEncodingFormat
 * - le_audio_SetSamplePcmChannelNumber
 * - le_audio_GetSamplePcmChannelNumber
 * - le_audio_SetSamplePcmSamplingRate
 * - le_audio_GetSamplePcmSamplingRate
 * - le_audio_SetSamplePcmSamplingResolution
 * - le_audio_GetSamplePcmSamplingResolution
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_audio_RecordFile
(
    void
)
{
    unlink("test.wav");

    le_audio_StreamRef_t captureStreamRef = NULL;
    uint32_t nbChannel = 2, tmpNbChannel=0;
    uint32_t sampleRate = 44100, tmpSampleRate = 0;
    uint32_t bitsPerSample = 8, tmpBitsPerSample = 0;

    // Create a wav file
    FileFd = open("./test.wav", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR );
    LE_ASSERT(FileFd != -1);

    // Init the pcm buffer in pa_pcm_simu side.
    pa_pcmSimu_InitData(BUFFER_LEN);

    // Open the recorder stream
    captureStreamRef = le_audio_OpenRecorder();
    LE_ASSERT(captureStreamRef != NULL);

    // Set the samples configuration
    LE_ASSERT(le_audio_SetEncodingFormat(captureStreamRef, LE_AUDIO_WAVE) == LE_OK);
    LE_ASSERT(le_audio_SetSamplePcmChannelNumber(captureStreamRef, nbChannel) == LE_OK);
    LE_ASSERT(le_audio_GetSamplePcmChannelNumber(captureStreamRef, &tmpNbChannel) == LE_OK);
    LE_ASSERT(nbChannel == tmpNbChannel);
    LE_ASSERT(le_audio_SetSamplePcmSamplingRate(captureStreamRef, sampleRate) == LE_OK);
    LE_ASSERT(le_audio_GetSamplePcmSamplingRate(captureStreamRef, &tmpSampleRate) == LE_OK);
    LE_ASSERT(sampleRate == tmpSampleRate);
    LE_ASSERT(le_audio_SetSamplePcmSamplingResolution(captureStreamRef, bitsPerSample) == LE_OK);
    LE_ASSERT(le_audio_GetSamplePcmSamplingResolution(captureStreamRef,
              &tmpBitsPerSample) == LE_OK);
    LE_ASSERT(bitsPerSample == tmpBitsPerSample);

    // Send the semaphore to the pa_pcm_simu
    pa_pcmSimu_SetSemaphore(&ThreadSemaphore);

    // Init the data in the pa_pcm_simu
    uint8_t* sentPcmPtr = pa_pcmSimu_GetDataPtr();

    memcpy(sentPcmPtr,Buffer,BUFFER_LEN);

    // Set the test case
    TestCase = TEST_REC_FILES;

    // Create the test thread which will execute le_audio_RecordFile
    CreateTestThread(captureStreamRef);

    // Wait for pa_pcm_simu
    le_sem_Wait(ThreadSemaphore);

    // Stop the capture
    LE_ASSERT(le_audio_Stop(captureStreamRef) == LE_OK);

    // Release buffer in pa_pcm_simu
    pa_pcmSimu_ReleaseData();

    // Stop the test thread
    le_thread_Cancel(TestThreadRef);
    le_thread_Join(TestThreadRef,NULL);

    // Close the recorder stream
    le_audio_Close(captureStreamRef);

    // Close the fd
    // Closing the fd is unncessary since the messaging infrastructure underneath
    // le_audio_RecordFiles would close it.

    // Check recording file
    FileFd = open("./test.wav", O_RDONLY);
    LE_ASSERT(FileFd != -1);

    uint32_t fileLen = lseek(FileFd, 0, SEEK_END);
    LE_ASSERT(fileLen >= sizeof(WavHeader_t));
    lseek(FileFd, 0, SEEK_SET);
    uint8_t file[fileLen+1];
    memset(file, 0,fileLen+1);
    LE_ASSERT(read(FileFd, file, fileLen) == fileLen);
    close(FileFd);

    WavHeader_t* hdPtr = (WavHeader_t*) file;

    // Check data
    LE_ASSERT(memcmp(&hdPtr->riffId, "RIFF", sizeof(hdPtr->riffId)) == 0);
    LE_ASSERT(memcmp(&hdPtr->riffFmt, "WAVE", sizeof(hdPtr->riffFmt)) == 0);
    LE_ASSERT(memcmp(&hdPtr->fmtId, "fmt ", sizeof(hdPtr->fmtId)) == 0);
    LE_ASSERT(memcmp(&hdPtr->dataId, "data", sizeof(hdPtr->dataId)) == 0);
    LE_ASSERT(hdPtr->riffSize == (fileLen - sizeof(hdPtr->riffId) - sizeof(hdPtr->riffSize)));
    LE_ASSERT(hdPtr->fmtSize == 16);
    LE_ASSERT(hdPtr->audioFormat == 1);
    LE_ASSERT(hdPtr->channelsCount == nbChannel);
    LE_ASSERT(hdPtr->sampleRate == sampleRate);
    LE_ASSERT(hdPtr->bitsPerSample == bitsPerSample);
    LE_ASSERT(hdPtr->byteRate == (sampleRate*nbChannel*bitsPerSample)/8);
    LE_ASSERT(hdPtr->blockAlign == nbChannel*bitsPerSample/8);
    LE_ASSERT(hdPtr->dataSize == (fileLen - sizeof(WavHeader_t)));

    int i = 0;
    int index=0;

    for (; i < hdPtr->dataSize; i++)
    {
        LE_ASSERT(*(file+sizeof(WavHeader_t)+i) == *(Buffer+index));
        index++;
        if (index == BUFFER_LEN)
        {
            index=0;
        }
    }

    unlink("test.wav");

    // Check that no more call of the semaphore
    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test the dtmf decoding functionality.
 * dtmf are simulated in the pa_audio_simu. The test checks that the the events are received with
 * the correct dtmf.
 *
 *
 * API tested:
 * - le_audio_AddDtmfDetectorHandler
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_audio_DecodingDtmf
(
    void
)
{
    // Open the modem voice RX stream
    le_audio_StreamRef_t streamVoiceRxRef = le_audio_OpenModemVoiceRx();

    char dtmfList[]="0123456789ABCD*#";

    // Set the test case
    TestCase = TEST_DTMF_DECODING;

    // Create the test thread which will execute le_audio_AddDtmfDetectorHandler
    CreateTestThread(streamVoiceRxRef);

    // Set the dtmf to be played in the pa_audio_simu
    int i;
    for (i=0; i < strlen(dtmfList); i++)
    {
        Dtmf = dtmfList[i];
        pa_audioSimu_ReceiveDtmf(Dtmf);
        le_sem_Wait(ThreadSemaphore);
    }

    // Stop the test thread
    le_thread_Cancel(TestThreadRef);
    le_thread_Join(TestThreadRef,NULL);

    // Close the modem voice RX stream
    le_audio_Close(streamVoiceRxRef);

    // Check that no more call of the semaphore
    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test the dtmf playing functionality.
 * Sub-test 1 : dtmf is played over the network
 * Sub-test 2 : dtmf are played over the network and in local mode.
 *
 * API tested:
 * - le_audio_PlaySignallingDtmf
 * - le_audio_PlayDtmf
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_audio_PlayDtmf
(
    void
)
{
    //------------
    // Sub-test 1
    //------------

    // Try to generate a dtmf over the network
    pa_audioSimu_PlaySignallingDtmf(DtmfList, DtmfDuration, DtmfPause);
    LE_ASSERT(le_audio_PlaySignallingDtmf( DtmfList, DtmfDuration, DtmfPause ) == LE_OK);

    //------------
    // Sub-test 2
    //------------

    // Try to play a dtmf in local

    // Open the player stream
    le_audio_StreamRef_t  playbackStreamRef = le_audio_OpenPlayer();

    // Init the buffer in the pa_pcm_simu
    uint32_t sampleRate = 16000;
    uint32_t sampleCount = sampleRate*(DtmfDuration+DtmfPause)*4*strlen(DtmfList)/1000;
    pa_pcmSimu_InitData(sampleCount*2);

    // Set the test case
    TestCase = TEST_PLAY_DTMF;

    // Create the test thread which will execute le_audio_PlayDtmf
    CreateTestThread(playbackStreamRef);

    // Wait the event LE_AUDIO_MEDIA_ENDED
    le_sem_Wait(ThreadSemaphore);

    // Get the buffer address of the received data in the pa_pcm_simu
    uint8_t* dataPtr = pa_pcmSimu_GetDataPtr();

    // Check that something has been wrote
    LE_ASSERT((uint32_t) *dataPtr == 0x00000000);

    pa_pcmSimu_ReleaseData();

    // Stop the test thread
    le_thread_Cancel(TestThreadRef);
    le_thread_Join(TestThreadRef,NULL);

    // Close the player stream
    le_audio_Close(playbackStreamRef);

    // Check that no more call of the semaphore
    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test the capturing status of Echo canceller and noise suppressor.
 * echo canceller and noise suppressor are simulated in the pa_audio_simu. The test checks that the
 * streamref is not NULL.
 *
 *
 * API tested:
 * - le_audio_IsNoiseSuppressorEnabled
 * - le_audio_IsEchoCancellerEnabled
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_audio_EchoCancellerNoiseSuppressor
(
    void
)
{
    bool status;
    le_audio_StreamRef_t MdmTxAudioRef;

    MdmTxAudioRef = le_audio_OpenModemVoiceTx();

    LE_ASSERT(MdmTxAudioRef != NULL)

    //------------
    // Sub-test 1
    //------------

    LE_ASSERT(le_audio_EnableNoiseSuppressor(MdmTxAudioRef) == LE_OK);

    LE_ASSERT(le_audio_IsNoiseSuppressorEnabled(MdmTxAudioRef, &status) == LE_OK);

    LE_ASSERT(status == 1);

    //------------
    // Sub-test 2
    //------------

    LE_ASSERT(le_audio_DisableNoiseSuppressor(MdmTxAudioRef) == LE_OK);

    LE_ASSERT(le_audio_IsNoiseSuppressorEnabled(MdmTxAudioRef, &status) == LE_OK);

    LE_ASSERT(status == 0);

    //------------
    // Sub-test 3
    //------------

    LE_ASSERT(le_audio_EnableEchoCanceller(MdmTxAudioRef) == LE_OK);

    LE_ASSERT(le_audio_IsEchoCancellerEnabled(MdmTxAudioRef, &status) == LE_OK);

    LE_ASSERT(status == 1);

    //------------
    // Sub-test 4
    //------------

    LE_ASSERT(le_audio_DisableEchoCanceller(MdmTxAudioRef) == LE_OK);

    LE_ASSERT(le_audio_IsEchoCancellerEnabled(MdmTxAudioRef, &status) == LE_OK);

    LE_ASSERT(status == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * main thread: this thread is used to initialized the le_audio and pa_audio, and to have an event
 * loop to treat the events.
 *
 */
//--------------------------------------------------------------------------------------------------
void* MainThread
(
    void* contextPtr
)
{
    le_sem_Post(ThreadSemaphore);

    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // To reactivate for all DEBUG logs
    // le_log_SetFilterLevel(LE_LOG_DEBUG);

    ThreadSemaphore = le_sem_Create("HandlerSem",0);

    // Create the thread to subcribe and call the handlers
    le_thread_Start(le_thread_Create("MainThread", MainThread, NULL));

    // Wait the thread to be ready
    le_sem_Wait(ThreadSemaphore);

    // Prepare the audio samples buffer
    int i;
    for (i=0; i < BUFFER_LEN; i++)
    {
        Buffer[i] = i % 255;
    }

    LE_INFO("======== Start UnitTest of AUDIO API ========");

    LE_INFO("======== Test all Open stream APIs ========");
    Testle_audio_OpenStream();

    LE_INFO("======== Test Close Opened stream ========");
    Testle_audio_CloseStream();

    LE_INFO("======== Test Connector ========");
    Testle_audio_Connector();

    LE_INFO("======== Test play samples ========");
    Testle_audio_PlaySamples();

    LE_INFO("======== Test play file ========");
    Testle_audio_PlayFile();

    LE_INFO("======== Test play to invalid destination ========");
    Testle_audio_PlayInvalid();

    LE_INFO("======== Test capture samples ========");
    Testle_audio_CaptureSamples();

    LE_INFO("======== Test capture file ========");
    Testle_audio_RecordFile();

    LE_INFO("======== Test decoding dtmf ========");
    Testle_audio_DecodingDtmf();

    LE_INFO("======== Test play dtmf ========");
    Testle_audio_PlayDtmf();

    LE_INFO("======== Test Echo canceller and Noise suppressor ========");
    Testle_audio_EchoCancellerNoiseSuppressor();

    LE_INFO("======== UnitTest of AUDIO API ends with SUCCESS ========");
    exit(0);
}
