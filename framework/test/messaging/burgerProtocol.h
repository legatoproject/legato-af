/**
 * My crappy little test protocol is called the "Burger Protocol" because it exchanges the
 * constants 0xDEADBEEF and 0xBEEFDEAD back and forth.
 *
 * - The client can send in 0xBEEFBEEF.  Server just logs this.
 * - The client can do an asynchronous request-response transaction, with 0xDEADBEEF in the request.
 *   The server responds to this with 0xBEEFDEAD.
 * - The server can send an unsolicited 0xDEADDEAD message.
 *
 * My apologies to vegetarians and cattle rights advocates.  No actual bovines were harmed in
 * the making of this protocol.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef BURGER_PROTOCOL_H_INCLUDE_GUARD
#define BURGER_PROTOCOL_H_INCLUDE_GUARD

#define BURGER_PROTOCOL_ID_STR "DeadBeefProtocol"

typedef struct
{
    uint32_t payload;
}
burger_Message_t;

#endif // BURGER_PROTOCOL_H_INCLUDE_GUARD
