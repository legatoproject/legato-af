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
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn("rpcProxy/systems");

    if (le_cfg_NodeExists(iteratorRef, "") == false)
    {
        LE_WARN("RPC Proxy 'rpcProxy/systems' configuration not found.");

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
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn("rpcProxy/bindings");

    if (le_cfg_NodeExists(iteratorRef, "") == false)
    {
        LE_WARN("RPC Proxy 'rpcProxy/bindings' configuration not found.");

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
