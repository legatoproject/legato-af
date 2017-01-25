/**
 * This module implements some stub for atServer unit test.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service refrence stub
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_atServer_GetServiceRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session refrence stub
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_atServer_GetClientSessionRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add service open handler stub
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t AddServiceOpenHandler
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
le_msg_SessionEventHandlerRef_t AddServiceCloseHandler
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionEventHandler_t handlerFunc,
    void *contextPtr
)
{
    return NULL;
}
