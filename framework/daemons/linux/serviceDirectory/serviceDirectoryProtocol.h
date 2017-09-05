/**
 * @page serviceDirectoryProtocol Legato Service Directory Protocol
 *
 * @ref serviceDirectoryProtocol_Intro <br>
 * @ref serviceDirectoryProtocol_SocketsAndCredentials <br>
 * @ref serviceDirectoryProtocol_Servers <br>
 * @ref serviceDirectoryProtocol_Clients <br>
 * @ref serviceDirectoryProtocol_Packing
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
 * @section serviceDirectoryProtocol_SocketsAndCredentials Sockets and Credentials
 *
 * The Service Directory has two Unix domain sockets, bound to well-known file system paths.
 * Servers connect to one of these sockets when they need to provide a service to other processes.
 * Clients connect to the other one when they need to open a service offered by another process.
 *
 * When a client or server connects, the Service Directory gets a new socket that it can use to
 * communicate with that remote process.  Also, because it is a SOCK_SEQPACKET connection, it
 * can get the credentials (uid, gid, and pid) of the connected process using getsockopt() with
 * the SO_PEERCRED option.  These credentials are authenticated by the OS kernel, so the
 * Service Directory can be assured that they have not been forged when using them to enforce
 * access control restrictions.
 *
 * @section serviceDirectoryProtocol_Servers Server-to-Directory Communication
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
 * @note This implies a pair of connected sockets per session.
 *
 * When a server wants to stop offering a service, it simply closes its connection to the Service
 * Directory.
 *
 * @note The server socket is a named socket, rather than an abstract socket because this allows
 *       file system permissions to be used to prevent DoS attacks on this socket.
 *
 * @section serviceDirectoryProtocol_Clients Client-to-Directory Communication
 *
 * When a client wants to open a session with a service, it opens a socket and connects it to
 * the Service Directory's client connection socket.  The client then sends in the name of the
 * interface that it wants to connect and information about the protocol it intends to use to
 * communicate with that service.
 *
 * If the client's interface is bound to a service and that service is advertised by its server,
 * then the Service Directory sends the file descriptor for the client connection over to the server
 * using the server connection (see @ref serviceDirectoryProtocol_Servers) and closes its
 * file descriptor for the client connection, thereby taking the Service Directory out of the
 * loop for IPC between that client and that server.  The client should then receive a welcome
 * message (LE_OK) from the server over that connection and switch to using the protocol that
 * it requested for that service.
 *
 * If the client interface is bound to a service, but the service does not yet exist, the
 * client can (and usually does) request that the Service Directory hold onto the client connection
 * until the server connects and advertises the service.  If the client does not ask to wait
 * for the server, then the Service Directory will immediately respond with an LE_UNAVAILABLE
 * result code message and close the connection to the client.
 *
 * If the client interface is not bound to a service, then the client can (and usually does)
 * request that the Service Directory hold onto the client connection until a binding is created
 * for that client interface.  If the client does not ask to wait then the Service Directory will
 * immediately respond with an LE_NOT_PERMITTED result code message and close the connection to
 * the client.
 *
 * If the client misbehaves according to the protocol rules, the Service Directory will send
 * LE_FAULT to the client and drop its connection.
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
 * Copyright (C) Sierra Wireless Inc.
 */

/** @file serviceDirectoryProtocol.h
 *
 * See @ref serviceDirectoryProtocol
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_SERVICE_DIRECTORY_PROTOCOL_INCLUDE_GUARD
#define LEGATO_SERVICE_DIRECTORY_PROTOCOL_INCLUDE_GUARD

#include "limit.h"
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
/**
 * Interface details.  Both client and server need to send this information to the
 * Service Directory when they connect.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    size_t              maxProtocolMsgSize; ///< Max size of protocol's messages, in bytes.
    char                protocolId[LIMIT_MAX_PROTOCOL_ID_BYTES];      ///< Protocol identifier.
    char                interfaceName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];///< Interface name.
}
svcdir_InterfaceDetails_t;


//--------------------------------------------------------------------------------------------------
/**
 * Open Session request.
 *
 * Messages sent from the client to the Service Directory to request that a session with a server
 * be opened have this structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    svcdir_InterfaceDetails_t  interface;   ///< Details of the client-side interface that
                                            ///  the client wants to connect to a service.

    bool shouldWait;        ///< true = ask the Service Directory to hold onto the request until
                            ///         the binding or advertisement happens if the client
                            ///         interface is not bound or the server is not advertising
                            ///         the service at this time.
                            ///  false = fail immediately if either a binding or advertisement is
                            ///         missing at this time.
}
svcdir_OpenRequest_t;


#endif // LEGATO_SERVICE_DIRECTORY_PROTOCOL_INCLUDE_GUARD
