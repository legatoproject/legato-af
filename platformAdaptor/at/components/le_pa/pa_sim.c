/** @file pa_sim.c
 *
 * AT implementation of c_pa_sim API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


#include "legato.h"
#include "le_atClient.h"

#include "pa_sim.h"
#include "pa_utils_local.h"

#define DEFAULT_SIMEVENT_POOL_SIZE  1


//--------------------------------------------------------------------------------------------------
/**
 * Sim memory pool
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   SimEventPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Unsolicited event
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t      EventUnsolicitedId;

//--------------------------------------------------------------------------------------------------
/**
 * Sim Status event
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t      EventNewSimStateId;

//--------------------------------------------------------------------------------------------------
/**
 * External SIM selected by default
 */
//--------------------------------------------------------------------------------------------------
static le_sim_Id_t        UimSelect = LE_SIM_EXTERNAL_SLOT_1;


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to translate the code status for +CMS ERROR parsing.
 *
 */
//--------------------------------------------------------------------------------------------------
static void CheckCmsErrorCode
(
    const char*       valPtr,   ///< [IN] the value to change
    le_sim_States_t*  statePtr  ///< [OUT] SIM state
)
{
    switch (atoi(valPtr))
    {
        case 310:   /*SIM not inserted*/
        {
            *statePtr = LE_SIM_ABSENT;
            break;
        }
        case 311:   /*SIM PIN required*/
        case 312:   /*PH-SIM PIN required*/
        case 317:   /*SIM PIN2 required*/
        {
            *statePtr = LE_SIM_INSERTED;
            break;
        }
        case 515:   /*Please wait, init or command  processing in progress.*/
        {
            *statePtr = LE_SIM_BUSY;
            break;
        }
        case 316:   /*SIM PUK required*/
        case 318:   /*SIM PUK2 required*/
        {
            *statePtr = LE_SIM_BLOCKED;
            break;
        }
        case 313:   /*SIM failure*/
        default:
        {
            *statePtr = LE_SIM_STATE_UNKNOWN;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to translate the code status for +CME ERROR parsing
 *
 */
//--------------------------------------------------------------------------------------------------
static void CheckCmeErrorCode
(
    const char*      valPtr,   ///< [IN] the value to change
    le_sim_States_t* statePtr  ///< [OUT] SIM state
)
{
    switch (atoi(valPtr))
    {

        case 5:     /* PH-SIM PIN required (SIM lock) */
        case 11:    /* SIM PIN required */
        case 16:    /* Incorrect password Bad user pin */
        case 17:    /* SIM PIN2 required */
        {
            *statePtr = LE_SIM_INSERTED;
            break;
        }
        case 10:    /* SIM not inserted */
        {
            *statePtr = LE_SIM_ABSENT;
            break;
        }
        case 12:    /* SIM PUK required */
        case 18:    /* SIM PUK2 required */
        {
            *statePtr = LE_SIM_BLOCKED;
            break;
        }
        default:
        {
            *statePtr = LE_SIM_STATE_UNKNOWN;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to translate the code status for +CPIN parsing
 *
 */
//--------------------------------------------------------------------------------------------------
static void CheckCpinCode
(
    const char*      valPtr,   ///< [IN] the value to change
    le_sim_States_t* statePtr  ///< [OUT] SIM state
)
{
    if          (strcmp(valPtr,"READY")==0)
    {
        *statePtr = LE_SIM_READY;
    }
    else if (
                (strcmp(valPtr,"SIM PIN")==0)
                ||
                (strcmp(valPtr,"PH-SIM PIN")==0)
                ||
                (strcmp(valPtr,"SIM PIN2")==0)
            )
    {
        *statePtr = LE_SIM_INSERTED;
    }
    else if (
                (strcmp(valPtr,"SIM PUK")==0)
                ||
                (strcmp(valPtr,"SIM PUK2")==0)
            )
    {
        *statePtr = LE_SIM_BLOCKED;
    }
    else
    {
        *statePtr = LE_SIM_STATE_UNKNOWN;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to check the status of a line received
 *
 * @return true and filed statePtr
 * @return false
 */
//--------------------------------------------------------------------------------------------------
static bool CheckStatus
(
    const char*      lineStr,   ///< [IN] line to parse
    le_sim_States_t* statePtr   ///< [OUT] SIM state
)
{
    #define MAXLINE     18

    char line[MAXLINE+1];
    memcpy(line,lineStr,MAXLINE);

    line[MAXLINE] = '\0';
   *statePtr      = LE_SIM_STATE_UNKNOWN;

    le_result_t result = true;

    pa_utils_CountAndIsolateLineParameters(line);

    if (FIND_STRING("OK",pa_utils_IsolateLineParameter(line,1)))
    {
        *statePtr = LE_SIM_READY;
    }
    else if (FIND_STRING("+CME ERROR:",pa_utils_IsolateLineParameter(line,1)))
    {
        CheckCmeErrorCode(pa_utils_IsolateLineParameter(line,2),statePtr);
    }
    else if (FIND_STRING("+CMS ERROR:",pa_utils_IsolateLineParameter(line,1)))
    {
        CheckCmsErrorCode(pa_utils_IsolateLineParameter(line,2),statePtr);
    }
    else if (FIND_STRING("+CPIN:",pa_utils_IsolateLineParameter(line,1)))
    {
        CheckCpinCode(pa_utils_IsolateLineParameter(line,2),statePtr);
    }
    else
    {
        LE_DEBUG("this pattern is not expected -%s-",line);
       *statePtr = LE_SIM_STATE_UNKNOWN;
        result   = false;
    }

    LE_DEBUG("SIM Card Status %d",*statePtr);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to send event to all registered handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReportState
(
    uint32_t        simCard,  ///< [IN] Sim Card Number
    le_sim_States_t simState  ///< [IN] Sim Card Status
)
{
    pa_sim_Event_t* eventPtr = le_mem_ForceAlloc(SimEventPoolRef);
    eventPtr->simId = simCard;
    eventPtr->state = simState;

    LE_DEBUG("Send Event SIM identifier %d, SIM state %d",eventPtr->simId,eventPtr->state);
    le_event_ReportWithRefCounting(EventNewSimStateId,eventPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for SIM state changing handling.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SimUnsolicitedHandler
(
    void* reportPtr
)
{
    char*           unsolPtr = reportPtr;
    le_sim_States_t simState = LE_SIM_STATE_UNKNOWN;

    if (CheckStatus(unsolPtr,&simState))
    {
        ReportState(UimSelect,simState);
    }
}

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the sim module.
 *
 * @return LE_FAULT         The function failed to initialize the module.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_Init
(
    void
)
{
    SimEventPoolRef = le_mem_CreatePool("SimEventPool", sizeof(pa_sim_Event_t));
    SimEventPoolRef = le_mem_ExpandPool(SimEventPoolRef, DEFAULT_SIMEVENT_POOL_SIZE);

    EventUnsolicitedId = le_event_CreateId("SIMEventIdUnsol", LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    EventNewSimStateId = le_event_CreateIdWithRefCounting("SIMEventIdNewState");
    le_event_AddHandler("SimUnsolicitedHandler", EventUnsolicitedId, SimUnsolicitedHandler);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function counts number of sim card available.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return number           number of count slots
 */
//--------------------------------------------------------------------------------------------------
uint32_t pa_sim_CountSlots
(
    void
)
{
     return 1;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function selects the Card on which all further SIM operations have to be operated.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_SelectCard
(
    le_sim_Id_t  cardId     ///< The SIM to be selected
)
{
    if (cardId != LE_SIM_EXTERNAL_SLOT_1)
    {
        return LE_FAULT;
    }
    UimSelect = cardId;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function get the card on which operations are operated.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetSelectedCard
(
    le_sim_Id_t*  cardIdPtr     ///< [OUT] The card number selected.
)
{
   *cardIdPtr = 1;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function get the card identification (ICCID).
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetCardIdentification
(
    pa_sim_CardId_t iccid     ///< [OUT] CCID value
)
{
    le_atClient_CmdRef_t cmdRef   = NULL;
    char*                tokenPtr = NULL;
    le_result_t          res      = LE_OK;
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (!iccid)
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CCID",
                                        "+CCID:",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);

    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command");
        return res;
    }

    res = le_atClient_GetFinalResponse(cmdRef,
                                       finalResponse,
                                       LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);

    if ((res != LE_OK) || (strcmp(finalResponse,"OK") != 0))
    {
        LE_ERROR("Failed to get the response");
        le_atClient_Delete(cmdRef);
        return res;
    }

    res = le_atClient_GetFirstIntermediateResponse(cmdRef,
                                                   intermediateResponse,
                                                   LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);

    if (res != LE_OK)
    {
        LE_ERROR("Failed to get the response");
        le_atClient_Delete(cmdRef);
        return res;
    }

    // Cut the string for keep just the CCID number
    tokenPtr = strtok(intermediateResponse,"+CCID: ");
    strncpy(iccid, tokenPtr, strlen(tokenPtr));

    le_atClient_Delete(cmdRef);
    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function get the International Mobile Subscriber Identity (IMSI).
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetIMSI
(
    pa_sim_Imsi_t imsi   ///< [OUT] IMSI value
)
{
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_OK;
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (!imsi)
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CIMI",
                                        "0|1|2|3|4|5|6|7|8|9",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);

    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command");
        return res;
    }

    res = le_atClient_GetFinalResponse(cmdRef,
                                       finalResponse,
                                       LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);

    if ((res != LE_OK) || (strcmp(finalResponse,"OK") != 0))
    {
        LE_ERROR("Failed to get the response");
        le_atClient_Delete(cmdRef);
        return res;
    }

    res = le_atClient_GetFirstIntermediateResponse(cmdRef,
                                                   intermediateResponse,
                                                   LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);

    if (res != LE_OK)
    {
        LE_ERROR("Failed to get the response");
        le_atClient_Delete(cmdRef);
        return res;
    }

    strncpy(imsi, intermediateResponse, strlen(intermediateResponse));

    le_atClient_Delete(cmdRef);
    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function get the SIM Status.
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetState
(
    le_sim_States_t* statePtr    ///< [OUT] SIM state
)
{
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_OK;
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (!statePtr)
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

   *statePtr = LE_SIM_STATE_UNKNOWN;

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CPIN?",
                                        "",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);

    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command");
        return res;
    }

    res = le_atClient_GetFinalResponse(cmdRef,
                                       finalResponse,
                                       LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);

    if (res != LE_OK)
    {
        LE_ERROR("Failed to get the response");
        le_atClient_Delete(cmdRef);
        return res;
    }

    if (CheckStatus(finalResponse,statePtr))
    {
        ReportState(UimSelect,*statePtr);
    }

    le_atClient_Delete(cmdRef);
    return res;
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
    LE_DEBUG("Set new SIM State handler");

    LE_FATAL_IF(handler == NULL, "New SIM State handler is NULL");

    return (le_event_AddHandler("NewSIMStateHandler",
                                EventNewSimStateId,
                                (le_event_HandlerFunc_t) handler));
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
 * @return LE_FAULT         The function failed to enter the value.
 * @return LE_TIMEOUT       No response was received from the SIM card.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_EnterPIN
(
    pa_sim_PinType_t   type,  ///< [IN] pin type
    const pa_sim_Pin_t pin    ///< [IN] pin code
)
{
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_OK;

    snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CPIN=%s",pin);

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        command,
                                        "",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);

    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command");
        return res;
    }

    le_atClient_Delete(cmdRef);
    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function set the new PIN code.
 *
 *  - use to set pin code by providing the puk
 *
 * All depends on SIM state which must be retreived by @ref pa_sim_GetState
 *
 * @return LE_FAULT         The function failed to set the value.
 * @return LE_TIMEOUT       No response was received from the SIM card.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_EnterPUK
(
    pa_sim_PukType_t   type, ///< [IN] puk type
    const pa_sim_Puk_t puk,  ///< [IN] PUK code
    const pa_sim_Pin_t pin   ///< [IN] new PIN code
)
{
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_OK;

    snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CPIN=%s,%s",puk,pin);

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        command,
                                        "",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);

    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command");
        return res;
    }

    le_atClient_Delete(cmdRef);
    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function get the remaining attempts of a code.
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t pa_sim_GetRemainingAttempts
(
    uint32_t  idx,          ///< [IN] idx to read
    uint32_t* attemptsPtr  ///< [OUT] The number of attempts still possible
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function get the remaining attempts of a pin code.
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetPINRemainingAttempts
(
    pa_sim_PinType_t type,      ///< [IN] The pin type
    uint32_t*        attemptsPtr   ///< [OUT] The number of attempts still possible
)
{
    if      (type==PA_SIM_PIN)
    {
        return pa_sim_GetRemainingAttempts(0,attemptsPtr);
    }
    else if (type==PA_SIM_PIN2)
    {
        return pa_sim_GetRemainingAttempts(1,attemptsPtr);
    }
    else
    {
        return LE_BAD_PARAMETER;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function get the remaining attempts of a puk code.
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetPUKRemainingAttempts
(
    pa_sim_PukType_t type,      ///< [IN] The puk type
    uint32_t*        attemptsPtr   ///< [OUT] The number of attempts still possible
)
{
    if      (type==PA_SIM_PUK)
    {
        return pa_sim_GetRemainingAttempts(2,attemptsPtr);
    }
    else if (type==PA_SIM_PUK2)
    {
        return pa_sim_GetRemainingAttempts(3,attemptsPtr);
    }
    else
    {
        return LE_BAD_PARAMETER;
    }
}

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
le_result_t pa_sim_ChangePIN
(
    pa_sim_PinType_t   type,    ///< [IN] The code type
    const pa_sim_Pin_t oldcode, ///< [IN] Old code
    const pa_sim_Pin_t newcode  ///< [IN] New code
)
{
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_OK;

    if (type==PA_SIM_PIN)
    {
        snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CPWD=\"SC\",%s,%s",oldcode,newcode);
    }
    else if (type==PA_SIM_PIN2)
    {
        snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CPWD=\"P2\",%s,%s",oldcode,newcode);
    }
    else
    {
        return LE_BAD_PARAMETER;
    }

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        command,
                                        "",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);

    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command");
        return res;
    }

    le_atClient_Delete(cmdRef);
    return res;
}


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
le_result_t pa_sim_EnablePIN
(
    pa_sim_PinType_t   type,  ///< [IN] The pin type
    const pa_sim_Pin_t code   ///< [IN] code
)
{
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_OK;

    if (type==PA_SIM_PIN)
    {
        snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"at+clck=\"SC\",1,%s",code);
    }
    else if (type==PA_SIM_PIN2)
    {
        snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"at+clck=\"P2\",1,%s",code);
    }
    else
    {
        return LE_BAD_PARAMETER;
    }

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        command,
                                        "",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);

    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command");
        return res;
    }

    le_atClient_Delete(cmdRef);
    return res;
}


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
le_result_t pa_sim_DisablePIN
(
    pa_sim_PinType_t   type,  ///< [IN] The code type.
    const pa_sim_Pin_t code   ///< [IN] code
)
{
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_OK;

    if      (type==PA_SIM_PIN)
    {
        snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"at+clck=\"SC\",0,%s",code);
    }
    else if (type==PA_SIM_PIN2)
    {
        snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"at+clck=\"P2\",0,%s",code);
    }
    else
    {
        return LE_BAD_PARAMETER;
    }

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        command,
                                        "",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);

    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command");
        return res;
    }

    le_atClient_Delete(cmdRef);
    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM Phone Number.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Phone Number can't fit in phoneNumberStr
 *      - LE_FAULT on any other failure
 * @TODO
 *      implementation
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_GetSubscriberPhoneNumber
(
    char*        phoneNumberStr,    ///< [OUT] The phone Number
    size_t       phoneNumberStrSize ///< [IN]  Size of phoneNumberStr
)
{
    return LE_FAULT;
}

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
le_result_t pa_sim_GetHomeNetworkOperator
(
    char*       nameStr,               ///< [OUT] the home network Name
    size_t      nameStrSize            ///< [IN] the nameStr size
)
{
    le_result_t          res      = LE_OK;
    le_atClient_CmdRef_t cmdRef   = NULL;
    char*                tokenPtr = NULL;
    char*                savePtr  = NULL;
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (!nameStr)
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+COPS?",
                                        "+COPS:",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);

    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command");
        return res;
    }

    res = le_atClient_GetFinalResponse(cmdRef,
                                       finalResponse,
                                       LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);

    if ((res != LE_OK) || (strcmp(finalResponse,"OK") != 0))
    {
        LE_ERROR("Failed to get the response");
        le_atClient_Delete(cmdRef);
        return res;
    }

    res = le_atClient_GetFirstIntermediateResponse(cmdRef,
                                                   intermediateResponse,
                                                   LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);

    if (res != LE_OK)
    {
        LE_ERROR("Failed to get the response");
        le_atClient_Delete(cmdRef);
        return res;
    }

    // Cut the string for keep just the phone number
    tokenPtr = strtok_r(intermediateResponse, "\"", &savePtr);
    tokenPtr = strtok_r(NULL, "\"", &savePtr);
    strncpy(nameStr, tokenPtr, nameStrSize);

    le_atClient_Delete(cmdRef);
    return res;
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
    char*     mccPtr,                ///< [OUT] Mobile Country Code
    size_t    mccPtrSize,            ///< [IN] mccPtr buffer size
    char*     mncPtr,                ///< [OUT] Mobile Network Code
    size_t    mncPtrSize             ///< [IN] mncPtr buffer size
)
{
    return LE_FAULT;
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
    return LE_FAULT;
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
    return LE_FAULT;
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
    return LE_FAULT;
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
    return LE_FAULT;
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
    return NULL;
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
    return LE_FAULT;
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
    bool  confirmation  ///< [IN] true to accept, false to reject
)
{
    return LE_FAULT;
}

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
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sim_SendCommand
(
    le_sim_Command_t command,               ///< [IN] The SIM command
    const char*      fileIdentifier,        ///< [IN] File identifier
    uint8_t          p1,                    ///< [IN] Parameter P1 passed to the SIM
    uint8_t          p2,                    ///< [IN] Parameter P2 passed to the SIM
    uint8_t          p3,                    ///< [IN] Parameter P3 passed to the SIM
    const uint8_t*   dataPtr,               ///< [IN] Data command
    size_t           dataNumElements,       ///< [IN] Size of data command
    const char*      path,                  ///< [IN] Path of the elementary file
    uint8_t*         sw1Ptr,                ///< [OUT] SW1 received from the SIM
    uint8_t*         sw2Ptr,                ///< [OUT] SW2 received from the SIM
    uint8_t*         responsePtr,           ///< [OUT] SIM response
    size_t*          responseNumElementsPtr ///< [IN/OUT] Size of response
)
{
    return LE_FAULT;
}
