 /**
  * Header of implementation of the MQTT network operations and adaptors for paho lib.
  *
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#ifndef __MQTT_ADAPTOR_HEADER_GUARD__
#define __MQTT_ADAPTOR_HEADER_GUARD__

#include "legato.h"
#include "le_socketLib.h"

#include "mqttTimer.h"

struct Network;

// Declare few types that paho.mqtt.embedded-c lib needs
typedef le_mutex_Ref_t Mutex;
typedef le_thread_Ref_t Thread;
typedef void (*MqttThreadFunc)(void*);
typedef int (*MqttReadFunc) (struct Network*, unsigned char*, int, int);
typedef int (*MqttWriteFunc) (struct Network*, unsigned char*, int, int);

// Asynchronous callback function for Network Status
typedef void (*networkStatusHandler)(short events, void* contextPtr);


// Network structure
typedef struct Network
{
    le_socket_Ref_t socketRef;          ///< socket reference
    bool secure;                        ///< secure connection flag
    const uint8_t* certificatePtr;      ///< pointer of certificate
    size_t certificateLen;              ///< length of certificate
    MqttReadFunc mqttread;              ///< read function pointer
    MqttWriteFunc mqttwrite;            ///< write function pointer
    networkStatusHandler handlerFunc;   ///< Network status callback function
    void* contextPtr;                   ///< Network status callback function context pointer
} Network;


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
);

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
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * Start a MQTT task thread
 *
 * @return none.
 */
//--------------------------------------------------------------------------------------------------
int ThreadStart
(
    Thread* thread,             /// [OUT] Mqtt thread reference
    MqttThreadFunc func,        /// [IN] Mqtt thread function pointer
    void* arg                   /// [IN] Argument for thread function
);

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
);

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
);

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
);

#endif // __MQTT_ADAPTOR_HEADER_GUARD__
