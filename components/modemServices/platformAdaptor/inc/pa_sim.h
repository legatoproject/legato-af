/**
 * @page c_pa_sim Modem SIM Platform Adapter API
 *
 * @ref pa_sim.h "API Reference"
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
 * Copyright (C) Sierra Wireless Inc.
 */


/** @file pa_sim.h
 *
 * Legato @ref c_pa_sim include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PASIM_INCLUDE_GUARD
#define LEGATO_PASIM_INCLUDE_GUARD


#include "legato.h"
#include "interfaces.h"

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
 * Maximum EID Code length.
 *
 */
//--------------------------------------------------------------------------------------------------
#define PA_SIM_EID_MAX_LEN     32

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
 */
//--------------------------------------------------------------------------------------------------
typedef char pa_sim_Pin_t[PA_SIM_PIN_MAX_LEN+1];

//--------------------------------------------------------------------------------------------------
/**
 * Type definition for a puk code (8 digit max)
 */
//--------------------------------------------------------------------------------------------------
typedef char pa_sim_Puk_t[PA_SIM_PUK_MAX_LEN+1];

//--------------------------------------------------------------------------------------------------
/**
 * Type definition for EID code (32 digits)
 */
//--------------------------------------------------------------------------------------------------
typedef char pa_sim_Eid_t[PA_SIM_EID_MAX_LEN+1];

//--------------------------------------------------------------------------------------------------
/**
 * Event type used for new SIM state notification.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sim_Id_t      simId;   ///< The SIM identifier
    le_sim_States_t  state;   ///< The SIM state
}
pa_sim_Event_t;

//--------------------------------------------------------------------------------------------------
/**
 * Event type used for SIM Toolkit notification.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sim_Id_t              simId;             ///< The SIM identifier
    le_sim_StkEvent_t        stkEvent;          ///< The SIM Toolkit event
    le_sim_StkRefreshMode_t  stkRefreshMode;    ///< The SIM Toolkit Refresh mode
    le_sim_StkRefreshStage_t stkRefreshStage;   ///< The SIM Toolkit Refresh stage
}
pa_sim_StkEvent_t;

//--------------------------------------------------------------------------------------------------
/**
 * Mobile code.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char mcc[LE_MRC_MCC_BYTES]; ///< MCC: Mobile Country Code
    char mnc[LE_MRC_MNC_BYTES]; ///< MNC: Mobile Network Code
}
pa_sim_MobileCode_t;

//--------------------------------------------------------------------------------------------------
/**
 * FPLMN operators.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    pa_sim_MobileCode_t mobileCode; ///< Mobile code
    le_dls_Link_t       link;       ///< link for the FPLMN list
}
pa_sim_FPLMNOperator_t;

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report a new SIM state notification.
 *
 * @param event The event notification.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_sim_NewStateHdlrFunc_t)(pa_sim_Event_t* eventPtr);

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report a SIM Toolkit event.
 *
 * @param event The SIM Toolkit notification.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_sim_SimToolkitEventHdlrFunc_t)(pa_sim_StkEvent_t* eventPtr);


//--------------------------------------------------------------------------------------------------
/**
 * This function selects the Card on which all further SIM operations have to be operated.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
  */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_SelectCard
