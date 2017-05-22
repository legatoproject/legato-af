/**
 * interfaces.h
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "le_atClient_interface.h"

#undef LE_KILL_CLIENT
#define LE_KILL_CLIENT LE_WARN

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_atClient_GetClientSessionRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_atClient_GetServiceRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Advertise the server service
 */
//--------------------------------------------------------------------------------------------------
void le_atClient_AdvertiseService
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Add service open handler
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t AddServiceOpenHandler
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionEventHandler_t handlerFunc,
    void *contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Add service close handler
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t AddServiceCloseHandler
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionEventHandler_t handlerFunc,
    void *contextPtr
);
