#ifndef _INTERFACES_H
#define _INTERFACES_H

#include "le_port_interface.h"

#undef LE_KILL_CLIENT
#define LE_KILL_CLIENT LE_ERROR


//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message (STUBBED FUNCTION)
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_port_GetClientSessionRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference (STUBBED FUNCTION)
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_port_GetServiceRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called whenever one of this service's sessions is closed by
 * the client.  (STUBBED FUNCTION)
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t le_msg_AddServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef,  ///< [IN] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc, ///< [IN] Handler function.
    void*                           contextPtr   ///< [IN] Opaque pointer value to pass to handler.
);
#endif /* interfaces.h */
