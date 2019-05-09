/**
 * @file le_rpcProxyConfigCommon.c
 *
 * This file contains the common source code for the RPC Proxy Configuration Service.
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
extern rpcProxy_SystemServiceConfig_t rpcProxy_SystemServiceArray[];

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
)
{
    LE_INFO("Validating RPC Configuration");

    // Traverse all the Bindings in the System-service array
    for (uint32_t index = 0; rpcProxyConfig_GetSystemServiceArray(index).systemName; index++)
    {
        bool matchFound = false;
        const char* serviceName =
            rpcProxyConfig_GetSystemServiceArray(index).serviceName;

        LE_INFO("Searching for service '%s' in reference configuration",
                serviceName);

        // Search the Client-References for a matching Service-name
        for (uint32_t clientIndex = 0;
             rpcProxyConfig_GetClientReferenceArray(clientIndex) && !matchFound;
             clientIndex++)
        {
            // Set a pointer to the Client Reference element
            const rpcProxy_ExternClient_t* clientRefPtr =
                rpcProxyConfig_GetClientReferenceArray(clientIndex);

            if (clientRefPtr == NULL)
            {
                LE_ERROR("Client-Reference array is NULL");
                return LE_FAULT;
            }

            if (strcmp(serviceName, clientRefPtr->serviceName) == 0)
            {
                // Match found
                LE_INFO("Found match for service '%s' in client-reference configuration",
                        serviceName);

                // Set matchFound to true
                matchFound = true;
            }
        }

        // Search the Server-References for a matching Service-name
        for (uint32_t serverIndex = 0;
             rpcProxyConfig_GetServerReferenceArray(serverIndex) && !matchFound;
             serverIndex++)
        {
            // Set a pointer to the Server Reference element
            const rpcProxy_ExternServer_t* serverRefPtr =
                rpcProxyConfig_GetServerReferenceArray(serverIndex);

            if (serverRefPtr == NULL)
            {
                LE_ERROR("Server-Reference array is NULL");
                return LE_FAULT;
            }

            if (strcmp(serviceName, serverRefPtr->serviceName) == 0)
            {
                // Match found
                LE_INFO("Found match for service '%s' in server-reference configuration",
                        serviceName);

                // Set matchFound to true
                matchFound = true;
            }
        }

        if (!matchFound)
        {
            LE_ERROR("Unable to find service '%s' in reference configuration",
                     serviceName);

            return LE_NOT_FOUND;
        }

        // Set match found to false
        matchFound = false;

        const char* linkName =
            rpcProxyConfig_GetSystemServiceArray(index).linkName;

        LE_INFO("Searching for link '%s' in system-link configuration",
                linkName);

        // Search the System-links for a matching link-name
        for (uint32_t idx = 0;
             rpcProxyConfig_GetSystemLinkArray(idx).systemName && !matchFound;
             idx++)
        {
            if (strcmp(linkName, rpcProxyConfig_GetSystemLinkArray(idx).systemName) == 0)
            {
                // Match found
                LE_INFO("Found match for link '%s' in system-link configuration",
                        linkName);

                // Set matchFound to true
                matchFound = true;
            }
        }

        if (!matchFound)
        {
            LE_ERROR("Unable to find link '%s' in reference configuration",
                     linkName);

            return LE_NOT_FOUND;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Reads the Network System command-line configuration from the ConfigTree.
 *
 * NOTE: Format is,
 *
 * systems:
 * {
 *     "S1": {
 *         "LINK1": {
 *             "argc" : "2",
 *             "argv" : "10.1.1.2 443"
 *         },
 *     },
 *
 *     "S2": {
 *     }
 * }
 *
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_NOT_FOUND if node is not found.
 *      - LE_BAD_PARAMETER if number of elements exceeds the storage array size.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LoadSystemLinkCommandLineArgs
(
    const char* systemName, ///< System name
    const char* linkName, ///< Link name
    const uint8_t index ///< Current array element index
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    le_result_t result;

    // Open up a read transaction on the Config Tree
    le_cfg_IteratorRef_t iteratorRef =
        le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE);

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

    // Loop through all link-name nodes.
    do
    {
        // Get the Link-Name
        result = le_cfg_GetNodeName(iteratorRef, "", strBuffer, sizeof(strBuffer));
        if (result != LE_OK)
        {
            LE_WARN("Link-Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }

        // Check if this is the System we are provisioning
        if (strcmp(linkName, strBuffer) == 0)
        {
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
                rpcProxy_SystemServiceArray[index].argc = atoi(strBuffer);

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
                le_mem_ForceAlloc(rpcProxyConfig_GetArgumentArrayPoolRef());

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
                        le_mem_ForceAlloc(rpcProxyConfig_GetArgumentStringPoolRef());

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

                if (rpcProxy_SystemServiceArray[index].argc != argCount)
                {
                    LE_ERROR("Incorrect number of command-line arguments.");

                    // Close the transaction and return success
                    le_cfg_CancelTxn(iteratorRef);
                    return LE_BAD_PARAMETER;
                }

                // Set the last index to NULL
                argvCopyPtr[argCount] = NULL;

                // Set the Argument Variable pointer
                rpcProxy_SystemServiceArray[index].argv = argvCopyPtr;
            }
        }
        break;
    }
    while (le_cfg_GoToNextSibling(iteratorRef) == LE_OK);

    // Close the transaction and return success
    le_cfg_CancelTxn(iteratorRef);

    return LE_OK;
}


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
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_NOT_FOUND if node is not found.
 *      - LE_BAD_PARAMETER if number of elements exceeds the storage array size.
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
    le_cfg_IteratorRef_t iteratorRef =
        le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE);

    if (le_cfg_NodeExists(iteratorRef, "") == false)
    {
        LE_WARN("RPC Proxy '%s' configuration not found.", RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE);

        // Close the transaction and return the failure
        le_cfg_CancelTxn(iteratorRef);
        return LE_NOT_FOUND;
    }

    le_cfg_GoToNode(iteratorRef, systemName);
    if (le_cfg_IsEmpty(iteratorRef, ""))
    {
        LE_ERROR("System '%s' configuration not found", systemName);

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
        le_mem_ForceAlloc(rpcProxyConfig_GetStringPoolRef());

    // Copy the Link-Name into the allocated memory
    le_utf8_Copy(linkNameCopyPtr,
                 strBuffer,
                 LIMIT_MAX_IPC_INTERFACE_NAME_BYTES,
                 NULL);

    // Set the Link-Name pointer
    rpcProxy_SystemServiceArray[index].linkName = linkNameCopyPtr;

    // Load the command-line arguments for this link
    result = LoadSystemLinkCommandLineArgs(systemName, linkNameCopyPtr, index);

    // Close the transaction and return success
    le_cfg_CancelTxn(iteratorRef);

    return result;
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
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    int index = 0;
    le_result_t result;

    // Open up a read transaction on the Config Tree
    le_cfg_IteratorRef_t iteratorRef =
        le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_BINDINGS_TREE_NODE);

    if (le_cfg_NodeExists(iteratorRef, "") == false)
    {
        LE_WARN("RPC Proxy '%s' configuration not found.", RPC_PROXY_CONFIG_BINDINGS_TREE_NODE);

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
        // Check index has not exceeded storage array size
        if (index > RPC_PROXY_SERVICE_BINDINGS_MAX_NUM)
        {
            LE_ERROR("Too many RPC bindings.");

            // Close the transaction and return success
            le_cfg_CancelTxn(iteratorRef);
            return LE_BAD_PARAMETER;
        }

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
            le_mem_ForceAlloc(rpcProxyConfig_GetStringPoolRef());

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
            le_mem_ForceAlloc(rpcProxyConfig_GetStringPoolRef());

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
            le_mem_ForceAlloc(rpcProxyConfig_GetStringPoolRef());

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
    for (uint32_t index = 0; rpcProxyConfig_GetSystemServiceArray(index).systemName; index++)
    {
        // Check if service name matches
        if (strcmp(serviceName, rpcProxyConfig_GetSystemServiceArray(index).serviceName) == 0)
        {
            return rpcProxyConfig_GetSystemServiceArray(index).systemName;
        }
    }

    LE_WARN("Unable to find matching service-name [%s]", serviceName);
    return NULL;
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
    for (uint32_t index = 0; rpcProxyConfig_GetSystemServiceArray(index).systemName; index++)
    {
        // Check if service name matches
        if (strcmp(serviceName, rpcProxyConfig_GetSystemServiceArray(index).serviceName) == 0)
        {
            return rpcProxyConfig_GetSystemServiceArray(index).remoteServiceName;
        }
    }

    LE_WARN("Unable to find matching service-name [%s]", serviceName);
    return NULL;
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
    for (uint32_t index = 0; rpcProxyConfig_GetSystemServiceArray(index).systemName; index++)
    {
        // Check if service name matches
        if (strcmp(remoteServiceName,
                   rpcProxyConfig_GetSystemServiceArray(index).remoteServiceName) == 0)
        {
            return rpcProxyConfig_GetSystemServiceArray(index).serviceName;
        }
    }

    LE_WARN("Unable to find matching remote service-name [%s]", remoteServiceName);
    return NULL;
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
    for (uint32_t index = 0; rpcProxyConfig_GetSystemServiceArray(index).systemName; index++)
    {
        // Check if the link name matches
        if (strcmp(linkName, rpcProxyConfig_GetSystemServiceArray(index).linkName) == 0)
        {
            return rpcProxyConfig_GetSystemServiceArray(index).systemName;
        }
    }

    LE_WARN("Unable to find matching link-name [%s]", linkName);
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * RPC Configuration Service API to set a binding.
 *
 * @return
 *      - LE_OK if successful.
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

    sprintf(strBuffer, "%s/%s/systemName", RPC_PROXY_CONFIG_BINDINGS_TREE_NODE, serviceName);
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(strBuffer);
    le_cfg_SetString(iterRef, "", systemName);
    le_cfg_CommitTxn(iterRef);

    sprintf(strBuffer, "%s/%s/remoteService", RPC_PROXY_CONFIG_BINDINGS_TREE_NODE, serviceName);
    iterRef = le_cfg_CreateWriteTxn(strBuffer);
    le_cfg_SetString(iterRef, "", remoteServiceName);
    le_cfg_CommitTxn(iterRef);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC Configuration Service API to get a binding.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_NOT_FOUND if node is not found.
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

    sprintf(strBuffer, "%s/%s/systemName", RPC_PROXY_CONFIG_BINDINGS_TREE_NODE, serviceName);
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(strBuffer);
    le_cfg_GetString(iterRef, "", systemName, systemNameSize, "<EMPTY>");
    le_cfg_CancelTxn(iterRef);

    if (strcmp(systemName, "<EMPTY>") == 0)
    {
        return LE_NOT_FOUND;
    }

    sprintf(strBuffer, "%s/%s/remoteService", RPC_PROXY_CONFIG_BINDINGS_TREE_NODE, serviceName);
    iterRef = le_cfg_CreateReadTxn(strBuffer);
    le_cfg_GetString(iterRef, "", remoteServiceName, remoteServiceNameSize, "<EMPTY>");
    le_cfg_CancelTxn(iterRef);

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

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_BINDINGS_TREE_NODE);
    if (le_cfg_NodeExists(iterRef, "") == false)
    {
        LE_WARN("RPC Proxy '%s' configuration not found.", RPC_PROXY_CONFIG_BINDINGS_TREE_NODE);

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
    le_cfg_CancelTxn(iterRef);

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

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_BINDINGS_TREE_NODE);
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
    le_cfg_CancelTxn(iterRef);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * RPC Configuration Service API to reset a binding.
 *
 * @return
 *      - LE_OK if successful.
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
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(RPC_PROXY_CONFIG_BINDINGS_TREE_NODE);
    le_cfg_DeleteNode(iterRef, strBuffer);
    le_cfg_CommitTxn(iterRef);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC Configuration Service API to set a system-link.
 *
 * @return
 *      - LE_OK if successful.
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

    sprintf(strBuffer, "%s/%s/%s/%s",
            RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE,
            systemName,
            linkName,
            nodeName);

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(strBuffer);
    le_cfg_SetString(iterRef, "", nodeValue);
    le_cfg_CommitTxn(iterRef);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC Configuration Service API to get a system-link.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_NOT_FOUND if node is not found.
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

    sprintf(strBuffer, "%s/%s/%s/%s",
            RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE,
            systemName,
            linkName,
            nodeName);

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(strBuffer);
    le_cfg_GetString(iterRef, "", nodeValue, nodeValueSize, "<EMPTY>");
    le_cfg_CancelTxn(iterRef);

    if (strcmp(nodeValue, "<EMPTY>") == 0)
    {
        return LE_NOT_FOUND;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC Configuration Service API to reset a system-link.
 *
 * @return
 *      - LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_rpc_ResetSystemLink
(
    const char* LE_NONNULL systemName
        ///< [IN] Remote System-Name
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";

    sprintf(strBuffer, "%s", systemName);
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE);
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

    le_cfg_IteratorRef_t iterRef =
        le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE);

    if (le_cfg_NodeExists(iterRef, "") == false)
    {
        LE_WARN("RPC Proxy '%s' configuration not found.",
                RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE);

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

    le_cfg_CancelTxn(iterRef);
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
    le_cfg_IteratorRef_t iterRef =
        le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE);

    if (le_cfg_NodeExists(iterRef, "") == false)
    {
        LE_WARN("RPC Proxy '%s' configuration not found.",
                RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE);

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

    le_cfg_CancelTxn(iterRef);
    return LE_OK;
}

