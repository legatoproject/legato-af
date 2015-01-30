/**
 * @file pa_ecall_simu.c
 *
 * Simulation implementation of @ref c_pa_ecall API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "pa.h"
#include "pa_ecall.h"
#include "pa_simu.h"

#define MSD_BLOB_SIZE 140

//--------------------------------------------------------------------------------------------------
/**
 * Call event ID used to report ecall events to the registered event handlers.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t ECallEvent;

static char PsapNumber[LE_MDMDEFS_PHONE_NUM_MAX_LEN] = PA_SIMU_ECALL_DEFAULT_PSAP;
static uint32_t MaxRedialAttempts = PA_SIMU_ECALL_DEFAULT_MAX_REDIAL_ATTEMPTS;
static le_ecall_MsdTxMode_t MsdTxMode = PA_SIMU_ECALL_DEFAULT_MSD_TX_MODE;
static uint8_t MsdData[MSD_BLOB_SIZE];

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the ecall module
 *
 * @return LE_NOT_POSSIBLE  The function failed to initialize the module.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t ecall_simu_Init
(
    void
)
{
    // Create the event for signaling user handlers.
    ECallEvent = le_event_CreateId("ECallEvent",sizeof(le_ecall_State_t));

    return LE_OK;
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
    LE_DEBUG("Set new eCall Event handler.");

    LE_FATAL_IF(handlerFuncPtr == NULL, "The new eCall Event handler is NULL.");

    return le_event_AddHandler("ECallEventHandler",
                               ECallEvent,
                               (le_event_HandlerFunc_t)handlerFuncPtr);

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
    LE_DEBUG("Clear eCall Event handler %p",handlerRef);
    le_event_RemoveHandler(handlerRef);
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
    char psap[LE_MDMDEFS_PHONE_NUM_MAX_LEN] ///< [IN] Public Safely Answering Point number
)
{
    return le_utf8_Copy(PsapNumber, psap, LE_MDMDEFS_PHONE_NUM_MAX_LEN, NULL);
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
    return le_utf8_Copy(psapPtr, PsapNumber, len, NULL);
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
    MaxRedialAttempts = redialAttemptsCount;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the maximum redial attempt when an ecall failed.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_GetMaxRedialAttempts
(
    uint32_t* redialAttemptsCountPtr ///< [OUT] Number of redial attempt
)
{
    *redialAttemptsCountPtr = MaxRedialAttempts;
    return LE_OK;
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
    MsdTxMode = mode;
    return LE_OK;
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
    *modePtr = MsdTxMode;
    return LE_OK;
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
    if(msdSize > sizeof(MsdData))
    {
        LE_ERROR("MSD data is too big (= %zd, max %zd)", msdSize, sizeof(MsdData));
        return LE_FAULT;
    }

    memcpy(MsdData, msdPtr, msdSize);
    return LE_OK;
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
    // TODO implementation
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
    // TODO implementation
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
