/**
 * @file pa_ecall_qmi.c
 *
 * QMI implementation of @ref c_pa_ecall API.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
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
    void
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
 * This function must be called to set the maximum redial attempt when an ecall failed.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_SetMaxRedialAttempts
(
    uint32_t redialAttemptsCount ///< [IN] Number of redial attempt
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
    pa_ecall_MsdTxMode_t mode
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to load the Minimum Set of Data for the eCall.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_LoadMsd
(
    uint8_t  *msdPtr,   ///< [IN] Encoded Msd
    size_t    msdSize   ///< [IN] msd buffer size
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start an eCall test.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_StartTest
(
    void
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start an automatic eCall.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_StartAutomatic
(
    void
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start a manual eCall.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_StartManual
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

