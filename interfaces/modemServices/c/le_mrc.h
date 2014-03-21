/**
 * @page c_mrc Modem Radio Control API
 *
 * @ref le_mrc.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * This file contains the the prototype definitions of the high level Modem Radio Control (MRC)
 * APIs.
 *
 * @ref le_mrc_power <br>
 * @ref le_mrc_registration <br>
 * @ref le_mrc_signal <br>
 *
 * It's important for many M2M apps to know details about cellular network environments (like network registration and signal quality). 
 * It allows you to limit some M2M services based on the reliability of the network environment, and
 * provides information to control power consumption (power on or shutdown the radio module).
 *
 * @section le_mrc_power Radio Power Management
 * @c le_mrc_SetRadioPower() API allows the application to power up or shutdown the radio module.
 *
 * @c le_mrc_GetRadioPower() API displays radio module power state.
 *
 * @section le_mrc_registration Network Registration
 * @c le_mrc_GetNetRegState() API retrieves the radio module network registration status.
 *
 * The application can register a handler function to retrieve the registration status each time the
 * registration state changes.
 *
 * @c le_mrc_AddNetRegStateHandler() API installs a registration state handler.
 *
 * @c le_mrc_RemoveNetRegStateHandler() API uninstalls the handler function.
 * @note If only one handler is registered, the le_mrc_RemoveNetRegStateHandler() API
 *       resets the registration mode to its original value before any handler functions were added.
 *
 * @section le_mrc_signal Signal Quality
 * @c le_mrc_GetSignalQual() retrieves the received signal strength details.
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


/** @file le_mrc.h
 *
 * Legato @ref c_mrc include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_MRC_INCLUDE_GUARD
#define LEGATO_MRC_INCLUDE_GUARD

#include "legato.h"
#include "le_mdm_defs.h"



//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Reference type for Network Registration State's Changes Handler references.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mrc_NetRegStateHandler* le_mrc_NetRegStateHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report that the network registration state has changed.
 *
 * @param state       Parameter ready to receive the Network Registration state.
 * @param contextPtr  Context information the event handler may require.
 */
//--------------------------------------------------------------------------------------------------
typedef void(*le_mrc_NetRegStateHandlerFunc_t)
(
    le_mrc_NetRegState_t  state,
    void*                 contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Register an handler for network registration state change.
 *
 * @return Handler reference: only needed to remove the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_NetRegStateHandlerRef_t le_mrc_AddNetRegStateHandler
(
    le_mrc_NetRegStateHandlerFunc_t handlerFuncPtr, ///< [IN] The handler function.
    void*                           contextPtr      ///< [IN] The handler's context.
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove a handler for network registration state changes.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_RemoveNetRegStateHandler
(
    le_mrc_NetRegStateHandlerRef_t    handlerRef ///< [IN] The handler reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the power of the Radio Module.
 *
 * @return LE_FAULT  Function failed.
 * @return LE_OK     Function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_SetRadioPower
(
    le_onoff_t    power   ///< [IN] The power state.
);

//--------------------------------------------------------------------------------------------------
/**
 * Must be called to get the Radio Module power state.
 *
 * @return LE_NOT_POSSIBLE  Function failed to get the Radio Module power state.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetRadioPower
(
    le_onoff_t*    powerPtr   ///< [OUT] Power state.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the network registration state.
 *
 * @return LE_NOT_POSSIBLE  Function failed to get the Network registration state.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetNetRegState
(
    le_mrc_NetRegState_t*   statePtr  ///< [OUT] Network Registration state.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the signal quality.
 *
 * @return LE_NOT_POSSIBLE  Function failed to obtain the signal quality.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetSignalQual
(
    uint32_t*   qualityPtr  ///< [OUT] Received signal strength quality (0 = no signal strength,
                            ///        5 = very good signal strength).
);

#endif // LEGATO_MRC_INCLUDE_GUARD
