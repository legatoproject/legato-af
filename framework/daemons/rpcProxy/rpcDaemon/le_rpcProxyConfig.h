/**
 * @file le_rpcProxyConfig.h
 *
 * Header file for RPC Proxy Configuration definitions and functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_RPC_PROXY_CONFIG_H_INCLUDE_GUARD
#define LE_RPC_PROXY_CONFIG_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Configuration strings per service.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_CONFIG_STRING_PER_SERVICE_MAX_NUM    6


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Configuration strings per system-link.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_CONFIG_STRING_PER_SYSTEM_LINK_MAX_NUM    2

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Command-line Arguments per system-link.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_COMMAND_LINE_ARG_PER_SYSTEM_LINK_MAX_NUM    3


//--------------------------------------------------------------------------------------------------
/**
 * Ptr to the RPC Proxy Bindings Configuration Tree Node.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_CONFIG_BINDINGS_TREE_NODE  "rpcProxy/bindings"

//--------------------------------------------------------------------------------------------------
/**
 * Ptr to the RPC Proxy Systems Configuration Tree Node.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE   "rpcProxy/systems"

//--------------------------------------------------------------------------------------------------
/**
 * Ptr to the RPC Proxy System-links Framework Configuration Tree Node.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_CONFIG_SYSTEM_LINKS_TREE_NODE  "framework/systemLinks"

//--------------------------------------------------------------------------------------------------
/**
 * Ptr to the RPC Proxy Client-Reference Framework Configuration Tree Node.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_CONFIG_CLIENT_REFERENCES_TREE_NODE  "framework/clientReferences"

//--------------------------------------------------------------------------------------------------
/**
 * Ptr to the RPC Proxy Server-Reference Framework Configuration Tree Node.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_CONFIG_SERVER_REFERENCES_TREE_NODE  "framework/serverReferences"



//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Configuration Function prototypes
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving a System-Link element from the System-Link array
 */
//--------------------------------------------------------------------------------------------------
const rpcProxy_SystemLinkElement_t rpcProxyConfig_GetSystemLinkArray
(
    uint32_t index
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving pointer to a Server-Reference element from the Server-Reference array
 */
//--------------------------------------------------------------------------------------------------
const rpcProxy_ExternServer_t* rpcProxyConfig_GetServerReferenceArray
(
    uint32_t index
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving pointer to a Client-Reference element from the Client-Reference array
 */
//--------------------------------------------------------------------------------------------------
const rpcProxy_ExternClient_t* rpcProxyConfig_GetClientReferenceArray
(
    uint32_t index
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving a System-service element from the System-service array
 */
//--------------------------------------------------------------------------------------------------
const rpcProxy_SystemServiceConfig_t rpcProxyConfig_GetSystemServiceArray
(
    uint32_t index
);

//--------------------------------------------------------------------------------------------------
/**
 * Reads the System-Links configuration from the 'Framework' ConfigTree.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_NOT_FOUND if node is not found.
 *      - LE_BAD_PARAMETER if number of elements exceeds the storage array size.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyConfig_LoadSystemLinks
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Reads the References configuration from the ConfigTree.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_NOT_FOUND if node is not found.
 *      - LE_BAD_PARAMETER if number of elements exceeds the storage array size.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyConfig_LoadReferences
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Reads the System-Service Bindings configuration from the ConfigTree.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_NOT_FOUND if node is not found.
 *      - LE_BAD_PARAMETER if number of elements exceeds the storage array size.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyConfig_LoadBindings
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * This function cross-validates dependencies in the RPC Proxy Configuration.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_NOT_FOUND if node is not found.
 *      - LE_FAULT for all other errors.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyConfig_ValidateConfiguration
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the systemName using a serviceName.
 */
//--------------------------------------------------------------------------------------------------
const char* rpcProxyConfig_GetSystemNameByServiceName
(
    const char* serviceName ///< Service name
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the remote service-Name using a service-name.
 */
//--------------------------------------------------------------------------------------------------
const char* rpcProxyConfig_GetRemoteServiceNameByServiceName
(
    const char* serviceName ///< Service name
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the service-name using a remote service-name.
 */
//--------------------------------------------------------------------------------------------------
const char* rpcProxyConfig_GetServiceNameByRemoteServiceName
(
    const char* remoteServiceName ///< Remote service name
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the system-name using a link-name.
 */
//--------------------------------------------------------------------------------------------------
const char* rpcProxyConfig_GetSystemNameByLinkName
(
    const char* linkName ///< Link name
);

//--------------------------------------------------------------------------------------------------
/**
 * This function initializes the RPC Proxy Configuration Services.
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 *
 * @return
 *      - LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyConfig_Initialize
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the Argument Array Pool reference.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t rpcProxyConfig_GetArgumentArrayPoolRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the Argument String Pool reference.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t rpcProxyConfig_GetArgumentStringPoolRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the String Pool reference.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t rpcProxyConfig_GetStringPoolRef
(
    void
);

#endif /* LE_RPC_PROXY_CONFIG_H_INCLUDE_GUARD */
