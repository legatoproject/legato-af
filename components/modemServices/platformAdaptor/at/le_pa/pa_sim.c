/** @file pa_sim.c
 *
 * AT implementation of c_pa_sim API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


#include "legato.h"
#include "atManager/inc/atCmdSync.h"
#include "atManager/inc/atPorts.h"

#include "pa_sim.h"
#include "pa_common_local.h"

#define DEFAULT_SIMEVENT_POOL_SIZE  1

static le_mem_PoolRef_t   SimEventPoolRef;

static le_event_Id_t      EventUnsolicitedId;
static le_event_Id_t      EventNewSimStateId;

static le_sim_Id_t        UimSelect = LE_SIM_EXTERNAL_SLOT_1; // external SIM selected by default


/**
 * This function resets the modem. It must be called after a new SIM card selection, otherwise the
 * new SIM card selection is not taken into account.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t resetModem
(
    void
)
{
    atcmdsync_ResultRef_t  resRef=NULL;
    const char* finalRespOkPtr[] = {"+WIND: 4",NULL };
    const char* finalRespKoPtr[] = {"ERROR","+CME ERROR:","+CMS ERROR:","TIMEOUT",NULL};

    atcmd_Ref_t atReqRef = atcmd_Create();

    atcmd_AddCommand(atReqRef,"at+cfun=1",false);
    atcmd_AddData       (atReqRef,NULL,0);
    atcmd_SetTimer      (atReqRef,30000,atcmdsync_GetTimerExpiryHandler());
    atcmd_AddIntermediateResp    (atReqRef,atcmdsync_GetIntermediateEventId(),NULL);
    atcmd_AddFinalResp(atReqRef,atcmdsync_GetFinalEventId(),finalRespOkPtr);
    atcmd_AddFinalResp(atReqRef,atcmdsync_GetFinalEventId(),finalRespKoPtr);

    resRef = atcmdsync_SendCommand(atports_GetInterface(ATPORT_COMMAND),atReqRef);

    le_result_t result = atcmdsync_CheckCommandResult(resRef,finalRespOkPtr,finalRespKoPtr);

    le_mem_Release(atReqRef);   // Release e_atmgr_Object_CreateATCommand
    le_mem_Release(resRef);  // Release le_pa_at_SendSync

    if ( result != LE_OK )
    {
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}


/**
 * This function must be called to translate the code status for +CMS ERROR parsing
 *
 */
