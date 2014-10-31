//--------------------------------------------------------------------------------------------------
/**
 * @file le_audio.c
 *
 * This file contains the source code of the high level Audio API.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013-2014. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "pa_audio.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Define the HashMap size.
 */
//--------------------------------------------------------------------------------------------------
#define AUDIO_HASHMAP_SIZE      10

//--------------------------------------------------------------------------------------------------
/**
 * Define the pools size.
 */
//--------------------------------------------------------------------------------------------------
#define STREAM_DEFAULT_POOL_SIZE        1
#define CONNECTOR_DEFAULT_POOL_SIZE     1
#define HASHMAP_DEFAULT_POOL_SIZE       1


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of audio stream objects we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_OF_STREAM               8

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Audio connector objects we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_OF_CONNECTOR            8

//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum size of format stream related field.
 */
//--------------------------------------------------------------------------------------------------
#define FORMAT_NAME_MAX_LEN             30
#define FORMAT_NAME_MAX_BYTES           (FORMAT_NAME_MAX_LEN+1)
//--------------------------------------------------------------------------------------------------
/**
 * Macro to verify if an interface is an output interface.
 */
//--------------------------------------------------------------------------------------------------
#define IS_OUTPUT_IF(interface)     (   (interface == PA_AUDIO_IF_CODEC_SPEAKER) || \
                                        (interface == PA_AUDIO_IF_DSP_FRONTEND_USB_TX) || \
                                        (interface == PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX) || \
                                        (interface == PA_AUDIO_IF_DSP_FRONTEND_PCM_TX) || \
                                        (interface == PA_AUDIO_IF_DSP_FRONTEND_I2S_TX) \
                                    )


//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * Stream Reference Node structure (information related to DTMF detector handlers).
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    void*                 streamRef;
    le_event_HandlerRef_t handlerRef;
    le_dls_Link_t         link;
} StreamRefNode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Audio stream structure.
 * Objects of this type are used to define an audio stream.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_audio_Stream {
    bool             isInput;                       ///< Flag to specify if the stream is an input
                                                    ///  or output.
    pa_audio_If_t    audioInterface;                ///< Audio interface identifier.
    char             format[FORMAT_NAME_MAX_BYTES]; ///< The name of the audio encoding as used by
                                                    ///  the Real-Time Protocol (RTP), specified by
                                                    ///  the IANA organisation.
    uint32_t         gain;                          ///< Gain
    int32_t          fd;                            ///< Audio file descriptor for playback or recording
    le_hashmap_Ref_t connectorList;                 ///< list of connectors to which the audio
                                                    ///  stream is tied to.
    le_dls_List_t    streamRefWithDtmfHdlrList;     ///< List containing information related to
                                                    ///  DTMF detector handlers
    le_event_Id_t    dtmfDetectionEventId;          ///< Event ID to report detected DTMF
    char             dtmfChar;                      ///< Detected DTMF
} le_audio_Stream_t;

//--------------------------------------------------------------------------------------------------
/**
 * Connector structure.
 * Objects of this type are used to define an audio connector.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_audio_Connector {
    le_hashmap_Ref_t streamInList;            ///< list of input streams tied to this connector.
    le_hashmap_Ref_t streamOutList;           ///< list of output streams tied to this connector.
    bool             captureThreadIsStarted;  ///< Capture thread flag tied to this connector.
    bool             playbackThreadIsStarted; ///< Playback thread flag tied to this connector.
    bool             recThreadIsStarted;      ///< Recorder thread flag tied to this connector.
    bool             injThreadIsStarted;      ///< Injector thread flag tied to this connector.
    le_dls_Link_t    connectorsLink;          ///< link for all connector
} le_audio_Connector_t;

//--------------------------------------------------------------------------------------------------
/**
 * HashMapList structure.
 * Objects of this type are used for le_audio_Stream and le_audio_Connector.
 *
 */
//--------------------------------------------------------------------------------------------------
struct hashMapList {
    le_hashmap_Ref_t    hashMapRef; ///< the real hashMap
    bool                isUsed;     ///< is it used?
    le_dls_Link_t       link;       ///< link for AudioHashMapList
};

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for audio stream objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t AudioStreamPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for audio stream objects
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t AudioStreamRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for audio connector objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t AudioConnectorPool;

//--------------------------------------------------------------------------------------------------
/**
 * List for all connector objects
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t  AllConnectorList=LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * List for all HashMap objects
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t    AudioHashMapList;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for HashMapList objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t AudioHashMapPool;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for stream reference node objects (used by DTMF add/remove handler functions)
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t StreamRefNodePool;

//--------------------------------------------------------------------------------------------------
/**
 * Create Unique Stream for each PA_AUDIO_IF
 *
 */
//--------------------------------------------------------------------------------------------------
static le_audio_Stream_t*   MicStreamPtr=NULL;
static le_audio_Stream_t*   SpeakerStreamPtr=NULL;
static le_audio_Stream_t*   UsbRxStreamPtr=NULL;
static le_audio_Stream_t*   UsbTxStreamPtr=NULL;
static le_audio_Stream_t*   PcmRxStreamPtr=NULL;
static le_audio_Stream_t*   PcmTxStreamPtr=NULL;
static le_audio_Stream_t*   I2sRxStreamPtr=NULL;
static le_audio_Stream_t*   I2sTxStreamPtr=NULL;
static le_audio_Stream_t*   ModemVoiceRxStreamPtr=NULL;
static le_audio_Stream_t*   ModemVoiceTxStreamPtr=NULL;
static le_audio_Stream_t*   FilePbStreamPtr=NULL;
static le_audio_Stream_t*   FileRecStreamPtr=NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for audio connector objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t AudioConnectorRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * SafeRef hashing function.
 *
 * @return  Returns the SafeRef itself as it can used as a hash
 *
 */
