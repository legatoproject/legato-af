/**
 * @file le_rpcProxyFileStreamPipe.c
 *
 * This file contains the source code for generating a pipe for each file stream
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "le_rpcProxy.h"
#include "le_rpcProxyFileStream.h"

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
    int32_t localFd = -1;
    int32_t rpcFd = -1;
    le_result_t res = LE_OK;
    const char* systemName = fileStreamRef->remoteSystemName;
    uint16_t fStreamId = fileStreamRef->streamId;
    int fds[2];
    /* Our fd must be non blocking so rpcDaemon is never blocked */
    if (pipe2(fds, O_NONBLOCK) != 0)
    {
        LE_ERROR("Error in creating pipe for file stream id:[%" PRIu16 "] of system: [%s],"
                 "errno:[%" PRId32 "]", fStreamId, systemName, (int32_t)errno);
        res = LE_FAULT;
        goto end;
    }
    if (fileStreamRef->direction == INCOMING_FILESTREAM)
    {
        rpcFd = fds[1];
        localFd = fds[0];
    }
    else if (fileStreamRef->direction == OUTGOING_FILESTREAM)
    {
        rpcFd = fds[0];
        localFd = fds[1];
    }
    if (!isLocalFdNonBlocking)
    {
        /* both fds are created as non blocking, here we make localfd blocking if non blocking
         * flag is not requested by remote side
         */
        int localFdOpenFlags = le_fd_Fcntl(localFd , F_GETFL);
        localFdOpenFlags = localFdOpenFlags & (~O_NONBLOCK);
        le_fd_Fcntl(localFd, F_SETFL, localFdOpenFlags);
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
    size_t bytesInFifo = 0;
    le_fd_Ioctl(fileStreamRef->rpcFd, FIONREAD, &bytesInFifo);
    int32_t fdBufferSize = le_fd_Fcntl(fileStreamRef->rpcFd, F_GETPIPE_SZ);
    if (fdBufferSize <= 0)
    {
        LE_ERROR("Pipe size is invalid for incoming stream, Cannot request data from system: [%s]",
                 fileStreamRef->remoteSystemName);
        return LE_FAULT;
    }
    *spaceAvailablePtr = fdBufferSize - bytesInFifo;
    return LE_OK;
}
