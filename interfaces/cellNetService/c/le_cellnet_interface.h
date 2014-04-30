/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 * @page c_le_cellnet Cellular Network Service
 *
 * @ref le_cellnet_interface.h "Click here for the API Reference documentation."
 *
 * <HR>
 *
 * @section c_le_cellnet_toc Table of Contents
 *
 *  - @ref c_le_cellnet_intro
 *  - @ref c_le_cellnet_requesting
 *  - @ref c_le_cellnet_simconfigdb
 *  - @ref c_le_cellnet_options
 *
 *
 * @section c_le_cellnet_intro Introduction
 *
 * The Cellular Network service ensures that the modem is registered on the network when an user
 * application makes a request for network access. This includes:
 * - ensuring that the radio is turned on.
 * - ensuring that there is a valid SIM, and it is unlocked.
 * - ensuring that the modem is registered on the network.
 *
 * If all of the above conditions are met, then the service indicates that the network is available.
 *
 * @section c_le_cellnet_requesting Requesting the Cellular Network
 *
 * The Cellular Network can be requested using @ref le_cellnet_Request. The @ref le_cellnet_Request
 * function will turn on the radio if it is switched off and it will unlock the SIM if a PIN code is
 * required (it will retrieve the needed information from the config DB,
 * cf. @ref c_le_cellnet_simconfigdb).
 * Before the cellular network is requested, an application should register a network state handler
 * using @ref le_cellnet_AddStateHandler. Once the cellular network becomes available, the handler
 * will be called to indicate that the modem is now registered on the network.
 *
 * If the state of the network changes, then the handler will be called with the new state.
 *
 * To release the cellular network, an application can use @ref le_cellnet_Release. Once all user
 * applications release the cellular network access, then the service will turn off the radio.
 *
 * All configuration data required for a network registration, such as the PIN code of the SIM, will
 * be stored in the Config DB.
 *
 * @section c_le_cellnet_simconfigdb SIM configuration tree
 *
 * The configuration database path for the SIM is:
 * @verbatim
   /
       modemServices/
           sim/
               1/
                   pin<string> == <PIN_CODE>

  @endverbatim
 *
 *  - '1' is the sim slot number that @ref le_sim_GetSelectedCard is returning.
 *
 *  - 'PIN_CODE' is the PIN code for the SIM card.
 *
 * @note
 * when a new SIM is inserted and :
 *   - is locked, Cellular Network Service will read automatically the config DB in order to try to
 * enter the pin for the SIM card.
 *   - is blocked, Cellular Network Service just log an error and did not try to enter the puk code.
 *
 * @section c_le_cellnet_options Network Options
 *
 * @note The functionaliy described in this section is not currently implemented; this
 * description is provided to outline future functionality.
 *
 * Some applications may have network requirements that are not met by the default cellular network
 * service. For example, it would specify the SIM on which it wants to operate.
 *
 * In this case, an application can create a request object using @ref le_cellnet_CreateRequest,
 * set optional values on that request object, using @ref le_cellnet_SelectSim and then submits that
 * object to a cellular network request, using @ref le_cellnet_SubmitRequest.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to
 * license.
 */
/**
 * @file le_cellnet_interface.h
 *
 * Legato @ref c_le_cellnet include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to
 * license.
 *
 */

#ifndef LE_CELLNET_INTERFACE_H_INCLUDE_GUARD
#define LE_CELLNET_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Start the service for the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_cellnet_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_cellnet_StopClient
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Reference returned by Request function and used by Release function
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_cellnet_RequestObj* le_cellnet_RequestObjRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Cellular Network states.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_CELLNET_RADIO_OFF,
        ///< The radio is Off.

    LE_CELLNET_REG_EMERGENCY,
        ///< Only Emergency calls are allowed.

    LE_CELLNET_REG_HOME,
        ///< Registered, home network.

    LE_CELLNET_REG_ROAMING,
        ///< Registered to a roaming network.

    LE_CELLNET_REG_UNKNOWN
        ///< Unknown state.
}
le_cellnet_State_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reference type for le_cellnet_StateHandler handler ADD/REMOVE functions
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_cellnet_StateHandler* le_cellnet_StateHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Handler for network state changes
 *
 * @param state
 *        The cellular network state
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_cellnet_StateHandlerFunc_t)
(
    le_cellnet_State_t state,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * le_cellnet_StateHandler handler ADD function
 */
//--------------------------------------------------------------------------------------------------
le_cellnet_StateHandlerRef_t le_cellnet_AddStateHandler
(
    le_cellnet_StateHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * le_cellnet_StateHandler handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void le_cellnet_RemoveStateHandler
(
    le_cellnet_StateHandlerRef_t addHandlerRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Request a cellular network
 *
 * @return
 *      - A reference to the cellular network
 *      - 0 (zero) if the network requested could not be processed
 */
//--------------------------------------------------------------------------------------------------
le_cellnet_RequestObjRef_t le_cellnet_Request
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Release a cellular network
 */
//--------------------------------------------------------------------------------------------------
void le_cellnet_Release
(
    le_cellnet_RequestObjRef_t cellNetRef
        ///< [IN]
        ///< Reference to a cellular network request.
);


#endif // LE_CELLNET_INTERFACE_H_INCLUDE_GUARD

