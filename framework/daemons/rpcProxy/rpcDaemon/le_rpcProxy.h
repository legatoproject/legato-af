/**
 * @file le_rpcProxy.h
 *
 * Header file for RPC Proxy Service definitions and types.
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#ifndef LE_RPC_PROXY_H_INCLUDE_GUARD
#define LE_RPC_PROXY_H_INCLUDE_GUARD

#include "legato.h"
#include "limit.h"
#include "le_comm.h"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of simultaneous service bindings hosted by the RPC Proxy.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_SERVICE_BINDINGS_MAX_NUM     LE_CONFIG_RPC_PROXY_SERVICE_BINDINGS_MAX_NUM

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of simultaneous server data objects supported by the RPC Proxy.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_MSG_REFERENCE_MAX_NUM        LE_CONFIG_RPC_PROXY_SERVICE_BINDINGS_MAX_NUM


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of simultaneous file streams going on.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_FILE_STREAM_MAX_NUM          LE_CONFIG_RPC_PROXY_FILE_STREAM_MAX_NUM

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Timer Interval Definitions
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Client-Request Timer
 * NOTE: This is used as part of an interim solution that ensures a Client does not become
 *       blocked indefinitely as a result of spurious packet-drops over the RPC-to-RPC network.
 *
 * TODO: Replace with a solution that is more deterministic, such as introducing a time-out
 *       solution originating at the Client or in the Legato Messaging framework.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_CLIENT_REQUEST_TIMER_INTERVAL \
            (LE_CONFIG_RPC_PROXY_NETWORK_KEEPALIVE_SERVICE_INTERVAL)

#define RPC_PROXY_CONNECT_SERVICE_REQUEST_TIMER_INTERVAL \
            (LE_CONFIG_RPC_PROXY_CONNECT_SERVICE_REQUEST_TIMER_INTERVAL)

#define RPC_PROXY_NETWORK_KEEPALIVE_SERVICE_INTERVAL \
            (LE_CONFIG_RPC_PROXY_NETWORK_KEEPALIVE_SERVICE_INTERVAL)

#define RPC_PROXY_NETWORK_KEEPALIVE_TIMEOUT_TIMER_INTERVAL  \
            (LE_CONFIG_RPC_PROXY_NETWORK_KEEPALIVE_TIMEOUT_TIMER_INTERVAL)


//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Message Definitions
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_MAX_FILESTREAM_PAYLOAD_SIZE  LE_CONFIG_RPC_PROXY_MAX_FILE_STREAM_PAYLOAD
#define RPC_LOCAL_MAX_LARGE_OUT_PARAMETER_SIZE LE_CONFIG_RPC_PROXY_MAX_LARGE_OUT_PARAMETER_SIZE
#define RPC_LOCAL_MAX_SMALL_OUT_PARAMETER_SIZE LE_CONFIG_RPC_PROXY_MAX_SMALL_OUT_PARAMETER_SIZE

#define RPC_PROXY_MSG_SERVICE_NAME_SIZE        (sizeof(char) * LIMIT_MAX_IPC_INTERFACE_NAME_BYTES)

#define RPC_PROXY_COMMON_HEADER_SIZE           (sizeof(rpcProxy_CommonHeader_t))

#define RPC_PROXY_CONNECT_SERVICE_MSG_SIZE     (sizeof(rpcProxy_ConnectServiceMessage_t) - \
                                               RPC_PROXY_COMMON_HEADER_SIZE)

#define RPC_PROXY_KEEPALIVE_MSG_SIZE           (sizeof(rpcProxy_KeepAliveMessage_t) - \
                                               RPC_PROXY_COMMON_HEADER_SIZE)
#define RPC_PROXY_MSG_METADATA_SIZE            (2 * LE_PACK_SIZEOF_TAG_ID + LE_PACK_SIZEOF_UINT32 +\
                                               LE_PACK_SIZEOF_INT16)

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Message Types
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_CONNECT_SERVICE_REQUEST      1
#define RPC_PROXY_CONNECT_SERVICE_RESPONSE     2
#define RPC_PROXY_DISCONNECT_SERVICE           3
#define RPC_PROXY_CLIENT_REQUEST               4
#define RPC_PROXY_SERVER_RESPONSE              5
#define RPC_PROXY_KEEPALIVE_REQUEST            6
#define RPC_PROXY_KEEPALIVE_RESPONSE           7
#define RPC_PROXY_SERVER_ASYNC_EVENT           8
#define RPC_PROXY_FILESTREAM_MESSAGE           9
#define RPC_PROXY_NUM_MESSAGE_TYPES            10

//--------------------------------------------------------------------------------------------------
/**
 * The direction of a parameter, either input or output
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    DIR_IN,
    DIR_OUT
}
rpcProxy_Direction_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Common Message Header Structure
 * NOTE:  Must be defined as first parameter in each RPC Proxy Message structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct __attribute__((packed))
{
    // Proxy Common Header
    uint32_t  id;         ///< Proxy Message Id, uniquely identifies each RPC Message.
    uint32_t  serviceId;  ///< Proxy Message Service Identification
    uint8_t   type;       ///< Proxy Message Type.
}
rpcProxy_CommonHeader_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Connect-Service Message Structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct __attribute__((packed))
{
    rpcProxy_CommonHeader_t commonHeader; ///< RPC Proxy Common Message Header
                                          ///< NOTE: Must be defined as first parameter

    char  systemName[LIMIT_MAX_SYSTEM_NAME_BYTES];        ///< System-Name
    char  serviceName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];  ///< Interface name
    char  protocolIdStr[LIMIT_MAX_PROTOCOL_ID_BYTES];       ///< Protocol ID Str
    int32_t serviceCode; ///< Connect-Service Set-up result-code
}
rpcProxy_ConnectServiceMessage_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Keep-Alive Message Structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct __attribute__((packed))
{
    rpcProxy_CommonHeader_t commonHeader; ///< RPC Proxy Common Message Header
                                          ///< NOTE: Must be defined as first parameter

    char  systemName[LIMIT_MAX_SYSTEM_NAME_BYTES];  ///< Destination of the request
}
rpcProxy_KeepAliveMessage_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Message metadata
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint16_t fileStreamId;                     ///< Contains Steam ID if any and flags
    uint16_t fileStreamFlags;                  ///< Contains file stream flags if stream Id is valid
    bool isFileStreamValid;                    ///< indicates whethere above two fields are valid
}
rpcProxy_MessageMetadata_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Client-Request and Server-Response Message Structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    rpcProxy_CommonHeader_t commonHeader; ///< RPC Proxy Common Message Header
                                          ///< NOTE: Must be defined as first parameter
    rpcProxy_MessageMetadata_t metaData;
    le_msg_MessageRef_t msgRef;
}
rpcProxy_Message_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy File Stream Message Structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    rpcProxy_CommonHeader_t commonHeader; ///< RPC Proxy Common Message Header
                                          ///< NOTE: Must be defined as first parameter
    rpcProxy_MessageMetadata_t metaData;
    // The file data Message
    uint16_t  requestedSize;                    ///< number of bytes requested.
    uint16_t  payloadSize;                      ///< Size of the Message payload.
    uint8_t   payload[RPC_PROXY_MAX_FILESTREAM_PAYLOAD_SIZE];   ///< Buffer to hold Message payload.

}
rpcProxy_FileStreamMessage_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Client Request-Response Record Message Structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct rpcProxy_ClientRequestResponseRecord
{
    rpcProxy_CommonHeader_t commonHeader; ///< RPC Proxy Common Message Header
                                          ///< NOTE: Must be defined as first parameter

    uint32_t  msgId;                                      ///< Legato Msg-ID
    char      systemName[LIMIT_MAX_SYSTEM_NAME_BYTES];  ///< Source of the request
#ifdef RPC_PROXY_LOCAL_SERVICE
    le_dls_List_t localBuffers;
#endif
}
rpcProxy_ClientRequestResponseRecord_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Local-Message Structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct rpcProxy_LocalBuffer
{
    le_dls_Link_t           link;
    rpcProxy_Direction_t    dir;           ///< Is this parameter an output
    size_t                  dataSz;        ///< Size of data parameter
    uint8_t                 bufferData[];  ///< Contents of buffer data
}
rpcProxy_LocalBuffer_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy System-Link Element - Defines a Network System-Link to be used by the RPC Proxy
 */
