/**
 * @file le_rpcProxyConfig.c
 *
 * Local Messaging version of the RPC Proxy Configuration Service.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "le_rpcProxy.h"
#include "le_rpcProxyConfig.h"
#include "le_rpcProxyNetwork.h"
#include "le_cfg_interface.h"

//--------------------------------------------------------------------------------------------------
/**
 * Extern Declarations
 */
//--------------------------------------------------------------------------------------------------
extern const rpcProxy_SystemLinkElement_t rpcProxy_SystemLinkArray[];
extern const rpcProxy_ExternServer_t *rpcProxy_ServerReferenceArray[];
extern const rpcProxy_ExternClient_t *rpcProxy_ClientReferenceArray[];

//--------------------------------------------------------------------------------------------------
/**
 * Configuration Declarations
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Array of System-Service Configuration bindings required by this system.
 */
//--------------------------------------------------------------------------------------------------
rpcProxy_SystemServiceConfig_t
rpcProxy_SystemServiceArray[RPC_PROXY_SERVICE_BINDINGS_MAX_NUM + 1];

//--------------------------------------------------------------------------------------------------
/**
 * This pool is used to allocate memory for general Config strings.
 * Initialized in the function le_rpcProxyConfig_Initialize()
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(
    ConfigStringPool,
    RPC_PROXY_SERVICE_BINDINGS_MAX_NUM * RPC_PROXY_CONFIG_STRING_PER_SERVICE_MAX_NUM,
    LIMIT_MAX_IPC_INTERFACE_NAME_BYTES);

static le_mem_PoolRef_t ConfigStringPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * This pool is used to allocate memory for System-Link command-line argument strings.
 * Initialized in the function le_rpcProxyConfig_Initialize()
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(
    ConfigCommandLineArgumentStringPool,
    RPC_PROXY_NETWORK_SYSTEM_MAX_NUM * RPC_PROXY_COMMAND_LINE_ARG_PER_SYSTEM_LINK_MAX_NUM,
    LIMIT_MAX_ARGS_STR_LEN);

static le_mem_PoolRef_t ConfigArgumentStringPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * This pool is used to allocate memory for System-Link command-line argument arrays.
 * Initialized in the function le_rpcProxyConfig_Initialize()
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(
    ConfigCommandLineArgumentArrayPool,
    RPC_PROXY_NETWORK_SYSTEM_MAX_NUM,
    (RPC_PROXY_COMMAND_LINE_ARG_PER_SYSTEM_LINK_MAX_NUM + 1) * LIMIT_MAX_ARGS_STR_LEN);

static le_mem_PoolRef_t ConfigArgumentArrayPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving a System-Link element from the System-Link array
 */
//--------------------------------------------------------------------------------------------------
const rpcProxy_SystemLinkElement_t rpcProxyConfig_GetSystemLinkArray
(
    uint32_t index
)
{
    return rpcProxy_SystemLinkArray[index];
};

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving pointer to a Server-Reference element from the Server-Reference array
 */
//--------------------------------------------------------------------------------------------------
const rpcProxy_ExternServer_t* rpcProxyConfig_GetServerReferenceArray
(
    uint32_t index

)
{
    return rpcProxy_ServerReferenceArray[index];
};

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving pointer to a Client-Reference element from the Client-Reference array
 */
//--------------------------------------------------------------------------------------------------
const rpcProxy_ExternClient_t* rpcProxyConfig_GetClientReferenceArray
(
    uint32_t index
)
{
    return rpcProxy_ClientReferenceArray[index];
};

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving a System-service element from the System-service array
 */
//--------------------------------------------------------------------------------------------------
const rpcProxy_SystemServiceConfig_t rpcProxyConfig_GetSystemServiceArray
(
    uint32_t index
)
{
    return rpcProxy_SystemServiceArray[index];
};


//--------------------------------------------------------------------------------------------------
/**
 * Reads the System-Links configuration from the 'Framework' ConfigTree.
 *
 * @note
 *      This function is stubbed
 *
 * @return
 *      - LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyConfig_LoadSystemLinks
(
    void
)
{
    // Stub function - return LE_OK
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Reads the References configuration from the ConfigTree
 *
 * @note
 *      This function is stubbed
 *
 * @return
 *      - LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyConfig_LoadReferences
(
    void
)
{
    // Stub function - return LE_OK
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the Argument Array Pool reference.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t rpcProxyConfig_GetArgumentArrayPoolRef
(
    void
)
{
    return ConfigArgumentArrayPoolRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the Argument String Pool reference.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t rpcProxyConfig_GetArgumentStringPoolRef
(
    void
)
{
    return ConfigArgumentStringPoolRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the String Pool reference.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t rpcProxyConfig_GetStringPoolRef
(
    void
)
{
    return ConfigStringPoolRef;
}


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
)
{
    // Initialize memory pool for allocating System and Service Name Strings.
    ConfigStringPoolRef = le_mem_InitStaticPool(
                              ConfigStringPool,
                              RPC_PROXY_SERVICE_BINDINGS_MAX_NUM *
                              RPC_PROXY_CONFIG_STRING_PER_SERVICE_MAX_NUM,
                              LIMIT_MAX_IPC_INTERFACE_NAME_BYTES);

    // Initialize memory pool for allocating System-Link Command-Line Argument Strings.
    ConfigArgumentStringPoolRef = le_mem_InitStaticPool(
                                      ConfigCommandLineArgumentStringPool,
                                      RPC_PROXY_NETWORK_SYSTEM_MAX_NUM *
                                      RPC_PROXY_COMMAND_LINE_ARG_PER_SYSTEM_LINK_MAX_NUM,
                                      LIMIT_MAX_ARGS_STR_LEN);

    // Initialize memory pool for allocating System-Link Command-Line Argument Arrays.
    ConfigArgumentArrayPoolRef = le_mem_InitStaticPool(
                                      ConfigCommandLineArgumentArrayPool,
                                      RPC_PROXY_NETWORK_SYSTEM_MAX_NUM,
                                      (RPC_PROXY_COMMAND_LINE_ARG_PER_SYSTEM_LINK_MAX_NUM + 1) *
                                      LIMIT_MAX_ARGS_STR_LEN);

    // Initialize System-Services Array
    memset(rpcProxy_SystemServiceArray, 0, sizeof(rpcProxy_SystemServiceArray));

    return LE_OK;
}
