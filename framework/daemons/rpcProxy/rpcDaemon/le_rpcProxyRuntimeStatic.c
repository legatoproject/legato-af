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

        // Make sure linkName is unique for each system.
        for (uint32_t i = 0; rpcProxyConfig_GetSystemServiceArray(i)->systemName; i++)
        {
            if(index == i)
            {
                continue;
            }

            if (strcmp(linkName, rpcProxyConfig_GetSystemServiceArray(i)->linkName) == 0)
            {
                const char* pName1 = rpcProxyConfig_GetSystemServiceArray(index)->systemName;
                const char* pName2 = rpcProxyConfig_GetSystemServiceArray(i)->systemName;

                if (strcmp(pName1, pName2) != 0)
                {
                    LE_ERROR("Systems: %s and %s have same link name %s, %d %d",
                             pName1, pName2, linkName, (int)i, (int)index);
                    return LE_FAULT;
                }
            }
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