//--------------------------------------------------------------------------------------------------
size_t HashAudioRef
(
    const void* safeRefPtr    ///< [IN] Safe Ref to be hashed
)
{
    // Return the key value itself.
    return (size_t) safeRefPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * SafeRef equality function.
 *
 * @return  Returns true if the integers are equal, false otherwise
 *
 */
//--------------------------------------------------------------------------------------------------

bool EqualsAudioRef
(
    const void* firstSafeRef,    ///< [IN] First SafeRef for comparing.
    const void* secondSafeRef    ///< [IN] Second SafeRef for comparing.
)
{
    return firstSafeRef == secondSafeRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function returns the number of channels of the stream's audio format.
 *
 * @return The number of channels of the stream's audio format.
 *
 * @note Only "L16-8K" format is supported.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t NumOfChannels
(
    le_audio_Stream_t const *     streamPtr       ///< [IN] The audio stream.
)
{
    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("streamPtr is NULL !");
        return 0;
    }

    LE_DEBUG("Stream format '%s'",streamPtr->format);
    if(!strcmp(streamPtr->format, "L16-8K"))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function set all dsp path from streamPtr to all stream into the streamListPtr
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t OpenStreamPaths
(
    le_audio_Connector_t*   connectorPtr,   ///< [IN] The connector.
    le_audio_Stream_t*      streamPtr,      ///< [IN] The stream
    le_hashmap_Ref_t        streamListPtr   ///< [IN] The stream list
)
{
    pa_audio_If_t inputInterface;
    pa_audio_If_t outputInterface;

    if (connectorPtr == NULL)
    {
        LE_KILL_CLIENT("connectorPtr is NULL !");
        return LE_FAULT;
    }
    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("streamPtr is NULL !");
        return LE_FAULT;
    }

    le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(streamListPtr);

    while (le_hashmap_NextNode(streamIterator)==LE_OK)
    {
        le_audio_Stream_t const * currentStreamPtr = le_hashmap_GetValue(streamIterator);

        LE_DEBUG("CurrentStream %p",currentStreamPtr);

        if (streamPtr->isInput)
        {
            inputInterface  = streamPtr->audioInterface;
            outputInterface = currentStreamPtr->audioInterface;
        }
        else
        {
            inputInterface  = currentStreamPtr->audioInterface;
            outputInterface = streamPtr->audioInterface;
        }

        // Set Audio Path
        LE_DEBUG("Input [%d] and Output [%d] are tied together.",
                 inputInterface,
                 outputInterface);

        if (pa_audio_SetDspAudioPath(inputInterface, outputInterface) != LE_OK)
        {
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function reset all dsp path from streamPtr to all stream into the streamListPtr
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CloseStreamPaths
(
    le_audio_Connector_t const * connectorPtr,   ///< [IN] The connector.
    le_audio_Stream_t const *    streamPtr,      ///< [IN] The stream
    le_hashmap_Ref_t             streamListPtr   ///< [IN] The stream list
)
{
    pa_audio_If_t inputInterface;
    pa_audio_If_t outputInterface;

    if (connectorPtr == NULL)
    {
        LE_KILL_CLIENT("connectorPtr is NULL !");
        return LE_FAULT;
    }
    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("streamPtr is NULL !");
        return LE_FAULT;
    }

    LE_DEBUG("CloseStreamPaths on connector.%p with stream.%p", connectorPtr, streamPtr);

    le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(streamListPtr);

    while (le_hashmap_NextNode(streamIterator)==LE_OK)
    {
        le_audio_Stream_t const * currentStreamPtr = le_hashmap_GetValue(streamIterator);

        if (streamPtr->isInput)
        {
            inputInterface  = streamPtr->audioInterface;
            outputInterface = currentStreamPtr->audioInterface;
        }
        else
        {
            inputInterface  = currentStreamPtr->audioInterface;
            outputInterface = streamPtr->audioInterface;
        }

        // Reset Audio Path
        LE_DEBUG("Flag for reset the DSP audio path (inputInterface.%d with outputInterface.%d)",
                 inputInterface,
                 outputInterface);
        if (pa_audio_FlagForResetDspAudioPath(inputInterface, outputInterface) != LE_OK)
        {
            return LE_FAULT;
        }
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function reset all dsp path, for all connector attached, for the streamPtr.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DeleteAllConnectorPathsFromStream
(
    le_audio_Stream_t*     streamPtr       ///< [IN] The audio stream.
)
{
    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("streamPtr is NULL !");
        return;
    }

    LE_DEBUG("DeleteAllConnectorPathsFromStream streamPtr.%p", streamPtr);

    le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(streamPtr->connectorList);

    while (le_hashmap_NextNode(streamIterator)==LE_OK)
    {
        le_audio_Connector_t const * currentConnectorPtr = le_hashmap_GetValue(streamIterator);

        // Check stream In
        if (streamPtr->isInput)
        {
            // close all connection from this input with all output
            CloseStreamPaths(currentConnectorPtr, streamPtr, currentConnectorPtr->streamOutList);
        }
        // check stream Out
        else
        {
            // close all connection from this output with all input
            CloseStreamPaths(currentConnectorPtr, streamPtr, currentConnectorPtr->streamInList);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function reset all dsp path for a connectorPtr
 *
 */
//--------------------------------------------------------------------------------------------------
static void CloseAllConnectorPaths
(
    le_audio_Connector_t*   connectorPtr   ///< [IN] The connector.
)
{
    if (connectorPtr == NULL)
    {
        LE_KILL_CLIENT("connectorPtr is NULL !");
        return;
    }

    LE_DEBUG("CloseAllConnectorPaths connectorPtr.%p", connectorPtr);

    le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(connectorPtr->streamInList);

    while (le_hashmap_NextNode(streamIterator)==LE_OK)
    {
        le_audio_Stream_t const * currentStreamPtr = le_hashmap_GetValue(streamIterator);

        CloseStreamPaths(connectorPtr,currentStreamPtr,connectorPtr->streamOutList);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function clean all hashmap
 *
 */
//--------------------------------------------------------------------------------------------------
static void ClearHashMap
(
    le_audio_Connector_t* connectorPtr
)
{
    le_hashmap_It_Ref_t streamIterator;

    if (connectorPtr == NULL)
    {
        LE_KILL_CLIENT("connectorPtr is NULL !");
        return;
    }

    streamIterator = le_hashmap_GetIterator(connectorPtr->streamInList);
    while (le_hashmap_NextNode(streamIterator)==LE_OK)
    {
        le_audio_Stream_t const * currentStreamPtr = le_hashmap_GetValue(streamIterator);

        le_hashmap_Remove(currentStreamPtr->connectorList,connectorPtr);
    }

    streamIterator = le_hashmap_GetIterator(connectorPtr->streamOutList);
    while (le_hashmap_NextNode(streamIterator)==LE_OK)
    {
        le_audio_Stream_t const * currentStreamPtr = le_hashmap_GetValue(streamIterator);

        le_hashmap_Remove(currentStreamPtr->connectorList,connectorPtr);
    }

    le_hashmap_RemoveAll(connectorPtr->streamInList);
    le_hashmap_RemoveAll(connectorPtr->streamOutList);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function starts the threads tied to an input connector
 *
 * @return LE_OK    The threads have started or are already started
 * @return LE_FAULT The thread failed to start
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartThreadsOnInputConnector
(
    le_audio_Connector_t*   connectorPtr   ///< [IN] The connector.
)
{
    int ch;

    if (connectorPtr == NULL)
    {
        LE_KILL_CLIENT("connectorPtr is NULL !");
        return LE_FAULT;
    }

    le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(connectorPtr->streamInList);

    while (le_hashmap_NextNode(streamIterator)==LE_OK)
    {
        le_audio_Stream_t const * currentStreamPtr = le_hashmap_GetValue(streamIterator);

        if (( currentStreamPtr->audioInterface == PA_AUDIO_IF_CODEC_MIC )           ||
            ( currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_USB_RX ) ||
            ( currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_PCM_RX ) ||
            ( currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_I2S_RX ))
        {
            // This is an INPUT connector
            if (!connectorPtr->captureThreadIsStarted)
            {
                if ((ch = NumOfChannels(currentStreamPtr))!=0)
                {
                    if (pa_audio_StartCapture(currentStreamPtr->format, ch) == LE_OK )
                    {
                        connectorPtr->captureThreadIsStarted = true;
                    }
                    else
                    {
                        return LE_FAULT;
                    }
                }
                else
                {
                    // Use default format
                    if (pa_audio_StartCapture("L16-8K",1) == LE_OK)
                    {
                        connectorPtr->captureThreadIsStarted = true;
                    }
                    else
                    {
                        return LE_FAULT;
                    }
                }
                return LE_OK;
            }
            else
            {
                LE_INFO("Capture thread is already running");
            }
        }
        else if ( currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_LOC_FILE_REC )
        {
            if ( pa_audio_StartRecorder(currentStreamPtr->audioInterface,
                                        currentStreamPtr->fd) == LE_OK )
            {
                connectorPtr->recThreadIsStarted = true;
            }
            else
            {
                return LE_FAULT;
            }
        }
        else if (currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_REM_FILE_PLAY)
        {
            if ( pa_audio_StartInjector(currentStreamPtr->audioInterface,
                                        currentStreamPtr->fd) == LE_OK )
            {
                connectorPtr->injThreadIsStarted = true;
            }
            else
            {
                return LE_FAULT;
            }
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function stops the threads tied to an input connector
 *
 * @return LE_OK    The thread is stopped
 * @return LE_BUSY  The thread failed to stop
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StopThreadsOnInputConnector
(
    void
)
{
    // Try to find, for all connector, one stream that need the capture thread
    le_dls_Link_t* linkConnectorPtr = le_dls_Peek(&AllConnectorList);

    while (linkConnectorPtr!=NULL)
    {
        le_audio_Connector_t* currentConnectorPtr = CONTAINER_OF(linkConnectorPtr,
                                                                 le_audio_Connector_t,
                                                                 connectorsLink);

        LE_DEBUG("Check connector input %p",currentConnectorPtr);

        le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(currentConnectorPtr->streamInList);

        while (le_hashmap_NextNode(streamIterator)==LE_OK)
        {
            le_audio_Stream_t const * currentStreamPtr = le_hashmap_GetValue(streamIterator);

            if (( currentStreamPtr->audioInterface == PA_AUDIO_IF_CODEC_MIC )           ||
                ( currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_USB_RX ) ||
                ( currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_PCM_RX ) ||
                ( currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_I2S_RX ) )
            {
                if (currentConnectorPtr->captureThreadIsStarted)
                {
                    LE_DEBUG("Found currentStreamPtr->audioInterface.%d, return BUSY",
                             currentStreamPtr->audioInterface);
                    return LE_BUSY;
                }
            }
        }
        linkConnectorPtr = le_dls_PeekNext(&AllConnectorList,linkConnectorPtr);
    }

    // No input interfaces have been found, I must reset captureThreadIsStarted in all my input
    // connectors
    linkConnectorPtr = le_dls_Peek(&AllConnectorList);
    while (linkConnectorPtr!=NULL)
    {
        le_audio_Connector_t* currentConnectorPtr = CONTAINER_OF(linkConnectorPtr,
                                                                 le_audio_Connector_t,
                                                                 connectorsLink);

        currentConnectorPtr->captureThreadIsStarted = false;
        currentConnectorPtr->recThreadIsStarted = false;
        currentConnectorPtr->injThreadIsStarted = false;
        linkConnectorPtr = le_dls_PeekNext(&AllConnectorList,linkConnectorPtr);
    }

    pa_audio_StopRecorder();
    pa_audio_StopInjector();
    pa_audio_StopCapture();

    pa_audio_ResetDspAudioPath();
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function starts the threads tied to an output connector
 *
 * @return LE_OK    The threads have started or are already started
 * @return LE_FAULT The thread failed to start
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartThreadsOnOutputConnector
(
    le_audio_Connector_t*   connectorPtr   ///< [IN] The connector.
)
{
    int ch;

    if (connectorPtr == NULL)
    {
        LE_KILL_CLIENT("connectorPtr is NULL !");
        return LE_FAULT;
    }

    le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(connectorPtr->streamOutList);

    while (le_hashmap_NextNode(streamIterator)==LE_OK)
    {
        le_audio_Stream_t const * currentStreamPtr = le_hashmap_GetValue(streamIterator);

        if (( currentStreamPtr->audioInterface == PA_AUDIO_IF_CODEC_SPEAKER )       ||
            ( currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_USB_TX ) ||
            ( currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_PCM_TX ) ||
            ( currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_I2S_TX ) )
        {
            // This is an OUTPUT connector
            if (!connectorPtr->playbackThreadIsStarted)
            {
                if ((ch = NumOfChannels(currentStreamPtr))!=0)
                {
                    if (pa_audio_StartPlayback(currentStreamPtr->format, ch) == LE_OK)
                    {
                        connectorPtr->playbackThreadIsStarted = true;
                    }
                    else
                    {
                        return LE_FAULT;
                    }
                }
                else
                {
                    // Use default format
                    if (pa_audio_StartPlayback("L16-8K",1) == LE_OK)
                    {
                        connectorPtr->playbackThreadIsStarted = true;
                    }
                    else
                    {
                        return LE_FAULT;
                    }
                }
                return LE_OK;
            }
            else
            {
                LE_INFO("Playback thread is already running");
            }
        }
        else if (currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_LOC_FILE_PLAY)
        {
            if ( pa_audio_StartInjector(currentStreamPtr->audioInterface,
                                        currentStreamPtr->fd) == LE_OK )
            {
                connectorPtr->injThreadIsStarted = true;
            }
            else
            {
                return LE_FAULT;
            }
        }
        else if (currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_REM_FILE_REC )
        {
            if ( pa_audio_StartRecorder(currentStreamPtr->audioInterface,
                                        currentStreamPtr->fd) == LE_OK )
            {
                connectorPtr->recThreadIsStarted = true;
            }
            else
            {
                return LE_FAULT;
            }
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function stops the threads tied to an output connector
 *
 * @return LE_OK    The thread is stopped
 * @return LE_BUSY  The thread failed to stop
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StopThreadsOnOutputConnector
(
    void
)
{
    // Try to find, for all connector, one stream that need the playback thread
    le_dls_Link_t* linkConnectorPtr = le_dls_Peek(&AllConnectorList);
    while (linkConnectorPtr!=NULL)
    {
        le_audio_Connector_t* currentConnectorPtr = CONTAINER_OF(linkConnectorPtr,
                                                                 le_audio_Connector_t,
                                                                 connectorsLink);

        le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(currentConnectorPtr->streamOutList);

        while (le_hashmap_NextNode(streamIterator)==LE_OK)
        {
            le_audio_Stream_t const * currentStreamPtr = le_hashmap_GetValue(streamIterator);

            if (( currentStreamPtr->audioInterface == PA_AUDIO_IF_CODEC_SPEAKER )       ||
                ( currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_USB_TX ) ||
                ( currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_PCM_TX ) ||
                ( currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_I2S_TX ))
            {
                if (currentConnectorPtr->playbackThreadIsStarted)
                {
                    return LE_BUSY;
                }
            }
        }
        linkConnectorPtr = le_dls_PeekNext(&AllConnectorList,linkConnectorPtr);
    }

    // No output interfaces have been found, I must reset playbackThreadIsStarted in all my output
    // connectors
    linkConnectorPtr = le_dls_Peek(&AllConnectorList);
    while (linkConnectorPtr!=NULL)
    {
        le_audio_Connector_t* currentConnectorPtr = CONTAINER_OF(linkConnectorPtr,
                                                                 le_audio_Connector_t,
                                                                 connectorsLink);

        currentConnectorPtr->playbackThreadIsStarted = false;
        currentConnectorPtr->recThreadIsStarted = false;
        currentConnectorPtr->injThreadIsStarted = false;
        linkConnectorPtr = le_dls_PeekNext(&AllConnectorList,linkConnectorPtr);
    }

    pa_audio_StopRecorder();
    pa_audio_StopInjector();
    pa_audio_StopPlayback();

    pa_audio_ResetDspAudioPath();
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function find or create a hashmap into the AudioHashMapList
 *
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t GetHashMapElement
(
    void
)
{
    struct hashMapList* currentPtr=NULL;;
    le_dls_Link_t* linkPtr = le_dls_Peek(&AudioHashMapList);

    while (linkPtr!=NULL)
    {
        currentPtr = CONTAINER_OF(linkPtr,
                                  struct hashMapList,
                                  link);

        if (!currentPtr->isUsed)
        {
            LE_DEBUG("Found one HashMap unused (%p)", currentPtr->hashMapRef);
            currentPtr->isUsed = true;
            return currentPtr->hashMapRef;
        }
        linkPtr = le_dls_PeekNext(&AudioHashMapList,linkPtr);
    }


    currentPtr = le_mem_ForceAlloc(AudioHashMapPool);

    currentPtr->hashMapRef = le_hashmap_Create("ConnectorMap",
                                               AUDIO_HASHMAP_SIZE,
                                               HashAudioRef,
                                               EqualsAudioRef);
    currentPtr->isUsed = true;
    currentPtr->link = LE_DLS_LINK_INIT;

    le_dls_Queue(&AudioHashMapList,&(currentPtr->link));

    LE_DEBUG("Create a new HashMap (%p)", currentPtr->hashMapRef);

    return currentPtr->hashMapRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function release a hashmap into the AudioHashMapList
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReleaseHashMapElement
(
    le_hashmap_Ref_t hashMapRef
)
{
    LE_ASSERT(hashMapRef);

    le_dls_Link_t* linkPtr = le_dls_Peek(&AudioHashMapList);

    while (linkPtr!=NULL)
    {
        struct hashMapList* currentPtr = CONTAINER_OF(linkPtr,
                                                      struct hashMapList,
                                                      link);

        if (currentPtr->hashMapRef == hashMapRef)
        {
            LE_DEBUG("Found HashMap to release (%p)", currentPtr->hashMapRef);
            currentPtr->isUsed = false;
            return;
        }
        linkPtr = le_dls_PeekNext(&AudioHashMapList,linkPtr);
    }

    LE_DEBUG("could not found HashMap to release");
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function returns true if the connector is an INPUT connector
 *
 */
//--------------------------------------------------------------------------------------------------
static bool IsInputConnector
(
    le_audio_Connector_t*   connectorPtr   ///< [IN] The connector.
)
{
    if (connectorPtr == NULL)
    {
        LE_KILL_CLIENT("connectorPtr is NULL !");
        return false;
    }

    le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(connectorPtr->streamInList);

    while (le_hashmap_NextNode(streamIterator)==LE_OK)
    {
        le_audio_Stream_t const * currentStreamPtr = le_hashmap_GetValue(streamIterator);

        if (( currentStreamPtr->audioInterface == PA_AUDIO_IF_CODEC_MIC )           ||
            ( currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_USB_RX ) ||
            ( currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_PCM_RX ) ||
            ( currentStreamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_I2S_RX ))
        {
            // This is an INPUT connector
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
 * This function initialize a stream
 *
 */
//--------------------------------------------------------------------------------------------------
static void InitializeStream
(
    le_audio_Stream_t* streamPtr
)
{
    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("streamPtr is NULL !");
        return;
    }

    streamPtr->audioInterface = 0;
    streamPtr->isInput = false;
    streamPtr->gain=0;
    memset(streamPtr->format, 0, sizeof(streamPtr->format));
    streamPtr->connectorList = GetHashMapElement();
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called when a stream is deleted
 *
 */
//--------------------------------------------------------------------------------------------------
static void DestructStream
(
    void *objPtr
)
{
    LE_ASSERT(objPtr);

    le_audio_Stream_t* streamPtr = objPtr;

    DeleteAllConnectorPathsFromStream (streamPtr);


    le_hashmap_RemoveAll(streamPtr->connectorList);
    ReleaseHashMapElement(streamPtr->connectorList);

    StopThreadsOnInputConnector ();
    StopThreadsOnOutputConnector ();

    switch (streamPtr->audioInterface)
    {
        case PA_AUDIO_IF_CODEC_MIC :
            MicStreamPtr = NULL;
            break;
        case PA_AUDIO_IF_CODEC_SPEAKER:
            SpeakerStreamPtr = NULL;
            break;
        case PA_AUDIO_IF_DSP_FRONTEND_USB_RX:
            UsbRxStreamPtr = NULL;
            break;
        case PA_AUDIO_IF_DSP_FRONTEND_USB_TX:
            UsbTxStreamPtr = NULL;
            break;
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX:
            ModemVoiceRxStreamPtr = NULL;
            break;
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX:
            ModemVoiceTxStreamPtr = NULL;
            break;
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_RX:
            PcmRxStreamPtr = NULL;
            break;
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_TX:
            PcmTxStreamPtr = NULL;
            break;
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_RX:
            I2sRxStreamPtr = NULL;
            break;
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_TX:
            I2sTxStreamPtr = NULL;
            break;
        case PA_AUDIO_IF_DSP_FRONTEND_FILE_PLAY:
        case PA_AUDIO_IF_DSP_FRONTEND_LOC_FILE_PLAY:
        case PA_AUDIO_IF_DSP_FRONTEND_REM_FILE_PLAY:
            FilePbStreamPtr = NULL;
            break;
        case PA_AUDIO_IF_DSP_FRONTEND_FILE_REC :
        case PA_AUDIO_IF_DSP_FRONTEND_LOC_FILE_REC:
        case PA_AUDIO_IF_DSP_FRONTEND_REM_FILE_REC:
            FileRecStreamPtr = NULL;
            break;
        case PA_AUDIO_NUM_INTERFACES:
        case PA_AUDIO_IF_DSP_BACKEND_DTMF_RX:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called when a connector is deleted
 *
 */
//--------------------------------------------------------------------------------------------------
static void DestructConnector
(
    void *objPtr
)
{
    LE_ASSERT(objPtr);

    le_audio_Connector_t* connectorPtr = objPtr;

    le_hashmap_RemoveAll(connectorPtr->streamInList);
    le_hashmap_RemoveAll(connectorPtr->streamInList);

    ReleaseHashMapElement(connectorPtr->streamInList);
    ReleaseHashMapElement(connectorPtr->streamOutList);
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer DTMF Event Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerDtmfDetectorHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_audio_StreamRef_t*              streamRef = reportPtr;
    le_audio_DtmfDetectorHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    le_audio_Stream_t*  streamPtr = le_ref_Lookup(AudioStreamRefMap, *streamRef);
    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid stream reference (%p) provided!", *streamRef);
        return;
    }

    LE_DEBUG("[%c] DTMF detected!", streamPtr->dtmfChar);

    clientHandlerFunc(*streamRef, streamPtr->dtmfChar, le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * Internal Dtmf detector Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DtmfDetectorHandler
(
    char  dtmf
)
{
    void*            streamRefTmp = NULL;
    StreamRefNode_t* nodePtr;
    le_dls_Link_t*   linkPtr;

    LE_DEBUG("[%c] DTMF detected!", dtmf);

    // Report DTMF to all registered handlers
    linkPtr = le_dls_Peek(&(ModemVoiceRxStreamPtr->streamRefWithDtmfHdlrList));
    while (linkPtr != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, StreamRefNode_t, link);
        LE_DEBUG("Found streamRef.%p", nodePtr->streamRef);
        if(nodePtr->streamRef != streamRefTmp)
        {
            // Notify only once on one streamRef
            ModemVoiceRxStreamPtr->dtmfChar=dtmf;

            le_event_Report(ModemVoiceRxStreamPtr->dtmfDetectionEventId,
                            &nodePtr->streamRef,
                            sizeof(nodePtr->streamRef));
            streamRefTmp = nodePtr->streamRef;
        }
        linkPtr = le_dls_PeekNext(&ModemVoiceRxStreamPtr->streamRefWithDtmfHdlrList, linkPtr);
    }
}

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the audio component.
 *
 * @note The process exits on failure
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Allocate the audio stream pool.
    AudioStreamPool = le_mem_CreatePool("AudioStreamPool", sizeof(le_audio_Stream_t));
    le_mem_ExpandPool(AudioStreamPool, STREAM_DEFAULT_POOL_SIZE);
    le_mem_SetDestructor(AudioStreamPool,DestructStream);

    // Create the Safe Reference Map to use for Audio object Safe References.
    AudioStreamRefMap = le_ref_CreateMap("AudioStreamMap", MAX_NUM_OF_STREAM);

    // Allocate the audio connector pool.
    AudioConnectorPool = le_mem_CreatePool("AudioConnectorPool", sizeof(le_audio_Connector_t));
    le_mem_ExpandPool(AudioConnectorPool, CONNECTOR_DEFAULT_POOL_SIZE);
    le_mem_SetDestructor(AudioConnectorPool,DestructConnector);

    AudioHashMapPool = le_mem_CreatePool("AudiohashMapPool", sizeof(struct hashMapList));
    le_mem_ExpandPool(AudioHashMapPool, HASHMAP_DEFAULT_POOL_SIZE);

    // init HashMapList
    AudioHashMapList = LE_DLS_LIST_INIT;

    // Create the Safe Reference Map to use for Connector object Safe References.
    AudioConnectorRefMap = le_ref_CreateMap("AudioConMap", MAX_NUM_OF_CONNECTOR);
    // Allocate the stream reference nodes pool (used by DTMF add/remove handler functions).
    StreamRefNodePool = le_mem_CreatePool("StreamRefNodePool", sizeof(StreamRefNode_t));

    // Register a handler function for Call Event indications
    LE_FATAL_IF((pa_audio_SetDtmfDetectorHandler(DtmfDetectorHandler) != LE_OK),
                "Add pa_audio_SetDtmfDetectorHandler failed");
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the Microphone.
 *
 * @return Reference to the input audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenMic
(
    void
)
{
    if (!MicStreamPtr)
    {
        MicStreamPtr = le_mem_ForceAlloc(AudioStreamPool);

        InitializeStream(MicStreamPtr);

        MicStreamPtr->audioInterface = PA_AUDIO_IF_CODEC_MIC;
        MicStreamPtr->isInput = true;

    }
    else
    {
        le_mem_AddRef(MicStreamPtr);
    }

    LE_DEBUG("Open Microphone input audio stream (%p)", MicStreamPtr);
    // Create and return a Safe Reference for this stream object.
    return le_ref_CreateRef(AudioStreamRefMap, MicStreamPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the Speakerphone.
 *
 * @return Reference to the output audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenSpeaker
(
    void
)
{
    if (!SpeakerStreamPtr)
    {
        SpeakerStreamPtr = le_mem_ForceAlloc(AudioStreamPool);

        InitializeStream(SpeakerStreamPtr);

        SpeakerStreamPtr->audioInterface = PA_AUDIO_IF_CODEC_SPEAKER;
        SpeakerStreamPtr->isInput = false;

    }
    else
    {
        le_mem_AddRef(SpeakerStreamPtr);
    }

    LE_DEBUG("Open Speaker output audio stream (%p)", SpeakerStreamPtr);
    // Create and return a Safe Reference for this stream object.
    return le_ref_CreateRef(AudioStreamRefMap, SpeakerStreamPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the received audio stream of an USB audio class.
 *
 * @return Reference to the input audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenUsbRx
(
    void
)
{
    if (!UsbRxStreamPtr)
    {
        UsbRxStreamPtr = le_mem_ForceAlloc(AudioStreamPool);

        InitializeStream(UsbRxStreamPtr);

        UsbRxStreamPtr->audioInterface = PA_AUDIO_IF_DSP_FRONTEND_USB_RX;
        UsbRxStreamPtr->isInput = true;
    }
    else
    {
        le_mem_AddRef(UsbRxStreamPtr);
    }

    LE_DEBUG("Open USB RX input audio stream (%p)", UsbRxStreamPtr);
    // Create and return a Safe Reference for this stream object.
    return le_ref_CreateRef(AudioStreamRefMap, UsbRxStreamPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the transmitted audio stream of an USB audio class.
 *
 * @return Reference to the output audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenUsbTx
(
    void
)
{
    if (!UsbTxStreamPtr)
    {
        UsbTxStreamPtr = le_mem_ForceAlloc(AudioStreamPool);

        InitializeStream(UsbTxStreamPtr);

        UsbTxStreamPtr->audioInterface = PA_AUDIO_IF_DSP_FRONTEND_USB_TX;
        UsbTxStreamPtr->isInput = false;
    }
    else
    {
        le_mem_AddRef(UsbTxStreamPtr);
    }

    LE_DEBUG("Open USB TX output audio stream (%p)", UsbTxStreamPtr);
    // Create and return a Safe Reference for this stream object.
    return le_ref_CreateRef(AudioStreamRefMap, UsbTxStreamPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Open the received audio stream of the PCM interface.
 *
 * @return Reference to the input audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenPcmRx
(
    uint32_t timeslot  ///< [IN] The time slot number.
)
{
    if (!PcmRxStreamPtr)
    {
        PcmRxStreamPtr = le_mem_ForceAlloc(AudioStreamPool);

        InitializeStream(PcmRxStreamPtr);

        PcmRxStreamPtr->audioInterface = PA_AUDIO_IF_DSP_FRONTEND_PCM_RX;
        PcmRxStreamPtr->isInput = true;

        if ( pa_audio_SetPcmTimeSlot(PA_AUDIO_IF_DSP_FRONTEND_PCM_RX, timeslot) != LE_OK )
        {
            LE_WARN("Cannot set timeslot of Secondary PCM RX interface");
            le_mem_Release(PcmRxStreamPtr);
            PcmRxStreamPtr = NULL;
            return NULL;
        }
        // Default mode must be Master
        if ( pa_audio_SetMasterMode(PA_AUDIO_IF_DSP_FRONTEND_PCM_RX) != LE_OK )
        {
            LE_WARN("Cannot open Secondary PCM RX input as Master");
            le_mem_Release(PcmRxStreamPtr);
            PcmRxStreamPtr = NULL;
            return NULL;
        }
    }
    else
    {
        le_mem_AddRef(PcmRxStreamPtr);
    }

    LE_DEBUG("Open Secondary PCM RX input audio stream (%p)", PcmRxStreamPtr);
    // Create and return a Safe Reference for this stream object.
    return le_ref_CreateRef(AudioStreamRefMap, PcmRxStreamPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the transmitted audio stream of the PCM interface.
 *
 * @return Reference to the output audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenPcmTx
(
    uint32_t timeslot  ///< [IN] The time slot number.
)
{
    if (!PcmTxStreamPtr)
    {
        PcmTxStreamPtr = le_mem_ForceAlloc(AudioStreamPool);

        InitializeStream(PcmTxStreamPtr);

        PcmTxStreamPtr->audioInterface = PA_AUDIO_IF_DSP_FRONTEND_PCM_TX;
        PcmTxStreamPtr->isInput = false;

        if ( pa_audio_SetPcmTimeSlot(PA_AUDIO_IF_DSP_FRONTEND_PCM_TX, timeslot) != LE_OK )
        {
            LE_WARN("Cannot set timeslot of Secondary PCM TX interface");
            le_mem_Release(PcmRxStreamPtr);
            PcmRxStreamPtr = NULL;
            return NULL;
        }
        // Default mode must be Master
        if ( pa_audio_SetMasterMode(PA_AUDIO_IF_DSP_FRONTEND_PCM_TX) != LE_OK )
        {
            LE_WARN("Cannot open Secondary PCM TX input as Master");
            le_mem_Release(PcmTxStreamPtr);
            PcmTxStreamPtr = NULL;
            return NULL;
        }
    }
    else
    {
        le_mem_AddRef(PcmTxStreamPtr);
    }

    LE_DEBUG("Open Secondary PCM TX output audio stream (%p)", PcmTxStreamPtr);
    // Create and return a Safe Reference for this stream object.
    return le_ref_CreateRef(AudioStreamRefMap, PcmTxStreamPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the received audio stream of the I2S interface.
 *
 * @return Reference to the input audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenI2sRx
(
    le_audio_I2SChannel_t mode  ///< [IN] The channel mode.
)
{
    if (!I2sRxStreamPtr)
    {
        I2sRxStreamPtr = le_mem_ForceAlloc(AudioStreamPool);

        InitializeStream(I2sRxStreamPtr);

        I2sRxStreamPtr->audioInterface = PA_AUDIO_IF_DSP_FRONTEND_I2S_RX;
        I2sRxStreamPtr->isInput = true;

        if ( pa_audio_SetI2sChannelMode(PA_AUDIO_IF_DSP_FRONTEND_I2S_RX, mode) != LE_OK )
        {
            LE_WARN("Cannot set the channel mode of I2S RX interface");
            le_mem_Release(I2sRxStreamPtr);
            I2sRxStreamPtr = NULL;
            return NULL;
        }
    }
    else
    {
        le_mem_AddRef(I2sRxStreamPtr);
    }

    LE_DEBUG("Open I2S RX input audio stream (%p)", I2sRxStreamPtr);
    // Create and return a Safe Reference for this stream object.
    return le_ref_CreateRef(AudioStreamRefMap, I2sRxStreamPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the transmitted audio stream of the I2S interface.
 *
 * @return Reference to the output audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenI2sTx
(
    le_audio_I2SChannel_t mode  ///< [IN] The channel mode.
)
{
    if (!I2sTxStreamPtr)
    {
        I2sTxStreamPtr = le_mem_ForceAlloc(AudioStreamPool);

        InitializeStream(I2sTxStreamPtr);

        I2sTxStreamPtr->audioInterface = PA_AUDIO_IF_DSP_FRONTEND_I2S_TX;
        I2sTxStreamPtr->isInput = false;

        if ( pa_audio_SetI2sChannelMode(PA_AUDIO_IF_DSP_FRONTEND_I2S_TX, mode) != LE_OK )
        {
            LE_WARN("Cannot set the channel mode of I2S TX interface");
            le_mem_Release(PcmRxStreamPtr);
            PcmRxStreamPtr = NULL;
            return NULL;
        }
    }
    else
    {
        le_mem_AddRef(I2sTxStreamPtr);
    }

    LE_DEBUG("Open I2S TX output audio stream (%p)", I2sTxStreamPtr);
    // Create and return a Safe Reference for this stream object.
    return le_ref_CreateRef(AudioStreamRefMap, I2sTxStreamPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the audio stream for file playback.
 *
 * @return Reference to the audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenFilePlayback
(
    int32_t     fd  ///< [IN] The audio file descriptor.
)
{
    if (!pa_audio_IsDspAudioPathsUsed())
    {
        LE_WARN("There is no audio path, cannot open this interface (%d)!",
                PA_AUDIO_IF_DSP_FRONTEND_FILE_PLAY);
        return NULL;
    }

    if (!FilePbStreamPtr)
    {
        FilePbStreamPtr = le_mem_ForceAlloc(AudioStreamPool);

        InitializeStream(FilePbStreamPtr);

        FilePbStreamPtr->audioInterface = PA_AUDIO_IF_DSP_FRONTEND_FILE_PLAY;
        FilePbStreamPtr->fd = fd;

    }
    else
    {
        le_mem_AddRef(FilePbStreamPtr);
    }

    LE_DEBUG("Open the audio stream for local file playback. (%p)", FilePbStreamPtr);
    // Create and return a Safe Reference for this stream object.
    return le_ref_CreateRef(AudioStreamRefMap, FilePbStreamPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the audio stream for file recording.
 *
 * @return Reference to the audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenFileRecording
(
    int32_t     fd  ///< [IN] The audio file descriptor.
)
{
    if (!pa_audio_IsDspAudioPathsUsed())
    {
        LE_WARN("There is no audio path, cannot open this interface (%d)!",
                PA_AUDIO_IF_DSP_FRONTEND_FILE_REC );
        return NULL;
    }

    if (!FileRecStreamPtr)
    {
        FileRecStreamPtr = le_mem_ForceAlloc(AudioStreamPool);

        InitializeStream(FileRecStreamPtr);

        FileRecStreamPtr->audioInterface = PA_AUDIO_IF_DSP_FRONTEND_FILE_REC ;
        FileRecStreamPtr->fd = fd;
    }
    else
    {
        le_mem_AddRef(FileRecStreamPtr);
    }

    LE_DEBUG("Open the audio stream for local file recording (%p), fd.%d", FileRecStreamPtr, fd);
    // Create and return a Safe Reference for this stream object.
    return le_ref_CreateRef(AudioStreamRefMap, FileRecStreamPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the received audio stream of a voice call.
 *
 * @return Reference to the input audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenModemVoiceRx
(
    void
)
{
    if (!ModemVoiceRxStreamPtr)
    {
        ModemVoiceRxStreamPtr = le_mem_ForceAlloc(AudioStreamPool);

        InitializeStream(ModemVoiceRxStreamPtr);

        ModemVoiceRxStreamPtr->audioInterface = PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX;
        ModemVoiceRxStreamPtr->isInput = true;
        ModemVoiceRxStreamPtr->streamRefWithDtmfHdlrList=LE_DLS_LIST_INIT;
        ModemVoiceRxStreamPtr->dtmfDetectionEventId = le_event_CreateId("DtmfMdmVoiceRx",
                                                                        sizeof(le_audio_StreamRef_t));
    }
    else
    {
        le_mem_AddRef(ModemVoiceRxStreamPtr);
    }

    LE_DEBUG("Open Modem Voice RX input audio stream (%p)", ModemVoiceRxStreamPtr);
    // Create and return a Safe Reference for this stream object.
    return le_ref_CreateRef(AudioStreamRefMap, ModemVoiceRxStreamPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the transmitted audio stream of a voice call.
 *
 * @return Reference to the output audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenModemVoiceTx
(
    void
)
{
    if (!ModemVoiceTxStreamPtr)
    {
        ModemVoiceTxStreamPtr = le_mem_ForceAlloc(AudioStreamPool);

        InitializeStream(ModemVoiceTxStreamPtr);

        ModemVoiceTxStreamPtr->audioInterface = PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX;
        ModemVoiceTxStreamPtr->isInput = false;
    }
    else
    {
        le_mem_AddRef(ModemVoiceTxStreamPtr);
    }

    LE_DEBUG("Open Modem Voice TX output audio stream (%p)", ModemVoiceTxStreamPtr);
    // Create and return a Safe Reference for this stream object.
    return le_ref_CreateRef(AudioStreamRefMap, ModemVoiceTxStreamPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the audio format of an input or output stream.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 *
 * @note The process exits, if an invalid audio stream reference is given or if the formatPtr
 *       pointer is NULL.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_GetFormat
(
    le_audio_StreamRef_t streamRef,  ///< [IN] The audio stream reference.
    char*                formatPtr,  ///< [OUT] The name of the audio encoding as used by the
                                     ///  Real-Time Protocol (RTP), specified by the IANA organisation.
    size_t               len         ///< [IN]  The length of format string.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    if (formatPtr == NULL)
    {
        LE_KILL_CLIENT("formatPtr is NULL !");
        return LE_FAULT;
    }

    return (le_utf8_Copy(formatPtr, streamPtr->format, len, NULL));
}

//--------------------------------------------------------------------------------------------------
/**
 * Close an audio stream.
 * If several Users own the stream reference, the interface will be definitively closed only after
 * the last user closes the audio stream.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_audio_Close
(
    le_audio_StreamRef_t streamRef  ///< [IN] The audio stream reference.
)
{
    le_audio_Stream_t*  streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return;
    }

    // Invalidate the Safe Reference.
    le_ref_DeleteRef(AudioStreamRefMap, streamRef);

    le_mem_Release(streamPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Gain value of an input or output stream.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_OUT_OF_RANGE  The gain parameter is not between 0 and 100
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_SetGain
(
    le_audio_StreamRef_t streamRef,  ///< [IN] The audio stream reference.
    uint32_t             gain        ///< [IN] The gain value [0..100] (0 means 'muted', 100 is the
                                     ///       maximum gain value)
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_BAD_PARAMETER;
    }

    if (gain > 100)
    {
        return LE_OUT_OF_RANGE;
    }

    if ( pa_audio_SetGain(streamPtr->audioInterface, gain) != LE_OK )
    {
        LE_ERROR("Cannot set stream gain");
        return LE_FAULT;
    }

    streamPtr->gain = gain;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Gain value of an input or output stream.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_GetGain
(
    le_audio_StreamRef_t streamRef,  ///< [IN] The audio stream reference.
    uint32_t            *gainPtr     ///< [OUT] The gain value [0..100] (0 means 'muted', 100 is the
                                     ///        maximum gain value)
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_BAD_PARAMETER;
    }
    if (gainPtr == NULL)
    {
        LE_KILL_CLIENT("gainPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if ( pa_audio_GetGain(streamPtr->audioInterface, gainPtr) != LE_OK )
    {
        LE_ERROR("Cannot get stream gain");
        return LE_FAULT;
    }

    streamPtr->gain = *gainPtr;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Mute an audio stream.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_Mute
(
    le_audio_StreamRef_t streamRef  ///< [IN] The audio stream reference.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_BAD_PARAMETER;
    }

    if ( pa_audio_GetGain(streamPtr->audioInterface, &streamPtr->gain) != LE_OK )
    {
        LE_ERROR("Cannot get stream gain");
        return LE_FAULT;
    }

    if ( pa_audio_SetGain(streamPtr->audioInterface, 0) != LE_OK )
    {
        LE_ERROR("Cannot set stream gain");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Unmute an audio stream.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_Unmute
(
    le_audio_StreamRef_t streamRef  ///< [IN] The audio stream reference.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_BAD_PARAMETER;
    }

    if ( pa_audio_SetGain(streamPtr->audioInterface, streamPtr->gain) != LE_OK )
    {
        LE_ERROR("Cannot set stream gain");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create an audio connector reference.
 *
 * @return Reference to the audio connector, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_ConnectorRef_t le_audio_CreateConnector
(
    void
)
{
    le_audio_Connector_t* newConnectorPtr = le_mem_ForceAlloc(AudioConnectorPool);

    newConnectorPtr->streamInList   = GetHashMapElement();
    newConnectorPtr->streamOutList  = GetHashMapElement();

    newConnectorPtr->connectorsLink = LE_DLS_LINK_INIT;

    newConnectorPtr->captureThreadIsStarted = false;
    newConnectorPtr->playbackThreadIsStarted = false;

    newConnectorPtr->recThreadIsStarted = false;
    newConnectorPtr->injThreadIsStarted = false;

    // Add the new connector into the the connector list
    le_dls_Queue(&AllConnectorList,&(newConnectorPtr->connectorsLink));

    // Create and return a Safe Reference for this connector object.
    return le_ref_CreateRef(AudioConnectorRefMap, newConnectorPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete an audio connector reference.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_audio_DeleteConnector
(
    le_audio_ConnectorRef_t connectorRef    ///< [IN] The connector reference.
)
{
    le_audio_Connector_t* connectorPtr = le_ref_Lookup(AudioConnectorRefMap, connectorRef);

    if (connectorPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", connectorRef);
        return;
    }

    CloseAllConnectorPaths (connectorPtr);

    ClearHashMap (connectorPtr);

    // Remove the connector from all the connector list
    le_dls_Remove(&AllConnectorList,&(connectorPtr->connectorsLink));

    // Invalidate the Safe Reference.
    le_ref_DeleteRef(AudioConnectorRefMap, connectorRef);

    le_mem_Release(connectorPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect an audio stream to the connector reference.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_BUSY          There are insufficient DSP resources available.
 * @return LE_BAD_PARAMETER The connector and/or the audio stream references are invalid.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_Connect
(
    le_audio_ConnectorRef_t connectorRef,   ///< [IN] The connector reference.
    le_audio_StreamRef_t    streamRef       ///< [IN] The audio stream reference.
)
{
    le_audio_Stream_t*    streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);
    le_audio_Connector_t* connectorPtr = le_ref_Lookup(AudioConnectorRefMap, connectorRef);
    le_hashmap_Ref_t      listPtr=NULL;

    if (connectorPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid connector reference (%p) provided!", connectorRef);
        return LE_BAD_PARAMETER;
    }
    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid stream reference (%p) provided!", streamRef);
        return LE_BAD_PARAMETER;
    }

    // Manage specific Playback and Recording interfaces cases
    if (IsInputConnector(connectorPtr))
    {
        if (streamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_FILE_PLAY)
        {
            streamPtr->isInput = true;
            streamPtr->audioInterface = PA_AUDIO_IF_DSP_FRONTEND_REM_FILE_PLAY;
        }
        if (streamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_FILE_REC)
        {
            streamPtr->isInput = true;
            streamPtr->audioInterface = PA_AUDIO_IF_DSP_FRONTEND_LOC_FILE_REC;
        }
    }
    else
    {
        if (streamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_FILE_PLAY)
        {
            streamPtr->isInput = false;
            streamPtr->audioInterface = PA_AUDIO_IF_DSP_FRONTEND_LOC_FILE_PLAY;
        }
        if (streamPtr->audioInterface == PA_AUDIO_IF_DSP_FRONTEND_FILE_REC)
        {
            streamPtr->isInput = false;
            streamPtr->audioInterface = PA_AUDIO_IF_DSP_FRONTEND_REM_FILE_REC;
        }
    }

    LE_DEBUG("StreamRef.%p (@%p) Connect [%d] '%s' to connectorRef.%p",
             streamRef,
             streamPtr,
             streamPtr->audioInterface,
             (streamPtr->isInput)?"input":"output",
             connectorRef);

    if ( streamPtr->isInput )
    {
        if (le_hashmap_ContainsKey(connectorPtr->streamInList, streamPtr))
        {
            LE_ERROR("This stream is already connected to this connector.");
            return LE_BUSY;
        }
        else
        {
            le_hashmap_Put(connectorPtr->streamInList, streamPtr, streamPtr);
            listPtr = connectorPtr->streamOutList;
        }
    }
    else
    {
        if (le_hashmap_ContainsKey(connectorPtr->streamOutList, streamPtr))
        {
            LE_ERROR("This stream is already connected to this connector.");
            return LE_BUSY;
        }
        else
        {
            le_hashmap_Put(connectorPtr->streamOutList, streamPtr, streamPtr);
            listPtr = connectorPtr->streamInList;
        }
    }

    // Add the connector to the stream
    le_hashmap_Put(streamPtr->connectorList, connectorPtr, connectorPtr);

    // If there are 1 input stream and 1 output stream, then we can create the corresponding thread.
    if (le_hashmap_Size(listPtr) >= 1)
    {
        if (OpenStreamPaths (connectorPtr, streamPtr, listPtr)!=LE_OK) {
            return LE_FAULT;
        }

        if (StartThreadsOnInputConnector (connectorPtr)!=LE_OK)
        {
            LE_DEBUG("Capture thread is not started");
            return LE_BUSY;
        }

        if (StartThreadsOnOutputConnector (connectorPtr)!=LE_OK)
        {
            LE_DEBUG("Playback thread is not started");
            return LE_BUSY;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disconnect an audio stream from the connector reference.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_audio_Disconnect
(
    le_audio_ConnectorRef_t connectorRef,   ///< [IN] The connector reference.
    le_audio_StreamRef_t    streamRef       ///< [IN] The audio stream reference.
)
{
    le_audio_Stream_t*    streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);
    le_audio_Connector_t* connectorPtr = le_ref_Lookup(AudioConnectorRefMap, connectorRef);

    if (connectorPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid connector reference (%p) provided!", connectorRef);
        return;
    }
    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid stream reference (%p) provided!", streamRef);
        return;
    }

    LE_DEBUG("Disconnect stream.%p from connector.%p", streamRef, connectorRef);
    if (streamPtr->isInput)
    {
        CloseStreamPaths (connectorPtr,streamPtr,connectorPtr->streamOutList);
        le_hashmap_Remove(connectorPtr->streamInList,streamPtr);
        le_hashmap_Remove(streamPtr->connectorList,connectorPtr);
        StopThreadsOnInputConnector();
    }
    else
    {
        CloseStreamPaths (connectorPtr,streamPtr,connectorPtr->streamInList);
        le_hashmap_Remove(connectorPtr->streamOutList,streamPtr);
        le_hashmap_Remove(streamPtr->connectorList,connectorPtr);
        StopThreadsOnOutputConnector();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * le_audio_DtmfDetectorHandler handler ADD function
 */
//--------------------------------------------------------------------------------------------------
le_audio_DtmfDetectorHandlerRef_t le_audio_AddDtmfDetectorHandler
(
    le_audio_StreamRef_t               streamRef,    ///< [IN] The audio stream reference.
    le_audio_DtmfDetectorHandlerFunc_t handlerPtr,   ///< [IN] The DTMF detector handler function.
    void*                              contextPtr    ///< [IN] The handler's context.
)
{
    le_event_HandlerRef_t  handlerRef;
    le_audio_Stream_t*     streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid stream reference (%p) provided!", streamRef);
        return NULL;
    }
    if (handlerPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    if (streamPtr->audioInterface != PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX)
    {
        LE_ERROR("Interface not yet supported!");
        return NULL;
    }
    else
    {
        LE_DEBUG("Add DTMF detector handler on interface %d.", streamPtr->audioInterface);

        handlerRef = le_event_AddLayeredHandler("DtmfDecoderHandler",
                                                streamPtr->dtmfDetectionEventId,
                                                FirstLayerDtmfDetectorHandler,
                                                (le_event_HandlerFunc_t)handlerPtr);

        le_event_SetContextPtr(handlerRef, contextPtr);

        // if list is void that means it is the first added handler
        if (le_dls_Peek(&(ModemVoiceRxStreamPtr->streamRefWithDtmfHdlrList)) == NULL)
        {
            pa_audio_StartDtmfDecoder(PA_AUDIO_IF_DSP_BACKEND_DTMF_RX);
        }

        // Associate the handlerRef to the streamRef
        StreamRefNode_t* streamRefNodePtr = le_mem_ForceAlloc(StreamRefNodePool);
        streamRefNodePtr->streamRef = streamRef;
        streamRefNodePtr->handlerRef = handlerRef;
        streamRefNodePtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&(ModemVoiceRxStreamPtr->streamRefWithDtmfHdlrList),&(streamRefNodePtr->link));

        return (le_audio_DtmfDetectorHandlerRef_t)(handlerRef);

    }
}

//--------------------------------------------------------------------------------------------------
/**
 * le_audio_DtmfDetectorHandler handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void le_audio_RemoveDtmfDetectorHandler
(
    le_audio_DtmfDetectorHandlerRef_t addHandlerRef ///< [IN]
)
{
    StreamRefNode_t* nodePtr;
    le_dls_Link_t*   linkPtr;

    // Remove corresponding node from the streamRefWithDtmfHdlrList
    linkPtr = le_dls_Peek(&(ModemVoiceRxStreamPtr->streamRefWithDtmfHdlrList));
    while (linkPtr != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, StreamRefNode_t, link);
        if (nodePtr->handlerRef == (le_event_HandlerRef_t)addHandlerRef)
        {
            le_dls_Remove(&(ModemVoiceRxStreamPtr->streamRefWithDtmfHdlrList), &nodePtr->link);
            le_mem_Release(nodePtr);
            break;
        }
        linkPtr = le_dls_PeekNext(&ModemVoiceRxStreamPtr->streamRefWithDtmfHdlrList, linkPtr);
    }
    // if No more DTMF handlers (list is void), stop DTMF decoder
    if (le_dls_Peek(&(ModemVoiceRxStreamPtr->streamRefWithDtmfHdlrList)) == NULL)
    {
        pa_audio_StopDtmfDecoder(PA_AUDIO_IF_DSP_BACKEND_DTMF_RX);
    }

    // TODO: do the same loop on the other input interfaces

    le_event_RemoveHandler((le_event_HandlerRef_t)addHandlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable the Noise Suppressor.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 * @note The process exits, if an invalid audio stream reference is given.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_EnableNoiseSuppressor
(
    le_audio_StreamRef_t    streamRef       ///< [IN] The audio stream reference.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    return pa_audio_NoiseSuppressorSwitch(streamPtr->audioInterface, LE_ON);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to disable the Noise Suppressor.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 * @note The process exits, if an invalid audio stream reference is given.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_DisableNoiseSuppressor
(
    le_audio_StreamRef_t    streamRef       ///< [IN] The audio stream reference.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    return pa_audio_NoiseSuppressorSwitch(streamPtr->audioInterface, LE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable the Echo Canceller.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 * @note The process exits, if an invalid audio stream reference is given.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_EnableEchoCanceller
(
    le_audio_StreamRef_t    streamRef       ///< [IN] The audio stream reference.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    return pa_audio_EchoCancellerSwitch(streamPtr->audioInterface, LE_ON);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to disable the Echo Canceller.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 * @note The process exits, if an invalid audio stream reference is given.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_DisableEchoCanceller
(
    le_audio_StreamRef_t    streamRef       ///< [IN] The audio stream reference.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    return pa_audio_EchoCancellerSwitch(streamPtr->audioInterface, LE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable the FIR (Finite Impulse Response) filter.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 * @note The process exits, if an invalid audio stream reference is given.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_EnableFirFilter
(
    le_audio_StreamRef_t    streamRef       ///< [IN] The audio stream reference.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    return pa_audio_FirFilterSwitch(streamPtr->audioInterface, LE_ON);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to disable the FIR (Finite Impulse Response) filter.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 * @note The process exits, if an invalid audio stream reference is given.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_DisableFirFilter
(
    le_audio_StreamRef_t    streamRef       ///< [IN] The audio stream reference.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    return pa_audio_FirFilterSwitch(streamPtr->audioInterface, LE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable the FIR (Finite Impulse Response) filter.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 * @note The process exits, if an invalid audio stream reference is given.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_EnableIirFilter
(
    le_audio_StreamRef_t    streamRef       ///< [IN] The audio stream reference.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    return pa_audio_IirFilterSwitch(streamPtr->audioInterface, LE_ON);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to disable the IIR (Infinite Impulse Response) filter.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 * @note The process exits, if an invalid audio stream reference is given.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_DisableIirFilter
(
    le_audio_StreamRef_t    streamRef       ///< [IN] The audio stream reference.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    return pa_audio_IirFilterSwitch(streamPtr->audioInterface, LE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable the automatic gain control on the selected audio stream.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 * @note The process exits, if an invalid audio stream reference is given.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_EnableAutomaticGainControl
(
    le_audio_StreamRef_t    streamRef       ///< [IN] The audio stream reference.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    return pa_audio_AutomaticGainControlSwitch(streamPtr->audioInterface, LE_ON);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to disable the automatic gain control on the selected audio stream.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 * @note The process exits, if an invalid audio stream reference is given.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_DisableAutomaticGainControl
(
    le_audio_StreamRef_t    streamRef       ///< [IN] The audio stream reference.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    return pa_audio_AutomaticGainControlSwitch(streamPtr->audioInterface, LE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the audio profile.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_SetProfile
(
    le_audio_Profile_t profile   ///< [IN] The audio profile.
)
{
    return pa_audio_SetProfile(profile);
}
//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the audio profile in use.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_GetProfile
(
    le_audio_Profile_t* profilePtr  ///< [OUT] The audio profile.
)
{
    return pa_audio_GetProfile(profilePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Configure the PCM Sampling Rate.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OUT_OF_RANGE  Your platform does not support the setting's value.
 * @return LE_BUSY          The PCM interface is already active.
 * @return LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_SetPcmSamplingRate
(
    uint32_t    rate         ///< [IN] Sampling rate in Hz.
)
{
    return pa_audio_SetPcmSamplingRate(rate);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the PCM Sampling Resolution.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OUT_OF_RANGE  Your platform does not support the setting's value.
 * @return LE_BUSY          The PCM interface is already active.
 * @return LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_SetPcmSamplingResolution
(
    uint32_t  bitsPerSample   ///< [IN] Sampling resolution (bits/sample).
)
{
    return pa_audio_SetPcmSamplingResolution(bitsPerSample);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the PCM Companding.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OUT_OF_RANGE  Your platform does not support the setting's value.
 * @return LE_BUSY          The PCM interface is already active.
 * @return LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_SetPcmCompanding
(
    le_audio_Companding_t companding   ///< [IN] Companding.
)
{
    return pa_audio_SetPcmCompanding(companding);
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the PCM Sampling Rate.
 *
 * @return The sampling rate in Hz.
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_audio_GetPcmSamplingRate
(
    void
)
{
    return pa_audio_GetPcmSamplingRate();
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the PCM Sampling Resolution.
 *
 * @return The sampling resolution (bits/sample).
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_audio_GetPcmSamplingResolution
(
    void
)
{
    return pa_audio_GetPcmSamplingResolution();
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the PCM Companding.
 *
 * @return The PCM companding.
 */
//--------------------------------------------------------------------------------------------------
le_audio_Companding_t le_audio_GetPcmCompanding
(
    void
)
{
    return pa_audio_GetPcmCompanding();
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the default PCM time slot used on the current platform.
 *
 * @return the time slot number.
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_audio_GetDefaultPcmTimeSlot
(
    void
)
{
    return pa_audio_GetDefaultPcmTimeSlot();
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the default I2S channel mode used on the current platform.
 *
 * @return the I2S channel mode.
 */
//--------------------------------------------------------------------------------------------------
le_audio_I2SChannel_t le_audio_GetDefaultI2sMode
(
    void
)
{
    return pa_audio_GetDefaultI2sMode();
}


