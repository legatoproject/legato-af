//--------------------------------------------------------------------------------------------------
/**
 * @file le_audio.c
 *
 * This file contains the source code of the high level Audio API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "le_audio_local.h"
#include "le_media_local.h"
#include "pa_audio.h"
#include "watchdogChain.h"

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
#define EVENTID_DEFAULT_POOL_SIZE       2

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
#define IS_OUTPUT_IF(interface)     (   (interface == LE_AUDIO_IF_CODEC_SPEAKER) || \
                                        (interface == LE_AUDIO_IF_DSP_FRONTEND_USB_TX) || \
                                        (interface == LE_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX) || \
                                        (interface == LE_AUDIO_IF_DSP_FRONTEND_PCM_TX) || \
                                        (interface == LE_AUDIO_IF_DSP_FRONTEND_I2S_TX) || \
                                        (interface == LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE) \
                                    )

//--------------------------------------------------------------------------------------------------
/**
 * The timer interval to kick the watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
#define MS_WDOG_INTERVAL 8

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
    le_audio_StreamEventBitMask_t       streamEventMask;
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
    le_audio_If_t    audioInterface;
    union
    {
        int32_t                 timeslot;
        le_audio_I2SChannel_t   mode;
        int32_t                 fd;
    } param;
    bool            physicalStream;

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
    le_hashmap_Ref_t        streamInList;     ///< list of input streams tied to this connector.
    le_hashmap_Ref_t        streamOutList;    ///< list of output streams tied to this connector.
    le_msg_SessionRef_t     sessionRef;       ///< client sessionRef
    le_audio_ConnectorRef_t connectorRef;     ///< connector reference
    le_dls_Link_t           connectorsLink;   ///< link for all connector
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
/**
 * EventIdList structure.
 * Objects of this type are used for le_audio_Stream.
 *
 */
