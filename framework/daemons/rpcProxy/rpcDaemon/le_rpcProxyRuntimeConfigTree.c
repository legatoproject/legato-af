/**
 * @file le_rpcProxyRuntimeConfigTree.c
 *
 * This file contains source code for implementing the RPC Proxy Run-time Configuration
 * Service using the Legato Configuration Tree.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "le_rpcProxy.h"
#include "le_rpcProxyConfig.h"
#include "le_rpcProxyNetwork.h"
#include "le_cfg_interface.h"
#include "le_rpc_common.h"

//--------------------------------------------------------------------------------------------------
/**
 * Extern Declarations
 */
//--------------------------------------------------------------------------------------------------
extern rpcProxy_SystemServiceConfig_t rpcProxy_SystemServiceArray[];

//--------------------------------------------------------------------------------------------------
/**
 * RPC Proxy Search String defines.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_CONFIG_SYSTEM_NAME_STR                "systemName"
#define RPC_PROXY_CONFIG_SERVICE_NAME_STR               "serviceName"
#define RPC_PROXY_CONFIG_LINK_NAME_STR                  "linkName"
#define RPC_PROXY_CONFIG_REMOTE_SERVICE_STR             "remoteService"
#define RPC_PROXY_CONFIG_PARAMETERS_STR                 "parameters"
#define RPC_PROXY_CONFIG_SYSTEM_NAME_SEARCH_STR         "%d/systemName"
#define RPC_PROXY_CONFIG_SERVICE_NAME_SEARCH_STR        "%d/serviceName"
#define RPC_PROXY_CONFIG_LINK_NAME_SEARCH_STR           "%d/linkName"
#define RPC_PROXY_CONFIG_REMOTE_SERVICE_SEARCH_STR      "%d/remoteService"
#define RPC_PROXY_CONFIG_PARAMETERS_SEARCH_STR          "%d/parameters"

#define RPC_PROXY_CONFIG_EMPTY_STR                      "<EMPTY>"
#define RPC_PROXY_CONFIG_END_STR                        "<END>"



//--------------------------------------------------------------------------------------------------
/**
 * Helper function for retrieving a RPC Proxy Configuration string based on the
 * specified search pattern and index.
 *
 * @return
 *      - LE_OK if successful, error otherwise.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetConfigString
(
    le_cfg_IteratorRef_t iteratorRef,  ///< Iterator Reference
    char *searchStrPattern,  ///< Search string pattern
    char *defaultStrPattern,  ///< Default string pattern
    int searchIdx, ///< Configuration search index
    char *strBuffer,  ///< Buffer to store the string result
    size_t strBufferSize ///< Size of string buffer
)
{
    char searchStr[LE_CFG_STR_LEN_BYTES] = "";

    // Create a search string that includes the search pattern and search index
    snprintf(searchStr,
             sizeof(searchStr),
             searchStrPattern,
             searchIdx);

    // Get the configuration string
    le_result_t result =
        le_cfg_GetString(iteratorRef,
                         searchStr,
                         strBuffer,
                         strBufferSize,
                         defaultStrPattern);

    return result;
}


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
    for (uint32_t index = 0; rpcProxyConfig_GetSystemServiceArray(index)->systemName; index++)
    {
        bool matchFound = false;
        const char* serviceName =
            rpcProxyConfig_GetSystemServiceArray(index)->serviceName;

        // Verify the Service-Name is not NULL
        if (serviceName == NULL)
        {
            LE_ERROR("Invalid service-name on system '%s'",
                     rpcProxyConfig_GetSystemServiceArray(index)->systemName);
            return LE_NOT_FOUND;
        }

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
            rpcProxyConfig_GetSystemServiceArray(index)->linkName;

        // Verify the Link-Name is not NULL
        if (linkName == NULL)
        {
            LE_ERROR("Invalid link-name on service '%s'", serviceName);
            return LE_NOT_FOUND;
        }

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
 * Reads the System configuration from the 'Systems' ConfigTree.
 *
 * - Each system is numerically indexed in the configTree and is comprised of a
 * systemName, linkName, and parameters.
 * - Numerical indexing must start at zero, and must be consecutive.
 * - Indices containing "<EMPTY>" systemName entries will be treated as empty and skipped.
 * - Systems are linked to the "Bindings" configuration through the systemName,
 * which is common to both.
 *
 * NOTE: Format is,
 *
 *  "system:/rpcProxy/systems/0": {
 *      "systemName": "Alice",
 *      "linkName": "LinkToAlice",
 *      "parameters": "192.168.3.5 54323",
 *  }
 *
 * (NOTE: Currently, only one link-name is supported at a time)
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_NOT_FOUND if node is not found.
 *      - LE_BAD_PARAMETER if number of elements exceeds the storage array size.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LoadSystemsFromConfigTree
(
    const char* systemName, ///< System name
    const uint8_t index ///< Current array element index
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    int searchIdx = 0;
    le_result_t result;

    // Open up a read transaction on the Config Tree
    le_cfg_IteratorRef_t iteratorRef =
        le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE);

    do
    {
        // Get the System-Name string
        result = GetConfigString(
                     iteratorRef,
                     RPC_PROXY_CONFIG_SYSTEM_NAME_SEARCH_STR,
                     RPC_PROXY_CONFIG_END_STR,
                     searchIdx,
                     strBuffer,
                     sizeof(strBuffer));

        if ((result != LE_OK) ||
            (strcmp(strBuffer, RPC_PROXY_CONFIG_END_STR) == 0))
        {
            LE_ERROR("System-Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return (result != LE_OK) ? result : LE_BAD_PARAMETER;
        }

        if ((strcmp(strBuffer, RPC_PROXY_CONFIG_EMPTY_STR) == 0) ||
            (strcmp(strBuffer, systemName) != 0))
        {
            searchIdx++;
            continue;
        }

        // Get the Link-Name string
        result = GetConfigString(
                     iteratorRef,
                     RPC_PROXY_CONFIG_LINK_NAME_SEARCH_STR,
                     RPC_PROXY_CONFIG_EMPTY_STR,
                     searchIdx,
                     strBuffer,
                     sizeof(strBuffer));

        if ((result != LE_OK) ||
            (strcmp(strBuffer, RPC_PROXY_CONFIG_EMPTY_STR) == 0))
        {
            LE_ERROR("System-Link Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return (result != LE_OK) ? result : LE_BAD_PARAMETER;
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

        // Get the Parameters string
        result = GetConfigString(
                     iteratorRef,
                     RPC_PROXY_CONFIG_PARAMETERS_SEARCH_STR,
                     RPC_PROXY_CONFIG_EMPTY_STR,
                     searchIdx,
                     strBuffer,
                     sizeof(strBuffer));

        if ((result != LE_OK) ||
            (strcmp(strBuffer, RPC_PROXY_CONFIG_EMPTY_STR) == 0))
        {
            LE_WARN("Parameters configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return (result != LE_OK) ? result : LE_BAD_PARAMETER;
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

        // Set the last index to NULL
        argvCopyPtr[argCount] = NULL;

        // Set the Argument Variable pointer
        rpcProxy_SystemServiceArray[index].argv = argvCopyPtr;
        rpcProxy_SystemServiceArray[index].argc = argCount;

        searchIdx++;
    }
    while ((GetConfigString(
                iteratorRef,
                RPC_PROXY_CONFIG_SYSTEM_NAME_SEARCH_STR,
                RPC_PROXY_CONFIG_END_STR,
                searchIdx,
                strBuffer,
                sizeof(strBuffer)) == LE_OK) &&
           (strcmp(strBuffer, RPC_PROXY_CONFIG_END_STR) != 0));

    // Close the transaction and return success
    le_cfg_CancelTxn(iteratorRef);

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Reads the System-Service Bindings configuration from the ConfigTree.
 *
 * - Each binding is numerically indexed in the configTree and is comprised of a serviceName,
 * systemName, and remoteService.
 * - Numerical indexing must start at zero, and must be consecutive.
 * - Indices containing "<EMPTY>" serviceName entries will be treated as empty and skipped.
 * - Bindings are linked to the "Systems" configuration through the systemName,
 * which is common to both.
 *
 * NOTE: Format is,
 *
 *  "system:/rpcProxy/bindings/0": {
 *      "serviceName": "bobClient",
 *      "systemName": "Alice",
 *      "remoteService": "bobServer",
 *  },
 *
 *  "system:/rpcProxy/bindings/1": {
 *      "serviceName": "aliceServer",
 *      "systemName": "Alice",
 *      "remoteService": "aliceClient",
 *  },
 *
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
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    int index = 0;
    int searchIdx = 0;
    le_result_t result;

    // Open up a read transaction on the Config Tree
    le_cfg_IteratorRef_t iteratorRef =
        le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_BINDINGS_TREE_NODE);

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

        // Get the Service Name string
        result = GetConfigString(
                     iteratorRef,
                     RPC_PROXY_CONFIG_SERVICE_NAME_SEARCH_STR,
                     RPC_PROXY_CONFIG_END_STR,
                     searchIdx,
                     strBuffer,
                     sizeof(strBuffer));

        if ((result != LE_OK) ||
            (strcmp(strBuffer, RPC_PROXY_CONFIG_END_STR) == 0))
        {
            LE_ERROR("Service-Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return (result != LE_OK) ? result : LE_BAD_PARAMETER;
        }

        if (strcmp(strBuffer, RPC_PROXY_CONFIG_EMPTY_STR) == 0)
        {
            searchIdx++;
            continue;
        }

        // Allocate memory from Configuration String pool to store the Service-Name
        char* serviceNameCopyPtr =
            le_mem_ForceAlloc(rpcProxyConfig_GetStringPoolRef());

        // Copy the Service-Name into the allocated memory
        le_utf8_Copy(serviceNameCopyPtr,
                     strBuffer,
                     LIMIT_MAX_IPC_INTERFACE_NAME_BYTES,
                     NULL);

        LE_INFO("Loading binding, service-name [%s], index [%d]", serviceNameCopyPtr, index);

        // Set the Service-Name pointer
        rpcProxy_SystemServiceArray[index].serviceName = serviceNameCopyPtr;

        // Get the System-Name string
        result = GetConfigString(
                     iteratorRef,
                     RPC_PROXY_CONFIG_SYSTEM_NAME_SEARCH_STR,
                     RPC_PROXY_CONFIG_EMPTY_STR,
                     searchIdx,
                     strBuffer,
                     sizeof(strBuffer));

        if ((result != LE_OK) ||
            (strcmp(strBuffer, RPC_PROXY_CONFIG_EMPTY_STR) == 0))
        {
            LE_ERROR("System-Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return (result != LE_OK) ? result : LE_BAD_PARAMETER;
        }

        // Allocate memory from Configuration String pool to store the System-Name
        char* systemNameCopyPtr =
            le_mem_ForceAlloc(rpcProxyConfig_GetStringPoolRef());

        // Copy the System-Name into the allocated memory
        le_utf8_Copy(systemNameCopyPtr,
                     strBuffer,
                     LIMIT_MAX_SYSTEM_NAME_BYTES,
                     NULL);

        // Set the Service-Name pointer
        rpcProxy_SystemServiceArray[index].systemName = systemNameCopyPtr;

        // Get the System Configuration associated with this system
        result = LoadSystemsFromConfigTree(systemNameCopyPtr, index);
        if (result != LE_OK)
        {
            LE_ERROR("Link-Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }

        // Get the Remote Serivce-Name string
        result = GetConfigString(
                     iteratorRef,
                     RPC_PROXY_CONFIG_REMOTE_SERVICE_SEARCH_STR,
                     RPC_PROXY_CONFIG_EMPTY_STR,
                     searchIdx,
                     strBuffer,
                     sizeof(strBuffer));

        if ((result != LE_OK) ||
            (strcmp(strBuffer, RPC_PROXY_CONFIG_EMPTY_STR) == 0))
        {
            LE_ERROR("Remote Service-Name configuration not found.");

            // Close the transaction and return the failure
            le_cfg_CancelTxn(iteratorRef);
            return (result != LE_OK) ? result : LE_BAD_PARAMETER;
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
        searchIdx++;
    }
    while ((GetConfigString(
                iteratorRef,
                RPC_PROXY_CONFIG_SERVICE_NAME_SEARCH_STR,
                RPC_PROXY_CONFIG_END_STR,
                searchIdx,
                strBuffer,
                sizeof(strBuffer)) == LE_OK) &&
           (strcmp(strBuffer, RPC_PROXY_CONFIG_END_STR) != 0));

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
    for (uint32_t index = 0; rpcProxyConfig_GetSystemServiceArray(index)->systemName; index++)
    {
        // Check if service name matches
        if (strcmp(serviceName, rpcProxyConfig_GetSystemServiceArray(index)->serviceName) == 0)
        {
            return rpcProxyConfig_GetSystemServiceArray(index)->systemName;
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
    for (uint32_t index = 0; rpcProxyConfig_GetSystemServiceArray(index)->systemName; index++)
    {
        // Check if service name matches
        if (strcmp(serviceName, rpcProxyConfig_GetSystemServiceArray(index)->serviceName) == 0)
        {
            return rpcProxyConfig_GetSystemServiceArray(index)->remoteServiceName;
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
    for (uint32_t index = 0; rpcProxyConfig_GetSystemServiceArray(index)->systemName; index++)
    {
        // Check if service name matches
        if (strcmp(remoteServiceName,
                   rpcProxyConfig_GetSystemServiceArray(index)->remoteServiceName) == 0)
        {
            return rpcProxyConfig_GetSystemServiceArray(index)->serviceName;
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
    for (uint32_t index = 0; rpcProxyConfig_GetSystemServiceArray(index)->systemName; index++)
    {
        // Check if the link name matches
        if (strcmp(linkName, rpcProxyConfig_GetSystemServiceArray(index)->linkName) == 0)
        {
            return rpcProxyConfig_GetSystemServiceArray(index)->systemName;
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
le_result_t le_rpc_SetSystemBinding
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
    int index = 0;
    le_cfg_IteratorRef_t iteratorRef;

    // Open up a read transaction on the Config Tree
    iteratorRef =
        le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_BINDINGS_TREE_NODE);

    // Search for the next available index in the Binding tree
    while ((GetConfigString(
                iteratorRef,
                RPC_PROXY_CONFIG_SERVICE_NAME_SEARCH_STR,
                RPC_PROXY_CONFIG_EMPTY_STR,
                index,
                strBuffer,
                sizeof(strBuffer)) == LE_OK) &&
           (strcmp(strBuffer, RPC_PROXY_CONFIG_EMPTY_STR) != 0) &&
           (strcmp(strBuffer, serviceName) != 0))
    {
        // Increment index counter
        index++;
    }

    // Close the transaction
    le_cfg_CancelTxn(iteratorRef);

    snprintf(strBuffer,
             sizeof(strBuffer),
             "%s/%d",
             RPC_PROXY_CONFIG_BINDINGS_TREE_NODE,
             index);

    iteratorRef = le_cfg_CreateWriteTxn(strBuffer);
    le_cfg_SetString(iteratorRef, RPC_PROXY_CONFIG_SERVICE_NAME_STR, serviceName);
    le_cfg_SetString(iteratorRef, RPC_PROXY_CONFIG_SYSTEM_NAME_STR, systemName);
    le_cfg_SetString(iteratorRef, RPC_PROXY_CONFIG_REMOTE_SERVICE_STR, remoteServiceName);
    le_cfg_CommitTxn(iteratorRef);

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
le_result_t le_rpc_GetSystemBinding
(
    const char* LE_NONNULL serviceName,
        ///< [IN] External RPC Interface-Name
    char* systemName,
        ///< [OUT] Remote System-Name
    size_t systemNameSize,
        ///< [IN]
    char* remoteServiceName,
        ///< [OUT] Remote RPC Interface-Name
    size_t remoteServiceNameSize,
        ///< [IN]
    uint32_t* serviceIdPtr
        ///< [OUT] Service ID (Non-zero if successfully connected, zero otherwise)
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    int index = 0;
    le_cfg_IteratorRef_t iteratorRef;

    // Verify the pointers are valid
    if ((systemName == NULL) ||
        (remoteServiceName == NULL) ||
        (serviceIdPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid pointer");
        return LE_FAULT;
    }

    // Open up a read transaction on the Config Tree
    iteratorRef =
        le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_BINDINGS_TREE_NODE);

    // Search for the serviceName in the Binding tree
    while ((GetConfigString(
                iteratorRef,
                RPC_PROXY_CONFIG_SERVICE_NAME_SEARCH_STR,
                RPC_PROXY_CONFIG_END_STR,
                index,
                strBuffer,
                sizeof(strBuffer)) == LE_OK) &&
           (strcmp(strBuffer, RPC_PROXY_CONFIG_END_STR) != 0))
    {
        if (strcmp(strBuffer, serviceName) == 0)
        {
            // Get the System-Name string
            GetConfigString(
                iteratorRef,
                RPC_PROXY_CONFIG_SYSTEM_NAME_SEARCH_STR,
                RPC_PROXY_CONFIG_EMPTY_STR,
                index,
                systemName,
                systemNameSize);

            // Get the Remote Service-Name string
            GetConfigString(
                iteratorRef,
                RPC_PROXY_CONFIG_REMOTE_SERVICE_SEARCH_STR,
                RPC_PROXY_CONFIG_EMPTY_STR,
                index,
                remoteServiceName,
                remoteServiceNameSize);

            // Close the transaction
            le_cfg_CancelTxn(iteratorRef);

            // Initialize the Service-ID to zero
            *serviceIdPtr = 0;

            // Retrieve the Service-ID HashMap Reference
            le_hashmap_Ref_t serviceIdHashMapRef =
                rpcProxy_GetServiceIDMapByName();

            if (serviceIdHashMapRef != NULL)
            {
                // Retrieve the Service-ID, using the service-name
                uint32_t* serviceIdCopyPtr =
                    le_hashmap_Get(serviceIdHashMapRef, serviceName);

                if (serviceIdCopyPtr != NULL)
                {
                    // Set the Service-ID
                    *serviceIdPtr = *serviceIdCopyPtr;
                }
            }
            return LE_OK;
        }

        // Increment index counter
        index++;
    }

    // Close the transaction
    le_cfg_CancelTxn(iteratorRef);
    return LE_NOT_FOUND;
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
le_result_t le_rpc_GetFirstSystemBinding
(
    char* serviceName,
        ///< [OUT] External RPC Interface-Name
    size_t serviceNameSize
        ///< [IN]
)
{
    le_cfg_IteratorRef_t iteratorRef;
    int index = 0;
    le_result_t result = LE_OK;

    // Verify the pointers are valid
    if (serviceName == NULL)
    {
        LE_KILL_CLIENT("Invalid pointer");
        return LE_FAULT;
    }

    // Open up a read transaction on the Config Tree
    iteratorRef =
        le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_BINDINGS_TREE_NODE);

    // Search for the currentServiceName in the Binding tree
    while ((GetConfigString(
                iteratorRef,
                RPC_PROXY_CONFIG_SERVICE_NAME_SEARCH_STR,
                RPC_PROXY_CONFIG_END_STR,
                index,
                serviceName,
                serviceNameSize) == LE_OK) &&
           (strcmp(serviceName, RPC_PROXY_CONFIG_END_STR) != 0))
    {
        if (strcmp(serviceName, RPC_PROXY_CONFIG_EMPTY_STR) == 0)
        {
            // Increment index counter
            index++;
            continue;
        }

        // Close the transaction
        le_cfg_CancelTxn(iteratorRef);
        return LE_OK;
    }

    if ((result != LE_OK) ||
        (strcmp(serviceName, RPC_PROXY_CONFIG_END_STR) == 0))
    {
        LE_WARN("Service-Name configuration not found.");
        result = LE_NOT_FOUND;
    }

    // Close the transaction
    le_cfg_CancelTxn(iteratorRef);
    return result;
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
le_result_t le_rpc_GetNextSystemBinding
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
    int index = 0;
    le_cfg_IteratorRef_t iteratorRef;
    le_result_t result = LE_OK;

    // Verify the pointers are valid
    if (nextServiceName == NULL)
    {
        LE_KILL_CLIENT("Invalid pointer");
        return LE_FAULT;
    }

    // Open up a read transaction on the Config Tree
    iteratorRef =
        le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_BINDINGS_TREE_NODE);

    // Search for the currentServiceName in the Binding tree
    while ((GetConfigString(
                iteratorRef,
                RPC_PROXY_CONFIG_SERVICE_NAME_SEARCH_STR,
                RPC_PROXY_CONFIG_END_STR,
                index,
                strBuffer,
                sizeof(strBuffer)) == LE_OK) &&
           (strcmp(strBuffer, RPC_PROXY_CONFIG_END_STR) != 0))
    {
        if (strcmp(strBuffer, RPC_PROXY_CONFIG_EMPTY_STR) == 0)
        {
            // Increment index counter
            index++;
            continue;
        }

        if (strcmp(strBuffer, currentServiceName) == 0)
        {
            // Increment index counter
            index++;

            // Search for the next ServiceName in the Binding tree
            while ((GetConfigString(
                        iteratorRef,
                        RPC_PROXY_CONFIG_SERVICE_NAME_SEARCH_STR,
                        RPC_PROXY_CONFIG_END_STR,
                        index,
                        nextServiceName,
                        nextServiceNameSize) == LE_OK) &&
                   (strcmp(nextServiceName, RPC_PROXY_CONFIG_END_STR) != 0))
            {
                if (strcmp(nextServiceName, RPC_PROXY_CONFIG_EMPTY_STR) == 0)
                {
                    // Increment index counter
                    index++;
                    continue;
                }

                // Close the transaction
                le_cfg_CancelTxn(iteratorRef);
                return LE_OK;
            }

            if ((result != LE_OK) ||
                (strcmp(nextServiceName, RPC_PROXY_CONFIG_END_STR) == 0))
            {
                // Cannot retrieve the next serviceName
                result = LE_NOT_FOUND;
            }

            // Close the transaction
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }

        // Increment index counter
        index++;
    }

    // Close the transaction
    le_cfg_CancelTxn(iteratorRef);
    return LE_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------------
/**
 * RPC Configuration Service API to reset a binding.
 *
 * @return
 *      - LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_rpc_ResetSystemBinding
(
    const char* LE_NONNULL serviceName
        ///< [IN] External RPC Interface-Name
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    int index = 0;
    le_cfg_IteratorRef_t iteratorRef;

    // Open up a read transaction on the Config Tree
    iteratorRef =
        le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_BINDINGS_TREE_NODE);

    // Search for the next available index in the Binding tree
    while ((GetConfigString(
                iteratorRef,
                RPC_PROXY_CONFIG_SERVICE_NAME_SEARCH_STR,
                RPC_PROXY_CONFIG_END_STR,
                index,
                strBuffer,
                sizeof(strBuffer)) == LE_OK) &&
           (strcmp(strBuffer, RPC_PROXY_CONFIG_END_STR) != 0))
    {
        if (strcmp(strBuffer, serviceName) == 0)
        {
            // Close the transaction
            le_cfg_CancelTxn(iteratorRef);

            snprintf(strBuffer,
                     sizeof(strBuffer),
                     "%s/%d", RPC_PROXY_CONFIG_BINDINGS_TREE_NODE,
                     index);

            iteratorRef = le_cfg_CreateWriteTxn(strBuffer);
            le_cfg_SetString(iteratorRef,
                             RPC_PROXY_CONFIG_SERVICE_NAME_STR,
                             RPC_PROXY_CONFIG_EMPTY_STR);

            le_cfg_SetString(iteratorRef,
                             RPC_PROXY_CONFIG_SYSTEM_NAME_STR,
                             RPC_PROXY_CONFIG_EMPTY_STR);

            le_cfg_SetString(iteratorRef,
                             RPC_PROXY_CONFIG_REMOTE_SERVICE_STR,
                             RPC_PROXY_CONFIG_EMPTY_STR);

            le_cfg_CommitTxn(iteratorRef);
            return LE_OK;
        }

        // Increment index counter
        index++;
    }

    LE_WARN("Service-Name configuration not found.");

    // Close the transaction and return success
    le_cfg_CancelTxn(iteratorRef);
    return LE_NOT_FOUND;
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
    const char* LE_NONNULL parameters
        ///< [IN] Parameters
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    le_cfg_IteratorRef_t iteratorRef;
    int index = 0;

    // Open up a read transaction on the Config Tree
    iteratorRef =
        le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE);

    // Search for the next available index in the Systems tree
    while ((GetConfigString(
                iteratorRef,
                RPC_PROXY_CONFIG_SYSTEM_NAME_SEARCH_STR,
                RPC_PROXY_CONFIG_EMPTY_STR,
                index,
                strBuffer,
                sizeof(strBuffer)) == LE_OK) &&
           (strcmp(strBuffer, RPC_PROXY_CONFIG_EMPTY_STR) != 0) &&
           (strcmp(strBuffer, systemName) != 0))
    {
        // Increment index counter
        index++;
    }

    // Close the transaction
    le_cfg_CancelTxn(iteratorRef);

    snprintf(strBuffer,
             sizeof(strBuffer),
             "%s/%d",
             RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE,
             index);

    iteratorRef = le_cfg_CreateWriteTxn(strBuffer);
    le_cfg_SetString(iteratorRef, RPC_PROXY_CONFIG_SYSTEM_NAME_STR, systemName);
    le_cfg_SetString(iteratorRef, RPC_PROXY_CONFIG_LINK_NAME_STR, linkName);
    le_cfg_SetString(iteratorRef, RPC_PROXY_CONFIG_PARAMETERS_STR, parameters);
    le_cfg_CommitTxn(iteratorRef);

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
    char* linkName,
        ///< [OUT] System Link-Name
    size_t linkNameSize,
        ///< [IN]
    char* parameters,
        ///< [OUT] Parameters
    size_t parametersSize,
        ///< [IN]
    le_rpc_NetworkState_t* statePtr
        ///< [OUT] Current Network Link State
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    int index = 0;
    le_cfg_IteratorRef_t iteratorRef;

    // Verify the pointers are valid
    if ((linkName == NULL) ||
        (parameters == NULL) ||
        (statePtr == NULL))
    {
        LE_KILL_CLIENT("Invalid pointer");
        return LE_FAULT;
    }

    // Open up a read transaction on the Config Tree
    iteratorRef =
        le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE);

    // Search for the systemName in the Systems tree
    while ((GetConfigString(
                iteratorRef,
                RPC_PROXY_CONFIG_SYSTEM_NAME_SEARCH_STR,
                RPC_PROXY_CONFIG_END_STR,
                index,
                strBuffer,
                sizeof(strBuffer)) == LE_OK) &&
           (strcmp(strBuffer, RPC_PROXY_CONFIG_END_STR) != 0))
    {
        if (strcmp(strBuffer, systemName) == 0)
        {
            // Get the Link-Name string
            GetConfigString(
                iteratorRef,
                RPC_PROXY_CONFIG_LINK_NAME_SEARCH_STR,
                RPC_PROXY_CONFIG_EMPTY_STR,
                index,
                linkName,
                linkNameSize);

            // Get the Parameters string
            GetConfigString(
                iteratorRef,
                RPC_PROXY_CONFIG_PARAMETERS_SEARCH_STR,
                RPC_PROXY_CONFIG_EMPTY_STR,
                index,
                parameters,
                parametersSize);

            // Initialize the Network State to UNKNOWN
            *statePtr = LE_RPC_NETWORK_UNKNOWN;

            // Retrieve the Network Record HashMap Reference
            le_hashmap_Ref_t networkRecordHashMapRef =
                rpcProxyNetwork_GetNetworkRecordHashMapByName();

            if (networkRecordHashMapRef != NULL)
            {
                // Retrieve the Network Record for this system
                NetworkRecord_t* networkRecordPtr =
                    le_hashmap_Get(networkRecordHashMapRef, systemName);

                if (networkRecordPtr != NULL)
                {
                    if (networkRecordPtr->state == NETWORK_DOWN)
                    {
                        // Set the Network State to DOWN
                        *statePtr = LE_RPC_NETWORK_DOWN;
                    }
                    else if (networkRecordPtr->state == NETWORK_UP)
                    {
                        // Set the Network State to UP
                        *statePtr = LE_RPC_NETWORK_UP;
                    }
                }
            }

            // Close the transaction
            le_cfg_CancelTxn(iteratorRef);
            return LE_OK;
        }

        // Increment index counter
        index++;
    }

    // Close the transaction
    le_cfg_CancelTxn(iteratorRef);
    return LE_NOT_FOUND;
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
    int index = 0;
    le_cfg_IteratorRef_t iteratorRef;

    // Open up a read transaction on the Config Tree
    iteratorRef =
        le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE);

    // Search for the next available index in the Systems tree
    while ((GetConfigString(
                iteratorRef,
                RPC_PROXY_CONFIG_SYSTEM_NAME_SEARCH_STR,
                RPC_PROXY_CONFIG_END_STR,
                index,
                strBuffer,
                sizeof(strBuffer)) == LE_OK) &&
           (strcmp(strBuffer, RPC_PROXY_CONFIG_END_STR) != 0))
    {
        if (strcmp(strBuffer, systemName) == 0)
        {
            // Close the transaction
            le_cfg_CancelTxn(iteratorRef);

            snprintf(strBuffer,
                     sizeof(strBuffer),
                     "%s/%d",
                     RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE,
                     index);

            iteratorRef = le_cfg_CreateWriteTxn(strBuffer);
            le_cfg_SetString(iteratorRef,
                             RPC_PROXY_CONFIG_SYSTEM_NAME_STR,
                             RPC_PROXY_CONFIG_EMPTY_STR);

            le_cfg_SetString(iteratorRef,
                             RPC_PROXY_CONFIG_LINK_NAME_STR,
                             RPC_PROXY_CONFIG_EMPTY_STR);

            le_cfg_SetString(iteratorRef,
                             RPC_PROXY_CONFIG_PARAMETERS_STR,
                             RPC_PROXY_CONFIG_EMPTY_STR);

            le_cfg_CommitTxn(iteratorRef);
            return LE_OK;
        }

        // Increment index counter
        index++;
    }

    LE_WARN("System-Name configuration not found.");

    // Close the transaction and return success
    le_cfg_CancelTxn(iteratorRef);
    return LE_NOT_FOUND;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the first system-link in the RPC Configuration tree.
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
    size_t systemNameSize
        ///< [IN]
)
{
    le_cfg_IteratorRef_t iteratorRef;
    int index = 0;
    le_result_t result = LE_OK;

    // Verify the pointers are valid
    if (systemName == NULL)
    {
        LE_KILL_CLIENT("Invalid pointer");
        return LE_FAULT;
    }

    // Open up a read transaction on the Config Tree
    iteratorRef =
        le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE);

    // Search for the systemName in the Systems tree
    while ((GetConfigString(
                iteratorRef,
                RPC_PROXY_CONFIG_SYSTEM_NAME_SEARCH_STR,
                RPC_PROXY_CONFIG_END_STR,
                index,
                systemName,
                systemNameSize) == LE_OK) &&
           (strcmp(systemName, RPC_PROXY_CONFIG_END_STR) != 0))
    {
        if (strcmp(systemName, RPC_PROXY_CONFIG_EMPTY_STR) == 0)
        {
            // Increment index counter
            index++;
            continue;
        }

        // Close the transaction
        le_cfg_CancelTxn(iteratorRef);
        return LE_OK;
    }

    if ((result != LE_OK) ||
        (strcmp(systemName, RPC_PROXY_CONFIG_END_STR) == 0))
    {
        LE_WARN("System-Name configuration not found.");
        result = LE_NOT_FOUND;
    }

    // Close the transaction
    le_cfg_CancelTxn(iteratorRef);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the next system-link in the RPC Configuration tree.
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
    char* nextSystemName,
        ///< [OUT] Next System-Name
    size_t nextSystemNameSize
        ///< [IN]
)
{
    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
    int index = 0;
    le_cfg_IteratorRef_t iteratorRef;
    le_result_t result = LE_OK;

    // Verify the pointers are valid
    if (nextSystemName == NULL)
    {
        LE_KILL_CLIENT("Invalid pointer");
        return LE_FAULT;
    }

    // Open up a read transaction on the Config Tree
    iteratorRef =
        le_cfg_CreateReadTxn(RPC_PROXY_CONFIG_SYSTEMS_TREE_NODE);

    // Search for the systemName in the Systems tree
    while ((GetConfigString(
                iteratorRef,
                RPC_PROXY_CONFIG_SYSTEM_NAME_SEARCH_STR,
                RPC_PROXY_CONFIG_END_STR,
                index,
                strBuffer,
                sizeof(strBuffer)) == LE_OK) &&
           (strcmp(strBuffer, RPC_PROXY_CONFIG_END_STR) != 0))
    {
        if (strcmp(strBuffer, RPC_PROXY_CONFIG_EMPTY_STR) == 0)
        {
            // Increment index counter
            index++;
            continue;
        }

        if (strcmp(strBuffer, currentSystemName) == 0)
        {
            // Increment index counter
            index++;

            // Search for the next systemName in the Systems tree
            while ((GetConfigString(
                        iteratorRef,
                        RPC_PROXY_CONFIG_SYSTEM_NAME_SEARCH_STR,
                        RPC_PROXY_CONFIG_END_STR,
                        index,
                        nextSystemName,
                        nextSystemNameSize) == LE_OK) &&
                   (strcmp(nextSystemName, RPC_PROXY_CONFIG_END_STR) != 0))
            {
                if (strcmp(nextSystemName, RPC_PROXY_CONFIG_EMPTY_STR) == 0)
                {
                    // Increment index counter
                    index++;
                    continue;
                }

                // Close the transaction
                le_cfg_CancelTxn(iteratorRef);
                return LE_OK;
            }

            if ((result != LE_OK) ||
                (strcmp(nextSystemName, RPC_PROXY_CONFIG_END_STR) == 0))
            {
                // Cannot retrieve the next systemName
                result = LE_NOT_FOUND;
            }

            // Close the transaction
            le_cfg_CancelTxn(iteratorRef);
            return result;
        }

        // Increment index counter
        index++;
    }

    // Close the transaction
    le_cfg_CancelTxn(iteratorRef);
    return LE_NOT_FOUND;
}