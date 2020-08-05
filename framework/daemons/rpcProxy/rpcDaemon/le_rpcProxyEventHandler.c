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
}
ClientEventData_t;


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
 * Function for deleting all ClientEventData_t records for given service
 */
//--------------------------------------------------------------------------------------------------
void rpcEventHandler_DeleteAll
(
    /// [IN] Service ID
    uint32_t serviceId
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ClientEventDataSafeRefMap);

    while(le_ref_NextNode(iterRef) == LE_OK)
    {
        ClientEventData_t *clientEventDataPtr = le_ref_GetValue(iterRef);

        if(clientEventDataPtr->serviceId == serviceId)
        {
            le_ref_DeleteRef(ClientEventDataSafeRefMap, (void*)le_ref_GetSafeRef(iterRef));
            le_mem_Release(clientEventDataPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for repacking client context in certain cases related to adding async event
 * handler to server and handling these events.
 * Packing and unpacking is always done on client side
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcEventHandler_RepackContext
(
    /// [IN/OUT] Pointer to current position in the Message Buffer pointer
    uint8_t**                msgBufPtr,

    /// [IN/OUT] Pointer to the previous position in the Message Buffer pointer
    uint8_t**                previousMsgBufPtr,

    /// [IN/OUT] Pointer to the current position in the New Message Buffer pointer
    uint8_t**                newMsgBufPtr,

    ///< [IN] Pointer to commont header needed to get message type and id
    rpcProxy_CommonHeader_t* commonHeader,

    ///< [IN] Boolean to identify if message is in-coming or out-going
    bool                     sending,

    //< [OUT] Pointer to client's session reference
    le_msg_SessionRef_t *sessionRefPtr
)
{
    ClientEventData_t *clientEventDataPtr;
    void            *contextPtr;
    TagID_t          tagId = **msgBufPtr;

    // Extract contextPtr from source message
    if(!le_pack_UnpackReference( msgBufPtr, &contextPtr ))
    {
        LE_ERROR("Unpacking failure");
        goto error;
    }

    // Client sends request to add async event handler
    if((tagId == LE_PACK_CONTEXT_PTR_REFERENCE) && sending && (commonHeader->type == RPC_PROXY_CLIENT_REQUEST))
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

        // Create safe reference for the new structure
        contextPtr = le_ref_CreateRef(ClientEventDataSafeRefMap, clientEventDataPtr);

        // Cache proxy reference to use later.
        // Store the proxy reference in a hash map using the proxy Id as the key.
        le_hashmap_Put(ProxyRefMapByMsgId, (void*)(uintptr_t) commonHeader->id, contextPtr);
    }

    // Client receives response for request to add async event handler
    else if((tagId == LE_PACK_ASYNC_HANDLER_REFERENCE) && !sending && (commonHeader->type == RPC_PROXY_SERVER_RESPONSE))
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
            contextPtr = proxyRef;
        }
        // Async event handler wasn't added. Delete ClientEventData_t record.
        else
        {
            le_ref_DeleteRef(ClientEventDataSafeRefMap, proxyRef);
            le_mem_Release(clientEventDataPtr);
        }
    }

    // Client receives async event from server
    else if(!sending && (commonHeader->type == RPC_PROXY_SERVER_ASYNC_EVENT))
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
        contextPtr = clientEventDataPtr->contextPtr;

        LE_ASSERT(sessionRefPtr);
        *sessionRefPtr = clientEventDataPtr->sessionRef;

        // In case of "one-shot" callback delete the record.
        if(tagId == LE_PACK_ASYNC_HANDLER_REFERENCE)
        {
            le_ref_DeleteRef(ClientEventDataSafeRefMap, tempPtr);
            le_mem_Release(clientEventDataPtr);
        }
    }

    // Client sends request to remove async event hsandler
    else if((tagId == LE_PACK_ASYNC_HANDLER_REFERENCE) && sending && (commonHeader->type == RPC_PROXY_CLIENT_REQUEST))
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


    // Replace server reference by proxy reference in destiantiom message
    if(!le_pack_PackTaggedReference( newMsgBufPtr, contextPtr, tagId ))
    {
        LE_ERROR("Packing failure");
        goto error;
    }

    // Update the previous message buffer pointer to reflect what has been processed
    *previousMsgBufPtr = *msgBufPtr;
    return LE_OK;

error:
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Processing Asynchronous Server Events on the client side
 */
//--------------------------------------------------------------------------------------------------
void rpcEventHandler_ProcessEvent
(
    rpcProxy_Message_t *proxyMessagePtr, ///< [IN] Pointer to the Proxy Message
    le_msg_SessionRef_t sessionRef       ///< [IN] Client session reference
)
{
    le_msg_MessageRef_t msgRef;
    void* msgPtr;

    // Sanity Check - Verify Message Type
    LE_ASSERT(proxyMessagePtr->commonHeader.type == RPC_PROXY_SERVER_ASYNC_EVENT);

    LE_ASSERT(sessionRef);

    // Retrieve the Service reference, using the Service-ID
    le_msg_ServiceRef_t serviceRef = rpcProxy_GetServiceRefById(proxyMessagePtr->commonHeader.serviceId);

    if (serviceRef == NULL)
    {
        LE_INFO("Error retrieving Service Reference, service id [%" PRIu32 "]",
                proxyMessagePtr->commonHeader.serviceId);
        return;
    }

    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);

    if (le_msg_GetMaxPayloadSize(msgRef) < proxyMessagePtr->msgSize)
    {
        LE_ERROR("Message Reference buffer too small, "
                    "msgRef [%" PRIuS "], byteCount [%" PRIu16 "]",
                    le_msg_GetMaxPayloadSize(msgRef),
                    proxyMessagePtr->msgSize);
        return;
    }

    // Copy the message payload
    memcpy(msgPtr, proxyMessagePtr->message, proxyMessagePtr->msgSize);

    // Send event to client
    LE_DEBUG("Sending event to client session %p : %u bytes sent",
                le_msg_GetSession(msgRef),
                proxyMessagePtr->msgSize);

    // Send response
    le_msg_Send(msgRef);
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
    rpcProxy_Message_t proxyMessage;

    le_result_t         result;
    uint32_t            serviceId;
    const char*         systemName = NULL;

    // Get service ID from context pointer
    serviceId = (uint32_t)contextPtr;

    systemName = rpcProxy_GetSystemNameByServiceId(serviceId);
    if (systemName == NULL)
    {
        LE_ERROR("Unable to retrieve system name for service id %lu", serviceId);
        goto end;
    }

    //
    // Build a Proxy Async Event Message
    //

    // Set the Proxy Message Id, Service Id, and type
    proxyMessage.commonHeader.id = 0;
    proxyMessage.commonHeader.serviceId = serviceId;
    proxyMessage.commonHeader.type = RPC_PROXY_SERVER_ASYNC_EVENT;

    // Copy the message payload
    memcpy(proxyMessage.message,
           le_msg_GetPayloadPtr(eventMsgRef),
           le_msg_GetMaxPayloadSize(eventMsgRef));

    // Save the message payload size
    proxyMessage.msgSize = le_msg_GetMaxPayloadSize(eventMsgRef);

    // Send Proxy Message to the far-side RPC Proxy
    result = rpcProxy_SendMsg(systemName, &proxyMessage, NULL);
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
 *       or indirectly as a Legato component via the RPC Proxy's COMPONENT_INIT.
 *
 */
//--------------------------------------------------------------------------------------------------
void rpcEventHandler_Initialize
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
