/**
 * @file le_rpcProxyRuntimeStatic.c
 *
 * This file contains source code for implementing the RPC Proxy Run-time Configuration
 * Service using static-definitions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "le_rpcProxy.h"
#include "le_rpcProxyConfig.h"
#include "le_rpcProxyNetwork.h"
#include "le_rpc_common.h"


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
    LE_UNUSED(serviceName);
    LE_UNUSED(systemName);
    LE_UNUSED(remoteServiceName);

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
    LE_UNUSED(serviceName);
    LE_UNUSED(systemNameSize);
    LE_UNUSED(remoteServiceNameSize);

    // Verify the pointers are valid
    if ((systemName == NULL) ||
        (remoteServiceName == NULL) ||
        (serviceIdPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid pointer");
        return LE_FAULT;
    }

    *serviceIdPtr = 0;

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
le_result_t le_rpc_GetFirstSystemBinding
(
    char* serviceName,
        ///< [OUT] External RPC Interface-Name
    size_t serviceNameSize
        ///< [IN]
)
{
    LE_UNUSED(serviceNameSize);

    // Verify the pointers are valid
    if (serviceName == NULL)
    {
        LE_KILL_CLIENT("Invalid pointer");
        return LE_FAULT;
    }

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
    LE_UNUSED(currentServiceName);
    LE_UNUSED(nextServiceNameSize);

    // Verify the pointers are valid
    if (nextServiceName == NULL)
    {
        LE_KILL_CLIENT("Invalid pointer");
        return LE_FAULT;
    }

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
LE_SHARED le_result_t le_rpc_ResetSystemBinding
(
    const char* LE_NONNULL serviceName
        ///< [IN] External RPC Interface-Name
)
{
    LE_UNUSED(serviceName);

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
    const char* LE_NONNULL parameters
        ///< [IN] Parameters
)
{
    LE_UNUSED(systemName);
    LE_UNUSED(linkName);
    LE_UNUSED(parameters);

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
    LE_UNUSED(systemName);
    LE_UNUSED(linkNameSize);
    LE_UNUSED(parametersSize);

    // Verify the pointers are valid
    if ((linkName == NULL) ||
        (parameters == NULL) ||
        (statePtr == NULL))
    {
        LE_KILL_CLIENT("Invalid pointer");
        return LE_FAULT;
    }

    *statePtr = LE_RPC_NETWORK_UNKNOWN;

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
    LE_UNUSED(systemName);

    return LE_OK;
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
    // Verify the pointers are valid
    if (systemName == NULL)
    {
        LE_KILL_CLIENT("Invalid pointer");
        return LE_FAULT;
    }

    LE_UNUSED(systemNameSize);

    return LE_OK;
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
    LE_UNUSED(currentSystemName);
    LE_UNUSED(nextSystemName);
    LE_UNUSED(nextSystemNameSize);

    return LE_OK;
}