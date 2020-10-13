/**
 * @file le_rpcProxyFileStream.c
 *
 * This file contains the source code for the RPC Proxy File Stream Feature.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "le_rpcProxy.h"
#include "le_rpcProxyFileStream.h"


//--------------------------------------------------------------------------------------------------
/**
 * This pool stores file stream instances.
 * It is shared for both streams that are created locally first(by rpcFStream_HandleFileDescriptor)
 * and streams that are replicated from remote side(created by rpcFStream_HandleStreamId).
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(FileStreamPool,
                          RPC_PROXY_FILE_STREAM_MAX_NUM,
                          sizeof(rpcProxy_FileStream_t));
static le_mem_PoolRef_t FileStreamPoolRef = NULL;

le_dls_List_t FileStreamRefList = LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Macros used for creating a unique name for each fd monitor
 */
//--------------------------------------------------------------------------------------------------
#define RPC_FSTREAM_FD_MON_NAME_FORMAT "rpc%s%"PRIu16"%c"
#define RPC_FSTREAM_FDMON_MAX_LEN      (sizeof(RPC_FSTREAM_FD_MON_NAME_FORMAT) + \
                                        RPC_FSTREAM_ID_MAX_DIGITS + \
                                        LIMIT_MAX_SYSTEM_NAME_LEN)

//--------------------------------------------------------------------------------------------------
/**
 * Traverse the file stream list and look for a file stream with given Id from the given system
 *
 * @note: A unique file stream is only identified by its streamId, the owner flag, and the remote
 * system name.
 *
 * @return
 *      - pointer to file stream instance or NULL if not found.
 */
