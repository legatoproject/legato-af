/**
 * @page c_unixSockets Unix Domain Sockets API
 *
 * @ref unixSocket.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * @section toc Table of Contents
 *
 *  - @ref c_unixSocketsIntro
 *  - @ref c_unixSocketsCreatingSingle
 *  - @ref c_unixSocketsCreatingPair
 *  - @ref c_unixSocketsConnecting
 *  - @ref c_unixSocketsSendingAndReceiving
 *  - @ref c_unixSocketsGettingCredentialsDirect
 *  - @ref c_unixSocketsDeleting
 *
 * <HR>
 *
 * @section c_unixSocketsIntro Introduction
 *
 * Unix domain sockets are a powerful and relatively efficient means of communicating between
 * processes in Unix systems.  However, the use of Unix domain sockets is not without pitfalls,
 * some of which can result in security holes or race conditions.  Therefore, Unix domain sockets
 * should be avoided, unless fully understood and carefully managed.  Use the Legato messaging
 * system(s) instead.
 *
 * Unix Domain Sockets can be @b named or @b unnamed.
 *
 * A @b named socket appears in the file system as a "socket" type file, and is addressed
 * using a file system path.
 *
 * An @b unnamed socket does not appear in the file system and essentially has no address.
 * It won't receive anything unless it is @b connected to another socket.
 *
 * @note In Linux, Unix Domain sockets can also be "abstract", where an abstract socket has a name
 * but does not appear in the file system.  However, this is not portable to other Unix platforms
 * (like BSD), and can be a security hole, since the abstract namespace does not support file system
 * permissions and can be accessed from inside chroot() jails.
 *
 * Unix Domain Sockets can also be @b datagram-, @b stream-, or <b> sequenced-packet- </b> oriented,
 * just like UDP, TCP, and SCTP sockets, respectively.  Unlike UDP datagram sockets, however,
 * Unix Domain datagram sockets are guaranteed to deliver every datagram in the order in which
 * they were sent.
 *
 * An added bonus of Unix Domain sockets is that they can be used to <b> pass file descriptors </b>
 * between processes.  Furthermore, they allow one process to <b> check the credentials </b>
 * (Process ID, User ID, and Group ID) of another process on the other end of a connection.
 * The OS checks the validity of the credentials, so the recipient can be certain that they are
 * valid.
 *
 * @section c_unixSocketsCreatingSingle Creating a Single Socket
 *
 * Use unixSocket_CreateDatagramNamed() to create a single, named datagram socket whose address
 * is a given file system path.
 *
 * @code
 * int socketFd;
 * le_result_t result;
 *
 * result = unixSocket_CreateDatagramNamed("/tmp/my_app/server_socket");
 *
 * LE_FATAL_IF(result != LE_OK, "Failed to create socket!");
 * @endcode
 *
 * This seems simple enough on the surface, but beware of pitfalls here.
 *
 * If the socket name corresponds to a location in a non-volatile file system
 * (e.g., a flash file system), then the socket will consume a small amount of space in
 * that file system.  Furthermore, creating and deleting sockets in a non-volatile
 * file system will wear the backing store (e.g., the flash memory device), thereby
 * shortening its life.  It is highly recommended that sockets be placed in RAM-based
 * file systems (e.g., tmpfs) to avoid these issues.
 *
 * Of course, the location you choose must be managed very carefully.  In particular, the only
 * way to portably guarantee that unauthorized processes don't hijack your socket address is to
 * put it in a directory that has its permissions set such that unauthorized processes can't
 * search (execute permission) or write that directory.
 *
 * Also beware that it is possible to leave socket bindings lying around in the file system after a
 * process dies.  This will consume system resources and could lead to a memory leak if
 * successive incarnations of the process use different socket names.  Furthermore, if
 * every incarnation of the process uses the same socket name, then socket creation will fail
 * if an old socket with the same name is still there.  As a result, its a good idea to remove the
 * socket from the filesystem using unlink() when your process dies.  However, even if your
 * process has clean-up code that unlinks a socket from the filesystem when it terminates, your
 * process may not always get the opportunity to run that code (e.g., if it receives SIG_KILL).
 * Therefore, your system design must ensure that your sockets get cleaned up somehow, even if
 * your process doesn't terminate gracefully.
 *
 * To create a single, named stream socket, use unixSocket_CreateStreamNamed().
 *
 * To create a single, named sequenced-packet socket, use unixSocket_CreateSeqPacketNamed().
 *
 * To create a single, unnamed datagram socket, use unixSocket_CreateDatagramUnnamed().
 *
 * To create a single, unnamed stream socket, use unixSocket_CreateStreamUnnamed().
 *
 * To create a single, unnamed sequenced-packet socket, use unixSocket_CreateSeqPacketUnnamed().
 *
 * @todo Implement additional functions, as needed.
 *
 * @section c_unixSocketsCreatingPair Creating a Pair of Connected Sockets
 *
 * Use unixSocket_CreateDatagramPair() to create a pair of unnamed datagram sockets that
 * are connected to each other.
 *
 * @code
 * int socketFd1;
 * int socketFd2;
 * le_result_t result;
 *
 * result = unixSocket_CreateDatagramPair(&socketFd1, &socketFd2);
 *
 * LE_FATAL_IF(result != LE_OK, "Failed to create socket pair!");
 * @endcode
 *
 * To create a pair of unnamed, connected stream sockets, use unixSocket_CreateStreamPair().
 *
 * To create a pair of unnamed, connected sequenced-packet sockets, use
 * unixSocket_CreateSeqPacketPair().
 *
 *
 * @section c_unixSocketsConnecting Listening and Connecting
 *
 * Unix domain sockets work the same as Internet network sockets, with respect to listening,
 * accepting, and connecting.  Refer to the man pages for listen(), accept() and connect()
 * for more information.
 *
 * When a socket is a listening socket, the socket will appear readable to a File Descriptor Monitor
 * (see @ref c_fdMonitor) when a connection is waiting to be accepted.  That is, a connection
 * handler can be registered by registering a handler for the @c POLLIN event type.
 *
 *
 * @section c_unixSocketsSendingAndReceiving Sending and Receiving
 *
 * Sending data on a socket can be done using the standard POSIX APIs for sending (write(),
 * writev(), send(), sendto(), sendmsg(), etc.).
 *
 * Receiving data from a socket can be done using the standard POSIX APIs for receiving (read(),
 * readv(), recv(), recvfrom(), recvmsg(), etc.).
 *
 * Likewise, the standard POSIX select() and poll() functions (and variants of those) can be
 * used to know when it is "clear-to-send" to a socket or when there is data available to be
 * read from a socket.  However, it is best when working in the Legato framework to use the
 * file descriptor monitoring features of the @ref c_eventLoop for this.
 *
 * The standard way to send file descriptors and authenticated credentials (PID, UID, GID)
 * through a Unix Domain socket is sendmsg() and recvmsg().  Those are particularly nasty APIs.
 * To make life easier, the Legato framework provides its own sending and receiving functions
 * that (in addition to providing normal data message delivery) allow a file descriptor and/or
 * authenticated process credentials to be sent between processes.
 *
 * - unixSocket_SendMsg() sends a message containing any combination of normal data, a
 *   file descriptor, and authenticated credentials.
 *
 * - unixSocket_ReceiveMsg() receives a message containing any combination of normal
 *   data, a file descriptor, and authenticated credentials.
 *
 * When file descriptors are sent, they are duplicated in the receiving process as if they had
 * been created using the POSIX dup() function.  This means that they remain open in the sending
 * process and must be closed by the sending process when it doesn't need them anymore.
 *
 * Authentication of credentials is done at the receiving socket, and this feature is normally
 * turned off by default (presumably to save overhead in the common case where authentication is
 * not needed).  To enable the authentication of credentials, unixSocket_EnableAuthentication()
 * must be called on the receiving socket.
 *
 * @code
 * static le_result_t PassFdToConnectedPeer(int sendSocketFd, int fdToSend)
 * {
 *     // NOTE: My socket is connected, so I don't need to specify a destination address.
 *     return unixSocket_SendMsg(sendSocketFd, NULL, NULL, 0, fdToSend, false);
 *
 *     close(fdToSend); // I don't need the fd locally anymore, so close my copy of it.
 * }
 * @endcode
 *
 * @warning When sending ancillary data (fds or credentials) over a stream socket, at least one
 *          byte of data must accompany the ancillary data.  This is a limitation of the
 *          underlying OS.  Datagram and sequenced-packet sockets don't have this limitation.
 *
 * A regular @ref c_eventLoop file descriptor monitor can be used to detect the arrival of
 * an ancillary data message. The indication that is provided when a message containing ancillary
 * data arrives is indistinguishable from the indication provided when normal data arrives.
 *
 * @code
 * static void SocketReadyForReadingHandler
 * (
 *     int socketFd
 * )
 * {
 *     char dataBuff[MAX_MESSAGE_DATA_BYTES];
 *     size_t numDataBytes = sizeof(dataBuff);
 *     int fd;
 *     struct ucred credentials;
 *
 *     result = unixSocket_ReceiveMsg(socketFd,
 *                                    NULL,    // addrBuffPtr - I don't need the sender's address.
 *                                    0,       // addrBuffSize
 *                                    dataBuff,
 *                                    &numDataBytes,
 *                                    &fd,
 *                                    &credentials);
 *     if (result != LE_OK)
 *     {
 *         LE_ERROR("Socket receive failed. Error = %d (%s).", result, LE_RESULT_TXT(result));
 *     }
 *     else
 *     {
 *         if (numDataBytes > 0)
 *         {
 *             // Got some data!
 *             ...
 *         }
 *
 *         if (fd > -1)
 *         {
 *             // Got a file descriptor!
 *             ...
 *         }
 *
 *         if (credentials.pid != 0)
 *         {
 *             // Got some credentials!
 *             ...
 *         }
 *     }
 * }
 * @endcode
 *
 *
 * @section c_unixSocketsGettingCredentialsDirect Getting Credentials Directly from a Connected Socket
 *
 * Although it is possible to explicitly send credentials over Unix domain sockets, it is often
 * not necessary to do so.  Instead, you can often just use getsockopt() to fetch credentials
 * directly from a local connected socket.
 *
 * If you have a connected Unix domain stream or sequenced-packet socket, you can use getsockopt()
 * to fetch the credentials of <b>the process that called bind()</b> on the socket at the other
 * end of the connection.
 *
 * If you have any kind of connected Unix domain socket that was created using socketpair() or
 * one of the helper functions built on top of socketpair (unixSocket_CreateDatagramPair(),
 * unixSocket_CreateStreamPair(), or unixSocket_CreateSeqPacketPair()), you can also use
 * getsockopt() to fetch the credentials of <b>the process that created that socket</b>.
 *
 * @code
 * struct ucred credentials;
 * socklen_t size = sizeof(struct ucred);
 *
 * if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &credentials, &size) != 0)
 * {
 *     LE_FATAL("Failed to obtain credentials from socket.  Errno = %d (%m)", errno);
 * }
 *
 * LE_INFO("Credentials of peer process:  pid = %d;  uid = %d;  gid = %d.",
 *         credentials.pid,
 *         credentials.uid,
 *         credentials.gid);
 * @endcode
 *
 *
 * @section c_unixSocketsDeleting Deleting a Socket
 *
 * The standard POSIX close() function can be used to delete a socket.  However, is recommended
 * that the function fd_Close() be used.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

/** @file unixSocket.h
 *
 * Legato @ref c_unixSockets include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_UNIX_SOCKET_INCLUDE_GUARD
#define LEGATO_UNIX_SOCKET_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Creates a named datagram Unix domain socket.  This binds the socket to a file system path.
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
int unixSocket_CreateDatagramNamed
(
    const char* pathStr ///< [IN] File system path to bind to the socket.
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a pair of unnamed Unix domain datagram sockets that are connected to each other.
 *
 * @return
 * - LE_NOT_PERMITTED if the calling process does not have permission to create a pair of sockets.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t unixSocket_CreateDatagramPair
(
    int* socketFd1Ptr,  ///< [OUT] Ptr to where the fd for one of the sockets will be stored.
    int* socketFd2Ptr   ///< [OUT] Ptr to where the other socket's fd will be stored.
);


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
    const char* pathStr ///< [IN] File system path to bind to the socket.
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a pair of unnamed Unix domain sequenced-packet sockets that are connected to each other.
 *
 * @return
 * - LE_OK if successful.
 * - LE_NOT_PERMITTED if the calling process does not have permission to create a pair of sockets.
 * - LE_FAULT if failed for some other reason (check your logs).
 */
