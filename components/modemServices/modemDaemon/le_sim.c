/** @file le_sim.c
 *
 * This file contains the data structures and the source code of the high level SIM APIs.
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#include "legato.h"
#include "interfaces.h"
#include "pa_sim.h"
#include "le_mrc_local.h"

//--------------------------------------------------------------------------------------------------
/**
 *  Maximum FPLMN list count.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_FPLMN_LISTS 1

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Enumeration for Subscription type.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    COMMERCIAL,           ///< Commercial subscription.
    ECS,                  ///< Emergency Call subscription.
    UNKNOWN_SUBSCRIPTION, ///< Unknown subscription.
    SUBSCRIPTION_MAX
}
Subscription_t;


//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * SIM structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sim_Obj
{
    le_sim_Id_t      simId;                      ///< SIM identifier.
    char             ICCID[LE_SIM_ICCID_BYTES];  ///< Integrated circuit card identifier.
    char             IMSI[LE_SIM_IMSI_BYTES];    ///< International mobile subscriber identity.
    char             PIN[LE_SIM_PIN_MAX_BYTES];  ///< PIN code.
    char             PUK[LE_SIM_PUK_MAX_BYTES];  ///< PUK code.
    char             EID[LE_SIM_EID_BYTES];      ///< eUICCID unique identifier (EID).
    char             phoneNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES]; /// < The Phone Number.
    bool             isPresent;                  ///< 'isPresent' flag.
    Subscription_t   subscription;               ///< Subscription type
}
Sim_t;

//--------------------------------------------------------------------------------------------------
/**
 * SIM state event.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sim_Id_t      simId;   ///< SIM identififier
    le_sim_States_t  state;   ///< SIM state.
}
Sim_Event_t;

//--------------------------------------------------------------------------------------------------
/**
 * FPLMN list structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sim_FPLMNListRef_t FPLMNListRef;      ///< FPLMN List reference.
    le_msg_SessionRef_t   sessionRef;        ///< Client session reference.
    le_dls_List_t         list;              ///< Link list to insert new FPLMN operator.
    le_dls_Link_t*        currentLink;       ///< Link list pointed to current FPLMN operator.
}
le_sim_FPLMNList_t;

//--------------------------------------------------------------------------------------------------
/**
 * List of all SIM objects.
 */
//--------------------------------------------------------------------------------------------------
static Sim_t SimList[LE_SIM_ID_MAX];

//--------------------------------------------------------------------------------------------------
/**
 * APDU message structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t apduLength;
    uint8_t  apduReq[16];
}
ApduMsg_t;


//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Current selected SIM card.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_sim_Id_t  SelectedCard;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for New SIM state notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NewSimStateEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for SIM Toolkit notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t SimToolkitEventId;

//--------------------------------------------------------------------------------------------------
/**
 * ECounter for SIM Toolkit event handlers.
 *
 */
//--------------------------------------------------------------------------------------------------
static uint32_t SimToolkitHandlerCount = 0;

//--------------------------------------------------------------------------------------------------
/**
 * PA SIM Toolkit handler reference.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_HandlerRef_t PaSimToolkitHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * The APDU messages to swap to Emergency Call subscription.
 */
//--------------------------------------------------------------------------------------------------
static ApduMsg_t EcsSwapApduReq[LE_SIM_MANUFACTURER_MAX] =
{
    // OBERTHUR
    { 10 , {0x80, 0xC2, 0x00, 0x00, 0x05, 0xEE, 0x03, 0xEF, 0x01, 0x20} },
    // GEMALTO
    { 14, {0x80, 0xC2, 0x00, 0x00, 0x09, 0xD3, 0x07, 0x02, 0x02, 0x01, 0x81, 0x10, 0x01, 0x7E} },
    // G_AND_D
    { 5, {0x00, 0xB6, 0x01, 0x00, 0x00} },
    // MORPHO
    { 13, {0x80, 0xC2, 0x00, 0x00, 0x08, 0xCF, 0x06, 0x19, 0x01, 0x99, 0x5F, 0x01, 0x81} },
    // VALID
    { 13 , {0x80, 0xC2, 0x00, 0x00, 0x08, 0xCF, 0x06, 0x19, 0x01, 0x80, 0x58, 0x01, 0x81} }
};

//--------------------------------------------------------------------------------------------------
/**
 * The APDU messages to swap to Commercial subscription.
 */
//--------------------------------------------------------------------------------------------------
static ApduMsg_t CommercialSwapApduReq[LE_SIM_MANUFACTURER_MAX] =
{
    // OBERTHUR
    { 10, {0x80, 0xC2, 0x00, 0x00, 0x05, 0xEE, 0x03, 0xEF, 0x01, 0x24} },
    // GEMALTO
    { 14, {0x80, 0xC2, 0x00, 0x00, 0x09, 0xD3, 0x07, 0x02, 0x02, 0x01, 0x81, 0x10, 0x01, 0x7F} },
    // G_AND_D
    { 5, {0x00, 0xB6, 0x02, 0x00, 0x00} },
    // MORPHO
    { 13, {0x80, 0xC2, 0x00, 0x00, 0x08, 0xCF, 0x06, 0x19, 0x01, 0x99, 0x5F, 0x01, 0x80} },
    // VALID
    { 13 , {0x80, 0xC2, 0x00, 0x00, 0x08, 0xCF, 0x06, 0x19, 0x01, 0x80, 0x58, 0x01, 0x80} }
};

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for FPLMN list.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t FPLMNListRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for FPLMN list.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t FPLMNListPool;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for FPLMN network Operator.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t FPLMNOperatorPool;


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to request the multi-profile eUICC to swap to commercial or ECS
 * subscription and to refresh.
 * The User's application must wait for eUICC reboot to be finished and network connection
 * available.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BUSY when a profile swap is already in progress
 *      - LE_FAULT for unexpected error
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LocalSwap
(
    le_sim_Manufacturer_t manufacturer,   ///< [IN] The card manufacturer.
    uint8_t*              swapApduReqPtr, ///< [IN] The swap APDU message.
    uint32_t              swapApduLen     ///< [IN] The swap APDU message length in bytes.
)
{
    uint8_t  channel = 0;
    uint8_t  resp[LE_SIM_RESPONSE_MAX_BYTES] = {0};
    size_t   lenResp = LE_SIM_RESPONSE_MAX_BYTES;

    // Response for APDU command successfully executed
    uint8_t  respOk[2]= {0x90, 0x00};

    // Get the logical channel to send APDu command.
    if (LE_OK != pa_sim_OpenLogicalChannel(&channel))
    {
        LE_ERROR("Cannot open Logical Channel!");
        return LE_FAULT;
    }

    if (LE_SIM_G_AND_D == manufacturer)
    {
        uint8_t  pduReq[] = {0x00, 0xA4, 0x04, 0x00, 0x10, 0xD2, 0x76, 0x00,
                             0x01, 0x18, 0x00, 0x02, 0xFF, 0x34, 0x10, 0x25,
                             0x89, 0xC0, 0x02, 0x10, 0x01};

        pduReq[0] = channel;
        if (LE_OK != pa_sim_SendApdu(channel, pduReq,
                                     sizeof(pduReq), resp, &lenResp))
        {
            LE_ERROR("Cannot send APDU message!");
            return LE_FAULT;
        }

        // Check if the command is successfully executed.
        if ((lenResp < sizeof(respOk))
            || (0 != memcmp(resp, respOk, sizeof(respOk))))
        {
            LE_ERROR("APDU response: %02X, %02X", resp[0], resp[1]);
            return LE_FAULT;
        }

        swapApduReqPtr[0] = channel;
    }

    if (LE_OK != pa_sim_SendApdu(channel,
                                 swapApduReqPtr,
                                 swapApduLen,
                                 resp, &lenResp))
    {
        LE_ERROR("Cannot swap subscription!");
        return LE_FAULT;
    }

    if (LE_OK != pa_sim_CloseLogicalChannel(channel))
    {
        LE_ERROR("Cannot close Logical Channel!");
        return LE_FAULT;
    }

    // Check if the command is successfully executed.
    if ((lenResp < sizeof(respOk))
        || (0 != memcmp(resp, respOk, sizeof(respOk))))
    {
        LE_ERROR("APDU response: %02X, %02X", resp[0], resp[1]);
        return LE_FAULT;
    }

    if ((manufacturer == LE_SIM_OBERTHUR)
         || (manufacturer == LE_SIM_MORPHO))
    {
        return LE_OK;
    }

    return pa_sim_Refresh();
}

