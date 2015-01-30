/**
 * @file pa_sim_simu.c
 *
 * Simulation implementation of @ref c_pa_sim API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "pa_sim.h"

#include "le_cfg_interface.h"
#include "pa_simu.h"

#define PIN_REMAINING_ATTEMPS_DEFAULT   3
#define PUK_REMAINING_ATTEMPS_DEFAULT   3

static uint32_t PinRemainingAttempts = PIN_REMAINING_ATTEMPS_DEFAULT;
static uint32_t PukRemainingAttempts = PUK_REMAINING_ATTEMPS_DEFAULT;
static le_sim_States_t CurrentState = LE_SIM_INSERTED;
static bool PinSecurity = true;
const le_sim_Type_t SelectedCard = 1;

//--------------------------------------------------------------------------------------------------
/**
 * SIM state event ID used to report SIM state events to the registered event handlers.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t SimStateEvent;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for event state parameters.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SimStateEventPool;

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
    le_sim_Type_t  sim     ///< The SIM to be selected
)
{
    LE_DEBUG("Select Card: %d", sim);

    if(sim != SelectedCard)
        return LE_NOT_POSSIBLE;

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
    le_sim_Type_t*  simTypePtr     ///< [OUT] The SIM type selected.
)
{
    *simTypePtr = SelectedCard;
    return LE_OK;
}

const char * SimStateStrings[LE_SIM_STATE_UNKNOWN] = {
        "INSERTED",
        "ABSENT",
        "READY",
        "BLOCKED",
        "BUSY",
};

static void ReportSimState
(
    void
)
{
    pa_sim_Event_t* eventPtr = le_mem_ForceAlloc(SimStateEventPool);
    eventPtr->simType   = SelectedCard;
    eventPtr->state     = CurrentState;

    // @todo: This needs to change to le_event_ReportWithPoolObj (yet to be implemented).
    LE_DEBUG("Send Event SIM number %d, SIM state %d", eventPtr->simType, eventPtr->state);
    le_event_ReportWithRefCounting(SimStateEvent, eventPtr);
}

static void SetSimState
(
    le_sim_States_t state
);

static le_sim_States_t GetSimState
(
    void
);

static void SetPinSecurity
(
    bool pinSecurity
)
{
    if(PinSecurity == pinSecurity)
        return;

    PinSecurity = pinSecurity;

    le_cfg_QuickSetBool(PA_SIMU_CFG_MODEM_ROOT "/sim/1/pinSecurity", PinSecurity);

    /* Update state accordingly */
    /* => if we are unsecured, INSERTED state doesn't exist */
    if( (!PinSecurity) && (GetSimState() == LE_SIM_INSERTED) )
    {
        SetSimState(LE_SIM_READY);
    }
}

static bool GetPinSecurity
(
    void
)
{
    PinSecurity = le_cfg_QuickGetBool(PA_SIMU_CFG_MODEM_ROOT "/sim/1/pinSecurity", PinSecurity);

    return PinSecurity;
}

static void SetSimState
(
    le_sim_States_t state
)
{
    const char * stateStr = "";

    if ((state < 0) || (state >= LE_SIM_STATE_UNKNOWN))
        stateStr = "UNKNOWN";

    stateStr = SimStateStrings[state];

    le_cfg_QuickSetString(PA_SIMU_CFG_MODEM_ROOT "/sim/1/state", stateStr);

    if(CurrentState != state)
    {
        LE_INFO("New SIM state: %s", stateStr);

        CurrentState = state;
        ReportSimState();
    }
}

static le_sim_States_t GetSimState
(
    void
)
{
    le_result_t res;
    char stateStr[513];
    le_sim_States_t newState = LE_SIM_STATE_UNKNOWN;
    int i;

    res = le_cfg_QuickGetString(PA_SIMU_CFG_MODEM_ROOT "/sim/1/state", stateStr, sizeof(stateStr), PA_SIMU_SIM_DEFAULT_STATE);
    if (res != LE_OK)
    {
        LE_FATAL("Unable to get SIM state");
    }

    for (i = 0; i < LE_SIM_STATE_UNKNOWN; i++)
    {
        if (strncmp(stateStr, SimStateStrings[i], sizeof(stateStr)) == 0)
        {
            newState = (le_sim_States_t) i;
            break;
        }
    }

    if(newState != CurrentState)
    {
        if(newState == LE_SIM_INSERTED)
        {
            SetPinSecurity(true);
        }

        CurrentState = newState;
        ReportSimState();
    }

    return newState;
}

