/** @file le_lpt.c
 *
 * This file contains the data structures and the source code of the LPT (Low Power Technologies)
 * APIs.
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Maximal value for eDRX cycle length, defined in 3GPP TS 24.008 Rel-13 section 10.5.5.32.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_EDRX_VALUE  15


//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

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
le_result_t le_lpt_SetEDrxState
(
    le_lpt_EDrxRat_t    eDrxRat,    ///< [IN] Radio Access Technology.
    le_onoff_t          activation  ///< [IN] eDRX activation state.
)
{
    if ((LE_LPT_EDRX_RAT_UNKNOWN == eDrxRat) || (eDrxRat >= LE_LPT_EDRX_RAT_MAX))
    {
        LE_ERROR("Invalid Radio Access Technology: %d", eDrxRat);
        return LE_BAD_PARAMETER;
    }

    switch (activation)
    {
        case LE_ON:
        case LE_OFF:
            return LE_UNSUPPORTED;

        default:
            LE_ERROR("Invalid activation state %d", activation);
            return LE_BAD_PARAMETER;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the requested eDRX cycle value for the given Radio Access Technology.
 * The eDRX cycle value is defined in 3GPP TS 24.008 Release 13 section 10.5.5.32.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNSUPPORTED    eDRX is not supported by the platform.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_lpt_SetRequestedEDrxValue
(
    le_lpt_EDrxRat_t    eDrxRat,    ///< [IN] Radio Access Technology.
    uint8_t             eDrxValue   ///< [IN] Requested eDRX cycle value, defined in 3GPP
                                    ///<      TS 24.008 Rel-13 section 10.5.5.32.
)
{
    if ((LE_LPT_EDRX_RAT_UNKNOWN == eDrxRat) || (eDrxRat >= LE_LPT_EDRX_RAT_MAX))
    {
        LE_ERROR("Invalid Radio Access Technology: %d", eDrxRat);
        return LE_BAD_PARAMETER;
    }

    if (eDrxValue > MAX_EDRX_VALUE)
    {
        LE_ERROR("Invalid eDRX cycle length %d, max is %d", eDrxValue, MAX_EDRX_VALUE);
        return LE_BAD_PARAMETER;
    }

    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the requested eDRX cycle value for the given Radio Access Technology.
 * The eDRX cycle value is defined in 3GPP TS 24.008 Release 13 section 10.5.5.32.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNSUPPORTED    eDRX is not supported by the platform.
 *  - LE_UNAVAILABLE    No requested eDRX cycle value.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_lpt_GetRequestedEDrxValue
(
    le_lpt_EDrxRat_t    eDrxRat,        ///< [IN]  Radio Access Technology.
    uint8_t*            eDrxValuePtr    ///< [OUT] Requested eDRX cycle value, defined in 3GPP
                                        ///<       TS 24.008 Rel-13 section 10.5.5.32.
)
{
    if ((LE_LPT_EDRX_RAT_UNKNOWN == eDrxRat) || (eDrxRat >= LE_LPT_EDRX_RAT_MAX))
    {
        LE_ERROR("Invalid Radio Access Technology: %d", eDrxRat);
        return LE_BAD_PARAMETER;
    }

    if (!eDrxValuePtr)
    {
        LE_ERROR("Invalid parameter");
        return LE_BAD_PARAMETER;
    }

    *eDrxValuePtr = 0;
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the network-provided eDRX cycle value for given Radio Access Technology.
 * The eDRX cycle value is defined in 3GPP TS 24.008 Release 13 section 10.5.5.32.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNSUPPORTED    eDRX is not supported by the platform.
 *  - LE_UNAVAILABLE    No network-provided eDRX cycle value.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_lpt_GetNetworkProvidedEDrxValue
(
    le_lpt_EDrxRat_t    eDrxRat,        ///< [IN]  Radio Access Technology.
    uint8_t*            eDrxValuePtr    ///< [OUT] Network-provided eDRX cycle value, defined in
                                        ///<       3GPP TS 24.008 Rel-13 section 10.5.5.32.
)
{
    if ((LE_LPT_EDRX_RAT_UNKNOWN == eDrxRat) || (eDrxRat >= LE_LPT_EDRX_RAT_MAX))
    {
        LE_ERROR("Invalid Radio Access Technology: %d", eDrxRat);
        return LE_BAD_PARAMETER;
    }

    if (!eDrxValuePtr)
    {
        LE_ERROR("Invalid parameter");
        return LE_BAD_PARAMETER;
    }

    *eDrxValuePtr = 0;
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the network-provided Paging Time Window for the given Radio Access Technology.
 * The Paging Time Window is defined in 3GPP TS 24.008 Release 13 section 10.5.5.32.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNSUPPORTED    eDRX is not supported by the platform.
 *  - LE_UNAVAILABLE    No defined Paging Time Window.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_lpt_GetNetworkProvidedPagingTimeWindow
(
    le_lpt_EDrxRat_t    eDrxRat,            ///< [IN]  Radio Access Technology.
    uint8_t*            pagingTimeWindowPtr ///< [OUT] Network-provided Paging Time Window, defined
                                            ///<       in 3GPP TS 24.008 Rel-13 section 10.5.5.32.
)
{
    if ((LE_LPT_EDRX_RAT_UNKNOWN == eDrxRat) || (eDrxRat >= LE_LPT_EDRX_RAT_MAX))
    {
        LE_ERROR("Invalid Radio Access Technology: %d", eDrxRat);
        return LE_BAD_PARAMETER;
    }

    if (!pagingTimeWindowPtr)
    {
        LE_ERROR("Invalid parameter");
        return LE_BAD_PARAMETER;
    }

    *pagingTimeWindowPtr = 0;
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Register a handler to be notified of changes in the network-provided eDRX parameters.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_lpt_EDrxParamsChangeHandlerRef_t le_lpt_AddEDrxParamsChangeHandler
(
    le_lpt_EDrxParamsChangeHandlerFunc_t handlerFuncPtr,    ///< [IN] The handler function.
    void*                                contextPtr         ///< [IN] The handler's context.
)
{
    if (!handlerFuncPtr)
    {
        LE_KILL_CLIENT("Handler function is NULL!");
        return NULL;
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for notification of changes in the network-provided eDRX parameters.
 */
//--------------------------------------------------------------------------------------------------
void le_lpt_RemoveEDrxParamsChangeHandler
(
    le_lpt_EDrxParamsChangeHandlerRef_t handlerRef  ///< [IN] The handler reference
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}
