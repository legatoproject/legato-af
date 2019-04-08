/**
 * @file fwupdate_stubs.c
 *
 * Stub functions required for firmware update tests.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
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
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_flash_GetClientSessionRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_flash_GetServiceRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the handler callback function to be called when the session is closed from the other
 * end.  A local termination of the session will not trigger this callback.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t le_msg_AddServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the current "system" in use.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_FAULT         On failure
 *      - LE_UNSUPPORTED   The feature is not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dualsys_GetCurrentSystem
(
    le_dualsys_System_t* systemMaskPtr ///< [OUT] Sub-system bitmask for "modem/lk/linux" partitions
)
{
    if (NULL == systemMaskPtr)
    {
        LE_ERROR("Null pointer provided!");
        return LE_FAULT;
    }

    *systemMaskPtr = 0;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop a watchdog.
 *
 * This can also cause the chain to be completely kicked, so check it.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_Stop
(
    uint32_t watchdog
)
{
    // do nothing
}
