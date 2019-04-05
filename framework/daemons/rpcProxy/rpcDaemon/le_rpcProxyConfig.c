/**
 * @file le_rpcProxyConfig.c
 *
 * This file contains the source code for the RPC Proxy Configuration Service.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "le_rpcProxy.h"
#include "le_rpcProxyNetwork.h"
#include "le_cfg_interface.h"

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
static const char* BindingsConfigTreeNode = "rpcProxy/bindings";


//--------------------------------------------------------------------------------------------------
/**
 * Ptr to the RPC Proxy Systems Configuration Tree Node.
 */
//--------------------------------------------------------------------------------------------------
static const char* SystemsConfigTreeNode = "rpcProxy/systems";


//--------------------------------------------------------------------------------------------------
/**
 * Extern Declarations
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Configuration Declarations
 */
//--------------------------------------------------------------------------------------------------
#ifndef RPC_PROXY_LOCAL_SERVICE
//--------------------------------------------------------------------------------------
/**
 * All services which should be exposed over RPC by this system.
 */
//--------------------------------------------------------------------------------------
const rpcProxy_ExternServer_t*
rpcProxy_ServerReferenceArray[RPC_PROXY_SERVICE_BINDINGS_MAX_NUM + 1] = { NULL };

//--------------------------------------------------------------------------------------
/**
 * All clients which are required over RPC by this system.
 */
//--------------------------------------------------------------------------------------
const rpcProxy_ExternClient_t*
rpcProxy_ClientReferenceArray[RPC_PROXY_SERVICE_BINDINGS_MAX_NUM + 1] = { NULL };

//--------------------------------------------------------------------------------------------------
/**
 * Array of Linux services which should be exposed over RPC by this system.
 */
//--------------------------------------------------------------------------------------------------
static rpcProxy_ExternLinuxServer_t
rpcProxy_LinuxServerReferenceArray[RPC_PROXY_SERVICE_BINDINGS_MAX_NUM + 1];

//--------------------------------------------------------------------------------------------------
/**
 * Array of Linux clients which are required over RPC by this system.
 */
//--------------------------------------------------------------------------------------------------
static rpcProxy_ExternLinuxClient_t
rpcProxy_LinuxClientReferenceArray[RPC_PROXY_SERVICE_BINDINGS_MAX_NUM+ 1];

//--------------------------------------------------------------------------------------------------
/**
 * Array of System-Link Configuration elements required by this system.
 */
//--------------------------------------------------------------------------------------------------
rpcProxy_SystemLinkElement_t
rpcProxy_SystemLinkArray[RPC_PROXY_NETWORK_SYSTEM_MAX_NUM + 1];
#endif

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