static le_result_t CheckPIN
(
    const pa_sim_Pin_t pin    ///< [IN] pin code
)
{
    char simPin[PA_SIM_PIN_MAX_LEN + 1];
    le_result_t res;

    LE_DEBUG("Checking PIN %s", pin);

    res = le_cfg_QuickGetString(PA_SIMU_CFG_MODEM_ROOT "/sim/1/pinCode", simPin, sizeof(pa_sim_Pin_t), PA_SIMU_SIM_DEFAULT_PIN);
    if (res != LE_OK)
    {
        LE_FATAL("Unable to get PIN code");
    }

    /* Check SIM code is valid */
    if (strncmp(pin, simPin, sizeof(pa_sim_Pin_t)) != 0)
    {
        LE_DEBUG("Bad Pin (expected %s)", simPin);
        return LE_BAD_PARAMETER;
    }

    LE_DEBUG("Pin OK");
    return LE_OK;
}

static le_result_t SetPIN
(
    const pa_sim_Pin_t pin    ///< [IN] pin code
)
{
    le_cfg_QuickSetString(PA_SIMU_CFG_MODEM_ROOT "/sim/1/pinCode", pin);
    return LE_OK;
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
    le_result_t res;

    switch (GetSimState())
    {
        case LE_SIM_BLOCKED:
        case LE_SIM_INSERTED:
        case LE_SIM_READY:
            break;
        default:
            return LE_NOT_POSSIBLE;
    }

    res = le_cfg_QuickGetString(PA_SIMU_CFG_MODEM_ROOT "/sim/1/iccid", iccid, sizeof(pa_sim_CardId_t), PA_SIMU_SIM_DEFAULT_ICCID);
    if (res != LE_OK)
    {
        LE_FATAL("Unable to get ICCID");
    }
    else
    {
        LE_DEBUG("ICCID: %s", iccid);
    }

    switch (res)
    {
        case LE_OK:
            break;

        default:
            LE_FATAL("Unexpected result: %i", res);
            break;
    }

    return LE_OK;
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
    le_result_t res;

    switch (GetSimState())
    {
        case LE_SIM_READY:
            break;
        default:
            return LE_NOT_POSSIBLE;
    }

    res = le_cfg_QuickGetString(PA_SIMU_CFG_MODEM_ROOT "/sim/1/imsi", imsi, sizeof(pa_sim_Imsi_t), PA_SIMU_SIM_DEFAULT_IMSI);
    if (res != LE_OK)
    {
        LE_FATAL("Unable to get IMSI");
    }
    else
    {
        LE_DEBUG("IMSI: %s", imsi);
    }

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
    *statePtr = GetSimState();

    if(*statePtr == LE_SIM_STATE_UNKNOWN)
    {
        return LE_NOT_POSSIBLE;
    }

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
    LE_FATAL_IF( (handler == NULL), "new SIM State handler is NULL");

    return le_event_AddHandler("SimStateHandler",
                               SimStateEvent,
                               (le_event_HandlerFunc_t) handler);
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
    le_event_RemoveHandler(handlerRef);

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
    le_result_t res;

    switch (GetSimState())
    {
        case LE_SIM_INSERTED:
            break;
        default:
            return LE_NOT_POSSIBLE;
    }

    res = CheckPIN(pin);

    /* Check SIM code is valid */
    switch (res)
    {
        case LE_COMM_ERROR:
            break;

        case LE_BAD_PARAMETER:
            if (PinRemainingAttempts == 1)
            {
                /* Blocked */
                SetSimState(LE_SIM_BLOCKED);

                PinRemainingAttempts = PIN_REMAINING_ATTEMPS_DEFAULT;
                PukRemainingAttempts = PUK_REMAINING_ATTEMPS_DEFAULT;
            }
            else
            {
                PinRemainingAttempts--;
            }

            break;

        case LE_OK:
            PinRemainingAttempts = PIN_REMAINING_ATTEMPS_DEFAULT;
            SetSimState(LE_SIM_READY);
            break;

        default:
            LE_FATAL("Unexpected result %d", res);
    }

    return res;
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
    le_result_t res;
    char simPuk[PA_SIM_PUK_MAX_LEN + 1];

    switch (GetSimState())
    {
        case LE_SIM_BLOCKED:
            break;
        default:
            return LE_NOT_POSSIBLE;
    }

    res = le_cfg_QuickGetString(PA_SIMU_CFG_MODEM_ROOT "/sim/1/pukCode", simPuk, sizeof(pa_sim_Puk_t), PA_SIMU_SIM_DEFAULT_PUK);
    if (res != LE_OK)
    {
        LE_FATAL("Unable to get PUK code");
    }

    /* Check PUK code is valid */
    if (strncmp(puk, simPuk, sizeof(pa_sim_Puk_t)) != 0)
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

    /* Set new PIN code */
    res = SetPIN(pin);
    if (res != LE_OK)
    {
        LE_FATAL("Unable to set new PIN");
    }

    SetSimState(LE_SIM_READY);

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
    *attemptsPtr = (PinRemainingAttempts-1);
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
    le_result_t res;

    switch (GetSimState())
    {
        case LE_SIM_READY:
            break;
        default:
            return LE_NOT_POSSIBLE;
    }

    res = CheckPIN(oldcode);

    if (res != LE_OK)
        return res;

    /* Set new PIN code */
    return SetPIN(newcode);
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
    le_result_t res;

    switch (GetSimState())
    {
        case LE_SIM_READY:
            break;
        default:
            return LE_NOT_POSSIBLE;
    }

    if(PinSecurity)
    {
        LE_WARN("PIN Security already enabled");
        return LE_NOT_POSSIBLE;
    }

    res = SetPIN(code);

    if (res != LE_OK)
        return LE_NOT_POSSIBLE;

    SetPinSecurity(true);
    return res;
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
    le_result_t res;

    if (code[0] == '\0')
        return LE_BAD_PARAMETER;

    switch (GetSimState())
    {
        case LE_SIM_INSERTED:
        case LE_SIM_READY:
            break;
        default:
            return LE_NOT_POSSIBLE;
    }

    if(!PinSecurity)
    {
        return LE_NOT_POSSIBLE;
    }

    res = CheckPIN(code);

    if (res != LE_OK)
        return LE_BAD_PARAMETER;

    SetPinSecurity(false);
    return res;
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
    le_result_t res;

    res = le_cfg_QuickGetString(PA_SIMU_CFG_MODEM_ROOT "/sim/1/number", phoneNumberStr, phoneNumberStrSize, PA_SIMU_SIM_DEFAULT_NUM);
    if (res != LE_OK)
    {
        LE_FATAL("Unable to get subscriber phone number");
    }

    return LE_OK;
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
    le_result_t res;

    res = le_cfg_QuickGetString(PA_SIMU_CFG_MODEM_ROOT "/sim/carrierName", nameStr, nameStrSize, PA_SIMU_SIM_DEFAULT_CARRIER);
    if (res != LE_OK)
    {
        LE_FATAL("Unable to get carrier name");
    }

    return LE_OK;
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
    le_result_t res;

    res = le_cfg_QuickGetString(PA_SIMU_CFG_MODEM_ROOT "/sim/mcc", mccPtr, mccPtrSize, PA_SIMU_SIM_DEFAULT_MCC);
    if (res != LE_OK)
    {
        LE_FATAL("Unable to get MCC");
    }

    res = le_cfg_QuickGetString(PA_SIMU_CFG_MODEM_ROOT "/sim/mnc", mncPtr, mncPtrSize, PA_SIMU_SIM_DEFAULT_MNC);
    if (res != LE_OK)
    {
        LE_FATAL("Unable to get MNC");
    }

    return LE_OK;
}


/**
 * SIM Stub initialization.
 *
 * @return LE_OK           The function succeeded.
 */
le_result_t sim_simu_Init
(
    void
)
{
    LE_INFO("PA SIM Init");

    SimStateEvent = le_event_CreateIdWithRefCounting("SimStateEvent");
    SimStateEventPool = le_mem_CreatePool("SimEventPool", sizeof(pa_sim_Event_t));

    /* Read initial tree */
    GetPinSecurity();
    GetSimState();

    return LE_OK;
}