//--------------------------------------------------------------------------------------------------
le_result_t unixSocket_CreateSeqPacketPair
(
    int* socketFd1Ptr,  ///< [OUT] Ptr to where the fd for one of the sockets will be stored.
    int* socketFd2Ptr   ///< [OUT] Ptr to where the other socket's fd will be stored.
);


//--------------------------------------------------------------------------------------------------
/**
 * Connect a local socket to another named socket.
 *
 * @return
 * - LE_OK if successful.
 * - LE_NOT_PERMITTED if the calling process does not have permission to connect to that socket.
 * - LE_FAULT if failed for some other reason (check your logs).
 */
//--------------------------------------------------------------------------------------------------
le_result_t unixSocket_Connect
(
    int         fd,     ///< [IN] Local socket file descriptor.
    const char* pathStr ///< [IN] File system path of the named socket to connect to.
);


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
);


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
);


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
);


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
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Enables authentication of credentials on a socket.  This must be called for a socket before
 * that socket can receive credentials.
 *
 * @return
 * - LE_OK if successful.
 * - LE_BAD_PARAMETER if the parameter is not a valid file descriptor.
 * - LE_NOT_PERMITTED if the the file descriptor is not a socket file descriptor.
 */
//--------------------------------------------------------------------------------------------------
le_result_t unixSocket_EnableAuthentication
(
    int fd              ///< [IN] fd of the socket that will be used to receive credentials.
);



#endif // LEGATO_UNIX_SOCKET_INCLUDE_GUARD
