/**
 * @file pa_ecall.c
 *
 * AT implementation of @ref c_pa_ecall API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


#include "legato.h"
#include "pa.h"
#include "pa_ecall.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the ecall module
 *
 * @return LE_FAULT         The function failed to initialize the module.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_Init
(
    pa_ecall_SysStd_t sysStd ///< [IN] Choosen system (PA_ECALL_PAN_EUROPEAN or PA_ECALL_ERA_GLONASS)
)
{
    // TODO implementation
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the eCall operation mode.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_SetOperationMode
(
    le_ecall_OpMode_t mode ///< [IN] Operation mode
)
{
    // TODO implementation
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to retrieve the configured eCall operation mode.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_GetOperationMode
(
    le_ecall_OpMode_t* mode ///< [OUT] Operation mode
)
{
     // TODO implementation
   return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for eCall event notifications.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_ecall_AddEventHandler
(
    pa_ecall_EventHandlerFunc_t   handlerFuncPtr ///< [IN] The event handler function.
)
{
    // TODO implementation
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for eCalls handling.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void pa_ecall_RemoveEventHandler
(
    le_event_HandlerRef_t handlerRef
)
{
    // TODO implementation
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the Public Safely Answering Point number.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OVERFLOW  psap number is too long.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_SetPsapNumber
(
    char psap[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] ///< [IN] Public Safely Answering Point number
)
{
    // TODO implementation
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Public Safely Answering Point number.
 *
 * @return LE_FAULT     The function failed.
 * @return LE_OVERFLOW  Retrieved PSAP number is too long for the out parameter.
 * @return LE_OK        The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_GetPsapNumber
(
    char*    psapPtr, ///< [OUT] Public Safely Answering Point number
    size_t   len      ///< [IN] The length of SMSC string.
)
{
    // TODO implementation
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set push/pull transmission mode.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_SetMsdTxMode
(
    le_ecall_MsdTxMode_t mode
)
{
    // TODO implementation
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get push/pull transmission mode.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_GetMsdTxMode
(
    le_ecall_MsdTxMode_t* modePtr
)
{
    // TODO implementation
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to send the Minimum Set of Data for the eCall.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_SendMsd
(
    uint8_t  *msdPtr,   ///< [IN] Encoded Msd
    size_t    msdSize   ///< [IN] msd buffer size
)
{
    // TODO implementation
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start the eCall.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if unsuccessful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_Start
(
    pa_ecall_StartType_t callType,
    uint32_t *           callIdPtr
)
{
    // TODO implementation
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop the eCall.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if unsuccessful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_Stop
(
    void
)
{
    // TODO implementation
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to end a eCall.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_End
(
    void
)
{
    // TODO implementation
    return LE_FAULT;
}