//--------------------------------------------------------------------------------------------------
/**
 * SIM card selector.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SelectSIMCard
(
    le_sim_Id_t simId    ///< [IN] The SIM identifier.
)
{
    if(simId != SelectedCard)
    {
        // Select the SIM card
        LE_DEBUG("Try to select sim identifier.%d", simId);
        if(pa_sim_SelectCard(simId) != LE_OK)
        {
            LE_ERROR("Failed to select sim identifier.%d", simId);
            return LE_NOT_FOUND;
        }
        SelectedCard = simId;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer New SIM state notification Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerNewSimStateHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    Sim_Event_t*                  simEvent = reportPtr;

    le_sim_NewStateHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(simEvent->simId, simEvent->state, le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM card EID.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetEID
(
    Sim_t* simPtr   ///< [IN] The SIM structure.
)
{
  pa_sim_Eid_t eid;

  if (LE_OK != pa_sim_GetCardEID(eid))
  {
      return LE_FAULT;
  }

  return le_utf8_Copy(simPtr->EID, eid, sizeof(simPtr->EID), NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM card ICCID.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetICCID
(
    Sim_t* simPtr   ///< [IN] The SIM structure.
)
{
  pa_sim_CardId_t  iccid;

  if (LE_OK != pa_sim_GetCardIdentification(iccid))
  {
      return LE_FAULT;
  }

  return le_utf8_Copy(simPtr->ICCID, iccid, sizeof(simPtr->ICCID), NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM card IMSI.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetIMSI
(
    Sim_t* simPtr   ///< [IN] The SIM structure.
)
{
  pa_sim_Imsi_t    imsi;

  if (LE_OK != pa_sim_GetIMSI(imsi))
  {
      return LE_FAULT;
  }

  return le_utf8_Copy(simPtr->IMSI, imsi, sizeof(simPtr->IMSI), NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM Phone Number.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetPhoneNumber
(
    Sim_t* simPtr   ///< [IN] The SIM structure.
)
{
  char phoneNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = {0};

  if (LE_OK != pa_sim_GetSubscriberPhoneNumber(phoneNumber, sizeof(phoneNumber)))
  {
      return LE_FAULT;
  }

  return le_utf8_Copy(simPtr->phoneNumber, phoneNumber, sizeof(simPtr->phoneNumber), NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM card information.
 *
 */
