/**
 * @file pa_lpt_default.c
 *
 * Default implementation of @ref c_pa_lpt.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_lpt.h"


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for eDRX parameters change indication.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_lpt_AddEDrxParamsChangeHandler
(
    pa_lpt_EDrxParamsChangeIndHandlerFunc_t handlerFuncPtr  ///< [IN] The handler function.
)
{
    LE_ERROR("Unsupported function called");
    return NULL;
}

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
le_result_t pa_lpt_SetEDrxState
(
    le_lpt_EDrxRat_t    eDrxRat,    ///< [IN] Radio Access Technology.
    le_onoff_t          activation  ///< [IN] eDRX activation state.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

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
le_result_t pa_lpt_SetRequestedEDrxValue
(
    le_lpt_EDrxRat_t    eDrxRat,    ///< [IN] Radio Access Technology.
    uint8_t             eDrxValue   ///< [IN] Requested eDRX cycle value.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

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
le_result_t pa_lpt_GetRequestedEDrxValue
(
    le_lpt_EDrxRat_t    eDrxRat,        ///< [IN] Radio Access Technology.
    uint8_t*            eDrxValuePtr    ///< [OUT] Requested eDRX cycle value
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

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
le_result_t pa_lpt_GetNetworkProvidedEDrxValue
(
    le_lpt_EDrxRat_t    eDrxRat,        ///< [IN]  Radio Access Technology.
    uint8_t*            eDrxValuePtr    ///< [OUT] Network-provided eDRX cycle value.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

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
le_result_t pa_lpt_GetNetworkProvidedPagingTimeWindow
(
    le_lpt_EDrxRat_t    eDrxRat,            ///< [IN]  Radio Access Technology.
    uint8_t*            pagingTimeWindowPtr ///< [OUT] Network-provided Paging Time Window.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}
