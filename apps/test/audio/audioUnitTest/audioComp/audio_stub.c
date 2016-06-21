/**
 * This module implements some stub for audio unit tetst.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_audio_GetClientSessionRef
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
le_msg_ServiceRef_t le_audio_GetServiceRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called whenever one of this service's sessions is closed by
 * the client.  (STUBBED FUNCTION)

 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t MyAddServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
)
{
    return NULL;
}