(
    le_sim_Id_t      simId   ///< [IN]  The SIM identififier
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the card on which operations are operated.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_GetSelectedCard
(
    le_sim_Id_t*  simIdPtr     ///< [OUT] The SIM identififier.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the card identification (ICCID).
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_GetCardIdentification
(
    pa_sim_CardId_t iccid     ///< [OUT] ICCID value
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the International Mobile Subscriber Identity (IMSI).
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_GetIMSI
(
    pa_sim_Imsi_t imsi        ///< [OUT] IMSI value
);

//--------------------------------------------------------------------------------------------------
/**
 * This function rerieves the identifier for the embedded Universal Integrated Circuit Card (EID)
 * (16 digits)
 *
 * @return LE_OK            The function succeeded.
 * @return LE_FAULT         The function failed.
 * @return LE_UNSUPPORTED   The platform does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_GetCardEID
(
   pa_sim_Eid_t eid           ///< [OUT] the EID value
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the SIM Status.
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_GetState
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
LE_SHARED le_event_HandlerRef_t pa_sim_AddNewStateHandler
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
LE_SHARED le_result_t pa_sim_RemoveNewStateHandler
(
    le_event_HandlerRef_t handlerRef
);

//--------------------------------------------------------------------------------------------------
/**
 * This function enter the PIN code.
 *
 * @return LE_OK            The function succeeded.
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response received from the SIM card.
  */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_EnterPIN
(
    pa_sim_PinType_t   type,  ///< [IN] pin type
    const pa_sim_Pin_t pin    ///< [IN] pin code
);

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the new PIN code.
 *
 *  - use to set pin code by providing the PUK
 *
 * All depends on SIM state which must be retrieved by @ref pa_sim_GetState
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received from the SIM card.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_EnterPUK
(
    pa_sim_PukType_t   type, ///< [IN] puk type
    const pa_sim_Puk_t puk,  ///< [IN] PUK code
    const pa_sim_Pin_t pin   ///< [IN] new PIN code
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the remaining attempts of a pin code.
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_GetPINRemainingAttempts
(
    pa_sim_PinType_t type,       ///< [IN] The pin type
    uint32_t*        attemptsPtr ///< [OUT] The number of attempts still possible
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the remaining attempts of a puk code.
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_GetPUKRemainingAttempts
(
    pa_sim_PukType_t type,       ///< [IN] The puk type
    uint32_t*        attemptsPtr ///< [OUT] The number of attempts still possible
);

//--------------------------------------------------------------------------------------------------
/**
 * This function change a code.
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_FAULT         The function failed to set the value.
 * @return LE_TIMEOUT       No response was received from the SIM card.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_ChangePIN
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
 * @return LE_FAULT         The function failed to set the value.
 * @return LE_TIMEOUT       No response was received from the SIM card.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_EnablePIN
(
    pa_sim_PinType_t   type,  ///< [IN] The pin type
    const pa_sim_Pin_t code   ///< [IN] code
);

//--------------------------------------------------------------------------------------------------
/**
 * This function disables PIN locking (PIN or PIN2).
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_FAULT         The function failed to set the value.
 * @return LE_TIMEOUT       No response was received from the SIM card.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_DisablePIN
(
    pa_sim_PinType_t   type,  ///< [IN] The code type.
    const pa_sim_Pin_t code   ///< [IN] code
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM Phone Number.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Phone Number can't fit in phoneNumberStr
 *      - LE_FAULT on any other failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_GetSubscriberPhoneNumber
(
    char        *phoneNumberStr,    ///< [OUT] The phone Number
    size_t       phoneNumberStrSize ///< [IN]  Size of phoneNumberStr
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Home Network Name information.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Home Network Name can't fit in nameStr
 *      - LE_FAULT on any other failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_GetHomeNetworkOperator
(
    char       *nameStr,               ///< [OUT] the home network Name
    size_t      nameStrSize            ///< [IN]  the nameStr size
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Home Network MCC MNC.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Home Network MCC/MNC can't fit in mccPtr and mncPtr
 *      - LE_FAULT for unexpected error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_GetHomeNetworkMccMnc
(
    char     *mccPtr,                ///< [OUT] Mobile Country Code
    size_t    mccPtrSize,            ///< [IN] mccPtr buffer size
    char     *mncPtr,                ///< [OUT] Mobile Network Code
    size_t    mncPtrSize             ///< [IN] mncPtr buffer size
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to open a logical channel on the SIM card.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for unexpected error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_OpenLogicalChannel
(
    uint8_t* channelPtr  ///< [OUT] channel number
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to close a logical channel on the SIM card.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for unexpected error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_CloseLogicalChannel
(
    uint8_t channel  ///< [IN] channel number
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to send an APDU message to the SIM card.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW the response length exceed the maximum buffer length.
 *      - LE_FAULT for unexpected error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_SendApdu
(
    uint8_t        channel, ///< [IN] Logical channel.
    const uint8_t* apduPtr, ///< [IN] APDU message buffer
    uint32_t       apduLen, ///< [IN] APDU message length in bytes
    uint8_t*       respPtr, ///< [OUT] APDU message response.
    size_t*        lenPtr   ///< [IN,OUT] APDU message response length in bytes.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to trigger a SIM refresh.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for unexpected error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_Refresh
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for SIM Toolkit event notification handling.
 *
 * @return
 *      - A handler reference on success, which is only needed for later removal of the handler.
 *      - NULL on failure
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_event_HandlerRef_t pa_sim_AddSimToolkitEventHandler
(
    pa_sim_SimToolkitEventHdlrFunc_t handler,    ///< [IN] The handler function.
    void*                            contextPtr  ///< [IN] The context to be given to the handler.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for SIM Toolkit event notification
 * handling.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_RemoveSimToolkitEventHandler
(
    le_event_HandlerRef_t handlerRef
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to confirm a SIM Toolkit command.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_ConfirmSimToolkitCommand
(
    bool  confirmation ///< [IN] true to accept, false to reject
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to send a generic command to the SIM.
 *
 * @return
 *      - LE_OK             Function succeeded.
 *      - LE_FAULT          The function failed.
 *      - LE_BAD_PARAMETER  A parameter is invalid.
 *      - LE_NOT_FOUND      - The function failed to select the SIM card for this operation
 *                          - The requested SIM file is not found
 *      - LE_OVERFLOW       Response buffer is too small to copy the SIM answer.
 *      - LE_UNSUPPORTED    The platform does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_SendCommand
(
    le_sim_Command_t command,               ///< [IN] The SIM command
    const char*      fileIdentifierPtr,     ///< [IN] File identifier
    uint8_t          p1,                    ///< [IN] Parameter P1 passed to the SIM
    uint8_t          p2,                    ///< [IN] Parameter P2 passed to the SIM
    uint8_t          p3,                    ///< [IN] Parameter P3 passed to the SIM
    const uint8_t*   dataPtr,               ///< [IN] Data command
    size_t           dataNumElements,       ///< [IN] Size of data command
    const char*      pathPtr,               ///< [IN] Path of the elementary file
    uint8_t*         sw1Ptr,                ///< [OUT] SW1 received from the SIM
    uint8_t*         sw2Ptr,                ///< [OUT] SW2 received from the SIM
    uint8_t*         responsePtr,           ///< [OUT] SIM response
    size_t*          responseNumElementsPtr ///< [IN/OUT] Size of response
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to reset UIM.
 *
 * @return
 *      - LE_OK          On success.
 *      - LE_FAULT       On failure.
 *      - LE_UNSUPPORTED The platform does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_Reset
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to write the FPLMN list.
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_FAULT         If FPLMN list is not able to write into device.
 *      - LE_BAD_PARAMETER A parameter is invalid.
 *      - LE_UNSUPPORTED   The platform does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_WriteFPLMNList
(
    le_dls_List_t *FPLMNListPtr   ///< [IN] FPLMN list.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the number of FPLMN operators present in the list.
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_FAULT         On failure.
 *      - LE_BAD_PARAMETER A parameter is invalid.
 *      - LE_UNSUPPORTED   The platform does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_CountFPLMNOperators
(
    uint32_t*  nbItemPtr     ///< [OUT] number of FPLMN operator found if success.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to read the FPLMN list.
 *
 * @return
 *      - LE_OK            On success.
 *      - LE_NOT_FOUND     If no FPLMN network is available.
 *      - LE_BAD_PARAMETER A parameter is invalid.
 *      - LE_UNSUPPORTED   The platform does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_ReadFPLMNOperators
(
    pa_sim_FPLMNOperator_t* FPLMNOperatorPtr,   ///< [OUT] FPLMN operators.
    uint32_t* FPLMNOperatorCountPtr             ///< [IN/OUT] FPLMN operator count.
);

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the last SIM Toolkit status.
 *
 * @return
 *      - LE_OK             On success.
 *      - LE_BAD_PARAMETER  A parameter is invalid.
 *      - LE_UNSUPPORTED    The platform does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_GetLastStkStatus
(
    pa_sim_StkEvent_t*  stkStatus  ///< [OUT] last SIM Toolkit event status
);

//--------------------------------------------------------------------------------------------------
/**
 * Powers up or down the current SIM card.
 *
 * @return
 *      - LE_OK           On success
 *      - LE_FAULT        For unexpected error
 *      - LE_UNSUPPORTED  The platform does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_SetPower
(
    le_onoff_t power     ///< [IN] The power state.
);

//--------------------------------------------------------------------------------------------------
/**
 * Enable or disable the automatic SIM selection
 *
 * @return
 *      - LE_OK          Function succeeded.
 *      - LE_FAULT       Function failed to execute.
 *      - LE_UNSUPPORTED The platform does not support this operation.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_SetAutomaticSelection
(
    bool    enable    ///< [IN] True if the feature needs to be enabled. False otherwise.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the automatic SIM selection
 *
 * @return
 *      - LE_OK             Function succeeded.
 *      - LE_FAULT          Function failed to execute.
 *      - LE_BAD_PARAMETER  Invalid parameter.
 *      - LE_UNSUPPORTED    The platform does not support this operation.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_sim_GetAutomaticSelection
(
    bool*    enablePtr    ///< [OUT] True if the feature is enabled. False otherwise.
);

#endif // LEGATO_PASIM_INCLUDE_GUARD
