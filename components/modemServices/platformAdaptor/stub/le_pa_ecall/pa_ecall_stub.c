/**
 * @file pa_ecall_qmi.c
 *
 * QMI implementation of @ref c_pa_ecall API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


#include "legato.h"
#include "pa_ecall.h"

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Component initializer automatically called by the application framework when the process starts.
 *
 **/
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}

//--------------------------------------------------------------------------------------------------
/**
 * This function initializes the platform adapter layer for eCall services.
 *
 * @return LE_OK if successful.
 * @return LE_FAULT if unsuccessful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_Init
(
    pa_ecall_SysStd_t sysStd ///< [IN] Choosen system (PA_ECALL_PAN_EUROPEAN or PA_ECALL_ERA_GLONASS)
)
{
    return LE_FAULT;
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
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the Public Safely Answering Point number.
 *
 * @return LE_FAULT     The function failed.
 * @return LE_OVERFLOW  psap number is too long.
 * @return LE_OK        The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_SetPsapNumber
(
    char psap[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] ///< [IN] Public Safely Answering Point number
)
{
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
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function can be recalled to indicate the modem to read the number to dial from the FDN/SDN
 * of the U/SIM, depending upon the eCall operating mode.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_UseUSimNumbers
(
    void
)
{
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
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the 'NAD Deregistration Time' value in minutes.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_SetNadDeregistrationTime
(
    uint16_t    deregTime  ///< [IN] the 'NAD Deregistration Time' value in minutes.
)
{
    // todo: implement this one
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the 'NAD Deregistration Time' value in minutes.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_GetNadDeregistrationTime
(
    uint16_t*    deregTimePtr  ///< [OUT] the 'NAD Deregistration Time' value in minutes.
)
{
    // todo: implement this one
    return LE_FAULT;
}



