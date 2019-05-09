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
 * Definition to enable the RPC Proxy to be built to use local-service messaging
 */
//--------------------------------------------------------------------------------------------------
#ifdef LE_CONFIG_RPC
#if defined (LE_CONFIG_RTOS) || defined(LE_CONFIG_RPC_PROXY_LIBRARY)
// Enable RPC Proxy to use local-service messaging
#define RPC_PROXY_LOCAL_SERVICE      1
#endif
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of simultaneous service bindings hosted by the RPC Proxy.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_SERVICE_BINDINGS_MAX_NUM     10

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of simultaneous server data objects supported by the RPC Proxy.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_MSG_REFERENCE_MAX_NUM        10

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
            (RPC_PROXY_NETWORK_KEEPALIVE_SERVICE_INTERVAL)

#define RPC_PROXY_CONNECT_SERVICE_REQUEST_TIMER_INTERVAL    15
#define RPC_PROXY_NETWORK_KEEPALIVE_SERVICE_INTERVAL        120
#define RPC_PROXY_NETWORK_KEEPALIVE_REQUEST_TIMER_INTERVAL  30


//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Message Definitions
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_MAX_MESSAGE                  1024
#define RPC_LOCAL_MAX_MESSAGE                  RPC_PROXY_MAX_MESSAGE

#define RPC_PROXY_MSG_SERVICE_NAME_SIZE        (sizeof(char) * LIMIT_MAX_IPC_INTERFACE_NAME_BYTES)

#define RPC_PROXY_MSG_HEADER_SIZE              (sizeof(rpcProxy_CommonHeader_t) + \
                                               sizeof(uint16_t))

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
#define RPC_PROXY_REQUEST_RESPONSE             8

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

    char  systemName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];   ///< System-Name
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

    char  systemName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];  ///< Destination of the request
}
rpcProxy_KeepAliveMessage_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Client-Request and Server-Response Message Structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct __attribute__((packed))
{
    rpcProxy_CommonHeader_t commonHeader; ///< RPC Proxy Common Message Header
                                          ///< NOTE: Must be defined as first parameter

    // Legato Message
    uint16_t  msgSize;                          ///< Size of the Legato Message payload.
    uint8_t   message[RPC_PROXY_MAX_MESSAGE];   ///< Buffer to hold Legato Message payload.
}
rpcProxy_Message_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Client Request-Response Record Message Structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct rpcProxy_ClientRequestResponseRecord
{
    rpcProxy_CommonHeader_t commonHeader; ///< RPC Proxy Common Message Header
                                          ///< NOTE: Must be defined as first parameter

    uint32_t msgId;                                        ///< Legato Msg-ID
    char  systemName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];  ///< Source of the request
}
rpcProxy_ClientRequestResponseRecord_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Local-Message Structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct rpcProxy_LocalMessage
{
    uint32_t                id;       ///< Proxy Message Id, uniquely identifies each RPC Message.
    uint8_t*                dataPtr;  ///< Pointer to String and Array Parameters
    le_dls_Link_t           link;
}
rpcProxy_LocalMessage_t;


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

#endif /* LE_RPC_PROXY_H_INCLUDE_GUARD */