//--------------------------------------------------------------------------------------------------
static void GetSimCardInformation
(
    Sim_t*          simPtr,
    le_sim_States_t state
)
{
    switch(state)
    {
        case LE_SIM_ABSENT:
            simPtr->ICCID[0] = '\0';
            simPtr->IMSI[0] = '\0';
            simPtr->EID[0] = '\0';
            simPtr->phoneNumber[0] = '\0';
            simPtr->isPresent = false;
            break;

        case LE_SIM_INSERTED:
        case LE_SIM_BLOCKED:
            simPtr->isPresent = true;
            simPtr->IMSI[0] = '\0';
            simPtr->EID[0] = '\0';
            GetICCID(simPtr);
            GetEID(simPtr);
            break;

        case LE_SIM_READY:
            simPtr->isPresent = true;
            // Get identification information
            GetICCID(simPtr);
            GetIMSI(simPtr);
            GetEID(simPtr);
            break;

        case LE_SIM_BUSY:
        case LE_SIM_STATE_UNKNOWN:
            simPtr->isPresent = true;
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for new SIM state notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static void NewSimStateHandler
(
    pa_sim_Event_t* eventPtr
)
{
    Sim_t*           simPtr;
    Sim_Event_t      simEvent;

    LE_DEBUG("New SIM state.%d for sim identifier.%d (eventPtr %p)", eventPtr->state,
        eventPtr->simId, eventPtr);
    simPtr = &SimList[eventPtr->simId];
    GetSimCardInformation(simPtr, eventPtr->state);

    // Discard transitional states
    switch (eventPtr->state)
    {
        case LE_SIM_BUSY:
        case LE_SIM_STATE_UNKNOWN:
            LE_DEBUG("Discarding report for sim identifie.%d, state.%d",
                            eventPtr->simId, eventPtr->state);
            return;

        default:
            break;
    }

    if ( eventPtr->simId != SelectedCard )
    {
        LE_DEBUG("New selected card");
        SelectedCard = eventPtr->simId;
    }

    // Notify all the registered client handlers
    simEvent.simId = eventPtr->simId;
    simEvent.state = eventPtr->state;
    le_event_Report(NewSimStateEventId, &simEvent, sizeof(simEvent));
    LE_DEBUG("Report state %d on SIM Id %d", simEvent.state, simEvent.simId);

    le_mem_Release(eventPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer SIM Toolkit events Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerSimToolkitHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    pa_sim_StkEvent_t*                  stkEventPtr = reportPtr;
    le_sim_SimToolkitEventHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    LE_DEBUG("Report stkEvent %d on SIM Id %d", stkEventPtr->stkEvent, stkEventPtr->simId);

    clientHandlerFunc(stkEventPtr->simId, stkEventPtr->stkEvent, le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SIM Toolkit events.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SimToolkitHandler
(
    pa_sim_StkEvent_t* eventPtr
)
{
    LE_DEBUG("Report stkEvent %d on SIM Id %d", eventPtr->stkEvent, eventPtr->simId);
    le_event_Report(SimToolkitEventId, eventPtr, sizeof(pa_sim_StkEvent_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function tests the SIM validity
 *
 * @return LE_OK            On success.
 * @return LE_NOT_FOUND     The function failed to select the SIM card for this operation.
 * @return LE_BAD_PARAMETER Invalid SIM identifier.
 * @return LE_FAULT         The function failed to get the number of remaining PIN insertion tries.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CheckSimValidity
(
    le_sim_Id_t simId   ///< [IN] The SIM identifier.
)
{
    Sim_t*    simPtr;
    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }

    if (LE_OK != SelectSIMCard(simId))
    {
        return LE_NOT_FOUND;
    }

    simPtr = &SimList[simId];

    if (!simPtr->isPresent)
    {
        return LE_NOT_FOUND;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the current FPLMN operator list.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 *      - LE_BAD_PARAMETER when bad parameter given into this function
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetFPLMNOperatorsList
(
    le_dls_List_t* FPLMNListPtr    ///< [IN/OUT] The FPLMN operators list.
)
{
    uint32_t total = 0;
    le_result_t res;

    if (NULL == FPLMNListPtr)
    {
        LE_ERROR("FPLMNListPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if (LE_OK == pa_sim_CountFPLMNOperators(&total))
    {
        if (0 == total)
        {
            LE_DEBUG("FPLMN list is empty");
        }
        else
        {
            size_t i;
            pa_sim_FPLMNOperator_t paFPLMNOperatorPtr[total];
            memset(paFPLMNOperatorPtr, 0, sizeof(pa_sim_FPLMNOperator_t)*total);

            res = pa_sim_ReadFPLMNOperators(paFPLMNOperatorPtr, &total);

            if (LE_OK == res)
            {
                for (i = 0; i < total; i++)
                {
                    pa_sim_FPLMNOperator_t* FPLMNOperatorPtr =
                        (pa_sim_FPLMNOperator_t*)le_mem_ForceAlloc(FPLMNOperatorPool);

                    memcpy(FPLMNOperatorPtr, &paFPLMNOperatorPtr[i],
                           sizeof(pa_sim_FPLMNOperator_t));

                    FPLMNOperatorPtr->link = LE_DLS_LINK_INIT;
                    le_dls_Queue(FPLMNListPtr, &(FPLMNOperatorPtr->link));

                    LE_DEBUG("MCC.%s MNC.%s", FPLMNOperatorPtr->mobileCode.mcc,
                                              FPLMNOperatorPtr->mobileCode.mnc);
                }
            }
        }

        return LE_OK;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get FPLMN operator code (MCC and  MNC) from FPLMN operator link.
 *
 * @return
 *      - LE_FAULT         Function failed.
 *      - LE_OVERFLOW      MCC/MNC string size is greater than string length parameter which has
 *                         been given into this function.
 *      - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetFPLMNOperator
(
    le_sim_FPLMNList_t* FPLMNListPtr,         ///< [IN] FPLMN list pointer.
    le_dls_Link_t* FPLMNLinkPtr,              ///< [IN] FPLMN operator link.
    char* mccPtr,                             ///< [OUT] Mobile Country Code.
    size_t mccPtrSize,                        ///< [IN] Size of Mobile Country Code.
    char* mncPtr,                             ///< [OUT] Mobile Network Code.
    size_t mncPtrSize                         ///< [IN] Size of Mobile Network Code.
)
{
    pa_sim_FPLMNOperator_t* FPLMNOperatorPtr;

    if (NULL != FPLMNLinkPtr)
    {
        FPLMNOperatorPtr = CONTAINER_OF(FPLMNLinkPtr, pa_sim_FPLMNOperator_t, link);
        FPLMNListPtr->currentLink = FPLMNLinkPtr;

        if (LE_OVERFLOW == le_utf8_Copy(mccPtr, FPLMNOperatorPtr->mobileCode.mcc, mccPtrSize, NULL))
        {
            LE_ERROR("Mobile Country Code string size is greater than mccPtrSize");
            return LE_OVERFLOW;
        }

        if (LE_OVERFLOW == le_utf8_Copy(mncPtr, FPLMNOperatorPtr->mobileCode.mnc, mncPtrSize, NULL))
        {
            LE_ERROR("Mobile Network Code string size is greater than mncPtrNumElements");
            return LE_OVERFLOW;
        }

        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to clear the FPLMN list.
 *
 * @return
 *      - LE_OK             on success
 *      - LE_FAULT          for all other errors
 */
//--------------------------------------------------------------------------------------------------
static void DeleteFPLMNOperatorsList
(
    le_dls_List_t *FPLMNOperatorsListPtr ///< [IN] List of FPLMN operators.
)
{
    pa_sim_FPLMNOperator_t* nodePtr;
    le_dls_Link_t *linkPtr;

    while ((linkPtr = le_dls_Pop(FPLMNOperatorsListPtr)) != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, pa_sim_FPLMNOperator_t, link);
        le_mem_Release(nodePtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function to the close session service.
 *
 */
//--------------------------------------------------------------------------------------------------
static void CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] Session reference of client application.
    void*               contextPtr   ///< [IN] Context pointer of CloseSessionEventHandler.
)
{
    if (!sessionRef)
    {
        LE_ERROR("ERROR sessionRef is NULL");
        return;
    }

    // Clean session context.
    LE_ERROR("SessionRef (%p) has been closed", sessionRef);

    le_ref_IterRef_t iterRef = le_ref_GetIterator(FPLMNListRefMap);
    le_result_t result = le_ref_NextNode(iterRef);
    while (LE_OK == result)
    {
        le_sim_FPLMNList_t* FPLMNListPtr = (le_sim_FPLMNList_t*)le_ref_GetValue(iterRef);

        // Check if the session reference saved matchs with the current session reference.
        if (FPLMNListPtr->sessionRef == sessionRef)
        {
            le_sim_FPLMNListRef_t FPLMNListRef = (le_sim_FPLMNListRef_t) le_ref_GetSafeRef(iterRef);

            LE_DEBUG("Release FPLMNList reference 0x%p, sessionRef 0x%p", FPLMNListRef, sessionRef);

            // Release message List.
            le_sim_DeleteFPLMNList(FPLMNListRef);
        }
        // Get the next value in the reference map.
        result = le_ref_NextNode(iterRef);
    }
}

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the SIM operations component
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_Init
(
    void
)
{
    int i;

    // Initialize the SIM list
    for (i = 0; i < LE_SIM_ID_MAX; i++)
    {
        SimList[i].simId = i;
        SimList[i].ICCID[0] = '\0';
        SimList[i].IMSI[0] = '\0';
        SimList[i].EID[0] = '\0';
        SimList[i].phoneNumber[0] = '\0';
        SimList[i].isPresent = false;
        SimList[i].subscription = UNKNOWN_SUBSCRIPTION;
        GetSimCardInformation(&SimList[i], LE_SIM_ABSENT);
    }

    // Create FPLMN list pool
    FPLMNListPool = le_mem_CreatePool("FPLMNListPool", sizeof(le_sim_FPLMNList_t));

    // Create the Safe Reference Map to use for FPLMN Operator list
    FPLMNListRefMap = le_ref_CreateMap("FPLMNListRefMap", MAX_NUM_FPLMN_LISTS);

    // Create the FPLMN operator memory pool
    FPLMNOperatorPool = le_mem_CreatePool("FPLMNOperatorPool", sizeof(pa_sim_FPLMNOperator_t));

    // Add a handler to the close session service.
    le_msg_AddServiceCloseHandler(le_sim_GetServiceRef(), CloseSessionEventHandler, NULL);

    // Create an event Id for new SIM state notifications
    NewSimStateEventId = le_event_CreateId("NewSimStateEventId", sizeof(Sim_Event_t));

    // Create an event Id for SIM Toolkit notifications
    SimToolkitEventId = le_event_CreateId("SimToolkitEventId", sizeof(pa_sim_StkEvent_t));

    SimToolkitHandlerCount = 0;
    PaSimToolkitHandlerRef = NULL;

    // Register a handler function for new SIM state notification
    if (pa_sim_AddNewStateHandler(NewSimStateHandler) == NULL)
    {
        LE_CRIT("Add new SIM state handler failed");
        return LE_FAULT;
    }

    if (pa_sim_GetSelectedCard(&SelectedCard) != LE_OK)
    {
        LE_CRIT("Unable to get selected card.");
        return LE_FAULT;
    }
    GetSimCardInformation(&SimList[SelectedCard], LE_SIM_STATE_UNKNOWN);

    LE_DEBUG("SIM %u is selected.", SelectedCard);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the current selected card.
 *
 * @return The number of the current selected SIM card.
 */
//--------------------------------------------------------------------------------------------------
le_sim_Id_t le_sim_GetSelectedCard
(
    void
)
{
    return SelectedCard;
}

//--------------------------------------------------------------------------------------------------
/**
 * Select a SIM.
 *
 * @return LE_FAULT         Function failed to select the requested SIM
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_SelectCard
(
    le_sim_Id_t simId   ///< [IN] The SIM identifier.
)
{
    // Select the SIM card
    if (SelectSIMCard(simId) != LE_OK)
    {
        LE_ERROR("Unable to select Sim Card slot %d !", simId);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function retrieves the integrated circuit card identifier (ICCID) of the
 *  SIM card (20 digits)
 *
 * @return LE_OK            The ICCID was successfully retrieved.
 * @return LE_OVERFLOW      The iccidPtr buffer was too small for the ICCID.
 * @return LE_BAD_PARAMETER if a parameter is invalid
 * @return LE_FAULT         The ICCID could not be retrieved.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetICCID
(
    le_sim_Id_t simId,        ///< [IN] The SIM identifier.
    char *      iccidPtr,     ///< [OUT] Buffer to hold the ICCID.
    size_t      iccidLen      ///< [IN] Buffer length
)
{
    le_sim_States_t  state;
    le_result_t      res;
    Sim_t*           simPtr = NULL;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if (NULL == iccidPtr)
    {
        LE_KILL_CLIENT("iccidPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if ((LE_OK == SelectSIMCard(simPtr->simId)) &&
        (LE_OK == pa_sim_GetState(&state)) &&
        ((LE_SIM_INSERTED == state) || (LE_SIM_READY == state) || (LE_SIM_BLOCKED == state))
       )
    {
        // Get ICCID
        res = GetICCID(simPtr);
    }
    else
    {
        LE_ERROR("Failed to get the ICCID of sim identifier.%d", simPtr->simId);
        simPtr->ICCID[0] = '\0';
        return LE_FAULT;
    }

    // The ICCID is available. Copy it to the result buffer.
    if (LE_OK == res)
    {
        res = le_utf8_Copy(iccidPtr, simPtr->ICCID, iccidLen, NULL);
    }

    // If the ICCID could not be retrieved for some reason, or a truncation error occurred when
    // copying the result, then ensure the cache is cleared.
    if (LE_OK != res)
    {
        LE_ERROR("Failed to get the ICCID of sim identifier.%d", simPtr->simId);
        simPtr->ICCID[0] = '\0';
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 *  This function retrieves the identifier for the embedded Universal Integrated Circuit Card
 *  identifier (EID) (32 digits)
 *
 * @return LE_OK             EID was successfully retrieved.
 * @return LE_OVERFLOW       eidPtr buffer was too small for the EID.
 * @return LE_BAD_PARAMETER  Invalid parameters.
 * @return LE_FAULT          The EID could not be retrieved.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t  le_sim_GetEID
(
    le_sim_Id_t simId,      ///< [IN] The SIM identifier.
    char*       eidPtr,     ///< [OUT] Buffer to hold the EID.
    size_t      eidLen      ///< [IN] Buffer length.
)
{
    le_sim_States_t  state;
    le_result_t      res;
    Sim_t*           simPtr = NULL;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if (NULL == eidPtr)
    {
        LE_KILL_CLIENT("eidPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if ((LE_OK == SelectSIMCard(simPtr->simId)) &&
        (LE_OK == pa_sim_GetState(&state)) &&
        ((LE_SIM_INSERTED == state) || (LE_SIM_READY == state) || (LE_SIM_BLOCKED == state))
       )
    {
        res = GetEID(simPtr);
    }
    else
    {
        LE_ERROR("Failed to get the EID of sim identifier.%d", simPtr->simId);
        simPtr->EID[0] = '\0';
        return LE_FAULT;
    }

    // The EID is available. Copy it to the result buffer.
    if (LE_OK == res)
    {
        res = le_utf8_Copy(eidPtr, simPtr->EID, eidLen, NULL);
    }

    // If the EID could not be retrieved for some reason, or a truncation error occurred when
    // copying the result, then ensure the cache is cleared.
    if (LE_OK != res)
    {
        LE_ERROR("Failed to get the EID of sim identifier.%d", simPtr->simId);
        simPtr->EID[0] = '\0';
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function retrieves the identification number (IMSI) of the SIM card. (max 15 digits)
 *
 * @return LE_OK            The IMSI was successfully retrieved.
 * @return LE_OVERFLOW      The imsiPtr buffer was too small for the IMSI.
 * @return LE_BAD_PARAMETER if a parameter is invalid
 * @return LE_FAULT         The IMSI could not be retrieved.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetIMSI
(
    le_sim_Id_t simId,       ///< [IN] The SIM identifier.
    char *      imsiPtr,     ///< [OUT] Buffer to hold the IMSI.
    size_t      imsiLen      ///< [IN] Buffer length
)
{
    le_sim_States_t  state;
    le_result_t      res;
    Sim_t*           simPtr = NULL;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if (NULL == imsiPtr)
    {
        LE_KILL_CLIENT("imsiPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if ((LE_OK == SelectSIMCard(simPtr->simId)) &&
        (LE_OK == pa_sim_GetState(&state)) &&
        (LE_SIM_READY == state)
       )
    {
        // Get identification information
        res = GetIMSI(simPtr);
    }
    else
    {
        LE_ERROR("Failed to get the IMSI of sim identifier.%d", simPtr->simId);
        simPtr->IMSI[0] = '\0';
        return LE_FAULT;
    }

    // The IMSI is available. Copy it to the result buffer.
    if (LE_OK == res)
    {
        res = le_utf8_Copy(imsiPtr, simPtr->IMSI, imsiLen, NULL);
    }

    // If the IMSI could not be retrieved for some reason, or a truncation error occurred when
    // copying the result, then ensure the cache is cleared.
    if (LE_OK != res)
    {
        LE_ERROR("Failed to get the IMSI of sim identifier.%d", simPtr->simId);
        simPtr->IMSI[0] = '\0';
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to verify if the SIM card is present or not.
 *
 * @return true  The SIM card is present.
 * @return false The SIM card is absent.
 *
 */
//--------------------------------------------------------------------------------------------------
bool le_sim_IsPresent
(
    le_sim_Id_t simId   ///< [IN] The SIM identifier.
)
{
    le_sim_States_t  state;
    Sim_t*           simPtr;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return false;
    }
    simPtr = &SimList[simId];

    if (SelectSIMCard(simId) != LE_OK)
    {
        return false;
    }

    if (pa_sim_GetState(&state) == LE_OK)
    {
        if((state != LE_SIM_ABSENT) && (state != LE_SIM_STATE_UNKNOWN))
        {
            simPtr->isPresent = true;
            return true;
        }
        else
        {
            simPtr->isPresent = false;
            return false;
        }
    }
    else
    {
        simPtr->isPresent = false;
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to verify if the SIM is ready (PIN code correctly inserted or not
 * required).
 *
 * @return true  The PIN is correctly inserted or not required.
 * @return false The PIN must be inserted.
 *
 */
//--------------------------------------------------------------------------------------------------
bool le_sim_IsReady
(
    le_sim_Id_t simId   ///< [IN] SIM identifier.
)
{
    le_sim_States_t  state;

    if (SelectSIMCard(simId) != LE_OK)
    {
        return false;
    }

    if (pa_sim_GetState(&state) == LE_OK)
    {
        if(state == LE_SIM_READY)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enter the PIN code.
 *
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_NOT_FOUND     The function failed to select the SIM card for this operation.
 * @return LE_UNDERFLOW     The PIN code is not long enough (min 4 digits).
 * @return LE_FAULT         The function failed to enter the PIN code.
 * @return LE_OK            The function succeeded.
 *
 * @note If PIN code is too long (max 8 digits), it is a fatal error, the
 *       function will not return.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_EnterPIN
(
    le_sim_Id_t  simId,     ///< [IN] SIM identifier.
    const char*  pinPtr     ///< [IN] PIN code.
)
{
    pa_sim_Pin_t pinloc;
    Sim_t*       simPtr;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if (strlen(pinPtr) > LE_SIM_PIN_MAX_LEN )
    {
        LE_KILL_CLIENT("strlen(pin) > %d", LE_SIM_PIN_MAX_LEN);
        return LE_BAD_PARAMETER;
    }

    if(strlen(pinPtr) < LE_SIM_PIN_MIN_LEN)
    {
        return LE_UNDERFLOW;
    }

    if (SelectSIMCard(simId) != LE_OK)
    {
        return LE_NOT_FOUND;
    }

    if (!simPtr->isPresent)
    {
        return LE_NOT_FOUND;
    }

    // Enter PIN
    le_utf8_Copy(pinloc, pinPtr, sizeof(pinloc), NULL);
    if(pa_sim_EnterPIN(PA_SIM_PIN,pinloc) != LE_OK)
    {
        LE_ERROR("Failed to enter PIN.%s sim identifier.%d", pinPtr, simId);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to change the PIN code.
 *
 * @return LE_NOT_FOUND     The function failed to select the SIM card for this operation.
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_UNDERFLOW     The PIN code is/are not long enough (min 4 digits).
 * @return LE_FAULT  The function failed to change the PIN code.
 * @return LE_OK            The function succeeded.
 *
 * @note If PIN code is/are too long (max 8 digits), it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_ChangePIN
(
    le_sim_Id_t   simId,     ///< [IN] SIM identifier.
    const char*   oldpinPtr, ///< [IN] Old PIN code.
    const char*   newpinPtr  ///< [IN] New PIN code.
)
{
    pa_sim_Pin_t oldpinloc;
    pa_sim_Pin_t newpinloc;
    Sim_t*       simPtr;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if(strlen(oldpinPtr) > LE_SIM_PIN_MAX_LEN)
    {
        LE_KILL_CLIENT("strlen(oldpin) > %d", LE_SIM_PIN_MAX_LEN);
        return LE_BAD_PARAMETER;
    }

    if(strlen(newpinPtr) > LE_SIM_PIN_MAX_LEN)
    {
        LE_KILL_CLIENT("strlen(newpin) > %d", LE_SIM_PIN_MAX_LEN);
        return LE_BAD_PARAMETER;
    }

    if((strlen(oldpinPtr) < LE_SIM_PIN_MIN_LEN) || (strlen(newpinPtr) < LE_SIM_PIN_MIN_LEN))
    {
        return LE_UNDERFLOW;
    }

    if (SelectSIMCard(simId) != LE_OK)
    {
        return LE_NOT_FOUND;
    }

    if (!simPtr->isPresent)
    {
        return LE_NOT_FOUND;
    }

    // Change PIN
    le_utf8_Copy(oldpinloc, oldpinPtr, sizeof(oldpinloc), NULL);
    le_utf8_Copy(newpinloc, newpinPtr, sizeof(newpinloc), NULL);
    if(pa_sim_ChangePIN(PA_SIM_PIN, oldpinloc, newpinloc) != LE_OK)
    {
        LE_ERROR("Failed to set new PIN.%s of sim identifier.%d", newpinPtr, simId);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the number of remaining PIN insertion tries.
 *
 * @return LE_NOT_FOUND     The function failed to select the SIM card for this operation.
 * @return LE_BAD_PARAMETER Invalid SIM identifier.
 * @return LE_FAULT         The function failed to get the number of remaining PIN insertion tries.
 * @return A positive value The function succeeded. The number of remaining PIN insertion tries.
 *
 */
//--------------------------------------------------------------------------------------------------
int32_t le_sim_GetRemainingPINTries
(
    le_sim_Id_t simId   ///< [IN] The SIM identifier.
)
{
    uint32_t  attempts=0;
    le_result_t res = CheckSimValidity(simId);

    if (LE_OK != res)
    {
        return res;
    }

    if (LE_OK != pa_sim_GetPINRemainingAttempts(PA_SIM_PIN, &attempts))
    {
        LE_ERROR("Failed to get remaining PIN attempts for sim identifier.%d", simId);
        return LE_FAULT;
    }

    return attempts;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the number of remaining PUK insertion tries.
 *
 * @return LE_OK            On success.
 * @return LE_NOT_FOUND     The function failed to select the SIM card for this operation.
 * @return LE_BAD_PARAMETER Invalid SIM identifier.
 * @return LE_FAULT         The function failed to get the number of remaining PUK insertion tries.
 *
 * @note If the caller is passing an null pointer to this function, it is a fatal error
 *       and the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetRemainingPUKTries
(
    le_sim_Id_t simId,                 ///< [IN] The SIM identifier.
    uint32_t*   remainingPukTriesPtr   ///< [OUT] The number of remaining PUK insertion tries.
)
{
    le_result_t res;

    if (NULL == remainingPukTriesPtr)
    {
        LE_KILL_CLIENT("remainingPukTriesPtr is NULL !");
        return LE_FAULT;
    }

    res = CheckSimValidity(simId);
    if (LE_OK != res)
    {
        return res;
    }

    if (LE_OK != pa_sim_GetPUKRemainingAttempts(PA_SIM_PUK, remainingPukTriesPtr))
    {
        LE_ERROR("Failed to get remaining PUK attempts for sim identifier.%d", simId);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unlock the SIM card: it disables the request of the PIN code.
 *
 * @return LE_NOT_FOUND     The function failed to select the SIM card for this operation.
 * @return LE_UNDERFLOW     The PIN code is not long enough (min 4 digits).
 * @return LE_FAULT         The function failed to unlock the SIM card.
 * @return LE_OK            The function succeeded.
 *
 * @note If PIN code is too long (max 8 digits), it is a fatal error, the
 *       function will not return.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_Unlock
(
    le_sim_Id_t     simId,   ///< [IN] SIM identifier.
    const char*     pinPtr   ///< [IN] PIN code.
)
{
    pa_sim_Pin_t  pinloc;
    Sim_t*        simPtr;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if(strlen(pinPtr) > LE_SIM_PIN_MAX_LEN)
    {
        LE_KILL_CLIENT("strlen(pinPtr) > %d", LE_SIM_PIN_MAX_LEN);
        return LE_BAD_PARAMETER;
    }

    if(strlen(pinPtr) < LE_SIM_PIN_MIN_LEN)
    {
        return LE_UNDERFLOW;
    }

    if (SelectSIMCard(simId) != LE_OK)
    {
        return LE_NOT_FOUND;
    }

    if (!simPtr->isPresent)
    {
        return LE_NOT_FOUND;
    }

    // Unlock SIM
    le_utf8_Copy(pinloc, pinPtr, sizeof(pinloc), NULL);
    if(pa_sim_DisablePIN(PA_SIM_PIN, pinloc) != LE_OK)
    {
        LE_ERROR("Failed to unlock sim identifier.%d", simId);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to Lock the SIM card: it enables the request of the PIN code.
 *
 * @return LE_NOT_FOUND     The function failed to select the SIM card for this operation.
 * @return LE_UNDERFLOW     The PIN code is not long enough (min 4 digits).
 * @return LE_FAULT         The function failed to unlock the SIM card.
 * @return LE_OK            The function succeeded.
 *
 * @note If PIN code is too long (max 8 digits), it is a fatal error, the
 *       function will not return.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_Lock
(
    le_sim_Id_t     simId,   ///< [IN] SIM identifier.
    const char*     pinPtr   ///< [IN] PIN code.
)
{
    pa_sim_Pin_t pinloc;
    Sim_t*       simPtr;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if(strlen(pinPtr) > LE_SIM_PIN_MAX_LEN)
    {
        LE_KILL_CLIENT("strlen(pinPtr) > %d", LE_SIM_PIN_MAX_LEN);
        return LE_BAD_PARAMETER;
    }

    if(strlen(pinPtr) < LE_SIM_PIN_MIN_LEN)
    {
        return LE_UNDERFLOW;
    }

    if (SelectSIMCard(simId) != LE_OK)
    {
        return LE_NOT_FOUND;
    }

    if (!simPtr->isPresent)
    {
        return LE_NOT_FOUND;
    }

    // Unlock card
    le_utf8_Copy(pinloc, pinPtr, sizeof(pinloc), NULL);
    if(pa_sim_EnablePIN(PA_SIM_PIN, pinloc) != LE_OK)
    {
        LE_ERROR("Failed to Lock sim identifier.%d", simId);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unblock the SIM card.
 *
 * @return LE_NOT_FOUND     The function failed to select the SIM card for this operation.
 * @return LE_UNDERFLOW     The PIN code is not long enough (min 4 digits).
 * @return LE_BAD_PARAMETER Invalid SIM identifier.
 * @return LE_OUT_OF_RANGE  The PUK code length is not correct (8 digits).
 * @return LE_FAULT         The function failed to unlock the SIM card.
 * @return LE_OK            The function succeeded.
 *
 * @note If new PIN or puk code are too long (max 8 digits), it is a fatal error, the
 *       function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_Unblock
(
    le_sim_Id_t     simId,     ///< [IN] SIM identifier.
    const char*     pukPtr,    ///< [IN] PUK code.
    const char*     newpinPtr  ///< [IN] New PIN code.
)
{
    pa_sim_Puk_t pukloc;
    pa_sim_Pin_t newpinloc;
    Sim_t*       simPtr;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if(strlen(pukPtr) != LE_SIM_PUK_MAX_LEN)
    {
        return LE_OUT_OF_RANGE;
    }

    if(strlen(newpinPtr) > LE_SIM_PIN_MAX_LEN)
    {
        LE_KILL_CLIENT("strlen(newpinPtr) > %d", LE_SIM_PIN_MAX_LEN);
        return LE_BAD_PARAMETER;
    }

    if(strlen(newpinPtr) < LE_SIM_PIN_MIN_LEN)
    {
        return LE_UNDERFLOW;
    }

    if (SelectSIMCard(simId) != LE_OK)
    {
        return LE_NOT_FOUND;
    }

    if (!simPtr->isPresent)
    {
        return LE_NOT_FOUND;
    }

    // Unblock card
    le_utf8_Copy(pukloc, pukPtr, sizeof(pukloc), NULL);
    le_utf8_Copy(newpinloc, newpinPtr, sizeof(newpinloc), NULL);
    if(pa_sim_EnterPUK(PA_SIM_PUK,pukloc, newpinloc) != LE_OK)
    {
        LE_ERROR("Failed to unblock sim identifier.%d", simId);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the SIM state.
 *
 * @return The current SIM state.
 *
 */
//--------------------------------------------------------------------------------------------------
le_sim_States_t le_sim_GetState
(
    le_sim_Id_t simId   ///< [IN] SIM identifier.
)
{
    le_sim_States_t state;

    if (SelectSIMCard(simId) != LE_OK)
    {
        return LE_SIM_STATE_UNKNOWN;
    }

    if (pa_sim_GetState(&state) == LE_OK)
    {
        return state;
    }
    else
    {
        return LE_SIM_STATE_UNKNOWN;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register an handler function for New State notification.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure; there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_sim_NewStateHandlerRef_t le_sim_AddNewStateHandler
(
    le_sim_NewStateHandlerFunc_t handlerPtr,
        ///< [IN] Handler function for New State notification.

    void* contextPtr
        ///< [IN] Handler's context.
)
{
    le_event_HandlerRef_t handlerRef;

    if (handlerPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("NewSimStateHandler",
                                            NewSimStateEventId,
                                            FirstLayerNewSimStateHandler,
                                            (le_event_HandlerFunc_t)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_sim_NewStateHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister a handler function
 *
 * @note Doesn't return on failure; there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_sim_RemoveNewStateHandler
(
    le_sim_NewStateHandlerRef_t   handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM Phone Number.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Phone Number can't fit in phoneNumberStr
 *      - LE_BAD_PARAMETER if a parameter is invalid
 *      - LE_FAULT on any other failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetSubscriberPhoneNumber
(
    le_sim_Id_t     simId,             ///< [IN] SIM identifier.
    char*           phoneNumberStr,    ///< [OUT] Phone Number
    size_t          phoneNumberStrSize ///< [IN]  Size of phoneNumberStr
)
{
    le_sim_States_t  state;
    le_result_t      res = LE_FAULT;
    Sim_t*           simPtr = NULL;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if (NULL == phoneNumberStr)
    {
        LE_KILL_CLIENT("phoneNumberStr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if ((LE_OK == SelectSIMCard(simPtr->simId)) &&
        (LE_OK == pa_sim_GetState(&state)) &&
        ((LE_SIM_INSERTED == state) || (LE_SIM_READY == state) || (LE_SIM_BLOCKED == state))
       )
    {
        // Get the phone number
        res = GetPhoneNumber(simPtr);
    }
    else
    {
        LE_ERROR("Failed to get the Phone Number of sim identifier.%d", simPtr->simId);
        simPtr->phoneNumber[0] = '\0';
        return LE_FAULT;
    }

    // The phone number is available. Copy it to the result buffer.
    if (LE_OK == res)
    {
        res = le_utf8_Copy(phoneNumberStr,simPtr->phoneNumber,phoneNumberStrSize,NULL);
    }

    // If the phone number could not be retrieved for some reason, or a truncation error occurred
    // when copying the result, then ensure the cache is cleared.
    if (LE_OK != res)
    {
        simPtr->phoneNumber[0] = '\0';
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Home Network Name information.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Home Network Name can't fit in nameStr
 *      - LE_NOT_FOUND if the network is not found
 *      - LE_BAD_PARAMETER if a parameter is invalid
 *      - LE_FAULT on any other failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetHomeNetworkOperator
(
    le_sim_Id_t        simId,          ///< [IN] SIM identifier.
    char              *nameStr,        ///< [OUT] Home network Name
    size_t             nameStrSize     ///< [IN] nameStr size
)
{
    if (SelectSIMCard(simId) != LE_OK)
    {
        return LE_FAULT;
    }

    if (nameStr == NULL)
    {
        LE_KILL_CLIENT("nameStr is NULL !");
        return LE_BAD_PARAMETER;
    }

    return pa_sim_GetHomeNetworkOperator(nameStr,nameStrSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Home Network MCC MNC.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if Home Network has not been provisioned
 *      - LE_FAULT for unexpected error
 */
//--------------------------------------------------------------------------------------------------
 le_result_t le_sim_GetHomeNetworkMccMnc
(
    le_sim_Id_t       simId,          ///< [IN] SIM identifier.
    char             *mccPtr,         ///< [OUT] Mobile Country Code
    size_t            mccPtrSize,     ///< [IN] mccPtr buffer size
    char             *mncPtr,         ///< [OUT] Mobile Network Code
    size_t            mncPtrSize      ///< [IN] mncPtr buffer size
)
{
    if (SelectSIMCard(simId) != LE_OK)
    {
        return LE_FAULT;
    }

    if (mccPtr == NULL)
    {
        LE_KILL_CLIENT("mccPtr is NULL");
        return LE_FAULT;
    }

    if (mncPtr == NULL)
    {
        LE_KILL_CLIENT("mncPtr is NULL");
        return LE_FAULT;
    }

    return pa_sim_GetHomeNetworkMccMnc(mccPtr, mccPtrSize, mncPtr, mncPtrSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to request the multi-profile eUICC to swap to ECS and to refresh.
 * The User's application must wait for eUICC reboot to be finished and network connection
 * available.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER invalid SIM identifier
 *      - LE_BUSY when a profile swap is already in progress
 *      - LE_FAULT for unexpected error
 *
 * @warning If you use a Morpho or Oberthur card, the SIM_REFRESH PRO-ACTIVE command must be
 *          accepted with le_sim_AcceptSimToolkitCommand() in order to complete the profile swap
 *          procedure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_LocalSwapToEmergencyCallSubscription
(
    le_sim_Id_t           simId,          ///< [IN] The SIM identifier.
    le_sim_Manufacturer_t manufacturer    ///< [IN] The card manufacturer.
)
{
    Sim_t* simPtr;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }

    if (LE_OK != SelectSIMCard(simId))
    {
        return LE_FAULT;
    }
    simPtr = &SimList[simId];

    //Clear sim information.
    simPtr->ICCID[0] = '\0';
    simPtr->IMSI[0] = '\0';
    simPtr->phoneNumber[0] = '\0';

    if (LE_OK == LocalSwap(manufacturer,
                           EcsSwapApduReq[manufacturer].apduReq,
                           EcsSwapApduReq[manufacturer].apduLength))
    {
        simPtr->subscription = ECS;
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to request the multi-profile eUICC to swap back to commercial
 * subscription and to refresh.
 * The User's application must wait for eUICC reboot to be finished and network connection
 * available.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER invalid SIM identifier
 *      - LE_BUSY when a profile swap is already in progress
 *      - LE_FAULT for unexpected error
 *
 * @warning If you use a Morpho or Oberthur card, the SIM_REFRESH PRO-ACTIVE command must be
 *          accepted with le_sim_AcceptSimToolkitCommand() in order to complete the profile swap
 *          procedure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_LocalSwapToCommercialSubscription
(
    le_sim_Id_t           simId,          ///< [IN] SIM identifier.
    le_sim_Manufacturer_t manufacturer    ///< [IN] The card manufacturer.
)
{
    Sim_t* simPtr;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }

    if (LE_OK != SelectSIMCard(simId))
    {
        return LE_FAULT;
    }
    simPtr = &SimList[simId];

    //Clear sim information (do not clear EID).
    simPtr->ICCID[0] = '\0';
    simPtr->IMSI[0] = '\0';
    simPtr->phoneNumber[0] = '\0';

    if (LE_OK == LocalSwap(manufacturer,
                           CommercialSwapApduReq[manufacturer].apduReq,
                           CommercialSwapApduReq[manufacturer].apduLength))
    {
        simPtr->subscription = COMMERCIAL;
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the current subscription.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER invalid SIM identifier or null ECS pointer is passed
 *      - LE_NOT_FOUND cannot determine the current selected subscription
 *      - LE_FAULT for unexpected errors
 *
 * @warning There is no standard method to interrogate the current selected subscription. The
 * returned value of this function is based on the last executed local swap command. This means
 * that this function will always return LE_NOT_FOUND error at Legato startup.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_IsEmergencyCallSubscriptionSelected
(
    le_sim_Id_t simId,   ///< [IN] SIM identifier.
    bool*       isEcsPtr ///< [OUT] true if Emergency Call Subscription (ECS) is selected,
                         ///<       false if Commercial Subscription is selected
)
{
    le_result_t res;
    Sim_t*      simPtr;

    if (NULL == isEcsPtr)
    {
        LE_ERROR("isEcsPtr is NULL!");
        return LE_BAD_PARAMETER;;
    }

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }

    if (SelectSIMCard(simId) != LE_OK)
    {
        return LE_FAULT;
    }
    simPtr = &SimList[simId];

    switch (simPtr->subscription)
    {
        case COMMERCIAL:
            *isEcsPtr = false;
            res = LE_OK;
            break;

        case ECS:
            *isEcsPtr = true;
            res = LE_OK;
            break;

        case UNKNOWN_SUBSCRIPTION:
            *isEcsPtr = false;
            res = LE_NOT_FOUND;
            break;

        default:
            *isEcsPtr = false;
            res = LE_FAULT;
            break;
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register an handler function for Sim Toolkit notification.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure; there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_sim_SimToolkitEventHandlerRef_t le_sim_AddSimToolkitEventHandler
(
    le_sim_SimToolkitEventHandlerFunc_t handlerPtr,
        ///< [IN] Handler function for New State notification.

    void* contextPtr
        ///< [IN] Handler's context.
)
{
    le_event_HandlerRef_t handlerRef;

    if (handlerPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    if (!SimToolkitHandlerCount)
    {
        // Register a handler function for SIM Toolkit notification
        PaSimToolkitHandlerRef = pa_sim_AddSimToolkitEventHandler(SimToolkitHandler, NULL);

        if (!PaSimToolkitHandlerRef)
        {
            LE_ERROR("Add PA SIM Toolkit handler failed");
            return NULL;
        }
    }

    handlerRef = le_event_AddLayeredHandler("SimToolkitHandler",
                                            SimToolkitEventId,
                                            FirstLayerSimToolkitHandler,
                                            (le_event_HandlerFunc_t)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);
    SimToolkitHandlerCount++;

    return (le_sim_SimToolkitEventHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister a handler function
 *
 * @note Doesn't return on failure; there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_sim_RemoveSimToolkitEventHandler
(
    le_sim_SimToolkitEventHandlerRef_t   handlerRef ///< [IN] Handler reference.
)
{
    SimToolkitHandlerCount--;
    if (!SimToolkitHandlerCount)
    {
        pa_sim_RemoveSimToolkitEventHandler(PaSimToolkitHandlerRef);
    }
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Accept the last SIM Toolkit command.
 *
 * @return LE_FAULT    Function failed.
 * @return LE_OK       Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_AcceptSimToolkitCommand
(
    le_sim_Id_t         simId        ///< [IN] SIM identifier.
)
{
    if (SelectSIMCard(simId) != LE_OK)
    {
        return LE_FAULT;
    }
    else
    {
        return pa_sim_ConfirmSimToolkitCommand(true);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Reject the last SIM Toolkit command.
 *
 * @return LE_FAULT     Function failed.
 * @return LE_OK        Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_RejectSimToolkitCommand
(
    le_sim_Id_t         simId        ///< [IN] SIM identifier.
)
{
    if (SelectSIMCard(simId) != LE_OK)
    {
        return LE_FAULT;
    }
    else
    {
        return pa_sim_ConfirmSimToolkitCommand(false);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Send APDU command to the SIM.
 *
 * @return
 *      - LE_OK             Function succeeded.
 *      - LE_FAULT          The function failed.
 *      - LE_BAD_PARAMETER  A parameter is invalid.
 *      - LE_NOT_FOUND      The function failed to select the SIM card for this operation.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_SendApdu
(
    le_sim_Id_t simId,                  ///< [IN] The SIM identifier.
    const uint8_t* commandApduPtr,      ///< [IN] APDU command.
    size_t commandApduNumElements,      ///< [IN]
    uint8_t* responseApduPtr,           ///< [OUT] SIM response.
    size_t* responseApduNumElementsPtr  ///< [INOUT]
)
{
    le_result_t res;
    uint8_t  channel = 0;

    if ((commandApduNumElements > LE_SIM_APDU_MAX_BYTES) ||
                    (*responseApduNumElementsPtr > LE_SIM_RESPONSE_MAX_BYTES))
    {
        LE_ERROR("Too many elements");
        return LE_BAD_PARAMETER;
    }

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }

    if (SelectSIMCard(simId) != LE_OK)
    {
        return LE_NOT_FOUND;
    }

    // Get the logical channel to send APDU command.
    if (LE_OK != pa_sim_OpenLogicalChannel(&channel))
    {
        LE_WARN("Can't open logical channel");
    }

    res = pa_sim_SendApdu(channel,
        commandApduPtr,
        commandApduNumElements,
        responseApduPtr,
        responseApduNumElementsPtr);

    // Close the logical channel.
    if (LE_OK != pa_sim_CloseLogicalChannel(channel))
    {
        LE_WARN("Can't close logical channel");
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a command to the SIM.
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
le_result_t le_sim_SendCommand
(
    le_sim_Id_t simId,              ///< [IN] The SIM identifier.
    le_sim_Command_t command,       ///< [IN] The SIM command.
    const char* fileIdentifier,     ///< [IN] File identifier
    uint8_t p1,                     ///< [IN] Parameter P1 passed to the SIM
    uint8_t p2,                     ///< [IN] Parameter P2 passed to the SIM
    uint8_t p3,                     ///< [IN] Parameter P3 passed to the SIM
    const uint8_t* dataPtr,         ///< [IN] data command.
    size_t dataNumElements,         ///< [IN]
    const char* path,               ///< [IN] path of the elementary file
    uint8_t* sw1Ptr,                ///< [OUT] SW1 received from the SIM
    uint8_t* sw2Ptr,                ///< [OUT] SW2 received from the SIM
    uint8_t* responsePtr,           ///< [OUT] SIM response.
    size_t* responseNumElementsPtr  ///< [INOUT]
)
{
    if ((simId >= LE_SIM_ID_MAX) ||
        (command >= LE_SIM_COMMAND_MAX) ||
        (dataNumElements > LE_SIM_DATA_MAX_BYTES) ||
        (*responseNumElementsPtr > LE_SIM_RESPONSE_MAX_BYTES))
    {
        LE_ERROR("Invalid argument");
        return LE_BAD_PARAMETER;
    }

    return pa_sim_SendCommand( command,
                               fileIdentifier,
                               p1,
                               p2,
                               p3,
                               dataPtr,
                               dataNumElements,
                               path,
                               sw1Ptr,
                               sw2Ptr,
                               responsePtr,
                               responseNumElementsPtr
                             );
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset the SIM.
 *
 * @return
 *      - LE_FAULT         Function failed.
 *      - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_Reset
(
    le_sim_Id_t simId        ///< [IN] The SIM identifier.
)
{
    if (LE_OK != le_sim_SelectCard(simId))
    {
        LE_ERROR("Not able to select the SIM");
        return LE_FAULT;
    }

    if (LE_OK != pa_sim_Reset())
    {
        LE_ERROR("Not able to reset the SIM");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create empty FPLMN list to insert FPLMN operators.
 *
 * @return
 *      - Reference to the List object.
 *      - Null pointer if not able to create list reference.
 */
//--------------------------------------------------------------------------------------------------
le_sim_FPLMNListRef_t le_sim_CreateFPLMNList
(
    void
)
{
    le_sim_FPLMNList_t* FPLMNListPtr = (le_sim_FPLMNList_t*)le_mem_ForceAlloc(FPLMNListPool);

    FPLMNListPtr->list = LE_DLS_LIST_INIT;
    FPLMNListPtr->currentLink = NULL;

    // Store client session reference.
    FPLMNListPtr->sessionRef = le_sim_GetClientSessionRef();

    // Create and return a Safe Reference for this List object.
    FPLMNListPtr->FPLMNListRef = le_ref_CreateRef(FPLMNListRefMap, FPLMNListPtr);
    return FPLMNListPtr->FPLMNListRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add FPLMN network into the newly created FPLMN list.
 * If the FPLMNListRef, mcc or mnc is not valid then this function will kill the calling client.
 *
 * @return
 *      - LE_FAULT         Function failed.
 *      - LE_OK            Function succeeded.
 *      - LE_OVERFLOW      If FPLMN operator can not be inserted into FPLMN list.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_AddFPLMNOperator
(
    le_sim_FPLMNListRef_t FPLMNListRef,    ///< [IN] FPLMN list reference.
    const char* mcc,                       ///< [IN] Mobile Country Code.
    const char* mnc                        ///< [IN] Mobile Network Code.
)
{
    pa_sim_FPLMNOperator_t* FPLMNOperatorPtr;

    le_sim_FPLMNList_t *FPLMNListPtr = le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);
    if (NULL == FPLMNListPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", FPLMNListRef);
        return LE_FAULT;
    }

    if (LE_OK != le_mrc_TestMccMnc(mcc, mnc))
    {
        LE_KILL_CLIENT("Invalid mcc or mnc");
        return LE_FAULT;
    }

    FPLMNOperatorPtr = le_mem_ForceAlloc(FPLMNOperatorPool);

    le_utf8_Copy(FPLMNOperatorPtr->mobileCode.mcc, mcc, LE_MRC_MCC_BYTES, NULL);
    le_utf8_Copy(FPLMNOperatorPtr->mobileCode.mnc, mnc, LE_MRC_MNC_BYTES, NULL);
    FPLMNOperatorPtr->link = LE_DLS_LINK_INIT;

    le_dls_Queue(&(FPLMNListPtr->list), &(FPLMNOperatorPtr->link));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write FPLMN list into the SIM.
 * If the FPLMNListRef is not valid then this function will kill the calling client.
 *
 * @return
 *      - LE_FAULT         Function failed.
 *      - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_WriteFPLMNList
(
    le_sim_Id_t simId,                    ///< [IN] The SIM identifier.
    le_sim_FPLMNListRef_t FPLMNListRef    ///< [IN] FPLMN list reference.
)
{
    le_sim_FPLMNList_t *FPLMNListPtr = le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);
    if (NULL == FPLMNListPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", FPLMNListRef);
        return LE_FAULT;
    }

    if (LE_OK != le_sim_SelectCard(simId))
    {
        LE_ERROR("Not able to select the SIM");
        return LE_FAULT;
    }

    if (LE_OK != pa_sim_WriteFPLMNList(&(FPLMNListPtr->list)))
    {
        LE_ERROR("Could not write FPLMN list into the SIM");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read FPLMN list from the SIM.
 *
 * @return
 *      - Reference to the List object.
 *      - Null pointer if FPLMN list is not able to read from the SIM.
 */
//--------------------------------------------------------------------------------------------------
le_sim_FPLMNListRef_t le_sim_ReadFPLMNList
(
    le_sim_Id_t simId                       ///< [IN] The SIM identifier.
)
{
    le_sim_FPLMNList_t* FPLMNListPtr = (le_sim_FPLMNList_t*)le_mem_ForceAlloc(FPLMNListPool);

    FPLMNListPtr->list = LE_DLS_LIST_INIT;
    FPLMNListPtr->currentLink = NULL;

    le_result_t res = GetFPLMNOperatorsList(&(FPLMNListPtr->list));

    if (LE_OK == res)
    {
        // Store client session reference.
        FPLMNListPtr->sessionRef = le_sim_GetClientSessionRef();

        // Create and return a Safe Reference for this List object.
        FPLMNListPtr->FPLMNListRef = le_ref_CreateRef(FPLMNListRefMap, FPLMNListPtr);
        return FPLMNListPtr->FPLMNListRef;
    }
    else
    {
        LE_ERROR("Not able to read the FPLMN List from the SIM");
        le_mem_Release(FPLMNListPtr);
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the first FPLMN network from the list of FPLMN objects retrieved with
 * le_sim_ReadFPLMNList().
 * If the FPLMNListRef, mccPtr or mncPtr is not valid then this function will kill the calling
 * client.
 *
 * @return
 *      - LE_FAULT         Function failed.
 *      - LE_OVERFLOW      MCC/MNC string size is greater than string length parameter which has
 *                         been given into this function.
 *      - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetFirstFPLMNOperator
(
    le_sim_FPLMNListRef_t FPLMNListRef,       ///< [IN] FPLMN list reference.
    char* mccPtr,                             ///< [OUT] Mobile Country Code.
    size_t mccPtrSize,                        ///< [IN] Size of Mobile Country Code.
    char* mncPtr,                             ///< [OUT] Mobile Network Code.
    size_t mncPtrSize                         ///< [IN] Size of Mobile Network Code.
)
{
    le_dls_Link_t*          FPLMNLinkPtr;
    le_sim_FPLMNList_t*     FPLMNListPtr = le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);

    if (NULL == FPLMNListPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", FPLMNListRef);
        return LE_FAULT;
    }

    if (NULL == mccPtr)
    {
        LE_KILL_CLIENT("mccPtr is NULL !");
        return LE_FAULT;
    }

    if (NULL == mncPtr)
    {
        LE_KILL_CLIENT("mncPtr is NULL !");
        return LE_FAULT;
    }

    FPLMNLinkPtr = le_dls_Peek(&(FPLMNListPtr->list));

    // Get MCC/MNC code from FPLMN list.
    return GetFPLMNOperator(FPLMNListPtr, FPLMNLinkPtr, mccPtr, mccPtrSize, mncPtr, mncPtrSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the next FPLMN network from the list of FPLMN objects retrieved with le_sim_ReadFPLMNList().
 * If the FPLMNListRef, mccPtr or mncPtr is not valid then this function will kill the calling
 * client.
 *
 * @return
 *      - LE_FAULT         Function failed.
 *      - LE_OVERFLOW      MCC/MNC string size is greater than string length parameter which has
 *                         been given into this function.
 *      - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetNextFPLMNOperator
(
    le_sim_FPLMNListRef_t FPLMNListRef,       ///< [IN] FPLMN list reference.
    char* mccPtr,                             ///< [OUT] Mobile Country Code.
    size_t mccPtrSize,                        ///< [IN] Size of Mobile Country Code.
    char* mncPtr,                             ///< [OUT] Mobile Network Code.
    size_t mncPtrSize                         ///< [IN] Size of Mobile Network Code.
)
{
    le_dls_Link_t*          FPLMNLinkPtr;
    le_sim_FPLMNList_t*     FPLMNListPtr = le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);

    if (NULL == FPLMNListPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", FPLMNListRef);
        return LE_FAULT;
    }

    if (NULL == mccPtr)
    {
        LE_KILL_CLIENT("mccPtr is NULL !");
        return LE_FAULT;
    }

    if (NULL == mncPtr)
    {
        LE_KILL_CLIENT("mncPtr is NULL !");
        return LE_FAULT;
    }

    FPLMNLinkPtr = le_dls_PeekNext(&(FPLMNListPtr->list), FPLMNListPtr->currentLink);

    // Get MCC/MNC code from FPLMN list.
    return GetFPLMNOperator(FPLMNListPtr, FPLMNLinkPtr, mccPtr, mccPtrSize, mncPtr, mncPtrSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete the FPLMN list created by le_sim_ReadFPLMNList() or le_sim_CreateFPLMNList().
 * If the FPLMNListRef is not valid then this function will kill the calling client.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
void le_sim_DeleteFPLMNList
(
    le_sim_FPLMNListRef_t FPLMNListRef               ///< [IN] FPLMN list reference.
)
{
    le_sim_FPLMNList_t* FPLMNListPtr = le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);

    if (FPLMNListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", FPLMNListRef);
        return;
    }

    FPLMNListPtr->currentLink = NULL;

    // Release FPLMN Operator entries from the list.
    DeleteFPLMNOperatorsList(&(FPLMNListPtr->list));

    // Invalidate the Safe Reference.
    le_ref_DeleteRef(FPLMNListRefMap, FPLMNListRef);

    // Release FPLMN list object.
    le_mem_Release(FPLMNListPtr);
}
