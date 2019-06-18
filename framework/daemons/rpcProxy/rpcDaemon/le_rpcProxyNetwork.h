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
 * Maximum receive buffer size.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_RECV_BUFFER_MAX               (RPC_PROXY_MAX_MESSAGE + RPC_PROXY_MSG_HEADER_SIZE)


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
    ASYNC     ///< Connections are handled asychronously (non-blocking)
}
NetworkConnectionType_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Network Message Receive State
 */
//--------------------------------------------------------------------------------------------------
typedef enum NetworkMessageReceiveState
{
    NETWORK_MSG_IDLE = 0, ///< IDLE State
    NETWORK_MSG_PARTIAL_HEADER, ///< Partial HEADER State
    NETWORK_MSG_HEADER, ///< Complete HEADER State
    NETWORK_MSG_PARTIAL_MESSAGE, ///< Partial MESSAGE State
    NETWORK_MSG_MESSAGE, ///< Complete MESSAGE State
    NETWORK_MSG_PARTIAL_VARIABLE_LEN_MESSAGE, ///< Partial (Variable-length) MESSAGE State
    NETWORK_MSG_DONE ///< DONE - Complete message received
}
NetworkMessageReceiveState_t;

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
}
NetworkMessageState_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Network Record structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct NetworkRecord
{
    void*                    handle;    ///< Opague handle to the network connection
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

    char systemName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES]; ///< Name of Destination System
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
    const char* systemName, ///< System name
    rpcProxy_KeepAliveMessage_t* proxyMessagePtr ///< Pointer to the Proxy Message
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
    const char* systemName, ///< System name
    rpcProxy_KeepAliveMessage_t* proxyMessagePtr ///< Pointer to the Proxy Message
);

//--------------------------------------------------------------------------------------------------
/**
 * This function initializes the RPC Proxy Network Services.
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyNetwork_Initialize
(
    void
);

#endif /* LE_RPC_PROXY_NETWORK_H_INCLUDE_GUARD */