//--------------------------------------------------------------------------------------------------
typedef struct rpcProxy_SystemLinkElement
{
    const char *systemName;    ///< System-Name
    const char *libraryName;   ///< le_comm plugin
    int argc;                  ///< Number of strings pointed to by argv.
    const char * const *argv;  ///< Array of character strings.
}
rpcProxy_SystemLinkElement_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Server Reference Element - Defines a Service Reference being
 * hosted by the RPC Proxy
 */
//--------------------------------------------------------------------------------------------------
typedef struct rpcProxy_ExportedServer
{
    const char *serviceName; ///< Service-Instance-Name
    const char *protocolIdStr; ///< Protocol-Id String
    size_t messageSize;        ///< Maximum message size
}
rpcProxy_ExternServer_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Server Reference Element - Defines a Local Service Reference being
 * hosted by the RPC Proxy
 */
//--------------------------------------------------------------------------------------------------
typedef struct rpcProxy_ExternLocalServer
{
    struct rpcProxy_ExportedServer common;
    le_msg_ServiceRef_t (*initLocalServicePtr)(void);
} rpcProxy_ExternLocalServer_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Server Reference Element - Defines a Linux Service Reference being
 * hosted by the RPC Proxy
 */
//--------------------------------------------------------------------------------------------------
typedef struct rpcProxy_ExternLinuxServer
{
    struct rpcProxy_ExportedServer common;
    const char *localServiceInstanceName;
} rpcProxy_ExternLinuxServer_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Client Reference Element - Defines a Service Reference being
 * required by the RPC Proxy
 */
