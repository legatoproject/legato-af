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
#include "mdmCfgEntries.h"

//--------------------------------------------------------------------------------------------------
/**
 *  Maximum FPLMN list count.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_FPLMN_LISTS 1

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of expected FPLMN operators
 */
//--------------------------------------------------------------------------------------------------
#define HIGH_FPLMN_OPERATOR_COUNT 5

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
    COMMERCIAL,           ///< Commercial subscription
    ECS,                  ///< Emergency Call subscription
    UNKNOWN_SUBSCRIPTION, ///< Unknown subscription
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
    le_sim_Id_t      simId;                      ///< SIM identifier
    char             ICCID[LE_SIM_ICCID_BYTES];  ///< Integrated circuit card identifier
    char             IMSI[LE_SIM_IMSI_BYTES];    ///< International mobile subscriber identity
    char             PIN[LE_SIM_PIN_MAX_BYTES];  ///< PIN code
    char             PUK[LE_SIM_PUK_MAX_BYTES];  ///< PUK code
    char             EID[LE_SIM_EID_BYTES];      ///< eUICCID unique identifier (EID)
    char             phoneNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES]; /// < The Phone Number
    bool             isPresent;                  ///< 'isPresent' flag
    bool             isReacheable;               ///< SIM is reachable when its state is inserted,
                                                 ///< ready or blocked
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
    le_sim_States_t  state;   ///< SIM state
}
Sim_Event_t;

//--------------------------------------------------------------------------------------------------
/**
 * ICCID change event.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sim_Id_t      simId;                       ///< SIM identififier
    char             ICCID[LE_SIM_ICCID_BYTES];   ///< Integrated circuit card identifier
}
Sim_IccidChangeEvent_t;

//--------------------------------------------------------------------------------------------------
/**
 * FPLMN list structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sim_FPLMNListRef_t FPLMNListRef;      ///< FPLMN List reference
    le_msg_SessionRef_t   sessionRef;        ///< Client session reference
    le_dls_List_t         list;              ///< Link list to insert new FPLMN operator
    le_dls_Link_t*        currentLink;       ///< Link list pointed to current FPLMN operator
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
 */
//--------------------------------------------------------------------------------------------------
static le_sim_Id_t  SelectedCard;

//--------------------------------------------------------------------------------------------------
/**
 * During initialization of the service, each new subscription to the ICCID change event is notified
 * by the last monitored event.
 * On expiry, this timer is used to stop this special behavior of the service.
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t EventTimerRef;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for SIM profile notification.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t SimProfileEventId;


//--------------------------------------------------------------------------------------------------
/**
 * Event ID for New SIM state notification.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NewSimStateEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for SIM Toolkit notification.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t SimToolkitEventId;

#if LE_CONFIG_ENABLE_CONFIG_TREE
//--------------------------------------------------------------------------------------------------
/**
 * Event ID for ICCID change notification.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t IccidChangeEventId;
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Counter for SIM Toolkit event handlers.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t SimToolkitHandlerCount = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Counter for SIM profile event handlers.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t SimProfileHandlerCount = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Forward the last ICCID change to every new subscribed client to the ICCID change event.
 */
//--------------------------------------------------------------------------------------------------
static bool ForwardLastIccidChange = true;

#if LE_CONFIG_ENABLE_CONFIG_TREE
//--------------------------------------------------------------------------------------------------
/**
 * Last ICCID change event.
 */
//--------------------------------------------------------------------------------------------------
static Sim_IccidChangeEvent_t LastIccidChange =
{
    .simId = LE_SIM_ID_MAX,
    .ICCID = {0}
};
#endif

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
LE_REF_DEFINE_STATIC_MAP(FPLMNListRefMap, MAX_NUM_FPLMN_LISTS);

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for FPLMN list.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t FPLMNListRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for FPLMN list.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(FPLMNList,
                          MAX_NUM_FPLMN_LISTS,
                          sizeof(le_sim_FPLMNList_t));

//--------------------------------------------------------------------------------------------------
/**
 * Pool for FPLMN list.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t FPLMNListPool;

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for FPLMN network Operator.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(FPLMNOperator,
                          HIGH_FPLMN_OPERATOR_COUNT,
                          sizeof(pa_sim_FPLMNOperator_t));

//--------------------------------------------------------------------------------------------------
/**
 * Pool for FPLMN network Operator.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t FPLMNOperatorPool;

//--------------------------------------------------------------------------------------------------
/**
 * Check if the APDU response notifies a correct execution of the APDU command with a normal ending.
 *
 * @return
 *      - TRUE if the APDU command ended normally
 *      - FALSE if an error occured
 */
