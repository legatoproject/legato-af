/**
 * @file le_rpcProxyEventHandler.c
 *
 * This file contains the source code for the RPC Proxy Event Handler Feature.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "le_rpcProxy.h"
#include "le_rpcProxyConfig.h"
#include "le_rpcProxyEventHandler.h"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of asynchronous even handlers.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_ASYNC_EVENT_HANDLER_MAX_NUM   LE_CONFIG_RPC_PROXY_ASYNC_EVENT_HANDLER_MAX_NUM

//--------------------------------------------------------------------------------------------------
/**
 * Structure is used in async events handling. It helps identify a client to pass async event to.
 */
//--------------------------------------------------------------------------------------------------
typedef struct ClientEventData
{
    le_msg_SessionRef_t sessionRef;   ///< Reference to client's session
    void*               contextPtr;   ///< Client's context pointer
    void*               handlerRef;   ///< Server's handle reference
    uint32_t            serviceId;    ///< Service ID
    uint32_t            ipcMsgId;     ///< IPC message ID
    uint16_t            ipcMsgSize;   ///< IPC message SIZE
}
ClientEventData_t;

//--------------------------------------------------------------------------------------------------
/**
 * Structure is used to save common IPC payload data buffer.
 */
//--------------------------------------------------------------------------------------------------
typedef struct CommonPayloadData
{
    uint32_t id;
    uint8_t buffer[0];
}
CommonPayloadData_t;

//--------------------------------------------------------------------------------------------------
/**
 * Hash Map to store Proxy Message ID (Key) and Proxy Reference (value) mappings.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(ProxyRefHashMap, RPC_PROXY_MSG_REFERENCE_MAX_NUM);
static le_hashmap_Ref_t ProxyRefMapByMsgId = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Client context (key) and Client session (value)
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(ClientEventDataSafeRefStaticMap, RPC_PROXY_ASYNC_EVENT_HANDLER_MAX_NUM);
static le_ref_MapRef_t ClientEventDataSafeRefMap = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * This pool is used to to allocate memory for Client context-session records.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ClientHandlerPool,
                          RPC_PROXY_ASYNC_EVENT_HANDLER_MAX_NUM,
                          sizeof(ClientEventData_t));
static le_mem_PoolRef_t ClientHandlerPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Function for deleting all ClientEventData_t records for given service and client session
 * If specified client session is NULL , deleting all records of the given service.
 */
