/** @file unixSocket.c
 *
 * Legato @ref c_unixSockets implementation file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "unixSocket.h"
#include "fileDescriptor.h"

/// Size of the ancillary (control) message buffer needed to send or receive one file descriptor
/// and one set of process credentials through a Unix domain socket.
/// @note We use CMSG_SPACE to ensure this is big enough to hold the cmsghdr structures.
#define CMSG_BUFF_SIZE (CMSG_SPACE(sizeof(int)) + CMSG_SPACE(sizeof(struct ucred)))


//--------------------------------------------------------------------------------------------------
/**
 * Extract a file descriptor from an SCM_RIGHTS ancillary data message.
 *
 * @return The file descriptor.
 */
//--------------------------------------------------------------------------------------------------
static int ExtractFileDescriptor
(
    struct cmsghdr* cmsgHeaderPtr
)
//--------------------------------------------------------------------------------------------------
{
    int fd;
    memcpy(&fd, CMSG_DATA(cmsgHeaderPtr), sizeof(int));

    LE_DEBUG("Received fd (%d).", fd);

    // Check that we didn't receive more than one file descriptor.
    // If we did, close the extra ones.
    size_t extraBytes = cmsgHeaderPtr->cmsg_len - CMSG_LEN(sizeof(int));
    int* extraFdPtr = ((int*)CMSG_DATA(cmsgHeaderPtr)) + 1;
    while (extraBytes >= sizeof(int))
    {
        int extraFd;
        LE_WARN("Discarding extra received file descriptor.");
        memcpy(&extraFd, extraFdPtr, sizeof(int));
        fd_Close(extraFd);
        extraBytes -= sizeof(int);
        extraFdPtr += 1;
    }

    return fd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Extract ancillary data from a received message.
 */
//--------------------------------------------------------------------------------------------------
static void ExtractAncillaryData
(
    struct msghdr* msgHeaderPtr,    ///< [IN] Pointer to the struct msghdr that was filled in
                                    ///       by a call to recvmsg().
    int* fdPtr,             ///< [OUT] Pointer to where the received file descriptor will be put.
                            ///        (-1 will be stored here if no fd was received.)
    struct ucred* credPtr   ///< [OUT] Pointer to where received credentials will be stored.
                            ///        (NOTE: PID is set to zero if no credentials received.)
)
//--------------------------------------------------------------------------------------------------
{
    // If we are trying to receive a file descriptor, set the output param to -1 in case we don't.
    if (NULL != fdPtr)
    {
        *fdPtr = -1;
    }

    // If we are trying to receive process credentials, set the output param's PID to 0 in case we
    // don't.
    if (NULL != credPtr)
    {
        credPtr->pid = 0;
    }

    // Get a pointer to the first control message header inside our control message buffer.
    struct cmsghdr* cmsgHeaderPtr = CMSG_FIRSTHDR(msgHeaderPtr);

    if (NULL == cmsgHeaderPtr)
    {
        LE_ERROR("Invalid control message header ptr");
        return;
    }

    // Until there aren't any ancillary data messages left,
    do
    {
        // First, check that the message's "level" is correct.
        if (cmsgHeaderPtr->cmsg_level != SOL_SOCKET)
        {
            LE_ERROR("Received unexpected ancillary data message level %d.",
                     cmsgHeaderPtr->cmsg_level);
        }
        else if (cmsgHeaderPtr->cmsg_type == SCM_RIGHTS)
        {
            // We received at least one file descriptor.

            int fd = ExtractFileDescriptor(cmsgHeaderPtr);

            if (NULL == fdPtr)
            {
                LE_WARN("Discarding received file descriptor.");
                fd_Close(fd);
            }
            else if (*fdPtr != -1)
            {
                LE_WARN("Discarding an extra list of file descriptors.");
                fd_Close(fd);
            }
            else
            {
                *fdPtr = fd;
            }
        }
        else if (cmsgHeaderPtr->cmsg_type == SCM_CREDENTIALS)
        {
            // We received credentials.
            if (NULL == credPtr)
            {
                LE_WARN("Discarding received credentials.");
            }
            else if (0 != credPtr->pid)
            {
                LE_WARN("Discarding duplicate set of credentials.");
            }
            else
            {
                memcpy(credPtr, (struct ucred*)CMSG_DATA(cmsgHeaderPtr), sizeof(*credPtr));
                LE_DEBUG("Received credentials (pid = %d, uid = %d, gid = %d).",
                         credPtr->pid,
                         credPtr->uid,
                         credPtr->gid);
            }
        }
        else
        {
            LE_ERROR("Received unexpected ancillary data message type %d.",
                     cmsgHeaderPtr->cmsg_type);
        }

        cmsgHeaderPtr = CMSG_NXTHDR(msgHeaderPtr, cmsgHeaderPtr);
    }
    while (NULL != cmsgHeaderPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a named sequenced-packet Unix domain socket. This binds the socket to a file system path.
 * A "socket" type pseudo file will appear at that location in the file system.
 *
 * @return
 * - The file descriptor (a number > 0) of the socket, if successful.
 * - LE_NOT_PERMITTED if the calling process does not have permission to create a socket at
 *      that location in the file system.
 * - LE_DUPLICATE if something already exists at that location in the file system.
 * - LE_FAULT if failed for some other reason (check your logs).
 */
//--------------------------------------------------------------------------------------------------
int unixSocket_CreateSeqPacketNamed
(
    const char* pathStr ///< [in] File system path to bind to the socket.
)
//--------------------------------------------------------------------------------------------------
{
    int fd;
    le_result_t result;
    struct sockaddr_un socketAddr;

    // Create the socket.
    fd = unixSocket_CreateSeqPacketUnnamed();
    if (fd < 0)
    {
        return LE_FAULT;
    }

    // Bind the socket to the file system path given.
    memset(&socketAddr, 0, sizeof(socketAddr));
    socketAddr.sun_family = AF_UNIX;
    result = le_utf8_Copy(socketAddr.sun_path, pathStr, sizeof(socketAddr.sun_path), NULL);
    if (result != LE_OK)
    {
        // Close the fd
        fd_Close(fd);
        LE_CRIT("Socket path '%s' too long.", pathStr);
        return LE_FAULT;
    }

    if (bind(fd, (struct sockaddr*)(&socketAddr), SUN_LEN(&socketAddr)) != 0)
    {
        switch (errno)
        {
            case EACCES:
                result = LE_NOT_PERMITTED;
                break;
            case EADDRINUSE:
                result = LE_DUPLICATE;
                break;
            default:
                LE_ERROR("bind failed on address '%s'. Errno = %d (%m). See 'man 7 unix'.",
                         pathStr,
                         errno);
                result = LE_FAULT;
                break;
        }

        //Close the fd
        fd_Close(fd);

        return result;
    }

    return fd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates an unnamed sequenced-packet Unix domain socket.
 *
 * @return
 * - The file descriptor (a number > 0) of the socket, if successful.
 * - LE_NOT_PERMITTED if the calling process does not have permission to create the socket.
 * - LE_FAULT if failed for some other reason (check your logs).
 */
//--------------------------------------------------------------------------------------------------
int unixSocket_CreateSeqPacketUnnamed
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    int fd;

    // Create the socket.
    fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (fd == -1)
    {
        LE_ERROR("socket(AF_UNIX, SOCK_SEQPACKET, 0) failed. Errno = %d (%m). See 'man 7 unix'.",
                 errno);
        return LE_FAULT;
    }

    return fd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a pair of unnamed Unix domain sequenced-packet sockets that are connected to each other.
 *
 * @return
 * - LE_OK if successful.
 * - LE_NOT_PERMITTED if the calling process does not have permission to create a pair of sockets.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t unixSocket_CreateSeqPacketPair
(
    int* socketFd1Ptr,  ///< [out] Ptr to where the fd for one of the sockets will be stored.
    int* socketFd2Ptr   ///< [out] Ptr to where the other socket's fd will be stored.
)
//--------------------------------------------------------------------------------------------------
{
    int fds[2];
    int result = socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fds);

    if (result != 0)
    {
        LE_CRIT("socketpair() failed with errno %d (%m).", errno);
        return LE_NOT_PERMITTED;
    }

    *socketFd1Ptr = fds[0];
    *socketFd2Ptr = fds[1];

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Connect a local socket to another named socket.
 *
 * @return
 * - LE_OK if successful.
 * - LE_WOULD_BLOCK if socket is non-blocking and could not be immediately connected.
 * - LE_NOT_FOUND if the path provided does not refer to a listening socket.
 * - LE_NOT_PERMITTED if the calling process does not have permission to connect to that socket.
 * - LE_FAULT if failed for some other reason (check your logs).
 *
 * @note In non-blocking mode, if LE_WOULD_BLOCK is returned, monitor the socket fd for
 *       writeability, and when it becomes writeable, call unixSocket_GetErrorState() to
 *       find out if the connection succeeded or failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t unixSocket_Connect
(
    int         fd,     ///< [in] Local socket file descriptor.
    const char* pathStr ///< [in] File system path of the named socket to connect to.
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;
    int connectResult;
    struct sockaddr_un socketAddr;

    memset(&socketAddr, 0, sizeof(socketAddr));
    socketAddr.sun_family = AF_UNIX;
    result = le_utf8_Copy(socketAddr.sun_path, pathStr, sizeof(socketAddr.sun_path), NULL);
    if (result != LE_OK)
    {
        LE_CRIT("Socket path '%s' too long.", pathStr);
        return LE_FAULT;
    }

    do
    {
        connectResult = connect(fd, &socketAddr, sizeof(socketAddr));
    }
    while ((connectResult == -1) && (errno == EINTR));

    if (connectResult != 0)
    {
        switch (errno)
        {
            case EACCES:
                return LE_NOT_PERMITTED;

            case ECONNREFUSED:
                return LE_NOT_FOUND;

            case EINPROGRESS:
                return LE_WOULD_BLOCK;

            default:
                LE_ERROR("Connect failed with errno %d (%m).", errno);
                return LE_FAULT;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends through a connected Unix domain socket a message containing any combination of:
 * - a data payload
 * - a file descriptor
 * - authenticated credentials
 *
 * All of the above are optional, with the following exceptions:
 * - it doesn't make sense to omit everything
 * - when using stream sockets, at least one byte of data payload must be sent.
 *
 * For example, if data and credentials are to be sent, but not file descriptors, then fdToSend
 * could be set to -1.
 *
 * @note When file descriptors are sent, they are duplicated in the receiving process's address
 * space, as if they were created using dup().  This means that they are left open in the sending
 * process and must be closed by the sender if the sender doesn't need to continue using them.
 *
 * @return
 * - LE_OK if successful
 * - LE_COMM_ERROR if the localSocketFd is not connected.
 * - LE_FAULT if failed for some other reason (check your logs).
 * - LE_NO_MEMORY if the send socket is set to non-blocking and it doesn't have enough buffer
 *                  space to send right now. Wait for the "writeable" event on the file descriptor.
 *
 * @warning DO NOT SEND DIRECTORY FILE DESCRIPTORS.  That can be exploited to break out of chroot()
 *          jails.
 */
//--------------------------------------------------------------------------------------------------
le_result_t unixSocket_SendMsg
(
    int localSocketFd,          ///< [IN] fd of the local socket that will be used to send.
    void* dataPtr,              ///< [IN] Pointer to the data payload to be sent (NULL if none).
    size_t dataSize,            ///< [IN] Number of bytes of data payload to be sent.
    int fdToSend,               ///< [IN] The file descriptor to be sent (-1 if no FD to send).
    bool sendCredentials        ///< [IN] true = Send credentials.  false = Don't send credentials.
)
//--------------------------------------------------------------------------------------------------
{
    char cmsgBuffer[CMSG_BUFF_SIZE];        // Ancillary data (control message) buffer.
    struct cmsghdr* cmsgHeaderPtr = NULL;   // Ptr to an ancillary data header inside our buffer.

    struct msghdr msgHeader = {0};  // Message "header" structure required by sendmsg().
    struct iovec ioVector;          // I/O vector structure used when data is being sent.

    size_t cmsgLenSum = 0;  // Sum of the cmsg_len fields of all ancillary data message headers.

    // First, fill the message header with zeros.
    memset(&msgHeader, 0, sizeof(msgHeader));

    // If we are sending a data payload,
    if ((dataPtr != NULL) && (dataSize > 0))
    {
        ioVector.iov_base = dataPtr;
        ioVector.iov_len = dataSize;
        msgHeader.msg_iov = &ioVector;
        msgHeader.msg_iovlen = 1;
    }

    // If we are sending ancillary data,
    if ((fdToSend >= 0) || sendCredentials)
    {
        // Store the address and size of our control message buffer in the message header.
        // NOTE: This is necessary for the CMSG_FIRSTHDR and CMSG_NXTHDR macros to work.
        msgHeader.msg_control = cmsgBuffer;
        msgHeader.msg_controllen = sizeof(cmsgBuffer);

        // Get a pointer to the first control message header inside our control message buffer.
        cmsgHeaderPtr = CMSG_FIRSTHDR(&msgHeader);
    }

    // If we are sending a file descriptor,
    if (fdToSend >= 0)
    {
        // Fill in the file descriptor's control message header.
        // (It's a "send rights to access an fd" control message, which is a socket-level message.)
        cmsgHeaderPtr->cmsg_level = SOL_SOCKET;
        cmsgHeaderPtr->cmsg_type = SCM_RIGHTS;
        cmsgHeaderPtr->cmsg_len = CMSG_LEN(sizeof(int)); // Payload is a single file descriptor.

        // Store the file descriptor in the control message payload.
        int* fdPtr = (int *)CMSG_DATA(cmsgHeaderPtr);
        *fdPtr = fdToSend;

        // Update the message header's control message length to be the actual total length of
        // all the control messages.
        cmsgLenSum = cmsgHeaderPtr->cmsg_len;

        LE_DEBUG("Sending fd %d.", fdToSend);

        // Get a pointer to the next control message header inside our control message buffer
        // for the credentials to be stored in, if needed.
        cmsgHeaderPtr = CMSG_NXTHDR(&msgHeader, cmsgHeaderPtr);
    }

    // If we are sending process credentials,
    if (sendCredentials)
    {
        // Fill in the credentials' control message header.
        // (It's a "send credentials" control message, which is a socket-level message.)
        cmsgHeaderPtr->cmsg_level = SOL_SOCKET;
        cmsgHeaderPtr->cmsg_type = SCM_CREDENTIALS;
        cmsgHeaderPtr->cmsg_len = CMSG_LEN(sizeof(struct ucred)); // Payload is a ucred structure.

        // Store the credentials in the control message payload.
        // NOTE: These must match the actual credentials of this process.
        struct ucred* credPtr = (struct ucred*)CMSG_DATA(cmsgHeaderPtr);
        credPtr->pid = getpid();
        credPtr->uid = getuid();
        credPtr->gid = getgid();

        // Update the message header's control message length to be the actual total length of
        // all the control messages.
        cmsgLenSum += cmsgHeaderPtr->cmsg_len;
    }

    // Update the message header with the actual amount of ancillary data message space.
    msgHeader.msg_controllen = cmsgLenSum;

    // Now send the message (retry if interrupted by a signal).
    ssize_t bytesSent;
    do
    {
        bytesSent = sendmsg(localSocketFd, &msgHeader, 0);
    }
    while ((bytesSent < 0) && (errno == EINTR));

    if (bytesSent < 0)
    {
        switch (errno)
        {
            case EAGAIN:  // Same as EWOULDBLOCK
                return LE_NO_MEMORY;

            case ENOTCONN:
            case ECONNRESET:
            case EPIPE:
                LE_WARN("sendmsg() failed with errno %d (%m).", errno);
                return LE_COMM_ERROR;

            default:
                LE_ERROR("sendmsg() failed with errno %d (%m).", errno);
                return LE_FAULT;
        }
    }

    if (bytesSent < dataSize)
    {
        LE_ERROR("The last %zu data bytes (of %zu total) were discarded by sendmsg()!",
                 dataSize - bytesSent,
                 dataSize);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends a message containing only data through a connected Unix domain datagram or
 * sequenced-packet socket.
 *
 * @note It is recommended that write() be used for stream sockets instead.
 *
 * @return
 * - LE_OK if successful
 * - LE_COMM_ERROR if the localSocketFd is not connected.
 * - LE_FAULT if failed for some other reason (check your logs).
 * - LE_NO_MEMORY if the send socket is set to non-blocking and it doesn't have enough buffer
 *                  space to send right now. Wait for the "writeable" event on the file descriptor.
 */
//--------------------------------------------------------------------------------------------------
le_result_t unixSocket_SendDataMsg
(
    int localSocketFd,          ///< [IN] fd of the local socket that will be used to send.
    void* dataPtr,              ///< [IN] Pointer to the data payload to be sent (NULL if none).
    size_t dataSize             ///< [IN] Number of bytes of data payload to be sent.
)
//--------------------------------------------------------------------------------------------------
{
    return unixSocket_SendMsg(localSocketFd,
                              dataPtr,
                              dataSize,
                              -1,          // fdToSend
                              false);      // sendCredentials
}


//--------------------------------------------------------------------------------------------------
/**
 * Receives through a connected Unix domain socket a message containing any combination of
 * - a data payload
 * - a file descriptor
 * - authenticated credentials
 *
 * NULL pointers can be passed in for any of the above that are not needed.  For example, if
 * data and credentials are expected, but not a file descriptor, then fdPtr could be set to NULL.
 *
 * @note    Authentication of credentials must be enabled using unixSocket_EnableAuthentication()
 *          before credentials can be received.
 *
 * @return
 * - LE_OK if successful
 * - LE_NO_MEMORY if more data was received than could fit in the buffer provided.
 * - LE_WOULD_BLOCK if the socket is set non-blocking and there is nothing to be received.
 * - LE_CLOSED if the connection closed.
 * - LE_FAULT if failed for some other reason (check your logs).
 *
 * @warning If LE_WOULD_BLOCK is returned when using a stream socket, some data may have been read.
 *          Check the returned data size to find out how much.  Furthermore, if LE_NO_MEMORY is
 *          returned for a datagram (or sequenced-packet?) socket, the remainder of the message
 *          that couldn't fit into the receive buffer will have been lost.
 */
//--------------------------------------------------------------------------------------------------
le_result_t unixSocket_ReceiveMsg
(
    int localSocketFd,      ///< [IN] fd of local socket that will be used to receive the message.
    void* dataBuffPtr,      ///< [OUT] Pointer to where any received data payload will be put.
    size_t* dataSizePtr,    ///< [IN+OUT] Ptr to the number of bytes that can fit in the array
                            ///     pointed to by dataBuffPtr.  This will be updated to the number
                            ///     of bytes of data received.
    int* fdPtr,             ///< [OUT] Pointer to where the received file descriptor will be put.
                            ///        (-1 will be stored here if no fd was received.)
    struct ucred* credPtr   ///< [OUT] Pointer to where received credentials will be stored.
                            ///        (NOTE: PID is set to zero if no credentials received.)
)
//--------------------------------------------------------------------------------------------------
{
    char cmsgBuffer[CMSG_BUFF_SIZE];    // Ancillary data buffer.

    struct msghdr msgHeader;    // Message "header" structure required by recvmsg().
    struct iovec ioVector;      // I/O vector structure used when data is being received.

    // First, fill the message header with zeros.
    memset(&msgHeader, 0, sizeof(msgHeader));

    // Store the address and size of our control message buffer in the message header.
    msgHeader.msg_control = cmsgBuffer;
    msgHeader.msg_controllen = sizeof(cmsgBuffer);

    // If we are receiving a data payload,
    if (dataBuffPtr != NULL)
    {
        LE_ASSERT(dataSizePtr != NULL);

        if (*dataSizePtr > 0)
        {
            // Set up the I/O vector to point to the data buffer.
            ioVector.iov_base = dataBuffPtr;
            ioVector.iov_len = *dataSizePtr;

            // Attach the I/O vector to the "header" structure so recvmsg() can find it.
            msgHeader.msg_iov = &ioVector;
            msgHeader.msg_iovlen = 1;

            // Zero the output parameter in case there's an error.
            *dataSizePtr = 0;
        }
    }

    // If we are trying to receive a file descriptor, set the output param to -1 in case we don't.
    if (fdPtr != NULL)
    {
        *fdPtr = -1;
    }

    // If we are trying to receive process credentials, set the output param's PID to 0 in case we
    // don't.
    if (credPtr != NULL)
    {
        credPtr->pid = 0;
    }

    // Keep trying to receive until we don't get interrupted by a signal.
    ssize_t bytesReceived;
    do
    {
        bytesReceived = recvmsg(localSocketFd, &msgHeader, 0);
    }
    while ((bytesReceived < 0) && (errno == EINTR));

    // If we failed, process the error and return.
    if (bytesReceived < 0)
    {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
        {
            return LE_WOULD_BLOCK;
        }
        else if (errno == ECONNRESET)
        {
            return LE_CLOSED;
        }
        else
        {
            LE_ERROR("recvmsg() failed with errno %d (%m).", errno);
            return LE_FAULT;
        }
    }

    // If we received any ancillary data messages (control messages), extract what we want
    // from them.
    if (msgHeader.msg_controllen > 0)
    {
        ExtractAncillaryData(&msgHeader, fdPtr, credPtr);
    }

    // Check if ancillary data was discarded.
    if ((msgHeader.msg_flags & MSG_CTRUNC) != 0)
    {
        LE_WARN("Ancillary data was discarded because it couldn't fit in our buffer.");
        if (bytesReceived == 0)
        {
            return LE_FAULT;
        }
    }
    // If we didn't receive any ancillary data, and recvmsg() still returned zero,
    // then the socket must have closed.
    else if (msgHeader.msg_controllen == 0 && bytesReceived == 0)
    {
        return LE_CLOSED;
    }


    // If we tried to receive data,
    if (dataBuffPtr != NULL)
    {
        // Set the received data count output parameter.
        *dataSizePtr = bytesReceived;

        // Check to see if the data message fit into the buffer provided by the caller.
        if ((msgHeader.msg_flags & MSG_TRUNC) != 0)
        {
            return LE_NO_MEMORY;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Receives a message containing only data payload through a connected Unix domain datagram or
 * sequenced-packet socket.
 *
 * @note
 * - Any ancillary data (file descriptors or credentials) associated with the message will
 *          be discarded.
 * - It is recommended that read() be used for stream sockets.
 *
 * @return
 * - LE_OK if successful
 * - LE_NO_MEMORY if more data was sent than could fit in the buffer provided.
 * - LE_WOULD_BLOCK if the socket is set non-blocking and there is nothing to be received.
 * - LE_CLOSED if the connection closed.
 * - LE_FAULT if failed for some other reason (check your logs).
 *
 * @warning If LE_NO_MEMORY is returned, the remainder of the message that couldn't fit into
 *          the receive buffer will have been lost.
 */
//--------------------------------------------------------------------------------------------------
le_result_t unixSocket_ReceiveDataMsg
(
    int localSocketFd,      ///< [IN] fd of local socket that will be used to receive the message.
    void* dataBuffPtr,      ///< [OUT] Pointer to where any received data payload will be put.
    size_t* dataSizePtr     ///< [IN+OUT] Ptr to the number of bytes that can fit in the array
                            ///     pointed to by dataBuffPtr.  This will be updated to the number
                            ///     of bytes of data received.
)
//--------------------------------------------------------------------------------------------------
{
    return unixSocket_ReceiveMsg(localSocketFd,
                                 dataBuffPtr,
                                 dataSizePtr,
                                 NULL,          // fdPtr
                                 NULL);         // credPtr
}



//--------------------------------------------------------------------------------------------------
/**
 * Fetches the socket error state code (SO_ERROR).
 *
 * @return The SO_ERROR value, or EBADE if failed.
 */
//--------------------------------------------------------------------------------------------------
int unixSocket_GetErrorState
(
    int localSocketFd       ///< [IN] fd of local socket that will be used to receive the message.
)
//--------------------------------------------------------------------------------------------------
{
    int errCode;
    socklen_t errCodeSize = sizeof(errCode);

    if (getsockopt(localSocketFd, SOL_SOCKET, SO_ERROR, &errCode, &errCodeSize) == -1)
    {
        LE_ERROR("Failed to read socket option SOL_ERROR (%m) for fd %d.", localSocketFd);
        return EFAULT;
    }

    return errCode;
}