//--------------------------------------------------------------------------------------------------
typedef struct rpcProxy_ExportedClient
{
    const char *serviceName; ///< Service-Instance-Name
    const char *protocolIdStr; ///< Protocol-Id String
    size_t messageSize;        ///< Maximum message size
}
rpcProxy_ExternClient_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Client Reference Element - Defines a Local Service Reference being
 * required by the RPC Proxy
 */
//--------------------------------------------------------------------------------------------------
typedef struct rpcProxy_ExternLocalClient
{
    struct rpcProxy_ExportedClient common;
    le_msg_LocalService_t *localServicePtr;
} rpcProxy_ExternLocalClient_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Client Reference Element - Defines a Linux Service Reference being
 * required by the RPC Proxy
 */
//--------------------------------------------------------------------------------------------------
typedef struct rpcProxy_ExternLinuxClient
{
    struct rpcProxy_ExportedClient common;
    const char *localServiceInstanceName;
} rpcProxy_ExternLinuxClient_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy System-Service Configuration Element - Defines the System/Service bindings
 * required by the RPC Proxy
 */
//--------------------------------------------------------------------------------------------------
typedef struct rpcProxy_SystemServiceConfig
{
    const char *systemName;        ///< System-Name
    const char *linkName;          ///< Link-Name
    const char *serviceName;       ///< Service-Instance-Name
    const char *remoteServiceName; ///< Remote Service-Instance-Name
    int argc; ///< Number of strings pointed to by argv (used by le_comm plugin implementation)
    const char **argv; ///< Array of character strings (used by le_comm plugin implementation)
} rpcProxy_SystemServiceConfig_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Function prototypes
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Receive Handler Callback Function for RPC Communication
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_AsyncRecvHandler
(
    void* handle,  ///< [IN] Opaque communication handle
    short events   ///< [IN] Event flags
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the server and advertise the service.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_AdvertiseServices
(
    const char* systemName ///< [IN] Name of System on which to advertise services
);

//--------------------------------------------------------------------------------------------------
/**
 * Hide All Server sessions affected by the Network Connection failure.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_HideServices
(
    const char* systemName ///< [IN] Name of System on which to hide services
);

//--------------------------------------------------------------------------------------------------
/**
 * Disconnect all Client sessions affected by the Network Connection failure.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_DisconnectSessions
(
    const char* systemName ///< [IN] Name of System on which to disconnect sessions
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for generating unique Proxy Message IDs
 */
//--------------------------------------------------------------------------------------------------
uint32_t rpcProxy_GenerateProxyMessageId
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for sending Proxy Messages to the far side via the le_comm API
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxy_SendMsg
(
    const char* systemName, ///< [IN] Name of the system message is being sent to
    void* messagePtr ///< [IN] Void pointer to the message buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Expired Proxy Message Timers
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_ProxyMessageTimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the Expiry Timer Hash-map reference.
 */
//--------------------------------------------------------------------------------------------------
le_hashmap_Ref_t rpcProxy_GetExpiryTimerRefByProxyId
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the Service-ID Hash-map reference.
 */
//--------------------------------------------------------------------------------------------------
le_hashmap_Ref_t rpcProxy_GetServiceIDMapByName
(
    void
);


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
);

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
);

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
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 *  Send out the body of a variable length message
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxy_SendVariableLengthMsgBody
(
    void* handle, ///< [IN] Opaque handle to the le_comm communication channel
    void* messagePtr ///< [IN] Void pointer to the message buffer
);

#ifdef RPC_PROXY_LOCAL_SERVICE
//--------------------------------------------------------------------------------------------------
/**
 * Retrieve and remove the next parameter buffer for a request response call
 */
//--------------------------------------------------------------------------------------------------
le_dls_Link_t *rpcProxy_PopNextParameter
(
    uint32_t serviceId
);

//--------------------------------------------------------------------------------------------------
/**
 * Cleans up local message resources associated with a proxy message id
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_CleanUpLocalMessageResources
(
    uint32_t proxyMsgId
);
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Initialize streaming memory pools and hash tables
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_InitializeOnceStreamingMemPools();

#endif /* LE_RPC_PROXY_H_INCLUDE_GUARD */
