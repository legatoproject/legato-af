/**
 * @file le_rpcProxyNetwork.c
 *
 * This file contains the source code for the RPC Proxy Network Service.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "le_rpcProxy.h"
#include "le_rpcProxyConfig.h"
#include "le_rpcProxyNetwork.h"
#include "le_rpcProxyFileStream.h"
#include "le_comm.h"


//--------------------------------------------------------------------------------------------------
/**
 * Extern declaration for system links
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Forward Function Declarations
 */
//--------------------------------------------------------------------------------------------------
static void AsyncConnectionCallbackHandler(void* handle, short events);


//--------------------------------------------------------------------------------------------------
/**
 * Hash Map to store the System-Name (value) by Asynchronous Communication handle (key).
 * Initialized in COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(SystemNameByAsyncHandleHashMap, RPC_PROXY_NETWORK_SYSTEM_MAX_NUM);
static le_hashmap_Ref_t SystemNameByAsyncHandle = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * This pool is used to allocate memory for the Network Timer record, which is used for
 * Network RECONNECT and KEEPALIVE timer events.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(NetworkTimerRecordPool,
                          RPC_PROXY_NETWORK_TIMER_RECORD_MAX_NUM,
                          sizeof(NetworkTimerRecord_t));
static le_mem_PoolRef_t NetworkTimerRecordPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * This pool is used to allocate memory for the Network Communication record.
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(NetworkRecordPool,
                          RPC_PROXY_NETWORK_SYSTEM_MAX_NUM,
                          sizeof(NetworkRecord_t));
static le_mem_PoolRef_t NetworkRecordPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Hash Map to store Network Record structures (value), using the System-name (key).
 * Initialized in rpcProxy_COMPONENT_INIT().
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(NetworkRecordHashMap, RPC_PROXY_NETWORK_SYSTEM_MAX_NUM);
static le_hashmap_Ref_t NetworkRecordHashMapByName = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Expired Network-related Timers
 */
//--------------------------------------------------------------------------------------------------
static void NetworkTimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    NetworkTimerRecord_t* networkTimerRecordPtr;
    void* handle;
    char* systemName;
    le_result_t result;

    // Retrieve ContextPtr data (network record)
    networkTimerRecordPtr = le_timer_GetContextPtr(timerRef);

    if (networkTimerRecordPtr == NULL)
    {
        LE_ERROR("Error extracting Network Record from timer event");
        return;
    }

    // Extract the system handle and name
    handle = networkTimerRecordPtr->record.handle;
    systemName = networkTimerRecordPtr->systemName;

    switch (networkTimerRecordPtr->event)
    {
        case RECONNECT:  /// Network Re-connect timer
        {
            if (networkTimerRecordPtr->record.state == NETWORK_DOWN)
            {
                //
                // Network is DOWN - try to reconnect
                //
                LE_INFO("Network Status: DOWN, system-name [%s], "
                        "handle [%d] - check if network is reachable",
                        systemName,
                        le_comm_GetId(handle));

                // Create and connect Network Communication channel
                result = rpcProxyNetwork_CreateNetworkCommunicationChannel(systemName);
                if (result == LE_IN_PROGRESS)
                {
                    LE_INFO("Network Status: WAITING, system-name [%s], "
                            "handle [%d] - deleting connection timer",
                            systemName,
                            le_comm_GetId(handle));

                    goto exit;
                }
                else if (result != LE_OK)
                {
                    LE_INFO("Network Status: DOWN, system-name [%s], "
                            "handle [%d] - restarting connection timer",
                            systemName,
                            le_comm_GetId(handle));

                    le_timer_Start(timerRef);
                    return;
                }

                // Retrieve the Network Record for this system, if one exists
                NetworkRecord_t* networkRecordPtr =
                    le_hashmap_Get(NetworkRecordHashMapByName, systemName);

                if (networkRecordPtr == NULL)
                {
                    LE_ERROR("Unable to retrieve network record, "
                             "system-name [%s] - unknown system",
                             systemName);
                    goto exit;
                }

                LE_INFO("Network Status: UP, system-name [%s], "
                        "handle [%d] - stopping connection timer",
                        systemName,
                        le_comm_GetId(handle));

                // Start the Advertise-Service sequence for services being hosted by the RPC Proxy
                // NOTE:  The advertise-service will only be completed once we have successfully performed
                //        a connect-service on the far-side
                rpcProxy_AdvertiseServices(systemName);
            }
            else
            {
                LE_INFO("Network Status: UP, system-name [%s], "
                        "handle [%d] - stopping connection timer",
                        systemName,
                        le_comm_GetId(handle));
            }
            break;
        }

        case KEEPALIVE:   /// Network Keep-Alive timer
        {
            // Retrieve the Network Record for this system, if one exists
            NetworkRecord_t* networkRecordPtr =
                le_hashmap_Get(NetworkRecordHashMapByName, systemName);

            if (networkRecordPtr == NULL)
            {
                LE_ERROR("Unable to retrieve network record, "
                         "system-name [%s] - unknown system",
                         systemName);
                goto exit;
            }

            if (networkRecordPtr->state == NETWORK_UP)
            {
                // Generate a Network Keepalive event
                rpcProxyNetwork_SendKeepAliveRequest(systemName);

                // Kick off the timer again
                le_timer_Start(timerRef);
                return;
            }
            break;
        }

        default:
            LE_ERROR("Unknown Network Timer Record event, event [%d]",
                     networkTimerRecordPtr->event);
            break;
    }

