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
 * RPC Proxy Bindings Configuration Tree Node.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_CONFIG_BINDINGS_TREE_NODE  "system:/rpcProxy/bindings"

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Systems Configuration Tree Node.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE   "system:/rpcProxy/systems"

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy System-links Framework Configuration Tree Node.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_CONFIG_SYSTEM_LINKS_TREE_NODE  "system:/framework/systemLinks"

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Client-Reference Framework Configuration Tree Node.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_CONFIG_CLIENT_REFERENCES_TREE_NODE  "system:/framework/clientReferences"

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Server-Reference Framework Configuration Tree Node.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_CONFIG_SERVER_REFERENCES_TREE_NODE  "system:/framework/serverReferences"

//--------------------------------------------------------------------------------------------------
/**
 * Convenience macro to add a static configuration entry.
 *
 * @note The index parameter is only expanded once, so it is safe to use the postfix increment
 *       operator on it.
 *
 * @param   index   Index of service entry.
 * @param   system  Remote system name.
 * @param   link    Name of link to remote system.
 * @param   local   Local API to expose.
 * @param   remote  Remote API to connect to.
 * @param   argc    Number of arguments to link driver.
 * @param   argv    Array of arguments to link driver.
 */
//--------------------------------------------------------------------------------------------------
#define LE_RPC_PROXY_ADD_API(index, system, link, local, remote, argc, argv)    \
    *(rpcProxyConfig_GetSystemServiceArray(index)) =                            \
    (rpcProxy_SystemServiceConfig_t) { (system), (link), (local), (remote), (argc), (argv) }

//--------------------------------------------------------------------------------------------------
/**
 * Convenience macro to terminate static configuration array.
 */
//--------------------------------------------------------------------------------------------------
#define LE_RPC_PROXY_END_APIS(index) LE_RPC_PROXY_ADD_API((index), NULL, NULL, NULL, NULL, 0, NULL)

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
rpcProxy_SystemLinkElement_t rpcProxyConfig_GetSystemLinkArray
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
LE_SHARED rpcProxy_SystemServiceConfig_t* rpcProxyConfig_GetSystemServiceArray
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
 * Abstract function for providing the RPC Proxy Run-time Configuration
 * for the RPC System-Service Bindings.
 *
 * NOTE: For systems with config-tree support, this will be internally provided by the RPC Proxy
 * component.  While, systems without config-tree support, a third-party component must be
 * supplied, such as the $SWI_ROOT/components/rpcProxyConfig, to define these
 * definitions statically.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_NOT_FOUND if node is not found.
 *      - LE_BAD_PARAMETER if number of elements exceeds the storage array size.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t rpcProxyConfig_LoadBindings
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
le_result_t rpcProxyConfig_InitializeOnce
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
