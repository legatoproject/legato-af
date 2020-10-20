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

#define MQTT_TIMEOUT_MS    5000

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
    struct Network* net,        /// [IN] Network structure
    bool secure,                /// [IN] Secure connection flag
    const uint8_t* certPtr,     /// [IN] Certificate pointer
    size_t certLen              /// [IN] Length in byte of certificate certPtr
)
{
    net->socketRef = NULL;
    net->secure = secure;
    net->certificatePtr = certPtr;
    net->certificateLen = certLen;
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
    struct Network* net,        /// [IN] Network structure
    char*           addr,       /// [IN] Remote server address
    int             port        /// [IN] Remote server port
)
{
    char srcIpAddress[LE_MDC_IPV6_ADDR_MAX_BYTES] = {0};

    if (net->socketRef != NULL || !addr || port > 65535 || port < 0)
    {
        LE_ERROR("Bad parameter");
        return LE_BAD_PARAMETER;
    }

    // use default profile
    le_mdc_ProfileRef_t profileRef = le_mdc_GetProfile((uint32_t)LE_MDC_DEFAULT_PROFILE);
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
        return LE_FAULT;
    }

    if (net->secure)
    {
        LE_INFO("Adding security certificate...");
        if (LE_OK != le_socket_AddCertificate(net->socketRef, net->certificatePtr, net->certificateLen))
        {
            LE_ERROR("Failed to add certificate");
            goto freeSocket;
        }
    }

    // Set response timeout.
    if (LE_OK != le_socket_SetTimeout(net->socketRef, MQTT_TIMEOUT_MS))
    {
       LE_ERROR("Failed to set response timeout.");
       goto freeSocket;
    }

    // Connect the MQTT broker.
    if (LE_OK != le_socket_Connect(net->socketRef))
    {
        LE_ERROR("Failed to connect MQTT broker %s:%d.", addr, port);
        goto freeSocket;
    }

    return LE_OK;

freeSocket:
    if(net->socketRef != NULL)
    {
        le_socket_Delete(net->socketRef);
        net->socketRef = NULL;
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
        le_socket_Disconnect(net->socketRef);
        le_socket_Delete(net->socketRef);
        net->socketRef = NULL;
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
       return -1;
    }

    rc = le_socket_Read(net->socketRef, (char*)buffer, &bufLen);
    if (rc == LE_OK)
    {
        LE_DEBUG("Read %d bytes from network", bufLen);
        return bufLen;
    }
    else if (rc == LE_TIMEOUT)
    {
        return 0;
    }
    else
    {
        LE_ERROR("Reading data error on socket ref: %p", net->socketRef);
        return -1;
    }
}
