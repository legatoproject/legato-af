/**
 * @file le_rpcProxyFileStreamFifo.c
 *
 * This file contains the source code for generating a fifo for each file stream
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "le_rpcProxy.h"
#include "le_rpcProxyFileStream.h"

//--------------------------------------------------------------------------------------------------
/**
 * Macros used for creating a unique name for each fifo
 */
//--------------------------------------------------------------------------------------------------
#define RPC_FSTREAM_FIFO_PATH        "/tmp/rpc%s%"PRIu16""
#define RPC_FSTREAM_FIFO_PATH_MAX_LEN (sizeof(RPC_FSTREAM_FIFO_PATH) + RPC_FSTREAM_ID_MAX_DIGITS + \
                                       LIMIT_MAX_SYSTEM_NAME_LEN)
//--------------------------------------------------------------------------------------------------
/**
 * Create a local channel with two file descriptors, one for rpcFd one for localFd
 *
 * @return
 *      - LE_OK if channel was created successfully.
 *      - LE_FAULT error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcFStream_CreateChannel
(
    rpcProxy_FileStream_t* fileStreamRef,  ///< [IN] Pointer to file stream instance
    int32_t* rpcFdPtr,                     ///< [OUT] rpcFd
    int32_t* localFdPtr,                   ///< [OUT] localFd
    bool isLocalFdNonBlocking              ///< [IN] boolean determining whether localfd should be
                                           /// non blocking
)
{
    le_result_t res = LE_OK;
    int32_t localFd = -1;
    int32_t rpcFd = -1;
    const char* systemName = fileStreamRef->remoteSystemName;
    uint16_t fStreamId = fileStreamRef->streamId;
    char fifoPath[RPC_FSTREAM_FIFO_PATH_MAX_LEN] = {0};
    snprintf(fifoPath, sizeof(fifoPath), RPC_FSTREAM_FIFO_PATH, systemName, fStreamId);
    LE_INFO("Creating fifo %s for rpc proxy file stream id: [%" PRId16 "]", fifoPath, fStreamId);
    if ( (-1 == le_fd_MkFifo(fifoPath, S_IRWXU | S_IRWXG | S_IRWXO)) && (EEXIST != errno) )
    {
        LE_ERROR("Failed to create fifo for stream id:[%" PRIu16 "] of system: [%s], "
                 "errno:[%" PRId32 "]", fStreamId, systemName, (int32_t)errno);
        res = LE_FAULT;
        goto end;
    }
    /* Our fd must be non blocking so rpcDaemon is never blocked */
    int rpcFdOpenFlags = O_NONBLOCK;
    int localFdOpenFlags = (isLocalFdNonBlocking)? O_NONBLOCK : 0;
    if (fileStreamRef->direction == INCOMING_FILESTREAM)
    {
        rpcFdOpenFlags |= O_WRONLY;
        localFdOpenFlags |= O_RDONLY;
    }
    else if (fileStreamRef->direction == OUTGOING_FILESTREAM)
    {
        rpcFdOpenFlags |= O_RDONLY;
        localFdOpenFlags |= O_WRONLY;
    }
    /* Opening rpcfd first because that is for certain non blocking */
    rpcFd = le_fd_Open(fifoPath, rpcFdOpenFlags);    //ours to use
    localFd = le_fd_Open(fifoPath, localFdOpenFlags);//to be given to local process
    if (localFd < 0)
    {
        LE_ERROR("Failed to create local fd for stream id:[%" PRIu16 "] of system: [%s], "
                 "errno:[%" PRId32 "]", fStreamId, systemName, (int32_t)errno);
        res = LE_FAULT;
        if (rpcFd > 0)
        {
            le_fd_Close(rpcFd);
        }
        goto end;
    }
    if (rpcFd < 0)
    {
        LE_ERROR("Failed to create rpcFd for stream id:[%" PRIu16 "] of system: [%s], "
                 "errno:[%" PRId32 "]", fStreamId, systemName, (int32_t)errno);
        res = LE_FAULT;
        le_fd_Close(localFd);
        goto end;
    }
end:
    if (res == LE_OK)
    {
        *rpcFdPtr = rpcFd;
        *localFdPtr = localFd;
    }
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get space available on the channel.
 *
 * This will represent the number of bytes that can be written to rpcFd without being blocked
 *
 * @return
 *      - LE_OK if available space was read successfully.
 *      - LE_FAULT any error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcFStream_GetAvailableSpace
(
    rpcProxy_FileStream_t* fileStreamRef,  ///< [IN] Pointer to file stream instance
    uint32_t* spaceAvailablePtr            ///< [OUT] Pointer to available space
)
{
    if (le_fd_Ioctl(fileStreamRef->rpcFd, LE_FD_FIFO_GET_AVAILABLE_SPACE, spaceAvailablePtr) != 0)
    {
        LE_ERROR("Error in reading LE_FD_FIFO_GET_AVAILABLE_SPACE on fd:[%" PRId32 "] for stream:"
                 "[%" PRIu16 "]", fileStreamRef->rpcFd, fileStreamRef->streamId);
        return LE_FAULT;
    }
    return LE_OK;
}