//--------------------------------------------------------------------------------------------------
struct eventIdList {
    le_event_Id_t   eventId; ///< the eventId
    bool            isUsed;  ///< is it used?
    le_dls_Link_t   link;    ///< link for eventIdList
};
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
 * List for all eventId objects
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t    EventIdList;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for HashMapList objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t AudioHashMapPool;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for EventIdList objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t EventIdPool;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for stream reference node objects (used by stream event add/remove handler
 * functions)
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t StreamEventHandlerRefNodePool;

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
static le_audio_SamplePcmConfig_t SampleDefaultPcmConfig =
{
    .sampleRate = 8000,
    .channelsCount = 1,
    .bitsPerSample = 16,
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
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t OpenStreamPaths
(
    le_audio_Stream_t*      streamPtr,      ///< [IN] The stream
    le_hashmap_Ref_t        streamListPtr   ///< [IN] The stream list
)
{
    le_audio_Stream_t* inputStreamPtr;
    le_audio_Stream_t* outputStreamPtr;
    le_result_t   res = LE_OK;

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("streamPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(streamListPtr);

    while (le_hashmap_NextNode(streamIterator)==LE_OK)
    {
        le_audio_Stream_t* currentStreamPtr=(le_audio_Stream_t*)le_hashmap_GetValue(streamIterator);

        LE_DEBUG("CurrentStream %p",currentStreamPtr);

        if (streamPtr->isInput)
        {
            inputStreamPtr  = streamPtr;
            outputStreamPtr = currentStreamPtr;
        }
        else
        {
            inputStreamPtr  = currentStreamPtr;
            outputStreamPtr = streamPtr;
        }

        // Set Audio Path
        LE_DEBUG("Input [%d] and Output [%d] are tied together.",
                 inputStreamPtr->audioInterface,
                 outputStreamPtr->audioInterface);

        res = pa_audio_SetDspAudioPath(inputStreamPtr, outputStreamPtr);
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function reset all dsp path from streamPtr to all stream into the streamListPtr
 *
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CloseStreamPaths
(
    le_audio_Stream_t*    streamPtr,      ///< [IN] The stream
    le_hashmap_Ref_t      streamListPtr   ///< [IN] The stream list
)
{
    le_audio_Stream_t* inputStreamPtr;
    le_audio_Stream_t* outputStreamPtr;

    le_result_t   res = LE_OK;

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("streamPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    LE_DEBUG("CloseStreamPaths stream.%p", streamPtr);

    le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(streamListPtr);

    while (le_hashmap_NextNode(streamIterator)==LE_OK)
    {
        le_audio_Stream_t* currentStreamPtr=(le_audio_Stream_t*)le_hashmap_GetValue(streamIterator);

        if (streamPtr->isInput)
        {
            inputStreamPtr  = streamPtr;
            outputStreamPtr = currentStreamPtr;
        }
        else
        {
            inputStreamPtr  = currentStreamPtr;
            outputStreamPtr = streamPtr;
        }

        // Reset Audio Path
        LE_DEBUG("Reset the DSP audio path (inputInterface.%d with outputInterface.%d)",
                 inputStreamPtr->audioInterface,
                 outputStreamPtr->audioInterface);

        res = pa_audio_ResetDspAudioPath(inputStreamPtr, outputStreamPtr);
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function reset all dsp path, for all connector attached, for the streamPtr.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DisconnectStreamFromAllConnectors
(
    le_audio_Stream_t*     streamPtr       ///< [IN] The audio stream.
)
{
    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("streamPtr is NULL !");
        return;
    }

    LE_DEBUG("DisconnectStreamFromAllConnectors streamPtr.%p", streamPtr);

    le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(streamPtr->connectorList);

    while (le_hashmap_NextNode(streamIterator)==LE_OK)
    {
        le_audio_Connector_t const * currentConnectorPtr = le_hashmap_GetValue(streamIterator);

        le_audio_Disconnect(currentConnectorPtr->connectorRef, streamPtr->streamRef);
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
        le_audio_Stream_t* currentStreamPtr=(le_audio_Stream_t*)le_hashmap_GetValue(streamIterator);

        CloseStreamPaths(currentStreamPtr,connectorPtr->streamOutList);
    }
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
    struct hashMapList* currentPtr=NULL;
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

    char ConnectorMapName[20];

    snprintf( ConnectorMapName,20,"ConnMap%d", (int) (le_dls_NumLinks(&AudioHashMapList)+1) );

    currentPtr->hashMapRef = le_hashmap_Create(ConnectorMapName,
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
 * This function finds or creates an eventId into the EventIdList
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t GetEventId
(
    void
)
{
    struct eventIdList* currentPtr = NULL;
    le_dls_Link_t*      linkPtr = le_dls_Peek(&EventIdList);
    char                eventIdName[25];
    int32_t             eventIdIdx = 1;

    while (linkPtr!=NULL)
    {
        currentPtr = CONTAINER_OF(linkPtr,
                                  struct eventIdList,
                                  link);

        if (!currentPtr->isUsed)
        {
            LE_DEBUG("Found one unused eventId (%p)", currentPtr->eventId);
            currentPtr->isUsed = true;
            return currentPtr->eventId;
        }
        linkPtr = le_dls_PeekNext(&EventIdList,linkPtr);

        eventIdIdx++;
    }

    snprintf(eventIdName, sizeof(eventIdName), "streamEventId-%d", eventIdIdx);

    currentPtr = le_mem_ForceAlloc(EventIdPool);
    currentPtr->eventId = le_event_CreateId(eventIdName, sizeof(le_audio_StreamEvent_t));
    currentPtr->isUsed = true;
    currentPtr->link = LE_DLS_LINK_INIT;

    le_dls_Queue(&EventIdList, &(currentPtr->link));

    LE_DEBUG("Create a new eventId (%p)", currentPtr->eventId);

    return currentPtr->eventId;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function releases an eventId from the EventIdList
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReleaseEventId
(
    le_event_Id_t eventId
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&EventIdList);

    while (linkPtr!=NULL)
    {
        struct eventIdList* currentPtr = CONTAINER_OF(linkPtr,
                                                      struct eventIdList,
                                                      link);

        if (currentPtr->eventId == eventId)
        {
            LE_DEBUG("Found eventId to release (%p)", currentPtr->eventId);
            currentPtr->isUsed = false;
            return;
        }
        linkPtr = le_dls_PeekNext(&EventIdList,linkPtr);
    }

    LE_DEBUG("could not found eventId to release");
    return;
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

    memset(streamPtr,0,sizeof(le_audio_Stream_t));
    streamPtr->fd=LE_AUDIO_NO_FD;
    streamPtr->connectorList = GetHashMapElement();
    streamPtr->deviceIdentifier = -1;
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
    le_audio_StreamEvent_t*      streamEventPtr = reportPtr;
    StreamEventHandlerRefNode_t* streamRefNodePtr = le_event_GetContextPtr();

    if ((!streamRefNodePtr) || (!streamEventPtr) || (!streamEventPtr->streamPtr))
    {
        LE_ERROR("Invalid reference provided!");
        return;
    }

    le_audio_Stream_t* streamPtr = streamEventPtr->streamPtr;

    switch ( streamEventPtr->streamEvent )
    {
        case LE_AUDIO_BITMASK_MEDIA_EVENT:
        {
            le_audio_MediaEvent_t mediaEvent = streamEventPtr->event.mediaEvent;

            LE_DEBUG("mediaEvent %d", mediaEvent);

            // In case of file playing, MEDIA_NO_MORE_SAMPLES corresponds to MEDIA_ENDED event
            if (streamPtr->playFile)
            {
                if (mediaEvent == LE_AUDIO_MEDIA_NO_MORE_SAMPLES)
                {
                       mediaEvent = LE_AUDIO_MEDIA_ENDED;
                }
            }

            le_audio_MediaHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;
            clientHandlerFunc(streamPtr->streamRef,
                              mediaEvent,
                              streamRefNodePtr->userCtx);
        }
        break;

        case LE_AUDIO_BITMASK_DTMF_DETECTION:
        {
            le_audio_DtmfDetectorHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;
            clientHandlerFunc(streamPtr->streamRef,
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
    le_audio_Stream_t* streamPtr,
    le_event_HandlerFunc_t handlerPtr,
    le_audio_StreamEventBitMask_t streamEventBitMask,
    void* contextPtr
)
{
    le_event_HandlerRef_t  handlerRef;

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid stream reference (%p) provided!", streamPtr);
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
        LE_DEBUG("Cannot find stream reference (%p)", streamRefNodePtr);
        return;
    }

    le_audio_Stream_t* streamPtr = streamRefNodePtr->streamPtr;

    le_event_RemoveHandler((le_event_HandlerRef_t)streamRefNodePtr->handlerRef);

    le_ref_DeleteRef(StreamEventHandlerRefMap, streamRefNodePtr->streamHandlerRef);

    if (streamRefNodePtr->streamEventMask == LE_AUDIO_BITMASK_DTMF_DETECTION)
    {
        // Count the number of DTMF_DETECTION handler installed
        uint32_t dtmfDetectionHandlerCount = 0;

        le_dls_Link_t* linkPtr = le_dls_Peek(&(streamPtr->streamRefWithEventHdlrList));

        while (linkPtr != NULL)
        {
            StreamEventHandlerRefNode_t* nodePtr;

            nodePtr = CONTAINER_OF(linkPtr, StreamEventHandlerRefNode_t, link);

            linkPtr = le_dls_PeekNext(&streamPtr->streamRefWithEventHdlrList, linkPtr);

            if (nodePtr->streamEventMask == LE_AUDIO_BITMASK_DTMF_DETECTION)
            {
                dtmfDetectionHandlerCount++;
            }
        }

        LE_DEBUG("dtmfDetectionHandlerCount %d", dtmfDetectionHandlerCount);
        if (dtmfDetectionHandlerCount == 1)
        {
            pa_audio_StopDtmfDecoder(streamPtr);

            pa_audio_RemoveDtmfStreamEventHandler(streamPtr->dtmfEventHandler);
            streamPtr->dtmfEventHandler = NULL;
        }
    }

    le_dls_Remove(&streamPtr->streamRefWithEventHdlrList,
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

    RemoveAllHandlersFromHdlrLists(streamPtr);

    LE_DEBUG("close streamRef %p of interface.%d",
                 streamPtr->streamRef,
                 streamPtr->audioInterface);

    le_media_Stop(streamPtr);

    if (streamPtr->fd != LE_AUDIO_NO_FD)
    {
        close(streamPtr->fd);
        streamPtr->fd = LE_AUDIO_NO_FD;
    }

    DisconnectStreamFromAllConnectors (streamPtr);

    pa_audio_ReleasePaParameters(streamPtr);

    le_hashmap_RemoveAll(streamPtr->connectorList);
    ReleaseHashMapElement(streamPtr->connectorList);

    ReleaseEventId(streamPtr->streamEventId);

    // Invalidate the Safe Reference.
    le_ref_DeleteRef(AudioStreamRefMap, streamPtr->streamRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Internal stream event Handler used for dtmf.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DtmfStreamEventHandler
(
    le_audio_StreamEvent_t* streamEventPtr,
    void*                   contextPtr
)
{
    if ((!streamEventPtr) || (!contextPtr))
    {
        LE_ERROR("Bad input !!");
        return;
    }

    le_audio_Stream_t* streamPtr = contextPtr;

    LE_DEBUG("Event detected, interface %d, streamEvent %d", streamPtr->audioInterface,
                                                             streamEventPtr->streamEvent);

    streamEventPtr->streamPtr = streamPtr;

    if (streamPtr->streamEventId)
    {
        le_event_Report(streamPtr->streamEventId,
                        streamEventPtr,
                        sizeof(le_audio_StreamEvent_t));
    }
    else
    {
        LE_ERROR("Unitialized streamEventId for streamPtr %p interface %d",
                                                            streamPtr, streamPtr->audioInterface);
    }
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
    bool alreadyOpened = false;
    le_audio_Stream_t* audioStreamPtr=NULL;

    // Physical stream can be opened only once: check if the stream is not already opened
    if (streamPtr->physicalStream)
    {
        le_ref_IterRef_t iterRef = le_ref_GetIterator(AudioStreamRefMap);

        while (!alreadyOpened && (le_ref_NextNode(iterRef) == LE_OK))
        {
            audioStreamPtr = (le_audio_Stream_t*) le_ref_GetValue(iterRef);

            if (audioStreamPtr->audioInterface == streamPtr->audioInterface)
            {
                alreadyOpened = true;
            }
        }
    }

    if ( !alreadyOpened )
    {
        audioStreamPtr = le_mem_ForceAlloc(AudioStreamPool);

        InitializeStream(audioStreamPtr);

        audioStreamPtr->audioInterface = streamPtr->audioInterface;
        audioStreamPtr->isInput = !IS_OUTPUT_IF(streamPtr->audioInterface);
        audioStreamPtr->streamRefWithEventHdlrList=LE_DLS_LIST_INIT;
        audioStreamPtr->sessionRefList=LE_DLS_LIST_INIT;

        // Specifc treatment of the asked audio interface
        switch ( streamPtr->audioInterface )
        {
            case LE_AUDIO_IF_DSP_FRONTEND_PCM_RX:
            case LE_AUDIO_IF_DSP_FRONTEND_PCM_TX:
                if ( pa_audio_SetPcmTimeSlot(audioStreamPtr, streamPtr->param.timeslot)
                                                                                          != LE_OK )
                {
                    LE_WARN("Cannot set timeslot of Secondary PCM interface");
                    le_mem_Release(audioStreamPtr);
                    return NULL;
                }
                // Default mode must be Master
                if ( pa_audio_SetMasterMode(audioStreamPtr) != LE_OK )
                {
                    LE_WARN("Cannot open Secondary PCM input as Master");
                    le_mem_Release(audioStreamPtr);
                    return NULL;
                }
            break;
            case LE_AUDIO_IF_DSP_FRONTEND_I2S_RX:
            case LE_AUDIO_IF_DSP_FRONTEND_I2S_TX:
                if ( pa_audio_SetI2sChannelMode(audioStreamPtr, streamPtr->param.mode)
                                                                                          != LE_OK )
                {
                    LE_WARN("Cannot set the channel mode of I2S interface");
                    le_mem_Release(audioStreamPtr);
                    return NULL;
                }
            break;
            case LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE:
                audioStreamPtr->encodingFormat = LE_AUDIO_WAVE;
                audioStreamPtr->sampleAmrConfig.amrMode = LE_AUDIO_AMR_NB_7_4_KBPS;
                audioStreamPtr->sampleAmrConfig.dtx = true;
                // No break here
            case LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY:
                audioStreamPtr->fd = streamPtr->param.fd;
                audioStreamPtr->streamEventId = GetEventId();

                memcpy( &audioStreamPtr->samplePcmConfig,
                        &SampleDefaultPcmConfig,
                        sizeof(le_audio_SamplePcmConfig_t) );
            break;
            case LE_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX:
                // streamEventId needed for DTMF detection
                audioStreamPtr->streamEventId = GetEventId();
            case LE_AUDIO_IF_CODEC_MIC:
            case LE_AUDIO_IF_CODEC_SPEAKER:
            default:
            break;
        }

        // Create and return a Safe Reference for this stream object.
        audioStreamPtr->streamRef = le_ref_CreateRef(AudioStreamRefMap, audioStreamPtr);

        LE_DEBUG("Open streamRef %p of interface.%d",
                 audioStreamPtr->streamRef,
                 audioStreamPtr->audioInterface);
    }
    else
    {
        le_mem_AddRef(audioStreamPtr);

        LE_DEBUG("AddRef for streamRef %p of interface.%d",
                 audioStreamPtr->streamRef,
                 audioStreamPtr->audioInterface);
    }

    SessionRefNode_t* newSessionRefPtr = le_mem_ForceAlloc(SessionRefPool);
    newSessionRefPtr->sessionRef = le_audio_GetClientSessionRef();
    newSessionRefPtr->link = LE_DLS_LINK_INIT;

    // Add the new sessionRef into the the sessionRef list
    le_dls_Queue(&audioStreamPtr->sessionRefList, &(newSessionRefPtr->link));

    return audioStreamPtr->streamRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove the client sessionRef from the audio stream, and release the audio stream
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReleaseAudioStream
(
    le_audio_Stream_t*  streamPtr,
    le_msg_SessionRef_t sessionRef,
    bool                releaseAllReferences
)
{
    SessionRefNode_t* sessionRefNodePtr;
    le_dls_Link_t* linkPtr;

    LE_DEBUG("audioItf %d, sessionRef %p", streamPtr->audioInterface, sessionRef);

    // Remove corresponding node from the sessionRefList
    linkPtr = le_dls_Peek(&(streamPtr->sessionRefList));
    while (linkPtr != NULL)
    {
        sessionRefNodePtr = CONTAINER_OF(linkPtr, SessionRefNode_t, link);

        linkPtr = le_dls_PeekNext(&(streamPtr->sessionRefList), linkPtr);

        LE_DEBUG("sessionRefNodePtr->sessionRef %p", sessionRefNodePtr->sessionRef);

        // Remove corresponding node from the sessionRefList
        if ( sessionRefNodePtr->sessionRef == sessionRef )
        {
            le_dls_Remove(&(streamPtr->sessionRefList),
                &(sessionRefNodePtr->link));

            le_mem_Release(sessionRefNodePtr);

            LE_DEBUG("Release stream %d", streamPtr->audioInterface);
            le_mem_Release(streamPtr);

            if (!releaseAllReferences)
            {
                return;
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

    // Close audio streams
    // This is a two stage process: parse audio stream reference map
    // once in order to close dsp frontend file play/capture streams
    // first, then parse it a second time to close remaining streams.
    iterRef = le_ref_GetIterator(AudioStreamRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        le_audio_Stream_t* audioStreamPtr = (le_audio_Stream_t*) le_ref_GetValue(iterRef);
        if ((audioStreamPtr->audioInterface == LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY) ||
            (audioStreamPtr->audioInterface == LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE))
        {
            ReleaseAudioStream(audioStreamPtr, sessionRef, true);
        }
    }
    // Reset map iterator and close remaining streams
    iterRef = le_ref_GetIterator(AudioStreamRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        le_audio_Stream_t* audioStreamPtr = (le_audio_Stream_t*) le_ref_GetValue(iterRef);
        ReleaseAudioStream(audioStreamPtr, sessionRef, true);
    }

    // Close connectors
    while ( result == LE_OK )
    {
        le_audio_ConnectorRef_t connectorRef = (le_audio_ConnectorRef_t) le_ref_GetSafeRef(iterRef);
        le_audio_Connector_t* connectorPtr = le_ref_Lookup(AudioConnectorRefMap, connectorRef);
        if (NULL == connectorPtr)
        {
            LE_ERROR("Invalid reference (%p) provided!", connectorRef);
            return;
        }

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
    // init multimedia service
    le_media_Init();

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

    EventIdPool = le_mem_CreatePool("EventIdPool", sizeof(struct eventIdList));
    le_mem_ExpandPool(EventIdPool, EVENTID_DEFAULT_POOL_SIZE);

    SessionRefPool = le_mem_CreatePool("SessionRefPool", sizeof(SessionRefNode_t));
    le_mem_ExpandPool(SessionRefPool, MAX_NUM_OF_STREAM);

    // init HashMapList
    AudioHashMapList = LE_DLS_LIST_INIT;

    // init EventIdList
    EventIdList = LE_DLS_LIST_INIT;

    // Create the Safe Reference Map to use for Connector object Safe References.
    AudioConnectorRefMap = le_ref_CreateMap("AudioConMap", MAX_NUM_OF_CONNECTOR);

    // Allocate the stream reference nodes pool.
    StreamEventHandlerRefNodePool = le_mem_CreatePool("StreamEventHandlerRefNodePool",
                                                sizeof(StreamEventHandlerRefNode_t));

    // Create the safe Reference Map to use for the Stream Event object safe Reference.
    StreamEventHandlerRefMap =  le_ref_CreateMap("StreamEventHandlerRefMap", MAX_NUM_OF_CONNECTOR);

    // Add a handler to the close session service
    le_msg_AddServiceCloseHandler( le_audio_GetServiceRef(),
                                   CloseSessionEventHandler,
                                   NULL );

    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);
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

    openStream.audioInterface = LE_AUDIO_IF_CODEC_MIC;
    openStream.physicalStream = true;

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

    openStream.audioInterface = LE_AUDIO_IF_CODEC_SPEAKER;
    openStream.physicalStream = true;

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

    openStream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_USB_RX;
    openStream.physicalStream = true;

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

    openStream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_USB_TX;
    openStream.physicalStream = true;

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

    openStream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_PCM_RX;
    openStream.param.timeslot = timeslot;
    openStream.physicalStream = true;

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

    openStream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_PCM_TX;
    openStream.param.timeslot = timeslot;
    openStream.physicalStream = true;

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

    openStream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_I2S_RX;
    openStream.param.mode = mode;
    openStream.physicalStream = true;

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

    openStream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_I2S_TX;
    openStream.param.mode = mode;
    openStream.physicalStream = true;

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

    openStream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY;
    openStream.param.fd = LE_AUDIO_NO_FD;
    openStream.physicalStream = false;

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

    openStream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE;
    openStream.param.fd = LE_AUDIO_NO_FD;
    openStream.physicalStream = false;

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

    openStream.audioInterface = LE_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX;
    openStream.physicalStream = true;

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

    openStream.audioInterface = LE_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX;
    openStream.physicalStream = true;

    return OpenAudioStream(&openStream);
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

    ReleaseAudioStream(streamPtr, le_audio_GetClientSessionRef(), false);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Gain value of an input or output stream.
 *
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_OUT_OF_RANGE  The gain value is out of range.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_SetGain
(
    le_audio_StreamRef_t streamRef,  ///< [IN] The audio stream reference.
    int32_t             gain         ///< [IN] The gain value (specific to the platform)
)
{
    le_result_t        res;
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_BAD_PARAMETER;
    }

    if ( (res = pa_audio_SetGain(streamPtr, gain)) != LE_OK )
    {
        LE_ERROR("Cannot set stream gain");
        return res;
    }
    else
    {
        streamPtr->gain = gain;
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Gain value of an input or output stream.
 *
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_GetGain
(
    le_audio_StreamRef_t streamRef,  ///< [IN] The audio stream reference.
    int32_t            *gainPtr      ///< [OUT] The gain value (specific to the platform)
)
{
    le_result_t res;
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_BAD_PARAMETER;
    }
    if (gainPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_BAD_PARAMETER;
    }

    if ((res = pa_audio_GetGain(streamPtr, gainPtr)) != LE_OK)
    {
        LE_ERROR("Cannot get stream gain");
        return res;
    }

    streamPtr->gain = *gainPtr;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the value of a platform specific gain in the audio subsystem.
 *
 * @return LE_BAD_PARAMETER The pointer to the name of the platform specific gain is invalid.
 * @return LE_NOT_FOUND     The specified gain's name is not recognized in your audio subsystem.
 * @return LE_OUT_OF_RANGE  The gain parameter is out of range.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 * @warning Ensure to check the names of supported gains for your specific platform.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_SetPlatformSpecificGain
(
    const char*    gainNamePtr, ///< [IN] Name of the platform specific gain.
    int32_t        gain         ///< [IN] The gain value (specific to the platform)
)
{
    if (gainNamePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", gainNamePtr);
        return LE_BAD_PARAMETER;
    }
    return pa_audio_SetPlatformSpecificGain(gainNamePtr, gain);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the value of a platform specific gain in the audio subsystem.
 *
 * @return LE_BAD_PARAMETER The pointer to the name of the platform specific gain is invalid.
 * @return LE_NOT_FOUND     The specified gain's name is not recognized in your audio subsystem.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 * @warning Ensure to check the names of supported gains for your specific platform.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_GetPlatformSpecificGain
(
    const char*    gainNamePtr, ///< [IN] Name of the platform specific gain.
    int32_t*       gainPtr      ///< [OUT] The gain value (specific to the platform)
)
{
    if (gainNamePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", gainNamePtr);
        return LE_BAD_PARAMETER;
    }
    return pa_audio_GetPlatformSpecificGain(gainNamePtr, gainPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Mute an audio stream.
 *
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
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
    le_result_t res;
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_BAD_PARAMETER;
    }

    if ((res = pa_audio_Mute(streamPtr, true)) != LE_OK )
    {
        LE_ERROR("Cannot mute the interface");
        return res;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Unmute an audio stream.
 *
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
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
    le_result_t res;
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_BAD_PARAMETER;
    }


    if ((res = pa_audio_Mute(streamPtr, false)) != LE_OK )
    {
        LE_ERROR("Cannot unmute the interface");
        return res;
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
    newConnectorPtr->connectorRef = le_ref_CreateRef(AudioConnectorRefMap, newConnectorPtr);

    newConnectorPtr->connectorsLink = LE_DLS_LINK_INIT;

    // Add the new connector into the the connector list
    le_dls_Queue(&AllConnectorList,&(newConnectorPtr->connectorsLink));

    // Create and return a Safe Reference for this connector object.
    return newConnectorPtr->connectorRef;
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
 * @return LE_BUSY          There are insufficient DSP resources available.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_BAD_PARAMETER The connector and/or the audio stream references are invalid.
 * @return LE_FAULT         On any other failure.
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
    le_result_t res;
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
    // If there are 1 input stream and 1 output stream, then we can create the audio path.
    if (le_hashmap_Size(listPtr) >= 1)
    {
        if ((res = OpenStreamPaths (streamPtr, listPtr)) != LE_OK)
        {
            return res;
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
        if (le_hashmap_ContainsKey(connectorPtr->streamInList, streamPtr))
        {
            CloseStreamPaths (streamPtr,connectorPtr->streamOutList);
            le_hashmap_Remove(connectorPtr->streamInList,streamPtr);
            le_hashmap_Remove(streamPtr->connectorList,connectorPtr);
        }
        else
        {
            LE_ERROR("stream is not linked to the connector");
        }
    }
    else
    {
        if (le_hashmap_ContainsKey(connectorPtr->streamOutList, streamPtr))
        {
            CloseStreamPaths (streamPtr,connectorPtr->streamInList);
            le_hashmap_Remove(connectorPtr->streamOutList,streamPtr);
            le_hashmap_Remove(streamPtr->connectorList,connectorPtr);
        }
        else
        {
            LE_ERROR("stream is not linked to the connector");
        }
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
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid stream reference (%p) provided!", streamRef);
        return NULL;
    }

    // Register a handler function for Dtmf streams events
    if (streamPtr->dtmfEventHandler == NULL)
    {
        streamPtr->dtmfEventHandler = pa_audio_AddDtmfStreamEventHandler(DtmfStreamEventHandler,
                                                                     streamPtr);

        if (streamPtr->dtmfEventHandler == NULL)
        {
            LE_ERROR("Cannot register the dtmf handler function");
            return NULL;
        }

        if (pa_audio_StartDtmfDecoder(streamPtr) != LE_OK)
        {
            LE_ERROR("Cannot start DTMF detection!");
            return NULL;
        }
    }

    return (le_audio_DtmfDetectorHandlerRef_t) AddStreamEventHandler(
                                                    streamPtr,
                                                    (le_event_HandlerFunc_t) handlerPtr,
                                                    LE_AUDIO_BITMASK_DTMF_DETECTION,
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
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
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
    le_result_t res;

    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_BAD_PARAMETER;
    }

    if ( (res = pa_audio_NoiseSuppressorSwitch(streamPtr, LE_ON)) != LE_OK )
    {
        LE_ERROR("Cannot enable Noise Suppressor for audio stream");
        return res;
    }
    else
    {
        streamPtr->noiseSuppressorEnabled = true;
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to disable the Noise Suppressor.
 *
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
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
    le_result_t res;

    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_BAD_PARAMETER;
    }

    if ( (res = pa_audio_NoiseSuppressorSwitch(streamPtr, LE_OFF)) != LE_OK )
    {
        LE_ERROR("Cannot disable Noise Suppressor for audio stream");
        return res;
    }
    else
    {
        streamPtr->noiseSuppressorEnabled = false;
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable the Echo Canceller.
 *
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
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
    le_result_t res;

    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_BAD_PARAMETER;
    }

    if ( (res = pa_audio_EchoCancellerSwitch(streamPtr, LE_ON)) != LE_OK )
    {
        LE_ERROR("Cannot enable Echo Canceller for audio stream");
        return res;
    }
    else
    {
        streamPtr->echoCancellerEnabled = true;
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to disable the Echo Canceller.
 *
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
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
    le_result_t        res;

    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_BAD_PARAMETER;
    }

    if ( (res = pa_audio_EchoCancellerSwitch(streamPtr, LE_OFF)) != LE_OK )
    {
        LE_ERROR("Cannot disable Echo Canceller for audio stream");
        return res;
    }
    else
    {
        streamPtr->echoCancellerEnabled = false;
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the status of Noise Suppressor.
 *
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            Function succeeded.
 *
 * @note The process exits, if an invalid audio stream reference is given.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_IsNoiseSuppressorEnabled
(
    le_audio_StreamRef_t    streamRef,       ///< [IN] The audio stream reference.
    bool*                   statusPtr        ///< [OUT] true if NS is enabled, false otherwise
)
{
    le_result_t res;
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_BAD_PARAMETER;
    }

    if (statusPtr == NULL)
    {
        LE_KILL_CLIENT("statusPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if ((res = pa_audio_GetNoiseSuppressorStatus(streamPtr, statusPtr)) != LE_OK)
    {
        LE_ERROR("Cannot get stream NS status");
        return res;
    }

    streamPtr->noiseSuppressorEnabled = *statusPtr;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the status of Echo Canceller.
 *
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            Function succeeded.
 *
 * @note The process exits, if an invalid audio stream reference is given.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_IsEchoCancellerEnabled
(
    le_audio_StreamRef_t    streamRef,       ///< [IN] The audio stream reference.
    bool*                   statusPtr        ///< [OUT] true if EC is enabled, false otherwise
)
{
    le_result_t res;
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_BAD_PARAMETER;
    }

    if (statusPtr == NULL)
    {
        LE_KILL_CLIENT("statusPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if ((res = pa_audio_GetEchoCancellerStatus(streamPtr, statusPtr)) != LE_OK )
    {
        LE_ERROR("Cannot get stream NS status");
        return res;
    }

    streamPtr->echoCancellerEnabled = *statusPtr;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable the FIR (Finite Impulse Response) filter.
 *
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
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
        return LE_BAD_PARAMETER;
    }

    return pa_audio_FirFilterSwitch(streamPtr, LE_ON);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to disable the FIR (Finite Impulse Response) filter.
 *
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
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
        return LE_BAD_PARAMETER;
    }

    return pa_audio_FirFilterSwitch(streamPtr, LE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable the IIR (Infinite Impulse Response) filter.
 *
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
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
        return LE_BAD_PARAMETER;
    }

    return pa_audio_IirFilterSwitch(streamPtr, LE_ON);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to disable the IIR (Infinite Impulse Response) filter.
 *
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
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
        return LE_BAD_PARAMETER;
    }

    return pa_audio_IirFilterSwitch(streamPtr, LE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable the automatic gain control on the selected audio stream.
 *
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
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
        return LE_BAD_PARAMETER;
    }

    return pa_audio_AutomaticGainControlSwitch(streamPtr, LE_ON);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to disable the automatic gain control on the selected audio stream.
 *
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_UNAVAILABLE   The audio service initialization failed.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            Function succeeded.
 *
 * @note The process exits, if an invalid audio stream reference is given.
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
        return LE_BAD_PARAMETER;
    }

    return pa_audio_AutomaticGainControlSwitch(streamPtr, LE_OFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the audio profile.
 *
 * @return LE_UNAVAILABLE   On audio service initialization failure.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_SetProfile
(
    uint32_t profile   ///< [IN] The audio profile.
)
{
    return pa_audio_SetProfile(profile);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the audio profile in use.
 *
 * @return LE_UNAVAILABLE   On audio service initialization failure.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_GetProfile
(
    uint32_t* profilePtr  ///< [OUT] The audio profile.
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

    if ((streamPtr->audioInterface != LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY) &&
        (streamPtr->audioInterface != LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE))
    {
        LE_ERROR("Bad Interface!");
        return NULL;
    }

    return (le_audio_MediaHandlerRef_t) AddStreamEventHandler(streamPtr,
                                                                (le_event_HandlerFunc_t) handlerPtr,
                                                                LE_AUDIO_BITMASK_MEDIA_EVENT,
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
 * @note the used file descriptor is not deallocated, but is is rewound to the beginning.
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

    if (le_media_Stop(streamPtr) == LE_OK)
    {
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
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

    return le_media_Pause(streamPtr);

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

    return le_media_Resume(streamPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Flush the remaining audio samples.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_Flush
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

    return le_media_Flush(streamPtr);
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
 * @note
 *  - The fd is closed by the IPC API. To play again the same file, the fd parameter can be set
 *    to LE_AUDIO_NO_FD: in this case, the previous file descriptor is re-used.
 *    If the fd as to be kept on its side, the application should duplicate the fd (e.g., using
 *    dup() ) before calling the API.
 *    In that case, the old and new file descriptors refer to the same open file description and
 *    thus share file offset. So, once a playback has reached the end of file, the application must
 *    reset the file offset by using lseek on the duplicated descriptor to start the playback from
 *    the beginning.
 *
 * @note
 *  - Calling le_audio_PlayFile(<..>, LE_AUDIO_NO_FD) will rewind the audio file to the
 *    beginning when a playback is already in progress.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_PlayFile
(
    le_audio_StreamRef_t  streamRef,  ///< [IN] The audio stream reference.
    int32_t               fd          ///< [IN] The audio file descriptor.
)
{
    le_audio_Stream_t*          streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);
    le_audio_SamplePcmConfig_t  samplePcmConfig;
    le_result_t                 res;

    if ((streamPtr == NULL) || (fd < LE_AUDIO_NO_FD) || (fd == 0))
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    if ( le_media_IsStreamBusy(streamPtr) )
    {
        return LE_BUSY;
    }

    streamPtr->playFile = true;

    if (( fd != LE_AUDIO_NO_FD ) && ( streamPtr->fd != fd ))
    {
        // close previous file
        LE_DEBUG("close previous streamPtr->fd.%d of interface.%d",
                 streamPtr->fd,
                 streamPtr->audioInterface);

        close(streamPtr->fd);
        streamPtr->fd = fd;
    }
    else
    {
        LE_DEBUG("Rewind audio file.%d", streamPtr->fd);
        lseek(streamPtr->fd, 0, SEEK_SET);
    }

    if ( (res = le_media_Open(streamPtr, &samplePcmConfig)) != LE_OK )
    {
        return res;
    }

    return le_media_PlaySamples(streamPtr, &samplePcmConfig);
}

//--------------------------------------------------------------------------------------------------
/**
 * Initiate a playback sending samples over a pipe.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BUSY          The player interface is already active.
 * @return LE_OK            Function succeeded.
 *
 * @note The fd is closed by the IPC API. To use again the same pipe, the fd parameter can be set
 * to LE_AUDIO_NO_FD: in this case, the previous file descriptor is re-used.
 * If the fd as to be kept on its side, the application should duplicate the fd (e.g., using dup() )
 * before calling the API.
 *
 * @note Playback initiated with this function must be stopped by calling le_audio_Stop().
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

    if ( le_media_IsStreamBusy(streamPtr) )
    {
        return LE_BUSY;
    }

    if ( fd != LE_AUDIO_NO_FD )
    {
        LE_DEBUG("close previous streamPtr->fd.%d of interface.%d",
                 streamPtr->fd,
                 streamPtr->audioInterface);

        // close previous file
        close(streamPtr->fd);
        streamPtr->fd = fd;
    }

    streamPtr->playFile = false;

    return le_media_PlaySamples(streamPtr,
                                &streamPtr->samplePcmConfig);
}

//--------------------------------------------------------------------------------------------------
/**
 * Record a file on a recorder stream.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BAD_PARAMETER The audio stream reference is invalid.
 * @return LE_BUSY          The recorder interface is already active.
 * @return LE_OK            Function succeeded.
 *
 * @note the fd is closed by the API. To record again the same file, the fd parameter can be set to
 * LE_AUDIO_NO_FD: in this case, the previous file descriptor is re-used.
 * If the fd as to be kept on its side, the application should duplicate the fd (e.g., using dup() )
 * before calling the API.
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
    le_audio_SamplePcmConfig_t samplePcmConfig;

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    if ( le_media_IsStreamBusy(streamPtr) )
    {
        return LE_BUSY;
    }

    if (( fd != LE_AUDIO_NO_FD ) && ( streamPtr->fd != fd ))
    {
        LE_DEBUG("close previous streamPtr->fd.%d of interface.%d",
                 streamPtr->fd,
                 streamPtr->audioInterface);
        // close previous file
        close(streamPtr->fd);
        streamPtr->fd = fd;
    }
    else
    {
        LE_DEBUG("Rewind audio file.%d", streamPtr->fd);
        lseek(streamPtr->fd, 0, SEEK_SET);
    }

    if ( le_media_Open(streamPtr, &samplePcmConfig) != LE_OK )
    {
        return LE_FAULT;
    }

    if ( streamPtr->fd != LE_AUDIO_NO_FD )
    {
        if ( le_media_Capture(streamPtr, &samplePcmConfig) != LE_OK )
        {
            le_media_Stop(streamPtr);
            return LE_FAULT;
        }
    }
    else
    {
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Record a file on a recorder stream.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BUSY          The recorder interface is already active.
 * @return LE_OK            Function succeeded.
 *
 * @note The fd is closed by the API. To use again the same pipe, the fd parameter can be set to
 * LE_AUDIO_NO_FD: in this case, the previous file descriptor is re-used.
 * If the fd as to be kept on its side, the application should duplicate the fd (e.g., using dup() )
 * before calling the API.
 *
 * @note When using this function recording must be stopped by calling le_audio_Stop().
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

    if ( le_media_IsStreamBusy(streamPtr) )
    {
        return LE_BUSY;
    }

    if ( fd != LE_AUDIO_NO_FD )
    {
        LE_DEBUG("close previous streamPtr->fd.%d of interface.%d",
                 streamPtr->fd,
                 streamPtr->audioInterface);
        // close previous file
        close(streamPtr->fd);
        streamPtr->fd = fd;
    }

    return le_media_Capture(streamPtr,
                                &streamPtr->samplePcmConfig);
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
 * @note If the DTMF string is too long (max DTMF_MAX_LEN characters), it is a fatal
 *       error, the function will not return.
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

    if ( le_media_IsStreamBusy(streamPtr) )
    {
        return LE_BUSY;
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
 * @note If the DTMF string is too long (max DTMF_MAX_LEN characters), it is a fatal
 *       error, the function will not return.
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

//--------------------------------------------------------------------------------------------------
/**
 * Set the encoding format of a recorder stream.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_SetEncodingFormat
(
    le_audio_StreamRef_t streamRef,
        ///< [IN]
        ///< Audio stream reference.

    le_audio_Format_t format
        ///< [IN]
        ///< Encoding format.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid stream reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    streamPtr->encodingFormat = format;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the encoding format of a recorder stream.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 * @note A client calling this function with either an invalid
 * streamRef or null formatPtr parameter will be killed and the
 * function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_GetEncodingFormat
(
    le_audio_StreamRef_t streamRef,
        ///< [IN]
        ///< Audio stream reference.

    le_audio_Format_t* formatPtr
        ///< [OUT]
        ///< Encoding format.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid stream reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    if (formatPtr == NULL)
    {
        LE_KILL_CLIENT("formatPtr is NULL!");
        return LE_FAULT;
    }

    *formatPtr = streamPtr->encodingFormat;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the AMR mode for AMR encoder.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_SetSampleAmrMode
(
    le_audio_StreamRef_t streamRef,
        ///< [IN]
        ///< Audio stream reference.

    le_audio_AmrMode_t mode
        ///< [IN]
        ///< AMR mode.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid stream reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    streamPtr->sampleAmrConfig.amrMode = mode;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the AMR mode for AMR encoder.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 * @note A client calling this function with a null modePtr parameter
 * will be killed and the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_GetSampleAmrMode
(
    le_audio_StreamRef_t streamRef,
        ///< [IN]
        ///< Audio stream reference.

    le_audio_AmrMode_t* modePtr
        ///< [OUT]
        ///< AMR mode.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid stream reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    if (modePtr == NULL)
    {
        LE_KILL_CLIENT("modePtr is NULL!");
        return LE_FAULT;
    }

    *modePtr = streamPtr->sampleAmrConfig.amrMode;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the AMR discontinuous transmission (DTX)
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_SetSampleAmrDtx
(
    le_audio_StreamRef_t streamRef,
        ///< [IN]
        ///< Audio stream reference.

    bool dtx
        ///< [IN]
        ///< DTX.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid stream reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    streamPtr->sampleAmrConfig.dtx = dtx;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the AMR discontinuous transmission (DTX) value.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_GetSampleAmrDtx
(
    le_audio_StreamRef_t streamRef,
        ///< [IN]
        ///< Audio stream reference.

    bool* dtxPtr
        ///< [OUT]
        ///< DTX.
)
{
    le_audio_Stream_t* streamPtr = le_ref_Lookup(AudioStreamRefMap, streamRef);

    if (streamPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid stream reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    *dtxPtr = streamPtr->sampleAmrConfig.dtx;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Mute the Call Waiting Tone.
 *
 * @return LE_UNAVAILABLE   On audio service initialization failure.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_MuteCallWaitingTone
(
    void
)
{
    return pa_audio_MuteCallWaitingTone(true);
}

//--------------------------------------------------------------------------------------------------
/**
 * Unmute the Call Waiting Tone.
 *
 * @return LE_UNAVAILABLE   On audio service initialization failure.
 * @return LE_FAULT         On any other failure.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_audio_UnmuteCallWaitingTone
(
    void
)
{
    return pa_audio_MuteCallWaitingTone(false);
}

