/** @file messagingProtocol.h
 *
 * Low-Level Messaging API implementation's "Protocol" module's inter-module interface definitions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef MESSAGING_PROTOCOL_H_INCLUDE_GUARD
#define MESSAGING_PROTOCOL_H_INCLUDE_GUARD

#include "limit.h"
#include "messagingMessage.h"

//--------------------------------------------------------------------------------------------------
/**
 * Represents a messaging protocol.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_msg_Protocol
{
    le_sls_Link_t link;                     ///< Used to link this into the Protocol List.
    char id[LIMIT_MAX_PROTOCOL_ID_BYTES];   ///< Unique identifier for the protocol.
    size_t maxPayloadSize;                  ///< Max payload size (in bytes) in this protocol.
    le_mem_PoolRef_t messagePoolRef;        ///< Pool of Message objects.
}
msgProtocol_Protocol_t;


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the messagingProtocol module.  This must be called only once at start-up, before
 * any other functions in that module are called.
 */
//--------------------------------------------------------------------------------------------------
void msgProto_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Allocate a Message object from a given Protocol's Message Pool.
 *
 * @return A pointer to the (uninitialized) Message object memory.
 */
//--------------------------------------------------------------------------------------------------
UnixMessage_t *msgProto_AllocMessage
(
    le_msg_ProtocolRef_t protocolRef
);


#endif // MESSAGING_PROTOCOL_H_INCLUDE_GUARD