//--------------------------------------------------------------------------------------------------
static void CheckStatus_CmsErrorCode
(
    const char      *valPtr,  ///< [IN] the value to change
    le_sim_States_t *statePtr  ///< [OUT] SIM state
)
{
    uint32_t value = atoi(valPtr);

    switch (value)
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
static void CheckStatus_CmeErrorCode
(
    const char      *valPtr,  ///< [IN] the value to change
    le_sim_States_t *statePtr  ///< [OUT] SIM state
)
{
    uint32_t value = atoi(valPtr);

    switch (value)
    {

        case 5:     /*PH-SIM PIN required (SIM lock)*/
        case 11:    /*SIM PIN required*/
        case 16:    /*Incorrect password Bad user pin*/
        case 17:    /*SIM PIN2 required*/
        {
            *statePtr = LE_SIM_INSERTED;
            break;
        }
        case 10:    /*SIM not inserted*/
        {
            *statePtr = LE_SIM_ABSENT;
            break;
        }
        case 12:    /*SIM PUK required*/
        case 18:    /*SIM PUK2 required*/
        {
            *statePtr = LE_SIM_BLOCKED;
            break;
        }
        case 3:
        case 4:
        case 13:
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
static void CheckStatus_CpinCode
(
    const char      *valPtr,   ///< [IN] the value to change
    le_sim_States_t *statePtr  ///< [OUT] SIM state
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
 * This function must be called to translate the code status for +WIND parsing
 *
 */
//--------------------------------------------------------------------------------------------------
static void CheckStatus_WindCode
(
    const char      *valPtr,   ///< [IN] the value to change
    le_sim_States_t *statePtr  ///< [OUT] SIM state
)
{
    uint32_t value = atoi(valPtr);

    switch (value)
    {

        case 0:     /*SIM card is removed.*/
//         case 14:    /*The rack has been detected as opened.*/
        {
            *statePtr = LE_SIM_ABSENT;
            break;
        }
        case 1:    /*SIM card is inserted.*/
//         case 10:   /*Reload status of each SIM phonebook after init phase.*/
//         case 11:   /*Checksum of SIM phonebooks after initialization.*/
//         case 13:   /*SIM card is inserted.*/
        {
            *statePtr = LE_SIM_INSERTED;
            break;
        }
        default:
        {
            LE_DEBUG("WIND error (%s) not used",valPtr);
            *statePtr = LE_SIM_STATE_UNKNOWN;
        }
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
    const char      *lineStr, ///< [IN] line to parse
    le_sim_States_t *statePtr ///< [OUT] SIM state
)
{
    *statePtr = LE_SIM_STATE_UNKNOWN;
    le_result_t result = true;
#define MAXLINE     18
    char line[MAXLINE+1];

    memcpy(line,lineStr,MAXLINE);
    line[MAXLINE] = '\0';

    atcmd_CountLineParameter(line);

    if (FIND_STRING("OK",atcmd_GetLineParameter(line,1)))
    {
        *statePtr = LE_SIM_READY;
    }
    else if (FIND_STRING("+CME ERROR:",atcmd_GetLineParameter(line,1)))
    {
        CheckStatus_CmeErrorCode(atcmd_GetLineParameter(line,2),statePtr);
    }
    else if (FIND_STRING("+CMS ERROR:",atcmd_GetLineParameter(line,1)))
    {
        CheckStatus_CmsErrorCode(atcmd_GetLineParameter(line,2),statePtr);
    }
    else if (FIND_STRING("+CPIN:",atcmd_GetLineParameter(line,1)))
    {
        CheckStatus_CpinCode(atcmd_GetLineParameter(line,2),statePtr);
    }
    else if (FIND_STRING("+WIND:",atcmd_GetLineParameter(line,1)))
    {
        CheckStatus_WindCode(atcmd_GetLineParameter(line,2),statePtr);
    }
    else
    {
        LE_DEBUG("this pattern is not expected -%s-",line);
        *statePtr = LE_SIM_STATE_UNKNOWN;
        result = false;
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
static void ReportStatus
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
static void SIMUnsolHandler
(
    void* reportPtr
)
{
    atmgr_UnsolResponse_t* unsolPtr = reportPtr;
    le_sim_States_t simState = LE_SIM_STATE_UNKNOWN;

    if (CheckStatus(unsolPtr->line,&simState))
    {
        ReportStatus(UimSelect,simState);
    }
}

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------
/**
 * This function sets the SierraWireless proprietary indicator +WIND unsolicited
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 */

static le_result_t SetIndicator
(
    void
)
{
    uint32_t wind;

    if ( pa_common_GetWindIndicator(&wind) != LE_OK)
    {
        return LE_FAULT;
    }

    if ( pa_common_SetWindIndicator(wind|1|8) != LE_OK)
    {
        return LE_FAULT;
    }

    atmgr_SubscribeUnsolReq(atports_GetInterface(ATPORT_COMMAND),
                                   EventUnsolicitedId,
                                   "+WIND: 0",
                                   false);

    atmgr_SubscribeUnsolReq(atports_GetInterface(ATPORT_COMMAND),
                                   EventUnsolicitedId,
                                   "+WIND: 1",
                                   false);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the sim module
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
    if (atports_GetInterface(ATPORT_COMMAND)==NULL) {
        LE_DEBUG("SIM Module is not initialize in this session");
        return LE_FAULT;
    }

    SimEventPoolRef = le_mem_CreatePool("SimEventPool", sizeof(pa_sim_Event_t));
    SimEventPoolRef = le_mem_ExpandPool(SimEventPoolRef,DEFAULT_SIMEVENT_POOL_SIZE);

    EventUnsolicitedId    = le_event_CreateId("SIMEventIdUnsol",sizeof(atmgr_UnsolResponse_t));
    EventNewSimStateId    = le_event_CreateIdWithRefCounting("SIMEventIdNewState");
    le_event_AddHandler("SIMUnsolHandler",EventUnsolicitedId  ,SIMUnsolHandler);

    LE_DEBUG_IF(SetIndicator()!=LE_OK,"cannot set sim +WIND indicator");

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function counts number of sim card available
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
    atcmdsync_ResultRef_t  resRef = NULL;
    const char* interRespPtr[] = {"+WHCNF: 4",NULL};
    uint32_t numberOfSim=1;

    le_result_t result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                                "at+whcnf=?",
                                                &resRef,
                                                interRespPtr,
                                                30000);

    if ( result != LE_OK ) {
        le_mem_Release(resRef); // release pa_at_SendSyncDefaultExt
        return LE_FAULT;
    }

    // If there is more than one line then it mean that the command is OK so the first line is
    if (atcmdsync_GetNumLines(resRef) == 2)
    {
        char* line = atcmdsync_GetLine(resRef,0);
        if (FIND_STRING("+WHCNF: 4,(0-3)",line))
        {
            numberOfSim = 2;
        }
        else
        {
            LE_WARN("this pattern is not expected");
            numberOfSim = 1;
        }
    } else {
        numberOfSim = 1;
    }

    le_mem_Release(resRef);     // Release pa_at_SendSyncDefaultExt

    return numberOfSim;
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
    char atcommand[ATCOMMAND_SIZE] ;

    if ((cardId==LE_SIM_EXTERNAL_SLOT_1) || (cardId==LE_SIM_EXTERNAL_SLOT_2)) {
        atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"at+whcnf=4,%d",cardId);
    } else {
        LE_DEBUG("This card number (%d) is not supported",cardId);
        return LE_FAULT;
    }

    le_result_t result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                                atcommand,
                                                NULL,
                                                NULL,
                                                30000);

    if ( result != LE_OK ) {
        return LE_FAULT;
    }

    if (resetModem()!=LE_OK)
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
    le_result_t result=LE_OK;
    atcmdsync_ResultRef_t  resRef = NULL;
    const char* interRespPtr[] = {"+WHCNF: 4",NULL};

    result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                    "at+whcnf?",
                                    &resRef,
                                    interRespPtr,
                                    30000);

    if ( result != LE_OK ) {
        le_mem_Release(resRef); // release pa_at_SendSyncDefaultExt
        return LE_FAULT;
    }

    // If there is more than one line then it mean that the command is OK so the first line is
    // the intermediate one
    if (atcmdsync_GetNumLines(resRef) == 2)
    {
        char* line = atcmdsync_GetLine(resRef,0);
        if (FIND_STRING("+WHCNF: 4,1",line))
        {
            *cardIdPtr = 1;
        }
        else if (FIND_STRING("+WHCNF: 4,2",line))
        {
            *cardIdPtr = 2;
        }
        else
        {
            LE_WARN("this pattern is not expected");
            result =  LE_FAULT;
        }
    }

    le_mem_Release(resRef);     // Release pa_at_SendSyncDefaultExt

    return result;
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
    le_result_t result=LE_OK;
    atcmdsync_ResultRef_t  resRef = NULL;
    const char* interRespPtr[] = {"+CCID:",NULL};

    if (!iccid)
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                    "at+ccid",
                                    &resRef,
                                    interRespPtr,
                                    30000);

    if ( result != LE_OK ) {
        le_mem_Release(resRef);
        return LE_FAULT;
    }

    le_sim_States_t simState=LE_SIM_STATE_UNKNOWN;
    char* line = atcmdsync_GetLine(resRef,0);
    if (CheckStatus(line,&simState))
    {
        ReportStatus(UimSelect,simState);
    }

    // check error
    if (atcmdsync_GetNumLines(resRef) == 2)
    {

        line = atcmdsync_GetLine(resRef,0);
        uint32_t numParam = atcmd_CountLineParameter(line);
        // it parse just the first line because of '\0'

        if (FIND_STRING("+CCID:",atcmd_GetLineParameter(line,1)))
        {
            if (numParam==2)
            {
                atcmd_CopyStringWithoutQuote(iccid,
                                            atcmd_GetLineParameter(line,2),
                                            strlen(atcmd_GetLineParameter(line,2)));
                result = LE_OK;
            } else {
                LE_WARN("this pattern is not expected");
                result=LE_FAULT;
            }
        } else {
            LE_WARN("this pattern is not expected");
            result=LE_FAULT;
        }
    }

    le_mem_Release(resRef);     // Release atcmdsync_SendCommandDefaultExt
    return result;
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
    le_result_t result;
    atcmdsync_ResultRef_t  resRef = NULL;
    // IMSI start with 0|1|2|3|4|5|6|7|8|9
    const char* interRespPtr[] = {"0","1","2","3","4","5","6","7","8","9",NULL};

    if (!imsi)
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                    "at+cimi",
                                    &resRef,
                                    interRespPtr,
                                    30000);

    if ( result != LE_OK ) {
        le_mem_Release(resRef);
        return LE_FAULT;
    }

    le_sim_States_t simState=LE_SIM_STATE_UNKNOWN;
    char* line = atcmdsync_GetLine(resRef,0);
    if (CheckStatus(line,&simState))
    {
        ReportStatus(UimSelect,simState);
    }

    // If there is more than one line then it mean that the command is OK so the first line is
    // the intermediate one
    if (atcmdsync_GetNumLines(resRef) == 2)
    {
        line = atcmdsync_GetLine(resRef,0);
        // copy just the first line because of '\0'
        atcmd_CopyStringWithoutQuote(imsi,
                                   line,
                                   strlen(line));

        result = LE_OK;
    }
    // it is not expected
    else {
        LE_WARN("this pattern is not expected");
        result = LE_FAULT;
    }

    le_mem_Release(resRef);     // Release atcmdsync_SendCommandDefaultExt

    return result;
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
    int32_t result = LE_FAULT;
    atcmd_Ref_t atReqRef;
    atcmdsync_ResultRef_t  resRef = NULL;

    const char* finalRespOkPtr[] = {"OK","+CPIN:",NULL};
    const char* finalRespKoPtr[] = {"ERROR","+CME ERROR:","+CMS ERROR:","TIMEOUT",NULL};

    if (!statePtr)
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    *statePtr=LE_SIM_STATE_UNKNOWN;

    atReqRef = atcmd_Create();
    atcmd_AddCommand(atReqRef,"at+cpin?",false);
    atcmd_AddData(atReqRef,NULL,0);
    atcmd_SetTimer(atReqRef,30000,atcmdsync_GetTimerExpiryHandler());
    atcmd_AddIntermediateResp    (atReqRef,atcmdsync_GetIntermediateEventId(),NULL);
    atcmd_AddFinalResp(atReqRef,atcmdsync_GetFinalEventId(),finalRespOkPtr);
    atcmd_AddFinalResp(atReqRef,atcmdsync_GetFinalEventId(),finalRespKoPtr);

    resRef    = atcmdsync_SendCommand(atports_GetInterface(ATPORT_COMMAND),atReqRef);

    char* line = atcmdsync_GetFinalLine(resRef);

    // Check timeout
    if (FIND_STRING("TIMEOUT",line)) {
        LE_WARN("Modem failed");
        le_mem_Release(atReqRef); le_mem_Release(resRef);
        return LE_TIMEOUT;
    }

    if (CheckStatus(line,statePtr))
    {
        ReportStatus(UimSelect,*statePtr);
        result = LE_OK;
    }

    le_mem_Release(atReqRef);  // Release atcmdsync_SetCommand
    le_mem_Release(resRef);     // Release atcmdsync_SendCommand

    return result;
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

    if (handler==NULL) {
        LE_FATAL("new SIM State handler is NULL");
    }

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
    atcmdsync_ResultRef_t  resRef = NULL;
    char atcommand[ATCOMMAND_SIZE] ;

    atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"at+cpin=%s",pin);

    le_result_t result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                                atcommand,
                                                &resRef,
                                                NULL,
                                                30000);

    if ( result != LE_OK ) {
        le_mem_Release(resRef);
        return LE_FAULT;
    }

    le_sim_States_t simState=LE_SIM_STATE_UNKNOWN;
    char* line = atcmdsync_GetLine(resRef,0);
    if (CheckStatus(line,&simState))
    {
        ReportStatus(UimSelect,simState);
    }

    le_mem_Release(resRef); // release atcmdsync_SendCommandDefault
    return LE_OK;
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
    atcmdsync_ResultRef_t  resRef = NULL;
    char atcommand[ATCOMMAND_SIZE] ;

    atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"at+cpin=%s,%s",puk,pin);

    le_result_t result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                                atcommand,
                                                &resRef,
                                                NULL,
                                                30000);

    if ( result != LE_OK ) {
        le_mem_Release(resRef);
        return LE_FAULT;
    }

    le_sim_States_t simState=LE_SIM_STATE_UNKNOWN;
    char* line = atcmdsync_GetLine(resRef,0);
    if (CheckStatus(line,&simState))
    {
        ReportStatus(UimSelect,simState);
    }

    le_mem_Release(resRef); // release atcmdsync_SendCommandDefault
    return LE_OK;
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
    uint32_t  idx,      ///< [IN] idx to read
    uint32_t* attemptsPtr  ///< [OUT] The number of attempts still possible
)
{
    le_result_t result = LE_FAULT;
    atcmdsync_ResultRef_t  resRef = NULL;
    const char* interRespPtr[] = {"+CPINC:",NULL};

    if (!attemptsPtr)
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                    "at+cpinc",
                                    &resRef,
                                    interRespPtr,
                                    30000);

    if ( result != LE_OK ) {
        le_mem_Release(resRef); // release atcmdsync_SendCommandDefaultExt
        return LE_FAULT;
    }

    // If there is more than one line then it mean that the command is OK so the first line is
    // the intermediate one
    if (atcmdsync_GetNumLines(resRef) == 2)
    {
        // it parse just the first line because of '\0'
        char* line = atcmdsync_GetLine(resRef,0);
        uint32_t numParam = atcmd_CountLineParameter(line);
        // it parse just the first line because of '\0'
        if ((numParam==5) && (FIND_STRING("+CPINC:",atcmd_GetLineParameter(line,1))))
        {
            *attemptsPtr = atoi(atcmd_GetLineParameter(line,idx));

            result = LE_OK;
        } else {
            LE_WARN("this pattern is not expected");
            result = LE_FAULT;
        }

    } else {
        LE_WARN("this pattern is not expected");
        result = LE_FAULT;
    }

    le_mem_Release(resRef);     // Release atcmdsync_SendCommandDefaultExt

    return result;
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
        return pa_sim_GetRemainingAttempts(2,attemptsPtr);
    }
    else if (type==PA_SIM_PIN2)
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
        return pa_sim_GetRemainingAttempts(4,attemptsPtr);
    }
    else if (type==PA_SIM_PUK2)
    {
        return pa_sim_GetRemainingAttempts(5,attemptsPtr);
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
    atcmdsync_ResultRef_t  resRef = NULL;
    char atcommand[ATCOMMAND_SIZE] ;

    if (type==PA_SIM_PIN)
    {
        atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"at+cpwd=\"SC\",%s,%s",oldcode,newcode);
    } else if (type==PA_SIM_PIN2)
    {
        atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"at+cpwd=\"P2\",%s,%s",oldcode,newcode);
    } else
    {
        return LE_BAD_PARAMETER;
    }

    le_result_t result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                                atcommand,
                                                &resRef,
                                                NULL,
                                                30000);

    if ( result != LE_OK ) {
        le_mem_Release(resRef);
        return result;
    }

    le_sim_States_t simState=LE_SIM_STATE_UNKNOWN;
    char* line = atcmdsync_GetLine(resRef,0);
    if (CheckStatus(line,&simState))
    {
        ReportStatus(UimSelect,simState);
    }

    le_mem_Release(resRef);     // Release atcmdsync_SendCommandDefault

    return LE_OK;
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
    atcmdsync_ResultRef_t  resRef = NULL;
    char atcommand[ATCOMMAND_SIZE] ;

    if (type==PA_SIM_PIN)
    {
        atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"at+clck=\"SC\",1,%s",code);
    } else if (type==PA_SIM_PIN2)
    {
        atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"at+clck=\"P2\",1,%s",code);
    } else
    {
        return LE_BAD_PARAMETER;
    }

    le_result_t result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                                atcommand,
                                                &resRef,
                                                NULL,
                                                30000);
    if ( result != LE_OK ) {
        le_mem_Release(resRef);
        return LE_FAULT;
    }

    le_sim_States_t simState=LE_SIM_STATE_UNKNOWN;
    char* line = atcmdsync_GetLine(resRef,0);
    if (CheckStatus(line,&simState))
    {
        ReportStatus(UimSelect,simState);
    }

    le_mem_Release(resRef);     // Release atcmdsync_SendCommandDefault

    return LE_OK;
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
    atcmdsync_ResultRef_t  resRef = NULL;
    char atcommand[ATCOMMAND_SIZE] ;

    if (type==PA_SIM_PIN)
    {
        atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"at+clck=\"SC\",0,%s",code);
    } else if (type==PA_SIM_PIN2)
    {
        atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"at+clck=\"P2\",0,%s",code);
    } else
    {
        return LE_BAD_PARAMETER;
    }

    le_result_t result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                                atcommand,
                                                &resRef,
                                                NULL,
                                                30000);

    if ( result != LE_OK ) {
        le_mem_Release(resRef);
        return LE_FAULT;
    }

    le_sim_States_t simState=LE_SIM_STATE_UNKNOWN;
    char* line = atcmdsync_GetLine(resRef,0);
    if (CheckStatus(line,&simState))
    {
        ReportStatus(UimSelect,simState);
    }

    le_mem_Release(resRef);     // Release atcmdsync_SendCommandDefault

    return LE_OK;
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
    char        *phoneNumberStr,    ///< [OUT] The phone Number
    size_t       phoneNumberStrSize ///< [IN]  Size of phoneNumberStr
)
{
    return le_utf8_Copy(phoneNumberStr,"",phoneNumberStrSize, NULL);
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
    char       *nameStr,               ///< [OUT] the home network Name
    size_t      nameStrSize            ///< [IN] the nameStr size
)
{
    return LE_FAULT;
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
    bool  confirmation ///< [IN] true to accept, false to reject
)
{
    return LE_FAULT;
}
