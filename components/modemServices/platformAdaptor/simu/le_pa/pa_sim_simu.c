/**
 * @file pa_sim_simu.c
 *
 * Simulation implementation of @ref c_pa_sim API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "pa_sim.h"
#include "pa_sim_simu.h"

#define PIN_REMAINING_ATTEMPS_DEFAULT   3
#define PUK_REMAINING_ATTEMPS_DEFAULT   3

static uint32_t PinRemainingAttempts = PIN_REMAINING_ATTEMPS_DEFAULT;
static uint32_t PukRemainingAttempts = PUK_REMAINING_ATTEMPS_DEFAULT;
static le_sim_Id_t SelectedCard = 1;
static le_sim_States_t SimState;
static char HomeMcc[LE_MRC_MCC_BYTES]="";
static char HomeMnc[LE_MRC_MNC_BYTES]="";
static pa_sim_Imsi_t Imsi = "";
static pa_sim_CardId_t Iccid = "";
static char PhoneNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = "";
static char* HomeNetworkOperator = NULL;
static char Pin[PA_SIM_PIN_MAX_LEN+1]="";
static char Puk[PA_SIM_PUK_MAX_LEN+1]="";
static bool StkConfirmation = false;
static le_event_Id_t SimToolkitEvent;
static pa_sim_NewStateHdlrFunc_t SimStateHandler;
static le_mem_PoolRef_t SimStateEventPool;

//--------------------------------------------------------------------------------------------------
/**
 * Set the Puk code
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_simSimu_SetPuk
(
    char* puk
)
{
    strncpy(Puk, puk, PA_SIM_PUK_MAX_LEN);
    Puk[PA_SIM_PUK_MAX_LEN]='\0';
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Pin code
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_simSimu_SetPin
(
    char* pin
)
{
    strncpy(Pin, pin, LE_SIM_PIN_MIN_LEN);
    Pin[LE_SIM_PIN_MIN_LEN]='\0';
}

//--------------------------------------------------------------------------------------------------
/**
 * Select the Sim
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_simSimu_SetSelectCard
(
    le_sim_Id_t simId
)
{
    SelectedCard = simId;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function selects the Card on which all further SIM operations have to be operated.
 *
 * @return
 * - LE_OK            The function succeeded.
 * - LE_FAULT         on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_SelectCard
(
    le_sim_Id_t  sim     ///< The SIM to be selected
)
{
    LE_ASSERT(sim == SelectedCard);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function get the card on which operations are operated.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetSelectedCard
(
    le_sim_Id_t*  simIdPtr     ///< [OUT] The SIM identifier selected.
)
{
    *simIdPtr = SelectedCard;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Report the Sim state
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_simSimu_ReportSimState
(
    le_sim_States_t state
)
{
    SimState = state;

    if (SimStateHandler)
    {
        pa_sim_Event_t* eventPtr = le_mem_ForceAlloc(SimStateEventPool);
        eventPtr->simId   = SelectedCard;
        eventPtr->state     = SimState;

        SimStateHandler(eventPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Report the Stk event
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_simSimu_ReportStkEvent
(
    le_sim_StkEvent_t  leStkEvent
)
{
    pa_sim_StkEvent_t  stkEvent;
    stkEvent.simId = SelectedCard;
    stkEvent.stkEvent = leStkEvent;

    le_event_Report(SimToolkitEvent, &stkEvent, sizeof(stkEvent));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function set the card identification (ICCID).
 *
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_simSimu_SetCardIdentification
(
    pa_sim_CardId_t iccid
)
{
    memcpy(Iccid, iccid, sizeof(pa_sim_CardId_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function get the card identification (ICCID).
 *
 *
 * @return
 *      \return LE_NOT_POSSIBLE  The function failed to get the value.
 *      \return LE_COMM_ERROR    The communication device has returned an error.
 *      \return LE_TIMEOUT       No response was received from the SIM card.
 *      \return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetCardIdentification
(
    pa_sim_CardId_t iccid     ///< [OUT] ICCID value
)
{
    switch (SimState)
    {
        case LE_SIM_BLOCKED:
        case LE_SIM_INSERTED:
        case LE_SIM_READY:
            break;
        default:
            return LE_NOT_POSSIBLE;
    }

    memcpy(iccid, Iccid, sizeof(pa_sim_CardId_t));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function set the International Mobile Subscriber Identity (IMSI).
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_simSimu_SetIMSI
(
    pa_sim_Imsi_t imsi   ///< [OUT] IMSI value
)
{
    strncpy(Imsi, imsi, sizeof(pa_sim_Imsi_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function get the International Mobile Subscriber Identity (IMSI).
 *
 *
 * @return
 *      \return LE_NOT_POSSIBLE  The function failed to get the value.
 *      \return LE_COMM_ERROR    The communication device has returned an error.
 *      \return LE_TIMEOUT       No response was received from the SIM card.
 *      \return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetIMSI
(
    pa_sim_Imsi_t imsi   ///< [OUT] IMSI value
)
{
    switch (SimState)
    {
        case LE_SIM_READY:
            break;
        default:
            return LE_NOT_POSSIBLE;
    }

    strncpy(imsi, Imsi, sizeof(pa_sim_Imsi_t));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function get the SIM Status.
 *
 *
 * @return
 *      \return LE_NOT_POSSIBLE  The function failed to get the value.
 *      \return LE_COMM_ERROR    The communication device has returned an error.
 *      \return LE_TIMEOUT       No response was received from the SIM card.
 *      \return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetState
(
    le_sim_States_t* statePtr    ///< [OUT] SIM state
)
{
    *statePtr = SimState;

    return LE_OK;
}

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
)
{
    SimStateHandler = handler;

    // just for returning something
    return (le_event_HandlerRef_t) SimStateHandler;
}

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
)
{
    SimStateHandler = NULL;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function enter the PIN code.
 *
 *
 * @return
 *      \return LE_BAD_PARAMETER The parameter is invalid.
 *      \return LE_NOT_POSSIBLE  The function failed to enter the value.
 *      \return LE_COMM_ERROR    The communication device has returned an error.
 *      \return LE_TIMEOUT       No response was received from the SIM card.
 *      \return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_EnterPIN
(
    pa_sim_PinType_t   type,  ///< [IN] pin type
    const pa_sim_Pin_t pin    ///< [IN] pin code
)
{
    switch (SimState)
    {
        case LE_SIM_INSERTED:
            break;
        default:
            return LE_NOT_POSSIBLE;
    }

    // add a function to check the PIN
    if (strncmp(Pin, pin, strlen(Pin) ) != 0)
    {
        if (PinRemainingAttempts == 1)
        {
            /* Blocked */
            pa_simSimu_ReportSimState(LE_SIM_BLOCKED);
        }

        PinRemainingAttempts--;

        return LE_BAD_PARAMETER;
    }
    else
    {
        PinRemainingAttempts = PIN_REMAINING_ATTEMPS_DEFAULT;
        pa_simSimu_ReportSimState(LE_SIM_READY);
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function set the new PIN code.
 *
 *  - use to set pin code by providing the PUK
 *
 * All depends on SIM state which must be retrieved by @ref pa_sim_GetState
 *
 * @return
 *      \return LE_NOT_POSSIBLE  The function failed to set the value.
 *      \return LE_BAD_PARAMETER The parameters are invalid.
 *      \return LE_COMM_ERROR    The communication device has returned an error.
 *      \return LE_TIMEOUT       No response was received from the SIM card.
 *      \return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_EnterPUK
(
    pa_sim_PukType_t   type, ///< [IN] puk type
    const pa_sim_Puk_t puk,  ///< [IN] PUK code
    const pa_sim_Pin_t pin   ///< [IN] new PIN code
)
{
    switch (SimState)
    {
        case LE_SIM_BLOCKED:
            break;
        default:
            return LE_NOT_POSSIBLE;
    }

    /* Check PUK code is valid */
    if (strncmp(puk, Puk, sizeof(pa_sim_Puk_t)) != 0)
    {
        if (PukRemainingAttempts <= 1)
        {
            /* TODO */
            PukRemainingAttempts = PUK_REMAINING_ATTEMPS_DEFAULT;
        }
        else
        {
            PukRemainingAttempts--;
        }

        return LE_BAD_PARAMETER;
    }

    PinRemainingAttempts = PUK_REMAINING_ATTEMPS_DEFAULT;
    pa_simSimu_ReportSimState(LE_SIM_READY);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function get the remaining attempts of a pin code.
 *
 *
 * @return
 *      \return LE_NOT_POSSIBLE  The function failed to get the value.
 *      \return LE_BAD_PARAMETER The 'type' parameter is invalid.
 *      \return LE_COMM_ERROR    The communication device has returned an error.
 *      \return LE_TIMEOUT       No response was received from the SIM card.
 *      \return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetPINRemainingAttempts
(
    pa_sim_PinType_t type,       ///< [IN] The pin type
    uint32_t*        attemptsPtr ///< [OUT] The number of attempts still possible
)
{
    switch (SimState)
    {
        case LE_SIM_BUSY:
        case LE_SIM_STATE_UNKNOWN:
            return LE_NOT_POSSIBLE;
        default:
        break;
    }

    *attemptsPtr = PinRemainingAttempts;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function get the remaining attempts of a puk code.
 *
 *
 * @return
 *      \return LE_NOT_POSSIBLE  The function failed to get the value.
 *      \return LE_BAD_PARAMETER The 'type' parameter is invalid.
 *      \return LE_COMM_ERROR    The communication device has returned an error.
 *      \return LE_TIMEOUT       No response was received from the SIM card.
 *      \return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetPUKRemainingAttempts
(
    pa_sim_PukType_t type,       ///< [IN] The puk type
    uint32_t*        attemptsPtr ///< [OUT] The number of attempts still possible
)
{
    switch (SimState)
    {
        case LE_SIM_BUSY:
        case LE_SIM_STATE_UNKNOWN:
            return LE_NOT_POSSIBLE;
        default:
        break;
    }

    *attemptsPtr = (PukRemainingAttempts-1);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function change a code.
 *
 * @return
 *      \return LE_NOT_POSSIBLE  The function failed to set the value.
 *      \return LE_BAD_PARAMETER The parameters are invalid.
 *      \return LE_COMM_ERROR    The communication device has returned an error.
 *      \return LE_TIMEOUT       No response was received from the SIM card.
 *      \return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_ChangePIN
(
    pa_sim_PinType_t   type,    ///< [IN] The code type
    const pa_sim_Pin_t oldcode, ///< [IN] Old code
    const pa_sim_Pin_t newcode  ///< [IN] New code
)
{
    switch (SimState)
    {
        case LE_SIM_READY:
            break;
        default:
            return LE_NOT_POSSIBLE;
    }

    if (strncmp(Pin, oldcode, strlen(Pin)) != 0)
    {
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function enables PIN locking (PIN or PIN2).
 *
 *
 * @return
 *      \return LE_NOT_POSSIBLE  The function failed to set the value.
 *      \return LE_BAD_PARAMETER The parameters are invalid.
 *      \return LE_COMM_ERROR    The communication device has returned an error.
 *      \return LE_TIMEOUT       No response was received from the SIM card.
 *      \return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_EnablePIN
(
    pa_sim_PinType_t   type,  ///< [IN] The pin type
    const pa_sim_Pin_t code   ///< [IN] code
)
{
    switch (SimState)
    {
        case LE_SIM_READY:
            break;
        default:
            return LE_NOT_POSSIBLE;
    }

    if (strncmp(code,Pin,strlen(Pin)) != 0)
    {
        return LE_NOT_POSSIBLE;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function disables PIN locking (PIN or PIN2).
 *
 *
 * @return
 *      \return LE_NOT_POSSIBLE  The function failed to set the value.
 *      \return LE_BAD_PARAMETER The parameters are invalid.
 *      \return LE_COMM_ERROR    The communication device has returned an error.
 *      \return LE_TIMEOUT       No response was received from the SIM card.
 *      \return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_DisablePIN
(
    pa_sim_PinType_t   type,  ///< [IN] The code type.
    const pa_sim_Pin_t code   ///< [IN] code
)
{
    if (code[0] == '\0')
        return LE_BAD_PARAMETER;

    switch (SimState)
    {
        case LE_SIM_INSERTED:
        case LE_SIM_READY:
            break;
        default:
            return LE_NOT_POSSIBLE;
    }


    if (strncmp(code,Pin,strlen(Pin)) != 0)
    {
        return LE_NOT_POSSIBLE;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the SIM Phone Number.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_simSimu_SetSubscriberPhoneNumber
(
    char        *phoneNumberStr
)
{
    strncpy(PhoneNumber, phoneNumberStr, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM Phone Number.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Phone Number can't fit in phoneNumberStr
 *      - LE_NOT_POSSIBLE on any other failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetSubscriberPhoneNumber
(
    char        *phoneNumberStr,    ///< [OUT] The phone Number
    size_t       phoneNumberStrSize ///< [IN]  Size of phoneNumberStr
)
{
    switch (SimState)
    {
        case LE_SIM_READY:
            break;
        default:
            return LE_NOT_POSSIBLE;
    }

    if (phoneNumberStrSize < strlen(PhoneNumber))
    {
        return LE_OVERFLOW;
    }

    strncpy(phoneNumberStr, PhoneNumber, phoneNumberStrSize);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the Home Network Name information.
 */
//--------------------------------------------------------------------------------------------------
void pa_simSimu_SetHomeNetworkOperator
(
    char       *nameStr
)
{
    int len = strlen(nameStr)+1;
    HomeNetworkOperator = malloc( len );
    strcpy(HomeNetworkOperator, nameStr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Home Network Name information.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Home Network Name can't fit in nameStr
 *      - LE_NOT_POSSIBLE on any other failure
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetHomeNetworkOperator
(
    char       *nameStr,               ///< [OUT] the home network Name
    size_t      nameStrSize            ///< [IN] the nameStr size
)
{
    switch (SimState)
    {
        case LE_SIM_READY:
            break;
        default:
            return LE_FAULT;
    }

    if ( nameStrSize < strlen(HomeNetworkOperator) )
    {
        return LE_OVERFLOW;
    }

    strncpy(nameStr, HomeNetworkOperator, nameStrSize );

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the Home Network MCC MNC.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_simSimu_SetHomeNetworkMccMnc
(
    char     *mccPtr,
    char     *mncPtr
)
{
    LE_ASSERT((strlen(mccPtr) <= LE_MRC_MCC_BYTES) && (strlen(mncPtr) <= LE_MRC_MNC_BYTES));

    strcpy(HomeMcc, mccPtr);
    strcpy(HomeMnc, mncPtr);
}

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
le_result_t pa_sim_GetHomeNetworkMccMnc
(
    char     *mccPtr,                ///< [OUT] Mobile Country Code
    size_t    mccPtrSize,            ///< [IN] mccPtr buffer size
    char     *mncPtr,                ///< [OUT] Mobile Network Code
    size_t    mncPtrSize             ///< [IN] mncPtr buffer size
)
{
    switch (SimState)
    {
        case LE_SIM_READY:
            break;
        default:
            return LE_FAULT;
    }


    if ( ( mccPtrSize <  strlen(HomeMcc) ) || ( mncPtrSize <  strlen(HomeMnc) ) )
    {
        return LE_OVERFLOW;
    }

    strncpy(mccPtr, HomeMcc, mccPtrSize );
    strncpy(mncPtr, HomeMnc, mncPtrSize );

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to open a logical channel on the SIM card.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for unexpected error
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_OpenLogicalChannel
(
    uint8_t* channelPtr  ///< [OUT] channel number
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to close a logical channel on the SIM card.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for unexpected error
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_CloseLogicalChannel
(
    uint8_t channel  ///< [IN] channel number
)
{
    return LE_OK;
}

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
le_result_t pa_sim_SendApdu
(
    const uint8_t* apduPtr, ///< [IN] APDU message buffer
    uint32_t       apduLen, ///< [IN] APDU message length in bytes
    uint8_t*       respPtr, ///< [OUT] APDU message response.
    size_t*        lenPtr   ///< [IN,OUT] APDU message response length in bytes.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to trigger a SIM refresh.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for unexpected error
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_Refresh
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for SIM Toolkit event notification handling.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_sim_AddSimToolkitEventHandler
(
    pa_sim_SimToolkitEventHdlrFunc_t handler,    ///< [IN] The handler function.
    void*                            contextPtr  ///< [IN] The context to be given to the handler.
)
{
    le_event_HandlerRef_t handlerRef = le_event_AddHandler("SimToolkitEventHandler",
                                           SimToolkitEvent,
                                           (le_event_HandlerFunc_t) handler);

    le_event_SetContextPtr (handlerRef, contextPtr);

    return handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for SIM Toolkit event notification
 * handling.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_RemoveSimToolkitEventHandler
(
    le_event_HandlerRef_t handlerRef
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the expected confirmation command.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_simSimu_SetExpectedStkConfirmationCommand
(
    bool  confirmation ///< [IN] true to accept, false to reject
)
{
    StkConfirmation = confirmation;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to confirm a SIM Toolkit command.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_ConfirmSimToolkitCommand
(
    bool  confirmation ///< [IN] true to accept, false to reject
)
{
    LE_ASSERT(StkConfirmation == confirmation);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * SIM Stub initialization.
 *
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_simSimu_Init
(
    void
)
{
    LE_INFO("PA SIM Init");

    SimStateEventPool = le_mem_CreatePool("SimEventPool", sizeof(pa_sim_Event_t));
    SimToolkitEvent = le_event_CreateId("SimToolkitEvent", sizeof(pa_sim_StkEvent_t));

    return LE_OK;
}


