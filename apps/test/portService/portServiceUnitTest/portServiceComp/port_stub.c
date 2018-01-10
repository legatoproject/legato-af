/**
 * This module implements some stub code for port service unit test.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */


#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Get the server service refrence stub
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_port_GetServiceRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session refrence stub
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_port_GetClientSessionRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add service close handler stub.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t MyAddServiceCloseHandler
(
    le_msg_ServiceRef_t serviceRef,            ///< [IN] Service reference.
    le_msg_SessionEventHandler_t handlerFunc,  ///< [IN] Handler function pointer.
    void *contextPtr                           ///< [IN] Context pointer.
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
    uint32_t wdogCount         ///< [IN] Watchdog count
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
    uint32_t watchdog,             ///< [IN] Watchdog to use for monitoring
    le_clk_Time_t watchdogInterval ///< [IN] Interval at which to check event loop is functioning
)
{
}
