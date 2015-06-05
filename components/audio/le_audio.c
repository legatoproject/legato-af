//--------------------------------------------------------------------------------------------------
/**
 * @file le_audio.c
 *
 * This file contains the source code of the high level Audio API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "pa_audio.h"
#include "le_audio_local.h"
#include "le_media_local.h"

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
 * Macro to verify if an interface is an output interface.
 */
//--------------------------------------------------------------------------------------------------
#define IS_OUTPUT_IF(interface)     (   (interface == PA_AUDIO_IF_CODEC_SPEAKER) || \
                                        (interface == PA_AUDIO_IF_DSP_FRONTEND_USB_TX) || \
                                        (interface == PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX) || \
                                        (interface == PA_AUDIO_IF_DSP_FRONTEND_PCM_TX) || \
                                        (interface == PA_AUDIO_IF_DSP_FRONTEND_I2S_TX) || \
                                        (interface == PA_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE) \
                                    )

//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Reference type used by Add/Remove functions for EVENT 'StreamEvent'
 */
//--------------------------------------------------------------------------------------------------
typedef struct StreamEventHandlerRef* StreamEventHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Stream Event Handler Reference Node structure (information related to stream event handlers).
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_event_HandlerRef_t               handlerRef;
    StreamEventHandlerRef_t             streamHandlerRef;
    pa_audio_StreamEventBitMask_t       streamEventMask;
    struct le_audio_Stream*             streamPtr;
    void*                               userCtx;
    le_dls_Link_t                       link;
}
StreamEventHandlerRefNode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Open stream structure.
 * Objects of this type are used to open an audio stream.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    pa_audio_If_t    audioInterface;
    union
    {
        int32_t                 timeslot;
        le_audio_I2SChannel_t   mode;
        int32_t                 fd;
    } param;

}
OpenStream_t;

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
    le_msg_SessionRef_t sessionRef;           ///< client sessionRef
    le_dls_Link_t    connectorsLink;          ///< link for all connector
}
le_audio_Connector_t;

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
/**
 * SessionRef node structure used for the sessionRef list.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_SessionRef_t sessionRef;           ///< client sessionRef
    le_dls_Link_t       link;                 ///< link for SessionRefList
}
SessionRefNode_t;

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for the clients sessionRef objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SessionRefPool;

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
 * The memory pool for stream reference node objects (used by stream event add/remove handler
 * functions)
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t StreamEventHandlerRefNodePool;

//--------------------------------------------------------------------------------------------------
/**
 * Create Unique Stream for each PA_AUDIO_IF
 *
 */
//--------------------------------------------------------------------------------------------------
static le_audio_Stream_t*   AudioStream[PA_AUDIO_NUM_INTERFACES] = { NULL };

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for audio connector objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t AudioConnectorRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for stream event handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t StreamEventHandlerRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Default PCM configuation values.
 *
 */
//--------------------------------------------------------------------------------------------------
static pa_audio_SamplePcmConfig_t SampleDefaultPcmConfig =
{
    .sampleRate = 8000,
    .channelsCount = 1,
    .bitsPerSample = 16,
    .fileSize = -1,
    .pcmFormat = PCM_WAVE
};

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
static bool EqualsAudioRef
(
    const void* firstSafeRef,    ///< [IN] First SafeRef for comparing.
    const void* secondSafeRef    ///< [IN] Second SafeRef for comparing.
)
{
    return firstSafeRef == secondSafeRef;
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
    le_audio_Stream_t*      streamPtr,      ///< [IN] The stream
    le_hashmap_Ref_t        streamListPtr   ///< [IN] The stream list
)
{
    pa_audio_If_t inputInterface;
    pa_audio_If_t outputInterface;
    le_result_t   res = LE_OK;

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
            res = LE_FAULT;
        }
    }

    return res;
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
    le_audio_Stream_t const *    streamPtr,      ///< [IN] The stream
    le_hashmap_Ref_t             streamListPtr   ///< [IN] The stream list
)
{
    pa_audio_If_t inputInterface;
    pa_audio_If_t outputInterface;
    le_result_t   res = LE_OK;

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("streamPtr is NULL !");
        return LE_FAULT;
    }

    LE_DEBUG("CloseStreamPaths stream.%p", streamPtr);

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
            res = LE_FAULT;
        }
    }

    return res;
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
            CloseStreamPaths(streamPtr, currentConnectorPtr->streamOutList);
        }
        // check stream Out
        else
        {
            // close all connection from this output with all input
            CloseStreamPaths(streamPtr, currentConnectorPtr->streamInList);
        }
    }

    pa_audio_ResetDspAudioPath();
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

        CloseStreamPaths(currentStreamPtr,connectorPtr->streamOutList);
    }

    pa_audio_ResetDspAudioPath();
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

    ReleaseHashMapElement(connectorPtr->streamInList);
    ReleaseHashMapElement(connectorPtr->streamOutList);
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
    streamPtr->fd=LE_AUDIO_NO_FD;
    memset(streamPtr->format, 0, sizeof(streamPtr->format));
    streamPtr->connectorList = GetHashMapElement();
}