#ifndef RPC_PROXY_LOCAL_SERVICE
//--------------------------------------------------------------------------------------------------
/**
 * This pool is used to allocate memory for Protocol ID strings.
 * Initialized in the function le_rpcProxyConfig_Initialize()
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ConfigProtocolIdStringPool,
                          RPC_PROXY_SERVICE_BINDINGS_MAX_NUM,
                          LIMIT_MAX_PROTOCOL_ID_BYTES);

static le_mem_PoolRef_t ConfigProtocolIdStringPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * This pool is used to allocate memory for System-Link strings.
 * Initialized in the function le_rpcProxyConfig_Initialize()
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(
    ConfigSystemLinkStringPool,
    RPC_PROXY_NETWORK_SYSTEM_MAX_NUM * RPC_PROXY_CONFIG_STRING_PER_SYSTEM_LINK_MAX_NUM,
    LIMIT_MAX_IPC_INTERFACE_NAME_BYTES);

static le_mem_PoolRef_t ConfigSystemLinkStringPoolRef = NULL;

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
 * Reads the System-Links configuration from the 'Framework' ConfigTree.
 *
 * NOTE: Format is,
 *
 * links:
 * {
 *     "S1": {
 *         "libraryName" : "libComponent_networkSocket.so",
 *         "argc" : "2",
 *         "argv" : "10.0.0.5 54323",
 *     },
 *
 *     "S2": {
 *         "libraryName" : "libComponent_localLoopback.so",
 *         "argc" : "0"
 *         "argv" : ""
 *     }
 * }
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyConfig_LoadSystemLinks
(
    void
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    int index = 0;
    le_result_t result;

    // Open up a read transaction on the Config Tree
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn("framework/systemLinks");

    if (le_cfg_NodeExists(iteratorRef, "") == false)
    {
        LE_WARN("RPC Proxy 'framework/systemLinks' configuration not found.");

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iteratorRef);
        return LE_NOT_FOUND;
    }

    result = le_cfg_GoToFirstChild(iteratorRef);
    if (result != LE_OK)
    {
        LE_WARN("No configuration found.");

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iteratorRef);
        return result;
    }

    // Loop through all System-Link nodes.
    do
    {
        // Get the System-Name
        result = le_cfg_GetNodeName(iteratorRef, "", strBuffer, sizeof(strBuffer));
        if (result != LE_OK)
        {
            LE_ERROR("System-Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }

        // Allocate memory from Configuration String pool to store the System-Name
        char* systemNameCopyPtr =
            le_mem_ForceAlloc(ConfigSystemLinkStringPoolRef);

        // Copy the System-Name into the allocated memory
        le_utf8_Copy(systemNameCopyPtr,
                     strBuffer,
                     LIMIT_MAX_IPC_INTERFACE_NAME_BYTES,
                     NULL);

        // Set the System-Name pointer
        rpcProxy_SystemLinkArray[index].systemName = systemNameCopyPtr;

        // Get the Library-Name
        result = le_cfg_GetString(iteratorRef,
                                  "libraryName",
                                  strBuffer,
                                  sizeof(strBuffer),
                                  "");
        if (result != LE_OK)
        {
            LE_ERROR("Library-Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }

        // Allocate memory from Configuration String pool to store the Library-Name
        char* libraryNameCopyPtr =
            le_mem_ForceAlloc(ConfigSystemLinkStringPoolRef);

        // Copy the System-Name into the allocated memory
        le_utf8_Copy(libraryNameCopyPtr,
                     strBuffer,
                     LIMIT_MAX_IPC_INTERFACE_NAME_BYTES,
                     NULL);

        // Set the Library-Name pointer
        rpcProxy_SystemLinkArray[index].libraryName = libraryNameCopyPtr;

        // Get the Command-Line Argument Count
        result = le_cfg_GetString(iteratorRef,
                                  "argc",
                                  strBuffer,
                                  sizeof(strBuffer),
                                  "");
        if (result != LE_OK)
        {
            LE_ERROR("Argument-Count configuration not found.");

        }
        else
        {
            // Set the Argument Count
            rpcProxy_SystemLinkArray[index].argc = atoi(strBuffer);

            // Get the Argument Variable
            result = le_cfg_GetString(iteratorRef,
                                      "argv",
                                      strBuffer,
                                      sizeof(strBuffer),
                                      "");
            if (result != LE_OK)
            {
                LE_WARN("Argument Variable configuration not found.");

                // Close the transaction and return the failure
                le_cfg_CancelTxn(iteratorRef);
                return result;
            }

            // Allocate memory for the Command-Line Argument array
            const char **argvCopyPtr =
               le_mem_ForceAlloc(ConfigArgumentArrayPoolRef);

            char *token = NULL;
            char *search = " ";
            int argCount = 0;

            // Returns first token
            token = strtok(strBuffer, search);

            // Keep looping while delimiter is present in strBuffer[] and
            // there is still space available in the argument array.
            while ((token != NULL) &&
                   (argCount < RPC_PROXY_COMMAND_LINE_ARG_PER_SYSTEM_LINK_MAX_NUM))
            {
                // Allocate memory to store command-line argument string
                char* argvStringCopyPtr =
                    le_mem_ForceAlloc(ConfigArgumentStringPoolRef);

                // Copy the Command-Line Argument string token into the allocated memory
                le_utf8_Copy(argvStringCopyPtr,
                             token,
                             LIMIT_MAX_ARGS_STR_LEN,
                             NULL);

                // Set the Argument Variable pointer
                argvCopyPtr[argCount] = argvStringCopyPtr;

                token = strtok(NULL, search);
                argCount++;
            }

            // Set the last index to NULL
            argvCopyPtr[argCount] = NULL;

            // Set the Argument Variable pointer
            rpcProxy_SystemLinkArray[index].argv = argvCopyPtr;
        }

        index++;
    }
    while (le_cfg_GoToNextSibling(iteratorRef) == LE_OK);

    // Close the transaction and return success
    le_cfg_CancelTxn(iteratorRef);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Reads the Server-References configuration from the ConfigTree.
 * (Supported on LINUX only)
 *
 * NOTE: Format is,
 *
 * serverReferences:
 * {
 *     "aaa": {
 *         "protocolIdStr": "05109c4d3b4e60f24ade159aa7c5a214",
 *         "messageSize": "128",
 *         "localServiceInstanceName": "printer2"
 *     }
 * }
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LoadServerReferencesFromConfigTree
(
    void
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    int index = 0;
    le_result_t result;

    // Open up a read transaction on the Config Tree
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn("framework/serverReferences");

    if (le_cfg_NodeExists(iteratorRef, "") == false)
    {
        LE_WARN("RPC Proxy 'framework/serverReferences' configuration not found.");
        le_cfg_CancelTxn(iteratorRef);
        return LE_NOT_FOUND;
    }

    result = le_cfg_GoToFirstChild(iteratorRef);
    if (result != LE_OK)
    {
        LE_WARN("No server reference configuration found.");

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iteratorRef);
        return result;
    }

    // Loop through all Server-Reference nodes
    do
    {
        // Get the Service-Name
        result = le_cfg_GetNodeName(iteratorRef,
                                    "",
                                    strBuffer,
                                    sizeof(strBuffer));
        if (result != LE_OK)
        {
            LE_ERROR("Service-Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }

        // Allocate memory from Configuration String pool to store the Service-Name
        char* serviceNameCopyPtr =
            le_mem_ForceAlloc(ConfigStringPoolRef);

        // Copy the Service-Name into the allocated memory
        le_utf8_Copy(serviceNameCopyPtr,
                     strBuffer,
                     LIMIT_MAX_IPC_INTERFACE_NAME_BYTES,
                     NULL);

        // Set the Service-Name pointer
        rpcProxy_LinuxServerReferenceArray[index].common.serviceName =
            serviceNameCopyPtr;

        // Get the Protocol ID String
        result = le_cfg_GetString(iteratorRef,
                                  "protocolIdStr",
                                  strBuffer,
                                  sizeof(strBuffer),
                                  "");
        if (result != LE_OK)
        {
            LE_ERROR("Protocol ID String configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }

        // Allocate memory from Configuration Protocol ID String pool to
        // store the Protocol ID String
        char* protocolIdStrCopyPtr =
            le_mem_ForceAlloc(ConfigProtocolIdStringPoolRef);

        // Copy the Protocol ID String into the allocated memory
        le_utf8_Copy(protocolIdStrCopyPtr,
                     strBuffer,
                     LIMIT_MAX_PROTOCOL_ID_BYTES,
                     NULL);

        // Set the Protocol ID String pointer
        rpcProxy_LinuxServerReferenceArray[index].common.protocolIdStr =
            protocolIdStrCopyPtr;

        // Get the Message Size
        result = le_cfg_GetString(iteratorRef,
                                  "messageSize",
                                  strBuffer,
                                  sizeof(strBuffer),
                                  "");
        if (result != LE_OK)
        {
            LE_ERROR("Message Size configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }
        // Set the Message Size
        rpcProxy_LinuxServerReferenceArray[index].common.messageSize = atoi(strBuffer);

        // Get the Local-Service Instance Name
        result = le_cfg_GetString(
                     iteratorRef,
                     "localServiceInstanceName",
                     strBuffer,
                     sizeof(strBuffer),
                     "");

        if (result != LE_OK)
        {
            LE_ERROR("Local-Service Instance Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }

        // Allocate memory from Configuration String pool to
        // store the Local-Service Instance Name
        char* localServiceInstanceNameCopyPtr =
            le_mem_ForceAlloc(ConfigStringPoolRef);

        // Copy the Local-Service Instance Name into the allocated memory
        le_utf8_Copy(localServiceInstanceNameCopyPtr,
                     strBuffer,
                     LIMIT_MAX_IPC_INTERFACE_NAME_BYTES,
                     NULL);

        // Set the Local-Service Instance Name pointer
        rpcProxy_LinuxServerReferenceArray[index].localServiceInstanceName =
            localServiceInstanceNameCopyPtr;

        index++;
    }
    while (le_cfg_GoToNextSibling(iteratorRef) == LE_OK);

    // Set the Server Reference Array pointer
    rpcProxy_ServerReferenceArray[0] = &rpcProxy_LinuxServerReferenceArray[0].common;
    rpcProxy_ServerReferenceArray[index] = NULL;

    // Close the transaction and return success
    le_cfg_CancelTxn(iteratorRef);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Reads the Client-References configuration from the ConfigTree.
 * (Supported on LINUX only)
 *
 * NOTE: Format is,
 *
 * clientReferences:
 * {
 *     "ccc": {
 *         "protocolIdStr": "79e63e188305d7db4d98f2bb7d8c18c0",
 *         "messageSize": "133",
 *         "localServiceInstanceName": "printer"
 *     }
 * }
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LoadClientReferencesFromConfigTree
(
    void
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    int index = 0;
    le_result_t result;

    // Open up a read transaction on the Config Tree
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn("framework/clientReferences");

    if (le_cfg_NodeExists(iteratorRef, "") == false)
    {
        LE_WARN("RPC Proxy 'framework/clientReferences' configuration not found.");

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iteratorRef);
        return LE_NOT_FOUND;
    }

    result = le_cfg_GoToFirstChild(iteratorRef);
    if (result != LE_OK)
    {
        LE_WARN("No client reference configuration found.");

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iteratorRef);
        return result;
    }

    // Loop through all Client-Reference nodes
    do
    {
        // Get the Service-Name
        result = le_cfg_GetNodeName(iteratorRef, "", strBuffer, sizeof(strBuffer));
        if (result != LE_OK)
        {
            LE_ERROR("Service-Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }

        // Allocate memory from Configuration String pool to store the Service-Name
        char* serviceNameCopyPtr =
            le_mem_ForceAlloc(ConfigStringPoolRef);

        // Copy the Service-Name into the allocated memory
        le_utf8_Copy(serviceNameCopyPtr,
                     strBuffer,
                     LIMIT_MAX_IPC_INTERFACE_NAME_BYTES,
                     NULL);

        // Set the Service-Name pointer
        rpcProxy_LinuxClientReferenceArray[index].common.serviceName =
            serviceNameCopyPtr;

        // Get the Protocol ID String
        result = le_cfg_GetString(iteratorRef,
                                  "protocolIdStr",
                                  strBuffer,
                                  sizeof(strBuffer),
                                  "");
        if (result != LE_OK)
        {
            LE_ERROR("Protocol ID String configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }

        // Allocate memory from Configuration String pool to
        // store the Protocol ID String
        char* protocolIdStrCopyPtr =
            le_mem_ForceAlloc(ConfigProtocolIdStringPoolRef);

        // Copy the Protocol ID String into the allocated memory
        le_utf8_Copy(protocolIdStrCopyPtr,
                     strBuffer,
                     LIMIT_MAX_PROTOCOL_ID_BYTES,
                     NULL);

        // Set the Protocol ID String pointer
        rpcProxy_LinuxClientReferenceArray[index].common.protocolIdStr =
            protocolIdStrCopyPtr;

        // Get the Message Size
        result = le_cfg_GetString(iteratorRef,
                                  "messageSize",
                                  strBuffer,
                                  sizeof(strBuffer),
                                  "");
        if (result != LE_OK)
        {
            LE_ERROR("Message Size configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }
        // Set the Message Size
        rpcProxy_LinuxClientReferenceArray[index].common.messageSize = atoi(strBuffer);

        // Get the Local-Service Instance Name
        result = le_cfg_GetString(iteratorRef,
                                  "localServiceInstanceName",
                                  strBuffer,
                                  sizeof(strBuffer),
                                  "");
        if (result != LE_OK)
        {
            LE_ERROR("Local-Service Instance Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }

        // Allocate memory from Configuration String pool to
        // store the Local-Service Instance Name
        char* localServiceInstanceNameCopyPtr =
            le_mem_ForceAlloc(ConfigStringPoolRef);

        // Copy the Local-Service Instance Name into the allocated memory
        le_utf8_Copy(localServiceInstanceNameCopyPtr,
                     strBuffer,
                     LIMIT_MAX_IPC_INTERFACE_NAME_BYTES,
                     NULL);

        // Set the Local-Service Instance Name pointer
        rpcProxy_LinuxClientReferenceArray[index].localServiceInstanceName =
            localServiceInstanceNameCopyPtr;

        index++;
    }
    while (le_cfg_GoToNextSibling(iteratorRef) == LE_OK);

    // Set the Client Reference Array pointer
    rpcProxy_ClientReferenceArray[0] = &rpcProxy_LinuxClientReferenceArray[0].common;
    rpcProxy_ClientReferenceArray[index] = NULL;

    // Close the transaction and return success
    le_cfg_CancelTxn(iteratorRef);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Reads the References configuration from the ConfigTree
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyConfig_LoadReferences
(
    void
)
{
    le_result_t result;

    // Load the Client-References from the ConfigTree
    result = LoadClientReferencesFromConfigTree();
    if (result != LE_OK)
    {
        LE_WARN("Unable to load Client-Reference configuration, result [%d]", result);
    }

    // Load the Server-References from the ConfigTree
    result = LoadServerReferencesFromConfigTree();
    if (result != LE_OK)
    {
        LE_WARN("Unable to load Server-Reference configuration, result [%d]", result);
    }

    return LE_OK;
}
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Reads the Link-Name configuration from the 'Systems' ConfigTree.
 *
 * NOTE: Format is,
 *
 * systems:
 * {
 *     "S1": {
 *         "LINK1": {
 *             ....
 *         },
 *     },
 *
 *     "S2": {
 *     }
 * }
 *
 * (NOTE: Currently, only one link-name is supported at a time)
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LoadLinkNameFromConfigTree
(
    const char* systemName, ///< System name
    const uint8_t index ///< Current array element index
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    le_result_t result;

    // Open up a read transaction on the Config Tree
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn(SystemsConfigTreeNode);

    if (le_cfg_NodeExists(iteratorRef, "") == false)
    {
        LE_WARN("RPC Proxy '%s' configuration not found.", SystemsConfigTreeNode);

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iteratorRef);
        return LE_NOT_FOUND;
    }

    le_cfg_GoToNode(iteratorRef, systemName);
    if (le_cfg_IsEmpty(iteratorRef, ""))
    {
        LE_ERROR("System %s configuration not found", systemName);

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iteratorRef);
        return LE_NOT_FOUND;
    }

    result = le_cfg_GoToFirstChild(iteratorRef);
    if (result != LE_OK)
    {
        LE_WARN("No configuration found.");

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iteratorRef);
        return result;
    }

    // Get the Link-Name
    result = le_cfg_GetNodeName(iteratorRef, "", strBuffer, sizeof(strBuffer));
    if (result != LE_OK)
    {
        LE_ERROR("System-Link Name configuration not found.");

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iteratorRef);
        return result;
    }

    // Allocate memory from Configuration String pool to store the Link-Name
    char* linkNameCopyPtr =
        le_mem_ForceAlloc(ConfigStringPoolRef);

    // Copy the Link-Name into the allocated memory
    le_utf8_Copy(linkNameCopyPtr,
                 strBuffer,
                 LIMIT_MAX_IPC_INTERFACE_NAME_BYTES,
                 NULL);

    // Set the Link-Name pointer
    rpcProxy_SystemServiceArray[index].linkName = linkNameCopyPtr;

    // Close the transaction and return success
    le_cfg_CancelTxn(iteratorRef);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Reads the System-Service Bindings configuration from the ConfigTree.
 *
 * NOTE: Format is,
 *
 * bindings:
 * {
 *     "aaa": {
 *         "systemName" : "S1",
 *         "remoteService" : "bbb"
 *     },
 *
 *     "ccc": {
 *         "systemName" : S1",
 *         "remoteService" : "ddd"
 *     }
 * }
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyConfig_LoadBindings
(
    void
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    int index = 0;
    le_result_t result;

    // Open up a read transaction on the Config Tree
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn(BindingsConfigTreeNode);

    if (le_cfg_NodeExists(iteratorRef, "") == false)
    {
        LE_WARN("RPC Proxy '%s' configuration not found.", BindingsConfigTreeNode);

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iteratorRef);
        return LE_NOT_FOUND;
    }

    result = le_cfg_GoToFirstChild(iteratorRef);
    if (result != LE_OK)
    {
        LE_WARN("No configuration found.");

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iteratorRef);
        return result;
    }

    // Loop through all Service-binding nodes.
    do
    {
        // Get the Service-Name
        result = le_cfg_GetNodeName(iteratorRef, "", strBuffer, sizeof(strBuffer));
        if (result != LE_OK)
        {
            LE_ERROR("Service-Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }

        // Allocate memory from Configuration String pool to store the Service-Name
        char* serviceNameCopyPtr =
            le_mem_ForceAlloc(ConfigStringPoolRef);

        // Copy the Service-Name into the allocated memory
        le_utf8_Copy(serviceNameCopyPtr,
                     strBuffer,
                     LIMIT_MAX_IPC_INTERFACE_NAME_BYTES,
                     NULL);

        // Set the Service-Name pointer
        rpcProxy_SystemServiceArray[index].serviceName = serviceNameCopyPtr;

        // Get the System-Name
        result = le_cfg_GetString(iteratorRef,
                                  "systemName",
                                  strBuffer,
                                  sizeof(strBuffer),
                                  "");
        if (result != LE_OK)
        {
            LE_ERROR("System-Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }

        // Allocate memory from Configuration String pool to store the System-Name
        char* systemNameCopyPtr =
            le_mem_ForceAlloc(ConfigStringPoolRef);

        // Copy the System-Name into the allocated memory
        le_utf8_Copy(systemNameCopyPtr,
                     strBuffer,
                     LIMIT_MAX_IPC_INTERFACE_NAME_BYTES,
                     NULL);

        // Set the Service-Name pointer
        rpcProxy_SystemServiceArray[index].systemName = systemNameCopyPtr;

        // Get the Link-Name for this system
        result = LoadLinkNameFromConfigTree(systemNameCopyPtr, index);
        if (result != LE_OK)
        {
            LE_ERROR("Link-Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }

        // Get the Remote Service-Name
        result = le_cfg_GetString(iteratorRef,
                                  "remoteService",
                                  strBuffer,
                                  sizeof(strBuffer),
                                  "");
        if (result != LE_OK)
        {
            LE_ERROR("Remote Service-Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }

        // Allocate memory from Configuration String pool to store the Remote Service-Name
        char* remoteServiceNameCopyPtr =
            le_mem_ForceAlloc(ConfigStringPoolRef);

        // Copy the Remote Service-Name into the allocated memory
        le_utf8_Copy(remoteServiceNameCopyPtr,
                     strBuffer,
                     LIMIT_MAX_IPC_INTERFACE_NAME_BYTES,
                     NULL);

        // Set the Remote Service-Name pointer
        rpcProxy_SystemServiceArray[index].remoteServiceName = remoteServiceNameCopyPtr;

        index++;
    }
    while (le_cfg_GoToNextSibling(iteratorRef) == LE_OK);

    // Close the transaction and return success
    le_cfg_CancelTxn(iteratorRef);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the system-name using a service-name.
 */
