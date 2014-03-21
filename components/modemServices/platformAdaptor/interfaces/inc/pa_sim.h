/**
 * @page c_pa_sim Modem SIM Platform Adapter API
 *
 * @ref pa_sim.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * @section pa_sim_toc Table of Contents
 *
 *  - @ref pa_sim_intro
 *  - @ref pa_sim_rational
 *
 *
 * @section pa_sim_intro Introduction
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developped upon these APIs.
 *
 *
 * @section pa_sim_rational Rational
 * These functions are all blocking functions, so that they return when the modem has answered or
 * when a timeout has occured due to an interrupted communication with the modem.
 *
 * They all verify the validity and the range of the input parameters before performing the modem
 * operation.
 *
 * Some functions are used to get some information with a fixed pattern string,
 * in this case no buffer overflow will occur has they always get a fixed string length.
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


/** @file pa_sim.h
 *
 * Legato @ref c_pa_sim include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_PASIM_INCLUDE_GUARD
#define LEGATO_PASIM_INCLUDE_GUARD


#include "legato.h"
#include "le_mdm_defs.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Maximum identification number length.
 */
//--------------------------------------------------------------------------------------------------
#define PA_SIM_CARDID_MAX_LEN     20

//--------------------------------------------------------------------------------------------------
/**
 * Maximum International Mobile Subscriber Identity length.
 */
//--------------------------------------------------------------------------------------------------
#define PA_SIM_IMSI_MAX_LEN     15

//--------------------------------------------------------------------------------------------------
/**
 * Maximum PIN Code length.
 *
 */
//--------------------------------------------------------------------------------------------------
#define PA_SIM_PIN_MAX_LEN     8

//--------------------------------------------------------------------------------------------------
/**
 * Maximum PUK Code length.
 *
 */
//--------------------------------------------------------------------------------------------------
#define PA_SIM_PUK_MAX_LEN     8

//--------------------------------------------------------------------------------------------------
/**
 * Type of PIN.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_SIM_PIN=0,     ///< PIN id
    PA_SIM_PIN2       ///< PIN2 id
}
pa_sim_PinType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Type of PUK.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_SIM_PUK=0,    ///< PUK id
    PA_SIM_PUK2      ///< PUK2 id
}
pa_sim_PukType_t;


//--------------------------------------------------------------------------------------------------
/**
 * Type definition for an Identification number of the SIM card (20 digit)
 */
//--------------------------------------------------------------------------------------------------
typedef char pa_sim_CardId_t[PA_SIM_CARDID_MAX_LEN+1];

//--------------------------------------------------------------------------------------------------
/**
 * Type definition for an International Mobile Subscriber Identity (16 digit)
 */
//--------------------------------------------------------------------------------------------------
typedef char pa_sim_Imsi_t[PA_SIM_IMSI_MAX_LEN+1];

//--------------------------------------------------------------------------------------------------
/**
 * Type definition for a pin code (8 digit max)
 *
 */
//--------------------------------------------------------------------------------------------------
typedef char pa_sim_Pin_t[PA_SIM_PIN_MAX_LEN+1];

//--------------------------------------------------------------------------------------------------
/**
 * Type definition for a puk code (8 digit max)
 *
 */
//--------------------------------------------------------------------------------------------------
typedef char pa_sim_Puk_t[PA_SIM_PUK_MAX_LEN+1];

//--------------------------------------------------------------------------------------------------
/**
 * Event type used for new SIM state notification.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t         num;     ///< The SIM card number.
    le_sim_States_t  state;   ///< The SIM state.
}
pa_sim_Event_t;

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report that a new SIM state notification.
 *
 * @param event The event notification.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_sim_NewStateHdlrFunc_t)(pa_sim_Event_t* eventPtr);

//--------------------------------------------------------------------------------------------------
/**
 * This function counts number of sim card available
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return number           number of count slots
 */
//--------------------------------------------------------------------------------------------------
uint32_t pa_sim_CountSlots
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function selects the Card on which all further SIM operations have to be operated.
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_SelectCard
(
    uint32_t  cardNum     ///< [IN] The card number to be selected.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function get the card on which operations are operated.
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetSelectedCard
(
    uint32_t*  cardNumPtr     ///< [OUT] The card number selected.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function get the card identification (ICCID).
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetCardIdentification
(
    pa_sim_CardId_t iccid     ///< [OUT] ICCID value
);

//--------------------------------------------------------------------------------------------------
/**
 * This function get the International Mobile Subscriber Identity (IMSI).
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetIMSI
(
    pa_sim_Imsi_t imsi   ///< [OUT] IMSI value
);

//--------------------------------------------------------------------------------------------------
/**
 * This function get the SIM Status.
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetState
(
    le_sim_States_t* statePtr    ///< [OUT] SIM state
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for new SIM state notification handling.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_sim_AddNewStateHandler
(
    pa_sim_NewStateHdlrFunc_t handler ///< [IN] The handler function.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for new SIM state notification handling.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_RemoveNewStateHandler
(
    le_event_HandlerRef_t handlerRef
);

//--------------------------------------------------------------------------------------------------
/**
 * This function enter the PIN code.
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_EnterPIN
(
    pa_sim_PinType_t   type,  ///< [IN] pin type
    const pa_sim_Pin_t pin    ///< [IN] pin code
);

//--------------------------------------------------------------------------------------------------
/**
 * This function set the new PIN code.
 *
 *  - use to set pin code by providing the PUK
 *
 * All depends on SIM state which must be retrieved by @ref pa_sim_GetState
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_EnterPUK
(
    pa_sim_PukType_t   type, ///< [IN] puk type
    const pa_sim_Puk_t puk,  ///< [IN] PUK code
    const pa_sim_Pin_t pin   ///< [IN] new PIN code
);

//--------------------------------------------------------------------------------------------------
/**
 * This function get the remaining attempts of a pin code.
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received from the SIM card.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetPINRemainingAttempts
(
    pa_sim_PinType_t type,       ///< [IN] The pin type
    uint32_t*        attemptsPtr ///< [OUT] The number of attempts still possible
);

//--------------------------------------------------------------------------------------------------
/**
 * This function get the remaining attempts of a puk code.
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received from the SIM card.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetPUKRemainingAttempts
(
    pa_sim_PukType_t type,       ///< [IN] The puk type
    uint32_t*        attemptsPtr ///< [OUT] The number of attempts still possible
);

//--------------------------------------------------------------------------------------------------
/**
 * This function change a code.
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received from the SIM card.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_ChangePIN
(
    pa_sim_PinType_t   type,    ///< [IN] The code type
    const pa_sim_Pin_t oldcode, ///< [IN] Old code
    const pa_sim_Pin_t newcode  ///< [IN] New code
);

//--------------------------------------------------------------------------------------------------
/**
 * This function enables PIN locking (PIN or PIN2).
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received from the SIM card.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_EnablePIN
(
    pa_sim_PinType_t   type,  ///< [IN] The pin type
    const pa_sim_Pin_t code   ///< [IN] code
);

//--------------------------------------------------------------------------------------------------
/**
 * This function disables PIN locking (PIN or PIN2).
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received from the SIM card.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_DisablePIN
(
    pa_sim_PinType_t   type,  ///< [IN] The code type.
    const pa_sim_Pin_t code   ///< [IN] code
);


#endif // LEGATO_PASIM_INCLUDE_GUARD