//--------------------------------------------------------------------------------------------------
static rpcProxy_FileStream_t* GetFileStreamRef
(
    uint16_t fStreamId,        ///< [IN] file stream ID
    const char* systemName,    ///< [IN] remote system's name
    bool owner                 ///< [IN] Owner field, true if we're the owner, false otherwise
)
{
    rpcProxy_FileStream_t* fileStreamRef;
    LE_DLS_FOREACH(&FileStreamRefList, fileStreamRef, rpcProxy_FileStream_t, link)
    {
        if ((fileStreamRef->streamId == fStreamId) &&
            (strcmp(fileStreamRef->remoteSystemName, systemName) == 0) &&
            (fileStreamRef->owner == owner))
        {
            return fileStreamRef;
        }
    }
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * delete resources in a file stream and release it self. also remove from list.
 */
//--------------------------------------------------------------------------------------------------
static void RemoveFileStreamInstance
(
    rpcProxy_FileStream_t* fileStreamRef    ///< [IN] pointer to file stream to be deleted.
)
{
    LE_ASSERT(fileStreamRef != NULL);
    LE_INFO("Removing fileStream id:[%" PRIu16 "] of system: [%s], rpcFd:[%" PRId32 "]",
             fileStreamRef->streamId, fileStreamRef->remoteSystemName, fileStreamRef->rpcFd);
    /* Delete fd monitor if it's not null.
     * If it is null, it must have been deleted when we received a POLLHUP or POLLRDHUP on rpcFd
     */
    if (fileStreamRef->fdMonitorRef)
    {
        le_fdMonitor_Delete(fileStreamRef->fdMonitorRef);
    }
    le_fd_Close(fileStreamRef->rpcFd);
    le_dls_Remove(&FileStreamRefList, &(fileStreamRef->link));
    le_mem_Release(fileStreamRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Validate stream Id flags in an rpcProxy file stream message:
 *
 * @return
 *      - LE_OK if the combination of flags is valid and LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ValidateStreamIdFlags
(
    rpcProxy_FileStream_t* fileStreamRef,    ///< [IN] pointer to file stream
    uint16_t fStreamFlags                    ///< [IN] file stream flags
)
{
    // Check flag combination:
    if (((fStreamFlags & RPC_FSTREAM_DATA_PACKET) && (fStreamFlags & RPC_FSTREAM_REQUEST_DATA)) ||
        ((fStreamFlags & RPC_FSTREAM_EOF) && (fStreamFlags & RPC_FSTREAM_REQUEST_DATA)) ||
        ((fStreamFlags & RPC_FSTREAM_IOERROR) && (fStreamFlags & RPC_FSTREAM_REQUEST_DATA)) ||
        ((fStreamFlags & RPC_FSTREAM_FORCE_CLOSE) && (fStreamFlags & RPC_FSTREAM_REQUEST_DATA)))
    {
        return LE_FAULT;
    }
    // Check whether these flags are expected according to the file stream direction:
    fStream_direction_t direction = fileStreamRef->direction;
    if (((direction == INCOMING_FILESTREAM) && (fStreamFlags & RPC_FSTREAM_REQUEST_DATA)) ||
        ((direction == OUTGOING_FILESTREAM) && (fStreamFlags & (RPC_FSTREAM_DATA_PACKET |
                                                                RPC_FSTREAM_EOF |
                                                                RPC_FSTREAM_IOERROR))))
    {
        return LE_FAULT;
    }

    return LE_OK;

}

//--------------------------------------------------------------------------------------------------
/**
 * Compare function for sorting file streams according to id.
 * Compares two file stream ids
 *
 * @return
 *          - true if first file stream has a lower id, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool CompareFileStreamIds
(
    le_dls_Link_t* aLinkPtr,
    le_dls_Link_t* bLinkPtr
)
{
    rpcProxy_FileStream_t* fileStreamAPtr = CONTAINER_OF(aLinkPtr, rpcProxy_FileStream_t, link);
    rpcProxy_FileStream_t* fileStreamBPtr = CONTAINER_OF(bLinkPtr, rpcProxy_FileStream_t, link);
    return  (fileStreamAPtr->streamId < fileStreamBPtr->streamId);
}
//--------------------------------------------------------------------------------------------------
/**
 * Get a new and unique stream ID:
 *
 * @return
 *      - A new and unique stream ID
 */
//--------------------------------------------------------------------------------------------------
static uint16_t GetUniqueStreamId
(
    void
)
{
    uint16_t uniqueId = 0;
    le_dls_Sort(&FileStreamRefList, CompareFileStreamIds);
    rpcProxy_FileStream_t* fileStreamRef;
    LE_DLS_FOREACH(&FileStreamRefList, fileStreamRef, rpcProxy_FileStream_t, link)
    {
        if (fileStreamRef->streamId == uniqueId)
        {
            uniqueId++;
        }
        else if (fileStreamRef->streamId > uniqueId)
        {
            break;
        }
    }
    return uniqueId;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send an rpc proxy filestream message with given stream id.
 */
//--------------------------------------------------------------------------------------------------
static void SendFileStreamErrorMessage
(
    const char* systemName,          ///< [IN] Name of remote system the message is to be send to
    uint32_t serviceId,              ///< [IN] service id
    uint16_t streamId,               ///< [IN] streamId including flags
    uint16_t flags                   ///< [IN] flags to include
)
{
    rpcProxy_FileStreamMessage_t fileStreamMsg = {0};
    fileStreamMsg.commonHeader.id = rpcProxy_GenerateProxyMessageId();
    fileStreamMsg.commonHeader.serviceId = serviceId;
    fileStreamMsg.commonHeader.type = RPC_PROXY_FILESTREAM_MESSAGE;

    fileStreamMsg.metaData.fileStreamId = streamId;
    fileStreamMsg.metaData.fileStreamFlags = flags;
    fileStreamMsg.metaData.isFileStreamValid = true;

    le_result_t result = rpcProxy_SendMsg(systemName, &fileStreamMsg);
    if (result != LE_OK)
    {
        LE_ERROR("le_comm_Send failed, result [%" PRId32 "]", (int32_t)result);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle a POLLIN event on an outgoing file stream.
 */
//--------------------------------------------------------------------------------------------------
static void HandlePollinOutgoing
(
    rpcProxy_FileStream_t* fileStreamRef       ///< [IN] pointer to file stream instance
)
{
    LE_ASSERT(fileStreamRef->direction = OUTGOING_FILESTREAM);
    if (fileStreamRef->requestedSize == 0)
    {
        // other side cannot accept data now. so ignore this.
        // but to make sure we don't get any more notifications, disable fd monitor:
        LE_WARN("fd monitor handler called even though other side buffer is 0");
        if (fileStreamRef->fdMonitorRef)
        {
            le_fdMonitor_Disable(fileStreamRef->fdMonitorRef, POLLIN);
        }
    }
    else
    {
        // There's something to read, read it and send to the other side.
        // note: rpcFd is non blocking
        const char* systemName = fileStreamRef->remoteSystemName;
        rpcProxy_FileStreamMessage_t fileStreamMsg = {0};
        rpcProxy_MessageMetadata_t* metaDataPtr = &(fileStreamMsg.metaData);
        metaDataPtr->fileStreamFlags = 0;
        metaDataPtr->fileStreamFlags |= RPC_FSTREAM_DATA_PACKET;
        metaDataPtr->fileStreamFlags |= (fileStreamRef->owner)? RPC_FSTREAM_OWNER : 0;
        metaDataPtr->fileStreamId = fileStreamRef->streamId;
        metaDataPtr->isFileStreamValid = true;
        fileStreamMsg.commonHeader.id = rpcProxy_GenerateProxyMessageId();
        fileStreamMsg.commonHeader.type = RPC_PROXY_FILESTREAM_MESSAGE;
        fileStreamMsg.commonHeader.serviceId = fileStreamRef->serviceId;
        size_t bytesToRead = (fileStreamRef->requestedSize > RPC_PROXY_MAX_FILESTREAM_PAYLOAD_SIZE)?
            RPC_PROXY_MAX_FILESTREAM_PAYLOAD_SIZE:fileStreamRef->requestedSize;
        size_t totalBytesRead = 0;
        uint8_t* buffPtr = fileStreamMsg.payload;
        int fd = fileStreamRef->rpcFd;
        while(totalBytesRead < bytesToRead)
        {
            ssize_t bytesRead = le_fd_Read(fd, buffPtr + totalBytesRead,
                                           bytesToRead - totalBytesRead);
            if ( bytesRead < 0 )
            {
                // check errno
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // done reading what's to read, get out of this loop.
                    break;
                }
                else
                {
                    // serious error while reading from fifo:
                    RemoveFileStreamInstance(fileStreamRef);
                    metaDataPtr->fileStreamFlags |= RPC_FSTREAM_IOERROR;
                    fileStreamRef = NULL;
                    break;
                }
            }
            else
            {
                totalBytesRead += bytesRead;
                if (bytesRead == 0)
                {
                    // EOF is reached
                    RemoveFileStreamInstance(fileStreamRef);
                    metaDataPtr->fileStreamFlags |= RPC_FSTREAM_EOF;
                    fileStreamRef = NULL;
                    break;
                }
            }
        } // End of while
        fileStreamMsg.payloadSize += totalBytesRead;
        if (fileStreamRef)
        {
            /* Reset the requestedSize and disable the fd monitor. The other side will
             * request again with their new buffer amount if they have space upon receiving this
             * data packet.
             */
            fileStreamRef->requestedSize = 0;
            le_fdMonitor_Disable(fileStreamRef->fdMonitorRef, POLLIN);
        }
#if RPC_PROXY_HEX_DUMP
        LE_INFO("Sending this rpc filestream data messgae to %s:", systemName);
        LE_LOG_DUMP(LE_LOG_INFO, fileStreamMsg.payload, fileStreamMsg.payloadSize);
#endif
        le_result_t result = rpcProxy_SendMsg(systemName, &fileStreamMsg);
        if (result != LE_OK)
        {
            LE_ERROR("le_comm_Send failed, result [%" PRId32 "]", (int32_t)result);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle a POLLOUT event on an incoming file stream.
 */
//--------------------------------------------------------------------------------------------------
static void HandlePolloutIncoming
(
    rpcProxy_FileStream_t* fileStreamRef       ///< [IN] pointer to file stream instance
)
{
    LE_ASSERT(fileStreamRef->direction = INCOMING_FILESTREAM);
    const char* systemName = fileStreamRef->remoteSystemName;
    uint32_t bytesToRequest;
    if (rpcFStream_GetAvailableSpace(fileStreamRef, &bytesToRequest) != LE_OK)
    {
        LE_ERROR("Error in reading available space for stream:[%" PRIu16 "]",
                 fileStreamRef->streamId);
        le_fdMonitor_Disable(fileStreamRef->fdMonitorRef, POLLOUT);
    }
    if (bytesToRequest == 0)
    {
        /*
         * this should not happen because if fifo has zero capacity then fdmonitor
         * would not call us to write
         */
        return;
    }
    rpcProxy_FileStreamMessage_t fileStreamMsg = {0};
    rpcProxy_MessageMetadata_t* metaDataPtr = &(fileStreamMsg.metaData);
    metaDataPtr->fileStreamId = fileStreamRef->streamId;
    metaDataPtr->fileStreamFlags |= RPC_FSTREAM_REQUEST_DATA;
    metaDataPtr->fileStreamFlags |= (fileStreamRef->owner)? RPC_FSTREAM_OWNER : 0;
    metaDataPtr->isFileStreamValid = true;
    fileStreamMsg.requestedSize = bytesToRequest;
    fileStreamMsg.payloadSize = 0;

    fileStreamMsg.commonHeader.id = rpcProxy_GenerateProxyMessageId();
    fileStreamMsg.commonHeader.type = RPC_PROXY_FILESTREAM_MESSAGE;
    fileStreamMsg.commonHeader.serviceId = fileStreamRef->serviceId;

    //disable this fd monitor:
    le_fdMonitor_Disable(fileStreamRef->fdMonitorRef, POLLOUT);
#if RPC_PROXY_HEX_DUMP
    LE_INFO("Sending request message to %s, request size:[%" PRId32 "]", systemName, bytesToRequest);
#endif
    le_result_t result = rpcProxy_SendMsg(systemName, &fileStreamMsg);
    if (result != LE_OK)
    {
        LE_ERROR("le_comm_Send failed, result [%" PRId32 "]", (int32_t)result);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for file descriptor monitoring fifo
 *
 * The fd here is rpcFd. It is important to remember that rpcFd can be of any type, regular file,
 * pipe or socket(unix or network). This is because although when a file stream instance is created
 * by HandleStreamId the rpcFd is always a pipe, when created by HandleFileDescriptor, rpcFd is
 * given to us by the local client/server already opened and it can have any type.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FileStreamFifoHandler
(
    int fd,
    short events
)
{
    rpcProxy_FileStream_t* fileStreamRef = (rpcProxy_FileStream_t*) le_fdMonitor_GetContextPtr();
    LE_ASSERT(fileStreamRef != NULL);
    LE_ASSERT(le_fdMonitor_GetMonitor() == fileStreamRef->fdMonitorRef);
    LE_ASSERT(fd == fileStreamRef->rpcFd);
    LE_DEBUG("FileStreamFifoHandler: fd:[%" PRId32 "], stream id:[%" PRIu16 "],"
             "events:[%" PRId16 "]", (int32_t)fd, fileStreamRef->streamId, events);

    // we shouldn't be notified for writing on an outgoing file stream
    LE_ASSERT(!((events & POLLOUT) && (fileStreamRef->direction == OUTGOING_FILESTREAM)));
    // we shouldn't be notified for reading on an incoming file stream
    LE_ASSERT(!((events & POLLIN) && (fileStreamRef->direction == INCOMING_FILESTREAM)));

    if ((events & POLLIN) && fileStreamRef->direction == OUTGOING_FILESTREAM)
    {
        HandlePollinOutgoing(fileStreamRef);
    }

    else if ((events & POLLOUT) && fileStreamRef->direction == INCOMING_FILESTREAM)
    {
        HandlePolloutIncoming(fileStreamRef);
    }

    else if ((events & POLLERR) ||
        ((events & (POLLHUP)) && fileStreamRef->direction == INCOMING_FILESTREAM))
    {
        /* For incoming streams, rpcFd is the write only side(incoming data is written to the
         * rcpFd). If rpcFd is a pipe and the localFD (read side) is closed we will get a POLLERR.
         * We propagate this to the other side with a close flag so they will close their fd as
         * well.
         * If rpcFd is a socket, and localFd is closed, We will get a POLLHUP and POLLERR.
         * Which we will again propagate to the other side with a close flag.
         * If we receive a POLLRDHUP on rpcFd of an incoming stream it means the other side has
         * shutdown the the write half of their socket. We're not processing this event because
         * for incoming streams, rpcFd only does write operation and we don't expect the other side
         * do any writing anyway.
        */
        // some error happened. close and propagate this to the other side
        // downgrading the log to info level becuase this is possibly because the read side has
        // closed localfd
        LE_INFO("Received POLLERR on rpcFd of stream id: [%" PRIu16 "]", fileStreamRef->streamId);
        const char* systemName = fileStreamRef->remoteSystemName;
        uint32_t flags = RPC_FSTREAM_FORCE_CLOSE;
        flags |= (fileStreamRef->owner)? RPC_FSTREAM_OWNER : 0;
        SendFileStreamErrorMessage(systemName, fileStreamRef->serviceId,
                                   fileStreamRef->streamId, flags);
        RemoveFileStreamInstance(fileStreamRef);
        fileStreamRef = NULL;
        return;

    }
    else if ((events & (POLLHUP | POLLRDHUP)) && fileStreamRef->direction == OUTGOING_FILESTREAM)
    {
       /*
        * About outgoing streams and pollhup or polldrhup events:
        * For outgoing streams, rpcFd is the read only side and localFd is the write side.
        * If localFd is closed a POLLHUP will be generated on rpcFD, if rpcFd and localFd are
        * sockets, and write half of localFd is shutdown or FIN packet is received, a POLLRDHUP is
        * reported on rpcFd.
        * This event will keep getting raised until we get requested for more data by the rpc peer
        * read all remaining data and close the socket or pipe.
        * The other side may never ask for any more data or just ask for more data after a very
        * long time. But we need to hold on to the data in case they do.
        * We will disable and delete the fdMonitorRef, and when we were requested for data we read
        * from rpcFd directly.
        */
        le_fdMonitor_Delete(fileStreamRef->fdMonitorRef);
        fileStreamRef->fdMonitorRef = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the embedded file descriptor of an le_msg_MessageRef_t
 *
 * @return
 *      - LE_OK if successfully handled file descriptor.
 *      - LE_NO_MEMORY a memory pool was full.
 *      - LE_FAULT for any other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcFStream_HandleFileDescriptor
(
    le_msg_MessageRef_t msgRef,               ///< [IN] ipc message reference
    rpcProxy_MessageMetadata_t* metaDataPtr,  ///< [OUT] pointer to proxy message meta data
    uint32_t serviceId,                       ///< [IN] service id
    const char* systemName                    ///< [IN] name of system message is being prepared for
)
{
    int32_t fdToSend = le_msg_GetFd(msgRef);
    le_result_t res = LE_OK;
    if (fdToSend < 0)
    {
        metaDataPtr->isFileStreamValid = false;
        goto end;
    }

    LE_INFO("msgRef contains file descriptor: [%" PRId32 "] to send to: [%s]", fdToSend,
            systemName);
    // check local fd flags and make sure it's one directional.
    // use w/r flag to determine direction.
    fStream_direction_t direction;
    bool isFdToSendBlocking = false;
    int fdToSendFlags;
    fdToSendFlags = le_fd_Fcntl(fdToSend, F_GETFL);
    if (fdToSendFlags == -1)
    {
        res = LE_FAULT;
        LE_ERROR("Error in getting file descriptor flags of fd:[%" PRId32 "], errno:[%" PRId32 "]",
                 fdToSend, (int32_t)errno);
        goto end;
    }
    int rpcFd = fdToSend; // this is our FD now

    if ((fdToSendFlags & O_ACCMODE) == O_RDWR)
    {
        LE_ERROR("Bidirectional file streams are not supported yet");
        res = LE_UNSUPPORTED;
        goto end;
    }
    else if((fdToSendFlags & O_ACCMODE) == O_WRONLY)
    {
        direction = INCOMING_FILESTREAM;
    }
    else if((fdToSendFlags & O_ACCMODE) == O_RDONLY)
    {
        direction = OUTGOING_FILESTREAM;
    }
    else
    {
        LE_ERROR("Error in determining file stream direction");
        res = LE_FAULT;
        goto end;
    }

    if (fdToSendFlags & O_NONBLOCK)
    {
        isFdToSendBlocking= true;
    }
   //make fd to send (rpcFd) non blocking because we don't want rpcdaemon to be blocked
   //EINVAL error is checked because on freeRTOS, using F_SETFL on stdout or stderr returns -1
   if (le_fd_Fcntl(rpcFd, F_SETFL, fdToSendFlags | O_NONBLOCK) != 0 && (errno != EINVAL))
   {
       LE_ERROR("Not able to make rpcFd non blocking");
       res = LE_FAULT;
       goto end;
   }

    /* Create a file stream instance.
     * We're using TryAlloc here instead of Alloc because FileStreamPoolRef is a shared pool
     * for both streams that were created by HandleFileDescriptor and rpcFStream_HandleStreamId.
     * Streams created by rpcFStream_HandleStreamId may have used up the pool and we don't want
     * a crash here in that case.
     * */
    rpcProxy_FileStream_t* fileStreamRef = le_mem_TryAlloc(FileStreamPoolRef);
    if (fileStreamRef == NULL)
    {
        LE_ERROR("No memory left to allocate fileStreamRef");
        res = LE_NO_MEMORY;
        goto end;
    }
    // find a fStreamId that is unique:
    fileStreamRef->streamId = GetUniqueStreamId();
    fileStreamRef->owner = true;
    fileStreamRef->rpcFd = rpcFd;
    fileStreamRef->serviceId = serviceId;
    fileStreamRef->remoteSystemName = systemName;
    fileStreamRef->direction = direction;
    fileStreamRef->requestedSize = 0;

    // create fd monitor for the fd.
    // setup an fd monitor on rpcFd(fd to send), use events according to direction.
    char fdMonName[RPC_FSTREAM_FDMON_MAX_LEN] = {0};
    snprintf(fdMonName, sizeof(fdMonName), RPC_FSTREAM_FD_MON_NAME_FORMAT, systemName,
             fileStreamRef->streamId, (fileStreamRef->owner)?'u':'t');
    le_fdMonitor_Ref_t fdMonitor = le_fdMonitor_Create(fdMonName, rpcFd, FileStreamFifoHandler, 0);
    /*
     * We only enable the fd monitor for incoming streams because we want to request for data.
     * For outgoing streams, we enable the fd monitor once the other side made a request
     */
    if (direction == INCOMING_FILESTREAM)
    {
        le_fdMonitor_Enable(fdMonitor, POLLOUT);
    }
    fileStreamRef->fdMonitorRef = fdMonitor;
    // set context for this fd monitor:
    le_fdMonitor_SetContextPtr(fdMonitor, fileStreamRef);

    fileStreamRef->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&FileStreamRefList, &(fileStreamRef->link));

    metaDataPtr->fileStreamId = fileStreamRef->streamId;
    uint16_t flags = 0;
    // set flags:
    if (direction == OUTGOING_FILESTREAM)
    {
        flags |= RPC_FSTREAM_INIT_OUTGOING;
    }
    else if (direction == INCOMING_FILESTREAM)
    {
        flags |= RPC_FSTREAM_INIT_INCOMING;
    }
    if (isFdToSendBlocking)
    {
        flags |= RPC_FSTREAM_NONBLOCK;
    }
    // this filestream was just created in our system, so it's owned by us:
    flags |= RPC_FSTREAM_OWNER;
    metaDataPtr->fileStreamFlags = flags;
    LE_DEBUG("Created filestream:[%" PRIu16 "], flags:[%" PRIu16 "], for system: [%s]",
             metaDataPtr->fileStreamId, metaDataPtr->fileStreamFlags, systemName);
    metaDataPtr->isFileStreamValid = true;
end:
    if (res != LE_OK)
    {
        metaDataPtr->isFileStreamValid = false;
        le_fd_Close(fdToSend);
    }
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the embedded stream in an rpc proxy message
 *
 * @return
 *      - LE_OK if handled properly and without error.
 *      - LE_NO_MEMORY ran into memory issue while handing stream id
 *      - LE_FAULT any other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcFStream_HandleStreamId
(
    le_msg_MessageRef_t msgRef,               ///< [IN] ipc message reference
    rpcProxy_MessageMetadata_t* metaDataPtr,  ///< [IN] pointer to proxy message meta data
    uint32_t serviceId,                       ///< [IN] service id
    const char* systemName                    ///< [IN] name of system message was received on
)
{
    rpcProxy_FileStream_t* fileStreamRef = NULL;
    le_result_t res = LE_OK;
    if (!metaDataPtr->isFileStreamValid)
    {
        goto end;
    }
    uint16_t fStreamId = metaDataPtr->fileStreamId;
    /*
     * The owner flag is set from the perspective of the sender so it must be inverted to reflect
     * our ownership of the filestream
     */
    bool owner = !(metaDataPtr->fileStreamFlags & RPC_FSTREAM_OWNER);
    if (owner)
    {
        // we're being asked to create a stream that we own! this isn't possible here,
        // we create our streams in handleFileDescriptor.
        LE_ERROR("rpcProxy message has file stream with wrong owner flag");
        res = LE_FAULT;
        goto end;
    }
    LE_INFO("rpcProxy message has a valid file stream id: [%" PRId16 "]", fStreamId);
    rpcProxy_FileStream_t* tmpFileStream = GetFileStreamRef(fStreamId, systemName, owner);
    if (tmpFileStream != NULL)
    {
        LE_WARN("Received proxy message to create stream with an Id that already exists."
                "Deleting streamId:[%" PRIu16 "] of system: [%s]",fStreamId, systemName);
        RemoveFileStreamInstance(tmpFileStream);
    }
    fStream_direction_t direction;
    uint16_t flags = metaDataPtr->fileStreamFlags;
    bool isLocalFdNonBlocking = false;
    // the initial stream id also contains flags indicating the direction of this stream from
    // the other side's perspective, our direction is the opposite.
    if (flags & RPC_FSTREAM_INIT_INCOMING)
    {
        direction = OUTGOING_FILESTREAM;
    }
    else if (flags & RPC_FSTREAM_INIT_OUTGOING)
    {
        direction = INCOMING_FILESTREAM;
    }
    else
    {
        LE_ERROR("Received new stream without the direction flag set");
        res = LE_FAULT;
        goto end;
    }

    if (flags & RPC_FSTREAM_NONBLOCK)
    {
        isLocalFdNonBlocking = true;
    }
    /* Create our side of the filestream
     * Using TryAlloc here because this stream is given to us by the remote side and we don't
     * want a crash if the remote side is giving us too many streams.
     */
    fileStreamRef = le_mem_TryAlloc(FileStreamPoolRef);
    if (fileStreamRef == NULL)
    {
        LE_ERROR("Cannot create any more file stream instances");
        res = LE_NO_MEMORY;
        goto end;
    }
    fileStreamRef->remoteSystemName = systemName;
    fileStreamRef->serviceId = serviceId;
    fileStreamRef->streamId = fStreamId;
    fileStreamRef->direction = direction;
    fileStreamRef->owner = false;
    int32_t localFd = -1;
    int32_t rpcFd = -1;
    if (rpcFStream_CreateChannel(fileStreamRef, &rpcFd, &localFd, isLocalFdNonBlocking) != LE_OK)
    {
        LE_ERROR("Error in creating a channel for stream id:[%" PRIu16 "] of system: [%s]",
                fStreamId, systemName);
    }
    LE_INFO("Opened two sides of fifo: localFd:[%" PRId32 "], rpcFd:[%" PRId32 "], "
            "direction:[%" PRId32 "]", localFd, rpcFd, (int32_t)direction);

    //TODO: need to give SMACK access to the receiving process:(LE-15062)

    fileStreamRef->rpcFd = rpcFd;
    // hook up an fd monitor to this fd:
    char fdMonName[RPC_FSTREAM_FDMON_MAX_LEN] = {0};
    snprintf(fdMonName, sizeof(fdMonName), RPC_FSTREAM_FD_MON_NAME_FORMAT, systemName,
             fileStreamRef->streamId, (fileStreamRef->owner)?'u':'t');
    le_fdMonitor_Ref_t fdMonitor = le_fdMonitor_Create(fdMonName, rpcFd, FileStreamFifoHandler, 0);
    /*
     * We only enable the fd monitor for incoming streams because we want to request for data.
     * For outgoing streams, we enable the fd monitor once the other side made a request
     */
    if (direction == INCOMING_FILESTREAM)
    {
        le_fdMonitor_Enable(fdMonitor, POLLOUT);
    }
    // set context for this fd monitor:
    le_fdMonitor_SetContextPtr(fdMonitor, fileStreamRef);

    fileStreamRef->fdMonitorRef = fdMonitor;
    le_msg_SetFd(msgRef, localFd);

    //store the file stream ptr into the list:
    fileStreamRef->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&FileStreamRefList, &(fileStreamRef->link));
end:
    if (res != LE_OK)
    {
        if (fileStreamRef)
        {
            le_mem_Release(fileStreamRef);
        }
        // An error has happened, send an error to the other side to close their file stream:
        //we're never the owner here.
        SendFileStreamErrorMessage(systemName, serviceId, fStreamId, RPC_FSTREAM_FORCE_CLOSE);
    }
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process a file Stream message
 *
 * @return
 *      - LE_OK if message was processed without error
 *      - LE_FAULT an error happened during processing message
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcFStream_ProcessFileStreamMessage
(
    void* handle,                   ///< [IN] Opaque handle to the le_comm communication channel
    const char* systemName,         ///< [IN] name of system this message was received from
    StreamState_t* streamStatePtr,  ///< [IN] Pointer to the Stream State-Machine data
    void* proxyMessagePtr           ///< [IN] pointer to rpc message received
)
{
    rpcProxy_FileStreamMessage_t* fileStreamMsgPtr = (rpcProxy_FileStreamMessage_t*) proxyMessagePtr;
    le_result_t recvRes = rpcProxy_RecvStream(handle, streamStatePtr, fileStreamMsgPtr);
    if (recvRes == LE_IN_PROGRESS)
    {
        // return now, come back later
        return LE_OK;
    }
    else if (recvRes != LE_OK)
    {
        LE_ERROR("Error when receiving a file stream");
        return LE_FAULT;
    }

    rpcProxy_MessageMetadata_t* metaDataPtr = &(fileStreamMsgPtr->metaData);

    uint16_t fStreamId = metaDataPtr->fileStreamId;
    uint16_t fStreamFlags = metaDataPtr->fileStreamFlags;
    uint8_t* msgBufPtr = &fileStreamMsgPtr->payload[0];
    /*
     * The owner flag is set from the perspective of the sender so it must be inverted to reflect
     * our ownership of the filestream
     */
    bool owner = !(metaDataPtr->fileStreamFlags & RPC_FSTREAM_OWNER);
    rpcProxy_FileStream_t* fileStreamRef = GetFileStreamRef(fStreamId, systemName, owner);
    if (fileStreamRef)
    {
        LE_DEBUG("Found a matching stream id:[%" PRIu16 "], rpcfd:[%" PRId32 "], system: [%s]",
                fStreamId, fileStreamRef->rpcFd, systemName);
    }
    else
    {
        LE_ERROR("Cannot find file stream id %"PRIu16" send by %s in local list", fStreamId,
                 systemName);
        uint32_t flags = RPC_FSTREAM_FORCE_CLOSE;
        flags |= (owner)? RPC_FSTREAM_OWNER : 0;
        SendFileStreamErrorMessage(systemName, fileStreamMsgPtr->commonHeader.serviceId,
                                   fStreamId, flags);
        return LE_FAULT;
    }
    if (fileStreamRef->serviceId != fileStreamMsgPtr->commonHeader.serviceId)
    {
        LE_ERROR("rpcProxy file stream message service id does not match expected value");
        return LE_FAULT;
    }
    if (ValidateStreamIdFlags(fileStreamRef, fStreamFlags) != LE_OK)
    {
        LE_ERROR("rpcProxy file stream message has an invalid combination of flags");
        return LE_FAULT;
    }
    if (fStreamFlags & RPC_FSTREAM_DATA_PACKET)
    {
#if RPC_PROXY_HEX_DUMP
        LE_INFO("Received this data packet from %s:", systemName);
        LE_LOG_DUMP(LE_LOG_INFO, fileStreamMsgPtr->payload, fileStreamMsgPtr->payloadSize);
#endif
        size_t bufferSize = fileStreamMsgPtr->payloadSize;
        LE_INFO("Received file stream message with DATA_PACKET flag. size: %d", bufferSize);
        ssize_t bytesWritten = 0;

        if (bufferSize > 0)
        {
            bytesWritten = le_fd_Write(fileStreamRef->rpcFd, msgBufPtr, bufferSize);
        }
        bool closeIndication = fStreamFlags & (RPC_FSTREAM_EOF | RPC_FSTREAM_IOERROR);
        /*
         * If closeIndication is true it means there is a flag that indicates we have to close our
         * stream, this case is treated separately later in this function because despite local
         * errors, when there is closeIndication we don't need to send an error message to the
         * other side.
         */
        if ((bytesWritten < 0 || (size_t)bytesWritten < bufferSize) && !closeIndication)
        {
            LE_INFO("Need to close rpcFd because of le_fd_Write error, errno:[%" PRId32 "]",
                    (int32_t)errno);
            // This is a local error, so need to tell the other side that we've closed stream.
            // This could've happened because localfd is closed by the process holding it.
            uint32_t flags = RPC_FSTREAM_FORCE_CLOSE;
            flags |= (owner)? RPC_FSTREAM_OWNER : 0;
            SendFileStreamErrorMessage(systemName, fileStreamRef->serviceId,
                                       fileStreamRef->streamId, flags);
            RemoveFileStreamInstance(fileStreamRef);
            fileStreamRef = NULL;
            return LE_OK;
        }
        else if(!closeIndication)
        {
            // TODO:(LE-15063) check if we've got all we've asked for and only then enable the fd
            // monitor
            //enable fd monitor to get notification once we can request more:
            le_fdMonitor_Enable(fileStreamRef->fdMonitorRef, POLLOUT);
        }
    }
    if(fStreamFlags & RPC_FSTREAM_REQUEST_DATA)
    {
        LE_DEBUG("Received file stream message with REQUEST_DATA flag from system %s", systemName);
#if RPC_PROXY_HEX_DUMP
        LE_INFO("Received this data request message from %s:", systemName);
        LE_LOG_DUMP(LE_LOG_INFO, fileStreamMsgPtr->payload, fileStreamMsgPtr->payloadSize);
#endif
        uint32_t requested_size = fileStreamMsgPtr->requestedSize;;
        LE_INFO("Other side of stream id:[%" PRIu16 "] at system: [%s] requested [%" PRIu32 "] "
                 "bytes", fStreamId, systemName, requested_size);
        fileStreamRef->requestedSize = requested_size;
        if (fileStreamRef->fdMonitorRef != NULL)
        {
           le_fdMonitor_Enable(fileStreamRef->fdMonitorRef, POLLIN);
        }
        else
        {
            /* if the fdmonitor reference is null. we must have received POLLHUP or POLLRDHUP on
             * rpcFd. This means localFd is done writing. So we can read whatever is left to read
             * here without being blocked untill EOF.
             */
            HandlePollinOutgoing(fileStreamRef);
        }
    }
    if((fStreamFlags & RPC_FSTREAM_FORCE_CLOSE) || (fStreamFlags & RPC_FSTREAM_IOERROR) ||
       (fStreamFlags & RPC_FSTREAM_EOF))
    {
        LE_INFO("Received file stream message with indication to close streamId:[%" PRIu16 "] from "
                "system [%s]", fStreamId, systemName);
        RemoveFileStreamInstance(fileStreamRef);
        fileStreamRef = NULL;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete instance of a specific streamid that is owned by us if it's valid
 */
//--------------------------------------------------------------------------------------------------
void rpcFStream_DeleteOurStream
(
    uint16_t fStreamId,              ///< [IN] id of the file stream instance to be deleted
    const char* systemName           ///< [IN] The system that of it's file stream is to be deleted
)
{
    rpcProxy_FileStream_t* fileStreamRef = GetFileStreamRef(fStreamId, systemName, true);
    if (fileStreamRef != NULL)
    {
        RemoveFileStreamInstance(fileStreamRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete all file streams associated with a system
 */
//--------------------------------------------------------------------------------------------------
void rpcFStream_DeleteStreamsBySystemName
(
    const char* systemName          ///< [IN] The system that of it's file streams are to be deleted
)
{
    rpcProxy_FileStream_t* fileStreamRef;
    LE_DLS_FOREACH(&FileStreamRefList, fileStreamRef, rpcProxy_FileStream_t, link)
    {
        if (strncmp(fileStreamRef->remoteSystemName, systemName, LIMIT_MAX_SYSTEM_NAME_LEN) == 0)
        {
            RemoveFileStreamInstance(fileStreamRef);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete all file streams associated with a service Id:
 */
//--------------------------------------------------------------------------------------------------
void rpcFStream_DeleteStreamsByServiceId
(
    uint32_t serviceId              ///< [IN] Id of service being closed.
)
{
    rpcProxy_FileStream_t* fileStreamRef;
    LE_DLS_FOREACH(&FileStreamRefList, fileStreamRef, rpcProxy_FileStream_t, link)
    {
        if (fileStreamRef->serviceId == serviceId)
        {
            RemoveFileStreamInstance(fileStreamRef);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize file stream pool:
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcFStream_InitFileStreamPool
(
    void
)
{
    FileStreamPoolRef = le_mem_InitStaticPool(
             FileStreamPool,
             RPC_PROXY_FILE_STREAM_MAX_NUM,
             sizeof(rpcProxy_FileStream_t));
    return LE_OK;
}