exit:
    // Free Network Status record memory
    le_mem_Release(networkTimerRecordPtr);

    // Delete Timer
    le_timer_Delete(timerRef);
    timerRef = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start Network Connection Retry timer.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyNetwork_StartNetworkConnectionRetryTimer
(
    const char* systemName ///< System name
)
{
    // Retrieve the Network Record for this system, if one exists
    NetworkRecord_t* networkRecordPtr =
       le_hashmap_Get(NetworkRecordHashMapByName, systemName);

    LE_INFO("Network is unavailable, system-name [%s] - "
            "starting timer (%d secs.) to trigger a retry",
            systemName,
            LE_CONFIG_RPC_PROXY_NETWORK_CONNECTION_RETRY_TIMER_INTERVAL);

    //
    // Set-up Network Status timer to periodically attempt to bring-up Network Connection
    //
    le_timer_Ref_t  networkStatusTimerRef;
    le_clk_Time_t timerInterval =
        { .sec=LE_CONFIG_RPC_PROXY_NETWORK_CONNECTION_RETRY_TIMER_INTERVAL, .usec=0 };

    // Create a timer to trigger a Network status check
    networkStatusTimerRef = le_timer_Create("Network-Status timer");
    le_timer_SetInterval(networkStatusTimerRef, timerInterval);
    le_timer_SetHandler(networkStatusTimerRef, NetworkTimerExpiryHandler);
    le_timer_SetWakeup(networkStatusTimerRef, false);

    // Allocate memory for the Network Timer event
    NetworkTimerRecord_t* networkTimerPtr = le_mem_Alloc(NetworkTimerRecordPoolRef);

    // Set the Network Timer event type
    networkTimerPtr->event = RECONNECT;

    // Set the Network Timer system-name
    le_utf8_Copy(networkTimerPtr->systemName,
                 systemName,
                 sizeof(networkTimerPtr->systemName),
                 NULL);

    // Set the Network Timer network record
    if (networkRecordPtr)
    {
        memcpy(&networkTimerPtr->record, networkRecordPtr, sizeof(networkTimerPtr->record));
    }
    else
    {
        // Reset Network Record
        networkTimerPtr->record.handle = NULL;
        networkTimerPtr->record.state = NETWORK_DOWN;
        networkTimerPtr->record.type = UNKNOWN;

        // Reset Network Message Re-assembly State-Machine
        networkTimerPtr->record.messageState.recvState = NETWORK_MSG_IDLE;
    }

    // Set Network Status record  in the timer event
    le_timer_SetContextPtr(networkStatusTimerRef, networkTimerPtr);

    // Start timer
    le_timer_Start(networkStatusTimerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Start Network Keep-Alive service.
 */
//--------------------------------------------------------------------------------------------------
static void StartNetworkKeepAliveService
(
    const char* systemName, ///< System name
    NetworkRecord_t* networkRecordPtr ///< Pointer to the Proxy Message
)
{
    // Sanity Check - Verify Network Keep-Alive timer reference is NULL
    if (networkRecordPtr->keepAliveTimerRef != NULL)
    {
        // Keep-alive timer is already running
        LE_ERROR("Network-KEEPALIVE Service Timer is already running, "
                 "system-name [%s], handle [%d] - silently ignore",
                 systemName,
                 le_comm_GetId(networkRecordPtr->handle));
        return;
    }

    LE_INFO("Starting Network-KEEPALIVE Service - frequency is %d seconds, "
            "system-name [%s], handle [%d]",
            RPC_PROXY_NETWORK_KEEPALIVE_SERVICE_INTERVAL,
            systemName,
            le_comm_GetId(networkRecordPtr->handle));

    //
    // Set-up Network Keep-Alive timer to periodically
    // check if network connection is still alive
    //
    NetworkTimerRecord_t* networkTimerPtr = NULL;
    le_clk_Time_t timerInterval = {.sec=RPC_PROXY_NETWORK_KEEPALIVE_SERVICE_INTERVAL, .usec=0};

    // Create a timer to trigger a Network Keep-Alive check
    networkRecordPtr->keepAliveTimerRef = le_timer_Create("Network-KEEPALIVE Service timer");
    le_timer_SetInterval(networkRecordPtr->keepAliveTimerRef, timerInterval);
    le_timer_SetHandler(networkRecordPtr->keepAliveTimerRef, NetworkTimerExpiryHandler);
    le_timer_SetWakeup(networkRecordPtr->keepAliveTimerRef, false);

    // Allocate memory for the Network Timer event
    networkTimerPtr = le_mem_Alloc(NetworkTimerRecordPoolRef);
    networkTimerPtr->event = KEEPALIVE;
    le_utf8_Copy(networkTimerPtr->systemName,
                 systemName,
                 sizeof(networkTimerPtr->systemName),
                 NULL);

    // Set the Network Record
    memcpy(&networkTimerPtr->record, networkRecordPtr, sizeof(networkTimerPtr->record));

    // Set Network Status record  in the timer event
    le_timer_SetContextPtr(networkRecordPtr->keepAliveTimerRef, networkTimerPtr);

    // Start timer
    le_timer_Start(networkRecordPtr->keepAliveTimerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop Network Keep-Alive service.
 */
//--------------------------------------------------------------------------------------------------
static void StopNetworkKeepAliveService
(
    const char* systemName, ///< System name
    NetworkRecord_t* networkRecordPtr ///< Pointer to the Proxy Message
)
{
    le_result_t result;

    // Sanity Check - Verify Network Keep-Alive timer reference is not NULL
    if (networkRecordPtr->keepAliveTimerRef == NULL)
    {
        // Keep-alive service is not running
        return;
    }

    // Sanity Check - Verify Keep-Alive service is not running
    if (!le_timer_IsRunning(networkRecordPtr->keepAliveTimerRef))
    {
        // Keep-alive timer is not running
        LE_ERROR("Network-KEEPALIVE Service timer is not running, system-name [%s]",
                 systemName);
        return;
    }

    LE_INFO("Stopping Network-KEEPALIVE Service, system-name [%s]",
            systemName);

    result = le_timer_Stop(networkRecordPtr->keepAliveTimerRef);
    if (result != LE_OK)
    {
        LE_ERROR("Error stopping Network-KEEPALIVE Service timer, system-name [%s], result [%d]",
                 systemName,
                 result);
        return;
    }

    // Get the timer's context pointer
    NetworkTimerRecord_t* networkTimerPtr =
        le_timer_GetContextPtr(networkRecordPtr->keepAliveTimerRef);

    // Free the memory allocated for the Network Timer record
    le_mem_Release(networkTimerPtr);

    // Delete Timer
    le_timer_Delete(networkRecordPtr->keepAliveTimerRef);
    networkRecordPtr->keepAliveTimerRef = NULL;

    //
    // Next, clean-up the Keep-Alive expiry-timer associated
    // with this system, if set
    //

    // Create iterator to traverse ExpiryTimerRefByProxyId map
    le_hashmap_It_Ref_t iter =
        le_hashmap_GetIterator(rpcProxy_GetExpiryTimerRefByProxyId());

    // Boolean to track when we are done
    bool done = false;

    // Traverse the entire ExpiryTimerRefByProxyId map
    while ((le_hashmap_NextNode(iter) == LE_OK) && !done)
    {
        // Get the timer reference
        le_timer_Ref_t timerRef =
            (le_timer_Ref_t) le_hashmap_GetValue(iter);

        if (timerRef == NULL)
        {
            LE_ERROR("Error retrieving the expiry-timer reference");
            return;
        }

        // Retrieve ContextPtr data (Proxy Message Common Header copy)
        rpcProxy_CommonHeader_t* commonHeaderPtr =
            (rpcProxy_CommonHeader_t*) le_timer_GetContextPtr(timerRef);

        if (commonHeaderPtr == NULL)
        {
            LE_ERROR("Error extracting copy of Proxy Message from timer record");
            return;
        }

        // Switch on the Message Type
        switch (commonHeaderPtr->type)
        {
            case RPC_PROXY_KEEPALIVE_REQUEST:
            {
                rpcProxy_KeepAliveMessage_t* proxyMessageCopyPtr =
                    (rpcProxy_KeepAliveMessage_t*) le_timer_GetContextPtr(timerRef);

                if (proxyMessageCopyPtr == NULL)
                {
                    LE_ERROR("Unable to retrieve copy of the "
                             "Proxy Keep-Alive Message Reference");
                    continue;
                }

                // Check if this belongs to the system that is being prcoessed
                if (strcmp(proxyMessageCopyPtr->systemName, systemName) == 0)
                {
                    // Found matching Keep-Alive expiry-timer for this system
                    LE_INFO("Removing KEEPALIVE-Request expiry-timer, "
                            "system [%s]",
                            proxyMessageCopyPtr->systemName);

                    // Remove entry from hash-map
                    le_hashmap_Remove(
                        rpcProxy_GetExpiryTimerRefByProxyId(),
                        (void*)(uintptr_t) proxyMessageCopyPtr->commonHeader.id);

                    // Free Proxy Message Copy Memory
                    le_mem_Release(proxyMessageCopyPtr);

                    // Delete Timer
                    le_timer_Delete(timerRef);
                    timerRef = NULL;

                    // We are done
                    done = true;
                }
                break;
            }

            default:
                break;
        } /* End of switch-statement */
    } /* End of while-loop */
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the systemName using the communication handle (reverse look-up).
 */
//--------------------------------------------------------------------------------------------------
char* rpcProxyNetwork_GetSystemNameByHandle
(
    void* handle ///< Opaque handle to a le_comm communication channel
)
{
    le_hashmap_It_Ref_t iter = le_hashmap_GetIterator(NetworkRecordHashMapByName);
    while (le_hashmap_NextNode(iter) == LE_OK)
    {
        NetworkRecord_t* networkRecordPtr = le_hashmap_GetValue(iter);

        // Search for matching handles
        if (networkRecordPtr->handle == handle)
        {
            // Return the system-name (key)
            return (char*) le_hashmap_GetKey(iter);
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the Network Record, using the communication handle (reverse look-up).
 */
//--------------------------------------------------------------------------------------------------
NetworkRecord_t* rpcProxyNetwork_GetNetworkRecordByHandle
(
    void* handle ///< Opaque handle to a le_comm communication channel
)
{
    le_hashmap_It_Ref_t iter = le_hashmap_GetIterator(NetworkRecordHashMapByName);
    while (le_hashmap_NextNode(iter) == LE_OK)
    {
        NetworkRecord_t* networkRecordPtr = le_hashmap_GetValue(iter);

        // Search for matching handles
        if (networkRecordPtr->handle == handle)
        {
            // Return the netword record (value)
            return (NetworkRecord_t*) le_hashmap_GetValue(iter);
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the Network Record Hash-map By Name reference.
 */
//--------------------------------------------------------------------------------------------------
le_hashmap_Ref_t rpcProxyNetwork_GetNetworkRecordHashMapByName
(
    void
)
{
    return NetworkRecordHashMapByName;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for creating and connecting a Network Communication Channel.
 *
 * @return
 *      - LE_OK, if successfully,
 *      - LE_IN_PROGRESS, if pending on asynchronous connection,
 *      - otherwise failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyNetwork_CreateNetworkCommunicationChannel
(
    const char* systemName ///< System name
)
{
    le_result_t result = LE_OK;

    NetworkRecord_t* networkRecordPtr = le_hashmap_Get(NetworkRecordHashMapByName, systemName);
    if (networkRecordPtr == NULL)
    {
        // Need allocate a Network Record
        networkRecordPtr = le_mem_Alloc(NetworkRecordPoolRef);

        // Initialize the Network Record
        networkRecordPtr->state = NETWORK_DOWN;
        networkRecordPtr->type = UNKNOWN;
        networkRecordPtr->handle = NULL;
        networkRecordPtr->keepAliveTimerRef = NULL;

        le_hashmap_Put(NetworkRecordHashMapByName, systemName, networkRecordPtr);
    }

    LE_INFO("Creating network communication channel, system-name [%s], handle [%d]",
            systemName,
            le_comm_GetId(networkRecordPtr->handle));

    // Reset Network Message Re-assembly State-Machine
    networkRecordPtr->messageState.recvState = NETWORK_MSG_IDLE;

    LE_ASSERT(networkRecordPtr->handle == NULL);

    // Traverse the System-Link array and
    // retrieve the argc and argv[] arguments configured for the systemName
    for (uint32_t index = 0; rpcProxyConfig_GetSystemServiceArray(index)->systemName; index++)
    {
        if (strcmp(rpcProxyConfig_GetSystemServiceArray(index)->systemName, systemName) == 0)
        {
            // Create the Network Connection, passing in the command-line
            // arguments that were read from the RPC Proxy links configuration
            networkRecordPtr->handle =
                le_comm_Create(
                    rpcProxyConfig_GetSystemServiceArray(index)->argc,
                    rpcProxyConfig_GetSystemServiceArray(index)->argv,
                    &result);

            if ((result != LE_OK) ||
                (networkRecordPtr->handle == NULL))
            {
                LE_INFO("Unable to Create RPC Communication Handle, result [%d]", result);
                return LE_FAULT;
            }

            LE_DEBUG("Successfully created network communication channel, "
                     "system-name [%s], handle [%d], result [%d]",
                     systemName,
                     le_comm_GetId(networkRecordPtr->handle),
                     result);

            // Register Connection Callback Handler to receive asynchronous connections.
            result = le_comm_RegisterHandleMonitor(
                        networkRecordPtr->handle,
                        &AsyncConnectionCallbackHandler,
                        0x00);

            if (result != LE_OK)
            {
                LE_INFO("Unable to register callback handler for "
                        "RPC responses, result %d", result);

                // Delete Communication channel
                le_comm_Delete(networkRecordPtr->handle);
                networkRecordPtr->handle = NULL;
                return result;
            }

            break;
        } // End of if-statement
    } // End of for-loop statement

    // Check if the handle is still NULL.
    // NOTE: This may occur if the systemName match is not found.
    if (networkRecordPtr->handle == NULL)
    {
        return LE_BAD_PARAMETER;
    }

    // Connect RPC Communication channel
    result = le_comm_Connect(networkRecordPtr->handle);
    if ((result != LE_OK) && (result != LE_IN_PROGRESS))
    {
        // NOTE: If connect() fails, consider the state of the socket as unspecified.
        // Portable applications should close the socket and create a new one for reconnecting.
        LE_DEBUG("Unable to connect Communication channel, "
                 "system-name [%s], handle [%d], result %d",
                 systemName,
                 le_comm_GetId(networkRecordPtr->handle),
                 result);

        // Delete Communication channel
        le_comm_Delete(networkRecordPtr->handle);
        networkRecordPtr->handle = NULL;
        return result;
    }

    if (result == LE_IN_PROGRESS)
    {
        LE_INFO("Waiting for out-of-band connection callback, system-name [%s], handle [%d]",
                systemName,
                le_comm_GetId(networkRecordPtr->handle));

        // Set the Network Type
        networkRecordPtr->type = ASYNC;

        // Store the system-name, using the Asynchronous Communication Handle
        le_hashmap_Put(SystemNameByAsyncHandle, networkRecordPtr->handle, systemName);
        return result;
    }

    // Register Callback Handler to receive incoming RPC messages asynchronously.
    result = le_comm_RegisterHandleMonitor(
                networkRecordPtr->handle,
                &rpcProxy_AsyncRecvHandler,
                POLLIN | POLLRDHUP | POLLERR);

    if (result != LE_OK)
    {
        LE_INFO("Unable to register callback handler for "
                "RPC responses, result %d", result);

        // Delete Communication channel
        le_comm_Delete(networkRecordPtr->handle);
        networkRecordPtr->handle = NULL;
        return result;
    }

    LE_DEBUG("Successfully connected network communication channel, "
             "system-name [%s], handle [%d]",
             systemName,
             le_comm_GetId(networkRecordPtr->handle));

    // Mark Network Connection State as UP
    networkRecordPtr->state = NETWORK_UP;
    networkRecordPtr->type = SYNC;

    // Start Keep-Alive service to monitor the health of the network
    StartNetworkKeepAliveService(systemName, networkRecordPtr);
    LE_INFO("Network Status: UP, system-name [%s], handle [%d] - ready to receive events",
            systemName,
            le_comm_GetId(networkRecordPtr->handle));

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for deleting a Network Communication Channel, using system-name.
 *
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyNetwork_DeleteNetworkCommunicationChannel
(
    const char* systemName ///< System name
)
{
    le_result_t result;

    NetworkRecord_t* networkRecordPtr = le_hashmap_Get(NetworkRecordHashMapByName, systemName);
    if (networkRecordPtr == NULL)
    {
        LE_ERROR("Unable to retrieve matching Network Record, "
                 "system-name [%s] - unknown system", systemName);
        return;
    }

    // Verify the Network State and handle
    if ((networkRecordPtr->state == NETWORK_DOWN) ||
        (networkRecordPtr->handle == NULL))
    {
        return;
    }

    LE_INFO("Network Status: DOWN, system-name [%s], "
            "handle [%d] - deleting communication channel",
            systemName,
            le_comm_GetId(networkRecordPtr->handle));

    // Hide all affected services
    rpcProxy_HideServices(systemName);
    rpcProxy_DisconnectSessions(systemName);
    rpcFStream_DeleteStreamsBySystemName(systemName);

    // Delete the Communication channel
    result = le_comm_Delete(networkRecordPtr->handle);
    networkRecordPtr->handle = NULL;
    if (result != LE_OK)
    {
        LE_ERROR("Unable to delete Communication channel, result %d", result);
    }

    // Set Network Connection state to DOWN
    networkRecordPtr->state = NETWORK_DOWN;

    // Reset Network Message Re-assembly State-Machine
    networkRecordPtr->messageState.recvState = NETWORK_MSG_IDLE;

    // Stop Network Keep-Alive service
    StopNetworkKeepAliveService(systemName, networkRecordPtr);

    // Start Network Connection Retry timer to periodically
    // attempt to bring-up Network Connection
    rpcProxyNetwork_StartNetworkConnectionRetryTimer(systemName);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for deleting a Network Communication Channel, using the communication handle.
 *
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyNetwork_DeleteNetworkCommunicationChannelByHandle
(
    void* handle ///< Opaque handle to a le_comm communication channel
)
{
    char* systemName = NULL;

    if (handle == NULL)
    {
        return;
    }

    // Perform a reverse look-up and retrieve the system-name using the handle
    systemName = rpcProxyNetwork_GetSystemNameByHandle(handle);
    if (systemName)
    {
        // Delete the Network Communication Channel
        rpcProxyNetwork_DeleteNetworkCommunicationChannel(systemName);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for connecting a Network Communication Channel.
 *
 * @return
 *      - LE_OK, if successfully,
 *      - otherwise failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyNetwork_ConnectNetworkCommunicationChannel
(
    void* handle ///< Opaque handle to a le_comm communication channel
)
{
    char* systemName;
    void* parentHandlePtr;
    le_result_t result = LE_OK;

    if (handle == NULL)
    {
        LE_ERROR("Connection Handle is NULL");
        return LE_FAULT;
    }

    parentHandlePtr = le_comm_GetParentHandle(handle);
    if (parentHandlePtr == NULL)
    {
        LE_ERROR("Parent Handle is NULL, handle [%d]", le_comm_GetId(handle));
        return LE_FAULT;
    }

    // Retrieve the system-name, using the Handle
    systemName = le_hashmap_Get(SystemNameByAsyncHandle, parentHandlePtr);
    if (systemName == NULL)
    {
        LE_ERROR("Unable to retrieve system-name, handle [%d] - unknown system", le_comm_GetId(handle));
        return LE_FAULT;
    }

    NetworkRecord_t* networkRecordPtr = le_hashmap_Get(NetworkRecordHashMapByName, systemName);
    if (networkRecordPtr == NULL)
    {
        LE_ERROR("Unable to connect network communciation channel, "
                 "system-name [%s] - unknown system", systemName);
        return LE_FAULT;
    }

    // Verify the Network State
    if (networkRecordPtr->state == NETWORK_UP)
    {
        // Reject any new client connection.  In the event the Network is down,
        // the Keep-Alive service will detect it (shortly), clean-up the Network record,
        // and allow new client connections.   Until then, reject any new client connections.
        LE_INFO("Rejecting Client socket connection, system-name [%s], "
                "handle [%d] - network is already connected",
                systemName,
                le_comm_GetId(handle));

        le_comm_Delete(handle);
        return LE_OK;
    }

    // Register Callback Handler to receive incoming RPC messages asynchronously.
    result = le_comm_RegisterHandleMonitor(
                handle,
                &rpcProxy_AsyncRecvHandler,
                POLLIN | POLLRDHUP | POLLERR);

    if (result != LE_OK)
    {
        LE_INFO("Unable to register callback handler for RPC responses, result %d", result);

        // Unable to estblish Network Connection
        // Start Network Retry Timer
        rpcProxyNetwork_StartNetworkConnectionRetryTimer(systemName);

        // Delete Communication channel
        le_comm_Delete(handle);
        return result;
    }

    LE_INFO("Network Status: UP, system-name [%s], handle [%d] - ready to receive events",
            systemName,
            le_comm_GetId(handle));

    // Mark Network Connection State as UP
    networkRecordPtr->state = NETWORK_UP;

    if (parentHandlePtr != handle)
    {
        // Delete the parent handle before taking the new connection handle
        le_comm_Delete(parentHandlePtr);
    }

    // Store the new connection handle
    networkRecordPtr->handle = handle;

    // Start Keep-Alive service to monitor the health of the network
    StartNetworkKeepAliveService(systemName, networkRecordPtr);

    // Start the Advertise-Service sequence for services being hosted by the RPC Proxy
    // NOTE:  The advertise-service will only be completed once we have successfully performed
    //        a connect-service on the far-side
    rpcProxy_AdvertiseServices(systemName);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Processing KEEPALIVE-Requests arriving from a far-side RPC Proxy
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyNetwork_ProcessKeepAliveRequest
(
    void* handle,                  ///< [IN] Opaque handle to the le_comm communication channel
    const char* systemName,        ///< [IN] Name of the system that sent the Client-Request
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Message State-Machine data
    void* proxyMessagePtr          ///< Pointer to the Proxy Message
)
{
    rpcProxy_KeepAliveMessage_t* keepAliveMsgPtr = (rpcProxy_KeepAliveMessage_t*) proxyMessagePtr;
    le_result_t result;

    // Sanity Check - Verify Message Type
    LE_ASSERT(keepAliveMsgPtr->commonHeader.type == RPC_PROXY_KEEPALIVE_REQUEST);

    le_result_t recvRes = rpcProxy_RecvStream(handle, streamStatePtr, keepAliveMsgPtr);
    if (recvRes == LE_IN_PROGRESS)
    {
        // return now, come back later
        return LE_OK;
    }
    else if (recvRes != LE_OK)
    {
        LE_ERROR("Error when receiving a keepalive request message from %s", systemName);
        return LE_FAULT;
    }

    //
    // Restart Keep-Alive Network timer for the specified system
    //

    // Retrieve the Network Record for this system, if one exists
    NetworkRecord_t* networkRecordPtr =
        le_hashmap_Get(NetworkRecordHashMapByName, systemName);

    if (networkRecordPtr == NULL)
    {
        LE_ERROR("Unable to retrieve network record, "
                 "system-name [%s] - unknown system",
                 systemName);

        return LE_FAULT;
    }


    // Verify Network Keep-Alive timer reference is not NULL
    if (networkRecordPtr->keepAliveTimerRef != NULL)
    {
        // Verify Keep-Alive service is running
        if (le_timer_IsRunning(networkRecordPtr->keepAliveTimerRef))
        {
            // Restart Keep-Alive timer
            le_timer_Restart(networkRecordPtr->keepAliveTimerRef);
        }
    }

    //
    // Prepare the KEEPALIVE-Response Proxy Message
    //

    // Set the Proxy Message type to SERVER_RESPONSE
    keepAliveMsgPtr->commonHeader.type = RPC_PROXY_KEEPALIVE_RESPONSE;

    LE_INFO("Sending Proxy KEEPALIVE-Response Message, id [%" PRIu32 "]",
             keepAliveMsgPtr->commonHeader.id);

    result = rpcProxy_SendMsg(systemName, keepAliveMsgPtr);
    if (result != LE_OK)
    {
        LE_ERROR("le_comm_Send failed, result %d", result);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Processing KEEPALIVE-Responses arriving from a far-side RPC Proxy
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyNetwork_ProcessKeepAliveResponse
(
    void* handle,                  ///< [IN] Opaque handle to the le_comm communication channel
    const char* systemName,        ///< [IN] Name of the system that sent the Client-Request
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Message State-Machine data
    void* proxyMessagePtr          ///< Pointer to the Proxy Message
)
{
    rpcProxy_KeepAliveMessage_t* keepAliveMsgPtr = (rpcProxy_KeepAliveMessage_t*) proxyMessagePtr;
    // Sanity Check - Verify Message Type
    LE_ASSERT(keepAliveMsgPtr->commonHeader.type == RPC_PROXY_KEEPALIVE_RESPONSE);

    le_result_t recvRes = rpcProxy_RecvStream(handle, streamStatePtr, keepAliveMsgPtr);
    if (recvRes == LE_IN_PROGRESS)
    {
        // return now, come back later
        return LE_OK;
    }
    else if (recvRes != LE_OK)
    {
        LE_ERROR("Error when receiving a keep alive response from %s", systemName);
        return LE_FAULT;
    }

    // Sanity Check - Verify that the responding system matches the destination system
    if (strcmp(systemName, keepAliveMsgPtr->systemName) != 0)
    {
        LE_ERROR("Sanity Check Failure: System-name mismatch, systemName [%s], "
                 "systemName [%s], proxy id [%" PRIu32 "]",
                 systemName,
                 keepAliveMsgPtr->systemName,
                 keepAliveMsgPtr->commonHeader.id);

        return LE_FAULT;
    }

    // Retrieve the Network Record for this system
    NetworkRecord_t* networkRecordPtr =
        le_hashmap_Get(NetworkRecordHashMapByName, keepAliveMsgPtr->systemName);

    if (networkRecordPtr == NULL)
    {
        LE_ERROR("Unable to retrieve network record, "
                 "system-name [%s] - unknown system", keepAliveMsgPtr->systemName);
        return LE_FAULT;
    }

    // Sanity Check - Verify the State of the Network
    if (networkRecordPtr->state == NETWORK_DOWN)
    {
        // Inconsistent state
        LE_ERROR("Sanity Check: Unexpected Network state, system [%s]",
                 keepAliveMsgPtr->systemName);
    }

    // Retrieve and delete the timer associated with Proxy Message ID
    le_timer_Ref_t timerRef =
        le_hashmap_Get(rpcProxy_GetExpiryTimerRefByProxyId(),
                       (void*)(uintptr_t) keepAliveMsgPtr->commonHeader.id);

    if (timerRef != NULL)
    {
        LE_DEBUG("Deleting timer for KEEPALIVE-Request, '%s', id [%" PRIu32 "]",
                 keepAliveMsgPtr->systemName,
                 keepAliveMsgPtr->commonHeader.id);

        // Remove timer entry associated with Proxy Message ID from hash-map
        le_hashmap_Remove(rpcProxy_GetExpiryTimerRefByProxyId(),
                          (void*)(uintptr_t) keepAliveMsgPtr->commonHeader.id);

        // Delete Timer
        le_timer_Delete(timerRef);
        timerRef = NULL;
    }
    else
    {
        LE_ERROR("Unable to find matching Timer record, system-name [%s], proxy id [%" PRIu32 "]",
                 keepAliveMsgPtr->systemName,
                 keepAliveMsgPtr->commonHeader.id);
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Timer expiry handler for keep alive messages
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyNetwork_KeepAliveTimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    const char* systemName = (const char*) le_timer_GetContextPtr(timerRef);

    if (systemName != NULL)
    {
        LE_INFO("KEEPALIVE-Request timer expired; "
                "Declare the network down, system [%s]", systemName);

        // Delete the Network Communication Channel
        rpcProxyNetwork_DeleteNetworkCommunicationChannel(systemName);
    }
    else
    {
        LE_ERROR("Unable to retrieve system name of the Proxy Keep-Alive Message Reference");
    }

    // Timer Reference deleted as a part of the
    // Network Communication Channel deletion.
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Generating an KEEPALIVE-Request
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyNetwork_SendKeepAliveRequest
(
    const char* systemName ///< System name
)
{
    rpcProxy_KeepAliveMessage_t  keepAliveProxyMessage = {0};
    le_result_t                  result;


    // Allocate memory for a Proxy Message copy
     rpcProxy_KeepAliveMessage_t * proxyMessagePtr = &keepAliveProxyMessage;

    // Create an Keep-Alive Request Message
    proxyMessagePtr->commonHeader.id = rpcProxy_GenerateProxyMessageId();
    proxyMessagePtr->commonHeader.type = RPC_PROXY_KEEPALIVE_REQUEST;
    proxyMessagePtr->commonHeader.serviceId = 0;

    // Set the System-Name
    strncpy(proxyMessagePtr->systemName, systemName, sizeof(proxyMessagePtr->systemName) - 1);

    LE_INFO("Sending Proxy KEEPALIVE-Request Message, id [%" PRIu32 "]",
             proxyMessagePtr->commonHeader.id);

    // Send Proxy Message to far-side
    result = rpcProxy_SendMsg(systemName, proxyMessagePtr);
    if (result != LE_OK)
    {
        LE_ERROR("le_comm_Send failed, result %d", result);
    }

    //
    // KEEPALIVE-Request requires a response - Set-up a timer in the event
    // we do not hear back from the far-side RPC Proxy
    //
    le_timer_Ref_t  keepAliveRequestTimerRef;
    le_clk_Time_t  timerInterval = {.sec=RPC_PROXY_NETWORK_KEEPALIVE_TIMEOUT_TIMER_INTERVAL,
                                    .usec=0};

    // Create a timer to handle "lost" requests
    keepAliveRequestTimerRef = le_timer_Create("KEEPALIVE-Request timer");
    le_timer_SetInterval(keepAliveRequestTimerRef, timerInterval);
    le_timer_SetHandler(keepAliveRequestTimerRef, rpcProxyNetwork_KeepAliveTimerExpiryHandler);
    le_timer_SetWakeup(keepAliveRequestTimerRef, false);

    // Set system name in the timer event
    le_timer_SetContextPtr(keepAliveRequestTimerRef, (void*)systemName);

    // Start timer
    le_timer_Start(keepAliveRequestTimerRef);

    // Store the timerRef in a hashmap, using the Proxy Message ID as a key,
    // in order to retrieve it in the event we receive a response
    le_hashmap_Put(rpcProxy_GetExpiryTimerRefByProxyId(),
                   (void*)(uintptr_t) proxyMessagePtr->commonHeader.id,
                   keepAliveRequestTimerRef);

    LE_INFO("Starting timer (%d secs.) for KEEPALIVE-Request, '%s', id [%" PRIu32 "]",
            RPC_PROXY_NETWORK_KEEPALIVE_TIMEOUT_TIMER_INTERVAL,
            systemName,
            proxyMessagePtr->commonHeader.id);
}

//--------------------------------------------------------------------------------------------------
/**
 * Callback Handler Function for asynchronously connecting RPC Proxy Communication Channels
 */
//--------------------------------------------------------------------------------------------------
static void AsyncConnectionCallbackHandler
(
    void* handle, ///< Opaque handle to a le_comm communication channel
    short events ///< Event bit-mask
)
{
    le_result_t result = LE_OK;

    LE_INFO("Asynchronous Connection Callback function triggered, "
            "handle [%d], events [%d]",
            le_comm_GetId(handle),
            events);

    // Check if this is an error condition
    if (events & POLLERR)
    {
        // Time-out error waiting for network connection
        // Retrieve the system-name, using the Handle
        char* systemName =
            le_hashmap_Get(SystemNameByAsyncHandle, handle);

        if (systemName == NULL)
        {
            LE_ERROR("Unable to retrieve system-name, "
                     "handle [%d] - unknown system", le_comm_GetId(handle));
            return;
        }

        // Retrieve the network-record
        NetworkRecord_t* networkRecordPtr =
            le_hashmap_Get(NetworkRecordHashMapByName, systemName);

        if (networkRecordPtr == NULL)
        {
            LE_ERROR("Unable to connect network communication channel, "
                     "system-name [%s] - unknown system", systemName);
            return;
        }

        // Assert that this is the same handle that
        // was created through le_comm_Create
        LE_ASSERT(handle == networkRecordPtr->handle);

        // Delete Communication channel
        le_comm_Delete(networkRecordPtr->handle);
        networkRecordPtr->handle = NULL;

        // Start Network Retry Timer
        rpcProxyNetwork_StartNetworkConnectionRetryTimer(systemName);
        return;
    }

    // Process network connection event
    result = rpcProxyNetwork_ConnectNetworkCommunicationChannel(handle);
    if (result != LE_OK)
    {
        LE_INFO("Error connecting network communication channel, result [%d]", result);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function initializes the RPC Proxy Network Services.
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxyNetwork_InitializeOnce
(
    void
)
{
    // Initialize memory pool for allocating Network Timer Records.
    NetworkTimerRecordPoolRef = le_mem_InitStaticPool(NetworkTimerRecordPool,
                                                      RPC_PROXY_NETWORK_TIMER_RECORD_MAX_NUM,
                                                      sizeof(NetworkTimerRecord_t));

    // Initialize memory pool for allocating Network Records.
    NetworkRecordPoolRef = le_mem_InitStaticPool(NetworkRecordPool,
                                                 RPC_PROXY_NETWORK_SYSTEM_MAX_NUM,
                                                 sizeof(NetworkRecord_t));

    // Create hash map for storing Network Records (value), using System-name as key.
    NetworkRecordHashMapByName = le_hashmap_InitStatic(NetworkRecordHashMap,
                                                       RPC_PROXY_NETWORK_SYSTEM_MAX_NUM,
                                                       le_hashmap_HashString,
                                                       le_hashmap_EqualsString);

    // Create hash map for storing System-name (value), using Asynchronous Communication handle (key).
    SystemNameByAsyncHandle = le_hashmap_InitStatic(
                                 SystemNameByAsyncHandleHashMap,
                                 RPC_PROXY_NETWORK_SYSTEM_MAX_NUM,
                                 le_hashmap_HashVoidPointer,
                                 le_hashmap_EqualsVoidPointer);

    return LE_OK;
}
