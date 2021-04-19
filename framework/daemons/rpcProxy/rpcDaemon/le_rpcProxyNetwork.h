/**
 * @file le_rpcProxyNetwork.h
 *
 * Header file for RPC Proxy Network definitions and functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_RPC_PROXY_NETWORK_H_INCLUDE_GUARD
#define LE_RPC_PROXY_NETWORK_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Remote-host Systems within a Network.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_NETWORK_SYSTEM_MAX_NUM        1


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Network Timer Records
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_NETWORK_TIMER_RECORD_MAX_NUM  (RPC_PROXY_NETWORK_SYSTEM_MAX_NUM * 2)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum receive buffer size must be set to maximum size of RPC message structure.
 */
//--------------------------------------------------------------------------------------------------
union allMessages{
    rpcProxy_ConnectServiceMessage_t connectServiceMessage;
    rpcProxy_KeepAliveMessage_t keepAliveMessage;
    rpcProxy_Message_t ipcMessage;
    rpcProxy_FileStreamMessage_t fileStreamMessage;
};
#define RPC_PROXY_RECV_BUFFER_MAX  sizeof(union allMessages)


//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Network Operational State definition
 */
//--------------------------------------------------------------------------------------------------
typedef enum NetworkState
{
    NETWORK_UP,
    NETWORK_DOWN
}
NetworkState_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Network Connection Type definition - Determined by the LE_COMM API implementation
 */
//--------------------------------------------------------------------------------------------------
typedef enum NetworkConnectionType
{
    UNKNOWN,
    SYNC,     ///< Connections are handled synchronously (blocking)
    ASYNC     ///< Connections are handled asynchronously (non-blocking)
}
NetworkConnectionType_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Network Message Receive State
 */
//--------------------------------------------------------------------------------------------------
typedef enum NetworkMessageReceiveState
{
    NETWORK_MSG_IDLE = 0,       ///< IDLE State
    NETWORK_MSG_PARTIAL_HEADER, ///< Partial HEADER State
    NETWORK_MSG_HEADER,         ///< Complete HEADER State
    NETWORK_MSG_STREAM,         ///< Streaming a message
    NETWORK_MSG_DONE            ///< DONE - Complete message received
}
NetworkMessageReceiveState_t;

//--------------------------------------------------------------------------------------------------
/**
 * State of streaming
 *
 * The NETWORK_MSG_STREAM state of NetworkMessageReceiveState_t itself is handled in another state
 * machine. Below are the states of that state machine:
 */
//--------------------------------------------------------------------------------------------------
typedef enum MessageStreamState
{
    STREAM_IDLE,                ///< Initial state for stream state machine
    STREAM_MSG_ID,              ///< Expecting IPC message ID
    STREAM_CONSTANT_LENGTH_MSG, ///< Expecting message body for non variable length messages.
    STREAM_ASYNC_EVENT_INIT,    ///< Expecting first bytes of an aync event message
    STREAM_CBOR_HEADER,         ///< Expecting CBOR header byte of an item
    STREAM_CBOR_ITEM_BODY,      ///< Expecting body of a cbor byte string or txt string item
    STREAM_INTEGER_ITEM,        ///< Expecting an integer CBOR item.
    STREAM_DONE                 ///< Streaming is done.
} MessageStreamState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Structure to hold the streaming state:
 */
//--------------------------------------------------------------------------------------------------
typedef struct StreamState
{
    MessageStreamState_t state;      ///< Stream state
    char workBuff[16];               ///< Temp buffer for state machine
    size_t expectedSize;             ///< Number of bytes that needed to be read
    size_t recvSize;                 ///< Number of bytes that have been read so far
    le_msg_MessageRef_t msgRef;      ///< Messsage reference for the message being streamed
    size_t ipcMsgPayloadOffset;      ///< Offset in the ipc message buffer
    size_t msgBuffSizeLeft;          ///< Number of bytes left in msg buff size
    void* destBuff;                  ///< Destination buffer
    le_pack_SemanticTag_t lastTag;   ///< Last semantic tag observed
    unsigned int nextItemDispatchIdx;///< An index to the dispatch table that indicates where
                                     /// handling of next item should be dispatched to.
    unsigned int collectionsLayer;   ///< Collections layer.
    bool isAsyncMsg;                 ///< Determines whether this is an async message.
    uint32_t asyncMsgId;             ///< Stores message id for async messages
#ifdef RPC_PROXY_LOCAL_SERVICE
    uint8_t slotIndex;               ///< Slot index for optimization of local service messages
    le_dls_List_t localBuffers;      ///< List of local buffers which have been created for
                                     ///< optimizaiton of local messages
#endif
} StreamState_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Network Message Re-assembly State-Machine structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct NetworkMessageState
{
    char     buffer[RPC_PROXY_RECV_BUFFER_MAX]; ///< Receive Message Buffer
    NetworkMessageReceiveState_t  recvState;    ///< Receive Message State
    size_t   expectedSize; ///< Number of bytes that needed to be read
    size_t   recvSize;     ///< Number of bytes that have been read so far
    size_t   offSet;       ///< Offset into the receive buffer where new data should be written
    uint8_t  type;         ///< Message Type (RPC_PROXY_CONNECT_SERVICE_REQUEST,
                           ///< RPC_PROXY_CONNECT_SERVICE_RESPONSE, etc.)
    StreamState_t streamState; ///< Holds the state information for streaming messages
}
NetworkMessageState_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Network Record structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct NetworkRecord
{
    void*                    handle;    ///< Opaque handle to the network connection
    NetworkState_t           state;     ///< Operational state of the network connection
    NetworkConnectionType_t  type;      ///< Type of network connection
    le_timer_Ref_t           keepAliveTimerRef; ///< Keep-Alive Timer Ref
    NetworkMessageState_t    messageState; ///< Message Re-assembly State-Machine
}
NetworkRecord_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Network Timer Record Structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct NetworkTimerRecord
{
    enum {
        RECONNECT,
        KEEPALIVE,
    } event;  ///< Type of Timer Event

    char systemName[LIMIT_MAX_SYSTEM_NAME_BYTES]; ///< Name of Destination System
    NetworkRecord_t record;  ///< Network Record for the Destination System
}
NetworkTimerRecord_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Network Function prototypes
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the systemName using the communication handle (reverse look-up).
 */
