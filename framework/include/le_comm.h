/**
 * @file le_comm.h
 *
 * This file contains the prototypes of Legato RPC Communication API,
 * used to provide network communication between two or more remote-host
 * systems.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#ifndef LE_COMM_H_INCLUDE_GUARD
#define LE_COMM_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Legato RPC Communication Types
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_comm_CallbackHandlerFunc_t) (void* handle, short events);


//--------------------------------------------------------------------------------------------------
/**
 * Function for Creating a RPC Communication Channel
 *
 * Return Code
 *      - LE_OK if successfully,
 *      - LE_WAITING if pending on asynchronous connection,
 *      - otherwise failure
 *
 * @return
 *      Opaque handle to the Communication Channel.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((weak))
LE_SHARED void* le_comm_Create
(
    const int argc,         ///< [IN] Number of strings pointed to by argv.
    const char *argv[],     ///< [IN] Pointer to an array of character strings.
    le_result_t* resultPtr  ///< [OUT] Return Code
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for Registering a Callback Handler function to monitor events on the specific handle
 *
 * @return
 *      - LE_OK if successfully.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((weak))
LE_SHARED le_result_t le_comm_RegisterHandleMonitor
(
    void* handle,                               ///< [IN] Communication channel handle.
    le_comm_CallbackHandlerFunc_t handlerFunc,  ///< [IN] Callback Handler Function.
    short events                                ///< [IN] Events to be monitored.
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for Deleting a RPC Communicaiton Channel
 *
 * @return
 *      - LE_OK if successfully.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((weak))
LE_SHARED le_result_t le_comm_Delete
(
    void* handle        ///< [IN] Communication channel.
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for Connecting a RPC Communication Channel
 *
 * @return
 *      - LE_OK if successfully.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((weak))
LE_SHARED le_result_t le_comm_Connect
(
    void* handle        ///< [IN] Communication channel.
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for Disconnecting a RPC Communication Channel
 *
 * @return
 *      - LE_OK if successfully.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((weak))
LE_SHARED le_result_t le_comm_Disconnect
(
    void* handle        ///< [IN] Communication channel.
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for Sending Data over a RPC Communication Channel
 *
 * @return
 *      - LE_OK if successfully.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((weak))
LE_SHARED le_result_t le_comm_Send
(
    void* handle,       ///< [IN] Communication channel.
    const void* buf,    ///< [IN] Pointer to buffer of data to be sent.
    size_t len          ///< [IN] Size of data to be sent.
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for Receiving Data over a RPC Communication Channel
 *
 * @return
 *      - LE_OK if successfully.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((weak))
LE_SHARED le_result_t le_comm_Receive
(
    void* handle,       ///< [IN] Communication channel.
    void* buf,          ///< [IN] Pointer to buffer to hold received data.
    size_t* len         ///< [IN] Size of data received.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get Support Functions
 */
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * Function to retrieve an ID for the specified handle.
 * NOTE:  For logging or display purposes only.
 *
 * @return
 *      - Non-zero integer, if successful.
 *      - Negative one (-1), otherwise.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((weak))
LE_SHARED int le_comm_GetId
(
    void* handle
);

//--------------------------------------------------------------------------------------------------
/**
 * Function to retrieve the Parent Handle.
 * NOTE:  For asynchronous connections only.
 *
 * @return
 *      - Parent (Listening) handle, if successfully.
 *      - NULL, otherwise.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((weak))
LE_SHARED void* le_comm_GetParentHandle
(
    void* handle
);

#endif /* LE_COMM_H_INCLUDE_GUARD */