//--------------------------------------------------------------------------------------------------
void rpcEventHandler_DeleteAll
(
    /// [IN] Service ID
    uint32_t serviceId,
    /// [IN] Client session reference
    le_msg_SessionRef_t sessionRef
)
{
    le_hashmap_It_Ref_t iter = le_hashmap_GetIterator(ProxyRefMapByMsgId);

    // Traverse the entire ProxyRefMapByMsgId map
    while (le_hashmap_NextNode(iter) == LE_OK)
    {
        // Get the proxy reference
         void* proxyRef = (void*) le_hashmap_GetValue(iter);

        ClientEventData_t* clientEventDataPtr =
            le_ref_Lookup(ClientEventDataSafeRefMap, proxyRef);

        if ((clientEventDataPtr != NULL) &&
            (clientEventDataPtr->serviceId == serviceId) &&
            (sessionRef == NULL || clientEventDataPtr->sessionRef == sessionRef))
        {
            LE_INFO("Removed proxy reference for service id: %" PRIu32 "", serviceId);
            le_hashmap_Remove(ProxyRefMapByMsgId, le_hashmap_GetKey(iter));
        }
    }

    le_ref_IterRef_t iterRef = le_ref_GetIterator(ClientEventDataSafeRefMap);

    while(le_ref_NextNode(iterRef) == LE_OK)
    {
        ClientEventData_t *clientEventDataPtr = le_ref_GetValue(iterRef);

        if ((clientEventDataPtr != NULL) &&
            (clientEventDataPtr->serviceId == serviceId) &&
            (sessionRef == NULL || clientEventDataPtr->sessionRef == sessionRef))
        {
            LE_INFO("Removed clientEventData for service id: %" PRIu32 "", serviceId);
            le_ref_DeleteRef(ClientEventDataSafeRefMap, (void*)le_ref_GetSafeRef(iterRef));
            le_mem_Release(clientEventDataPtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get new context pointer to send.
 *
 * When a reference pointer is seen in an IPC message, this function must be used to convert the
 * pointer in the IPC message to a new value that can be packed into the outgoing rpc message.
 *
 * @return
 *      -LE_OK if successfully converted the context pointer
 *      -LE_FAULT otherwise
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcEventHandler_RepackOutgoingContext
(
    le_pack_SemanticTag_t    tagId,           ///< [IN] Tag id that appear before context pointer
    void*                    contextPtr,      ///< [IN] Context pointer in ipc message
    void**                   contextPtrPtr,   ///< [OUT] New context pointer
    rpcProxy_Message_t*      proxyMessagePtr  ///< [IN] Pointer to outgoing proxy message
)
{
    rpcProxy_CommonHeader_t* commonHeader = &(proxyMessagePtr->commonHeader);
    ClientEventData_t *clientEventDataPtr;
    // Client sends request to add async event handler
    if((tagId == LE_PACK_CONTEXT_PTR_REFERENCE) && (commonHeader->type == RPC_PROXY_CLIENT_REQUEST))
    {
        // Retrieve Message reference from hash map, using the Proxy Message Id.
        // We need message reference to get client's session which we want to save
        // for future use.
        le_msg_MessageRef_t msgRef = rpcProxy_GetMsgRefById(commonHeader->id);
        if (msgRef == NULL)
        {
            LE_ERROR("Message reference not found");
            goto error;
        }

        // Save client's context and associated session reference
        clientEventDataPtr = le_mem_Alloc(ClientHandlerPoolRef);
        clientEventDataPtr->contextPtr = contextPtr;
        clientEventDataPtr->sessionRef = le_msg_GetSession(msgRef);
        clientEventDataPtr->handlerRef = NULL;
        clientEventDataPtr->serviceId = commonHeader->serviceId;
        // Save IPC msg ID and msg SIZE of corresponding removeHandler.
        clientEventDataPtr->ipcMsgId =
            ((CommonPayloadData_t*)le_msg_GetPayloadPtr(msgRef))->id + 1;
        clientEventDataPtr->ipcMsgSize = le_msg_GetMaxPayloadSize(msgRef);

        // Create safe reference for the new structure
        contextPtr = le_ref_CreateRef(ClientEventDataSafeRefMap, clientEventDataPtr);

        // Cache proxy reference to use later.
        // Store the proxy reference in a hash map using the proxy Id as the key.
        le_hashmap_Put(ProxyRefMapByMsgId, (void*)(uintptr_t) commonHeader->id, contextPtr);
    }
    // Client sends request to remove async event hsandler
    else if((tagId == LE_PACK_ASYNC_HANDLER_REFERENCE) && (commonHeader->type == RPC_PROXY_CLIENT_REQUEST))
    {
        clientEventDataPtr = le_ref_Lookup(ClientEventDataSafeRefMap, contextPtr);
        if(clientEventDataPtr == NULL)
        {
            LE_ERROR("Attempt to remove event handler for unknown client");
            goto error;
        }

        le_ref_DeleteRef(ClientEventDataSafeRefMap, contextPtr);
        contextPtr = clientEventDataPtr->handlerRef;
        le_mem_Release(clientEventDataPtr);
    }

    *contextPtrPtr = contextPtr;
    return LE_OK;

error:
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for repacking client context seen in an incoming PPC message.
 * This function converts the context pointer seen in the RPC message into a new value that can be
 * packed into IPC message.
 *
 * @return
 *      -LE_OK if successfully converted the context pointer
 *      -LE_FAULT otherwise
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcEventHandler_RepackIncomingContext
(
    le_pack_SemanticTag_t    tagId,           ///< [IN] Tag id that appear before context pointer
    void*                    contextPtr,      ///< [IN] Context pointer in in rpc message
    void**                   contextPtrPtr,   ///< [OUT] New context pointer
    rpcProxy_Message_t*      proxyMessagePtr  ///< [IN] Pointer to incoming proxy message
)
{
    ClientEventData_t *clientEventDataPtr;

    rpcProxy_CommonHeader_t* commonHeader = &(proxyMessagePtr->commonHeader);
    (void) contextPtrPtr;
    // Client receives response for request to add async event handler
    if((tagId == LE_PACK_ASYNC_HANDLER_REFERENCE) && (commonHeader->type == RPC_PROXY_SERVER_RESPONSE))
    {
        void* proxyRef = le_hashmap_Get(ProxyRefMapByMsgId, (void*)(uintptr_t) commonHeader->id);
        if(proxyRef == NULL)
        {
            LE_ERROR("Proxy reference not found");
            goto error;
        }

        // Delete proxy reference from hash map
        le_hashmap_Remove(ProxyRefMapByMsgId, (void*)(uintptr_t) commonHeader->id);


        clientEventDataPtr = le_ref_Lookup(ClientEventDataSafeRefMap, proxyRef);
        if(clientEventDataPtr == NULL)
        {
            LE_ERROR("Received response for unknown client");
            goto error;
        }

        // Async event handler was added successfully, save it for future use.
        if(contextPtr)
        {
            // Save server's handler reference
            clientEventDataPtr->handlerRef = contextPtr;

            // Pass proxy reference to client
            *contextPtrPtr = proxyRef;
        }
        // Async event handler wasn't added. Delete ClientEventData_t record.
        else
        {
            le_ref_DeleteRef(ClientEventDataSafeRefMap, proxyRef);
            le_mem_Release(clientEventDataPtr);
        }
    }

    // Client receives async event from server
    else if(commonHeader->type == RPC_PROXY_SERVER_ASYNC_EVENT)
    {
        // Find client's session.
        // It was stored when the client sent request to add async event handler
        clientEventDataPtr = le_ref_Lookup(ClientEventDataSafeRefMap, contextPtr);
        if(clientEventDataPtr == NULL)
        {
            LE_ERROR("Received async event for unknown client");
            goto error;
        }

        void *tempPtr = contextPtr;

        // Get back original client's context
        *contextPtrPtr = clientEventDataPtr->contextPtr;

        le_msg_SessionRef_t sessionRef = clientEventDataPtr->sessionRef;

        proxyMessagePtr->msgRef = le_msg_CreateMsg(sessionRef);

        // In case of "one-shot" callback delete the record.
        if(tagId == LE_PACK_ASYNC_HANDLER_REFERENCE)
        {
            le_ref_DeleteRef(ClientEventDataSafeRefMap, tempPtr);
            le_mem_Release(clientEventDataPtr);
        }
    }
    // in other cases the new reference is the same:
    else
    {
        *contextPtrPtr = contextPtr;
    }

    return LE_OK;

error:
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Processing Asynchronous Server Events on the client side
 *
 * @return
 *      -LE_OK if async message was process successfully
 *      -LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcEventHandler_ProcessEvent
(
    void* handle,                  ///< [IN] Opaque handle to the le_comm communication channel
    const char* systemName,        ///< [IN] Name of the system that sent the event
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the streaming State-Machine data
    void *proxyMessagePtr          ///< [IN] Pointer to the Proxy Message
)
{
    rpcProxy_Message_t* eventMsgPtr = (rpcProxy_Message_t*) proxyMessagePtr;
    // Sanity Check - Verify Message Type
    LE_ASSERT(eventMsgPtr->commonHeader.type == RPC_PROXY_SERVER_ASYNC_EVENT);

    le_result_t recvRes = rpcProxy_RecvStream(handle, streamStatePtr, eventMsgPtr);
    if (recvRes == LE_IN_PROGRESS)
    {
        // return now, come back later
        return LE_OK;
    }
    else if (recvRes != LE_OK)
    {
        LE_ERROR("Error when receiving an event stream from %s", systemName);
        return LE_FAULT;
    }

    // Send event to client
    LE_DEBUG("Sending event to client session %p", le_msg_GetSession(eventMsgPtr->msgRef));

    // Send response
    le_msg_Send(eventMsgPtr->msgRef);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Receiving Aynchronous Server Events on the server side.
 * These events were previously registered by client.
 */
//--------------------------------------------------------------------------------------------------
void rpcEventHandler_EventCallback
(
    le_msg_MessageRef_t eventMsgRef, ///< [IN] Event Message reference
    void*               contextPtr   ///< [IN] Context pointer
)
{
    rpcProxy_Message_t proxyMessage = {0};

    le_result_t         result;
    uint32_t            serviceId;
    const char*         systemName = NULL;

    // Get service ID from context pointer
    serviceId = (uint32_t)contextPtr;

    systemName = rpcProxy_GetSystemNameByServiceId(serviceId);
    if (systemName == NULL)
    {
        LE_ERROR("Unable to retrieve system name for service id %"PRIu32"", serviceId);
        goto end;
    }

    //
    // Build a Proxy Async Event Message
    //

    // Set the Proxy Message Id, Service Id, and type
    proxyMessage.commonHeader.id = 0;
    proxyMessage.commonHeader.serviceId = serviceId;
    proxyMessage.commonHeader.type = RPC_PROXY_SERVER_ASYNC_EVENT;

    proxyMessage.msgRef = eventMsgRef;

    // Send Proxy Message to the far-side RPC Proxy
    result = rpcProxy_SendMsg(systemName, &proxyMessage);
    if (result != LE_OK)
    {
        LE_ERROR("le_comm_Send failed, result %d", result);
    }

end:

    le_msg_ReleaseMsg(eventMsgRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function initializes and starts the RPC Proxy Event Handler Services.
 *
 * @note Must be called either directly, such as in the case of the RPC Proxy Library,
 *       or indirectly as a Legato component via the RPC Proxy's COMPONENT_INIT_ONCE.
 *
 */
//--------------------------------------------------------------------------------------------------
void rpcEventHandler_InitializeOnce
(
    void
)
{
    ClientHandlerPoolRef = le_mem_InitStaticPool(
                                        ClientHandlerPool,
                                        RPC_PROXY_ASYNC_EVENT_HANDLER_MAX_NUM,
                                        sizeof(ClientEventData_t));

    ClientEventDataSafeRefMap = le_ref_InitStaticMap(ClientEventDataSafeRefStaticMap,
                                        RPC_PROXY_ASYNC_EVENT_HANDLER_MAX_NUM);

    ProxyRefMapByMsgId = le_hashmap_InitStatic(ProxyRefHashMap,
                                               RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                                               le_hashmap_HashVoidPointer,
                                               le_hashmap_EqualsVoidPointer);

}