//--------------------------------------------------------------------------------------------------
char* rpcProxyNetwork_GetSystemNameByHandle
(
    void* handle ///< Opaque handle to a le_comm communication channel
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the Network Record, using the communication handle (reverse look-up).
 */
//--------------------------------------------------------------------------------------------------
NetworkRecord_t* rpcProxyNetwork_GetNetworkRecordByHandle
(
    void* handle ///< Opaque handle to a le_comm communication channel
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the Network Record Hash-map By Name reference.
 */
//--------------------------------------------------------------------------------------------------
le_hashmap_Ref_t rpcProxyNetwork_GetNetworkRecordHashMapByName
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for creating and connecting a Network Communication Channel.
 *
 * @return
 *      - LE_OK, if successfully,
 *      - LE_WAITING, if pending on asynchronous connection,
 *      - otherwise failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyNetwork_CreateNetworkCommunicationChannel
(
    const char* systemName ///< System name
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for deleting a Network Communication Channel, using system-name.
 *
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyNetwork_DeleteNetworkCommunicationChannel
(
    const char* systemName ///< System name
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for deleting a Network Communication Channel, using the communication handle.
 *
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyNetwork_DeleteNetworkCommunicationChannelByHandle
(
    void* handle ///< Opaque handle to a le_comm communication channel
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for connecting a Network Communication Channel.
 *
 * @return
 *      - LE_OK, if successfully,
 *      - otherwise failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyNetwork_ConnectNetworkCommunicationChannel
(
    void* handle ///< Opaque handle to a le_comm communication channel
);

//--------------------------------------------------------------------------------------------------
/**
 * Start Network Connection Retry timer.
 *
 * @return
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyNetwork_StartNetworkConnectionRetryTimer
(
    const char* systemName ///< System name
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for Generating an KEEPALIVE-Request
 *
 * @return
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyNetwork_SendKeepAliveRequest
(
    const char* systemName ///< System name
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for Processing KEEPALIVE-Requests arriving from a far-side RPC Proxy
 *
 * @return
 *      - LE_OK, if successfully,
 *      - otherwise failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyNetwork_ProcessKeepAliveRequest
(
    void* handle,                  ///< [IN] Opaque handle to the le_comm communication channel
    const char* systemName,        ///< [IN] Name of the system that sent the Client-Request
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Message State-Machine data
    void* proxyMessagePtr          ///< Pointer to the Proxy Message
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for Processing KEEPALIVE-Responses arriving from a far-side RPC Proxy
 *
 * @return
 *      - LE_OK, if successfully,
 *      - otherwise failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyNetwork_ProcessKeepAliveResponse
(
    void* handle,                   ///< [IN] Opaque handle to the le_comm communication channel
    const char* systemName,         ///< [IN] Name of the system that sent the Client-Request
    StreamState_t* streamStatePtr,  ///< [IN] Pointer to the Message State-Machine data
    void* proxyMessagePtr           ///< Pointer to the Proxy Message
);

//--------------------------------------------------------------------------------------------------
/**
 * This function initializes the RPC Proxy Network Services.
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyNetwork_InitializeOnce
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Receive an rpc stream
 * @return:
 *      - LE_OK when streaming is finished.
 *      - LE_IN_PROGRESS when streaming is still ongoing.
 *      - LE_FAULT if an error has happened
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxy_RecvStream
(
    void* handle,                  ///< [IN] Opaque handle to the le_comm communication channel
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Stream State-Machine data
    void *proxyMessagePtr          ///< [IN] Pointer to the Proxy Message
);

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the stream state to receive a certain rpc message type
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxy_InitializeStreamState
(
    StreamState_t* streamStatePtr,      ///< [IN] Pointer to stream state variable.
    void* proxyMessagePtr               ///< [IN] Pointer to proxy message structure
);

#endif /* LE_RPC_PROXY_NETWORK_H_INCLUDE_GUARD */
