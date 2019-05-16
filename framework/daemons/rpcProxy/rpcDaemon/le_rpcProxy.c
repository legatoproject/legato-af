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
static le_result_t ProcessConnectServiceRequest(const char*, rpcProxy_ConnectServiceMessage_t*);
#ifdef RPC_PROXY_UNIT_TEST
void ServerMsgRecvHandler(le_msg_MessageRef_t, void*);
#else
static void ServerMsgRecvHandler(le_msg_MessageRef_t, void*);
#endif
static le_result_t DoConnectService(const char*, const uint32_t, const char*);
static void ProcessServerResponse(rpcProxy_Message_t*, bool);
static le_result_t RepackMessage(rpcProxy_Message_t*, rpcProxy_Message_t*, bool);
static le_result_t PreProcessResponse(void*, size_t*);
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
 * Maximum number of Response "out" parameters per message.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_MSG_OUT_PARAMETER_MAX_NUM    32

//--------------------------------------------------------------------------------------------------
/**
 * Maximum receive buffer size.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_RECV_BUFFER_MAX              (RPC_PROXY_MAX_MESSAGE + RPC_PROXY_MSG_HEADER_SIZE)

//--------------------------------------------------------------------------------------------------
/**
 * Array of non-variable message field pack sizes, indexed by TagID.
 * NOTE: If the TagID_t definition changes, this will need to be changed to match accordingly.
 */
//--------------------------------------------------------------------------------------------------
static const int ItemPackSize[] =
{
    [LE_PACK_UINT8]     = LE_PACK_SIZEOF_UINT8,
    [LE_PACK_UINT16]    = LE_PACK_SIZEOF_UINT16,
    [LE_PACK_UINT32]    = LE_PACK_SIZEOF_UINT32,
    [LE_PACK_UINT64]    = LE_PACK_SIZEOF_UINT64,
    [LE_PACK_INT8]      = LE_PACK_SIZEOF_INT8,
    [LE_PACK_INT16]     = LE_PACK_SIZEOF_INT16,
    [LE_PACK_INT32]     = LE_PACK_SIZEOF_INT32,
    [LE_PACK_INT64]     = LE_PACK_SIZEOF_INT64,
    [LE_PACK_SIZE]      = LE_PACK_SIZEOF_SIZE,
    [LE_PACK_BOOL]      = LE_PACK_SIZEOF_BOOL,
    [LE_PACK_CHAR]      = LE_PACK_SIZEOF_CHAR,
    [LE_PACK_DOUBLE]    = LE_PACK_SIZEOF_DOUBLE,
    [LE_PACK_RESULT]    = LE_PACK_SIZEOF_RESULT,
    [LE_PACK_ONOFF]     = LE_PACK_SIZEOF_ONOFF,
    [LE_PACK_REFERENCE] = LE_PACK_SIZEOF_REFERENCE,
};

#ifdef RPC_PROXY_LOCAL_SERVICE
//--------------------------------------------------------------------------------------------------
/**
 * Structure of response pointers provided by the client.
 */
//--------------------------------------------------------------------------------------------------
typedef struct responseParameterArray
{
#if UINTPTR_MAX == UINT32_MAX
    uint32_t pointer[RPC_PROXY_MSG_OUT_PARAMETER_MAX_NUM];
#elif UINTPTR_MAX == UINT64_MAX
    uint64_t pointer[RPC_PROXY_MSG_OUT_PARAMETER_MAX_NUM];
#endif
}
responseParameterArray_t;
#endif


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
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(ExpiryTimerRefHashMap, RPC_PROXY_MSG_REFERENCE_MAX_NUM);
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
 * Hash Map to store Response "out" parameter pointers (value), using the Proxy Message ID (key).
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(ResponseParameterArrayHashMap, RPC_PROXY_MSG_OUT_PARAMETER_MAX_NUM);
static le_hashmap_Ref_t ResponseParameterArrayByProxyId = NULL;

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
 * This pool is used to allocate memory for the Proxy Messages.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ProxyMessagePool,
                          RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                          sizeof(rpcProxy_Message_t));
static le_mem_PoolRef_t ProxyMessagesPoolRef = NULL;

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


//--------------------------------------------------------------------------------------------------
/**
 * This pool is used to allocate memory for local message pointers to String and Array Parameters.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(MessageDataPtrPool,
                          RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                          RPC_LOCAL_MAX_MESSAGE);
static le_mem_PoolRef_t MessageDataPtrPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * This pool is used to allocate memory for local message linked list.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(LocalMessagePool,
                          RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                          sizeof(rpcProxy_LocalMessage_t));
static le_mem_PoolRef_t LocalMessagePoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Doubly linked list to track outstanding local message memory allocation
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t LocalMessageList = LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * This pool is used to allocate memory for the Response "out" parameter response array.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ResponseParameterArrayPool,
                          RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                          sizeof(responseParameterArray_t));
static le_mem_PoolRef_t ResponseParameterArrayPoolRef = NULL;

#endif

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

        default:
            return "Unknown";
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Generic function for generating Server-Response Error Messages
 */
//--------------------------------------------------------------------------------------------------
static void GenerateServerResponseErrorMessage
(
    rpcProxy_Message_t* proxyMessagePtr,
    le_result_t resultCode
)
{
    //
    // Generate a Server-Response Error Message
    //

    // First field in message is the Msg ID (uint32_t)
    // Skip forward four bytes
    // Set pointer to buffer field in the message
    uint8_t* msgBufPtr = &proxyMessagePtr->message[LE_PACK_SIZEOF_UINT32];

    // Pack a result-code into the Proxy Message
    LE_ASSERT(le_pack_PackResult(&msgBufPtr, resultCode));

    // Set Proxy Message size and type
    proxyMessagePtr->msgSize = (msgBufPtr - &proxyMessagePtr->message[0]);
    proxyMessagePtr->commonHeader.type = RPC_PROXY_SERVER_RESPONSE;
}

#ifdef RPC_PROXY_LOCAL_SERVICE
//--------------------------------------------------------------------------------------------------
/**
 * Function for Cleaning-up Local Message Memory Pool resources that have been allocated
 * in order to facilitate string and array memory optimizations
 */
//--------------------------------------------------------------------------------------------------
static void CleanUpLocalMessageResources
(
    uint32_t proxyMsgId
)
{
    //
    // Clean-up local message memory allocation associated with this Proxy message ID
    //
    le_dls_Link_t* linkPtr = le_dls_Peek(&LocalMessageList);

    while (linkPtr != NULL)
    {
        rpcProxy_LocalMessage_t* localMessagePtr = CONTAINER_OF(linkPtr, rpcProxy_LocalMessage_t, link);

        // Move the linkPtr to the next node in the list now, in case we have to remove
        // the node it currently points to.
        linkPtr = le_dls_PeekNext(&LocalMessageList, linkPtr);

        // Verify if this is associated with our Proxy Message
        if (localMessagePtr->id == proxyMsgId)
        {
            LE_DEBUG("Cleaning up local-message resources, proxy id [%" PRIu32 "]", proxyMsgId);

            // Remove entry from linked list
            le_dls_Remove(&LocalMessageList, &(localMessagePtr->link));

            // Free memory allocated for the data pointer
            le_mem_Release(localMessagePtr->dataPtr);

            // Free memory allocated for this Local Message memory
            le_mem_Release(localMessagePtr);
        }
    }

    //
    // Clean-up the Response "out" parameter hashmap
    //

    // Retrieve the Response out" parameter hashmap entry
    responseParameterArray_t* arrayPtr =
        le_hashmap_Get(ResponseParameterArrayByProxyId, (void*) proxyMsgId);

    if (arrayPtr != NULL)
    {
        LE_DEBUG("Releasing response parameter array, proxy id [%" PRIu32 "]", proxyMsgId);

        // Free memory allocate for the Response "out" parameter array
        le_mem_Release(arrayPtr);

        // Delete Response "out" parameter hashmap entry
        le_hashmap_Remove(ResponseParameterArrayByProxyId, (void*) proxyMsgId);
    }
}
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Generic Expired Proxy Message Timers
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_ProxyMessageTimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    rpcProxy_CommonHeader_t* commonHeaderPtr = NULL;

    // Retrieve ContextPtr data (Proxy Message Common Header copy)
    commonHeaderPtr = (rpcProxy_CommonHeader_t*) le_timer_GetContextPtr(timerRef);

    if (commonHeaderPtr == NULL)
    {
        LE_ERROR("Error extracting copy of Proxy Message from timer record");
        return;
    }

    // Switch on the Message Type
    switch (commonHeaderPtr->type)
    {
        case RPC_PROXY_KEEPALIVE_REQUEST:
        {
            rpcProxy_KeepAliveMessage_t* proxyMessageCopyPtr =
                (rpcProxy_KeepAliveMessage_t*) le_timer_GetContextPtr(timerRef);

            if (proxyMessageCopyPtr != NULL)
            {
                LE_INFO("KEEPALIVE-Request timer expired; "
                        "Declare the network down, system [%s]",
                        proxyMessageCopyPtr->systemName);

                // Delete the Network Communication Channel
                rpcProxyNetwork_DeleteNetworkCommunicationChannel(
                    proxyMessageCopyPtr->systemName);

                // Remove entry from hash-map
                le_hashmap_Remove(ExpiryTimerRefByProxyId,
                                  (void*)(uintptr_t) proxyMessageCopyPtr->commonHeader.id);

                // Free Proxy Message Copy Memory
                le_mem_Release(proxyMessageCopyPtr);
            }
            else
            {
                LE_ERROR("Unable to retrieve copy of the "
                         "Proxy Keep-Alive Message Reference");
            }
            break;
        }

        case RPC_PROXY_CONNECT_SERVICE_REQUEST:
        {
            rpcProxy_ConnectServiceMessage_t proxyMessageCopy;

            // Retrieve the Connect-Service-Request message from the timer context
            const rpcProxy_ConnectServiceMessage_t* proxyMessageCopyPtr =
                (rpcProxy_ConnectServiceMessage_t*) le_timer_GetContextPtr(timerRef);

            if (proxyMessageCopyPtr == NULL)
            {
                LE_ERROR("Unable to retrieve copy of the "
                         "Proxy Connect-Service Message Reference");
            }

            // Make a copy of the Connect-Service-Request message
            memcpy(&proxyMessageCopy, proxyMessageCopyPtr, sizeof(proxyMessageCopy));

            LE_INFO("%s timer expired; Re-trigger "
                    "connect-service request '%s', service-id [%" PRIu32 "]",
                    DisplayMessageType(proxyMessageCopy.commonHeader.type),
                    proxyMessageCopy.serviceName,
                    proxyMessageCopy.commonHeader.serviceId);

            // Re-trigger connect-service-request to the remote system
            le_result_t result =
                rpcProxy_SendMsg(proxyMessageCopy.systemName, &proxyMessageCopy);

            if (result == LE_OK)
            {
                // Re-start the timer
                le_timer_Start(timerRef);
                return;
            }
            else
            {
                LE_ERROR("le_comm_Send failed, result %d", result);
            }
            break;
        }

        case RPC_PROXY_CLIENT_REQUEST:
        {
            rpcProxy_Message_t* proxyMessageCopyPtr =
                (rpcProxy_Message_t*) le_timer_GetContextPtr(timerRef);

            if (proxyMessageCopyPtr != NULL)
            {
                LE_INFO("Client-Request has timed out, "
                        "service-id [%" PRIu32 "], proxy id [%" PRIu32 "]; "
                        "check if client-response needs to be generated",
                        proxyMessageCopyPtr->commonHeader.serviceId,
                        proxyMessageCopyPtr->commonHeader.id);

                // Retrieve Message Reference from hash map, using the Proxy Message Id
                le_msg_MessageRef_t msgRef =
                    le_hashmap_Get(MsgRefMapByProxyId,
                                   (void*)(uintptr_t) proxyMessageCopyPtr->commonHeader.id);

                if (msgRef == NULL)
                {
                    LE_INFO("Unable to retrieve Message Reference, proxy id [%" PRIu32 "] - "
                            "do not generate response message",
                            proxyMessageCopyPtr->commonHeader.id);
                }
                else
                {
                    le_msg_ServiceRef_t serviceRef = NULL;

                    // Retrieve the Session reference, using the Service-ID
                    serviceRef =
                        le_hashmap_Get(
                            ServiceRefMapByID,
                            (void*)(uintptr_t) proxyMessageCopyPtr->commonHeader.serviceId);

                    if (serviceRef != NULL)
                    {
                        //
                        // Generate a LE_TIMEOUT event back to the client
                        //

                        // Generate LE_TIMEOUT Server-Response
                        GenerateServerResponseErrorMessage(
                            proxyMessageCopyPtr,
                            LE_TIMEOUT);

                        // Trigger a response back to the client
                        ProcessServerResponse(proxyMessageCopyPtr, false);
                    }
                    else
                    {
                        LE_INFO("Unable to retrieve Service Reference, service-id [%" PRIu32 "] - "
                                "do not generate response message",
                                proxyMessageCopyPtr->commonHeader.serviceId);
                    }
                }

                // Remove entry from hash-map
                le_hashmap_Remove(
                    ExpiryTimerRefByProxyId,
                    (void*)(uintptr_t) proxyMessageCopyPtr->commonHeader.id);

                // Free Proxy Message Copy Memory
                le_mem_Release(proxyMessageCopyPtr);
                proxyMessageCopyPtr = NULL;
            }
            else
            {
                LE_ERROR("Unable to retrieve copy of the Proxy Message Reference");
            }
            break;
        }

        default:
        {
            LE_ERROR("Unexpected Proxy Message, type [0x%x]", commonHeaderPtr->type);
            break;
        }
    }

    // Delete Timer
    le_timer_Delete(timerRef);
    timerRef = NULL;
}

