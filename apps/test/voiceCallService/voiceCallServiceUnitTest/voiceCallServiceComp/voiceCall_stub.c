/**
 * This module implements some stubs for the voice call service unit tests.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"

#undef LE_KILL_CLIENT
#define LE_KILL_CLIENT LE_ERROR

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_voicecall_GetServiceRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_voicecall_GetClientSessionRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the server and advertise the service.
 */
//--------------------------------------------------------------------------------------------------
void le_voicecall_AdvertiseService
(
    void
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * SetServiceRecvHandler stub
 *
 */
//--------------------------------------------------------------------------------------------------
void MySetServiceRecvHandler
(
    le_msg_ServiceRef_t     serviceRef, ///< [in] Reference to the service.
    le_msg_ReceiveHandler_t handlerFunc,///< [in] Handler function.
    void*                   contextPtr  ///< [in] Opaque pointer value to pass to the handler.
)
//--------------------------------------------------------------------------------------------------
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add service open handler stub
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t MyAddServiceOpenHandler
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionEventHandler_t handlerFunc,
    void *contextPtr
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add service close handler stub
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t MyAddServiceCloseHandler
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionEventHandler_t handlerFunc,
    void *contextPtr
)
{
    return NULL;
}
