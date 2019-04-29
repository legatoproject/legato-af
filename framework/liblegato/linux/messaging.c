/** @file messaging.c
 *
 * Legato @ref c_messaging implementation.
 *
 * Following is the basic data model for the part of the low-level messaging subsystem
 * that runs in the interface instances such as the client and the server.
 *
 * The interface instances share much of the same code and data structures.  Protocols, Interfaces,
 * Sessions, and Messages are all the same object types on all interface instances.  But, the way
 * they are used and some of of their contents differ among interface instances.
 *
 * IPC is done using unix domain, sequenced-packet sockets.  See the ref c_unixSockets for more
 * information.
 *
 * @verbatim
 *
 *   Protocol List --+--> Protocol ---> Message Pool
 *                           ^
 *                           |
 *   Interface ------+--> Interface --+--> socket (server-side only, connected to Service Directory)
 *   Instances Map           ^        |
 *   (hashmap)               |        +--> Session List
 *                           |                 |
 *                           | +---------------+
 *                           | |
 *                           | v
 *                        Session ---+--> socket
 *                           ^       |
 *                           |       +--> Transmit Queue
 *                           |                 |
 *                        Message <------------+
 *                           ^
 *                           |
 *   Transaction Ref Map ----+
 *   (client-side only)
 *
 * @endverbatim
 *
 * All sockets are put in non-blocking mode and the FD monitoring capabilities of the
 * @ref c_eventLoop are used to register for notification when messages arrive on the IPC sockets
 * and when IPC sockets become clear-to-send.
 *
 * In unix systems, there is a limit to how much socket buffer memory a given socket is allowed
 * to have.  Futhermore, with unix domain sockets, all the socket buffer memory is attributed to
 * the sender, even when it is waiting at the receiver socket for someone to receive it.  This
 * means that it is possible for a receiver to block another socket from being able to
 * send to anyone (even some other third-party socket) simply by not receiving messages sent
 * from that socket.  This can be a very tricky thing to diagnose and can result in deadlocks.
 * To avoid this, a separate socket is used for each session, so that if the other side stops
 * receiving, other sessions are not affected.
 *
 * In the Legato messaging system implementation, if the send socket's buffer space quota becomes
 * exhausted and attempts to send a message return EAGAIN or EWOULDBLOCK, the Message object is
 * placed on a queue for that socket (in the Session object) and the messaging system waits for
 * notification from the Event Loop that the socket has become clear-to-send before trying again.
 *
 * Another potential deadlock occurs when two threads are sending messages to each other.
 * If they both send a lot of messages to each other, they can both get blocked waiting for the
 * other side to receive messages that had been sent earlier, and because they are both blocked,
 * neither is able to receive any messages to unblock the other.  To avoid this, messages are sent
 * using non-blocking system calls.  If the message cannot be sent, it is queued for later
 * transmission when the socket becomes "writeable" again.  This makes use of the normal
 * "writeable" file descriptor event monitoring capabilities of the Event Loop API.
 *
 * Only when a thread calls le_msg_RequestSyncResponse() will the socket be placed in blocking mode.
 * In that case, the thread will block waiting on a receive operation on the socket.
 * If a message arrives while waiting for a response, the waiting thread will wake up and receive
 * that message.  If it is not the message that it was waiting for, then the message is pushed
 * onto the thread's Event Queue for later processing (otherwise, the thread returns from the
 * blocking receive call).  Now, if events on other sockets and timers, etc. happen during this
 * time, then those events will be delayed and messages arriving on that one IPC socket will
 * effectively jump ahead of those other events in the Event Queue.  The only way to prevent this
 * is to either use asynchronous request-response handling or spawn a separate thread for
 * the blocking request-response.  Other solutions were explored, but none were found that didn't
 * create opportunities for unexpected reentrancy and/or unbounded recursion.
 *
 * Only one thread can wait on a socket.  If two threads tried to call le_msg_RequestSyncResponse()
 * on the same socket, it would be impossible to ensure that the correct response went to each
 * thread.  Therefore, only the one thread that owns the session is allowed to call
 * le_msg_RequestSyncResponse().
 *
 * Each message transmission over the socket is prepended by a small header which contains
 * a 4-byte transaction identifier.  This is actually a Safe Reference (see @ref c_safeRef)
 * which is used to match response messages back to their associated request messages on the client
 * side.  For all other types of messages, this is set to 0 (NULL) to indicate that it does
 * not belong to a request-response transaction.
 *
 * See also @ref serviceDirectoryProtocol.
 *
 * @warning The code in this subsystem @b must be thread safe and re-entrant.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "messaging.h"
#include "serviceDirectory/serviceDirectoryProtocol.h"
#include "messagingCommon.h"
#include "messagingMessage.h"
#include "messagingProtocol.h"
#include "messagingSession.h"
#include "messagingInterface.h"
#include "messagingLocal.h"

// =======================================
//  PROTECTED (INTER-MODULE) FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Initializes this module.  This must be called only once at start-up, before any other functions
 * in this module are called.
 */
//--------------------------------------------------------------------------------------------------
void msg_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    msgCommon_Init();
    msgLocal_Init();
    msgProto_Init();
    msgMessage_Init();
    msgInterface_Init();
    msgSession_Init();
}
