/**
 * @file le_rpcProxy.c
 *
 * This file contains the source code for the RPC Proxy Service.
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#include "le_rpcProxy.h"
#include "le_rpcProxyNetwork.h"
#include "le_rpcProxyConfig.h"
#include "le_rpcProxyEventHandler.h"
#include "le_rpcProxyFileStream.h"

#ifndef RPC_PROXY_LOCAL_SERVICE
#include <dlfcn.h>
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Extern Declarations
 */
//--------------------------------------------------------------------------------------------------
#ifdef RPC_PROXY_UNIT_TEST
extern le_result_t rpcDaemonTest_ProcessClientRequest(rpcProxy_Message_t* proxyMessagePtr);
#endif


//--------------------------------------------------------------------------------------------------
//
// Forward Function Declarations
//
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessConnectServiceRequest(void* handle, const char* systemName,
                                                StreamState_t* streamStatePtr,
                                                void* proxyMessagePtr);
static le_result_t ProcessConnectServiceResponse(void* handle, const char* systemName,
                                                 StreamState_t* streamStatePtr,
                                                 void* proxyMessagePtr);
static le_result_t ProcessServerResponse(void* handle, const char* systemName,
                                         StreamState_t* streamStatePtr,
                                         void* proxyMessagePtr);
static le_result_t ProcessClientRequest(void* handle, const char* systemName,
                                        StreamState_t* streamStatePtr,
                                        void* proxyMessagePtr);
static le_result_t ProcessDisconnectService(void* handle, const char* systemName,
                                            StreamState_t* streamStatePtr,
                                            void* proxyMessagePtr);
#ifdef RPC_PROXY_UNIT_TEST
void ServerMsgRecvHandler(le_msg_MessageRef_t msgRef, void* contextPtr);
#else
static void ServerMsgRecvHandler(le_msg_MessageRef_t msgRef, void* contextPtr);
#endif
static le_result_t DoConnectService(const char* serviceName, const uint32_t serviceId,
                                    const char* protocolId);

static le_result_t PreProcessReceivedHeader(rpcProxy_CommonHeader_t * commonHeaderPtr);

#ifndef RPC_PROXY_LOCAL_SERVICE
static void SendDisconnectService(const char* systemName,
                                  const char* serviceInstanceName,
                                  const char* protocolIdStr);
#endif
static void SendSessionConnectRequest(const char* systemName,
                                      const char* serviceInstanceName,
                                      const char* protocolIdStr);

//--------------------------------------------------------------------------------------------------
/**
 * Array containing the mapping of next network-message receive states.
 */
//--------------------------------------------------------------------------------------------------
static const NetworkMessageReceiveState_t NetworkMessageNextRecvState[] =
{
    [NETWORK_MSG_IDLE]                         = NETWORK_MSG_PARTIAL_HEADER,
    [NETWORK_MSG_PARTIAL_HEADER]               = NETWORK_MSG_HEADER,
    [NETWORK_MSG_HEADER]                       = NETWORK_MSG_STREAM,
    // Note: Transition from STREAM state to DONE is handled by a secondary state machine that is
    // called from within process functions.
    [NETWORK_MSG_STREAM]                       = NETWORK_MSG_STREAM,
    [NETWORK_MSG_DONE]                         = NETWORK_MSG_IDLE,
};

//--------------------------------------------------------------------------------------------------
/**
 * Process functions for different message types:
 *
 * These functions are used in the NETWORK_MSG_STREAM state to receive and process a message body
 * after the header is received
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*process_fpt)(void*, const char*, StreamState_t* streamStatePtr, void*);

static const process_fpt NetworkMessageProcessFuncTable[RPC_PROXY_NUM_MESSAGE_TYPES] =
{
    [RPC_PROXY_CONNECT_SERVICE_REQUEST]  = ProcessConnectServiceRequest,
    [RPC_PROXY_CONNECT_SERVICE_RESPONSE] = ProcessConnectServiceResponse,
    [RPC_PROXY_DISCONNECT_SERVICE]       = ProcessDisconnectService,
    [RPC_PROXY_CLIENT_REQUEST]           = ProcessClientRequest,
    [RPC_PROXY_SERVER_RESPONSE]          = ProcessServerResponse,
    [RPC_PROXY_KEEPALIVE_REQUEST]        = rpcProxyNetwork_ProcessKeepAliveRequest,
    [RPC_PROXY_KEEPALIVE_RESPONSE]       = rpcProxyNetwork_ProcessKeepAliveResponse,
    [RPC_PROXY_SERVER_ASYNC_EVENT]       = rpcEventHandler_ProcessEvent,
    [RPC_PROXY_FILESTREAM_MESSAGE]       = rpcFStream_ProcessFileStreamMessage,
};

//--------------------------------------------------------------------------------------------------
/**
 * Global Message ID to uniquely identify each RPC Proxy Message.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t GlobalMsgId = 1;

//--------------------------------------------------------------------------------------------------
/**
 * Hash Map to store Proxy Message ID (Key) and Message Reference (value) mappings.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(MsgRefHashMap, RPC_PROXY_MSG_REFERENCE_MAX_NUM);
static le_hashmap_Ref_t MsgRefMapByProxyId = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Service-ID references.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(ServiceIDSafeRefStaticMap, RPC_PROXY_SERVICE_BINDINGS_MAX_NUM);
static le_ref_MapRef_t ServiceIDSafeRefMap = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Hash Map to store Service-Name (key) and Service-ID (value) mappings.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(ServiceIDHashMap, RPC_PROXY_MSG_REFERENCE_MAX_NUM);
static le_hashmap_Ref_t ServiceIDMapByName = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Hash Map to store Service-ID (key) and SessionRef (value) mappings.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(SessionRefHashMap, RPC_PROXY_MSG_REFERENCE_MAX_NUM);
static le_hashmap_Ref_t SessionRefMapByID = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Hash Map to store Service-ID (key) and ServiceRef (value) mappings.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(ServiceRefHashMap, RPC_PROXY_MSG_REFERENCE_MAX_NUM);
static le_hashmap_Ref_t ServiceRefMapByID = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Hash Map to store Proxy Message ID (key) and TimerRef (value) mappings.
 * NOTE: Maximum number of simultaneous timer-references is defined by the maximum number of
 * Message Reference and Keep-alive messages supported by the RPC Proxy.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(ExpiryTimerRefHashMap,
                         (RPC_PROXY_MSG_REFERENCE_MAX_NUM + RPC_PROXY_NETWORK_SYSTEM_MAX_NUM));
static le_hashmap_Ref_t ExpiryTimerRefByProxyId = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Hash Map to store Service-ID (key) and TimerRef (value) mappings.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(ExpiryTimerRefServiceIdHashMap, RPC_PROXY_SERVICE_BINDINGS_MAX_NUM);
static le_hashmap_Ref_t ExpiryTimerRefByServiceId = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Hash Map to store Request-Response Reference (key) and TimerRef (value) mappings.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(RequestResponseRefHashMap, RPC_PROXY_MSG_REFERENCE_MAX_NUM);
static le_hashmap_Ref_t RequestResponseRefByProxyId = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * This pool is used for the Service-Name string, which is used as a key in the
 * Serivce-ID hashmap.  Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ServiceNameStringPool,
                          RPC_PROXY_SERVICE_BINDINGS_MAX_NUM,
                          RPC_PROXY_MSG_SERVICE_NAME_SIZE);
static le_mem_PoolRef_t ServiceNameStringPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * This pool is used for the Service-ID, which is used as a value in a
 * hashmap.  Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ServiceIdPool,
                          RPC_PROXY_SERVICE_BINDINGS_MAX_NUM,
                          sizeof(uint32_t));
static le_mem_PoolRef_t ServiceIdPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * This pool is used to allocate memory for the Proxy Connect-Service Messages.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ProxyConnectServiceMessagePool,
                          RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                          sizeof(rpcProxy_ConnectServiceMessage_t));
static le_mem_PoolRef_t ProxyConnectServiceMessagesPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * This pool is used to allocate memory for the Proxy Client Request-Response Record.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ProxyClientRequestResponseRecordPool,
                          RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                          sizeof(rpcProxy_ClientRequestResponseRecord_t));
static le_mem_PoolRef_t ProxyClientRequestResponseRecordPoolRef = NULL;


#ifdef RPC_PROXY_LOCAL_SERVICE
//--------------------------------------------------------------------------------------------------
/**
 * Hash Map to store Service-Name (key) and Local-Messaging Queue ServiceRef (value) mappings.
 * Initialized in rpcProxy_COMPONENT_INIT_ONCE().
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(ServerRefHashMap, RPC_PROXY_SERVICE_BINDINGS_MAX_NUM);
static le_hashmap_Ref_t ServerRefMapByName = NULL;

#endif

//--------------------------------------------------------------------------------------------------
/**
 * This function gets reference to service using its id
 *
 * @return
 *      Reference to the service with given id or NULL if not found
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t rpcProxy_GetServiceRefById
(
    uint32_t serviceId  ///< [IN] Service ID
)
{
    return le_hashmap_Get(ServiceRefMapByID, (void*)(uintptr_t) serviceId);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets reference to message using proxy id
 *
 * @return
 *      Reference to the message with given proxy id or NULL if not found
 */
//--------------------------------------------------------------------------------------------------
le_msg_MessageRef_t rpcProxy_GetMsgRefById
(
    uint32_t proxyId    ///< [IN] Proxy ID
)
{
    return le_hashmap_Get(MsgRefMapByProxyId, (void*)(uintptr_t) proxyId);
}

//--------------------------------------------------------------------------------------------------
/**
 *  This function gets reference an ipc session using service ID
 *  @return
 *      reference to ipc session
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t rpcProxy_GetSessionRefById
(
    uint32_t serviceId  ///< [IN] Service ID
)
{
    return le_hashmap_Get(SessionRefMapByID, (void*)(uintptr_t) serviceId);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets system name using service id
 *
 * @return
 *      RPC system name or NULL if not found
 */
