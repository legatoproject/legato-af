 /**
  * This module implements the MQTT network operations.
  *
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include "interfaces.h"
#include "le_socketLib.h"

#include "mqttAdaptor.h"


// Structure for MQTT task
typedef struct
{
    MqttThreadFunc  task;           ///< Mqtt thread function
    void*           param;          ///< Parameter for the function
} MqttTask;

// Forward declaration
static int MqttRead(Network*, unsigned char*, int, int);
static int MqttWrite(Network*, unsigned char*, int, int);


//--------------------------------------------------------------------------------------------------
/**
 *  Asynchronous callback function for handling socket status events
 *  for all MQTT client sessions.
 *
 *  @return void
 */
//--------------------------------------------------------------------------------------------------
static void StatusRecvHandler
(
    le_socket_Ref_t  ref,       ///< [IN] Socket context reference
    short            events,    ///< [IN] Bitmap of events that occurred
    void*            userPtr    ///< [IN] User-defined pointer
)
{
    LE_UNUSED(ref);

    if (events & POLLOUT)
    {
        return;
    }

    LE_INFO("[%s] - events [0x%x]", __FUNCTION__, events);
    struct Network* networkPtr = (struct Network*) userPtr;

    if (networkPtr)
    {
        if (networkPtr->handlerFunc)
        {
            LE_INFO("[%s] - Calling handler function, events [0x%x]", __FUNCTION__, events);

            // Call the registered MQTT client session receive
            // handler function for this network
            networkPtr->handlerFunc(events, networkPtr->contextPtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize a MQTT mutex
 *
 * @return none.
 */
//--------------------------------------------------------------------------------------------------
void MutexInit
(
    Mutex* mtx                  /// [OUT] Mutex for MQTT client operations
)
{
    if (mtx)
    {
        *mtx = le_mutex_CreateNonRecursive("mqtt_mutex");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Lock a MQTT mutex
 *
 * @return none.
 */
//--------------------------------------------------------------------------------------------------
void MutexLock
(
    Mutex* mtx                  /// [IN] Mutex for MQTT client operations
)
{
    if (mtx)
    {
        le_mutex_Lock(*mtx);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Unlock a MQTT mutex
 *
 * @return none.
 */
//--------------------------------------------------------------------------------------------------
void MutexUnlock
(
    Mutex* mtx                  /// [IN] Mutex for MQTT client operations
)
{
    if (mtx)
    {
        le_mutex_Unlock(*mtx);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Perform the actual MQTT task
 *
 * @return none.
 */
//--------------------------------------------------------------------------------------------------
static void* MqttThreadWrapper(void* context)
{
    MqttTask* mqttTask = (MqttTask*)context;
    (*mqttTask->task)(mqttTask->param);

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a MQTT task thread
 *
 * @return 0 if successful, -1 otherwise.
 */
//--------------------------------------------------------------------------------------------------
int ThreadStart
(
    Thread* thread,             /// [OUT] Mqtt thread reference
    MqttThreadFunc func,        /// [IN] Mqtt thread function pointer
    void* arg                   /// [IN] Argument for thread function
)
{
    MqttTask task;
    task.task = func;
    task.param = arg;

    le_thread_Ref_t threadRef = le_thread_Create("MQTTTask", MqttThreadWrapper, &task);
    le_thread_Start(threadRef);

    if (!thread)
    {
        return -1;
    }

    *thread = threadRef;

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize a network structure.
 *
 * @return none.
 */
//--------------------------------------------------------------------------------------------------
void NetworkInit
(
    struct Network* net                /// [IN] Network structure
#ifndef MK_CONFIG_NO_SSL
    ,
    int secure,                        /// [IN] Secure connection flag
    uint8_t auth,                      /// [IN] Authentication mode
    uint32_t cipherIndex,              /// [IN] Cipher Suite profile index
    const uint8_t* certPtr,            /// [IN] Certificate pointer
    size_t certLen,                    /// [IN] Length in byte of certificate certPtr
    const uint8_t* ownCertPtr,         /// [IN] Own certificate pointer
    size_t ownCertLen,                 /// [IN] Length in byte of own certificate certPtr
    const uint8_t* ownPrivateKeyPtr,   /// [IN] Own private key pointer
    size_t ownPrivateKeyLen,           /// [IN] Length in byte of own private key
    const char *alpnList,              /// [IN] ALPN Protocol name
    uint8_t tlsVersion                 /// [IN] Supported TLS version (Minor version number)
#endif
)
{
    net->socketRef = NULL;
    net->handlerFunc = NULL;
    net->contextPtr = NULL;
#ifndef MK_CONFIG_NO_SSL
    net->secure = secure;
    net->auth = auth;
    net->cipherIdx = cipherIndex;
    net->certificatePtr = certPtr;
    net->certificateLen = certLen;
    net->ownCertificatePtr = ownCertPtr;
    net->ownCertificateLen = ownCertLen;
    net->ownPrivateKeyPtr = ownPrivateKeyPtr;
    net->ownPrivateKeyLen = ownPrivateKeyLen;
    net->alpnList[0] = alpnList;       // Only a single ALPN name supported currently
    net->alpnList[1] = NULL;           // Needed to terminate the ALPN with NULL
    net->tlsVersion = tlsVersion;
#endif
    net->mqttread = MqttRead;
    net->mqttwrite = MqttWrite;
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect to remote server (MQTT broker).
 *
 * @return
 *  - LE_OK            Successfully connected
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_COMM_ERROR    Connection failure
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t NetworkConnect
(
    struct Network*       net,         /// [IN] Network structure
    uint32_t              profileNum,  /// [IN] PDP profile number
    char*                 addr,        /// [IN] Remote server address
    int                   port,        /// [IN] Remote server port
    int                   timeoutMs,   /// [IN] Connection timeout in milliseconds
    networkStatusHandler  handlerFunc, /// [IN] Network status callback function
    void*                 contextPtr   /// [IN] Network status callback function context pointer
)
{
    char srcIpAddress[LE_MDC_IPV6_ADDR_MAX_BYTES] = {0};

    if (net->socketRef != NULL || !addr || port > 65535 || port < 0)
    {
        LE_ERROR("Bad parameter");
        return LE_BAD_PARAMETER;
    }

    // Get profile reference
    le_mdc_ProfileRef_t profileRef = le_mdc_GetProfile(profileNum);
    if(!profileRef)
    {
        LE_ERROR("le_mdc_GetProfile cannot get default profile");
        return LE_FAULT;
    }
    // Try IPv4, then Ipv6
    if (LE_OK == le_mdc_GetIPv4Address(profileRef, srcIpAddress, sizeof(srcIpAddress)))
    {
        LE_INFO("Using IPv4 profile & source addr %s", srcIpAddress);
    }
    else if (LE_OK == le_mdc_GetIPv6Address(profileRef, srcIpAddress, sizeof(srcIpAddress)))
    {
        LE_INFO("Using IPv6 profile & source addr %s", srcIpAddress);
    }
    else
    {
        LE_ERROR("No IPv4 or IPv6 profile");
        return LE_FAULT;
    }

    // Create the MQTT client socket.
    net->socketRef = le_socket_Create(addr, port, srcIpAddress, TCP_TYPE);

    if (!net->socketRef)
    {
        LE_ERROR("Failed to create MQTT client socket for server %s:%d.", addr, port);
        net->extError = EXT_SOC_SET_CLI_SOCKET_ERR;

        return LE_FAULT;
    }

#ifndef MK_CONFIG_NO_SSL
    if (net->secure)
    {
        LE_DEBUG("Setting TLS version...");
        if (LE_OK != le_socket_SetTlsVersion(net->socketRef, net->tlsVersion))
        {
            LE_ERROR("Failed to set TLS version");
            net->extError = EXT_TLS_SET_VERSION_ERR;
            goto freeSocket;
        }

        LE_DEBUG("Adding security certificate...");
        if (LE_OK != le_socket_AddCertificate(net->socketRef,
                                              net->certificatePtr,
                                              net->certificateLen))
        {
            LE_ERROR("Failed to add root CA certificate into tls");
            goto freeSocket;
        }

        LE_DEBUG("Auth type: %d", net->auth);

        if (net->auth == AUTH_MUTUAL) // Mutual Authentication
        {
            if (LE_OK != le_socket_AddOwnCertificate(net->socketRef,
                                                     net->ownCertificatePtr,
                                                     net->ownCertificateLen))
            {
                LE_ERROR("Failed to add client certificate into tls");
                goto freeSocket;
            }

            if (LE_OK != le_socket_AddOwnPrivateKey(net->socketRef,
                                                    net->ownPrivateKeyPtr,
                                                    net->ownPrivateKeyLen))
            {
                LE_ERROR("Failed to add client private key into tls");
                goto freeSocket;
            }
        } // End of mutual authentication

        LE_DEBUG("Setting Socket Authentication type...");
        if (LE_OK != le_socket_SetAuthType(net->socketRef, net->auth))
        {
            LE_ERROR("Failed to set mutual authentication type");
            net->extError = EXT_TLS_SET_AUTH_ERR;
            goto freeSocket;
        }

        LE_DEBUG("Setting Cipher Suites...");
        if (LE_OK != le_socket_SetCipherSuites(net->socketRef, net->cipherIdx))
        {
            LE_ERROR("Failed to set cipher suite");
            net->extError = EXT_TLS_SET_CIPHER_SUITE_ERR;
            goto freeSocket;
        }

        if (net->alpnList)
        {
            if (LE_OK != le_socket_SetAlpnProtocolList(net->socketRef, net->alpnList))
            {
                LE_ERROR("Failed to set ALPN protocol list");
                net->extError = EXT_TLS_SET_ALPN_ERR;
                goto freeSocket;
            }
        }
    } // End of secure
#endif //MK_CONFIG_NO_SSL

    // Set response timeout.
    if (LE_OK != le_socket_SetTimeout(net->socketRef, timeoutMs))
    {
        LE_ERROR("Failed to set response timeout.");
        net->extError = EXT_SOC_SET_CNX_TIMEOUT_ERR;

        goto freeSocket;
    }

    // Store the receive handler callback function and context pointer
    net->handlerFunc = handlerFunc;
    net->contextPtr = contextPtr;

    LE_INFO("[%s] Registering callback function", __FUNCTION__);
    if (LE_OK != le_socket_AddEventHandler(net->socketRef, StatusRecvHandler, net))
    {
        LE_ERROR("Failed to add socket event handler");
        net->extError = EXT_SOC_SET_EVENT_HANDLER_ERR;

        goto freeSocket;
    }

    // Enable async mode and start fd monitor.
    if (LE_OK != le_socket_SetMonitoring(net->socketRef, true))
    {
        LE_ERROR("Failed to enable data socket monitor.");
        net->extError = EXT_SOC_SET_ASYNC_ERR;

        goto freeSocket;
    }

    // Connect the MQTT broker.
    if (LE_OK != le_socket_Connect(net->socketRef))
    {
        LE_ERROR("Failed to connect MQTT broker %s:%d.", addr, port);
        net->extError = EXT_SOC_CNX_REMOTE_SERVER_ERR;
        goto freeSocket;
    }

    return LE_OK;

freeSocket:
    if(net->socketRef != NULL)
    {
#ifndef MK_CONFIG_NO_SSL
        /* NetworkConnect failed.
           Keep client tls err before delete client. */
        int tls_err = le_socket_GetTlsErrorCode(net->socketRef);
        if(tls_err != 0)
        {
            net->extError = (le_exterr_result_t)tls_err;
        }
#endif
        le_socket_Delete(net->socketRef);
        net->socketRef = NULL;
        net->handlerFunc = NULL;
        net->contextPtr = NULL;
    }
    return LE_COMM_ERROR;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disconnect to remote server (MQTT broker).
 *
 * @return none.
 */
//--------------------------------------------------------------------------------------------------
void NetworkDisconnect
(
    struct Network* net         /// [IN] Network structure
)
{
    if (net->socketRef != NULL)
    {
#ifndef MK_CONFIG_NO_SSL
        /* Keep client tls err before delete client. */
        int tls_err = le_socket_GetTlsErrorCode(net->socketRef);
        if(tls_err != 0)
        {
            net->extError = (le_exterr_result_t)tls_err;
        }
#endif
        le_socket_Disconnect(net->socketRef);
        le_socket_Delete(net->socketRef);
        net->socketRef = NULL;
        net->handlerFunc = NULL;
        net->contextPtr = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Write to remote server (MQTT broker).
 *
 * @return number of bytes sent if successful, -1 otherwise.
 */
//--------------------------------------------------------------------------------------------------
static int MqttWrite
(
    struct Network* net,        /// [IN] Network structure
    unsigned char* buffer,      /// [IN] Pointer of data need to be sent
    int len,                    /// [IN] Number of bytes in buffer need to be sent
    int timeoutMs               /// [IN] Timeout value in milliseconds
)
{
    le_result_t rc;

    // Set response timeout.
    if (LE_OK != le_socket_SetTimeout(net->socketRef, timeoutMs))
    {
       LE_ERROR("Failed to set response timeout.");
       net->extError = EXT_SOC_SET_WRITE_TIMEOUT_ERR;
       return -1;
    }

    rc = le_socket_Send(net->socketRef, (char*)buffer, (size_t)len);
    if (rc == LE_OK || rc == LE_TIMEOUT)
    {
        LE_DEBUG("Sent %d bytes out, rc=%d\n", len, rc);
        return len;
    }
    else
    {
        LE_ERROR("Sending data error on socket ref: %p", net->socketRef);
        return -1;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Read from remote server (MQTT broker).
 *
 * @return number of bytes read.
 */
//--------------------------------------------------------------------------------------------------
static int MqttRead
(
    struct Network* net,        /// [IN] Network structure
    unsigned char* buffer,      /// [IN] Pointer of buffer to receive data
    int len,                    /// [IN] Number of bytes need to read
    int timeoutMs               /// [IN] Timeout value in milliseconds
)
{
    le_result_t rc;
    size_t bufLen = len;

    // Set response timeout.
    if (LE_OK != le_socket_SetTimeout(net->socketRef, timeoutMs))
    {
       LE_ERROR("Failed to set response timeout.");
       net->extError = EXT_SOC_SET_READ_TIMEOUT_ERR;
       return -1;
    }

    rc = le_socket_Read(net->socketRef, (char*)buffer, &bufLen);
    if (rc == LE_OK)
    {
        LE_DEBUG("Read %d bytes from network", bufLen);
        return bufLen;
    }
    else if ((rc == LE_TIMEOUT) || (rc == LE_IN_PROGRESS))
    {
        // For TLS 1.3 le_socket_Read() may return LE_IN_PROGRESS.
        // This means post TLS handshake activities are going on.
        // This shouldn't be considered as error.
        return 0;
    }
    else
    {
        LE_ERROR("Reading data error on socket ref: %p", net->socketRef);
        return -1;
    }
}
