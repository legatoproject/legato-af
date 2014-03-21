/** @file messagingProtocol.h
 *
 * Low-Level Messaging API implementation's "Protocol" module's inter-module interface definitions.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef MESSAGING_PROTOCOL_H_INCLUDE_GUARD
#define MESSAGING_PROTOCOL_H_INCLUDE_GUARD

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
le_msg_MessageRef_t msgProto_AllocMessage
(
    le_msg_ProtocolRef_t protocolRef
);


#endif // MESSAGING_PROTOCOL_H_INCLUDE_GUARD