//--------------------------------------------------------------------------------------------------
/**
 * This function verify the validity of the DTMFs.
 *
 * @return true   All the DTMF are valid.
 * @return false  One or more DTMF are not valid.
 */
//--------------------------------------------------------------------------------------------------
static bool AreDtmfValid
(
    const char*   dtmfPtr  ///< [IN] The DTMFs to verify.
)
{
    char dtmfSet[] = "1234567890*#abcdABCD";
    uint32_t bufLen = strlen(dtmfPtr);
    uint32_t dtmfCount = strspn (dtmfPtr, dtmfSet);

    if (bufLen != dtmfCount)
    {
        return false;
    }
    else
    {
        return true;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer File Event Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerStreamEventHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    pa_audio_StreamEvent_t* streamEventPtr = reportPtr;
    StreamEventHandlerRefNode_t* streamRefNodePtr = le_event_GetContextPtr();


    if ((AudioStream[streamEventPtr->interface]->streamRef == NULL)||
        (streamRefNodePtr == NULL))
    {
        LE_KILL_CLIENT("Invalid reference provided!");
        return;
    }

    switch ( streamEventPtr->streamEvent )
    {
        case PA_AUDIO_BITMASK_MEDIA_EVENT:
        {
            le_audio_MediaHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;
                clientHandlerFunc(AudioStream[streamEventPtr->interface]->streamRef,
                      streamEventPtr->event.mediaEvent,
                      streamRefNodePtr->userCtx);
        }
        break;

        case PA_AUDIO_BITMASK_DTMF_DETECTION:
        {
            le_audio_DtmfDetectorHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;
                clientHandlerFunc(AudioStream[streamEventPtr->interface]->streamRef,
                      streamEventPtr->event.dtmf,
                      streamRefNodePtr->userCtx);
        }
        break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'StreamEvent'
 *
 * This event provides information on player / recorder stream events.
 */
//--------------------------------------------------------------------------------------------------
static StreamEventHandlerRef_t AddStreamEventHandler
(
    le_audio_StreamRef_t streamRef,
    le_event_HandlerFunc_t handlerPtr,
    pa_audio_StreamEventBitMask_t streamEventBitMask,
    void* contextPtr
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

    LE_DEBUG("Add stream event handler on interface %d.", streamPtr->audioInterface);

    // Associate the handlerRef to the streamRef
    StreamEventHandlerRefNode_t* streamRefNodePtr = le_mem_ForceAlloc(
                                                                StreamEventHandlerRefNodePool);
    streamRefNodePtr->streamEventMask = streamEventBitMask;

    streamRefNodePtr->link = LE_DLS_LINK_INIT;

    streamRefNodePtr->streamPtr = streamPtr;

    handlerRef = le_event_AddLayeredHandler("StreamEventHandler",
                                            streamPtr->streamEventId,
                                            FirstLayerStreamEventHandler,
                                            handlerPtr);

    streamRefNodePtr->userCtx = contextPtr;

    le_event_SetContextPtr(handlerRef, streamRefNodePtr);

    streamRefNodePtr->handlerRef = handlerRef;

    le_dls_Queue(&(streamPtr->streamRefWithEventHdlrList),&(streamRefNodePtr->link));

    streamRefNodePtr->streamHandlerRef = le_ref_CreateRef( StreamEventHandlerRefMap,
                                                                streamRefNodePtr );

    return streamRefNodePtr->streamHandlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_audio_RemoveStreamEventHandler handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
static void RemoveStreamEventHandler
(
    StreamEventHandlerRef_t addHandlerRef ///< [IN]
)
{
    StreamEventHandlerRefNode_t* streamRefNodePtr = le_ref_Lookup(StreamEventHandlerRefMap,
                                                                  addHandlerRef);

    if (streamRefNodePtr == NULL)
    {
        LE_WARN("Invalid reference (%p) provided!", streamRefNodePtr);
        return;
    }

    le_event_RemoveHandler((le_event_HandlerRef_t)streamRefNodePtr->handlerRef);

    le_ref_DeleteRef(StreamEventHandlerRefMap, streamRefNodePtr->streamHandlerRef);

    if (streamRefNodePtr->streamEventMask == PA_AUDIO_BITMASK_DTMF_DETECTION)
    {
        pa_audio_StopDtmfDecoder(streamRefNodePtr->streamPtr->audioInterface);
    }

    le_dls_Remove(&streamRefNodePtr->streamPtr->streamRefWithEventHdlrList,
                  &streamRefNodePtr->link);

    le_mem_Release(streamRefNodePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function remove all the handler references from the Handlers' lists tied to the stream
 *
 */
//--------------------------------------------------------------------------------------------------
static void RemoveAllHandlersFromHdlrLists
(
    le_audio_Stream_t* streamPtr
)
{
    le_dls_Link_t*               linkPtr;

    // Stream events handlers
    if (streamPtr->streamRefWithEventHdlrList.headLinkPtr != NULL)
    {
        StreamEventHandlerRefNode_t* nodePtr;

        // Remove corresponding node from the streamRefWithEventHdlrList
        linkPtr = le_dls_Peek(&(streamPtr->streamRefWithEventHdlrList));
        while (linkPtr != NULL)
        {
            nodePtr = CONTAINER_OF(linkPtr, StreamEventHandlerRefNode_t, link);

            linkPtr = le_dls_PeekNext(&streamPtr->streamRefWithEventHdlrList, linkPtr);

            RemoveStreamEventHandler(nodePtr->streamHandlerRef);
        }
    }
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

    pa_audio_Stop(streamPtr->audioInterface);

    if (streamPtr->fd != LE_AUDIO_NO_FD)
    {
        close(streamPtr->fd);
    }

    DeleteAllConnectorPathsFromStream (streamPtr);

    RemoveAllHandlersFromHdlrLists(streamPtr);

    le_hashmap_RemoveAll(streamPtr->connectorList);
    ReleaseHashMapElement(streamPtr->connectorList);

    // Invalidate the Safe Reference.
    le_ref_DeleteRef(AudioStreamRefMap, streamPtr->streamRef);

    AudioStream[streamPtr->audioInterface] = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Internal stream event Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void StreamEventHandler
(
    pa_audio_StreamEvent_t* streamEventPtr,
    void*                 contextPtr
)
{
    LE_DEBUG("Event detected, interface %d, streamEvent %d", streamEventPtr->interface,
                                                             streamEventPtr->streamEvent);

    if (!AudioStream[streamEventPtr->interface] ||
        !AudioStream[streamEventPtr->interface]->streamEventId)
    {
        LE_ERROR("Stream not opened %d !!!!!", streamEventPtr->interface);
        return;
    }

    le_event_Report(AudioStream[streamEventPtr->interface]->streamEventId,
                    streamEventPtr,
                    sizeof(pa_audio_StreamEvent_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Open an audio stream.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_audio_StreamRef_t OpenAudioStream
(
    OpenStream_t* streamPtr
)
{
    LE_DEBUG("Open audio stream (%d)", streamPtr->audioInterface);

    if (!AudioStream[streamPtr->audioInterface])
    {
        AudioStream[streamPtr->audioInterface] = le_mem_ForceAlloc(AudioStreamPool);

        InitializeStream(AudioStream[streamPtr->audioInterface]);

        AudioStream[streamPtr->audioInterface]->audioInterface = streamPtr->audioInterface;
        AudioStream[streamPtr->audioInterface]->isInput = !IS_OUTPUT_IF(streamPtr->audioInterface);
        AudioStream[streamPtr->audioInterface]->streamRefWithEventHdlrList=LE_DLS_LIST_INIT;
        AudioStream[streamPtr->audioInterface]->sessionRefList=LE_DLS_LIST_INIT;

        // Create and return a Safe Reference for this stream object.
        AudioStream[streamPtr->audioInterface]->streamRef = le_ref_CreateRef(AudioStreamRefMap,
                                                            AudioStream[streamPtr->audioInterface]);

        // Specifc treatment of the asked audio interface
        switch ( streamPtr->audioInterface )
        {
            case PA_AUDIO_IF_CODEC_MIC:
            case PA_AUDIO_IF_CODEC_SPEAKER:
                if ( !pa_audio_IsCodecPresent() )
                {
                    LE_WARN("This device does not provide an in-built Codec");
                    le_mem_Release(AudioStream[streamPtr->audioInterface]);
                    AudioStream[streamPtr->audioInterface] = NULL;
                    return NULL;
                }
            break;
            case PA_AUDIO_IF_DSP_FRONTEND_PCM_RX:
            case PA_AUDIO_IF_DSP_FRONTEND_PCM_TX:
                if ( pa_audio_SetPcmTimeSlot(streamPtr->audioInterface,  streamPtr->param.timeslot) != LE_OK )
                {
                    LE_WARN("Cannot set timeslot of Secondary PCM interface");
                    le_mem_Release(AudioStream[streamPtr->audioInterface]);
                    AudioStream[streamPtr->audioInterface] = NULL;
                    return NULL;
                }
                // Default mode must be Master
                if ( pa_audio_SetMasterMode(streamPtr->audioInterface) != LE_OK )
                {
                    LE_WARN("Cannot open Secondary PCM input as Master");
                    le_mem_Release(AudioStream[streamPtr->audioInterface]);
                    AudioStream[streamPtr->audioInterface] = NULL;
                    return NULL;
                }
            break;
            case PA_AUDIO_IF_DSP_FRONTEND_I2S_RX:
            case PA_AUDIO_IF_DSP_FRONTEND_I2S_TX:
                if ( pa_audio_SetI2sChannelMode(streamPtr->audioInterface, streamPtr->param.mode) != LE_OK )
                {
                    LE_WARN("Cannot set the channel mode of I2S interface");
                    le_mem_Release(AudioStream[streamPtr->audioInterface]);
                    AudioStream[streamPtr->audioInterface] = NULL;
                    return NULL;
                }
            break;
            case PA_AUDIO_IF_DSP_FRONTEND_FILE_PLAY:
            case PA_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE:
                AudioStream[streamPtr->audioInterface]->fd = streamPtr->param.fd;
                AudioStream[streamPtr->audioInterface]->streamEventId = le_event_CreateId("streamEventId",
                                                                    sizeof(pa_audio_StreamEvent_t));

                memcpy( &AudioStream[streamPtr->audioInterface]->samplePcmConfig,
                        &SampleDefaultPcmConfig,
                        sizeof(pa_audio_SamplePcmConfig_t) );
            break;
            case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX:
                // streamEventId needed for DTMF detection
                AudioStream[streamPtr->audioInterface]->streamEventId = le_event_CreateId("streamEventId",
                                                                    sizeof(pa_audio_StreamEvent_t));
            default:
            break;
        }

    }
    else
    {
        le_mem_AddRef(AudioStream[streamPtr->audioInterface]);
    }

    SessionRefNode_t* newSessionRefPtr = le_mem_ForceAlloc(SessionRefPool);
    newSessionRefPtr->sessionRef = le_audio_GetClientSessionRef();
    newSessionRefPtr->link = LE_DLS_LINK_INIT;

    // Add the new sessionRef into the the sessionRef list
    le_dls_Queue(&AudioStream[streamPtr->audioInterface]->sessionRefList,
                        &(newSessionRefPtr->link));

    return AudioStream[streamPtr->audioInterface]->streamRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove the client sessionRef from the audio stream, and release the audio stream if asked
 *
 */
//--------------------------------------------------------------------------------------------------
static void RemoveSessionRefFromAudioStream
(
    le_audio_Stream_t*  streamPtr,
    le_msg_SessionRef_t sessionRef,
    bool                releaseAudioStream
)
{
    SessionRefNode_t* sessionRefNodePtr;
    le_dls_Link_t* linkPtr;

    // Remove corresponding node from the sessionRefList
    linkPtr = le_dls_Peek(&(streamPtr->sessionRefList));
    while (linkPtr != NULL)
    {
        sessionRefNodePtr = CONTAINER_OF(linkPtr, SessionRefNode_t, link);

        linkPtr = le_dls_PeekNext(&(streamPtr->sessionRefList), linkPtr);

        // Remove corresponding node from the sessionRefList
        if ( sessionRefNodePtr->sessionRef == sessionRef )
        {
            le_dls_Remove(&(streamPtr->sessionRefList),
                &(sessionRefNodePtr->link));

            le_mem_Release(sessionRefNodePtr);

            if (releaseAudioStream)
            {
                LE_DEBUG("Release stream %d", streamPtr->audioInterface);
                le_mem_Release(streamPtr);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * handler function to the close session service
 *
 */
//--------------------------------------------------------------------------------------------------
static void CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(AudioConnectorRefMap);

    le_result_t result = le_ref_NextNode(iterRef);

    // Close connectors
    while ( result == LE_OK )
    {
        le_audio_ConnectorRef_t connectorRef = (le_audio_ConnectorRef_t) le_ref_GetSafeRef(iterRef);
        le_audio_Connector_t* connectorPtr = le_ref_Lookup(AudioConnectorRefMap, connectorRef);

        // Get the next value in the reference maps (before releasing the node)
        result = le_ref_NextNode(iterRef);

        // Check if the session reference saved matchs with the current session reference.
        if (connectorPtr->sessionRef == sessionRef)
        {
            // Release the data connexion
            LE_DEBUG("Delete connector %p", connectorRef);
            le_audio_DeleteConnector( connectorRef );
        }
    }

    // Close audio stream
    int i;

    for (i = 0; i < PA_AUDIO_NUM_INTERFACES; i++)
    {
        if (AudioStream[i] != NULL)
        {
            RemoveSessionRefFromAudioStream(AudioStream[i], sessionRef, true);
        }
    }
}

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

#ifdef PRE_BUILT_PA
void _le_pa_audio_COMPONENT_INIT(void);
#endif /* PRE_BUILT_PA */

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the audio component.
 *
 * @note The process exits on failure
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    #ifdef PRE_BUILT_PA
    _le_pa_audio_COMPONENT_INIT();
    #endif /* PRE_BUILT_PA */

    // Allocate the audio stream pool.
    AudioStreamPool = le_mem_CreatePool("AudioStreamPool", sizeof(le_audio_Stream_t));
    le_mem_ExpandPool(AudioStreamPool, STREAM_DEFAULT_POOL_SIZE);
    le_mem_SetDestructor(AudioStreamPool,DestructStream);

    // Create the Safe Reference Map to use for Audio object Safe References.
    AudioStreamRefMap = le_ref_CreateMap("AudioStreamMap", MAX_NUM_OF_STREAM);

    // Allocate the audio connector pool.
    AudioConnectorPool = le_mem_CreatePool("AudioConnectorPool", sizeof(le_audio_Connector_t));
    le_mem_ExpandPool(AudioConnectorPool, CONNECTOR_DEFAULT_POOL_SIZE);

    AudioHashMapPool = le_mem_CreatePool("AudiohashMapPool", sizeof(struct hashMapList));
    le_mem_ExpandPool(AudioHashMapPool, HASHMAP_DEFAULT_POOL_SIZE);

    SessionRefPool = le_mem_CreatePool("SessionRefPool", sizeof(SessionRefNode_t));
    le_mem_ExpandPool(SessionRefPool, MAX_NUM_OF_STREAM);

    // init HashMapList
    AudioHashMapList = LE_DLS_LIST_INIT;

    // Create the Safe Reference Map to use for Connector object Safe References.
    AudioConnectorRefMap = le_ref_CreateMap("AudioConMap", MAX_NUM_OF_CONNECTOR);

    // Allocate the stream reference nodes pool.
    StreamEventHandlerRefNodePool = le_mem_CreatePool("StreamEventHandlerRefNodePool",
                                                sizeof(StreamEventHandlerRefNode_t));

    // Create the safe Reference Map to use for the Stream Event object safe Reference.
    StreamEventHandlerRefMap =  le_ref_CreateMap("StreamEventHandlerRefMap", MAX_NUM_OF_CONNECTOR);

    // Register a handler function for streams events
    LE_ERROR_IF((pa_audio_AddStreamEventHandler(StreamEventHandler, NULL) == NULL),
                "pa_audio_AddStreamEventHandler failed");

    // Add a handler to the close session service
    le_msg_AddServiceCloseHandler( le_audio_GetServiceRef(),
                                   CloseSessionEventHandler,
                                   NULL );

    // init multimedia service
    le_media_Init();
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
    OpenStream_t openStream;

    openStream.audioInterface = PA_AUDIO_IF_CODEC_MIC;

    return OpenAudioStream(&openStream);
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
    OpenStream_t openStream;

    openStream.audioInterface = PA_AUDIO_IF_CODEC_SPEAKER;

    return OpenAudioStream(&openStream);
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
    OpenStream_t openStream;

    openStream.audioInterface = PA_AUDIO_IF_DSP_FRONTEND_USB_RX;

    return OpenAudioStream(&openStream);
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
    OpenStream_t openStream;

    openStream.audioInterface = PA_AUDIO_IF_DSP_FRONTEND_USB_TX;

    return OpenAudioStream(&openStream);
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
    OpenStream_t openStream;

    openStream.audioInterface = PA_AUDIO_IF_DSP_FRONTEND_PCM_RX;
    openStream.param.timeslot = timeslot;

    return OpenAudioStream(&openStream);
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
    OpenStream_t openStream;

    openStream.audioInterface = PA_AUDIO_IF_DSP_FRONTEND_PCM_TX;
    openStream.param.timeslot = timeslot;

    return OpenAudioStream(&openStream);
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
    OpenStream_t openStream;

    openStream.audioInterface = PA_AUDIO_IF_DSP_FRONTEND_I2S_RX;
    openStream.param.mode = mode;

    return OpenAudioStream(&openStream);
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
    OpenStream_t openStream;

    openStream.audioInterface = PA_AUDIO_IF_DSP_FRONTEND_I2S_TX;
    openStream.param.mode = mode;

    return OpenAudioStream(&openStream);
}


//--------------------------------------------------------------------------------------------------
/**
 * Open the audio stream for playback.
 *
 * @return Reference to the audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenPlayer
(
    void
)
{
    OpenStream_t openStream;

    openStream.audioInterface = PA_AUDIO_IF_DSP_FRONTEND_FILE_PLAY;
    openStream.param.fd = LE_AUDIO_NO_FD;

    return OpenAudioStream(&openStream);
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the audio stream for recording.
 *
 * @return Reference to the audio stream, NULL if the function fails.
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenRecorder
(
    void
)
{
    OpenStream_t openStream;

    openStream.audioInterface = PA_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE;
    openStream.param.fd = LE_AUDIO_NO_FD;

    return OpenAudioStream(&openStream);
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
    OpenStream_t openStream;

    openStream.audioInterface = PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX;

    return OpenAudioStream(&openStream);
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
    OpenStream_t openStream;

    openStream.audioInterface = PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX;

    return OpenAudioStream(&openStream);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the audio format of an input or output stream.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 * @return LE_OVERFLOW      The format buffer wasn't big enough to accept the audio format string
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

    RemoveSessionRefFromAudioStream(streamPtr, le_audio_GetClientSessionRef(), false);

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

    if ( pa_audio_Mute(streamPtr->audioInterface, true) != LE_OK )
    {
        LE_ERROR("Cannot mute the interface");
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


    if ( pa_audio_Mute(streamPtr->audioInterface, false) != LE_OK )
    {
        LE_ERROR("Cannot unmute the interface");
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
    newConnectorPtr->sessionRef = le_audio_GetClientSessionRef();

    newConnectorPtr->connectorsLink = LE_DLS_LINK_INIT;

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

    LE_DEBUG("le_hashmap_Size(listPtr) %d", (int) le_hashmap_Size(listPtr));
    // If there are 1 input stream and 1 output stream, then we can create the corresponding thread.
    if (le_hashmap_Size(listPtr) >= 1)
    {
        if (OpenStreamPaths (streamPtr, listPtr)!=LE_OK)
        {
            return LE_FAULT;
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
        CloseStreamPaths (streamPtr,connectorPtr->streamOutList);
        le_hashmap_Remove(connectorPtr->streamInList,streamPtr);
        le_hashmap_Remove(streamPtr->connectorList,connectorPtr);
    }
    else
    {
        CloseStreamPaths (streamPtr,connectorPtr->streamInList);
        le_hashmap_Remove(connectorPtr->streamOutList,streamPtr);
        le_hashmap_Remove(streamPtr->connectorList,connectorPtr);
    }
    pa_audio_ResetDspAudioPath();
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
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid stream reference (%p) provided!", streamRef);
        return NULL;
    }

    if (pa_audio_StartDtmfDecoder(streamPtr->audioInterface) != LE_OK)
    {
        LE_ERROR("Bad Interface!");
        return NULL;
    }

    return (le_audio_DtmfDetectorHandlerRef_t) AddStreamEventHandler(
                                                    streamRef,
                                                    (le_event_HandlerFunc_t) handlerPtr,
                                                    PA_AUDIO_BITMASK_DTMF_DETECTION,
                                                    contextPtr
                                                    );
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
    RemoveStreamEventHandler( (StreamEventHandlerRef_t) addHandlerRef );
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable the Noise Suppressor.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 * @return LE_BAD_PARAMETER If streamRef contains an invalid audioInterface
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
 * @return LE_BAD_PARAMETER If streamRef contains an invalid audioInterface
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
 * @return LE_BAD_PARAMETER If streamRef contains an invalid audioInterface
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
 * @return LE_BAD_PARAMETER If streamRef contains an invalid audioInterface
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
 * @return LE_BAD_PARAMETER If streamRef contains an invalid audioInterface
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
 * @return LE_BAD_PARAMETER If streamRef contains an invalid audioInterface
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
 * @return LE_BAD_PARAMETER If streamRef contains an invalid audioInterface
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
 * @return LE_BAD_PARAMETER If streamRef contains an invalid audioInterface
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
 * @return LE_BAD_PARAMETER If streamRef contains an invalid audioInterface
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
 * @return LE_BAD_PARAMETER If streamRef contains an invalid audioInterface
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


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_audio_Media'
 *
 * This event provides information on player / recorder stream events.
 */
//--------------------------------------------------------------------------------------------------
le_audio_MediaHandlerRef_t le_audio_AddMediaHandler
(
    le_audio_StreamRef_t streamRef,
        ///< [IN]
        ///< The audio stream reference.

    le_audio_MediaHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    le_audio_Stream_t*     streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid stream reference (%p) provided!", streamRef);
        return NULL;
    }

    if ((streamPtr->audioInterface != PA_AUDIO_IF_DSP_FRONTEND_FILE_PLAY) &&
        (streamPtr->audioInterface != PA_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE))
    {
        LE_ERROR("Bad Interface!");
        return NULL;
    }

    return (le_audio_MediaHandlerRef_t) AddStreamEventHandler(streamRef,
                                                                (le_event_HandlerFunc_t) handlerPtr,
                                                                PA_AUDIO_BITMASK_MEDIA_EVENT,
                                                                contextPtr
                                                              );
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_audio_Media'
 */
//--------------------------------------------------------------------------------------------------
void le_audio_RemoveMediaHandler
(
    le_audio_MediaHandlerRef_t addHandlerRef
        ///< [IN]
)
{
    RemoveStreamEventHandler( (StreamEventHandlerRef_t) addHandlerRef );
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop the file playback/recording.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 * Note: the used file descriptor is not deallocated, but is is rewound to the beginning.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_Stop
(
    le_audio_StreamRef_t  streamRef  ///< [IN] The audio stream reference.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    return pa_audio_Stop(streamPtr->audioInterface);
}

//--------------------------------------------------------------------------------------------------
/**
 * Pause the file playback/recording.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_Pause
(
    le_audio_StreamRef_t  streamRef  ///< [IN] The audio stream reference.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    return pa_audio_Pause(streamPtr->audioInterface);

}

//--------------------------------------------------------------------------------------------------
/**
 * Resume a file playback/recording (need to be in pause state).
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_Resume
(
    le_audio_StreamRef_t  streamRef
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    return pa_audio_Resume(streamPtr->audioInterface);
}

//--------------------------------------------------------------------------------------------------
/**
 * Play a file on a playback stream.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_BUSY          The player interface is already active.
 * @return LE_OK            Function succeeded.
 *
 * Note: if the fd parameter is set to LE_AUDIO_NO_FD, the previous file descriptor is used to play
 * again the file.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_PlayFile
(
    le_audio_StreamRef_t  streamRef,  ///< [IN] The audio stream reference.
    int32_t               fd          ///< [IN] The audio file descriptor.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);
    pa_audio_SamplePcmConfig_t samplePcmConfig;

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    if (( fd != LE_AUDIO_NO_FD ) && ( streamPtr->fd != fd ))
    {
        // close previous file
        close(streamPtr->fd);
        streamPtr->fd = fd;
    }
    else
    {
        lseek(streamPtr->fd, 0, SEEK_SET);
    }

    if ( le_media_Open(streamPtr, &samplePcmConfig) != LE_OK )
    {
        return LE_FAULT;
    }

    if ( streamPtr->fd != LE_AUDIO_NO_FD )
    {
        return pa_audio_PlaySamples(streamPtr->audioInterface, streamPtr->fd, &samplePcmConfig);
    }
    else
    {
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initiate a playback sending samples over a pipe.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BUSY          The player interface is already active.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_PlaySamples
(
    le_audio_StreamRef_t streamRef , ///< Audio stream reference.
    int32_t              fd          ///< The file descriptor.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    if ( streamPtr->fd != LE_AUDIO_NO_FD )
    {
        // close previous file
        close(streamPtr->fd);
    }

    streamPtr->fd = fd;

    streamPtr->samplePcmConfig.pcmFormat = PCM_RAW;

    if ( streamPtr->fd != LE_AUDIO_NO_FD )
    {
        return pa_audio_PlaySamples(streamPtr->audioInterface,
                                    streamPtr->fd,
                                    &streamPtr->samplePcmConfig);
    }
    else
    {
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Record a file on a recording stream.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_BUSY          The recorder interface is already active.
 * @return LE_OK            Function succeeded.
 *
 * Note: if the fd parameter is set to LE_AUDIO_NO_FD, the previous file descriptor is used to record
 * again the file.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_RecordFile
(
    le_audio_StreamRef_t streamRef , ///< Audio stream reference.
    int32_t              fd          ///< The file descriptor.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    if (( fd != LE_AUDIO_NO_FD ) && ( streamPtr->fd != fd ))
    {
        // close previous file
        close(streamPtr->fd);
        streamPtr->fd = fd;
    }

    streamPtr->samplePcmConfig.pcmFormat = PCM_WAVE;

    if ( streamPtr->fd != LE_AUDIO_NO_FD )
    {
        return pa_audio_Capture( streamPtr->audioInterface,
                                streamPtr->fd,
                                &streamPtr->samplePcmConfig);
    }
    else
    {
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Record a file on a recording stream.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BUSY          The recorder interface is already active.
 * @return LE_OK            Function succeeded.
 *
 * Note: if the fd parameter is set to LE_AUDIO_NO_FD, the previous file descriptor is used to record
 * again the file.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_GetSamples
(
    le_audio_StreamRef_t streamRef , ///< Audio stream reference.
    int32_t              fd          ///< The file descriptor.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    if ( streamPtr->fd != LE_AUDIO_NO_FD )
    {
        // close previous file
        close(streamPtr->fd);
    }

    streamPtr->fd = fd;

    streamPtr->samplePcmConfig.pcmFormat = PCM_RAW;

    if ( streamPtr->fd != LE_AUDIO_NO_FD )
    {
        return pa_audio_Capture( streamPtr->audioInterface,
                                streamPtr->fd,
                                &streamPtr->samplePcmConfig);
    }
    else
    {
        return LE_FAULT;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the channel number of a PCM sample.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_SetSamplePcmChannelNumber
(
    le_audio_StreamRef_t streamRef,
        ///< [IN]
        ///< Audio stream reference.

    uint32_t nbChannel
        ///< [IN]
        ///< Channel Number
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    streamPtr->samplePcmConfig.channelsCount = nbChannel;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the channel number of a PCM sample.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_GetSamplePcmChannelNumber
(
    le_audio_StreamRef_t streamRef,
        ///< [IN]
        ///< Audio stream reference.

    uint32_t* nbChannelPtr
        ///< [OUT]
        ///< Channel Number
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    *nbChannelPtr = streamPtr->samplePcmConfig.channelsCount;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the PCM sampling rate of a PCM sample.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_SetSamplePcmSamplingRate
(
    le_audio_StreamRef_t streamRef,
        ///< [IN]
        ///< Audio stream reference.

    uint32_t rate
        ///< [IN]
        ///< PCM sampling Rate.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    streamPtr->samplePcmConfig.sampleRate = rate;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the PCM sampling rate of a PCM sample.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_GetSamplePcmSamplingRate
(
    le_audio_StreamRef_t streamRef,
        ///< [IN]
        ///< Audio stream reference.

    uint32_t* ratePtr
        ///< [OUT]
        ///< PCM sampling Rate.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    *ratePtr = streamPtr->samplePcmConfig.sampleRate;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the sampling resolution (in bits per sample) of a PCM sample.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_SetSamplePcmSamplingResolution
(
    le_audio_StreamRef_t streamRef,
        ///< [IN]
        ///< Audio stream reference.

    uint32_t samplingRes
        ///< [IN]
        ///< Sampling resolution (in bits per sample).
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    streamPtr->samplePcmConfig.bitsPerSample = samplingRes;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the sampling resolution (in bits per sample) of a PCM sample.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_GetSamplePcmSamplingResolution
(
    le_audio_StreamRef_t streamRef,
        ///< [IN]
        ///< Audio stream reference.

    uint32_t* samplingResPtr
        ///< [OUT]
        ///< Sampling resolution (in bits per sample).
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    *samplingResPtr = streamPtr->samplePcmConfig.bitsPerSample;

    return LE_OK;
}

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
le_result_t le_audio_PlayDtmf
(
    le_audio_StreamRef_t streamRef, ///< [IN] The audio stream reference.
    const char*          dtmfPtr,   ///< [IN] The DTMFs to play.
    uint32_t             duration,  ///< [IN] The DTMF duration in milliseconds.
    uint32_t             pause      ///< [IN] The pause duration between tones in milliseconds.
)
{
    le_audio_Stream_t*     streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);
    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid stream reference (%p) provided!", streamRef);
        return LE_BAD_PARAMETER;
    }
    if (dtmfPtr == NULL)
    {
        LE_KILL_CLIENT("dtmfPtr is NULL !");
        return LE_FAULT;
    }
    if(strlen(dtmfPtr) > LE_AUDIO_DTMF_MAX_LEN)
    {
        LE_KILL_CLIENT("strlen(dtmfPtr) > %d", LE_AUDIO_DTMF_MAX_LEN);
        return LE_FAULT;
    }

    if (!AreDtmfValid(dtmfPtr))
    {
        LE_ERROR("DTMF are not valid!");
        return LE_FORMAT_ERROR;
    }

    return le_media_PlayDtmf(streamPtr, dtmfPtr, duration, pause);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to ask to the Mobile Network to generate on the remote audio party
 * the DTMFs.
 *
 * @return LE_FORMAT_ERROR  The DTMF characters are invalid.
 * @return LE_BUSY          A DTMF playback is in progress.
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The funtion succeeded.
 *
 * @note The process exits, if an invalid audio stream reference is given.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_PlaySignallingDtmf
(
    const char*          dtmfPtr,   ///< [IN] The DTMFs to play.
    uint32_t             duration,  ///< [IN] The DTMF duration in milliseconds.
    uint32_t             pause      ///< [IN] The pause duration between tones in milliseconds.
)
{
    le_result_t res;

    if (!AreDtmfValid(dtmfPtr))
    {
        return LE_FORMAT_ERROR;
    }

    res = pa_audio_PlaySignallingDtmf(dtmfPtr, duration, pause);
    if (res == LE_DUPLICATE)
    {
        return LE_BUSY;
    }
    else if (res == LE_OK)
    {
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
}

