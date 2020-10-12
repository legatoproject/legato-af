/**
 * @file le_rpcProxyEventHandler.h
 *
 * Header for rpc Proxy Event Handler feature.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "le_rpcProxy.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function for sending RemoveHandler messages to far-side rpcProxy for all
 * ClientEventData_t records when the given client session is closed.
 */
//--------------------------------------------------------------------------------------------------
void rpcEventHandler_SendRemoveHandlerMessage
(
    ///< [IN] Name of the system
    const char* systemName,
    ///< [IN] Client session reference
    le_msg_SessionRef_t sessionRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for deleting all ClientEventData_t records for given service and client session.
 * If specified client session is NULL , deleting all records of the given service.
 */
//--------------------------------------------------------------------------------------------------
void rpcEventHandler_DeleteAll
(
    uint32_t serviceId,                  ///< [IN] Service ID
    le_msg_SessionRef_t sessionRef       ///< [IN] Client session reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for repacking client context in certain cases related to adding async event
 * handler to server and handling these events.
 * Packing and unpacking is always done on client side
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcEventHandler_RepackContext
(
    /// [IN/OUT] Pointer to current position in the Message Buffer pointer
    uint8_t**                msgBufPtr,

    /// [IN/OUT] Pointer to the previous position in the Message Buffer pointer
    uint8_t**                previousMsgBufPtr,

    /// [IN/OUT] Pointer to the current position in the New Message Buffer pointer
    uint8_t**                newMsgBufPtr,

    ///< [IN] Pointer to commont header needed to get message type and id
    rpcProxy_CommonHeader_t* commonHeader,

    ///< [IN] Boolean to identify if message is in-coming or out-going
    bool                     sending,

    //< [OUT] Pointer to client's session reference
    le_msg_SessionRef_t *sessionRefPtr

);


//--------------------------------------------------------------------------------------------------
/**
 * Function for Processing Asynchronous Server Events on the client side
 */
//--------------------------------------------------------------------------------------------------
void rpcEventHandler_ProcessEvent
(
    rpcProxy_Message_t *proxyMessagePtr, ///< [IN] Pointer to the Proxy Message
    le_msg_SessionRef_t sessionRef       ///< [IN] Client session reference
);


//--------------------------------------------------------------------------------------------------
/**
 * Function for Receiving Aynchronous Server Events on the server side.
 * These events were previously registered by client.
 */
//--------------------------------------------------------------------------------------------------
void rpcEventHandler_EventCallback
(
    le_msg_MessageRef_t eventMsgRef, ///< [IN] Event Message reference
    void*               contextPtr   ///< [IN] Context pointer
);



//--------------------------------------------------------------------------------------------------
/**
 * This function initializes and starts the RPC Proxy Event Handler Services.
 *
 * @note Must be called either directly, such as in the case of the RPC Proxy Library,
 *       or indirectly as a Legato component via the RPC Proxy's COMPONENT_INIT.
 *
 */
//--------------------------------------------------------------------------------------------------
void rpcEventHandler_Initialize
(
    void
);
