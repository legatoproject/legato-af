/**
 * @page c_pa_mrc Modem Radio Control Platform Adapter API
 *
 * @ref pa_mrc.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * @section pa_mrc_toc Table of Contents
 *
 *  - @ref pa_mrc_intro
 *  - @ref pa_mrc_rational
 *
 *
 * @section pa_mrc_intro Introduction
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developped upon these APIs.
 *
 *
 * @section pa_mrc_rational Rational
 * These functions are all blocking functions, so that they return when the modem has answered or
 * when a timeout has occured due to an interrupted communication with the modem.
 *
 * They all verify the validity and the range of the input parameters before performing the modem
 * operation.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


/** @file pa_mrc.h
 *
 * Legato @ref c_pa_mrc include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_PARC_INCLUDE_GUARD
#define LEGATO_PARC_INCLUDE_GUARD

#include "legato.h"
#include "le_mdm_defs.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Network Registration setting.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_MRC_DISABLE_REG_NOTIFICATION    = 0, ///< Disable network registration notification result
                                            ///  code.
    PA_MRC_ENABLE_REG_NOTIFICATION     = 1, ///< Enable network registration notification code.
    PA_MRC_ENABLE_REG_LOC_NOTIFICATION = 2, ///< Enable network registration and location
                                            ///  information notification result code if there is a
                                            ///  change of network cell.
}
pa_mrc_NetworkRegSetting_t;

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report the Network registration state.
 *
 * @param regStat The Network registration state.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_mrc_NetworkRegHdlrFunc_t)(le_mrc_NetRegState_t* regState);


//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the power of the Radio Module.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetRadioPower
(
    le_onoff_t    power   ///< [IN] The power state.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Radio Module power state.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetRadioPower
(
     le_onoff_t*    powerPtr   ///< [OUT] The power state.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for Network registration state handling.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_mrc_AddNetworkRegHandler
(
    pa_mrc_NetworkRegHdlrFunc_t regStateHandler ///< [IN] The handler function to handle the
                                                ///        Network registration state.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for Network registration state handling.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_RemoveNetworkRegHandler
(
    le_event_HandlerRef_t handlerRef
);

//--------------------------------------------------------------------------------------------------
/**
 * This function configures the Network registration setting.
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_ConfigureNetworkReg
(
    pa_mrc_NetworkRegSetting_t  setting ///< [IN] The selected Network registration setting.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Network registration setting.
 *
 * @return LE_NOT_POSSIBLE The function failed to get the Network registration setting.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetNetworkRegConfig
(
    pa_mrc_NetworkRegSetting_t*  settingPtr   ///< [OUT] The selected Network registration setting.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Network registration state.
 *
 * @return LE_NOT_POSSIBLE The function failed to get the Network registration state.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetNetworkRegState
(
    le_mrc_NetRegState_t* statePtr  ///< [OUT] The network registration state.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Signal Quality information.
 *
 * @return LE_OUT_OF_RANGE  The signal quality values are not known or not detectable.
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetSignalQuality
(
    int32_t*          rssiPtr    ///< [OUT] The received signal strength (in dBm).
);


#endif // LEGATO_PARC_INCLUDE_GUARD
