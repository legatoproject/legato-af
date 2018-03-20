/**
 * This module implements some stubs for the gnss service unit tests.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Store client number.
 */
//--------------------------------------------------------------------------------------------------
static Client_t Client;

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference stub for le_gnss
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_gnss_GetServiceRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference stub for le_gnss
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_gnss_GetClientSessionRef
(
    void
)
{
    le_msg_SessionRef_t sessionRef = NULL;

    if (CLIENT1 == Client)
    {
        sessionRef = (le_msg_SessionRef_t)(0x1234);
    }
    else if (CLIENT2 == Client)
    {
        sessionRef = (le_msg_SessionRef_t)(0x5678);
    }
    return sessionRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the client number for simulation of le_gnss_GetClientSessionRef() API.
 */
//--------------------------------------------------------------------------------------------------
void le_gnss_SetClientSimu(Client_t client)
{
    if (CLIENT1 == client)
    {
        Client = CLIENT1;
    }
    else if (CLIENT2 == client)
    {
        Client = CLIENT2;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Close and free the given iterator object. If the iterator is a write iterator, the transaction
 * will be canceled. If the iterator is a read iterator, the transaction will be closed.
 *
 * @note This operation will also delete the iterator object.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_CancelTxn
(
    le_cfg_IteratorRef_t iteratorRef
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a signed integer value from the config tree.
 *
 * If the underlying value is not an integer, the default value will be returned instead. The
 * default value is also returned if the node does not exist or if it's empty.
 *
 * If the value is a floating point value, then it will be rounded and returned as an integer.
 *
 * Valid for both read and write transactions.
 *
 * If the path is empty, the iterator's current node will be read.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_cfg_GetInt
(
    le_cfg_IteratorRef_t            iteratorRef,
    const char*                     path,
    int32_t                         defaultValue
)
{
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called whenever one of this service's sessions is closed by
 * the client.  (STUBBED FUNCTION)
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t MyAddServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef,  ///< [IN] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc, ///< [IN] Handler function.
    void*                           contextPtr   ///< [IN] Opaque pointer value to pass to handler.
)
{
    return NULL;
}