//--------------------------------------------------------------------------------------------------
const char* rpcProxy_GetSystemNameByServiceId
(
    uint32_t serviceId  ///< [IN] Service ID
)
{
    le_hashmap_It_Ref_t iter;
    const char*         serviceName = NULL;

    // Get service name by service ID
    iter = le_hashmap_GetIterator(ServiceIDMapByName);
    while (le_hashmap_NextNode(iter) == LE_OK)
    {
        uint32_t* nextVal = le_hashmap_GetValue(iter);

        if (*nextVal == serviceId)
        {
            serviceName = le_hashmap_GetKey(iter);
            break;
        }
    }

    if(serviceName == NULL)
    {
        LE_ERROR("Unable to retrieve service name for service ID %" PRIu32, serviceId);
        return NULL;
    }

    // Get system name by service name
    return rpcProxyConfig_GetSystemNameByServiceName(serviceName);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for checking if message has variable length
 *
 * @return
 *      TRUE if message has variable size
 */
//--------------------------------------------------------------------------------------------------
static inline bool IsVariableLengthType
(
    uint8_t type
)
{
    return  ((type == RPC_PROXY_CLIENT_REQUEST)  ||
             (type == RPC_PROXY_SERVER_RESPONSE) ||
             (type == RPC_PROXY_SERVER_ASYNC_EVENT) ||
             (type == RPC_PROXY_FILESTREAM_MESSAGE));
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for displaying a message type string
 */
//--------------------------------------------------------------------------------------------------
static inline char* DisplayMessageType
(
    uint32_t type
)
{
    switch (type)
    {
        case RPC_PROXY_CONNECT_SERVICE_REQUEST:
            return "Connect-Service-Request";
            break;

        case RPC_PROXY_CONNECT_SERVICE_RESPONSE:
            return "Connect-Service-Response";
            break;

        case RPC_PROXY_DISCONNECT_SERVICE:
            return "Disconnect-Service";
            break;

        case RPC_PROXY_KEEPALIVE_REQUEST:
            return "KEEPALIVE-Request";
            break;

        case RPC_PROXY_KEEPALIVE_RESPONSE:
            return "KEEPALIVE-Response";
            break;

        case RPC_PROXY_CLIENT_REQUEST:
            return "Client-Request";
            break;

        case RPC_PROXY_SERVER_RESPONSE:
            return "Server-Response";
            break;

        case RPC_PROXY_SERVER_ASYNC_EVENT:
            return "Server-Event";
            break;

        case RPC_PROXY_FILESTREAM_MESSAGE:
            return "File-Stream";
            break;

        default:
            return "Unknown";
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Generic function for generating Server-Response Error Messages
 * TODO: Error handling LE-15100
 */
//--------------------------------------------------------------------------------------------------
static void GenerateServerResponseErrorMessage
(
    rpcProxy_Message_t* proxyMessagePtr,
    le_result_t resultCode
)
{
    LE_UNUSED(resultCode);
    // Set Proxy Message size and type
    proxyMessagePtr->commonHeader.type = RPC_PROXY_SERVER_RESPONSE;
}

#if RPC_PROXY_LOCAL_SERVICE
static void DestroyLocalBuffers
(
    le_dls_List_t *bufferList
)
{
    while (1)
    {
        le_dls_Link_t *link = le_dls_Pop(bufferList);

        if (NULL == link)
        {
            break;
        }

        rpcProxy_LocalBuffer_t *localBufferPtr = CONTAINER_OF(link, rpcProxy_LocalBuffer_t, link);

        le_mem_Release(localBufferPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Destroy all local buffers assocated with a client request response
 */
//--------------------------------------------------------------------------------------------------
static void DestructRequestResponse
(
    void *itemPtr
)
{
    rpcProxy_ClientRequestResponseRecord_t *requestResponsePtr =
        (rpcProxy_ClientRequestResponseRecord_t*)itemPtr;

    DestroyLocalBuffers(&requestResponsePtr->localBuffers);
}
#endif


//--------------------------------------------------------------------------------------------------
/**
 * timer expiery handler for client request message
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_ClientRequestTimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    uint32_t proxyMsgId = (uint32_t) le_timer_GetContextPtr(timerRef);

    LE_WARN("Client-Request has timed out, proxy id [%" PRIu32 "];", proxyMsgId);

    // Retrieve Message Reference from hash map, using the Proxy Message Id
    le_msg_MessageRef_t msgRef = le_hashmap_Get(MsgRefMapByProxyId,
                                                (void*)(uintptr_t) proxyMsgId);

    if (msgRef == NULL)
    {
        LE_ERROR("Unable to retrieve Message Reference for timedout proxy message,"
                "proxy id [%" PRIu32 "]", proxyMsgId);
    }
    else
    {
        le_msg_CloseSession(le_msg_GetSession(msgRef));
    }

    // Remove entries from hash-map
    le_hashmap_Remove(MsgRefMapByProxyId, (void*)(uintptr_t) proxyMsgId);
    le_hashmap_Remove(ExpiryTimerRefByProxyId, (void*)(uintptr_t) proxyMsgId);
    le_timer_Delete(timerRef);
    timerRef = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Generic Expired Proxy Message Timers
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_ConnectServiceTimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    // Retrieve the Connect-Service-Request message from the timer context
    rpcProxy_ConnectServiceMessage_t* proxyMessagePtr =
        (rpcProxy_ConnectServiceMessage_t*) le_timer_GetContextPtr(timerRef);

    if (proxyMessagePtr == NULL)
    {
        LE_ERROR("Unable to retrieve copy of the "
                 "Proxy Connect-Service Message Reference");
    }
    else
    {
        LE_WARN("%s timer expired; Re-trigger "
                "connect-service request '%s', service-id [%" PRIu32 "]",
                DisplayMessageType(proxyMessagePtr->commonHeader.type),
                proxyMessagePtr->serviceName,
                proxyMessagePtr->commonHeader.serviceId);

        // Re-trigger connect-service-request to the remote system
        le_result_t result =
            rpcProxy_SendMsg(proxyMessagePtr->systemName, proxyMessagePtr);

        if (result == LE_OK)
        {
            // Re-start the timer
            le_timer_Start(timerRef);
            goto end;
        }
        else
        {
            LE_ERROR("le_comm_Send failed, result %d", result);
        }
    }
    // Delete Timer
    le_timer_Delete(timerRef);
    timerRef = NULL;

end:
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the Expiry Timer Hash-map reference.
 */
//--------------------------------------------------------------------------------------------------
le_hashmap_Ref_t rpcProxy_GetExpiryTimerRefByProxyId
(
    void
)
{
    return ExpiryTimerRefByProxyId;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the Service-ID Hash-map reference.
 */
//--------------------------------------------------------------------------------------------------
le_hashmap_Ref_t rpcProxy_GetServiceIDMapByName
(
    void
)
{
    return ServiceIDMapByName;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for generating unique Proxy Message IDs
 */
//--------------------------------------------------------------------------------------------------
uint32_t rpcProxy_GenerateProxyMessageId
(
    void
)
{
    // Proxy Message ID Generator
    // Monotonically increase proxy message ID to ensure each
    // client request is uniquely identified
    GlobalMsgId++;

    return GlobalMsgId;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for sending Proxy Messages to the far side via the le_comm API
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxy_SendMsg
(
    const char* systemName, ///< [IN] Name of the system message is being sent to
    void* messagePtr ///< [IN] Void pointer to the message buffer
)
{
    le_result_t         result;
    size_t              byteCount;
    void               *sendMessagePtr;

    // Retrieve the Network Record for this system
    NetworkRecord_t* networkRecordPtr =
        le_hashmap_Get(rpcProxyNetwork_GetNetworkRecordHashMapByName(), systemName);

    if (networkRecordPtr == NULL)
    {
        LE_ERROR("Unable to retrieve Network Record, "
                 "system-name [%s] - unknown system", systemName);
        return LE_FAULT;
    }

    // Verify the state of the Network Connection
    if (networkRecordPtr->state == NETWORK_DOWN)
    {
        LE_INFO("Network Status: DOWN, system [%s], handle [%d] - ignore send message request",
                systemName,
                le_comm_GetId(networkRecordPtr->handle));

        return LE_COMM_ERROR;
    }

    //
    // Prepare the Proxy Message for sending
    //

    // Set a pointer to the common message header
    rpcProxy_CommonHeader_t *commonHeaderPtr = (rpcProxy_CommonHeader_t*) messagePtr;

    switch (commonHeaderPtr->type)
    {
        case RPC_PROXY_CONNECT_SERVICE_REQUEST:
        case RPC_PROXY_CONNECT_SERVICE_RESPONSE:
        case RPC_PROXY_DISCONNECT_SERVICE:
        {
            // Set a pointer to the message
            rpcProxy_ConnectServiceMessage_t* proxyMsgPtr =
                (rpcProxy_ConnectServiceMessage_t*) messagePtr;

            // Calculate the total size
            byteCount = sizeof(rpcProxy_ConnectServiceMessage_t);

            // Prepare the Proxy Message Common Header
            commonHeaderPtr->id = htobe32(commonHeaderPtr->id);
            commonHeaderPtr->serviceId = htobe32(commonHeaderPtr->serviceId);

            // Prepare the service-code field
            proxyMsgPtr->serviceCode = htobe32(proxyMsgPtr->serviceCode);

            // Set send pointer to the message pointer
            sendMessagePtr = messagePtr;
            break;
        }

        case RPC_PROXY_KEEPALIVE_REQUEST:
        case RPC_PROXY_KEEPALIVE_RESPONSE:
        {
            // Calculate the total size
            byteCount = sizeof(rpcProxy_KeepAliveMessage_t);

            // Prepare the Proxy Message Common Header
            commonHeaderPtr->id = htobe32(commonHeaderPtr->id);
            commonHeaderPtr->serviceId = htobe32(commonHeaderPtr->serviceId);

            // Set send pointer to the message pointer
            sendMessagePtr = messagePtr;
            break;
        }
        case RPC_PROXY_FILESTREAM_MESSAGE:
        case RPC_PROXY_CLIENT_REQUEST:
        case RPC_PROXY_SERVER_RESPONSE:
        case RPC_PROXY_SERVER_ASYNC_EVENT:
        {
            // For these messages, the header and body are sent out separately.
            byteCount = RPC_PROXY_COMMON_HEADER_SIZE;

            // Prepare the Proxy Message Common Header
            commonHeaderPtr->id = htobe32(commonHeaderPtr->id);
            commonHeaderPtr->serviceId = htobe32(commonHeaderPtr->serviceId);

            // Set send pointer
            sendMessagePtr = messagePtr;
            break;
        }

        default:
        {
            LE_ERROR("Unexpected Proxy Message, type [0x%x]", commonHeaderPtr->type);
            return LE_FORMAT_ERROR;
            break;
        }
    } // End of switch-statement

    LE_DEBUG("Sending %s Proxy Message, service-id [%" PRIu32 "], "
             "proxy id [%" PRIu32 "], size [%" PRIuS "]",
             DisplayMessageType(commonHeaderPtr->type),
             be32toh(commonHeaderPtr->serviceId),
             be32toh(commonHeaderPtr->id),
             byteCount);

    // Send the Message Payload as an outgoing Proxy Message to the far-size RPC Proxy
    result = le_comm_Send(networkRecordPtr->handle, sendMessagePtr, byteCount);
    if (result != LE_OK)
    {
        // Delete the Network Communication Channel
        rpcProxyNetwork_DeleteNetworkCommunicationChannel(systemName);
    }

    // Prepare the Proxy Message Common Header
    commonHeaderPtr->id = be32toh(commonHeaderPtr->id);
    commonHeaderPtr->serviceId = be32toh(commonHeaderPtr->serviceId);

    if (result == LE_OK && IsVariableLengthType(commonHeaderPtr->type))
    {
        //now send the message body for variable length messages:
        result = rpcProxy_SendVariableLengthMsgBody(networkRecordPtr->handle, messagePtr);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for advancing the Receive Message state to the next state in the state-machine
 */
//--------------------------------------------------------------------------------------------------
static void AdvanceRecvMsgState
(
    NetworkMessageState_t* msgStatePtr
)
{
    // Retrieve the next network-message receive state
    msgStatePtr->recvState = NetworkMessageNextRecvState[msgStatePtr->recvState];
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for receiving Proxy Messages from the far side via the le_comm API
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RecvRpcMsg
(
    void* handle, ///< [IN] Opaque handle to the le_comm communication channel
    const char* systemName, ///< [IN] Name of the system that sent the message
    NetworkMessageState_t* msgStatePtr ///< [IN] Pointer to the Message State-Machine data
)
{
    le_result_t  result;
    //
    // Message Receive State Machine, loop is exited once there is no more data to be received.
    //
    while (true)
    {
        if (msgStatePtr->recvState == NETWORK_MSG_IDLE) // IDLE State
        {
            // Read the entire Common Header, if available
            msgStatePtr->expectedSize = RPC_PROXY_COMMON_HEADER_SIZE;
            msgStatePtr->recvSize = 0;
            msgStatePtr->offSet = 0;
            msgStatePtr->type = 0;
        }
        else if (msgStatePtr->recvState == NETWORK_MSG_PARTIAL_HEADER) // Partial HEADER State
        {
            // Calculate the amount of data to be read (expected minus recv)
            size_t remainingData = (msgStatePtr->expectedSize - msgStatePtr->recvSize);
            size_t receivedSize = remainingData;

            result = le_comm_Receive(handle,
                                     msgStatePtr->buffer + msgStatePtr->offSet, &receivedSize);
            if (result != LE_OK || receivedSize > remainingData)
            {
                return LE_COMM_ERROR;
            }
            // Adjust offSet to reflect data read into buffer
            msgStatePtr->offSet += receivedSize;
            if (receivedSize < remainingData)
            {
                // Partial header received (Incomplete RPC Message)
                // Cache the amount of data that has been received
                msgStatePtr->recvSize += receivedSize;
                // Return and wait until more data arrives
                return LE_OK;
            }
        }
        else if (msgStatePtr->recvState == NETWORK_MSG_HEADER) // HEADER State
        {
            // Set a pointer to the common message header
            rpcProxy_CommonHeader_t *commonHeaderPtr =
                (rpcProxy_CommonHeader_t*) msgStatePtr->buffer;

#if RPC_PROXY_HEX_DUMP
            LE_INFO("RPC Proxy received this header from: %s", systemName);
            LE_LOG_DUMP(LE_LOG_INFO, (const unsigned char*)commonHeaderPtr,
                        RPC_PROXY_COMMON_HEADER_SIZE);
#endif
            if (PreProcessReceivedHeader(commonHeaderPtr) == LE_OK)
            {
                msgStatePtr->type = commonHeaderPtr->type;
                LE_DEBUG("Initializing stream state to receive a %s message from %s",
                        DisplayMessageType(commonHeaderPtr->type), systemName);
                void* messageBuffer = msgStatePtr->buffer;
                if (rpcProxy_InitializeStreamState(&(msgStatePtr->streamState),
                                                   messageBuffer) != LE_OK)
                {
                    LE_ERROR("Failed to initialize stream state to receive a %s message from %s",
                            DisplayMessageType(commonHeaderPtr->type), systemName);
                    return LE_COMM_ERROR;
                }
            }
            else
            {
                LE_ERROR("Failed to process rpc message header received from %s", systemName);
                return LE_COMM_ERROR;
            }
        }
        else if (msgStatePtr->recvState == NETWORK_MSG_STREAM)
        {
            // call the appropriate process function.
            StreamState_t* streamStatePtr = &(msgStatePtr->streamState);
            void* messageBuffer = msgStatePtr->buffer;
            if (NetworkMessageProcessFuncTable[msgStatePtr->type](handle, systemName, streamStatePtr,
                    messageBuffer) != LE_OK)
            {
                LE_ERROR("Error happened when processing a proxy message from %s", systemName);
                return LE_COMM_ERROR;
            }
            break;
        }
        // Increment recv state to the next COMPLETE State
        AdvanceRecvMsgState(msgStatePtr);
    } // While-loop

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for pre-processing the Proxy Message header before receiving the message payload
 *
 *  @return:
 *      - LE_OK if header was successfully processed.
 *      - LE_FAULT if header format or content was not expected.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t PreProcessReceivedHeader
(
    rpcProxy_CommonHeader_t *commonHeaderPtr
)
{
    if (commonHeaderPtr->type > 0 && commonHeaderPtr->type < RPC_PROXY_NUM_MESSAGE_TYPES)
    {
        commonHeaderPtr->id = be32toh(commonHeaderPtr->id);
        commonHeaderPtr->serviceId = be32toh(commonHeaderPtr->serviceId);
        return LE_OK;
    }
    else
    {
        LE_ERROR("Unexpected Proxy Message type [0x%x]", commonHeaderPtr->type);
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Processing Server Responses
 *
 * @return
 *      - LE_OK if server response message was processed successfully.
 *      - LE_FAULT if an error happened.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessServerResponse
(
    void* handle,                  ///< [IN] Opaque handle to the le_comm communication channel
    const char* systemName,        ///< [IN] Name of the system that sent the message
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Message State-Machine data
    void *proxyMessagePtr          ///< [IN] Pointer to the Proxy Message
)
{
    rpcProxy_Message_t* serverResponseMsgPtr = (rpcProxy_Message_t*) proxyMessagePtr;
    // Sanity Check - Verify Message Type
    LE_ASSERT(serverResponseMsgPtr->commonHeader.type == RPC_PROXY_SERVER_RESPONSE);

    le_msg_MessageRef_t msgRef = serverResponseMsgPtr->msgRef;
    // Check if a client response is required
    if (le_msg_NeedsResponse(msgRef))
    {
        le_result_t recvRes = rpcProxy_RecvStream(handle, streamStatePtr, serverResponseMsgPtr);
        if (recvRes == LE_IN_PROGRESS)
        {
            // return now, come back later
            return LE_OK;
        }
        else if (recvRes != LE_OK)
        {
            LE_ERROR("Error when receiving a server response stream from %s", systemName);
            return LE_FAULT;
        }
        // At this point, we're done receiving the message:
        // Clean up the timer
        // Retrieve and delete timer associated with Proxy Message ID
        le_timer_Ref_t timerRef =
            le_hashmap_Get(
                ExpiryTimerRefByProxyId,
                (void*)(uintptr_t) serverResponseMsgPtr->commonHeader.id);

        if (timerRef != NULL)
        {
            LE_DEBUG("Deleting timer for Client-Request, "
                     "service-id [%" PRIu32 "], id [%" PRIu32 "]",
                     serverResponseMsgPtr->commonHeader.serviceId,
                     serverResponseMsgPtr->commonHeader.id);

            // Remove timer entry associated with Proxy Message ID from hash-map
            le_hashmap_Remove(
                ExpiryTimerRefByProxyId,
                (void*)(uintptr_t) serverResponseMsgPtr->commonHeader.id);

            // Delete Timer
            le_timer_Delete(timerRef);
            timerRef = NULL;
        }
        else
        {
            LE_ERROR("Unable to find Timer record, proxy id [%" PRIu32 "]",
                     serverResponseMsgPtr->commonHeader.id);
        }


        if(rpcFStream_HandleStreamId(msgRef, &(serverResponseMsgPtr->metaData),
                                     serverResponseMsgPtr->commonHeader.serviceId,
                                     systemName) != LE_OK)
        {
            LE_ERROR("Error in handling proxy message file stream id");
        }
        // Return the response
        LE_DEBUG("Sending response to client session %p", le_msg_GetSession(msgRef));

        // Send response
        le_msg_Respond(msgRef);

    } else {

        LE_DEBUG("Client response not required, session %p",
                 le_msg_GetSession(msgRef));
    }

#ifdef RPC_PROXY_LOCAL_SERVICE
    // Clean-up Local Message memory allocation associated with this Proxy Message ID
    rpcProxy_CleanUpLocalMessageResources(serverResponseMsgPtr->commonHeader.id);
#endif

    // Delete Message Reference from hash map
    le_hashmap_Remove(MsgRefMapByProxyId, (void*)(uintptr_t) serverResponseMsgPtr->commonHeader.id);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Receiving Aynchronous Server Responses, resulting from a Request-Response call
 */
//--------------------------------------------------------------------------------------------------
static void ServerResponseCompletionCallback
(
    le_msg_MessageRef_t responseMsgRef, ///< [IN] Response Message reference
    void*               contextPtr ///< [IN] Context pointer
)
{
    rpcProxy_Message_t proxyMessage = {0};
    rpcProxy_ClientRequestResponseRecord_t* requestResponsePtr = NULL;
    le_result_t result;

    LE_DEBUG("Received message from server");

    // Retrieve the Request-Response Reference pointer,
    // using the Proxy Message ID (stored in contextPtr)
    requestResponsePtr = le_hashmap_Get(RequestResponseRefByProxyId, contextPtr);
    if (requestResponsePtr == NULL)
    {
        LE_WARN("Matching Request-Response Record Reference not found, "
                "proxy id [%" PRIuPTR "] - Dropping packet",
                (uintptr_t) contextPtr);

        // Release the message object before returning.
        le_msg_ReleaseMsg(responseMsgRef);
        return;
    }

    // Sanity Check - Verify Message Type
    if (requestResponsePtr->commonHeader.type != RPC_PROXY_SERVER_RESPONSE)
    {
        LE_ERROR("Unexpected Proxy Message, type [0x%x]",
                 requestResponsePtr->commonHeader.type);
        goto exit;
    }

    //
    // Build a Proxy Server-Response Message
    //

    // Set the Proxy Message Id, Service Id, and type
    proxyMessage.commonHeader.id = requestResponsePtr->commonHeader.id;
    proxyMessage.commonHeader.serviceId = requestResponsePtr->commonHeader.serviceId;
    proxyMessage.commonHeader.type = RPC_PROXY_SERVER_RESPONSE;

    if (rpcFStream_HandleFileDescriptor(responseMsgRef, &(proxyMessage.metaData),
                                        requestResponsePtr->commonHeader.serviceId,
                                        requestResponsePtr->systemName) != LE_OK)
    {
        LE_ERROR("Error in handling file descriptor in the ipc message");
        // we're still sending the main message to the other side but fd will be -1.
    }

    proxyMessage.msgRef = responseMsgRef;
    // Send a request to the server and get the response.
    LE_DEBUG("Sending response back to RPC Proxy");

    // Send Proxy Message to the far-side RPC Proxy
    result = rpcProxy_SendMsg(requestResponsePtr->systemName, &proxyMessage);
    if (result != LE_OK)
    {
        LE_ERROR("le_comm_Send failed, result %d", result);
        rpcFStream_DeleteOurStream(proxyMessage.metaData.fileStreamId, requestResponsePtr->systemName);
    }


exit:
    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(responseMsgRef);

    // Remove entry from hash-map, using the Proxy Message Id
    le_hashmap_Remove(RequestResponseRefByProxyId,
                      (void*)(uintptr_t) requestResponsePtr->commonHeader.id);

    // Free the memory allocated for the request-response record
    le_mem_Release(requestResponsePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Processing Client Requests arriving from far-side RPC Proxy
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessClientRequest
(
    void* handle,                  ///< [IN] Opaque handle to the le_comm communication channel
    const char* systemName,        ///< [IN] Name of the system that sent the Client-Request
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Message State-Machine data
    void* proxyMessagePtr          ///< [IN] Pointer to the Proxy Message
)
{
    rpcProxy_Message_t* clientRequestMsgPtr = (rpcProxy_Message_t*) proxyMessagePtr;
    // Send Client Message to the Server
#ifndef RPC_PROXY_UNIT_TEST
    // Sanity Check - Verify Message Type
    LE_ASSERT(clientRequestMsgPtr->commonHeader.type == RPC_PROXY_CLIENT_REQUEST);

    le_result_t recvRes = rpcProxy_RecvStream(handle, streamStatePtr, clientRequestMsgPtr);
    if (recvRes == LE_IN_PROGRESS)
    {
        // return now, come back later
        return LE_OK;
    }
    else if (recvRes != LE_OK)
    {
        LE_ERROR("Error when receiving a client request stream");
        DestroyLocalBuffers(&streamStatePtr->localBuffers);
        return LE_FAULT;
    }

    le_msg_MessageRef_t msgRef = clientRequestMsgPtr->msgRef;

    if(rpcFStream_HandleStreamId(msgRef, &(clientRequestMsgPtr->metaData),
                                 clientRequestMsgPtr->commonHeader.serviceId, systemName) != LE_OK)
    {
        LE_ERROR("Error in handling proxy message file stream id");
    }

    LE_DEBUG("Sending message to server and waiting for response");

    LE_DEBUG("Allocating memory for Request-Response Record, "
             "service-id [%" PRIu32 "], proxy id [%" PRIu32 "]",
             clientRequestMsgPtr->commonHeader.serviceId,
             clientRequestMsgPtr->commonHeader.id);

#ifdef LE_CONFIG_DEBUG
    le_mem_PoolStats_t poolStats;

    le_mem_GetStats(ProxyClientRequestResponseRecordPoolRef, &poolStats);
    LE_DEBUG("Request-Response memory pool size = [%" PRIuS "]",
             poolStats.numFree);
#endif

    // Allocate memory for a Request-Response record
    rpcProxy_ClientRequestResponseRecord_t* requestResponsePtr =
        le_mem_TryAlloc(ProxyClientRequestResponseRecordPoolRef);

    if (requestResponsePtr == NULL)
    {
        LE_WARN("Request-Response Record memory pool is exhausted, "
                "service-id [%" PRIu32 "], proxy id [%" PRIu32 "] - "
                "Dropping request and returning error",
                clientRequestMsgPtr->commonHeader.serviceId,
                clientRequestMsgPtr->commonHeader.id);

        // Clean up local buffers
        DestroyLocalBuffers(&streamStatePtr->localBuffers);

        //
        // Generate a LE_NO_MEMORY event
        //

        // Generate LE_NO_MEMORY Server-Response
        GenerateServerResponseErrorMessage(
            clientRequestMsgPtr,
            LE_NO_MEMORY);

        // Send the Response to the far-side
        le_result_t result = rpcProxy_SendMsg(systemName, clientRequestMsgPtr);
        if (result != LE_OK)
        {
            LE_ERROR("le_comm_Send failed, result %d", result);
        }

        return LE_NO_MEMORY;
    }

    //
    // Build the Request-Response record
    //

    // Set the Proxy Message ID, Service-Id, and type
    requestResponsePtr->commonHeader.id = clientRequestMsgPtr->commonHeader.id;
    requestResponsePtr->commonHeader.serviceId = clientRequestMsgPtr->commonHeader.serviceId;
    requestResponsePtr->commonHeader.type = RPC_PROXY_SERVER_RESPONSE;
    requestResponsePtr->localBuffers = streamStatePtr->localBuffers;
    streamStatePtr->localBuffers = LE_DLS_LIST_INIT;

    // Copy the Source of the request (system-name) so we know who to send the response back to
    le_utf8_Copy(requestResponsePtr->systemName,
                 systemName,
                 sizeof(requestResponsePtr->systemName), NULL);

    // Send an asynchronous request-response to the server.
    le_msg_RequestResponse(msgRef, &ServerResponseCompletionCallback,
                           (void*)(uintptr_t) clientRequestMsgPtr->commonHeader.id);

    // Store the Request-Response Record Ptr in a hashmap,
    // using the Proxy Message ID as a key, so that it can be retrieved later by either
    // requestResponseTimerExpiryHandler() or ServerResponseCompletionCallback()
    le_hashmap_Put(RequestResponseRefByProxyId,
                   (void*)(uintptr_t) clientRequestMsgPtr->commonHeader.id,
                   requestResponsePtr);
#else
    // Evaluate Unit-Test results
    rpcDaemonTest_ProcessClientRequest(clientRequestMsgPtr);
#endif

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete and clean-up the Client-Request timers for given service id and client session.
 * If specified client session is NULL , deleting all Client-Request timers of the given service.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteClientRequestTimer
(
    /// [IN] Service ID
    uint32_t serviceId,
    /// [IN] Client session reference
    le_msg_SessionRef_t sessionRef
)
{
    // Create iterator to traverse ExpiryTimerRefByProxyId map
    le_hashmap_It_Ref_t iter = le_hashmap_GetIterator(rpcProxy_GetExpiryTimerRefByProxyId());

    // Traverse the entire ExpiryTimerRefByProxyId map
    while (le_hashmap_NextNode(iter) == LE_OK)
    {
        // Get the timer reference
        le_timer_Ref_t timerRef =
            (le_timer_Ref_t) le_hashmap_GetValue(iter);

        uint32_t proxyMsgId = (uint32_t) le_timer_GetContextPtr(timerRef);

        // Retrieve Message Reference from hash map, using the Proxy Message Id
        le_msg_MessageRef_t msgRef = le_hashmap_Get(MsgRefMapByProxyId,
                                                    (void*)(uintptr_t) proxyMsgId);

        if ((sessionRef == NULL) ||(le_msg_GetSession(msgRef) == sessionRef))
        {
            // Free the IPC msgRef
            if (msgRef)
            {
                LE_INFO("Free msgRef for service id [%" PRIuPTR "]  sessionRef [%p] ",
                        (uintptr_t)serviceId, sessionRef);
                le_msg_ReleaseMsg(msgRef);

                // Remove the msgRef from hash-map
                le_hashmap_Remove(MsgRefMapByProxyId, (void*)(uintptr_t) proxyMsgId);
            }
            else
            {
                LE_ERROR("Expiry timer but msgRef = NULL for service id [%" PRIuPTR "]"
                         " sessionRef %p",
                         (uintptr_t)serviceId, sessionRef);
            }

            // Remove the timer entry from hash-map
            le_hashmap_Remove(ExpiryTimerRefByProxyId, (void*)(uintptr_t) proxyMsgId);

            // Delete Timer
            le_timer_Delete(timerRef);
            timerRef = NULL;
        }
    }
}

#ifdef RPC_PROXY_LOCAL_SERVICE
//--------------------------------------------------------------------------------------------------
/**
 * Retrieve and remove the next parameter buffer for a request response call
 */
//--------------------------------------------------------------------------------------------------
le_dls_Link_t *rpcProxy_PopNextParameter
(
    uint32_t serviceId
)
{
    rpcProxy_ClientRequestResponseRecord_t* requestResponsePtr = NULL;

    // Retrieve the Request-Response Reference pointer,
    // using the Proxy Message ID (stored in contextPtr)
    requestResponsePtr = le_hashmap_Get(RequestResponseRefByProxyId, (void *)(uintptr_t)serviceId);
    if (requestResponsePtr == NULL)
    {
        return NULL;
    }

    return le_dls_Pop(&requestResponsePtr->localBuffers);
}
#endif

#ifndef RPC_PROXY_LOCAL_SERVICE
//--------------------------------------------------------------------------------------------------
/**
 * Handler for client session closes for clients that use the block/unblock API.
 */
//--------------------------------------------------------------------------------------------------
static void ServerCloseSessionHandler
(
    le_msg_SessionRef_t sessionRef, ///< [IN] Session reference
    void*               contextPtr ///< [IN] Context pointer to identify the service
)
{
    // Confirm context pointer is valid
    if (contextPtr == NULL)
    {
        LE_ERROR("contextPtr is NULL");
        return;
    }

    if ( sessionRef == NULL )
    {
        LE_ERROR("sessionRef is NULL");
        return;
    }

    // Retrieve the context data pointer
    rpcProxy_ExternServer_t* contextDataPtr = (rpcProxy_ExternServer_t*) contextPtr;

    // Retrieve the system-name and service-name from the context data pointer
    const char* serviceName = contextDataPtr->serviceName;
    const char* systemName =
        rpcProxyConfig_GetSystemNameByServiceName(contextDataPtr->serviceName);

    if (systemName == NULL)
    {
        LE_ERROR("Unable to retrieve system-name for service-name '%s'",
                 contextDataPtr->serviceName);
        return;
    }

    // Retrieve the Service-ID, using the service-name
    uint32_t* serviceIdCopyPtr =
        le_hashmap_Get(ServiceIDMapByName, serviceName);

    if (serviceIdCopyPtr != NULL)
    {
        // Free all ClientEventData_t records for given service id and client session
        rpcEventHandler_DeleteAll(*serviceIdCopyPtr, sessionRef);

        // Free all Client-Request timers for given service id and client session
        DeleteClientRequestTimer(*serviceIdCopyPtr, sessionRef);
    }

    LE_INFO("Client session %p closed, service '%s', system '%s'",
            sessionRef, serviceName, systemName);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for client session closes for clients that use the block/unblock API.
 */
//--------------------------------------------------------------------------------------------------
static void ServerOpenSessionHandler
(
    le_msg_SessionRef_t sessionRef, ///< [IN] Session reference
    void*               contextPtr ///< [IN] Context pointer to identify the service
)
{
    // Confirm context pointer is valid
    if (contextPtr == NULL)
    {
        LE_ERROR("contextPtr is NULL");
        return;
    }

    if ( sessionRef == NULL )
    {
        LE_ERROR("sessionRef is NULL");
        return;
    }

    // Retrieve the context data pointer
    rpcProxy_ExternServer_t* contextDataPtr = (rpcProxy_ExternServer_t*) contextPtr;

    // Retrieve the system-name and service-name from the context data pointer
    const char* serviceName = contextDataPtr->serviceName;
    const char* systemName =
        rpcProxyConfig_GetSystemNameByServiceName(contextDataPtr->serviceName);

    if (systemName == NULL)
    {
        LE_ERROR("Unable to retrieve system-name for service-name '%s'",
                 contextDataPtr->serviceName);
        return;
    }

    LE_INFO("Client session %p opened, service '%s', system '%s'",
            sessionRef, serviceName, systemName);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for service closures.
 */
//--------------------------------------------------------------------------------------------------
static void ClientServiceCloseHandler
(
    le_msg_SessionRef_t sessionRef, ///< [IN] Session reference
    void*               contextPtr ///< [IN] Context pointer
)
{
    if ( sessionRef == NULL )
    {
        LE_ERROR("sessionRef is NULL");
        return;
    }

    LE_INFO("Service %p closed", sessionRef);

    rpcProxy_ExternClient_t* bindingRefPtr = (rpcProxy_ExternClient_t*) contextPtr;

    // Retrieve the system-name for the specified service-name
    const char* systemName =
        rpcProxyConfig_GetSystemNameByServiceName(bindingRefPtr->serviceName);

    if (systemName == NULL)
    {
        LE_ERROR("Unable to retrieve system-name for service-name '%s'",
                 bindingRefPtr->serviceName);
        return;
    }

    // Retrieve the Service-ID, using the service-name
    uint32_t* serviceIdCopyPtr = le_hashmap_Get(ServiceIDMapByName, bindingRefPtr->serviceName);
    if (serviceIdCopyPtr != NULL)
    {
        // Retrieve the Session reference, using the Service-ID
        sessionRef = le_hashmap_Get(SessionRefMapByID, (void*)(uintptr_t) *serviceIdCopyPtr);
        if (sessionRef != NULL)
        {
            LE_INFO("======= Stopping client for '%s' service =======", bindingRefPtr->serviceName);
            // Remove sessionRef from hash-map
            le_hashmap_Remove(SessionRefMapByID, (void*)(uintptr_t) *serviceIdCopyPtr);
        }

        // Traverse the RequestResponseRefByProxyId map
        le_hashmap_It_Ref_t iter = le_hashmap_GetIterator(RequestResponseRefByProxyId);

        while (le_hashmap_NextNode(iter) == LE_OK)
        {
            rpcProxy_ClientRequestResponseRecord_t* requestResponsePtr =
              (rpcProxy_ClientRequestResponseRecord_t*) le_hashmap_GetValue(iter);

            if (requestResponsePtr != NULL)
            {
                // Check if the serviceId of the request-response
                if (requestResponsePtr->commonHeader.serviceId == *serviceIdCopyPtr)
                {
                    LE_INFO("==== Cleaning up Request-Response record for service Id [%" PRIu32 "]",
                            requestResponsePtr->commonHeader.serviceId);
#ifdef RPC_PROXY_LOCAL_SERVICE
                    // Clean-up Local Message memory allocation associated with the Proxy Message ID
                    CleanUpLocalMessageResources(requestResponsePtr->commonHeader.id);
#endif
                    // Remove entry from hash-map, using the Proxy Message Id
                    le_hashmap_Remove(RequestResponseRefByProxyId,
                                      (void*)(uintptr_t) requestResponsePtr->commonHeader.id);

                    // Free Proxy Message Copy Memory
                    le_mem_Release(requestResponsePtr);
                }
            }
        }

        // Generate a Disconnect-Service event to inform the far-side that this
        // service is no longer available
        SendDisconnectService(
            systemName,
            bindingRefPtr->serviceName,
            bindingRefPtr->protocolIdStr);

        // Get the stored key object
        char* serviceNameCopyPtr =
            le_hashmap_GetStoredKey(
                ServiceIDMapByName,
                bindingRefPtr->serviceName);

        if (serviceNameCopyPtr != NULL)
        {
            // Remove the serviceId in a hashmap, using the service-name as a key
            le_hashmap_Remove(ServiceIDMapByName, serviceNameCopyPtr);

            // Free the memory allocated for the Service Name string
            le_mem_Release(serviceNameCopyPtr);
        }

        // Free the memory allocated for the Service-ID
        le_mem_Release(serviceIdCopyPtr);
    }
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Delete and clean-up the Connect-Service-Request timer.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DeleteConnectServiceRequestTimer
(
    uint32_t serviceId
)
{
    // Retrieve and delete timer associated with Proxy Message ID
    le_timer_Ref_t timerRef =
        le_hashmap_Get(
            ExpiryTimerRefByServiceId,
            (void*)(uintptr_t) serviceId);

    if (timerRef == NULL)
    {
        LE_INFO("Unable to find Timer record, serivce-id [%" PRIu32 "]", serviceId);
        return LE_FAULT;
    }

    LE_DEBUG("Deleting timer for Connect-Service Request, "
             "service-id [%" PRIu32 "]",
             serviceId);

    // Retrieve the Connect-Service-Request message from the timer context
    rpcProxy_ConnectServiceMessage_t* proxyMessagePtr =
        (rpcProxy_ConnectServiceMessage_t*) le_timer_GetContextPtr(timerRef);

    // Release the connect service proxy message
    le_mem_Release(proxyMessagePtr);

    // Remove timer entry associated with Service-ID from hash-map
    le_hashmap_Remove(ExpiryTimerRefByServiceId, (void*)(uintptr_t) serviceId);

    // Delete Timer
    le_timer_Delete(timerRef);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Processing Connect-Service Response
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessConnectServiceResponse
(
    void* handle,                  ///< [IN] Opaque handle to the le_comm communication channel
    const char* systemName,        ///< [IN] Name of the system that sent the message
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Message State-Machine data
    void* proxyMessagePtr          ///< [IN] Pointer to the Proxy Message
)
{
    rpcProxy_ConnectServiceMessage_t* connectServiceMsgPtr =
        (rpcProxy_ConnectServiceMessage_t*) proxyMessagePtr;

    // Sanity Check - Verify Message Type
    LE_ASSERT(connectServiceMsgPtr->commonHeader.type == RPC_PROXY_CONNECT_SERVICE_RESPONSE);

    le_result_t recvRes = rpcProxy_RecvStream(handle, streamStatePtr, connectServiceMsgPtr);
    if (recvRes == LE_IN_PROGRESS)
    {
        // return now, come back later
        return LE_OK;
    }
    else if (recvRes != LE_OK)
    {
        LE_ERROR("Error when receiving a connect service response message from %s", systemName);
        return LE_FAULT;
    }

    // Now that the message is fully received, preprocess this message before continuing:
    connectServiceMsgPtr->serviceCode = be32toh(connectServiceMsgPtr->serviceCode);

    // Check if service has been established successfully on the far-side
    if (connectServiceMsgPtr->serviceCode != LE_OK)
    {
        // Remote-side failed to set-up service
        LE_INFO("%s failed, serviceId "
                "[%" PRIu32 "], service-code [%" PRIi32 "] - retry later",
                DisplayMessageType(connectServiceMsgPtr->commonHeader.type),
                connectServiceMsgPtr->commonHeader.serviceId,
                connectServiceMsgPtr->serviceCode);

        /*
         * The return value of this functions being something other than LE_OK means there was a
         * problem when receiving the message during processing of the message, and causes the
         * connection being teared down which affects all services on that connection.
         * On the other hand, connectServiceMsgPtr->serviceCode being non LE_OK simply means the
         * other side could not setup the service, this can happen if the server app that RPCclient
         * was supposed to connect to hadn't started yet.
         * Because at this point, we've received and processed the message successfully we should
         * return LE_OK. It's just the message content says there was a problem with this service on
         * the other side. We obviously won't proceed with this service anymore and return now but
         * as far as the caller, receive state machine, is concerned our result should be LE_OK.
         */
        return LE_OK;
    }

    // Delete and clean-up the Connect-Service-Request timer
    DeleteConnectServiceRequestTimer(connectServiceMsgPtr->commonHeader.serviceId);

    // Traverse all Service Reference entries in the Service Reference array and
    // search for matching service-name
    for (uint32_t index = 0; rpcProxyConfig_GetServerReferenceArray(index); index++)
    {
        const rpcProxy_ExternServer_t* serviceRefPtr = NULL;

        // Set a pointer to the Service Reference element
        serviceRefPtr = rpcProxyConfig_GetServerReferenceArray(index);

        // Retrieve the system-name for the specified service-name
        const char* systemName =
            rpcProxyConfig_GetSystemNameByServiceName(serviceRefPtr->serviceName);

        if (systemName == NULL)
        {
            LE_ERROR("Unable to retrieve system-name for service-name '%s'",
                     serviceRefPtr->serviceName);
            return LE_FAULT;
        }

        // Retrieve the service-name for the specified remote service-name
        const char* serviceName =
            rpcProxyConfig_GetServiceNameByRemoteServiceName(connectServiceMsgPtr->serviceName);

        if (serviceName == NULL)
        {
            LE_ERROR("Unable to retrieve service-name for remote service-name '%s'",
                     connectServiceMsgPtr->serviceName);
            return LE_FAULT;
        }

        // Compare the system-name, remote service-name, and protocol-Id-str
        if ((strcmp(systemName, connectServiceMsgPtr->systemName) == 0) &&
            (strcmp(serviceRefPtr->serviceName, serviceName) == 0) &&
            (strcmp(serviceRefPtr->protocolIdStr, connectServiceMsgPtr->protocolIdStr) == 0))
        {
            le_msg_ServiceRef_t serviceRef = NULL;

            // Check to see if service is already advertised (UP)
            // Retrieve the Service reference, using the Service-ID
            serviceRef =
                le_hashmap_Get(
                    ServiceRefMapByID,
                    (void*)(uintptr_t) connectServiceMsgPtr->commonHeader.serviceId);

            if (serviceRef == NULL)
            {
                LE_INFO("======= Advertise Server '%s' ========", serviceRefPtr->serviceName);

#ifndef RPC_PROXY_LOCAL_SERVICE
                le_msg_ProtocolRef_t protocolRef =
                    le_msg_GetProtocolRef(serviceRefPtr->protocolIdStr,
                                          (serviceRefPtr->messageSize + sizeof(uint32_t)));

                if (protocolRef == NULL)
                {
                    LE_ERROR("Could not get protocol reference for '%s' service, '%s' protocol",
                             serviceRefPtr->serviceName,
                             serviceRefPtr->protocolIdStr);
                    return LE_FAULT;
                }

                rpcProxy_ExternLinuxServer_t* refPtr =
                    CONTAINER_OF(serviceRefPtr, rpcProxy_ExternLinuxServer_t, common);

                // Create the Service
                serviceRef = le_msg_CreateService(protocolRef, refPtr->localServiceInstanceName);
                if (serviceRef == NULL)
                {
                    LE_ERROR("Could not create service for '%s' service",
                             refPtr->localServiceInstanceName);
                    return LE_FAULT;
                }

                // Add a handler for client session close
                le_msg_AddServiceCloseHandler(serviceRef,
                                              ServerCloseSessionHandler,
                                              (void*) serviceRefPtr);

                // Add a handler for client session open
                le_msg_AddServiceOpenHandler(serviceRef,
                                             ServerOpenSessionHandler,
                                             (void*) serviceRefPtr);
#else
                // Retrieve the Service reference, using the Service-Name
                serviceRef = le_hashmap_Get(ServerRefMapByName, serviceName);
                if (serviceRef == NULL)
                {
                    LE_ERROR("Unable to retrieve server-reference for '%s' service",
                             serviceRefPtr->serviceName);
                    return LE_FAULT;
                }
#endif

                // Start the server side of the service
                le_msg_SetServiceRecvHandler(serviceRef, ServerMsgRecvHandler, (void *)serviceRefPtr);
                le_msg_AdvertiseService(serviceRef);

            }

            // Allocate memory from Service Name string pool to hold the service-name
            char* serviceNameCopyPtr = le_mem_Alloc(ServiceNameStringPoolRef);

            // Copy the service-name string
            le_utf8_Copy(serviceNameCopyPtr,
                         serviceRefPtr->serviceName,
                         LIMIT_MAX_IPC_INTERFACE_NAME_BYTES, NULL);

            // Allocate memory from Service-ID pool
            uint32_t* serviceIdCopyPtr = le_mem_Alloc(ServiceIdPoolRef);

            // Copy the Service-ID
            *serviceIdCopyPtr = connectServiceMsgPtr->commonHeader.serviceId;

            // Store the Service-ID in a hashmap, using the service-name as a key
            le_hashmap_Put(ServiceIDMapByName, serviceNameCopyPtr, serviceIdCopyPtr);

            LE_INFO("Successfully saved Service Reference ID, "
                    "service-name [%s], service-id [%" PRIu32 "]",
                    serviceNameCopyPtr,
                    *serviceIdCopyPtr);

            // Store the serviceRef in a hashmap, using the Service-ID as a key
            le_hashmap_Put(ServiceRefMapByID,
                           (void*)(uintptr_t) connectServiceMsgPtr->commonHeader.serviceId,
                           serviceRef);

            LE_INFO("Successfully saved Service Reference, "
                    "service safe reference [%" PRIuPTR "], service-id [%" PRIu32 "]",
                    (uintptr_t) serviceRef,
                    connectServiceMsgPtr->commonHeader.serviceId);
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Processing Session Connect Request
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessConnectServiceRequest
(
    void* handle,                  ///< [IN] Opaque handle to the le_comm communication channel
    const char* systemName,        ///< [IN] Name of the system that sent the message
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Message State-Machine data
    void* proxyMessagePtr          ///< [IN] Pointer to the Proxy Message
)
{
    rpcProxy_ConnectServiceMessage_t* connectServiceMsgPtr =
        (rpcProxy_ConnectServiceMessage_t*) proxyMessagePtr;
    le_result_t  result;

    // Sanity Check - Verify Message Type
    LE_ASSERT(connectServiceMsgPtr->commonHeader.type == RPC_PROXY_CONNECT_SERVICE_REQUEST);

    le_result_t recvRes = rpcProxy_RecvStream(handle, streamStatePtr, connectServiceMsgPtr);
    if (recvRes == LE_IN_PROGRESS)
    {
        // return now, come back later
        return LE_OK;
    }
    else if (recvRes != LE_OK)
    {
        LE_ERROR("Error when receiving a process connect service request from %s", systemName);
        return LE_FAULT;
    }

    // Now that the message is fully received, preprocess this message before continuing:
    connectServiceMsgPtr->serviceCode = be32toh(connectServiceMsgPtr->serviceCode);

    LE_INFO("======= Starting RPC Proxy client for '%s' service, '%s' protocol ========",
            connectServiceMsgPtr->serviceName, connectServiceMsgPtr->protocolIdStr);

    // Generate Do-Connect-Service call on behalf of the remote client
    result = DoConnectService(connectServiceMsgPtr->serviceName,
                              connectServiceMsgPtr->commonHeader.serviceId,
                              connectServiceMsgPtr->protocolIdStr);
    //
    // Prepare a Session-Connect-Response Proxy Message
    //

    // Set the Proxy Message type to CONNECT_SERVICE_RESPONSE
    connectServiceMsgPtr->commonHeader.type = RPC_PROXY_CONNECT_SERVICE_RESPONSE;

    // Set the service-code with the DoConnectService result-code
    connectServiceMsgPtr->serviceCode = result;

    // Send Proxy Message to far-side
    result = rpcProxy_SendMsg(systemName, connectServiceMsgPtr);
    if (result != LE_OK)
    {
        LE_ERROR("le_comm_Send failed, result %d", result);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Deleting all timers associated with Service, using the service-name
 */
//--------------------------------------------------------------------------------------------------
static void DeleteAllTimers
(
    const char* serviceName  ///< Name of service being deleted
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ServiceIDSafeRefMap);

    // Iterate over all Service-ID Safe References looking for the service-name match
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        char* namePtr = (char*)le_ref_GetValue(iterRef);
        uint32_t serviceId = (uint32_t)(uintptr_t)le_ref_GetSafeRef(iterRef);

        if ((namePtr != NULL) &&
            (serviceName != NULL) &&
            (strcmp(namePtr, serviceName) == 0))
        {
            LE_INFO("Clean-up timers for Service ID Safe Reference, "
                    "service-name [%s], service-id [%" PRIu32 "]",
                    serviceName, serviceId);

            // Delete the connect-service-request timer if exists.
            DeleteConnectServiceRequestTimer(serviceId);
            // Delete the client-request timers if exists.
            DeleteClientRequestTimer(serviceId, NULL);

            return;
        }
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Deleting a Service, using the service-name
 */
//--------------------------------------------------------------------------------------------------
static void DeleteService
(
    const char* serviceName  ///< Name of service being deleted
)
{
    // Retrieve the Service-ID, using the service-name
    uint32_t* serviceIdCopyPtr =
        le_hashmap_Get(ServiceIDMapByName, serviceName);

    if (serviceIdCopyPtr != NULL)
    {
        // Retrieve the Session reference, using the Service-ID
        le_msg_ServiceRef_t serviceRef =
            le_hashmap_Get(ServiceRefMapByID, (void*)(uintptr_t) *serviceIdCopyPtr);

        if (serviceRef != NULL)
        {
            LE_INFO("======= Stopping service '%s' ========", serviceName);

#ifndef RPC_PROXY_LOCAL_SERVICE
            // Delete the server side of the service
            le_msg_DeleteService(serviceRef);
#endif
            // Delete data structures allocated to handle async events
            rpcEventHandler_DeleteAll(*serviceIdCopyPtr, NULL);

            // Remove sessionRef from hash-map
            le_hashmap_Remove(ServiceRefMapByID, (void*)(uintptr_t) *serviceIdCopyPtr);

            LE_INFO("======= Service '%s' stopped ========", serviceName);
        }

        // Free the memory allocated for the Service-ID
        le_mem_Release(serviceIdCopyPtr);
    }
    else
    {
        LE_INFO("Unable to retrieve service-Id for service '%s'",
                serviceName);
    }

    // Get the stored key object
    char* serviceNameCopyPtr =
        le_hashmap_GetStoredKey(ServiceIDMapByName, serviceName);

    if (serviceNameCopyPtr != NULL)
    {
        // Remove the Service-ID in a hashmap, using the service-name as a key
        le_hashmap_Remove(ServiceIDMapByName, serviceNameCopyPtr);

        LE_INFO("Successfully removed Service ID Reference, service-name [%s]",
                serviceNameCopyPtr);

        // Free the memory allocated for the Service Name string
        le_mem_Release(serviceNameCopyPtr);
    }
    else
    {
        LE_INFO("Unable to retrieve service-name object key "
                "from hashmap record, service '%s'",
                serviceName);
    }

    // Retrieve the remote service-name for the specified service-name
    const char* remoteServiceNamePtr =
        rpcProxyConfig_GetRemoteServiceNameByServiceName(serviceName);

    //
    // Clean-up Service ID Safe Reference for this service-name, if it exists
    //
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ServiceIDSafeRefMap);

    // Iterate over all Service-ID Safe References looking for the service-name match
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        char* namePtr = (char*)le_ref_GetValue(iterRef);
        uint32_t serviceId = (uint32_t)(uintptr_t)le_ref_GetSafeRef(iterRef);

        if ((namePtr != NULL) &&
            (remoteServiceNamePtr != NULL) &&
            (strcmp(namePtr , remoteServiceNamePtr) == 0))
        {
            LE_INFO("Releasing Service ID Safe Reference, "
                    "service-name [%s], service-id [%" PRIu32 "]",
                    namePtr, serviceId);

            // Free the Service-ID Safe Reference now that the Service is being deleted
            le_ref_DeleteRef(ServiceIDSafeRefMap, (void*) (uintptr_t)serviceId);
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Processing Disconnect Service Request
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessDisconnectService
(
    void* handle,                  ///< [IN] Opaque handle to the le_comm communication channel
    const char* systemName,        ///< [IN] Name of the system that sent the message
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Message State-Machine data
    void* proxyMessagePtr          ///< [IN] Pointer to the Proxy Message
)
{
    rpcProxy_ConnectServiceMessage_t* disconnectServiceMsgPtr =
        (rpcProxy_ConnectServiceMessage_t*) proxyMessagePtr;
    // Sanity Check - Verify Message Type
    LE_ASSERT(disconnectServiceMsgPtr->commonHeader.type == RPC_PROXY_DISCONNECT_SERVICE);

    le_result_t recvRes = rpcProxy_RecvStream(handle, streamStatePtr, disconnectServiceMsgPtr);
    if (recvRes == LE_IN_PROGRESS)
    {
        // return now, come back later
        return LE_OK;
    }
    else if (recvRes != LE_OK)
    {
        LE_ERROR("Error when receiving a client request stream");
        return LE_FAULT;
    }

    // Now that the message is fully received, preprocess this message before continuing:
    disconnectServiceMsgPtr->serviceCode = be32toh(disconnectServiceMsgPtr->serviceCode);

    LE_INFO("======= Stopping RPC Proxy server for '%s' service, '%s' protocol ========",
            disconnectServiceMsgPtr->serviceName, disconnectServiceMsgPtr->protocolIdStr);

    const char* serviceName =
        rpcProxyConfig_GetServiceNameByRemoteServiceName(disconnectServiceMsgPtr->serviceName);

    const char* remoteServiceName =
        rpcProxyConfig_GetRemoteServiceNameByServiceName(serviceName);

    if (serviceName == NULL)
    {
        LE_ERROR("Unable to retrieve service-name for remote service-name '%s'",
                 disconnectServiceMsgPtr->serviceName);
        return LE_FAULT;
    }

    // delete all rpc file stream instances:
    rpcFStream_DeleteStreamsByServiceId(disconnectServiceMsgPtr->commonHeader.serviceId);

    // Delete all timers associated with the service-name
    DeleteAllTimers(disconnectServiceMsgPtr->serviceName);

    // Delete the Service associated with the service-name
    DeleteService(serviceName);

    // Send Connect-Service Message to the far-side for the specified service-name
    // and wait for a valid Connect-Service response before advertising the service
    SendSessionConnectRequest(
        systemName,
        remoteServiceName,
        disconnectServiceMsgPtr->protocolIdStr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Receive Handler Callback Function for RPC Communication
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_AsyncRecvHandler
(
    void* handle, ///< [IN] Opaque handle to the le_comm communication channel
    short events ///< [IN] Event bit-mask
)
{
    char*                    systemName;
    le_result_t              result;
    NetworkMessageState_t*   msgStatePtr = NULL;
    // Retrieve the system-name from where this message has been sent
    // by a reverse look-up, using the handle
    systemName = rpcProxyNetwork_GetSystemNameByHandle(handle);
    if (systemName == NULL)
    {
        LE_ERROR("Unable to retrieve system-name, handle [%d] - unknown system",
                 le_comm_GetId(handle));
        return;
    }

    if (events & POLLIN)
    {
        //
        // Data waiting to be read
        //

        // Retrieve the Network Record Message State-Machine, using the handle
        NetworkRecord_t* networkRecordPtr =
            rpcProxyNetwork_GetNetworkRecordByHandle(handle);

        if (networkRecordPtr == NULL)
        {
            LE_ERROR("Unknown Network Record");
            return;
        }
        msgStatePtr = &(networkRecordPtr->messageState);
        // Receive Proxy Message Header from far-side
        result = RecvRpcMsg(handle, systemName, msgStatePtr);

        if (result != LE_OK)
        {
            LE_ERROR("Receiving proxy message from %s failed, result %d", systemName, result);

            // Delete the Network Communication Channel, using the communication handle
            rpcProxyNetwork_DeleteNetworkCommunicationChannelByHandle(handle);
        }
    }
    else if (events & (POLLRDHUP | POLLHUP | POLLERR))
    {
        //
        // Remote-side has hung up and no more data to be read
        //
        LE_INFO("Communication to the remote-side has been lost, "
                "handle [%" PRIiPTR "], events [0x%x]",
                (intptr_t) handle,
                events);

        // Delete the Network Communication Channel, using the communication handle
        rpcProxyNetwork_DeleteNetworkCommunicationChannelByHandle(handle);
    }
    else
    {
        LE_ERROR("Unknown POLL event, handle [%" PRIiPTR "], events [0x%x]",
                 (intptr_t) handle,
                 events);
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to Receive Client Service Messages and generate RPC Proxy Client-Request Messages
 */
//--------------------------------------------------------------------------------------------------
#ifdef RPC_PROXY_UNIT_TEST
void ServerMsgRecvHandler
#else
static void ServerMsgRecvHandler
#endif
(
    /// [IN] Client message reference
    le_msg_MessageRef_t msgRef,
    /// [IN] Context pointer to identify the service for this message
    void*               contextPtr
)
{
    le_result_t         result;
    bool                send = true;

    // Confirm context pointer is valid
    if (contextPtr == NULL)
    {
        LE_ERROR("contextPtr is NULL");
        return;
    }

    // Retrieve the context data pointer
    const rpcProxy_ExternServer_t* contextDataPtr = (rpcProxy_ExternServer_t*) contextPtr;

    // Retrieve the system-name and service-name from the context data pointer
    const char* serviceName = contextDataPtr->serviceName;
    const char* systemName =
        rpcProxyConfig_GetSystemNameByServiceName(contextDataPtr->serviceName);

    if (systemName == NULL)
    {
        LE_ERROR("Unable to retrieve system-name for service-name '%s'",
                 contextDataPtr->serviceName);
        return;
    }

    //
    // Prepare a Client-Request Proxy Message
    //

    // Allocate memory for a Proxy Message buffer
    rpcProxy_Message_t proxyMessage = {0};

    LE_DEBUG("Received message from client");

    // Set the Proxy Message common header id and type
    proxyMessage.commonHeader.id = rpcProxy_GenerateProxyMessageId();
    proxyMessage.commonHeader.type = RPC_PROXY_CLIENT_REQUEST;

    // Cache Message Reference to use later.
    // Store the Message Reference in a hash map using the proxy Id as the key.
    le_hashmap_Put(MsgRefMapByProxyId,
                   (void*)(uintptr_t) proxyMessage.commonHeader.id,
                   msgRef);

    // Retrieve the Service-ID for the specified service-name
    uint32_t* serviceIdPtr = le_hashmap_Get(ServiceIDMapByName, serviceName);
    if (serviceIdPtr == NULL)
    {
        // Raise a warning message
        LE_WARN("Service is not available, service-name [%s]", serviceName);
        proxyMessage.commonHeader.serviceId = 0;

        // Service is not available - do not send message to far-side
        send = false;
    }
    else
    {
        proxyMessage.commonHeader.serviceId = *serviceIdPtr;
    }

    // Check if message should be sent to the far-side
    if (!send)
    {
        goto exit;
    }


    if (rpcFStream_HandleFileDescriptor(msgRef, &(proxyMessage.metaData), *serviceIdPtr, systemName) != LE_OK)
    {
        LE_ERROR("Error in handling file descriptor in the ipc message");
        // we're still sending the main message to the other side but fd will be -1.
    }

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to '%s' RPC Proxy and waiting for response", systemName);

    proxyMessage.msgRef = msgRef;

    // Send Proxy Message HEADER to far-side
    result = rpcProxy_SendMsg(systemName, &proxyMessage);
    if (result != LE_OK)
    {
        LE_ERROR("le_comm_Send failed, result %d", result);
        rpcFStream_DeleteOurStream(proxyMessage.metaData.fileStreamId, systemName);
    }

exit:
    // Check if client requires a response
    if (le_msg_NeedsResponse(msgRef))
    {
        //
        // Client requires a response - Set-up a timer in the event
        // we do not hear back from the far-side RPC Proxy
        //
        le_timer_Ref_t clientRequestTimerRef;
        le_clk_Time_t  timerInterval = {.sec=RPC_PROXY_CLIENT_REQUEST_TIMER_INTERVAL, .usec=0 };

        // Create a timer to handle "lost" requests
        clientRequestTimerRef = le_timer_Create("Client-Request timer");
        le_timer_SetInterval(clientRequestTimerRef, timerInterval);
        le_timer_SetHandler(clientRequestTimerRef, rpcProxy_ClientRequestTimerExpiryHandler);
        le_timer_SetWakeup(clientRequestTimerRef, false);


        le_timer_SetContextPtr(clientRequestTimerRef,
                               (void*)(uintptr_t) proxyMessage.commonHeader.id);

        // Start timer
        le_timer_Start(clientRequestTimerRef);

        // Store the timerRef in a hashmap, using the Proxy Message ID as a key, so that
        // it can be retrieved later
        le_hashmap_Put(ExpiryTimerRefByProxyId,
                       (void*)(uintptr_t) proxyMessage.commonHeader.id,
                       clientRequestTimerRef);

        LE_DEBUG("Starting timer (%d secs.) for Client-Request, "
                 "service-name [%s], id [%" PRIu32 "]",
                 RPC_PROXY_CLIENT_REQUEST_TIMER_INTERVAL,
                 serviceName,
                 proxyMessage.commonHeader.id);
    }
}

#ifndef RPC_PROXY_LOCAL_SERVICE
//--------------------------------------------------------------------------------------------------
/**
 * Function for Generating a Disconnect Service event
 */
//--------------------------------------------------------------------------------------------------
static void SendDisconnectService
(
    const char* systemName,  ///< [IN] Name of the system
    const char* serviceInstanceName, ///< [IN] Name of the service instance
    const char* protocolIdStr ///< [IN] Protocol ID str
)
{
    rpcProxy_ConnectServiceMessage_t  proxyMessage;
    le_result_t result;

    // Create a Disconnect-Service Request Message
    proxyMessage.commonHeader.id = rpcProxy_GenerateProxyMessageId();
    proxyMessage.commonHeader.type = RPC_PROXY_DISCONNECT_SERVICE;

    // Retrieve the Service-ID for the specified service-name
    uint32_t* serviceIdPtr = le_hashmap_Get(ServiceIDMapByName, serviceInstanceName);
    if (serviceIdPtr == NULL)
    {
        LE_ERROR("Service is not available, remote service-name [%s]", serviceInstanceName);
        return;
    }

    // tear down all file stream instances associated with this service id:
    rpcFStream_DeleteStreamsByServiceId(*serviceIdPtr);

    // Set the Service-ID
    proxyMessage.commonHeader.serviceId = *serviceIdPtr;

    // Copy the system-name into the Proxy Connect-Service Message
    le_utf8_Copy(proxyMessage.systemName,
                 systemName,
                 sizeof(proxyMessage.systemName),
                 NULL);

    // Copy the Service-Name into the Proxy Connect-Service Message
    le_utf8_Copy(proxyMessage.serviceName,
                 serviceInstanceName,
                 sizeof(proxyMessage.serviceName),
                 NULL);

    // Copy the Protocol-ID-Str into the Proxy Connect-Service Message
    le_utf8_Copy(proxyMessage.protocolIdStr,
                 protocolIdStr,
                 sizeof(proxyMessage.protocolIdStr),
                 NULL);

    // Initialize the service-code to LE_OK
    proxyMessage.serviceCode = LE_OK;

    LE_INFO("Sending %s Proxy Message, service-id [%" PRIu32 "], service-name [%s]",
            DisplayMessageType(proxyMessage.commonHeader.type),
            proxyMessage.commonHeader.serviceId,
            proxyMessage.serviceName);

    // Send Proxy Message to far-side
    result = rpcProxy_SendMsg(systemName, &proxyMessage);
    if (result != LE_OK)
    {
        LE_ERROR("le_comm_Send failed, result %d", result);
    }
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Function for Generating Session Connect
 */
//--------------------------------------------------------------------------------------------------
static void SendSessionConnectRequest
(
    const char* systemName,  ///< [IN] Name of the system
    const char* serviceInstanceName, ///< [IN] Name of the service instance
    const char* protocolIdStr ///< [IN] Protocol ID str
)
{
    uint32_t     serviceId;
    le_result_t  result;

    // Checking if the same RPC Connect Request for the specified service is already sent.
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ServiceIDSafeRefMap);

    // Iterate over all Service-ID Safe References looking for the service-name match
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        char* namePtr = (char*)le_ref_GetValue(iterRef);
        uint32_t serviceId = (uint32_t)(uintptr_t)le_ref_GetSafeRef(iterRef);

        if ((namePtr != NULL) &&
            (serviceInstanceName != NULL) &&
            (strcmp(namePtr, serviceInstanceName) == 0))
        {
            LE_INFO("Found existing Service ID Safe Reference, "
                    "service-name [%s], service-id [%" PRIu32 "]",
                    namePtr, serviceId);
            return;
        }
    }

    //
    // Create a Session Connect Proxy Message
    //

    // Allocate memory for a Proxy Message
    rpcProxy_ConnectServiceMessage_t *proxyMessagePtr =
        le_mem_Alloc(ProxyConnectServiceMessagesPoolRef);

    // Generate the proxy message id
    proxyMessagePtr->commonHeader.id = rpcProxy_GenerateProxyMessageId();

    // Generate the Service-ID, using a safe reference
    proxyMessagePtr->commonHeader.serviceId = serviceId =
       (uint32_t)(uintptr_t) le_ref_CreateRef(ServiceIDSafeRefMap, (void*) serviceInstanceName);

    proxyMessagePtr->commonHeader.type = RPC_PROXY_CONNECT_SERVICE_REQUEST;

    // Copy the system-name into the Proxy Connect-Service Message
    le_utf8_Copy(proxyMessagePtr->systemName,
                 systemName,
                 sizeof(proxyMessagePtr->systemName),
                 NULL);

    // Copy the Service-Name into the Proxy Connect-Service Message
    le_utf8_Copy(proxyMessagePtr->serviceName,
                 serviceInstanceName,
                 sizeof(proxyMessagePtr->serviceName),
                 NULL);

    // Copy the Protocol-ID-Str into the Proxy Connect-Service Message
    le_utf8_Copy(proxyMessagePtr->protocolIdStr,
                 protocolIdStr,
                 sizeof(proxyMessagePtr->protocolIdStr),
                 NULL);

    // Initialize the service-code to LE_OK
    proxyMessagePtr->serviceCode = LE_OK;

    // Send Proxy Message to far-side
    result = rpcProxy_SendMsg(systemName, proxyMessagePtr);
    if (result != LE_OK)
    {
        LE_ERROR("le_comm_Send failed, result %d", result);

        // Free the Service-ID Safe Reference now that the Service is being deleted
        le_ref_DeleteRef(ServiceIDSafeRefMap, (void*)(uintptr_t) serviceId);
        return;
    }

    //
    // Set the retry timer in the event we do not receive a Connect-Service-Response
    //
    le_timer_Ref_t  connectServiceTimerRef;
    le_clk_Time_t timerInterval = {.sec=RPC_PROXY_CONNECT_SERVICE_REQUEST_TIMER_INTERVAL,
                                   .usec=0};

    // Create a timer to trigger a Try-Connect-Service retry
    connectServiceTimerRef = le_timer_Create("Connect-Service-Request timer");
    le_timer_SetInterval(connectServiceTimerRef, timerInterval);
    le_timer_SetHandler(connectServiceTimerRef, rpcProxy_ConnectServiceTimerExpiryHandler);
    le_timer_SetWakeup(connectServiceTimerRef, false);

    // Set Proxy Message (copy) in the timer event
    le_timer_SetContextPtr(connectServiceTimerRef, proxyMessagePtr);

    // Start timer
    le_timer_Start(connectServiceTimerRef);

    // Store the timerRef in a hashmap, using the Service-ID as a key, so that
    // it can be retrieved later
    le_hashmap_Put(ExpiryTimerRefByServiceId,
                   (void*)(uintptr_t) proxyMessagePtr->commonHeader.serviceId,
                   connectServiceTimerRef);

    LE_INFO("Connecting to service '%s' - "
            "starting retry timer (%d secs.), service-id [%" PRIu32 "]",
            proxyMessagePtr->serviceName,
            RPC_PROXY_CONNECT_SERVICE_REQUEST_TIMER_INTERVAL,
            proxyMessagePtr->commonHeader.serviceId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to Connect a Service
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DoConnectService
(
    const char* serviceName, ///< [IN] Name of the service
    const uint32_t serviceId, ///< [IN] Service ID associated with the service
    const char* protocolId ///< [IN] Protocol ID string
)
{
    bool serviceMatch = false;

    // Checking if the DoConnectService() for the specified serviceName is already established.
    // Retrieve the Service-ID, using the service-name
    uint32_t* serviceIdCopyPtr = le_hashmap_Get(ServiceIDMapByName, serviceName);
    if (serviceIdCopyPtr != NULL)
    {
        LE_WARN("DoConnectService() for service name:'%s' is duplicated.", serviceName);
        return LE_DUPLICATE;
    }

    // Traverse the Binding Reference array searching for a service-name match
    for (uint32_t index = 0; rpcProxyConfig_GetClientReferenceArray(index); index++)
    {
        const rpcProxy_ExternClient_t* bindingRefPtr = NULL;

        // Set a pointer to the Binding Reference element
        bindingRefPtr = rpcProxyConfig_GetClientReferenceArray(index);

        // Traverse service-name array searching for matching service-name
        if (strcmp(bindingRefPtr->serviceName, serviceName) == 0)
        {
            le_msg_SessionRef_t sessionRef = NULL;
            le_result_t result = LE_OK;

            // Verify the Protocol ID Str
            if (strcmp(bindingRefPtr->protocolIdStr, protocolId) != 0)
            {
                // Invalid API version
                LE_ERROR("API Version Check failed - Protocol-ID-Str '%s' does not match '%s'",
                         protocolId, bindingRefPtr->protocolIdStr);
                return LE_FORMAT_ERROR;
            }

            LE_INFO("======= Starting client for '%s' service, '%s' protocol ========",
                    bindingRefPtr->serviceName, bindingRefPtr->protocolIdStr);

#ifndef RPC_PROXY_LOCAL_SERVICE
            le_msg_ProtocolRef_t protocolRef =
                le_msg_GetProtocolRef(bindingRefPtr->protocolIdStr,
                                      (bindingRefPtr->messageSize + sizeof(uint32_t)));

            if (protocolRef == NULL)
            {
                LE_ERROR("Could not get protocol reference for '%s' service, '%s' protocol",
                         bindingRefPtr->serviceName,
                         bindingRefPtr->protocolIdStr);
                return LE_FAULT;
            }

            rpcProxy_ExternLinuxClient_t* refPtr =
                CONTAINER_OF(bindingRefPtr, rpcProxy_ExternLinuxClient_t, common);

            // Create the session
            sessionRef = le_msg_CreateSession(protocolRef, refPtr->localServiceInstanceName);
            if (sessionRef == NULL)
            {
                LE_ERROR("Could not create session for '%s' service",
                         refPtr->localServiceInstanceName);
                return LE_FAULT;
            }
#else
            rpcProxy_ExternLocalClient_t* refPtr =
                CONTAINER_OF(bindingRefPtr, rpcProxy_ExternLocalClient_t, common);

            // Set a pointer to the Service Reference
            le_msg_ServiceRef_t serviceRef = (le_msg_ServiceRef_t) refPtr->localServicePtr;

            if (serviceRef->type == LE_MSG_SERVICE_LOCAL)
            {
                // Create the session
                sessionRef =
                    le_msg_CreateLocalSession(
                        CONTAINER_OF(serviceRef, le_msg_LocalService_t, service));

                if (sessionRef == NULL)
                {
                    LE_ERROR("Could not create session for '%s' service",
                             bindingRefPtr->serviceName);
                    return LE_FAULT;
                }
            }
            else
            {
                LE_ERROR("Unsupported service type: %d", serviceRef->type);
                return LE_FAULT;
            }
#endif

            // Found matching service
            serviceMatch = true;

#ifndef RPC_PROXY_LOCAL_SERVICE
            // Add a handler for session closures
            le_msg_SetSessionCloseHandler(sessionRef,
                                          ClientServiceCloseHandler,
                                          (void*) bindingRefPtr);
#endif

            le_msg_SetSessionRecvHandler(sessionRef, rpcEventHandler_EventCallback, (void*)serviceId);

            // Try to Open the Session
            result = le_msg_TryOpenSessionSync(sessionRef);

            if ( result != LE_OK )
            {
                // Clean-up Session
                le_msg_DeleteSession(sessionRef);

                LE_WARN("Could not connect to '%s' service, result %d",
                        bindingRefPtr->serviceName, result);
                return result;
            }

            LE_INFO("Successfully opened session for '%s' service",
                    bindingRefPtr->serviceName);

            // Allocate memory from Service Name string pool
            char* serviceNameCopyPtr = le_mem_Alloc(ServiceNameStringPoolRef);

            le_utf8_Copy(serviceNameCopyPtr,
                         bindingRefPtr->serviceName,
                         LIMIT_MAX_IPC_INTERFACE_NAME_BYTES,
                         NULL);

            // Allocate memory from Service ID pool
            uint32_t* serviceIdCopyPtr = le_mem_Alloc(ServiceIdPoolRef);
            *serviceIdCopyPtr = serviceId;

            // Store the Service-ID in a hashmap, using the service-name as a key
            le_hashmap_Put(ServiceIDMapByName, serviceNameCopyPtr, serviceIdCopyPtr);

            LE_INFO("Successfully saved Service ID Reference, "
                    "service-name [%s], service-id [%" PRIu32 "]",
                    serviceNameCopyPtr,
                    *serviceIdCopyPtr);

            // Store the sessionRef in a hashmap, using the Service-ID as a key
            le_hashmap_Put(SessionRefMapByID, (void*)(uintptr_t) *serviceIdCopyPtr, sessionRef);

            LE_INFO("Successfully saved Session Reference, "
                    "session safe reference [%" PRIuPTR "], service-id [%" PRIu32 "]",
                    (uintptr_t) sessionRef,
                    serviceId);

            // No need to traverse remaining Service References
            break;
        }
    }

    if (!serviceMatch)
    {
        LE_ERROR("Cannot start '%s' service - service not found", serviceName);
        return LE_UNAVAILABLE;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the server and advertise the service.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_AdvertiseServices
(
    const char* systemName ///< [IN] Name of System on which to advertise services
)
{
    // Retrieve the Network Record for this system, if one exists
    NetworkRecord_t* networkRecordPtr =
        le_hashmap_Get(rpcProxyNetwork_GetNetworkRecordHashMapByName(), systemName);

    if (networkRecordPtr == NULL)
    {
        LE_ERROR("Unable to retrieve Network Record, "
                 "system-name [%s] - unknown system",
                 systemName);
        return;
    }

    // Traverse all Service Reference entries in the Service Reference array and
    // advertise their service
    for (uint32_t index = 0; rpcProxyConfig_GetServerReferenceArray(index); index++)
    {
        const rpcProxy_ExternServer_t* serviceRefPtr = NULL;

        // Set a pointer the Service Reference element
        serviceRefPtr = rpcProxyConfig_GetServerReferenceArray(index);

        const char* systemNamePtr =
            rpcProxyConfig_GetSystemNameByServiceName(serviceRefPtr->serviceName);

        if (systemNamePtr == NULL)
        {
            // RPC binding is not set.
            LE_WARN("Unable to retrieve system-name for service-name '%s'",
                     serviceRefPtr->serviceName);
            continue;
        }

        // Only interested in those services on the specified system-name
        if (strcmp(systemName, systemNamePtr) != 0)
        {
            continue;
        }

        // Check the Network Connection status
        // Only start the Advertise-Service sequence if Network is UP
        if (networkRecordPtr->state == NETWORK_UP)
        {
            LE_INFO("======= Starting Server %s ========", serviceRefPtr->serviceName);

            const char* remoteServiceName =
                rpcProxyConfig_GetRemoteServiceNameByServiceName(serviceRefPtr->serviceName);

            if (remoteServiceName == NULL)
            {
                LE_ERROR("Unable to retrieve remote service-name for service-name '%s'",
                         serviceRefPtr->serviceName);
                continue;
            }

            // Send Connect-Service Message to the far-side for the specified service-name
            // and wait for a valid Connect-Service response before advertising the service
            SendSessionConnectRequest(
                systemName,
                remoteServiceName,
                serviceRefPtr->protocolIdStr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Hide All Server sessions affected by the Network Connection failure.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_HideServices
(
    const char* systemName ///< [IN] Name of System on which to hide services
)
{
    // Traverse all Service Reference entries in the Service Reference array and
    // hide their service
    for (uint32_t index = 0; rpcProxyConfig_GetServerReferenceArray(index); index++)
    {
        const rpcProxy_ExternServer_t* serviceRefPtr = NULL;

        // Set a pointer to the Service Reference element
        serviceRefPtr = rpcProxyConfig_GetServerReferenceArray(index);

        // Retrieve the system-name for the specified service-name
        const char* systemNamePtr =
            rpcProxyConfig_GetSystemNameByServiceName(serviceRefPtr->serviceName);

        if (systemNamePtr == NULL)
        {
            LE_WARN("Unable to retrieve system-name for service-name '%s'",
                     serviceRefPtr->serviceName);
            continue;
        }

        // Only interested in those services on the specified system-name
        if (strcmp(systemName, systemNamePtr) != 0)
        {
            continue;
        }

        // Retrieve the remote service-name for the specified service-name
        const char* remoteServiceNamePtr =
            rpcProxyConfig_GetRemoteServiceNameByServiceName(serviceRefPtr->serviceName);

        // Delete all timers associated with the service-name
        DeleteAllTimers(remoteServiceNamePtr);

        // Delete the Service associated with the service-name
        DeleteService(serviceRefPtr->serviceName);

    } // End of for-loop
}

//--------------------------------------------------------------------------------------------------
/**
 * Disconnect all Client sessions affected by the Network Connection failure.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_DisconnectSessions
(
    const char* systemName ///< [IN] Name of System on which to disconnect sessions
)
{
    // Traverse the Binding Reference array searching for a service-name match
    for (uint32_t index = 0; rpcProxyConfig_GetClientReferenceArray(index); index++)
    {
        const rpcProxy_ExternClient_t* sessionRefPtr = NULL;
        le_msg_SessionRef_t sessionRef = NULL;

        // Set a pointer to the Binding Reference element
        sessionRefPtr = rpcProxyConfig_GetClientReferenceArray(index);

        // Retrieve the system-name for the specified service-name
        const char* systemNamePtr =
            rpcProxyConfig_GetSystemNameByServiceName(sessionRefPtr->serviceName);

        if (systemNamePtr == NULL)
        {
            LE_ERROR("Unable to retrieve system-name for service-name '%s'",
                     sessionRefPtr->serviceName);
            return;
        }

        // Only interested in those sessions on the specified system-name
        if (strcmp(systemName, systemNamePtr) != 0)
        {
            continue;
        }

        // Retrieve the Service-ID, using the service-name
        uint32_t* serviceIdCopyPtr = le_hashmap_Get(ServiceIDMapByName, sessionRefPtr->serviceName);
        if (serviceIdCopyPtr != NULL)
        {
            // Retrieve the Session reference, using the Service-ID
            sessionRef = le_hashmap_Get(SessionRefMapByID, (void*)(uintptr_t) *serviceIdCopyPtr);
            if (sessionRef != NULL)
            {
                LE_INFO("======= Stopping client for '%s' service ========", sessionRefPtr->serviceName);

#ifndef RPC_PROXY_LOCAL_SERVICE
                // Stop the client side of the service
                le_msg_DeleteSession(sessionRef);
#endif

                // Remove sessionRef from hash-map
                le_hashmap_Remove(SessionRefMapByID, (void*)(uintptr_t) *serviceIdCopyPtr);
            }

            // Traverse the RequestResponseRefByProxyId map
            le_hashmap_It_Ref_t iter = le_hashmap_GetIterator(RequestResponseRefByProxyId);

            while (le_hashmap_NextNode(iter) == LE_OK)
            {
                rpcProxy_ClientRequestResponseRecord_t* requestResponsePtr =
                    (rpcProxy_ClientRequestResponseRecord_t*) le_hashmap_GetValue(iter);

                if (requestResponsePtr != NULL)
                {
                    // Check if the serviceId of the request-response
                    if (requestResponsePtr->commonHeader.serviceId == *serviceIdCopyPtr)
                    {
                        LE_INFO("======= Cleaning up Request-Response record for service Id [%" PRIu32 "]",
                                requestResponsePtr->commonHeader.serviceId);

                        // Remove entry from hash-map, using the Proxy Message Id
                        le_hashmap_Remove(RequestResponseRefByProxyId,
                                          (void*)(uintptr_t) requestResponsePtr->commonHeader.id);

                        // Free request response Proxy Message Copy Memory
                        le_mem_Release(requestResponsePtr);
                    }
                }
            }

            // Get the stored key object
            char* serviceNameCopyPtr =
                le_hashmap_GetStoredKey(
                    ServiceIDMapByName,
                    sessionRefPtr->serviceName);
            if (serviceNameCopyPtr == NULL)
            {
                return;
            }

            if (serviceNameCopyPtr != NULL)
            {
                // Remove the serviceId in a hashmap, using the service-name as a key
                le_hashmap_Remove(ServiceIDMapByName, serviceNameCopyPtr);

                // Free the memory allocated for the Service Name string
                le_mem_Release(serviceNameCopyPtr);
            }

            // Free the memory allocated for the Service-ID
            le_mem_Release(serviceIdCopyPtr);
        }
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * One-time init for RPC Proxy application component
 *
 * This pre-initializes the Local-Messaging queues for all server-references.
 *
 * @note Must be called either directly, such as in the case of the RPC Proxy Library,
 *       or indirectly as a Legato component via the RPC Proxy's COMPONENT_INIT_ONCE.
 *
 * @return
 *      - LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_rpcProxy_InitializeOnce
(
    void
)
{
    le_result_t  result;

#ifdef RPC_PROXY_LOCAL_SERVICE
    // Create hash map for server references, using Service Name as key.
    ServerRefMapByName = le_hashmap_InitStatic(ServerRefHashMap,
                                               RPC_PROXY_SERVICE_BINDINGS_MAX_NUM,
                                               le_hashmap_HashString,
                                               le_hashmap_EqualsString);

    // Traverse all Service Reference entries in the Server-Reference array and
    // initialize the Local Messaging queue.
    for (uint32_t index = 0; rpcProxyConfig_GetServerReferenceArray(index); index++)
    {
        const rpcProxy_ExternServer_t* serviceRefPtr = NULL;

        // Set a pointer to the Service Reference element
        serviceRefPtr = rpcProxyConfig_GetServerReferenceArray(index);

        rpcProxy_ExternLocalServer_t* refPtr =
            CONTAINER_OF(serviceRefPtr, rpcProxy_ExternLocalServer_t, common);

        // Initialization the Local Service
        le_msg_ServiceRef_t serviceRef = refPtr->initLocalServicePtr();

        // Store the serviceRef in a hashmap, using the Service-Name as a key
        le_hashmap_Put(ServerRefMapByName,
                       serviceRefPtr->serviceName,
                       serviceRef);
    }
#endif

    ServiceNameStringPoolRef = le_mem_InitStaticPool(ServiceNameStringPool,
                                                     RPC_PROXY_SERVICE_BINDINGS_MAX_NUM,
                                                     RPC_PROXY_MSG_SERVICE_NAME_SIZE);

    ServiceIdPoolRef = le_mem_InitStaticPool(ServiceIdPool,
                                             RPC_PROXY_SERVICE_BINDINGS_MAX_NUM,
                                             sizeof(uint32_t));

    ProxyConnectServiceMessagesPoolRef = le_mem_InitStaticPool(
                                             ProxyConnectServiceMessagePool,
                                             RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                                             sizeof(rpcProxy_ConnectServiceMessage_t));

    ProxyClientRequestResponseRecordPoolRef = le_mem_InitStaticPool(
                                                  ProxyClientRequestResponseRecordPool,
                                                  RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                                                  sizeof(rpcProxy_ClientRequestResponseRecord_t));
#ifdef RPC_PROXY_LOCAL_SERVICE
    le_mem_SetDestructor(ProxyClientRequestResponseRecordPoolRef,
                         DestructRequestResponse);
#endif

    rpcFStream_InitFileStreamPool();

    // Create hash map for message references (value), using the Proxy Message ID (key)
    MsgRefMapByProxyId = le_hashmap_InitStatic(MsgRefHashMap,
                                               RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                                               le_hashmap_HashVoidPointer,
                                               le_hashmap_EqualsVoidPointer);

    // Create safe reference map to generate Service-IDs for a given service-name
    ServiceIDSafeRefMap = le_ref_InitStaticMap(ServiceIDSafeRefStaticMap,
                                               RPC_PROXY_SERVICE_BINDINGS_MAX_NUM);

    // Create hash map for service-IDs (value), using service-instance-name as key.
    ServiceIDMapByName = le_hashmap_InitStatic(ServiceIDHashMap,
                                               RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                                               le_hashmap_HashString,
                                               le_hashmap_EqualsString);

    // Create hash map for session references, using Service-ID as key.
    SessionRefMapByID = le_hashmap_InitStatic(SessionRefHashMap,
                                              RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                                              le_hashmap_HashVoidPointer,
                                              le_hashmap_EqualsVoidPointer);

    // Create hash map for service references, using Service ID as key.
    ServiceRefMapByID = le_hashmap_InitStatic(ServiceRefHashMap,
                                              RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                                              le_hashmap_HashVoidPointer,
                                              le_hashmap_EqualsVoidPointer);

    // Create hash map for expiry timer references, using the Proxy Message ID (key).
    ExpiryTimerRefByProxyId = le_hashmap_InitStatic(ExpiryTimerRefHashMap,
                                                    (RPC_PROXY_MSG_REFERENCE_MAX_NUM +
                                                    RPC_PROXY_NETWORK_SYSTEM_MAX_NUM),
                                                    le_hashmap_HashVoidPointer,
                                                    le_hashmap_EqualsVoidPointer);

    // Create hash map for expiry timer references, using the Service-ID (key).
    ExpiryTimerRefByServiceId = le_hashmap_InitStatic(ExpiryTimerRefServiceIdHashMap,
                                                      RPC_PROXY_SERVICE_BINDINGS_MAX_NUM,
                                                      le_hashmap_HashVoidPointer,
                                                      le_hashmap_EqualsVoidPointer);

    // Create hash map for Request-Response Record references, using the Proxy Message ID (key).
    RequestResponseRefByProxyId = le_hashmap_InitStatic(
                                      RequestResponseRefHashMap,
                                      RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                                      le_hashmap_HashVoidPointer,
                                      le_hashmap_EqualsVoidPointer);


    rpcProxy_InitializeOnceStreamingMemPools();
    rpcEventHandler_InitializeOnce();

    LE_INFO("RPC Proxy Service Init start");

    // Initialize the RPC Proxy Configuration service before accessing
    result = rpcProxyConfig_InitializeOnce();
    if (result != LE_OK)
    {
        LE_ERROR("Error initializing RPC Proxy Network services, result [%d]", result);
        goto end;
    }

    // Initialize the RPC Proxy Network services
    result = rpcProxyNetwork_InitializeOnce();
    if (result != LE_OK)
    {
        LE_ERROR("Error initializing RPC Proxy Network services, result [%d]", result);
        goto end;
    }

end:
    return result;
}

#ifndef LE_CONFIG_RPC_PROXY_LIBRARY
//--------------------------------------------------------------------------------------------------
/**
 * Component once initializer.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT_ONCE
{
    le_rpcProxy_InitializeOnce();
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * This function initializes and starts the RPC Proxy Services.
 *
 * @note Must be called either directly, such as in the case of the RPC Proxy Library,
 *       or indirectly as a Legato component via the RPC Proxy's COMPONENT_INIT.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_NOT_FOUND if mandatory configuration is not found.
 *      - LE_BAD_PARAMETER if number of elements exceeds the storage array size.
 *      - LE_FAULT for all other errors.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_rpcProxy_Initialize
(
    void
)
{
    le_result_t  result = LE_OK;

    // Load the ConfigTree configuration for links, bindings and references
    result = rpcProxyConfig_LoadSystemLinks();
    if (result != LE_OK)
    {
        LE_ERROR("Unable to load System-Links configuration, result [%d]", result);
        goto end;
    }

    result = rpcProxyConfig_LoadReferences();
    if (result != LE_OK)
    {
        LE_ERROR("Unable to load References configuration, result [%d]", result);
        goto end;
    }

    result = rpcProxyConfig_LoadBindings();
    if (result != LE_OK)
    {
        LE_ERROR("Unable to load Bindings configuration, result [%d]", result);
        goto end;
    }

    result = rpcProxyConfig_ValidateConfiguration();
    if (result != LE_OK)
    {
        LE_ERROR("Configuration validation error, result [%d]", result);
        goto end;
    }

    //
    // Create RPC Communication channel
    //

    // Traverse all System-Link entries in the System-Link array and
    // create the Network Communication channel
    for (uint32_t index = 0; rpcProxyConfig_GetSystemLinkArray(index).systemName; index++)
    {
#ifndef RPC_PROXY_LOCAL_SERVICE
        LE_INFO("Opening library %s",
                rpcProxyConfig_GetSystemLinkArray(index).libraryName);

        // Open System-Link Library, using dlopen
        // Provides le_comm API implementation
        // NOTE: Only a single le_comm API implementation is currently supported at a time
        void* handle = dlopen(rpcProxyConfig_GetSystemLinkArray(index).libraryName,
                              RTLD_LAZY | RTLD_GLOBAL);
        if (!handle)
        {
            LE_ERROR("Failed to load library '%s' (%s)",
                     rpcProxyConfig_GetSystemLinkArray(index).libraryName,
                     dlerror());
            goto end;
        }

        LE_INFO("Finished opening library %s",
                rpcProxyConfig_GetSystemLinkArray(index).libraryName);
#endif

        // Get the System Name using the Link Name
        const char* systemName =
            rpcProxyConfig_GetSystemNameByLinkName(
                rpcProxyConfig_GetSystemLinkArray(index).systemName);

        if (systemName == NULL)
        {
            LE_ERROR("Unable to retrieve system--name for system-link '%s'",
                    rpcProxyConfig_GetSystemLinkArray(index).systemName);
            goto end;
        }

        // Create and connect a network communication channel
        result = rpcProxyNetwork_CreateNetworkCommunicationChannel(systemName);
        if (result == LE_OK)
        {
            // Start the Advertise-Service sequence for services being hosted by the RPC Proxy
            // NOTE:  The advertise-service will only be completed once we have
            //        successfully performed a connect-service on the far-side
            rpcProxy_AdvertiseServices(systemName);
        }
        else if (result != LE_IN_PROGRESS)
        {
            // Unable to estblish Network Connection
            // Start Network Retry Timer
            rpcProxyNetwork_StartNetworkConnectionRetryTimer(systemName);
        }
    }

    LE_INFO("RPC Proxy Service Init done");
end:
    return result;
}

#ifndef LE_CONFIG_RPC_PROXY_LIBRARY
//--------------------------------------------------------------------------------------------------
/**
 * Component initializer.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_rpcProxy_Initialize();
}
#endif
