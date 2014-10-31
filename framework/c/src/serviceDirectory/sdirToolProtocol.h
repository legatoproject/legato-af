//--------------------------------------------------------------------------------------------------
/** @file sdirToolProtocol.h
 *
 * Definitions related to the protocol used between the "sdir" tool and the Service Directory.
 *
 * <hr/>
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef SDIR_TOOL_PROTOCOL_H_INCLUDE_GUARD
#define SDIR_TOOL_PROTOCOL_H_INCLUDE_GUARD

#include "../limit.h"


//--------------------------------------------------------------------------------------------------
/// Name of the 'sdir' tool protocol service instance.
//--------------------------------------------------------------------------------------------------
#define LE_SDTP_SERVICE_NAME    "sdirTool"


//--------------------------------------------------------------------------------------------------
/// Protocol ID of the 'sdir' tool protocol.
//--------------------------------------------------------------------------------------------------
#define LE_SDTP_PROTOCOL_ID     "sdirTool"


//--------------------------------------------------------------------------------------------------
/**
 * Message type IDs.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_SDTP_MSGID_LIST,             ///< List all contents of the Service Directory.
                                    ///  Payload is a file descriptor to which output
                                    ///  should be written.

    LE_SDTP_MSGID_UNBIND_ALL,       ///< Delete all bindings (This message has no payload).

    LE_SDTP_MSGID_BIND,             ///< Create one binding.  The payload is the binding details.
                                    ///  If the Service Directory runs into an error, it will
                                    ///  drop the connection to the sdir tool without responding.
}
le_sdtp_MsgType_t;


//--------------------------------------------------------------------------------------------------
/**
 * Message structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sdtp_MsgType_t msgType;  ///< Indicates what type of message this is.
    uid_t client;               ///< Unix user ID of the client.
    uid_t server;               ///< Unix user ID of the server.
    char clientServiceName[LIMIT_MAX_SERVICE_NAME_BYTES];   ///< Client's service name.
    char serverServiceName[LIMIT_MAX_SERVICE_NAME_BYTES];   ///< Server's service name.
}
le_sdtp_Msg_t;


#endif // SDIR_TOOL_PROTOCOL_H_INCLUDE_GUARD
