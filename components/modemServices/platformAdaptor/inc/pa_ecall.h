/**
 * @page c_pa_ecall Modem eCall Platform Adapter API
 *
 * @ref pa_ecall.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * @section pa_info_toc Table of Contents
 *
 *  - @ref pa_ecall_intro
 *  - @ref pa_ecall_rational
 *
 *
 * @section pa_ecall_intro Introduction
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developped upon these APIs.
 *
 *
 * @section pa_ecall_rational Rational
 * These functions are all blocking functions, so that they return when the modem has answered or
 * when a timeout has occured due to an interrupted communication with the modem.
 *
 * They all verify the validity and the range of the input parameters before performing the modem
 * operation.
 *
 * Some functions are used to get some information with a fixed pattern string,
 * in this case no buffer overflow will occur has they always get a fixed string length.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */


/** @file pa_ecall.h
 *
 * Legato @ref c_pa_ecall include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */

#ifndef LEGATO_PAECALL_INCLUDE_GUARD
#define LEGATO_PAECALL_INCLUDE_GUARD


#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Enumeration used to specify the mode of transmission.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_ECALL_TX_MODE_PULL,  ///< Pull mode (modem/host waits for MSD request from PSAP to send MSD).
    PA_ECALL_TX_MODE_PUSH   ///< Push mode (modem/host sends MSD to PSAP right after eCall is connected).
}
pa_ecall_MsdTxMode_t;

//--------------------------------------------------------------------------------------------------
/**
 * A handler that is called whenever a eCall event is received by the modem.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_ecall_EventHandlerFunc_t)
(
    le_ecall_State_t*  statePtr  ///< eCall state
);

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

#endif // LEGATO_PAECALL_INCLUDE_GUARD
