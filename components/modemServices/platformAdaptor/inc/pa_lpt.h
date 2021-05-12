/**
 * @page c_pa_lpt Low Power Technologies Platform Adapter API
 *
 * @ref pa_lpt.h "API Reference"
 *
 * <HR>
 *
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developed upon these APIs.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */


/** @file pa_lpt.h
 *
 * Legato @ref c_pa_lpt include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PALPT_INCLUDE_GUARD
#define LEGATO_PALPT_INCLUDE_GUARD

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * eDRX parameters indication structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_lpt_EDrxRat_t    rat;                ///< Radio Access Technology
    le_onoff_t          activation;         ///< eDRX activation state
    uint8_t             eDrxValue;          ///< eDRX cycle value
    uint8_t             pagingTimeWindow;   ///< Paging Time Window
}
pa_lpt_EDrxParamsIndication_t;

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report a change in the eDRX parameters.
 *
 * @param eDrxParamsChangeIndPtr The eDRX parameters.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_lpt_EDrxParamsChangeIndHandlerFunc_t)
(
    pa_lpt_EDrxParamsIndication_t* eDrxParamsChangeIndPtr
);


//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for eDRX parameters change indication.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_event_HandlerRef_t pa_lpt_AddEDrxParamsChangeHandler
(
    pa_lpt_EDrxParamsChangeIndHandlerFunc_t handlerFuncPtr  ///< [IN] The handler function.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the eDRX activation state for the given Radio Access Technology.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNSUPPORTED    eDRX is not supported by the platform.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_lpt_SetEDrxState
(
    le_lpt_EDrxRat_t    eDrxRat,    ///< [IN] Radio Access Technology.
    le_onoff_t          activation  ///< [IN] eDRX activation state.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the requested eDRX cycle value for the given Radio Access Technology.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNSUPPORTED    eDRX is not supported by the platform.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_lpt_SetRequestedEDrxValue
(
    le_lpt_EDrxRat_t    eDrxRat,    ///< [IN] Radio Access Technology.
    uint8_t             eDrxValue   ///< [IN] Requested eDRX cycle value.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the requested eDRX cycle value for the given Radio Access Technology.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNSUPPORTED    eDRX is not supported by the platform.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_lpt_GetRequestedEDrxValue
(
    le_lpt_EDrxRat_t    eDrxRat,        ///< [IN] Radio Access Technology.
    uint8_t*            eDrxValuePtr    ///< [OUT] Requested eDRX cycle value
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the network-provided eDRX cycle value for the given Radio Access Technology.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNSUPPORTED    eDRX is not supported by the platform.
 *  - LE_UNAVAILABLE    No network-provided eDRX cycle value.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_lpt_GetNetworkProvidedEDrxValue
(
    le_lpt_EDrxRat_t    eDrxRat,        ///< [IN]  Radio Access Technology.
    uint8_t*            eDrxValuePtr    ///< [OUT] Network-provided eDRX cycle value.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the network-provided Paging Time Window for the given Radio Access Technology.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNSUPPORTED    eDRX is not supported by the platform.
 *  - LE_UNAVAILABLE    No defined Paging Time Window.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_lpt_GetNetworkProvidedPagingTimeWindow
(
    le_lpt_EDrxRat_t    eDrxRat,            ///< [IN]  Radio Access Technology.
    uint8_t*            pagingTimeWindowPtr ///< [OUT] Network-provided Paging Time Window.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the lowest power saving mode that the module can enter.
 *
 * @return
 *      - LE_OK             The function succeeded.
 *      - LE_BAD_PARAMETER  A parameter is invalid.
 *      - LE_UNSUPPORTED    Operation is not supported.
 *      - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_lpt_SetPmMode
(
    le_lpt_PMMode_t mode    ///< [IN] Power saving mode
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the PSM activation state.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNSUPPORTED    PSM is not supported by the platform.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_lpt_SetPSMState
(
    le_onoff_t          activation  ///< [IN] PSM activation state.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the Power Saving Mode Setting.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNSUPPORTED    PSM is not supported by the platform.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_lpt_SetPSMValue
(
    uint8_t rqstPeriodicRau,        ///< [IN] Requested periodic RAU
    uint8_t rqstGprsRdyTimer,       ///< [IN] Requested GPRS Ready timer
    uint8_t rqstPeriodicTau,        ///< [IN] Requested periodic TAU
    uint8_t rqstActiveTimer         ///< [IN] Requested active timer
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Power Saving Mode Setting.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNSUPPORTED    PSM is not supported by the platform.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_lpt_GetPSMValue
(
    uint8_t* rqstPeriodicRauPtr,      ///< [OUT] Requested periodic RAU
    uint8_t* rqstGprsRdyTimerPtr,     ///< [OUT] Requested GPRS Ready timer
    uint8_t* rqstPeriodicTauPtr,      ///< [OUT] Requested periodic TAU
    uint8_t* rqstActiveTimerPtr       ///< [OUT] Requested active timer
);

//--------------------------------------------------------------------------------------------------
/**
* This function must be called to initialize the LPT module.
*
* @return
*  - LE_OK            The function succeeded.
*  - LE_FAULT         The function failed to initialize the module.
*/
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_lpt_Init
(
   void
);
#endif // LEGATO_PALPT_INCLUDE_GUARD