#if RPC_PROXY_HEX_DUMP
void print_hex(uint8_t *s, uint16_t len) {
    for(int i = 0; i < len; i++) {
        LE_INFO("0x%x, ", s[i]);
    }
    LE_INFO("\n");
}
#endif

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
    le_result_t          result;
    size_t               byteCount;
    rpcProxy_Message_t   tmpProxyMessage;
    rpcProxy_Message_t  *proxyMessagePtr;
    void                *sendMessagePtr;
    uint32_t             id, serviceId;

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

    id = commonHeaderPtr->id;
    serviceId = commonHeaderPtr->serviceId;

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

        case RPC_PROXY_CLIENT_REQUEST:
        case RPC_PROXY_SERVER_RESPONSE:
        {
            // Set a Proxy Message pointer to the message
            proxyMessagePtr =
                (rpcProxy_Message_t*) messagePtr;

#if RPC_PROXY_HEX_DUMP
            print_hex(proxyMessagePtr->message, proxyMessagePtr->msgSize);
#endif

            // Re-package proxy message before sending
            result = RepackMessage(proxyMessagePtr, &tmpProxyMessage, true);
            if (result != LE_OK)
            {
                return result;
            }

#if RPC_PROXY_HEX_DUMP
            print_hex(tmpProxyMessage.message, tmpProxyMessage.msgSize);
#endif

            // Calculate the total size of the repacked
            // proxy message (header + message)
            byteCount = RPC_PROXY_MSG_HEADER_SIZE + tmpProxyMessage.msgSize;

            //
            // Prepare the Proxy Common Message Header of the tmpProxyMessage
            //

            // Set the Message Id, Service Id, and type
            tmpProxyMessage.commonHeader.id =
                htobe32(proxyMessagePtr->commonHeader.id);
            tmpProxyMessage.commonHeader.serviceId =
                htobe32(proxyMessagePtr->commonHeader.serviceId);
            tmpProxyMessage.commonHeader.type =
                proxyMessagePtr->commonHeader.type;

            // Put msgSize into Network-Order before sending
            tmpProxyMessage.msgSize = htobe16(tmpProxyMessage.msgSize);

            // Set send pointer to the tmpProxyMessage
            sendMessagePtr = &tmpProxyMessage;
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
             serviceId,
             id,
             byteCount);

    // Send the Message Payload as an outgoing Proxy Message to the far-size RPC Proxy
    result = le_comm_Send(networkRecordPtr->handle, sendMessagePtr, byteCount);

    if (result != LE_OK)
    {
        // Delete the Network Communication Channel
        rpcProxyNetwork_DeleteNetworkCommunicationChannel(systemName);
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for receiving Proxy Messages from the far side via the le_comm API
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RecvMsg
(
    void* handle, ///< [IN] Opaque handle to the le_comm communication channel
    void* bufferPtr, ///< [IN] Pointer to the buffer
    size_t* bufferSizePtr ///< [IN] Pointer to the size of the buffer
)
{
    le_result_t  result;

    // Receive incoming RPC Proxy message
    result = le_comm_Receive(handle, bufferPtr, bufferSizePtr);

    if (result != LE_OK)
    {
        return result;
    }

    if (*bufferSizePtr <= 0)
    {
        return LE_COMM_ERROR;
    }

    // Pre-process the buffer before processing the message payload
    result = PreProcessResponse(bufferPtr, bufferSizePtr);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to copy all un-copied content up from the Message Buffer into the new Messagge Buffer
 */
//--------------------------------------------------------------------------------------------------
static inline void RepackCopyContents
(
    /// [IN]Pointer to current position in the Message Buffer pointer
    uint8_t** msgBufPtr,
    /// [IN] Pointer to the previous position in the Message Buffer pointer
    uint8_t** previousMsgBufPtr,
    /// [IN] Pointer to the current position in the New Message Buffer pointer
    uint8_t** newMsgBufPtr
)
{
    // Calculate the number of bytes to be copied
    uint16_t byteCount = (*msgBufPtr - *previousMsgBufPtr);

    // Copy the contents before further processing
    memcpy(*newMsgBufPtr, *previousMsgBufPtr, byteCount);
    *newMsgBufPtr += byteCount;
    *previousMsgBufPtr = *msgBufPtr;
}


#ifdef RPC_PROXY_LOCAL_SERVICE
//--------------------------------------------------------------------------------------------------
/**
 * Function to retrieve a response memory buffer.
 * Helper function for facilitating rolling-up un-optimized data that is received over the wire.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RepackRetrieveResponsePointer
(
    /// [IN] Pointer to the Proxy Message
    rpcProxy_Message_t *proxyMessagePtr,
    /// [IN] Pointer to the slot-index in the response buffer array
    uint8_t* slotIndexPtr,
    /// [IN] Pointer to the response buffer pointer
    uint8_t** responsePtr
)
{
    // Retrieve existing array pointer, if it exists
    responseParameterArray_t* arrayPtr =
        le_hashmap_Get(ResponseParameterArrayByProxyId,
                       (void*)(uintptr_t) proxyMessagePtr->commonHeader.id);

    if (arrayPtr == NULL)
    {
        // Unable to find response parameter array - Not expected
        LE_ERROR("Pointer to response array is NULL, service-id [%" PRIu32 "], "
                 "proxy id [%" PRIu32 "]; Dropping packet",
                 proxyMessagePtr->commonHeader.serviceId,
                 proxyMessagePtr->commonHeader.id);

        return LE_BAD_PARAMETER;
    }

    // Increment the slotIndex
    *slotIndexPtr = *slotIndexPtr + 1;
    if (*slotIndexPtr == RPC_PROXY_MSG_OUT_PARAMETER_MAX_NUM)
    {
        LE_ERROR("Response array overflow error - out of array elements");
        return LE_OVERFLOW;
    }

    *responsePtr = (uint8_t*) arrayPtr->pointer[*slotIndexPtr];
    if (*responsePtr == NULL)
    {
        // Unable to find response parameter array - Not expected
        LE_ERROR("Response Pointer is NULL, service-id [%" PRIu32 "], "
                 "proxy id [%" PRIu32 "]; Dropping packet",
                 proxyMessagePtr->commonHeader.serviceId,
                 proxyMessagePtr->commonHeader.id);

        return LE_BAD_PARAMETER;
    }

    LE_DEBUG("Retrieving response pointer, proxy id [%" PRIu32 "], "
            "slot id [%" PRIu8 "], pointer [%" PRIuS "]",
             proxyMessagePtr->commonHeader.id,
             *slotIndexPtr,
             (size_t) arrayPtr->pointer[*slotIndexPtr]);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for allocating a response memory buffer.
 * Helper function for facilitating rolling-up un-optimized data that is received over the wire.
 */
//--------------------------------------------------------------------------------------------------
static void RepackAllocateResponseMemory
(
    rpcProxy_Message_t *proxyMessagePtr, ///< [IN] Pointer to the Proxy Message
    size_t size, ///< [IN] Size for the response buffer
    uint8_t** responsePtr ///< [IN] Pointer to the response pointer
)
{
    // Allocate a local message memory tracker record
    rpcProxy_LocalMessage_t* localMessagePtr = le_mem_ForceAlloc(LocalMessagePoolRef);

    // Allocate memory to hold the data
    localMessagePtr->dataPtr = le_mem_AssertVarAlloc(MessageDataPtrPoolRef, size + 1);
    memset(localMessagePtr->dataPtr, 0, size + 1);

    // Set the Proxy Message Id this belongs to
    localMessagePtr->id = proxyMessagePtr->commonHeader.id;

    // Initialize the link
    localMessagePtr->link = LE_DLS_LINK_INIT;

    // Enqueue this in the Local Message List
    le_dls_Queue(&LocalMessageList, &(localMessagePtr->link));

    *responsePtr = localMessagePtr->dataPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for Rolling-up un-optimized data.  It copies the data from the Message Buffer into
 * the response memory after being received over the wire.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RepackUnOptimizedData
(
    /// [IN] Pointer to current position in the Message Buffer pointer
    uint8_t** msgBufPtr,
    /// [IN] Pointer to the previous position in the Message Buffer pointer
    uint8_t** previousMsgBufPtr,
    /// [IN] Pointer to the current position in the New Message Buffer pointer
    uint8_t** newMsgBufPtr,
    /// [IN] Pointer to the original Proxy Message
    rpcProxy_Message_t *proxyMessagePtr,
    /// [IN] Current tag ID
    TagID_t tagId,
    /// [IN] Pointer to the slot-index in the response buffer array
    uint8_t* slotIndexPtr
)
{
    le_result_t result = LE_OK;

    if (proxyMessagePtr->commonHeader.type == RPC_PROXY_SERVER_RESPONSE)
    {
        uint32_t value = 0;

        //
        // Copy everything up to this tag
        //

        // Copy the contents
        RepackCopyContents(msgBufPtr, previousMsgBufPtr, newMsgBufPtr);

        // Unpack the string size
        LE_ASSERT(le_pack_UnpackUint32(msgBufPtr, &value));

        LE_DEBUG("Received string, size [%" PRIu32 "]", value);

        // Verify validity of the string size
        if (((*msgBufPtr + value) - &proxyMessagePtr->message[0]) >=
              RPC_PROXY_MAX_MESSAGE)
        {
            // Insufficient space to store the array data
            // Drop entire dataset
            LE_ERROR("Format Error - Insufficient space "
                     "to store String data, "
                     "proxy id [%" PRIu32 "], tagId [%d]",
                     proxyMessagePtr->commonHeader.id,
                     tagId);

            return LE_FORMAT_ERROR;
        }

        // Retrieve the response pointer
        uint8_t* responsePtr;
        result = RepackRetrieveResponsePointer(
                    proxyMessagePtr,
                    slotIndexPtr,
                    &responsePtr);

        if (result != LE_OK)
        {
            return result;
        }

        // Copy data from Proxy Message into local response buffer
        memcpy(responsePtr, *msgBufPtr, value);
        responsePtr[value] = 0;

        // Increment msgBufPtr by "newValue"
        *msgBufPtr = *msgBufPtr + value;
    }
    else
    {
        size_t value;

        // Retrieve data size from the Proxy Message
        LE_ASSERT(le_pack_UnpackSize(msgBufPtr, &value));

        // Verify validity of the pointer data size
        if (((*msgBufPtr + value) - &proxyMessagePtr->message[0]) >=
                RPC_PROXY_MAX_MESSAGE)
        {
            // Insufficient space to store the pointer data
            LE_ERROR("Format Error - Insufficient space "
                     "to store Pointer data, "
                     "proxy id [%" PRIu32 "], tagId [%d]",
                     proxyMessagePtr->commonHeader.id,
                     tagId);

            return LE_FORMAT_ERROR;
        }

        // Copy the contents
        RepackCopyContents(msgBufPtr, previousMsgBufPtr, newMsgBufPtr);

        // Allocate the "in" parameter memory
        uint8_t* responsePtr;
        RepackAllocateResponseMemory(proxyMessagePtr, value, &responsePtr);

        // Copy data from Proxy Message into local buffer
        memcpy(responsePtr, *msgBufPtr, value);

        LE_DEBUG("String = %s", responsePtr);

        // Increment msgBufPtr by "newValue"
        *msgBufPtr = *msgBufPtr + value;

        // Pack memory pointer
#if UINTPTR_MAX == UINT32_MAX
        // Set pointer to data in new message buffer
        LE_ASSERT(le_pack_PackUint32(newMsgBufPtr, (uint32_t) responsePtr));

        LE_DEBUG("Rolling-up data, dataSize [%" PRIuS "], "
                 "proxy id [%" PRIu32 "], pointer [%" PRIu32 "]",
                 value,
                 proxyMessagePtr->commonHeader.id,
                 (uint32_t) responsePtr);

#elif UINTPTR_MAX == UINT64_MAX
        // Set pointer to data in new message buffer
        LE_ASSERT(le_pack_PackUint64(newMsgBufPtr, (uint64_t) responsePtr));

        LE_DEBUG("Rolling-up data, dataSize [%" PRIuS "], "
                 "proxy id [%" PRIu32 "], pointer [%" PRIu64 "]",
                 value,
                 proxyMessagePtr->commonHeader.id,
                 (uint64_t) responsePtr);
#endif
    }

    // Update the previous message buffer pointer to reflect what has been processed
    *previousMsgBufPtr = *msgBufPtr;
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to store a response memory buffer.
 * Helper function for facilitating un-rolling optimized data before it is sent over the wire.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RepackStoreResponsePointer
(
    rpcProxy_Message_t *proxyMessagePtr, ///< [IN] Pointer to the Proxy Message
    uint8_t* slotIndexPtr, ///< [IN] Pointer to the slot-index in the response buffer array
#if UINTPTR_MAX == UINT32_MAX
    uint32_t pointer ///< [IN] Pointer to the response buffer
#elif UINTPTR_MAX == UINT64_MAX
    uint64_t pointer ///< [IN] Pointer to the response buffer
#endif
)
{
    // Retrieve existing array pointer, if it exists
    responseParameterArray_t* arrayPtr =
        le_hashmap_Get(
            ResponseParameterArrayByProxyId,
            (void*)(uintptr_t) proxyMessagePtr->commonHeader.id);

    if (arrayPtr == NULL)
    {
        // Allocate the response parameter array, in which to
        // store the repsonse pointers
        arrayPtr = le_mem_ForceAlloc(ResponseParameterArrayPoolRef);
        memset(arrayPtr, 0, sizeof(responseParameterArray_t));
    }

    // Increment the slotIndex
    *slotIndexPtr = *slotIndexPtr + 1;
    if (*slotIndexPtr == RPC_PROXY_MSG_OUT_PARAMETER_MAX_NUM)
    {
        LE_ERROR("Response array overflow error - out of array elements");
        return LE_OVERFLOW;
    }


    // Store the response pointer in the array, using the slot Id
    arrayPtr->pointer[*slotIndexPtr] = pointer;

    LE_DEBUG("Storing response pointer, proxy id [%" PRIu32 "], "
            "slot id [%" PRIu8 "], pointer [%" PRIuS "]",
            proxyMessagePtr->commonHeader.id,
            *slotIndexPtr,
            (size_t) pointer);

    // Store the array of memory pointer until
    // the server response is received, using the proxy message Id
    le_hashmap_Put(ResponseParameterArrayByProxyId,
                   (void*)(uintptr_t) proxyMessagePtr->commonHeader.id,
                   arrayPtr);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for Un-rolling optimized data.  It copies the contents in memory into the
 * new Message Buffer before being sent it out on the wire.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RepackOptimizedData
(
    /// [IN] Pointer to current position in the Message Buffer pointer
    uint8_t** msgBufPtr,
    /// [IN] Pointer to the previous position in the Message Buffer pointer
    uint8_t** previousMsgBufPtr,
    /// [IN] Pointer to the current position in the New Message Buffer pointer
    uint8_t** newMsgBufPtr,
    /// [IN] Pointer to the original Proxy Message
    rpcProxy_Message_t *proxyMessagePtr,
    /// [IN] Pointer to the new Proxy Message
    rpcProxy_Message_t *newProxyMessagePtr,
    /// [IN] Current tag ID
    TagID_t tagId,
    /// [IN] index to the response buffer array
    uint8_t* slotIndexPtr
)
{
    le_result_t result = LE_OK;
    uint8_t* msgBufCopyPtr = (*msgBufPtr - 1);
    size_t size;

    // Copy all content up to, but not including the Tuple TagID
    RepackCopyContents(&msgBufCopyPtr, previousMsgBufPtr, newMsgBufPtr);

    // Retrieve the Array size and pointer from the Proxy Message
#if UINTPTR_MAX == UINT32_MAX
    uint32_t pointer;
    LE_ASSERT(le_pack_UnpackUint32Tuple(msgBufPtr, &size, &pointer));

    LE_DEBUG("Received message, pointer [%" PRIu32 "]",
             (uint32_t) pointer);
#elif UINTPTR_MAX == UINT64_MAX
    uint64_t pointer;
    LE_ASSERT(le_pack_UnpackUint64Tuple(msgBufPtr, &size, &pointer));

    LE_DEBUG("Received message, pointer [%" PRIu64 "]",
             (uint64_t) pointer);
#endif

    // Verify there is sufficient space to store the array
    if (((*newMsgBufPtr + size) - &newProxyMessagePtr->message[0]) >=
          RPC_PROXY_MAX_MESSAGE)
    {
        // Insufficient space to store the pointer data
        LE_ERROR("Format Error - Insufficient space "
                 "to store Pointer data, "
                 "proxy id [%" PRIu32 "], tagId [%d]",
                 proxyMessagePtr->commonHeader.id,
                 tagId);

         return LE_FORMAT_ERROR;
    }

    switch (tagId)
    {
        case LE_PACK_OUT_STRING_POINTER:
        case LE_PACK_OUT_ARRAY_POINTER:
            if (proxyMessagePtr->commonHeader.type != RPC_PROXY_CLIENT_REQUEST)
            {
                return LE_FORMAT_ERROR;
            }

            // Store the response pointer for later
            result = RepackStoreResponsePointer(
                        proxyMessagePtr,
                        slotIndexPtr,
                        pointer);
            if (result != LE_OK)
            {
                return result;
            }

            // Pack the size of the response buffer into the new message buffer
            LE_ASSERT(le_pack_PackSize(newMsgBufPtr, size));
            break;

        case LE_PACK_IN_STRING_POINTER:
            LE_DEBUG("Un-rolling string, dataSize [%" PRIuS "], "
                     "proxy id [%" PRIu32 "]",
                     size,
                     proxyMessagePtr->commonHeader.id);

            // Pack the string into the new message buffer
            LE_ASSERT(le_pack_PackString(newMsgBufPtr, (const char*) pointer, size));
            break;

        case LE_PACK_IN_ARRAY_POINTER:
            LE_DEBUG("Un-rolling array, dataSize [%" PRIuS "], "
                     "proxy id [%" PRIu32 "]",
                     size,
                     proxyMessagePtr->commonHeader.id);

            // Pack the array data into the new message buffer
            LE_PACK_PACKARRAY(newMsgBufPtr,
                              &pointer,
                              size,
                              ((*newMsgBufPtr + size) - &newProxyMessagePtr->message[0]),
                              le_pack_PackUint8,
                              &result);
            break;

        default:
            return LE_BAD_PARAMETER;
            break;
    }

    // Update the previous message buffer pointer
    // to reflect what has been processed
    *previousMsgBufPtr = *msgBufPtr;
    return result;
}
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Function for preparing Proxy Messages either being sent to or received from the far side
 *
 * It handles local-session string and array optimizations, endianness, and
 * 32-bit/64-bit architectural differences.  Uses the Tag ID to achieve this.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RepackMessage
(
    rpcProxy_Message_t *proxyMessagePtr, ///< [IN] Pointer to the original Proxy Message
    rpcProxy_Message_t *newProxyMessagePtr, ///< [IN] Pointer to the new Proxy Message
    bool sending ///< [IN] Boolean to identify if message is in-coming or out-going
)
{
    uint8_t* msgBufPtr;
    uint8_t* previousMsgBufPtr;
    uint8_t* newMsgBufPtr;
    uint32_t id;
    uint16_t count = 0;
    bool done = false;
#ifdef RPC_PROXY_LOCAL_SERVICE
    uint8_t  slotIndex = 0;
#endif

    // Initialize the message buffer size
    newProxyMessagePtr->msgSize = 0;

    // Verify Message Size
    if (proxyMessagePtr->msgSize == 0)
    {
        // Empty message payload - no need to proceed with repack
        return LE_OK;
    }

    // Initialize Message Buffer Pointers
    msgBufPtr = &proxyMessagePtr->message[0];
    newMsgBufPtr = &newProxyMessagePtr->message[0];

    // First field in message is the Msg ID (uint32_t)
    memcpy((uint8_t*) &id, msgBufPtr, LE_PACK_SIZEOF_UINT32);

    if (sending)
    {
        // Sending out on the wire - convert to Network Order
        id = htobe32(id);

    } else {
        // Receiving from off the wire - convert to Host Order
        id = be32toh(id);
    }

    // Copy Msg ID into new buffer
    memcpy(newMsgBufPtr, (uint8_t*) &id, LE_PACK_SIZEOF_UINT32);

    // Skip forward four bytes
    msgBufPtr = msgBufPtr + LE_PACK_SIZEOF_UINT32;
    previousMsgBufPtr = msgBufPtr;

    newMsgBufPtr = newMsgBufPtr + LE_PACK_SIZEOF_UINT32;

    // Traverse through the Message buffer, using the Tag IDs as a reference
    while (((msgBufPtr - &proxyMessagePtr->message[0]) < proxyMessagePtr->msgSize) && !done)
    {
        TagID_t tagId = *msgBufPtr;

        LE_DEBUG("Proxy Message size [%" PRIu16 "], index [%" PRIu32 "], tagId [%d]",
                 proxyMessagePtr->msgSize,
                 (uint32_t)(msgBufPtr - &proxyMessagePtr->message[0]),
                 tagId);

        // Switch on the TagID
        switch(tagId)
        {
            // Fixed-length Types
            case LE_PACK_UINT8:
            case LE_PACK_INT8:
            case LE_PACK_BOOL:
            case LE_PACK_CHAR:
            case LE_PACK_UINT16:
            case LE_PACK_INT16:
            case LE_PACK_RESULT:
            case LE_PACK_ONOFF:
            case LE_PACK_UINT32:
            case LE_PACK_INT32:
            case LE_PACK_REFERENCE:
            case LE_PACK_SIZE:
            case LE_PACK_UINT64:
            case LE_PACK_INT64:
            case LE_PACK_DOUBLE:
            {
                msgBufPtr += (LE_PACK_SIZEOF_TAG_ID + ItemPackSize[tagId]);
                break;
            }

#ifndef RPC_PROXY_LOCAL_SERVICE
            // Variable-length Type, bundled with a size
            case LE_PACK_STRING:
            {
                uint32_t value = 0;

                // Unpack the string size
                LE_ASSERT(le_pack_UnpackUint32(&msgBufPtr, &value));

                // Verify validity of the string size
                if (((msgBufPtr + value) - &proxyMessagePtr->message[0]) >=
                          RPC_PROXY_MAX_MESSAGE)
                {
                    // Insufficient space to store the array data
                    // Drop entire dataset
                    LE_ERROR("Format Error - Insufficient space "
                             "to store String data, "
                             "proxy id [%" PRIu32 "], tagId [%d]",
                             proxyMessagePtr->commonHeader.id,
                             tagId);

                    return LE_FORMAT_ERROR;
                }

                // Increment msgBufPtr by "value"
                msgBufPtr = msgBufPtr + value;

                // Copy the contents
                RepackCopyContents(&msgBufPtr, &previousMsgBufPtr, &newMsgBufPtr);
                break;
            }

            // Variable-length Type, bundled with a size
            case LE_PACK_ARRAYHEADER:
            {
                size_t value;

                // Unpack the array size
                LE_ASSERT(le_pack_UnpackSize(&msgBufPtr, &value));

                // Verify validity of the array size
                if (((msgBufPtr + value) - &proxyMessagePtr->message[0]) >=
                          RPC_PROXY_MAX_MESSAGE)
                {
                    // Insufficient space to store the array data
                    LE_ERROR("Format Error - Insufficient space "
                             "to store Array data, "
                             "proxy id [%" PRIu32 "], tagId [%d]",
                             proxyMessagePtr->commonHeader.id,
                             tagId);

                    return LE_FORMAT_ERROR;
                }

                // Increment msgBufPtr by "value"
                msgBufPtr = msgBufPtr + value;

                // Copy the contents
                RepackCopyContents(&msgBufPtr, &previousMsgBufPtr, &newMsgBufPtr);
                break;
            }

#else
            // Variable-length Type, bundled with a size
            // Requires repack
            case LE_PACK_STRING:
            case LE_PACK_ARRAYHEADER:
            {
                // Should only be called when receiving a string or array
                // coming in from the wire
                LE_ASSERT(!sending);

                // Roll-up un-optimized data (string or array) coming in
                // from wire
                le_result_t result =
                    RepackUnOptimizedData(
                        &msgBufPtr,
                        &previousMsgBufPtr,
                        &newMsgBufPtr,
                        proxyMessagePtr,
                        tagId,
                        &slotIndex);

                if (result != LE_OK)
                {
                    return result;
                }
                break;
            }

            // Special Types that indicate a repack is required
            case LE_PACK_IN_STRING_POINTER:
            case LE_PACK_OUT_STRING_POINTER:
            case LE_PACK_IN_ARRAY_POINTER:
            case LE_PACK_OUT_ARRAY_POINTER:
            {
                // Should only be called when sending an optimized array
                // out on the wire
                LE_ASSERT(sending);

                // Un-roll optimized data (string or array) before it is sent
                // over wire
                le_result_t result =
                    RepackOptimizedData(
                        &msgBufPtr,
                        &previousMsgBufPtr,
                        &newMsgBufPtr,
                        proxyMessagePtr,
                        newProxyMessagePtr,
                        tagId,
                        &slotIndex);

                if (result != LE_OK)
                {
                    return result;
                }
                break;
            }
#endif

            default:
                done = true;
                break;

        }  // End of switch-statement
    } // End of while-statement

    // Copy the contents
    RepackCopyContents(&msgBufPtr, &previousMsgBufPtr, &newMsgBufPtr);

    // Calculate the new message size
    count = (newMsgBufPtr - &(newProxyMessagePtr->message[0]));

    LE_DEBUG("Re-packing Proxy Message, proxy id [%" PRIu32 "], previous msgSize [%u], new msgSize [%u]",
             proxyMessagePtr->commonHeader.id,
             proxyMessagePtr->msgSize,
             count);

    newProxyMessagePtr->msgSize = count;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for pre-processing the Proxy Message before processing the message payload
 */
//--------------------------------------------------------------------------------------------------
static le_result_t PreProcessResponse
(
    void* bufferPtr, ///< [IN] Pointer to buffer
    size_t* bufferSizePtr ///< [IN] Pointer to the size of the buffer
)
{
    le_result_t result;

    // Set a pointer to the common message header
    rpcProxy_CommonHeader_t *commonHeaderPtr = (rpcProxy_CommonHeader_t*) bufferPtr;

    // Convert message id and service-id into Host-Order before processing
    commonHeaderPtr->id = be32toh(commonHeaderPtr->id);
    commonHeaderPtr->serviceId = be32toh(commonHeaderPtr->serviceId);

    switch (commonHeaderPtr->type)
    {
        case RPC_PROXY_CONNECT_SERVICE_REQUEST:
        case RPC_PROXY_CONNECT_SERVICE_RESPONSE:
        case RPC_PROXY_DISCONNECT_SERVICE:
        {
            // Set a pointer to the message
            rpcProxy_ConnectServiceMessage_t* proxyMsgPtr =
                (rpcProxy_ConnectServiceMessage_t*) bufferPtr;

            // Convert service-code into Host-Order before processing
            proxyMsgPtr->serviceCode = be32toh(proxyMsgPtr->serviceCode);
            break;
        }

        case RPC_PROXY_KEEPALIVE_REQUEST:
        case RPC_PROXY_KEEPALIVE_RESPONSE:
        {
            // No further pre-processing to do; break
            break;
        }

        case RPC_PROXY_SERVER_RESPONSE:
        {
            //
            // Step 1. Verify Proxy Message Id is valid
            //

            // Retrieve Message Reference from hash map, using the Proxy Message Id
            le_msg_MessageRef_t msgRef =
                le_hashmap_Get(MsgRefMapByProxyId, (void*)(uintptr_t) commonHeaderPtr->id);

            if (msgRef == NULL)
            {
                LE_ERROR("Unknown Proxy Message Id, service-id ""[%" PRIu32 "], "
                         "proxy id [%" PRIu32 "]; Dropping packet",
                         commonHeaderPtr->serviceId,
                         commonHeaderPtr->id);
                return LE_NOT_FOUND;
            }

            // Retrieve the Session reference, using the Service-ID
            le_msg_ServiceRef_t serviceRef =
                le_hashmap_Get(ServiceRefMapByID, (void*)(uintptr_t) commonHeaderPtr->serviceId);

            if (serviceRef == NULL)
            {
                LE_ERROR("Unknown Service Reference, service-id [%" PRIu32 "]; "
                        " Dropping packet",
                        commonHeaderPtr->serviceId);
                return LE_NOT_FOUND;
            }

            // Fall-through...
        }
        case RPC_PROXY_CLIENT_REQUEST:
        {
            //
            // Step 2. Re-package the message
            //
            rpcProxy_Message_t tmpProxyMessage;
            rpcProxy_Message_t *proxyMessagePtr;

            // Set a Proxy Message pointer to the message
            proxyMessagePtr =
                (rpcProxy_Message_t*) bufferPtr;

            // Put msgSize into Host-Order before processing
            proxyMessagePtr->msgSize = be16toh(proxyMessagePtr->msgSize);

#if RPC_PROXY_HEX_DUMP
            print_hex(proxyMessagePtr->message, proxyMessagePtr->msgSize);
#endif

            // Re-package proxy message before processing
            result = RepackMessage(proxyMessagePtr, &tmpProxyMessage, false);
            if (result != LE_OK)
            {
                return result;
            }

            //
            // Step 3. Prepare the Proxy Common Message Header of the tmpProxyMessage
            //

            // Set the message id, service-id and type
            tmpProxyMessage.commonHeader.id = proxyMessagePtr->commonHeader.id;
            tmpProxyMessage.commonHeader.serviceId = proxyMessagePtr->commonHeader.serviceId;
            tmpProxyMessage.commonHeader.type = proxyMessagePtr->commonHeader.type;

            // Copy the repacked message back into the message receive buffer
            memcpy(proxyMessagePtr, &tmpProxyMessage, (RPC_PROXY_MSG_HEADER_SIZE + tmpProxyMessage.msgSize));

#if RPC_PROXY_HEX_DUMP
            print_hex(proxyMessagePtr->message, proxyMessagePtr->msgSize);
#endif
            break;
        }

        default:
        {
            LE_ERROR("Unknown Proxy Message, type [0x%x]; Dropping packet", commonHeaderPtr->type);
            return LE_FORMAT_ERROR;
            break;
        }
    } // End of switch-statement

    LE_DEBUG("Receiving %s Proxy Message, service-id [%" PRIu32 "], "
             "proxy id [%" PRIu32 "], size [%" PRIuS "]",
             DisplayMessageType(commonHeaderPtr->type),
             commonHeaderPtr->serviceId,
             commonHeaderPtr->id,
             *bufferSizePtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Processing Server Responses
 */
//--------------------------------------------------------------------------------------------------
static void ProcessServerResponse
(
    rpcProxy_Message_t *proxyMessagePtr, ///< {IN] Pointer to the Proxy Message
    bool cleanUpTimer ///< [IN] Boolean to indicate a timer clean-up
)
{
    // Sanity Check - Verify Message Type
    LE_ASSERT(proxyMessagePtr->commonHeader.type == RPC_PROXY_SERVER_RESPONSE);

    // Retrieve Message Reference from hash map, using the Proxy Message Id
    le_msg_MessageRef_t msgRef =
        le_hashmap_Get(MsgRefMapByProxyId, (void*)(uintptr_t) proxyMessagePtr->commonHeader.id);

    if (msgRef == NULL)
    {
        LE_INFO("Error retrieving Message Reference, proxy id [%" PRIu32 "]",
                proxyMessagePtr->commonHeader.id);
        return;
    }

    // Retrieve the Session reference, using the Service-ID
    le_msg_ServiceRef_t serviceRef =
        le_hashmap_Get(ServiceRefMapByID,
                       (void*)(uintptr_t) proxyMessagePtr->commonHeader.serviceId);
    if (serviceRef == NULL)
    {
        LE_INFO("Error retrieving Service Reference, service id [%" PRIu32 "]",
                proxyMessagePtr->commonHeader.serviceId);
        return;
    }

    LE_DEBUG("Successfully retrieved Message Reference, proxy id [%" PRIu32 "]",
             proxyMessagePtr->commonHeader.id);

    // Check if a client response is required
    if (le_msg_NeedsResponse(msgRef))
    {
        // Check if timer needs to be cleaned up
        if (cleanUpTimer)
        {
            // Retrieve and delete timer associated with Proxy Message ID
            le_timer_Ref_t timerRef =
                le_hashmap_Get(
                    ExpiryTimerRefByProxyId,
                    (void*)(uintptr_t) proxyMessagePtr->commonHeader.id);

            if (timerRef != NULL)
            {
                rpcProxy_Message_t* proxyMessageCopyPtr = NULL;

                LE_DEBUG("Deleting timer for Client-Request, "
                         "service-id [%" PRIu32 "], id [%" PRIu32 "]",
                         proxyMessagePtr->commonHeader.serviceId,
                         proxyMessagePtr->commonHeader.id);

                // Remove timer entry associated with Proxy Message ID from hash-map
                le_hashmap_Remove(
                    ExpiryTimerRefByProxyId,
                    (void*)(uintptr_t) proxyMessagePtr->commonHeader.id);

                //
                // Clean up Proxy Message Copy
                //

                // Retrieve ContextPtr data (proxyMessage copy)
                proxyMessageCopyPtr = le_timer_GetContextPtr(timerRef);

                if (proxyMessageCopyPtr == NULL)
                {
                    LE_ERROR("Error extracting copy of Proxy Message from timer record");
                }
                else
                {
                    // Sanity Check - Verify Proxy Message ID and Service-Name
                    if ((proxyMessageCopyPtr->commonHeader.id !=
                            proxyMessagePtr->commonHeader.id) ||
                        (proxyMessageCopyPtr->commonHeader.serviceId !=
                            proxyMessagePtr->commonHeader.serviceId))
                    {
                        // Proxy Messages are different
                        LE_ERROR("Proxy Message Sanity Failure - inconsistent timer record");
                    }

                    LE_DEBUG("Deleting copy of Proxy Message, "
                             "service-id [%" PRIu32 "], id [%" PRIu32 "]",
                             proxyMessageCopyPtr->commonHeader.serviceId,
                             proxyMessageCopyPtr->commonHeader.id);

                    // Free Proxy Message Copy Memory
                    le_mem_Release(proxyMessageCopyPtr);
                }

                // Delete Timer
                le_timer_Delete(timerRef);
                timerRef = NULL;
            }
            else
            {
                LE_ERROR("Unable to find Timer record, proxy id [%" PRIu32 "]",
                         proxyMessagePtr->commonHeader.id);
            }

        } // cleanUpTimer

        // Get the message buffer pointer
        void* msgPtr = le_msg_GetPayloadPtr(msgRef);

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

        // Return the response
        LE_DEBUG("Sending response to client session %p : %u bytes sent",
                 le_msg_GetSession(msgRef),
                 proxyMessagePtr->msgSize);

        // Send response
        le_msg_Respond(msgRef);

    } else {

        LE_DEBUG("Client response not required, session %p",
                 le_msg_GetSession(msgRef));
    }

#ifdef RPC_PROXY_LOCAL_SERVICE
    // Clean-up Local Message memory allocation associated with this Proxy Message ID
    CleanUpLocalMessageResources(proxyMessagePtr->commonHeader.id);
#endif

    // Delete Message Reference from hash map
    le_hashmap_Remove(MsgRefMapByProxyId, (void*)(uintptr_t) proxyMessagePtr->commonHeader.id);
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
    rpcProxy_Message_t proxyMessage;
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
    if (requestResponsePtr->commonHeader.type != RPC_PROXY_REQUEST_RESPONSE)
    {
        LE_ERROR("Unexpected Proxy Message, type [0x%x]",
                 requestResponsePtr->commonHeader.type);
        goto exit;
        return;
    }

    //
    // Build a Proxy Server-Response Message
    //

    // Set the Proxy Message Id, Service Id, and type
    proxyMessage.commonHeader.id = requestResponsePtr->commonHeader.id;
    proxyMessage.commonHeader.serviceId = requestResponsePtr->commonHeader.serviceId;
    proxyMessage.commonHeader.type = RPC_PROXY_SERVER_RESPONSE;

    // Copy the message payload
    memcpy(proxyMessage.message,
           le_msg_GetPayloadPtr(responseMsgRef),
           le_msg_GetMaxPayloadSize(responseMsgRef));

    // Save the message payload size
    proxyMessage.msgSize = le_msg_GetMaxPayloadSize(responseMsgRef);

    // Send a request to the server and get the response.
    LE_DEBUG("Sending response back to RPC Proxy : %u bytes sent",
             proxyMessage.msgSize);

    // Send Proxy Message to the far-side RPC Proxy
    result = rpcProxy_SendMsg(requestResponsePtr->systemName, &proxyMessage);
    if (result != LE_OK)
    {
        LE_ERROR("le_comm_Send failed, result %d", result);
    }


exit:
#ifdef RPC_PROXY_LOCAL_SERVICE
    // Clean-up Local Message memory allocation associated with this Proxy Message ID
    CleanUpLocalMessageResources(requestResponsePtr->commonHeader.id);
#endif

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
    const char* systemName, ///< [IN] Name of the system that sent the Client-Request
    rpcProxy_Message_t* proxyMessagePtr ///< [IN] Pointer to the Proxy Message
)
{
    //
    // Send Client Message to the Server
    //
    le_msg_MessageRef_t msgRef;
    le_msg_SessionRef_t sessionRef;
    void*               msgPtr;

    // Sanity Check - Verify Message Type
    LE_ASSERT(proxyMessagePtr->commonHeader.type == RPC_PROXY_CLIENT_REQUEST);

#ifndef RPC_PROXY_UNIT_TEST
    //
    // Create a new message object and get the message buffer
    //

    // Retrieve the Session reference for the specified Service-ID
    sessionRef =
        le_hashmap_Get(
            SessionRefMapByID,
            (void*)(uintptr_t) proxyMessagePtr->commonHeader.serviceId);

    if (sessionRef == NULL)
    {
        LE_ERROR("Unable to find matching Session Reference "
                 "in hashmap, service-id [%" PRIu32 "]",
                 proxyMessagePtr->commonHeader.serviceId);

        // Generate LE_UNAVAILABLE Server-Response
        GenerateServerResponseErrorMessage(
            proxyMessagePtr,
            LE_UNAVAILABLE);

        // Send the Response to the far-side
        le_result_t result =
            rpcProxy_SendMsg(systemName, proxyMessagePtr);

        if (result != LE_OK)
        {
            LE_ERROR("le_comm_Send failed, result %d", result);
        }

#ifdef RPC_PROXY_LOCAL_SERVICE
        // Clean-up Local Message memory allocation associated with this Proxy Message ID
        CleanUpLocalMessageResources(proxyMessagePtr->commonHeader.id);
#endif

        return LE_UNAVAILABLE;
    }

    LE_DEBUG("Successfully retrieved Session Reference, "
             "session safe reference [%" PRIuPTR "]",
             (uintptr_t) sessionRef);

    // Create Client Message
    msgRef = le_msg_CreateMsg(sessionRef);

    msgPtr = le_msg_GetPayloadPtr(msgRef);

    // Copy Proxy Message content into the out-going Message
    memcpy(msgPtr, proxyMessagePtr->message, proxyMessagePtr->msgSize);

    LE_DEBUG("Sending message to server and waiting for response : %u bytes sent",
             proxyMessagePtr->msgSize);

    LE_DEBUG("Allocating memory for Request-Response Record, "
             "service-id [%" PRIu32 "], proxy id [%" PRIu32 "]",
             proxyMessagePtr->commonHeader.serviceId,
             proxyMessagePtr->commonHeader.id);

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
#ifdef RPC_PROXY_LOCAL_SERVICE
        uint32_t proxyId = proxyMessagePtr->commonHeader.id;
#endif

        LE_WARN("Request-Response Record memory pool is exhausted, "
                "service-id [%" PRIu32 "], proxy id [%" PRIu32 "] - "
                "Dropping request and returning error",
                proxyMessagePtr->commonHeader.serviceId,
                proxyMessagePtr->commonHeader.id);

        //
        // Generate a LE_NO_MEMORY event
        //

        // Generate LE_NO_MEMORY Server-Response
        GenerateServerResponseErrorMessage(
            proxyMessagePtr,
            LE_NO_MEMORY);

        // Send the Response to the far-side
        le_result_t result = rpcProxy_SendMsg(systemName, proxyMessagePtr);
        if (result != LE_OK)
        {
            LE_ERROR("le_comm_Send failed, result %d", result);
        }

#ifdef RPC_PROXY_LOCAL_SERVICE
        // Clean-up Local Message memory allocation associated with this Proxy Message ID
        CleanUpLocalMessageResources(proxyId);
#endif

        return LE_NO_MEMORY;
    }

    //
    // Build the Request-Response record
    //

    // Set the Proxy Message ID, Service-Id, and type
    requestResponsePtr->commonHeader.id = proxyMessagePtr->commonHeader.id;
    requestResponsePtr->commonHeader.serviceId = proxyMessagePtr->commonHeader.serviceId;
    requestResponsePtr->commonHeader.type = RPC_PROXY_REQUEST_RESPONSE;

    // Copy the Source of the request (system-name) so we know who to send the response back to
    le_utf8_Copy(requestResponsePtr->systemName,
                 systemName,
                 sizeof(requestResponsePtr->systemName), NULL);

    // Copy the Legato Msg-Id, in the event we need to generate a
    // timeout response back to the requesting system
    memcpy(&requestResponsePtr->msgId, &proxyMessagePtr->message[0], LE_PACK_SIZEOF_UINT32);

    // Send an asynchronous request-response to the server.
    le_msg_RequestResponse(msgRef, &ServerResponseCompletionCallback,
                           (void*)(uintptr_t) proxyMessagePtr->commonHeader.id);

    // Store the Request-Response Record Ptr in a hashmap,
    // using the Proxy Message ID as a key, so that it can be retreived later by either
    // requestResponseTimerExpiryHandler() or ServerResponseCompletionCallback()
    le_hashmap_Put(RequestResponseRefByProxyId,
                   (void*)(uintptr_t) proxyMessagePtr->commonHeader.id,
                   requestResponsePtr);

#else
    // Evaluate Unit-Test results
    rpcDaemonTest_ProcessClientRequest(proxyMessagePtr);
#endif

    return LE_OK;
}


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

    // Generate a Disconnect-Service event to inform the far-side that this
    // service is no longer available
    SendDisconnectService(
        systemName,
        bindingRefPtr->serviceName,
        bindingRefPtr->protocolIdStr);
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

    rpcProxy_Message_t* proxyMessageCopyPtr = NULL;

    LE_DEBUG("Deleting timer for Connect-Service Request, "
             "service-id [%" PRIu32 "]",
             serviceId);

    // Remove timer entry associated with Service-ID from hash-map
    le_hashmap_Remove(ExpiryTimerRefByServiceId, (void*)(uintptr_t) serviceId);

    //
    // Clean up Proxy Message Copy
    //

    // Retrieve ContextPtr data (proxyMessage copy)
    proxyMessageCopyPtr = le_timer_GetContextPtr(timerRef);

    if (proxyMessageCopyPtr == NULL)
    {
        LE_ERROR("Error extracting copy of Proxy Message from timer record");
        return LE_FAULT;
    }

    LE_DEBUG("Deleting copy of Proxy Message, "
             "service-id [%" PRIu32 "]",
             serviceId);

    // Free Proxy Message Copy Memory
    le_mem_Release(proxyMessageCopyPtr);

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
    rpcProxy_ConnectServiceMessage_t* proxyMessagePtr ///< [IN] Pointer to the Proxy Message
)
{
    le_result_t result = LE_OK;

    // Sanity Check - Verify Message Type
    LE_ASSERT(proxyMessagePtr->commonHeader.type == RPC_PROXY_CONNECT_SERVICE_RESPONSE);

    // Check if service has been established successfully on the far-side
    if (proxyMessagePtr->serviceCode != LE_OK)
    {
        // Remote-side failed to set-up service
        LE_INFO("%s failed, serviceId "
                "[%" PRIu32 "], service-code [%" PRIi32 "] - retry later",
                DisplayMessageType(proxyMessagePtr->commonHeader.type),
                proxyMessagePtr->commonHeader.serviceId,
                proxyMessagePtr->serviceCode);

        return proxyMessagePtr->serviceCode;
    }

    // Delete and clean-up the Connect-Service-Request timer
    DeleteConnectServiceRequestTimer(proxyMessagePtr->commonHeader.serviceId);

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
            rpcProxyConfig_GetServiceNameByRemoteServiceName(proxyMessagePtr->serviceName);

        if (serviceName == NULL)
        {
            LE_ERROR("Unable to retrieve service-name for remote service-name '%s'",
                     proxyMessagePtr->serviceName);
            return LE_FAULT;
        }

        // Compare the system-name, remote service-name, and protocol-Id-str
        if ((strcmp(systemName, proxyMessagePtr->systemName) == 0) &&
            (strcmp(serviceRefPtr->serviceName, serviceName) == 0) &&
            (strcmp(serviceRefPtr->protocolIdStr, proxyMessagePtr->protocolIdStr) == 0))
        {
            le_msg_ServiceRef_t tmpServiceRef;

            // Check to see if service is already advertised (UP)
            // Retrieve the Service reference, using the Service-ID
            tmpServiceRef =
                le_hashmap_Get(
                    ServiceRefMapByID,
                    (void*)(uintptr_t) proxyMessagePtr->commonHeader.serviceId);

            if (tmpServiceRef == NULL)
            {
                le_msg_ServiceRef_t serviceRef = NULL;

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

                // Allocate memory from Service Name string pool to hold the service-name
                char* serviceNameCopyPtr = le_mem_ForceAlloc(ServiceNameStringPoolRef);

                // Copy the service-name string
                le_utf8_Copy(serviceNameCopyPtr,
                             serviceRefPtr->serviceName,
                             LIMIT_MAX_IPC_INTERFACE_NAME_BYTES, NULL);

                // Allocate memory from Service-ID pool
                uint32_t* serviceIdCopyPtr = le_mem_ForceAlloc(ServiceIdPoolRef);

                // Copy the Service-ID
                *serviceIdCopyPtr = proxyMessagePtr->commonHeader.serviceId;

                // Store the Service-ID in a hashmap, using the service-name as a key
                le_hashmap_Put(ServiceIDMapByName, serviceNameCopyPtr, serviceIdCopyPtr);

                LE_INFO("Successfully saved Service Reference ID, "
                        "service-name [%s], service-id [%" PRIu32 "]",
                        serviceNameCopyPtr,
                        *serviceIdCopyPtr);

                // Store the serviceRef in a hashmap, using the Service-ID as a key
                le_hashmap_Put(ServiceRefMapByID,
                               (void*)(uintptr_t) proxyMessagePtr->commonHeader.serviceId, serviceRef);

                LE_INFO("Successfully saved Service Reference, "
                        "service safe reference [%" PRIuPTR "], service-id [%" PRIu32 "]",
                        (uintptr_t) serviceRef,
                        proxyMessagePtr->commonHeader.serviceId);

#ifndef RPC_PROXY_LOCAL_SERVICE
                // Add a handler for client session close
                le_msg_AddServiceCloseHandler(serviceRef, ServerCloseSessionHandler, (void*) serviceRefPtr);

                // Add a handler for client session open
                le_msg_AddServiceOpenHandler(serviceRef, ServerOpenSessionHandler, (void*) serviceRefPtr);
#endif

                // Start the server side of the service
                le_msg_SetServiceRecvHandler(serviceRef, ServerMsgRecvHandler, (void *)serviceRefPtr);
                le_msg_AdvertiseService(serviceRef);
            }
        }
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for Processing Session Connect Request
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessConnectServiceRequest
(
    const char* systemName, ///< [IN] Name of the system that sent the Connect-Service Request
    rpcProxy_ConnectServiceMessage_t* proxyMessagePtr ///< [IN] Pointer to the Proxy Message
)
{
    le_result_t  result;

    // Sanity Check - Verify Message Type
    LE_ASSERT(proxyMessagePtr->commonHeader.type == RPC_PROXY_CONNECT_SERVICE_REQUEST);

    LE_INFO("======= Starting RPC Proxy client for '%s' service, '%s' protocol ========",
            proxyMessagePtr->serviceName, proxyMessagePtr->protocolIdStr);

    // Generate Do-Connect-Service call on behalf of the remote client
    result = DoConnectService(proxyMessagePtr->serviceName,
                              proxyMessagePtr->commonHeader.serviceId,
                              proxyMessagePtr->protocolIdStr);
    //
    // Prepare a Session-Connect-Response Proxy Message
    //

    // Set the Proxy Message type to CONNECT_SERVICE_RESPONSE
    proxyMessagePtr->commonHeader.type = RPC_PROXY_CONNECT_SERVICE_RESPONSE;

    // Set the service-code with the DoConnectService result-code
    proxyMessagePtr->serviceCode = result;

    // Send Proxy Message to far-side
    result = rpcProxy_SendMsg(systemName, proxyMessagePtr);
    if (result != LE_OK)
    {
        LE_ERROR("le_comm_Send failed, result %d", result);
    }

    return result;
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
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for Processing Disconnect Service Request
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessDisconnectService
(
    const char* systemName, ///< [IN] Name of the system that sent the Connect-Service Request
    rpcProxy_ConnectServiceMessage_t* proxyMessagePtr ///< [IN] Pointer to the Proxy Message
)
{
    // Sanity Check - Verify Message Type
    LE_ASSERT(proxyMessagePtr->commonHeader.type == RPC_PROXY_DISCONNECT_SERVICE);

    LE_INFO("======= Stopping RPC Proxy server for '%s' service, '%s' protocol ========",
            proxyMessagePtr->serviceName, proxyMessagePtr->protocolIdStr);

    const char* serviceName =
        rpcProxyConfig_GetServiceNameByRemoteServiceName(proxyMessagePtr->serviceName);

    if (serviceName == NULL)
    {
        LE_ERROR("Unable to retrieve service-name for remote service-name '%s'",
                 proxyMessagePtr->serviceName);
        return LE_FAULT;
    }

    // Delete the Service associated with the service-name
    DeleteService(serviceName);

    //
    // Clean-up Service ID Safe Reference for this service-name, if it exists
    //
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ServiceIDSafeRefMap);

    // Iterate over all Service-ID Safe References looking for the service-name match
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        if (strcmp(le_ref_GetValue(iterRef), serviceName) == 0)
        {
            LE_INFO("Releasing Service ID Safe Reference, "
                    "service-name [%s], service-id [%" PRIuPTR "]",
                    (char*) le_ref_GetValue(iterRef),
                    (uintptr_t) le_ref_GetSafeRef(iterRef));

            // Free the Service-ID Safe Reference now that the Service is being deleted
            le_ref_DeleteRef(ServiceIDSafeRefMap, (void*) le_ref_GetSafeRef(iterRef));
            break;
        }
    }

    // Send Connect-Service Message to the far-side for the specified service-name
    // and wait for a valid Connect-Service response before advertising the service
    SendSessionConnectRequest(
        systemName,
        proxyMessagePtr->serviceName,
        proxyMessagePtr->protocolIdStr);

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
    char                     buffer[RPC_PROXY_RECV_BUFFER_MAX];
    size_t                   bufferSize;
    rpcProxy_CommonHeader_t *commonHeaderPtr;
    le_result_t              result;

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

        bufferSize = sizeof(buffer);

        // Receive Proxy Message from far-side
        result = RecvMsg(handle, buffer, &bufferSize);

        if (result != LE_OK)
        {
            if ((result != LE_FORMAT_ERROR) &&
                (result != LE_NOT_FOUND) &&
                (result != LE_BAD_PARAMETER))
            {
                LE_ERROR("le_comm_Receive failed, result %d", result);

                // Delete the Network Communication Channel, using the communication handle
                rpcProxyNetwork_DeleteNetworkCommunicationChannelByHandle(handle);
            }
            // Do not proceed any further - return
            return;
        }

        // Set a pointer to the common message header
        commonHeaderPtr = (rpcProxy_CommonHeader_t*) buffer;

        // Test the Proxy Message type and dispatch the event
        switch (commonHeaderPtr->type)
        {
            case RPC_PROXY_KEEPALIVE_REQUEST:
            {
                LE_DEBUG("Received Proxy KEEPALIVE-Request Message, id [%" PRIu32 "]",
                         commonHeaderPtr->id);

                result = rpcProxyNetwork_ProcessKeepAliveRequest(
                             systemName,
                             (rpcProxy_KeepAliveMessage_t*) buffer);
                break;
            }

            case RPC_PROXY_KEEPALIVE_RESPONSE:
            {
                LE_DEBUG("Received Proxy KEEPALIVE-Response Message, id [%" PRIu32 "]",
                         commonHeaderPtr->id);

                result = rpcProxyNetwork_ProcessKeepAliveResponse(
                            systemName,
                            (rpcProxy_KeepAliveMessage_t*) buffer);
                break;
            }

            case RPC_PROXY_CLIENT_REQUEST:
            {
                LE_DEBUG("Received Proxy Client-Request Message, id [%" PRIu32 "]",
                         commonHeaderPtr->id);

                result = ProcessClientRequest(systemName, (rpcProxy_Message_t*) buffer);
                break;
            }

            case RPC_PROXY_SERVER_RESPONSE:
            {
                LE_DEBUG("Received Proxy Server-Response Message, proxy id [%" PRIu32 "]",
                         commonHeaderPtr->id);
                ProcessServerResponse((rpcProxy_Message_t*) buffer, true);
                break;
            }

            case RPC_PROXY_CONNECT_SERVICE_REQUEST:
            {
                LE_DEBUG("Received Proxy Connect-Service-Request Message, id [%" PRIu32 "]",
                         commonHeaderPtr->id);
                result =
                    ProcessConnectServiceRequest(
                        systemName,
                        (rpcProxy_ConnectServiceMessage_t*) buffer);
                break;
            }

            case RPC_PROXY_CONNECT_SERVICE_RESPONSE:
            {
                LE_DEBUG("Received Proxy Connect-Service-Response Message, id [%" PRIu32 "]",
                         commonHeaderPtr->id);
                result =
                    ProcessConnectServiceResponse(
                        (rpcProxy_ConnectServiceMessage_t*) buffer);
                break;
            }

            case RPC_PROXY_DISCONNECT_SERVICE:
            {
                LE_DEBUG("Received Proxy Disconnect-Service Message, id [%" PRIu32 "]",
                        commonHeaderPtr->id);
                result =
                    ProcessDisconnectService(
                        systemName,
                        (rpcProxy_ConnectServiceMessage_t*) buffer);
                break;
            }

            default:
            {
                LE_ERROR("Un-expected Proxy Message, type [0x%x], id [%" PRIu32 "]",
                         commonHeaderPtr->type,
                         commonHeaderPtr->id);
                return;
                break;
            }
        } // End of switch-statement
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
    rpcProxy_Message_t  proxyMessage;
    rpcProxy_Message_t *proxyMessageCopyPtr = NULL;
    le_result_t         result;

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

    // Copy the Message Reference payload into the Proxy Message
    memcpy(proxyMessage.message, le_msg_GetPayloadPtr(msgRef), le_msg_GetMaxPayloadSize(msgRef));

    // Save the message payload size
    proxyMessage.msgSize = le_msg_GetMaxPayloadSize(msgRef);

    LE_DEBUG("Received message from client, msgSize [%u]", proxyMessage.msgSize);


    // Set the Proxy Message common header id and type
    proxyMessage.commonHeader.id = rpcProxy_GenerateProxyMessageId();
    proxyMessage.commonHeader.type = RPC_PROXY_CLIENT_REQUEST;

    // Cache Message Reference to use later.
    // Store the Message Reference in a hash map using the proxy Id as the key.
    le_hashmap_Put(MsgRefMapByProxyId, (void*)(uintptr_t) proxyMessage.commonHeader.id, msgRef);

    // Retrieve the Service-ID for the specified service-name
    uint32_t* serviceIdPtr = le_hashmap_Get(ServiceIDMapByName, serviceName);
    if (serviceIdPtr == NULL)
    {
        // Raise an error message and return
        LE_ERROR("Service is not available, service-name [%s]", serviceName);
        goto exit;
    }

    proxyMessage.commonHeader.serviceId = *serviceIdPtr;

    if (sizeof(proxyMessage.message) < le_msg_GetMaxPayloadSize(msgRef))
    {
        // Raise an error message and return
        LE_ERROR("Proxy Message buffer too small");
        goto exit;
    }

    // Check if client requires a response
    if (le_msg_NeedsResponse(msgRef))
    {
        // Allocate memory for a Proxy Message copy
        proxyMessageCopyPtr = le_mem_ForceAlloc(ProxyMessagesPoolRef);

        // Make a copy of the Proxy Message
        // (NOTE: Needs to be done prior to calling SendMsg)
        memcpy(proxyMessageCopyPtr,
               &proxyMessage,
               (RPC_PROXY_MSG_HEADER_SIZE + proxyMessage.msgSize));
    }

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to '%s' RPC Proxy and waiting for response : %u bytes sent",
             systemName,
             proxyMessage.msgSize);

    // Send Proxy Message to far-side
    result = rpcProxy_SendMsg(systemName, &proxyMessage);
    if (result != LE_OK)
    {
        LE_ERROR("le_comm_Send failed, result %d", result);
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
        le_timer_SetHandler(clientRequestTimerRef, rpcProxy_ProxyMessageTimerExpiryHandler);
        le_timer_SetWakeup(clientRequestTimerRef, false);

        // Set Proxy Message (copy) in the timer event
        le_timer_SetContextPtr(clientRequestTimerRef, proxyMessageCopyPtr);

        // Start timer
        le_timer_Start(clientRequestTimerRef);

        // Store the timerRef in a hashmap, using the Proxy Message ID as a key, so that
        // it can be retrieved later
        le_hashmap_Put(ExpiryTimerRefByProxyId,
                       (void*)(uintptr_t) proxyMessageCopyPtr->commonHeader.id,
                       clientRequestTimerRef);

        LE_DEBUG("Starting timer for Client-Request, service-id [%" PRIu32 "], id [%" PRIu32 "]",
                 proxyMessageCopyPtr->commonHeader.serviceId,
                 proxyMessageCopyPtr->commonHeader.id);
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
    rpcProxy_ConnectServiceMessage_t  proxyMessage;
    uint32_t                          serviceId;
    le_result_t                       result;

    //
    // Create a Session Connect Proxy Message
    //

    // Generate the proxy message id
    proxyMessage.commonHeader.id = rpcProxy_GenerateProxyMessageId();

    // Generate the Service-ID, using a safe reference
    proxyMessage.commonHeader.serviceId = serviceId =
        (uint32_t)(uintptr_t) le_ref_CreateRef(ServiceIDSafeRefMap, (void*) serviceInstanceName);

    proxyMessage.commonHeader.type = RPC_PROXY_CONNECT_SERVICE_REQUEST;

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

    // Allocate memory for a Proxy Message copy
    rpcProxy_ConnectServiceMessage_t *proxyMessageCopyPtr =
        le_mem_ForceAlloc(ProxyConnectServiceMessagesPoolRef);

    // Make a copy of the Proxy Message
    // (NOTE: Needs to be done prior to calling SendMsg)
    memcpy(proxyMessageCopyPtr,
           &proxyMessage,
           sizeof(rpcProxy_ConnectServiceMessage_t));

    // Send Proxy Message to far-side
    result = rpcProxy_SendMsg(systemName, &proxyMessage);
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
    le_timer_SetHandler(connectServiceTimerRef, rpcProxy_ProxyMessageTimerExpiryHandler);
    le_timer_SetWakeup(connectServiceTimerRef, false);

    // Set Proxy Message (copy) in the timer event
    le_timer_SetContextPtr(connectServiceTimerRef, proxyMessageCopyPtr);

    // Start timer
    le_timer_Start(connectServiceTimerRef);

    // Store the timerRef in a hashmap, using the Service-ID as a key, so that
    // it can be retrieved later
    le_hashmap_Put(ExpiryTimerRefByServiceId,
                   (void*)(uintptr_t) proxyMessageCopyPtr->commonHeader.serviceId,
                   connectServiceTimerRef);

    LE_INFO("Connecting to service '%s' - starting retry timer, service-id [%" PRIu32 "]",
            proxyMessageCopyPtr->serviceName,
            proxyMessageCopyPtr->commonHeader.serviceId);
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
            char* serviceNameCopyPtr = le_mem_ForceAlloc(ServiceNameStringPoolRef);

            le_utf8_Copy(serviceNameCopyPtr,
                         bindingRefPtr->serviceName,
                         LIMIT_MAX_IPC_INTERFACE_NAME_BYTES,
                         NULL);

            // Allocate memory from Service ID pool
            uint32_t* serviceIdCopyPtr = le_mem_ForceAlloc(ServiceIdPoolRef);
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
            LE_ERROR("Unable to retrieve system-name for service-name '%s'",
                     serviceRefPtr->serviceName);
            return;
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
            LE_ERROR("Unable to retrieve system-name for service-name '%s'",
                     serviceRefPtr->serviceName);
            return;
        }

        // Only interested in those services on the specified system-name
        if (strcmp(systemName, systemNamePtr) != 0)
        {
            continue;
        }

        // Delete the Service associated with the service-name
        DeleteService(serviceRefPtr->serviceName);

        //
        // Clean-up Service ID Safe Reference for this service-name, if it exists
        //
        le_ref_IterRef_t iterRef = le_ref_GetIterator(ServiceIDSafeRefMap);

        // Iterate over all Service-ID Safe References looking for the service-name match
        while (le_ref_NextNode(iterRef) == LE_OK)
        {
            if (strcmp(le_ref_GetValue(iterRef), serviceRefPtr->serviceName) == 0)
            {
                LE_INFO("Releasing Service ID Safe Reference, "
                        "service-name [%s], service-id [%" PRIuPTR "]",
                        (char*) le_ref_GetValue(iterRef),
                        (uintptr_t) le_ref_GetSafeRef(iterRef));

                // Free the Service-ID Safe Reference now that the Service is being deleted
                le_ref_DeleteRef(ServiceIDSafeRefMap, (void*) le_ref_GetSafeRef(iterRef));
                break;
            }
        }
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

#ifdef RPC_PROXY_LOCAL_SERVICE
                        // Clean-up Local Message memory allocation associated with this Proxy Message ID
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


            // Get the stored key object
            char* serviceNameCopyPtr =
                le_hashmap_GetStoredKey(
                    ServiceIDMapByName,
                    sessionRefPtr->serviceName);

            // Remove the serviceId in a hashmap, using the service-name as a key
            le_hashmap_Remove(ServiceIDMapByName, serviceNameCopyPtr);

            // Free the memory allocated for the Service-ID
            le_mem_Release(serviceIdCopyPtr);

            // Free the memory allocated for the Service Name string
            le_mem_Release(serviceNameCopyPtr);
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

    return LE_OK;
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

    ServiceNameStringPoolRef = le_mem_InitStaticPool(ServiceNameStringPool,
                                                     RPC_PROXY_SERVICE_BINDINGS_MAX_NUM,
                                                     RPC_PROXY_MSG_SERVICE_NAME_SIZE);

    ServiceIdPoolRef = le_mem_InitStaticPool(ServiceIdPool,
                                             RPC_PROXY_SERVICE_BINDINGS_MAX_NUM,
                                             sizeof(uint32_t));

    ProxyMessagesPoolRef = le_mem_InitStaticPool(ProxyMessagePool,
                                                 RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                                                 sizeof(rpcProxy_Message_t));

    ProxyConnectServiceMessagesPoolRef = le_mem_InitStaticPool(
                                             ProxyConnectServiceMessagePool,
                                             RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                                             sizeof(rpcProxy_ConnectServiceMessage_t));

    ProxyClientRequestResponseRecordPoolRef = le_mem_InitStaticPool(
                                                  ProxyClientRequestResponseRecordPool,
                                                  RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                                                  sizeof(rpcProxy_ClientRequestResponseRecord_t));

#ifdef RPC_PROXY_LOCAL_SERVICE
    MessageDataPtrPoolRef = le_mem_InitStaticPool(MessageDataPtrPool,
                                                  RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                                                  RPC_LOCAL_MAX_MESSAGE);

    LocalMessagePoolRef = le_mem_InitStaticPool(LocalMessagePool,
                                                RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                                                sizeof(rpcProxy_LocalMessage_t));

    ResponseParameterArrayPoolRef = le_mem_InitStaticPool(
                                        ResponseParameterArrayPool,
                                        RPC_PROXY_MSG_REFERENCE_MAX_NUM,
                                        sizeof(responseParameterArray_t));
#endif

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
                                                    RPC_PROXY_MSG_REFERENCE_MAX_NUM,
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

    // Create hash map for response "OUT" parameter pointers, using the Proxy Message ID (key).
    ResponseParameterArrayByProxyId = le_hashmap_InitStatic(
                                          ResponseParameterArrayHashMap,
                                          RPC_PROXY_MSG_OUT_PARAMETER_MAX_NUM,
                                          le_hashmap_HashVoidPointer,
                                          le_hashmap_EqualsVoidPointer);

    LE_INFO("RPC Proxy Service Init start");

    // Initialize the RPC Proxy Configuration service before accessing
    result = rpcProxyConfig_Initialize();
    if (result != LE_OK)
    {
        LE_ERROR("Error initializing RPC Proxy Network services, result [%d]", result);
        goto exit;
    }

    // Load the ConfigTree configuration for links, bindings and references
    result = rpcProxyConfig_LoadSystemLinks();
    if (result != LE_OK)
    {
        LE_ERROR("Unable to load System-Links configuration, result [%d]", result);
        goto exit;
    }

    result = rpcProxyConfig_LoadReferences();
    if (result != LE_OK)
    {
        LE_ERROR("Unable to load References configuration, result [%d]", result);
        goto exit;
    }

    result = rpcProxyConfig_LoadBindings();
    if (result != LE_OK)
    {
        LE_ERROR("Unable to load Bindings configuration, result [%d]", result);
        goto exit;
    }

    result = rpcProxyConfig_ValidateConfiguration();
    if (result != LE_OK)
    {
        LE_ERROR("Configuration validation error, result [%d]", result);
        goto exit;
    }

    // Initialize the RPC Proxy Network services
    result = rpcProxyNetwork_Initialize();
    if (result != LE_OK)
    {
        LE_ERROR("Error initializing RPC Proxy Network services, result [%d]", result);
        goto exit;
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
            goto exit;
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
            goto exit;
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

exit:
    LE_INFO("RPC Proxy Service Init done");

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