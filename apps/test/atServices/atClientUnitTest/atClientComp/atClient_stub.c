/**
 * This module implements some stub for atClient unit test.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "defs.h"

//--------------------------------------------------------------------------------------------------
/**
 * Get the client service reference stub
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_atClient_GetServiceRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference stub
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_atClient_GetClientSessionRef
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
    le_msg_ServiceRef_t               serviceRef,
    le_msg_SessionEventHandler_t      handlerFunc,
    void                              *contextPtr
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
    le_msg_ServiceRef_t               serviceRef,
    le_msg_SessionEventHandler_t      handlerFunc,
    void                              *contextPtr
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start watchdogs 0..N-1.  Typically this is used in COMPONENT_INIT to start all watchdogs needed
 * by the process.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_Init
(
    uint32_t wdogCount
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Begin monitoring the event loop on the current thread.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_MonitorEventLoop
(
    uint32_t            watchdog,         ///< Watchdog to use for monitoring
    le_clk_Time_t       watchdogInterval  ///< Interval at which to check event loop is functioning
)
{
}
