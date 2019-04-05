/**
 * @file localLoopback.c
 *
 * This file provides a "local loopback" implementation of the RPC Communication API (le_comm.h).
 * It allows for testing the RPC Proxy as a single daemon acting as both
 * Proxy Client and Server in isolation.
 *
 * NOTE:  Temporary interim solution for testing the RPC Proxy communication framework
 *        while under development.
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#include "legato.h"
#include "interfaces.h"
#include "le_comm.h"
#include "le_rpcProxy.h"

static le_comm_CallbackHandlerFunc_t local_callback_handler;
static char local_buffer[200];
static size_t local_msgsize;

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the RPC Communication implementation.
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("RPC Local Loopback Init done");
}

LE_SHARED void* le_comm_Create
(
    const int argc,         ///< [IN] Number of strings pointed to by argv.
    const char *argv[],     ///< [IN] Pointer to an array of character strings.
    le_result_t* resultPtr  ///< [OUT] Return Code
)
{
    int fd = 1;
    if (resultPtr == NULL)
    {
        LE_ERROR("result pointer is NULL");
        return (void*) -1;
    }

    *resultPtr = LE_OK;
    return (void*) fd;
}

LE_SHARED le_result_t le_comm_RegisterHandleMonitor (void* handle, le_comm_CallbackHandlerFunc_t handlerFunc, short events)
{
    LE_INFO("Registering handle_monitor callback");

    local_callback_handler = handlerFunc;

    LE_INFO("Successfully registered handle_monitor callback");

    return LE_OK;
}

LE_SHARED le_result_t le_comm_Delete (void* handle)
{
    return LE_OK;
}

LE_SHARED le_result_t le_comm_Connect (void* handle)
{
    int fd = (int) handle;

    LE_INFO("Successfully connected, fd %d, ", fd);

    return LE_OK;
}

LE_SHARED le_result_t le_comm_Disconnect (void* handle)
{
    return LE_OK;
}

LE_SHARED le_result_t le_comm_Send (void* handle, const void* buf, size_t len)
{
    // Ensure local loopback buffer is big enough
    if (sizeof(local_buffer) < len)
    {
        LE_INFO("Send Buffer too small");
        return LE_OK;
    }

    // Copy Proxy Message onto local loopback buffer and set the size
    memcpy(&local_buffer, buf, len);
    local_msgsize = len;

    LE_INFO("Calling local_callback_handler() function");

    // Call RPC Proxy receive handler
    local_callback_handler(handle, 0x00);
    LE_INFO("Finished local_callback_handler() function");

    return LE_OK;
}

LE_SHARED le_result_t le_comm_Receive (void* handle, void* buf, size_t* len)
{
    LE_INFO("Calling myLocal le_comm_Receive");

    if (*len < local_msgsize)
    {
        LE_INFO("Receive Buffer too small");
        return LE_OK;
    }

    memcpy(buf, &local_buffer, local_msgsize);
    *len = local_msgsize;

    return LE_OK;
}

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
LE_SHARED int le_comm_GetId
(
    void* handle
)
{
    return 1;
}

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
LE_SHARED void* le_comm_GetParentHandle
(
    void* handle
)
{
    return NULL;
}
