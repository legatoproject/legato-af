/**
 * This module implements some stubs for the voice call service unit tests.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_GetRemoteTel() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_GetRemoteTel
(
    le_mcc_CallRef_t callRef,    ///< [IN]  The call reference to read from.
    char*                telPtr,     ///< [OUT] The telephone number string.
    size_t               len         ///< [IN]  The length of telephone number string.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_Start() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_Start
(
    le_mcc_CallRef_t callRef   ///< [IN] Reference to the call object.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  le_mcc_Answer() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_Answer
(
    le_mcc_CallRef_t callRef   ///< [IN] The call reference.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_AddCallEventHandler() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_mcc_CallEventHandlerRef_t le_mcc_AddCallEventHandler
(
    le_mcc_CallEventHandlerFunc_t       handlerFuncPtr, ///< [IN] The event handler function.
    void*                                   contextPtr      ///< [IN] The handlers context.
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_GetTerminationReason() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_mcc_TerminationReason_t le_mcc_GetTerminationReason
(
    le_mcc_CallRef_t callRef   ///< [IN] The call reference to read from.
)
{
   return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_Create() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_mcc_CallRef_t le_mcc_Create
(
    const char* phoneNumPtr
        ///< [IN]
        ///< The target number we are going to
        ///< call.
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_RemoveCallEventHandler() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_RemoveCallEventHandler
(
    le_mcc_CallEventHandlerRef_t handlerRef   ///< [IN] The handler object to remove.
)
{
   return;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_Delete() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_Delete
(
    le_mcc_CallRef_t callRef   ///< [IN] The call object to free.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_HangUp() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_HangUp
(
    le_mcc_CallRef_t callRef   ///< [IN] The call to end.
)
{
    return LE_OK;
}