//--------------------------------------------------------------------------------------------------
static bool IsCommandCorrectlyExecuted
(
    uint8_t* respPtr,    ///< [IN] APDU response pointer
    int respLen          ///< [IN] APDU response length
)
{
    if (2 != respLen)
    {
        return false;
    }

    // Values defined in 3GPP TS 11.11 V8.14.0 paragraph 9.4.1
    if (((0x90 == respPtr[0]) && (0x00 == respPtr[1])) || (0x91 == respPtr[0])
        || (0x9E == respPtr[0]) || (0x9F == respPtr[0]))
    {
        return true;
    }

    return false;
}

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
    le_sim_Manufacturer_t manufacturer,   ///< [IN] The card manufacturer
    uint8_t*              swapApduReqPtr, ///< [IN] The swap APDU message
    uint32_t              swapApduLen     ///< [IN] The swap APDU message length in bytes
)
{
    le_result_t status = LE_OK;
    uint8_t channel = 0;
    uint8_t resp[LE_SIM_RESPONSE_MAX_BYTES] = {0};
    size_t  lenResp = LE_SIM_RESPONSE_MAX_BYTES;
    bool useLogicalChannel;

    // On Oberthur and Gemalto SIM cards, it's not possible to change the SIM profile when using
    // a logical channel.
    if ((LE_SIM_OBERTHUR == manufacturer) || (LE_SIM_GEMALTO == manufacturer))
    {
        useLogicalChannel = false;
    }
    else
    {
        useLogicalChannel = true;
        status = pa_sim_OpenLogicalChannel(&channel);
        if (LE_OK != status)
        {
            LE_ERROR("Cannot open logical channel!");
            return status;
        }
    }

    // On G_AND_D SIM cards, an ERA Glonass applet is present. This applet should be selected before
    // performing the SIM profile swap.
    if (LE_SIM_G_AND_D == manufacturer)
    {
        uint8_t apduLen   = 21;
        uint8_t apduReq[] = {0x00, 0xA4, 0x04, 0x00, 0x10, 0xD2, 0x76, 0x00,
                             0x01, 0x18, 0x00, 0x02, 0xFF, 0x34, 0x10, 0x25,
                             0x89, 0xC0, 0x02, 0x10, 0x01};

        // Channel number must be specified in the CLI field of the APDU requests for G_AND_D SIMs.
        apduReq[0] = channel;
        swapApduReqPtr[0] = channel;

        status = pa_sim_SendApdu(channel, apduReq, apduLen, resp, &lenResp);
        if (LE_OK != status)
        {
            LE_ERROR("Cannot send APDU message!");
            goto end;
        }

        if (!IsCommandCorrectlyExecuted(resp, lenResp))
        {
            LE_ERROR("APDU response: %02X, %02X", resp[0], resp[1]);
            status = LE_FAULT;
            goto end;
        }
    }

    // Send the APDU request to swap SIM profile and check response.
    status = pa_sim_SendApdu(channel, swapApduReqPtr, swapApduLen, resp, &lenResp);
    if (LE_OK != status)
    {
        LE_ERROR("Cannot swap subscription!");
        goto end;
    }

    if (!IsCommandCorrectlyExecuted(resp, lenResp))
    {
        LE_ERROR("APDU response: %02X, %02X", resp[0], resp[1]);
        status = LE_FAULT;
    }

end:
    // Clean ressources if needed.
    if (useLogicalChannel)
    {
        status = pa_sim_CloseLogicalChannel(channel);
    }

    // Gemalto, G&D and Valid SIMs don't trigger a refresh automatically when swapping.
    // So, request it.
    if (LE_OK == status)
    {
        if ((LE_SIM_GEMALTO == manufacturer) || (LE_SIM_VALID == manufacturer)
            || (LE_SIM_G_AND_D == manufacturer))
        {
            status = pa_sim_Refresh();
        }
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * SIM card selector.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if unable to select the SIM card
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SelectSIMCard
(
    le_sim_Id_t simId    ///< [IN] SIM identifier
)
{
    if (simId >= LE_SIM_ID_MAX)
    {
        return LE_FAULT;
    }

    // If SIM is unspecified, then use the current SIM card
    if (simId == LE_SIM_UNSPECIFIED)
    {
        return LE_OK;
    }

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
 * First layer: Profile update notification handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerProfileUpdateHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    pa_sim_StkEvent_t* eventDataPtr = reportPtr;

    if (!eventDataPtr)
    {
       LE_ERROR("Null pointer provided!");
       return;
    }

    le_sim_ProfileUpdateHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(eventDataPtr->simId, eventDataPtr->stkEvent, le_event_GetContextPtr());
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
    Sim_Event_t* simEventPtr = reportPtr;

    if (!simEventPtr)
    {
       LE_ERROR("Null pointer provided!");
       return;
    }

    le_sim_NewStateHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(simEventPtr->simId, simEventPtr->state, le_event_GetContextPtr());
}

#if LE_CONFIG_ENABLE_CONFIG_TREE
//--------------------------------------------------------------------------------------------------
/**
 * First- layer: ICCID change notification handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerIccidChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    Sim_IccidChangeEvent_t* eventDataPtr = reportPtr;

    if (!eventDataPtr)
    {
       LE_ERROR("Null pointer provided!");
       return;
    }

    le_sim_IccidChangeHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(eventDataPtr->simId, eventDataPtr->ICCID, le_event_GetContextPtr());
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM card EID and store it in the SIM structure.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetEID
(
    Sim_t* simPtr   ///< [IN,OUT] The SIM structure
)
{
    pa_sim_Eid_t eid;

    // EID has already been retreived and stored in SIM structure. No need to read it again.
    if (0 != simPtr->EID[0])
    {
        return LE_OK;
    }

    // Select SIM card and attempt to read EID from PA.
    if (LE_OK != SelectSIMCard(simPtr->simId))
    {
        return LE_FAULT;
    }

    if (false == simPtr->isReacheable)
    {
        LE_ERROR("SIM (%u) is unreacheable", simPtr->simId);
        return LE_FAULT;
    }

    if (LE_OK != pa_sim_GetCardEID(eid))
    {
      return LE_FAULT;
    }

    return le_utf8_Copy(simPtr->EID, eid, sizeof(simPtr->EID), NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM card ICCID and store it in the SIM structure.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetICCID
(
    Sim_t* simPtr   ///< [IN,OUT] The SIM structure
)
{
    pa_sim_CardId_t  iccid;

    // ICCID has already been retreived and stored in SIM structure. No need to read it again.
    if (0 != simPtr->ICCID[0])
    {
        return LE_OK;
    }

    // Select SIM card and attempt to read ICCID from PA.
    if (LE_OK != SelectSIMCard(simPtr->simId))
    {
        return LE_FAULT;
    }

    if (false == simPtr->isReacheable)
    {
        LE_ERROR("SIM (%u) is unreacheable", simPtr->simId);
        return LE_FAULT;
    }

    if (LE_OK != pa_sim_GetCardIdentification(iccid))
    {
      return LE_FAULT;
    }

    return le_utf8_Copy(simPtr->ICCID, iccid, sizeof(simPtr->ICCID), NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM card IMSI and store it in the SIM structure.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetIMSI
(
    Sim_t* simPtr   ///< [IN,OUT] The SIM structure
)
{
    pa_sim_Imsi_t    imsi;

    // IMSI has already been retreived and stored in SIM structure. No need to read it again.
    if (0 != simPtr->IMSI[0])
    {
        return LE_OK;
    }

    // Select SIM card and attempt to read IMSI from PA.
    if (LE_OK != SelectSIMCard(simPtr->simId))
    {
        return LE_FAULT;
    }

    if (false == simPtr->isReacheable)
    {
        LE_ERROR("SIM (%u) is unreacheable", simPtr->simId);
        return LE_FAULT;
    }

    if (LE_OK != pa_sim_GetIMSI(imsi))
    {
      return LE_FAULT;
    }

    return le_utf8_Copy(simPtr->IMSI, imsi, sizeof(simPtr->IMSI), NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM Phone Number and store it in the SIM structure.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetPhoneNumber
(
    Sim_t* simPtr   ///< [IN,OUT] The SIM structure
)
{
    char phoneNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = {0};

    // Phone number has already been retreived and stored. No need to read it again from PA.
    if (0 != simPtr->phoneNumber[0])
    {
        return LE_OK;
    }

    // Select SIM card and attempt to read IMSI from PA.
    if (LE_OK != SelectSIMCard(simPtr->simId))
    {
        return LE_FAULT;
    }

    if (false == simPtr->isReacheable)
    {
        LE_ERROR("SIM (%u) is unreacheable", simPtr->simId);
        return LE_FAULT;
    }

    if (LE_OK != pa_sim_GetSubscriberPhoneNumber(phoneNumber, sizeof(phoneNumber)))
    {
      return LE_FAULT;
    }

    return le_utf8_Copy(simPtr->phoneNumber, phoneNumber, sizeof(simPtr->phoneNumber), NULL);
}

#if LE_CONFIG_ENABLE_CONFIG_TREE
//--------------------------------------------------------------------------------------------------
/**
 * Compare the current ICCID value with the ICCID stored in config tree
 *
 * @return
 *       - TRUE if the current ICCID is different from the stored ICCID.
 *       - FALSE otherwise
 */
//--------------------------------------------------------------------------------------------------
static bool IsNewICCID
(
    Sim_t* simPtr   ///< [IN,OUT] The SIM structure
)
{
    le_cfg_IteratorRef_t iteratorRef;
    char storedICCID[LE_SIM_ICCID_BYTES] = {0};

    if (NULL == simPtr)
    {
        LE_ERROR("Null pointer provided");
        return false;
    }

    // Get ICCID from config tree
    iteratorRef = le_cfg_CreateReadTxn(CFG_MODEMSERVICE_SIM_PATH);
    le_cfg_GetString(iteratorRef, CFG_NODE_ICCID, storedICCID, LE_SIM_ICCID_BYTES, "");
    le_cfg_CancelTxn(iteratorRef);

    if (!memcmp(simPtr->ICCID, storedICCID, LE_SIM_ICCID_BYTES))
    {
        return false;
    }
    else
    {
        return true;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Save the current ICCID in config tree
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SaveICCID
(
    Sim_t* simPtr   ///< [IN,OUT] The SIM structure
)
{
    le_cfg_IteratorRef_t iteratorRef;

    if (NULL == simPtr)
    {
        LE_ERROR("Null pointer provided");
        return LE_FAULT;
    }

    iteratorRef = le_cfg_CreateWriteTxn(CFG_MODEMSERVICE_SIM_PATH);
    le_cfg_SetString(iteratorRef, CFG_NODE_ICCID, simPtr->ICCID);
    le_cfg_CommitTxn(iteratorRef);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Report an event in case the current ICCID value is different than the value stored in config tree
 *
 */
//--------------------------------------------------------------------------------------------------
static void MonitorICCIDChange
(
    Sim_t* simPtr   ///< [IN,OUT] The SIM structure
)
{
    if (NULL == simPtr)
    {
        LE_ERROR("Null pointer provided");
        return;
    }

    if (!simPtr->isReacheable)
    {
        return;
    }

    if (IsNewICCID(simPtr))
    {
        // Report the ICCID change event
        LastIccidChange.simId = simPtr->simId;
        memcpy(LastIccidChange.ICCID, simPtr->ICCID, LE_SIM_ICCID_BYTES);
        le_event_Report(IccidChangeEventId, &LastIccidChange, sizeof(Sim_IccidChangeEvent_t));

        // Save the current ICCID in config tree
        SaveICCID(simPtr);
    }
}
#else
//--------------------------------------------------------------------------------------------------
/**
 * Report an event in case the current ICCID value is different than the value stored in config tree
 *
 */
//--------------------------------------------------------------------------------------------------
static void MonitorICCIDChange
(
    Sim_t* simPtr   ///< [IN,OUT] The SIM structure
)
{
    // Cannot monitor ICCID change with config tree disabled, so do nothing.
    return;
}
#endif

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
            simPtr->isReacheable = false;
            simPtr->ICCID[0] = '\0';
            simPtr->IMSI[0] = '\0';
            simPtr->EID[0] = '\0';
            simPtr->phoneNumber[0] = '\0';
            simPtr->isPresent = false;
            break;

        case LE_SIM_INSERTED:
        case LE_SIM_BLOCKED:
            simPtr->isReacheable = true;
            simPtr->isPresent = true;
            simPtr->IMSI[0] = '\0';
            simPtr->EID[0] = '\0';
            simPtr->ICCID[0] = '\0';
            simPtr->phoneNumber[0] = '\0';
            GetICCID(simPtr);
            GetIMSI(simPtr);
            GetEID(simPtr);
            GetPhoneNumber(simPtr);
            break;

        case LE_SIM_READY:
            simPtr->isReacheable = true;
            simPtr->isPresent = true;
            GetICCID(simPtr);
            GetIMSI(simPtr);
            GetEID(simPtr);
            GetPhoneNumber(simPtr);
            break;

        case LE_SIM_BUSY:
        case LE_SIM_POWER_DOWN:
        case LE_SIM_STATE_UNKNOWN:
            simPtr->isReacheable = false;
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

    // Since the SIM state and ID are not able to be correctly synchronized between PA level and
    // service level, here we will always check current SIM ID to set the correct value of variable
    // "SelectedCard" which is used by SIM service code in each event handler call.
    if ( LE_OK != pa_sim_GetSelectedCard(&SelectedCard))
    {
        LE_WARN("NewSimStateHandler: Failed to get current SIM ID.");
        return;
    }
    else
    {
        LE_INFO("NewSimStateHandler: Current Selected SIM ID=%d", SelectedCard);
    }

    //Update the SIM information for current SIM
    if ( eventPtr->simId == SelectedCard )
    {
        simPtr = &SimList[eventPtr->simId];
        GetSimCardInformation(simPtr, eventPtr->state);

        // Check if ICCID value changed
        MonitorICCIDChange(&SimList[eventPtr->simId]);
    }

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

    LE_DEBUG("Report STK event %d on SIM %d", stkEventPtr->stkEvent, stkEventPtr->simId);
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
    if (LE_SIM_REFRESH == eventPtr->stkEvent)
    {
        switch (eventPtr->stkRefreshStage)
        {
            case LE_SIM_STAGE_WAITING_FOR_OK:
                if ((SimProfileHandlerCount) && (LE_SIM_REFRESH_RESET == eventPtr->stkRefreshMode))
                {
                    // Do not accept or reject the toolkit command because a SIM profile update
                    // handler is subscribed.
                    le_event_Report(SimProfileEventId, eventPtr, sizeof(pa_sim_StkEvent_t));
                }
                else
                {
                    if (0 == SimToolkitHandlerCount)
                    {
                        // Accept the refresh automatically if no other handler has been subscribed
                        le_sim_AcceptSimToolkitCommand(eventPtr->simId);
                    }
                }
                break;

            case LE_SIM_STAGE_END_WITH_SUCCESS:
                LE_INFO("Update SIM Card information after a refresh");

                // Discard old SIM card information and perform a new read.
                GetSimCardInformation(&SimList[eventPtr->simId], LE_SIM_ABSENT);
                GetSimCardInformation(&SimList[eventPtr->simId], LE_SIM_READY);

                // Check if ICCID value changed
                MonitorICCIDChange(&SimList[eventPtr->simId]);
                break;

            default:
                break;
        }
    }
    else if (LE_SIM_OPEN_CHANNEL == eventPtr->stkEvent)
    {
        if (0 == SimProfileHandlerCount)
        {
            // Accept the refresh automatically if no other handler has been subscribed
            le_sim_AcceptSimToolkitCommand(eventPtr->simId);
        }
        else
        {
            le_event_Report(SimProfileEventId, eventPtr, sizeof(pa_sim_StkEvent_t));
        }
    }
    else
    {
        LE_ERROR("Unsupported event");
        return;
    }

    if (SimToolkitHandlerCount)
    {
        LE_DEBUG("Report STK event %d on SIM %d", eventPtr->stkEvent, eventPtr->simId);
        le_event_Report(SimToolkitEventId, eventPtr, sizeof(pa_sim_StkEvent_t));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function returns the specified SIM context.
 * @note If LE_SIM_UNSPECIFIED is requested, then return the context of the current SIM card.
 *
 * @return SIM context structure.
 */
//--------------------------------------------------------------------------------------------------
static Sim_t* GetSimContext
(
    le_sim_Id_t simId   ///< [IN] The SIM identifier
)
{
    simId = (LE_SIM_UNSPECIFIED == simId) ? SelectedCard : simId;

    return &SimList[simId];
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
    le_sim_Id_t simId   ///< [IN] The SIM identifier
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

    simPtr = GetSimContext(simId);

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
    le_dls_List_t* FPLMNListPtr    ///< [IN/OUT] The FPLMN operators list
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
    le_sim_FPLMNList_t* FPLMNListPtr,         ///< [IN] FPLMN list pointer
    le_dls_Link_t* FPLMNLinkPtr,              ///< [IN] FPLMN operator link
    char* mccPtr,                             ///< [OUT] Mobile Country Code
    size_t mccPtrSize,                        ///< [IN] Size of Mobile Country Code
    char* mncPtr,                             ///< [OUT] Mobile Network Code
    size_t mncPtrSize                         ///< [IN] Size of Mobile Network Code
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
    le_dls_List_t *FPLMNOperatorsListPtr ///< [IN] List of FPLMN operators
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
 * Timer handler: On expiry, this function stops the forwarding of the last ICCID change to new
 * registered clients
 *
 */
//--------------------------------------------------------------------------------------------------
void EventTimerHandler
(
    le_timer_Ref_t timerRef    ///< [IN] This timer has expired
)
{
    LE_INFO("Disabling last ICCID change forwarding");
    ForwardLastIccidChange = false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function to the close session service.
 *
 */
//--------------------------------------------------------------------------------------------------
static void CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] Session reference of client application
    void*               contextPtr   ///< [IN] Context pointer of CloseSessionEventHandler
)
{
    if (!sessionRef)
    {
        LE_ERROR("ERROR sessionRef is NULL");
        return;
    }

    // Clean session context.
    LE_DEBUG("SessionRef (%p) has been closed", sessionRef);

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
    le_sim_States_t  state;

    // Initialize the SIM list
    for (i = 0; i < LE_SIM_ID_MAX; i++)
    {
        SimList[i].simId = i;
        SimList[i].ICCID[0] = '\0';
        SimList[i].IMSI[0] = '\0';
        SimList[i].EID[0] = '\0';
        SimList[i].phoneNumber[0] = '\0';
        SimList[i].isPresent = false;
        SimList[i].isReacheable = false;
        SimList[i].subscription = UNKNOWN_SUBSCRIPTION;
    }

    // Create FPLMN list pool
    FPLMNListPool = le_mem_InitStaticPool(FPLMNList,
                                          MAX_NUM_FPLMN_LISTS,
                                          sizeof(le_sim_FPLMNList_t));

    // Create the Safe Reference Map to use for FPLMN Operator list
    FPLMNListRefMap = le_ref_InitStaticMap(FPLMNListRefMap, MAX_NUM_FPLMN_LISTS);

    // Create the FPLMN operator memory pool
    FPLMNOperatorPool = le_mem_InitStaticPool(FPLMNOperator,
                                              HIGH_FPLMN_OPERATOR_COUNT,
                                              sizeof(pa_sim_FPLMNOperator_t));

    // Add a handler to the close session service.
    le_msg_AddServiceCloseHandler(le_sim_GetServiceRef(), CloseSessionEventHandler, NULL);

    // Create an event Id for new SIM state notifications
    NewSimStateEventId = le_event_CreateId("NewSimStateEventId", sizeof(Sim_Event_t));

#if LE_CONFIG_ENABLE_CONFIG_TREE
    // Create an event Id for ICCID change notifications
    IccidChangeEventId = le_event_CreateId("IccidChangeEventId", sizeof(Sim_IccidChangeEvent_t));
#endif

    // Create an event Id for SIM Toolkit notifications
    SimToolkitEventId = le_event_CreateId("SimToolkitEventId", sizeof(pa_sim_StkEvent_t));

    // Create an event Id for SIM profile notifications
    SimProfileEventId = le_event_CreateId("SimProfileEventId", sizeof(pa_sim_StkEvent_t));

    // Initialize handler counter for SIM Toolkit event
    SimToolkitHandlerCount = 0;

    // Initialize handler counter for SIM profile event
    SimProfileHandlerCount = 0;

    // Register a handler function for new SIM state notification
    if (NULL == pa_sim_AddNewStateHandler(NewSimStateHandler))
    {
        LE_CRIT("Add new SIM state handler failed");
        return LE_FAULT;
    }

    // Register a handler function for SIM Toolkit notification
    if (NULL == pa_sim_AddSimToolkitEventHandler(SimToolkitHandler, NULL))
    {
        LE_ERROR("Add PA SIM Toolkit handler failed");
        return LE_FAULT;
    }

    if (LE_OK != pa_sim_GetSelectedCard(&SelectedCard))
    {
        LE_CRIT("Unable to get selected card.");
        return LE_FAULT;
    }

    LE_DEBUG("SIM %u is selected.", SelectedCard);

    if (LE_OK != pa_sim_GetState(&state))
    {
        LE_CRIT("Unable to get the initial card state");
        return LE_FAULT;
    }

    GetSimCardInformation(&SimList[SelectedCard], state);

    MonitorICCIDChange(&SimList[SelectedCard]);

    // Create a one-shot timer to delimit the initialization phase of the daemon.
    EventTimerRef = le_timer_Create("EventTimer");
    le_timer_SetMsInterval(EventTimerRef, 5000);
    le_timer_SetRepeat(EventTimerRef, 1);
    le_timer_SetHandler(EventTimerRef, EventTimerHandler);
    le_timer_Start(EventTimerRef);

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
    le_sim_Id_t simId,        ///< [IN] The SIM identifier
    char *      iccidPtr,     ///< [OUT] Buffer to hold the ICCID
    size_t      iccidLen      ///< [IN] Buffer length
)
{
    Sim_t*           simPtr = NULL;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }

    if (NULL == iccidPtr)
    {
        LE_KILL_CLIENT("iccidPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if (iccidLen < LE_SIM_ICCID_BYTES)
    {
        LE_ERROR("Incorrect buffer size");
        return LE_OVERFLOW;
    }

    simPtr = GetSimContext(simId);

    if (LE_OK != GetICCID(simPtr))
    {
        LE_ERROR("Failed to get the ICCID of sim identifier.%d", simPtr->simId);
        return LE_FAULT;
    }

    return le_utf8_Copy(iccidPtr, simPtr->ICCID, iccidLen, NULL);
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
 *
 * @warning le_sim_GetEID() function is platform dependent. Please refer to the
 *          @ref platformConstraintsSim_EID section for full details.
 */
//--------------------------------------------------------------------------------------------------
le_result_t  le_sim_GetEID
(
    le_sim_Id_t simId,      ///< [IN] The SIM identifier
    char*       eidPtr,     ///< [OUT] Buffer to hold the EID
    size_t      eidLen      ///< [IN] Buffer length
)
{
    Sim_t*           simPtr = NULL;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }

    if (NULL == eidPtr)
    {
        LE_KILL_CLIENT("eidPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if (eidLen < LE_SIM_EID_BYTES)
    {
        LE_ERROR("Incorrect buffer size");
        return LE_OVERFLOW;
    }

    simPtr = GetSimContext(simId);

    if (LE_OK != GetEID(simPtr))
    {
        LE_ERROR("Failed to get the EID of sim identifier.%d", simPtr->simId);
        return LE_FAULT;
    }

    return le_utf8_Copy(eidPtr, simPtr->EID, eidLen, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function retrieves the identification number (IMSI) of the SIM card. (15 digits)
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
    le_sim_Id_t simId,       ///< [IN] The SIM identifier
    char *      imsiPtr,     ///< [OUT] Buffer to hold the IMSI
    size_t      imsiLen      ///< [IN] Buffer length
)
{
    Sim_t*           simPtr = NULL;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }

    if (NULL == imsiPtr)
    {
        LE_KILL_CLIENT("imsiPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if (imsiLen < LE_SIM_IMSI_BYTES)
    {
        LE_ERROR("Incorrect buffer size");
        return LE_OVERFLOW;
    }

    simPtr = GetSimContext(simId);

    if (LE_OK != GetIMSI(simPtr))
    {
        LE_ERROR("Failed to get the IMSI of sim identifier.%d", simPtr->simId);
        return LE_FAULT;
    }

    return le_utf8_Copy(imsiPtr, simPtr->IMSI, imsiLen, NULL);
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
    le_sim_Id_t     simId,             ///< [IN] SIM identifier
    char*           phoneNumberStr,    ///< [OUT] Phone Number
    size_t          phoneNumberStrSize ///< [IN]  Size of phoneNumberStr
)
{
    Sim_t*           simPtr = NULL;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }

    if (NULL == phoneNumberStr)
    {
        LE_KILL_CLIENT("phoneNumberStr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if (phoneNumberStrSize < LE_MDMDEFS_PHONE_NUM_MAX_BYTES)
    {
        LE_ERROR("Incorrect buffer size");
        return LE_OVERFLOW;
    }

    simPtr = GetSimContext(simId);

    if (LE_OK != GetPhoneNumber(simPtr))
    {
        LE_ERROR("Failed to get the Phone Number of sim identifier.%d", simPtr->simId);
        return LE_FAULT;
    }

    return le_utf8_Copy(phoneNumberStr,simPtr->phoneNumber,phoneNumberStrSize,NULL);
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
    le_sim_Id_t simId   ///< [IN] The SIM identifier
)
{
    le_sim_States_t  state;
    Sim_t*           simPtr;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return false;
    }

    simPtr = GetSimContext(simId);

    if (SelectSIMCard(simId) != LE_OK)
    {
        return false;
    }

    if (pa_sim_GetState(&state) == LE_OK)
    {
        if ((state != LE_SIM_ABSENT) &&
           (state != LE_SIM_STATE_UNKNOWN) &&
           (state != LE_SIM_POWER_DOWN))
        {
            simPtr->isPresent = true;
            return true;
        }
        else
        {
            simPtr->isReacheable = false;
            simPtr->isPresent = false;
            return false;
        }
    }
    else
    {
        simPtr->isReacheable = false;
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
    le_sim_Id_t simId   ///< [IN] SIM identifier
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
    le_sim_Id_t  simId,     ///< [IN] SIM identifier
    const char*  pinPtr     ///< [IN] PIN code
)
{
    pa_sim_Pin_t pinloc;
    Sim_t*       simPtr;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }

    simPtr = GetSimContext(simId);

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
        LE_ERROR("Failed to enter PIN.%s sim identifier.%d", pinPtr, simPtr->simId);
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
    le_sim_Id_t   simId,     ///< [IN] SIM identifier
    const char*   oldpinPtr, ///< [IN] Old PIN code
    const char*   newpinPtr  ///< [IN] New PIN code
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

    simPtr = GetSimContext(simId);

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
        LE_ERROR("Failed to set new PIN.%s of sim identifier.%d", newpinPtr, simPtr->simId);
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
    le_sim_Id_t simId   ///< [IN] The SIM identifier
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
    le_sim_Id_t simId,                 ///< [IN] The SIM identifier
    uint32_t*   remainingPukTriesPtr   ///< [OUT] The number of remaining PUK insertion tries
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
    le_sim_Id_t     simId,   ///< [IN] SIM identifier
    const char*     pinPtr   ///< [IN] PIN code
)
{
    pa_sim_Pin_t  pinloc;
    Sim_t*        simPtr;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }

    simPtr = GetSimContext(simId);

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
        LE_ERROR("Failed to unlock sim identifier.%d", simPtr->simId);
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
    le_sim_Id_t     simId,   ///< [IN] SIM identifier
    const char*     pinPtr   ///< [IN] PIN code
)
{
    pa_sim_Pin_t pinloc;
    Sim_t*       simPtr;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }

    simPtr = GetSimContext(simId);

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
        LE_ERROR("Failed to Lock sim identifier.%d", simPtr->simId);
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
    le_sim_Id_t     simId,     ///< [IN] SIM identifier
    const char*     pukPtr,    ///< [IN] PUK code
    const char*     newpinPtr  ///< [IN] New PIN code
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

    simPtr = GetSimContext(simId);

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
        LE_ERROR("Failed to unblock sim identifier.%d", simPtr->simId);
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
    le_sim_Id_t simId   ///< [IN] SIM identifier
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
        ///< [IN] Handler function for New State notification

    void* contextPtr
        ///< [IN] Handler's context
)
{
    le_event_HandlerRef_t handlerRef;

    if (NULL == handlerPtr)
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
    le_sim_NewStateHandlerRef_t   handlerRef ///< [IN] Handler reference
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Home Network Name information.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Home Network Name can't fit in nameStr
 *      - LE_BAD_PARAMETER if a parameter is invalid
 *      - LE_FAULT on any other failure
 *
 * @note  The home network name can be given even if the device is not registered on the network.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetHomeNetworkOperator
(
    le_sim_Id_t        simId,          ///< [IN] SIM identifier
    char              *nameStr,        ///< [OUT] Home network Name
    size_t             nameStrSize     ///< [IN] nameStr size
)
{
    if (NULL == nameStr)
    {
        LE_KILL_CLIENT("nameStr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if (LE_OK != SelectSIMCard(simId))
    {
        return LE_FAULT;
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
    le_sim_Id_t       simId,          ///< [IN] SIM identifier
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

    if (NULL == mccPtr)
    {
        LE_KILL_CLIENT("mccPtr is NULL");
        return LE_FAULT;
    }

    if (NULL == mncPtr)
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
 *      - LE_DUPLICATE for duplicate operation
 *
 * @note Please ensure that the eUICC is selected using le_sim_SelectCard() and in a ready state
 *       before attempting a profile swap.
 *
 * @warning If you use a Morpho or Oberthur card, the SIM_REFRESH PRO-ACTIVE command must be
 *          accepted with le_sim_AcceptSimToolkitCommand() in order to complete the profile swap
 *          procedure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_LocalSwapToEmergencyCallSubscription
(
    le_sim_Id_t           simId,          ///< [IN] The SIM identifier
    le_sim_Manufacturer_t manufacturer    ///< [IN] The card manufacturer
)
{
    Sim_t* simPtr;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }

    if (simId != SelectedCard)
    {
        LE_ERROR("Mismatch between provided simId (%d) and current SIM (%d)", simId, SelectedCard);
        return LE_BAD_PARAMETER;
    }

    simPtr = GetSimContext(simId);

    if (ECS == simPtr->subscription)
    {
        LE_ERROR("Duplicate operation on swapping to ESC!");
        return LE_DUPLICATE;
    }

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
 *      - LE_DUPLICATE for duplicate operation
 *
 * @note Please ensure that the eUICC is selected using le_sim_SelectCard() and in a ready state
 *       before attempting a profile swap.
 *
 * @warning If you use a Morpho or Oberthur card, the SIM_REFRESH PRO-ACTIVE command must be
 *          accepted with le_sim_AcceptSimToolkitCommand() in order to complete the profile swap
 *          procedure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_LocalSwapToCommercialSubscription
(
    le_sim_Id_t           simId,          ///< [IN] SIM identifier
    le_sim_Manufacturer_t manufacturer    ///< [IN] The card manufacturer
)
{
    Sim_t* simPtr;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }

    if (simId != SelectedCard)
    {
        LE_ERROR("Mismatch between provided simId (%d) and current SIM (%d)", simId, SelectedCard);
        return LE_BAD_PARAMETER;
    }

    simPtr = GetSimContext(simId);

    if (COMMERCIAL == simPtr->subscription)
    {
        LE_ERROR("Duplicate operation on swapping back to Commercial subscription!");
        return LE_DUPLICATE;
    }

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
    le_sim_Id_t simId,   ///< [IN] SIM identifier
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

    simPtr = GetSimContext(simId);

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
 * This function must be called to register a handler function for ICCID change notification.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure; there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_sim_IccidChangeHandlerRef_t le_sim_AddIccidChangeHandler
(
    le_sim_IccidChangeHandlerFunc_t handlerPtr,     ///< [IN] Handler function
    void* contextPtr                                ///< [IN] Handler's context
)
{
#if LE_CONFIG_ENABLE_CONFIG_TREE
    le_event_HandlerRef_t handlerRef;

    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("IccidChangeHandler",
                                            IccidChangeEventId,
                                            FirstLayerIccidChangeHandler,
                                            (le_event_HandlerFunc_t)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    // During initialization of the daemon, each new subscription to the ICCID change event is
    // notified by the last monitored event.
    if ((ForwardLastIccidChange) && (LE_SIM_ID_MAX != LastIccidChange.simId))
    {
        handlerPtr(LastIccidChange.simId, LastIccidChange.ICCID, contextPtr);
    }

    return (le_sim_IccidChangeHandlerRef_t)handlerRef;
#else
    LE_ERROR("Monitoring for ICCID change is not supported if config tree is disabled");
    return NULL;
#endif
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler function for ICCID change notification.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_sim_RemoveIccidChangeHandler
(
    le_sim_IccidChangeHandlerRef_t handlerRef   ///< [IN] Handler reference
)
{
    if (NULL == handlerRef)
    {
        LE_ERROR("Null handler reference");
        return;
    }

    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
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
    le_sim_SimToolkitEventHandlerFunc_t handlerPtr,     ///< [IN] Handler function for
                                                        ///<      New State notification
    void* contextPtr                                    ///< [IN] Handler's context
)
{
    le_event_HandlerRef_t handlerRef;

    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
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
    le_sim_SimToolkitEventHandlerRef_t handlerRef   ///< [IN] Handler reference
)
{
    if (NULL == handlerRef)
    {
        LE_ERROR("Null handler reference");
        return;
    }

    if (SimToolkitHandlerCount)
    {
        SimToolkitHandlerCount--;
        le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler function for SIM profile update notification.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure; there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_sim_ProfileUpdateHandlerRef_t le_sim_AddProfileUpdateHandler
(
    le_sim_ProfileUpdateHandlerFunc_t handlerPtr,     ///< [IN] Handler function
    void* contextPtr                                  ///< [IN] Handler's context
)
{
    le_event_HandlerRef_t handlerRef;

    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("ProfileUpdateHandler",
                                            SimProfileEventId,
                                            FirstLayerProfileUpdateHandler,
                                            (le_event_HandlerFunc_t)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);
    SimProfileHandlerCount++;

    return (le_sim_ProfileUpdateHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler function for SIM profile update
 * notification.
 */
//--------------------------------------------------------------------------------------------------
void le_sim_RemoveProfileUpdateHandler
(
    le_sim_ProfileUpdateHandlerRef_t handlerRef   ///< [IN] Handler reference
)
{
    if (NULL == handlerRef)
    {
        LE_ERROR("Null handler reference");
        return;
    }

    if (SimProfileHandlerCount)
    {
        SimProfileHandlerCount--;
        le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Accept the last SIM Toolkit command.
 *
 * @return
 *  - LE_OK       The function succeeded.
 *  - LE_FAULT    The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_AcceptSimToolkitCommand
(
    le_sim_Id_t simId   ///< [IN] SIM identifier
)
{
    if (LE_OK != SelectSIMCard(simId))
    {
        return LE_FAULT;
    }

    return pa_sim_ConfirmSimToolkitCommand(true);
}

//--------------------------------------------------------------------------------------------------
/**
 * Reject the last SIM Toolkit command.
 *
 * @return
 *  - LE_OK       The function succeeded.
 *  - LE_FAULT    The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_RejectSimToolkitCommand
(
    le_sim_Id_t simId   ///< [IN] SIM identifier
)
{
    if (LE_OK != SelectSIMCard(simId))
    {
        return LE_FAULT;
    }

    return pa_sim_ConfirmSimToolkitCommand(false);
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the mode of the last SIM Toolkit Refresh command.
 * The modes are defined in ETSI TS 102 223 sections 6.4.7 and 8.6.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNAVAILABLE    The last SIM Toolkit command is not a Refresh command.
 *  - LE_FAULT          The function failed.
 *  - LE_UNSUPPORTED    The platform does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetSimToolkitRefreshMode
(
    le_sim_Id_t              simId,         ///< The SIM identifier
    le_sim_StkRefreshMode_t* refreshModePtr ///< The Refresh mode
)
{
    pa_sim_StkEvent_t stkStatus;

    if (!refreshModePtr)
    {
        LE_ERROR("refreshModePtr is NULL!");
        return LE_BAD_PARAMETER;
    }

    if (LE_OK != SelectSIMCard(simId))
    {
        return LE_FAULT;
    }

    le_result_t result = pa_sim_GetLastStkStatus(&stkStatus);
    if (LE_OK != result)
    {
        return result;
    }

    if (LE_SIM_REFRESH != stkStatus.stkEvent)
    {
        LE_INFO("Wrong event. Expected LE_SIM_REFRESH");
        return LE_FAULT;
    }

    *refreshModePtr = stkStatus.stkRefreshMode;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the stage of the last SIM Toolkit Refresh command.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNAVAILABLE    The last SIM Toolkit command is not a Refresh command.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetSimToolkitRefreshStage
(
    le_sim_Id_t               simId,            ///< The SIM identifier
    le_sim_StkRefreshStage_t* refreshStagePtr   ///< The Refresh stage
)
{
    pa_sim_StkEvent_t stkStatus;

    if (!refreshStagePtr)
    {
        LE_ERROR("refreshStagePtr is NULL!");
        return LE_BAD_PARAMETER;
    }

    if (LE_OK != SelectSIMCard(simId))
    {
        return LE_FAULT;
    }

    if (LE_OK != pa_sim_GetLastStkStatus(&stkStatus))
    {
        return LE_FAULT;
    }

    if (LE_SIM_REFRESH != stkStatus.stkEvent)
    {
        LE_INFO("Wrong event. Expected LE_SIM_REFRESH");
        return LE_FAULT;
    }

    *refreshStagePtr = stkStatus.stkRefreshStage;
    return LE_OK;
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
    le_sim_Id_t simId,                  ///< [IN] The SIM identifier
    const uint8_t* commandApduPtr,      ///< [IN] APDU command
    size_t commandApduNumElements,      ///< [IN] APDU command size
    uint8_t* responseApduPtr,           ///< [OUT] SIM response
    size_t* responseApduNumElementsPtr  ///< [INOUT] SIM response size
)
{
    // Send APDU on basic logical channel 0, which is always opened
    return le_sim_SendApduOnChannel(simId,
                                    0,
                                    commandApduPtr,
                                    commandApduNumElements,
                                    responseApduPtr,
                                    responseApduNumElementsPtr);
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
    le_sim_Id_t simId,              ///< [IN] The SIM identifier
    le_sim_Command_t command,       ///< [IN] The SIM command
    const char* fileIdentifier,     ///< [IN] File identifier
    uint8_t p1,                     ///< [IN] Parameter P1 passed to the SIM
    uint8_t p2,                     ///< [IN] Parameter P2 passed to the SIM
    uint8_t p3,                     ///< [IN] Parameter P3 passed to the SIM
    const uint8_t* dataPtr,         ///< [IN] data command
    size_t dataNumElements,         ///< [IN]
    const char* path,               ///< [IN] path of the elementary file
    uint8_t* sw1Ptr,                ///< [OUT] SW1 received from the SIM
    uint8_t* sw2Ptr,                ///< [OUT] SW2 received from the SIM
    uint8_t* responsePtr,           ///< [OUT] SIM response
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
 * Enable or disable the automatic SIM selection
 *
 * @note Automatic SIM selection uses the following rule: If an external SIM is inserted in
 *       slot 1 then select it. Otherwise, fall back to the internal SIM card.
 *
 * @return
 *      - LE_OK            Function succeeded.
 *      - LE_FAULT         Function failed to execute.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_SetAutomaticSelection
(
    bool    enable    ///< [IN] True if the feature needs to be enabled. False otherwise.
)
{
    return pa_sim_SetAutomaticSelection(enable);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the automatic SIM selection
 *
 * @note When enabled, automatic SIM selection uses the following rule: If an external SIM is
 *       inserted in slot 1 then select it. Otherwise, fall back to the internal SIM card.
 *
 * @return
 *      - LE_OK             Function succeeded.
 *      - LE_FAULT          Function failed to execute.
 *      - LE_BAD_PARAMETER  Invalid parameter.
 *      - LE_UNSUPPORTED    The platform does not support this operation.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetAutomaticSelection
(
    bool*    enablePtr    ///< [OUT] True if the automatic mode is enabled. False otherwise.
)
{
    if (!enablePtr)
    {
        return LE_BAD_PARAMETER;
    }

    return pa_sim_GetAutomaticSelection(enablePtr);
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
    le_sim_FPLMNListRef_t FPLMNListRef,    ///< [IN] FPLMN list reference
    const char* mcc,                       ///< [IN] Mobile Country Code
    const char* mnc                        ///< [IN] Mobile Network Code
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
    le_sim_Id_t simId,                    ///< [IN] The SIM identifier
    le_sim_FPLMNListRef_t FPLMNListRef    ///< [IN] FPLMN list reference
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
    le_sim_Id_t simId                       ///< [IN] The SIM identifier
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
    le_sim_FPLMNListRef_t FPLMNListRef,       ///< [IN] FPLMN list reference
    char* mccPtr,                             ///< [OUT] Mobile Country Code
    size_t mccPtrSize,                        ///< [IN] Size of Mobile Country Code
    char* mncPtr,                             ///< [OUT] Mobile Network Code
    size_t mncPtrSize                         ///< [IN] Size of Mobile Network Code
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
    le_sim_FPLMNListRef_t FPLMNListRef,       ///< [IN] FPLMN list reference
    char* mccPtr,                             ///< [OUT] Mobile Country Code
    size_t mccPtrSize,                        ///< [IN] Size of Mobile Country Code
    char* mncPtr,                             ///< [OUT] Mobile Network Code
    size_t mncPtrSize                         ///< [IN] Size of Mobile Network Code
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
    le_sim_FPLMNListRef_t FPLMNListRef               ///< [IN] FPLMN list reference
)
{
    le_sim_FPLMNList_t* FPLMNListPtr = le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);

    if (NULL == FPLMNListPtr)
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

//--------------------------------------------------------------------------------------------------
/**
 * Open a logical channel on the SIM card.
 *
 * @return
 *      - LE_OK            Function succeeded.
 *      - LE_BAD_PARAMETER Invalid parameter.
 *      - LE_FAULT         Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_OpenLogicalChannel
(
    uint8_t* channelPtr ///< [OUT] The number of the opened logical channel.
)
{
    if (!channelPtr)
    {
        LE_ERROR("channelPtr is NULL");
        return LE_BAD_PARAMETER;
    }

    // Get the logical channel to send APDU.
    if (LE_OK != pa_sim_OpenLogicalChannel(channelPtr))
    {
        LE_WARN("Can't open a logical channel");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close a logical channel on the SIM card.
 *
 * @return
 *      - LE_OK            Function succeeded.
 *      - LE_FAULT         Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_CloseLogicalChannel
(
    uint8_t channel ///< [IN] The number of the logical channel to close.
)
{
    // Close the logical channel.
    if (LE_OK != pa_sim_CloseLogicalChannel(channel))
    {
        LE_WARN("Can't close the logical channel %d", channel);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send APDU command on a dedicated logical channel.
 *
 * @return
 *      - LE_OK             Function succeeded.
 *      - LE_BAD_PARAMETER  A parameter is invalid.
 *      - LE_NOT_FOUND      The function failed to select the SIM card for this operation.
 *      - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_SendApduOnChannel
(
    le_sim_Id_t simId,                  ///< [IN] The SIM identifier
    uint8_t channel,                    ///< [IN] The logical channel number
    const uint8_t* commandApduPtr,      ///< [IN] APDU command
    size_t commandApduNumElements,      ///< [IN] APDU command size
    uint8_t* responseApduPtr,           ///< [OUT] SIM response
    size_t* responseApduNumElementsPtr  ///< [INOUT] SIM response size
)
{
    if ((!commandApduPtr) || (!responseApduPtr) || (!responseApduNumElementsPtr))
    {
        LE_ERROR("NULL pointer provided");
        return LE_BAD_PARAMETER;
    }

    if (   (commandApduNumElements > LE_SIM_APDU_MAX_BYTES)
        || (*responseApduNumElementsPtr > LE_SIM_RESPONSE_MAX_BYTES))
    {
        LE_ERROR("Too many elements");
        return LE_BAD_PARAMETER;
    }

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }

    if (LE_OK != SelectSIMCard(simId))
    {
        return LE_NOT_FOUND;
    }

    // Send APDU through opened logical channel
    LE_DEBUG("Send APDU on logical channel %d", channel);
    LE_DUMP(commandApduPtr, commandApduNumElements);
    return pa_sim_SendApdu(channel,
                           commandApduPtr,
                           commandApduNumElements,
                           responseApduPtr,
                           responseApduNumElementsPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Powers up or down the current SIM card.
 *
 * @return LE_OK            Function succeeded.
 * @return LE_FAULT         Function failed.
 *
 * @note For SIM power cycle operation, it must wait until SIM state is LE_SIM_POWER_DOWN
 *       before powering on the SIM, otherwise power up SIM will fail.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_SetPower
(
    le_sim_Id_t simId,    ///< [IN] The SIM identifier.
    le_onoff_t  power     ///< [IN] The power state.
)
{
    if (LE_OK != SelectSIMCard(simId))
    {
        return LE_FAULT;
    }

    return pa_sim_SetPower(power);
}