//--------------------------------------------------------------------------------------------------
const char* rpcProxyConfig_GetSystemNameByServiceName
(
    const char* serviceName ///< Service name
)
{
    // Traverse all System-Service entries
    for (uint8_t index = 0; rpcProxy_SystemServiceArray[index].systemName; index++)
    {
        rpcProxy_SystemServiceConfig_t* serviceRefPtr = NULL;

        // Set a pointer the Service Reference element
        serviceRefPtr = &rpcProxy_SystemServiceArray[index];

        // Check if service name matches
        if (strcmp(serviceName, serviceRefPtr->serviceName) == 0)
        {
            return serviceRefPtr->systemName;
        }
    }

    LE_WARN("Unable to find matching service-name [%s]", serviceName);
    return "N/A";
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the remote service-name using a service-name.
 */
//--------------------------------------------------------------------------------------------------
const char* rpcProxyConfig_GetRemoteServiceNameByServiceName
(
    const char* serviceName ///< Service name
)
{
    // Traverse all System-Service entries
    for (uint8_t index = 0; rpcProxy_SystemServiceArray[index].systemName; index++)
    {
        rpcProxy_SystemServiceConfig_t* serviceRefPtr = NULL;

        // Set a pointer the Service Reference element
        serviceRefPtr = &rpcProxy_SystemServiceArray[index];

        // Check if service name matches
        if (strcmp(serviceName, serviceRefPtr->serviceName) == 0)
        {
            return serviceRefPtr->remoteServiceName;
        }
    }

    LE_WARN("Unable to find matching service-name [%s]", serviceName);
    return "N/A";
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the service-name using a remote service-name.
 */
//--------------------------------------------------------------------------------------------------
const char* rpcProxyConfig_GetServiceNameByRemoteServiceName
(
    const char* remoteServiceName ///< Remote service name
)
{
    // Traverse all System-Service entries
    for (uint8_t index = 0; rpcProxy_SystemServiceArray[index].systemName; index++)
    {
        rpcProxy_SystemServiceConfig_t* serviceRefPtr = NULL;

        // Set a pointer the Service Reference element
        serviceRefPtr = &rpcProxy_SystemServiceArray[index];

        // Check if service name matches
        if (strcmp(remoteServiceName, serviceRefPtr->remoteServiceName) == 0)
        {
            return serviceRefPtr->serviceName;
        }
    }

    LE_WARN("Unable to find matching remote service-name [%s]", remoteServiceName);
    return "N/A";
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the system-name using a link-name.
 */
//--------------------------------------------------------------------------------------------------
const char* rpcProxyConfig_GetSystemNameByLinkName
(
    const char* linkName ///< Link name
)
{
    // Traverse all System-Service entries
    for (uint8_t index = 0; rpcProxy_SystemServiceArray[index].systemName; index++)
    {
        rpcProxy_SystemServiceConfig_t* serviceRefPtr = NULL;

        // Set a pointer the Service Reference element
        serviceRefPtr = &rpcProxy_SystemServiceArray[index];

        // Check if the link name matches
        if (strcmp(linkName, serviceRefPtr->linkName) == 0)
        {
            return serviceRefPtr->systemName;
        }
    }

    LE_WARN("Unable to find matching link-name [%s]", linkName);
    return "N/A";
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC Configuration Service API to set a binding.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_rpc_SetBinding
(
    const char* LE_NONNULL serviceName,
        ///< [IN] External Interface-Name
    const char* LE_NONNULL systemName,
        ///< [IN] Remote System-Name
    const char* LE_NONNULL remoteServiceName
        ///< [IN] Remote Interface-Name
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";

    sprintf(strBuffer, "%s/%s/systemName", BindingsConfigTreeNode, serviceName);
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(strBuffer);
    le_cfg_SetString(iterRef, "", systemName);
    le_cfg_CommitTxn(iterRef);

    sprintf(strBuffer, "%s/%s/remoteService", BindingsConfigTreeNode, serviceName);
    iterRef = le_cfg_CreateWriteTxn(strBuffer);
    le_cfg_SetString(iterRef, "", remoteServiceName);
    le_cfg_CommitTxn(iterRef);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC Configuration Service API to get a binding.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_rpc_GetBinding
(
    const char* LE_NONNULL serviceName,
        ///< [IN] External RPC Interface-Name
    char* systemName,
        ///< [OUT] Remote System-Name
    size_t systemNameSize,
        ///< [IN]
    char* remoteServiceName,
        ///< [OUT] Remote RPC Interface-Name
    size_t remoteServiceNameSize
        ///< [IN]
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";

    // Verify the pointers are valid
    if ((systemName == NULL) ||
        (remoteServiceName == NULL))
    {
        LE_KILL_CLIENT("Invalid pointer");
        return LE_FAULT;
    }

    sprintf(strBuffer, "%s/%s/systemName", BindingsConfigTreeNode, serviceName);
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(strBuffer);
    le_cfg_GetString(iterRef, "", systemName, systemNameSize, "<EMPTY>");
    le_cfg_CommitTxn(iterRef);

    if (strcmp(systemName, "<EMPTY>") == 0)
    {
        return LE_NOT_FOUND;
    }

    sprintf(strBuffer, "%s/%s/remoteService", BindingsConfigTreeNode, serviceName);
    iterRef = le_cfg_CreateReadTxn(strBuffer);
    le_cfg_GetString(iterRef, "", remoteServiceName, remoteServiceNameSize, "<EMPTY>");
    le_cfg_CommitTxn(iterRef);

    if (strcmp(systemName, "<EMPTY>") == 0)
    {
        return LE_NOT_FOUND;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Service-Name of the first binding in the RPC Configuration tree.
 *
 * @return
 *  - LE_OK if successful
 *  - LE_OVERFLOW if the buffer provided is too small to hold the child's path.
 *  - LE_NOT_FOUND if the resource doesn't have any children.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_rpc_GetFirstBinding
(
    char* serviceName,
        ///< [OUT] External RPC Interface-Name
    size_t serviceNameSize
        ///< [IN]
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    le_result_t result;

    // Verify the pointers are valid
    if (serviceName == NULL)
    {
        LE_KILL_CLIENT("Invalid pointer");
        return LE_FAULT;
    }

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(BindingsConfigTreeNode);
    if (le_cfg_NodeExists(iterRef, "") == false)
    {
        LE_WARN("RPC Proxy '%s' configuration not found.", BindingsConfigTreeNode);

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iterRef);
        return LE_NOT_FOUND;
    }

    result = le_cfg_GoToFirstChild(iterRef);
    if (result != LE_OK)
    {
        LE_WARN("No configuration found.");

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iterRef);
        return result;
    }

    // Get the Service-Name
    result = le_cfg_GetNodeName(iterRef, "", strBuffer, sizeof(strBuffer));
    if (result != LE_OK)
    {
        LE_WARN("Service-Name configuration not found.");

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iterRef);
        return result;
    }

    // Copy the Service-Name
    strncpy(serviceName, strBuffer, serviceNameSize);
    le_cfg_CommitTxn(iterRef);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Service-Name of the next binding in the RPC Configuration tree.
 *
 * @return
 *  - LE_OK if successful
 *  - LE_OVERFLOW if the buffer provided is too small to hold the next sibling's path.
 *  - LE_NOT_FOUND if the resource is the last child in its parent's list of children.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_rpc_GetNextBinding
(
    const char* LE_NONNULL currentServiceName,
        ///< [IN] External RPC Interface-Name of previous binding.
    char* nextServiceName,
        ///< [OUT] External RPC Interface-Name of next binding.
    size_t nextServiceNameSize
        ///< [IN]
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    le_result_t result;

    // Verify the pointers are valid
    if (nextServiceName == NULL)
    {
        LE_KILL_CLIENT("Invalid pointer");
        return LE_FAULT;
    }

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(BindingsConfigTreeNode);
    le_cfg_GoToNode(iterRef, currentServiceName);
    if (le_cfg_IsEmpty(iterRef, ""))
    {
        LE_ERROR("Binding %s configuration not found", currentServiceName);

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iterRef);
        return LE_NOT_FOUND;
    }

    result = le_cfg_GoToNextSibling(iterRef);
    if (result != LE_OK)
    {
        LE_WARN("No configuration found.");

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iterRef);
        return LE_NOT_FOUND;
    }

    // Get the Service-Name
    result = le_cfg_GetNodeName(iterRef, "", strBuffer, sizeof(strBuffer));
    if (result != LE_OK)
    {
        LE_WARN("Service-Name configuration not found.");

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iterRef);
        return result;
    }

    // Copy the Next Service-Name
    strncpy(nextServiceName, strBuffer, nextServiceNameSize);
    le_cfg_CommitTxn(iterRef);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * RPC Configuration Service API to reset a binding.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_rpc_ResetBinding
(
    const char* LE_NONNULL serviceName
        ///< [IN] External RPC Interface-Name
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";

    sprintf(strBuffer, "%s", serviceName);
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(BindingsConfigTreeNode);
    le_cfg_DeleteNode(iterRef, strBuffer);
    le_cfg_CommitTxn(iterRef);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC Configuration Service API to set a system-link.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_rpc_SetSystemLink
(
    const char* LE_NONNULL systemName,
        ///< [IN] Remote System-Name
    const char* LE_NONNULL linkName,
        ///< [IN] System Link-Name
    const char* LE_NONNULL nodeName,
        ///< [IN] Configuration Node-Name
    const char* LE_NONNULL nodeValue
        ///< [IN] Configuration Node-Value
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";

    sprintf(strBuffer, "%s/%s/%s/%s", SystemsConfigTreeNode, systemName, linkName, nodeName);
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(strBuffer);
    le_cfg_SetString(iterRef, "", nodeValue);
    le_cfg_CommitTxn(iterRef);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC Configuration Service API to get a system-link.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_rpc_GetSystemLink
(
    const char* LE_NONNULL systemName,
        ///< [IN] Remote System-Name
    const char* LE_NONNULL linkName,
        ///< [IN] System Link-Name
    const char* LE_NONNULL nodeName,
        ///< [IN] Configuration Node-Name
    char* nodeValue,
        ///< [OUT] Configuration Node-Value
    size_t nodeValueSize
        ///< [IN]
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";

    // Verify the pointers are valid
    if (nodeValue == NULL)
    {
        LE_KILL_CLIENT("Invalid pointer");
        return LE_FAULT;
    }

    sprintf(strBuffer, "%s/%s/%s/%s", SystemsConfigTreeNode, systemName, linkName, nodeName);
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(strBuffer);
    le_cfg_GetString(iterRef, "", nodeValue, nodeValueSize, "<EMPTY>");
    le_cfg_CommitTxn(iterRef);

    if (strcmp(nodeValue, "<EMPTY>") == 0)
    {
        return LE_NOT_FOUND;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC Configuration Service API to reset a system-link.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_rpc_ResetSystemLink
(
    const char* LE_NONNULL systemName,
        ///< [IN] Remote System-Name
    const char* LE_NONNULL linkName
        ///< [IN] System Link-Name
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";

    sprintf(strBuffer, "%s/%s", systemName, linkName);
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(SystemsConfigTreeNode);
    le_cfg_DeleteNode(iterRef, strBuffer);
    le_cfg_CommitTxn(iterRef);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Link tree using the given configTree iterator.
 *
 * @return
 *  - LE_OK if successful
 *  - LE_OVERFLOW if the buffer provided is too small to hold the child's path.
 *  - LE_NOT_FOUND if the resource doesn't have any children.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetLinkTree
(
    le_cfg_IteratorRef_t iterRef,
    char*                linkName,
    size_t               linkNameSize,
    char*                nodeName,
    size_t               nodeNameSize
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    le_result_t result;

    // Get the Link-Name
    result = le_cfg_GetNodeName(iterRef, "", strBuffer, sizeof(strBuffer));
    if (result != LE_OK)
    {
        LE_WARN("Link-Name configuration not found.");
        return result;
    }

    // Copy the Link-Name
    strncpy(linkName, strBuffer, linkNameSize);

    result = le_cfg_GoToFirstChild(iterRef);
    if (result != LE_OK)
    {
        LE_WARN("No configuration found.");
        return result;
    }

    // Get the Node-Name
    result = le_cfg_GetNodeName(iterRef, "", strBuffer, sizeof(strBuffer));
    if (result != LE_OK)
    {
        LE_WARN("Node-Name configuration not found.");
        return result;
    }

    // Copy the Node-Name
    strncpy(nodeName, strBuffer, nodeNameSize);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the System tree using the given configTree iterator.
 *
 * @return
 *  - LE_OK if successful
 *  - LE_OVERFLOW if the buffer provided is too small to hold the child's path.
 *  - LE_NOT_FOUND if the resource doesn't have any children.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetSystemTree
(
    le_cfg_IteratorRef_t iterRef,
    char*                systemName,
    size_t               systemNameSize,
    char*                linkName,
    size_t               linkNameSize,
    char*                nodeName,
    size_t               nodeNameSize
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    le_result_t result;

    // Get the System-Name
    result = le_cfg_GetNodeName(iterRef, "", strBuffer, sizeof(strBuffer));
    if (result != LE_OK)
    {
        LE_WARN("System-Name configuration not found.");
        return result;
    }

    // Copy the system-name
    strncpy(systemName, strBuffer, systemNameSize);

    result = le_cfg_GoToFirstChild(iterRef);
    if (result != LE_OK)
    {
        LE_WARN("No configuration found.");
        return result;
    }

    // Get the Link tree for the given iterator
    result = GetLinkTree(iterRef,
                         linkName,
                         linkNameSize,
                         nodeName,
                         nodeNameSize);
    if (result != LE_OK)
    {
        LE_WARN("Link-Name configuration not found.");
        return result;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Node-Name of the first system-link in the RPC Configuration tree.
 *
 * @return
 *  - LE_OK if successful
 *  - LE_OVERFLOW if the buffer provided is too small to hold the child's path.
 *  - LE_NOT_FOUND if the resource doesn't have any children.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_rpc_GetFirstSystemLink
(
    char* systemName,
        ///< [OUT] System-Name
    size_t systemNameSize,
        ///< [IN]
    char* linkName,
        ///< [OUT] System Link-Name
    size_t linkNameSize,
        ///< [IN]
    char* nodeName,
        ///< [OUT] Configuration Node-Name
    size_t nodeNameSize
        ///< [IN]
)
{
    le_result_t result;

    // Verify the pointers are valid
    if ((systemName == NULL) ||
        (linkName == NULL) ||
        (nodeName == NULL))
    {
        LE_KILL_CLIENT("Invalid pointer");
        return LE_FAULT;
    }

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(SystemsConfigTreeNode);
    if (le_cfg_NodeExists(iterRef, "") == false)
    {
        LE_WARN("RPC Proxy '%s' configuration not found.", SystemsConfigTreeNode);

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iterRef);
        return LE_NOT_FOUND;
    }

    result = le_cfg_GoToFirstChild(iterRef);
    if (result != LE_OK)
    {
        LE_WARN("No configuration found.");

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iterRef);
        return result;
    }

    // Get the First System Link-Name for the given iterator
    result = GetSystemTree(iterRef,
                           systemName,
                           systemNameSize,
                           linkName,
                           linkNameSize,
                           nodeName,
                           nodeNameSize);

    if (result != LE_OK)
    {
        LE_WARN("System tree configuration not found.");

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iterRef);
        return result;
    }

    le_cfg_CommitTxn(iterRef);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Node-Name of the next system-link in the RPC Configuration tree.
 *
 * @return
 *  - LE_OK if successful
 *  - LE_OVERFLOW if the buffer provided is too small to hold the next sibling's path.
 *  - LE_NOT_FOUND if the resource is the last child in its parent's list of children.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_rpc_GetNextSystemLink
(
    const char* LE_NONNULL currentSystemName,
        ///< [IN] Current System-Name
    const char* LE_NONNULL currentLinkName,
        ///< [IN] Current System Link-Name
    const char* LE_NONNULL currentNodeName,
        ///< [IN] Current configuration Node-Name
    char* nextSystemName,
        ///< [OUT] Next System-Name
    size_t nextSystemNameSize,
        ///< [IN]
    char* nextLinkName,
        ///< [OUT] Next Link-Name
    size_t nextLinkNameSize,
        ///< [IN]
    char* nextNodeName,
        ///< [OUT] Next configuration Node-Name
    size_t nextNodeNameSize
        ///< [IN]
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    le_result_t result;

    // Verify the pointers are valid
    if ((nextSystemName == NULL) ||
        (nextLinkName == NULL) ||
        (nextNodeName == NULL))
    {
        LE_KILL_CLIENT("Invalid pointer");
        return LE_FAULT;
    }

    // Open up a read transaction on the Config Tree
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(SystemsConfigTreeNode);

    if (le_cfg_NodeExists(iterRef, "") == false)
    {
        LE_WARN("RPC Proxy '%s' configuration not found.", SystemsConfigTreeNode);

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iterRef);
        return LE_NOT_FOUND;
    }

    sprintf(strBuffer,"%s/%s/%s", currentSystemName, currentLinkName, currentNodeName);

    // Go to the current node
    le_cfg_GoToNode(iterRef, strBuffer);
    if (le_cfg_IsEmpty(iterRef, ""))
    {
        LE_ERROR("Node-Name %s configuration not found", currentNodeName);

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iterRef);
        return LE_NOT_FOUND;
    }

    // Go to the next sibling node, if one exists
    result = le_cfg_GoToNextSibling(iterRef);
    if (result != LE_OK)
    {
        // Go To the Link-Name
        result = le_cfg_GoToParent(iterRef);
        if (result != LE_OK)
        {
            // Close the transaction and return the failure
            le_cfg_CancelTxn(iterRef);
            return result;
        }

        // Got to the next Link-Name sibling, if one exists
        result = le_cfg_GoToNextSibling(iterRef);
        if (result != LE_OK)
        {
            // Go To the System-Name
            result = le_cfg_GoToParent(iterRef);
            if (result != LE_OK)
            {
                // Close the transaction and return the failure
                le_cfg_CancelTxn(iterRef);
                return result;
            }

            // Got the the next System-Name sibling, if one exists
            result = le_cfg_GoToNextSibling(iterRef);
            if (result != LE_OK)
            {
                // Close the transaction and return the failure
                le_cfg_CancelTxn(iterRef);
                return result;
            }

            // Get the System tree
            result = GetSystemTree(iterRef,
                                   nextSystemName,
                                   nextSystemNameSize,
                                   nextLinkName,
                                   nextLinkNameSize,
                                   nextNodeName,
                                   nextNodeNameSize);
            if (result != LE_OK)
            {
                LE_WARN("System tree configuration not found.");

                // Close the transaction and return the failure
                le_cfg_CancelTxn(iterRef);
                return result;
            }
        }
        else
        {
            // Get the Link tree
            result = GetLinkTree(iterRef,
                                 nextLinkName,
                                 nextLinkNameSize,
                                 nextNodeName,
                                 nextNodeNameSize);
            if (result != LE_OK)
            {
                LE_WARN("Link tree configuration not found.");

                // Close the transaction and return the failure
                le_cfg_CancelTxn(iterRef);
                return result;
            }
        }
    }
    else
    {
        // Get the Node-Name for the sibling
        result = le_cfg_GetNodeName(iterRef, "", strBuffer, sizeof(strBuffer));
        if (result != LE_OK)
        {
            LE_WARN("Node-Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iterRef);
            return result;
        }

        // Copy the System-Name, Link-Name, and Node-Name
        strncpy(nextSystemName, currentSystemName, nextSystemNameSize);
        strncpy(nextLinkName, currentLinkName, nextLinkNameSize);
        strncpy(nextNodeName, strBuffer, nextNodeNameSize);
    }

    le_cfg_CommitTxn(iterRef);
    return LE_OK;
}


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
)
{
    // Initialize memory pool for allocating System and Service Name Strings.
    ConfigStringPoolRef = le_mem_InitStaticPool(
                              ConfigStringPool,
                              RPC_PROXY_SERVICE_BINDINGS_MAX_NUM *
                              RPC_PROXY_CONFIG_STRING_PER_SERVICE_MAX_NUM,
                              LIMIT_MAX_IPC_INTERFACE_NAME_BYTES);

#ifndef RPC_PROXY_LOCAL_SERVICE
    // Initialize memory pool for allocating Protocol Id Strings.
    ConfigProtocolIdStringPoolRef = le_mem_InitStaticPool(ConfigProtocolIdStringPool,
                                                          RPC_PROXY_SERVICE_BINDINGS_MAX_NUM,
                                                          LIMIT_MAX_PROTOCOL_ID_BYTES);

    // Initialize memory pool for allocating System-Link Strings.
    ConfigSystemLinkStringPoolRef = le_mem_InitStaticPool(
                                        ConfigSystemLinkStringPool,
                                        RPC_PROXY_NETWORK_SYSTEM_MAX_NUM *
                                        RPC_PROXY_CONFIG_STRING_PER_SYSTEM_LINK_MAX_NUM,
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


    // Initialize Linux Server Reference Array
    memset(rpcProxy_LinuxServerReferenceArray, 0, sizeof(rpcProxy_LinuxServerReferenceArray));

    // Initialize Linux Client Reference Array
    memset(rpcProxy_LinuxClientReferenceArray, 0, sizeof(rpcProxy_LinuxClientReferenceArray));

    // Initialize System-Link Array
    memset(rpcProxy_SystemLinkArray, 0, sizeof(rpcProxy_SystemLinkArray));
#endif

    // Initialize System-Services Array
    memset(rpcProxy_SystemServiceArray, 0, sizeof(rpcProxy_SystemServiceArray));

    return LE_OK;
}
