/**
 * @page serviceDirectoryProtocol Legato Service Directory Protocol
 *
 * @section serviceDirectoryProtocol_Intro Introduction
 *
 * The Legato Service Directory Protocol is the protocol that Legato inter-process communication
 * (IPC) clients and servers use to communicate with the Service Directory.
 *
 * The Service Directory is a daemon process that keeps track of what IPC services are offered
 * by what processes and what clients are connected to them.  It is a key component in the
 * implementation of the @ref c_messaging.
 *
 * @section serviceDirectoryProtocol_Description Protocol Description
 *
 * The Service Directory has two Unix domain sockets, bound to well-known file system paths.
 * Servers connect to one of these sockets when they need to provide a service to other processes.
 * Clients connect to the other one when they need to open a service offered by another process.
 *
 * When a client or server connects, the Service Directory gets a new socket that it can use to
 * communicate with that remote process.  Also, because it is a SOCK_SEQPACKET connection, it
 * can get the credentials (uid, gid, and pid) of the connected process using getsockopt() with
 * the SO_PEERCRED option.  This allows the Service Directory to enforce access control
 * restrictions.  It also makes it possible for the Service Directory to know what needs to
 * be cleaned up when the Supervisor notifies it that a certain process (identified by its pid)
 * has died.
 *
 * @section serviceDirectoryProtocol_Servers Server-Directory Communication
 *
 * When a server wants to offer a service to other processes, it opens a socket and connects it
 * to the Service Directory's server connection socket.  The server then sends in the name of the
 * service that it is offering and information about the protocol that clients will need to use
 * to communicate with that service.
 *
 * @note This implies one pair of connected sockets per service being offered, even if no clients
 *       are connected to the service.
 *
 * When a client connects to a service, the Service Directory will send the server a file descriptor
 * of a Unix Domain SOCK_SEQPACKET socket that is connected to the client.  The server should then
 * send a welcome message (LE_OK) to the client over that connection and switch to using the
 * protocol that it advertised for that service.
 *
 * @note This implies another pair of connected sockets per session.
 *
 * When a server wants to stop offering a service, it simply closes its connection to the Service
 * Directory.
 *
 * @note The server socket is a named socket, rather than an abstract socket because this allows
 *       file system permissions to be used to prevent DoS attacks on this socket.
 *
 * @section serviceDirectoryProtocol_Clients Client-Directory Communication
 *
 * When a client wants to open a session with a service, they open a socket and connect it to
 * the Service Directory's client connection socket.  The client then sends in the name of the
 * service that it wants to use and information about the protocol it intends to use to
 * communicate with that service.
 *
 * If the service exists and the client is authorized to use that service, then the
 * Service Directory sends the file descriptor for the client connection over to the server
 * using the server connection (see @ref serviceDirectoryProtocol_Servers) and closes its
 * file descriptor for the client connection, thereby taking the Service Directory out of the
 * loop for IPC between that client and that server.  The client should then receive a welcome
 * message (LE_OK) from the server over that connection and switch to using the protocol that
 * it requested for that service.
 *
 * If the client is authorized to use the service, but the service does not yet exist, the
 * Service Directory holds onto the client connection until a server connects and advertizes
 * a matching service.
 *
 * If the client is not authorized to use the service, then the Service Directory sends back
 * an LE_NOT_PERMITTED result code to the client and closes the connection.
 *
 * @note The client socket is a named socket, rather than an abstract socket because this allows
 *       file system permissions to be used to prevent DoS attacks on this socket.
 *
 * @section serviceDirectoryProtocol_Packing Byte Ordering and Packing
 *
 * This protocol only goes between processes on the same host, so there's no need to do
 * byte swapping.  Furthermore, all message members are multiples of the processor's
 * natural word size, so there's little risk of structure packing misalignment.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

/** @file serviceDirectoryProtocol.h
 *
 * See @ref serviceDirectoryProtocol
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_SERVICE_DIRECTORY_PROTOCOL_INCLUDE_GUARD
#define LEGATO_SERVICE_DIRECTORY_PROTOCOL_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Name of the Service Directory's "Server Socket", which is a named Unix domain sequenced-packet
 * socket (AF_UNIX, SOCK_SEQPACKET) that servers connect to when they want to offer a service.
 */
//--------------------------------------------------------------------------------------------------
#ifndef LE_SVCDIR_SERVER_SOCKET_NAME
#error LE_SVCDIR_SERVER_SOCKET_NAME not defined.
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Name of the Service Directory's "Client Socket", which is a named Unix domain sequenced-packet
 * socket (AF_UNIX, SOCK_SEQPACKET) that clients connect to when they want to access a service.
 */
//--------------------------------------------------------------------------------------------------
#ifndef LE_SVCDIR_CLIENT_SOCKET_NAME
#error LE_SVCDIR_CLIENT_SOCKET_NAME not defined.
#endif


//--------------------------------------------------------------------------------------------------
/// Maximum size of a service protocol identity string, including the null terminator byte.
//--------------------------------------------------------------------------------------------------
#define LE_SVCDIR_MAX_PROTOCOL_ID_SIZE     128


//--------------------------------------------------------------------------------------------------
/// Maximum size of a service instance name string, including the null terminator byte.
//--------------------------------------------------------------------------------------------------
#define LE_SVCDIR_MAX_SERVICE_NAME_SIZE    128


//--------------------------------------------------------------------------------------------------
/**
 * Service identity.  This structure contains everything that is required to uniquely identify
 * a Legato IPC service.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    size_t              maxProtocolMsgSize; ///< Max size of protocol's messages, in bytes.
    char                protocolId[LE_SVCDIR_MAX_PROTOCOL_ID_SIZE];   ///< Protocol identifier.
    char                serviceName[LE_SVCDIR_MAX_SERVICE_NAME_SIZE]; ///< Service instance name.
}
svcdir_ServiceId_t;


#endif // LEGATO_SERVICE_DIRECTORY_PROTOCOL_INCLUDE_GUARD
