/**
 * @file le_rpcProxyEventHandler.h
 *
 * Header for rpc Proxy Event Handler feature.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "le_rpcProxy.h"
#include "le_rpcProxyNetwork.h"

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
 * Function for Processing Asynchronous Server Events on the client side
 *
 * @return
 *      -LE_OK if async message was process successfully
 *      -LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcEventHandler_ProcessEvent
(
    void* handle,                   ///< [IN] Opaque handle to the le_comm communication channel
    const char* systemName,         ///< [IN] Name of the system that sent the event
    StreamState_t* streamStatePtr,  ///< [IN] Pointer to the Streaming State-Machine data
    void *proxyMessagePtr           ///< [IN] Pointer to the Proxy Message
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
 * Get new context pointer to send.
 *
 * When a reference pointer is seen in an IPC message, this function must be used to convert the
 * pointer in the IPC message to a new value that can be packed into the outgoing rpc message.
 *
 * @return
 *      -LE_OK if successfully converted the context pointer
 *      -LE_FAULT otherwise
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcEventHandler_RepackOutgoingContext
(
    le_pack_SemanticTag_t    tagId,           ///< [IN] Tag id that appear before context pointer
    void*                    contextPtr,      ///< [IN] Context pointer in ipc message
    void**                   contextPtrPtr,   ///< [OUT] New context pointer
    rpcProxy_Message_t*      proxyMessagePtr  ///< [IN] Pointer to outgoing proxy message
);

//--------------------------------------------------------------------------------------------------
/**
 * Function for repacking client context seen in an incoming PPC message.
 * This function converts the context pointer seen in the RPC message into a new value that can be
 * packed into IPC message.
 *
 * @return
 *      -LE_OK if successfully converted the context pointer
 *      -LE_FAULT otherwise
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcEventHandler_RepackIncomingContext
(
    le_pack_SemanticTag_t    tagId,           ///< [IN] Tag id that appear before context pointer
    void*                    contextPtr,      ///< [IN] Context pointer in in rpc message
    void**                   contextPtrPtr,   ///< [OUT] New context pointer
    rpcProxy_Message_t*      proxyMessagePtr  ///< [IN] Pointer to incoming proxy message
);

//--------------------------------------------------------------------------------------------------
/**
 * This function initializes and starts the RPC Proxy Event Handler Services.
 *
 * @note Must be called either directly, such as in the case of the RPC Proxy Library,
 *       or indirectly as a Legato component via the RPC Proxy's COMPONENT_INIT_ONCE.
 *
 */
//--------------------------------------------------------------------------------------------------
void rpcEventHandler_InitializeOnce
(
    void
);
