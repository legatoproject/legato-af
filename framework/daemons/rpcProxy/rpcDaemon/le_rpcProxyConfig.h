/**
 * @file le_rpcProxyConfig.h
 *
 * Header file for RPC Proxy Configuration definitions and functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_RPC_PROXY_CONFIG_H_INCLUDE_GUARD
#define LE_RPC_PROXY_CONFIG_H_INCLUDE_GUARD

#ifndef RPC_PROXY_LOCAL_SERVICE
//--------------------------------------------------------------------------------------------------
/**
 * Reads the System-Links configuration from the 'Framework' ConfigTree.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyConfig_LoadSystemLinks
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Reads the References configuration from the ConfigTree.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyConfig_LoadReferences
(
    void
);
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Reads the System-Service Bindings configuration from the ConfigTree.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyConfig_LoadBindings
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
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyConfig_Initialize
(
    void
);

#endif /* LE_RPC_PROXY_CONFIG_H_INCLUDE_GUARD */
