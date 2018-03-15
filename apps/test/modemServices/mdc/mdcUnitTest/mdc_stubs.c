/**
 * @file fwupdate_stubs.c
 *
 * Stub functions required for firmware update tests.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include <stdint.h>

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
    uint32_t watchdog,          ///< Watchdog to use for monitoring
    le_clk_Time_t watchdogInterval ///< Interval at which to check event loop is functioning
)
{
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
    void* contextPtr                           ///< [IN] Context pointer.
)
{
    return NULL;
}

