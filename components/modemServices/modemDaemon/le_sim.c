/** @file le_sim.c
 *
 * This file contains the data structures and the source code of the high level SIM APIs.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


#include "legato.h"
#include "interfaces.h"
#include "pa_sim.h"


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
    char             phoneNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES]; /// < The Phone Number.
    bool             isPresent;                  ///< 'isPresent' flag.
    Subscription_t   Subscription;               ///< Subscription type
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
    { 13, {0x80, 0xC2, 0x00, 0x00, 0x08, 0xCF, 0x06, 0x19, 0x01, 0x99, 0x5F, 0x01, 0x81} }
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
    { 13, {0x80, 0xC2, 0x00, 0x00, 0x08, 0xCF, 0x06, 0x19, 0x01, 0x99, 0x5F, 0x01, 0x80} }
};

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


    if (manufacturer == LE_SIM_G_AND_D)
    {
        uint8_t  pduReq[] = {0x00, 0xA4, 0x04, 0x00, 0x10, 0xD2, 0x76, 0x00,
                             0x01, 0x18, 0x00, 0x02, 0xFF, 0x34, 0x10, 0x25,
                             0x89, 0xC0, 0x02, 0x10, 0x01};

        // APDU command to select the applet
        if (pa_sim_OpenLogicalChannel(&channel) != LE_OK)
        {
            LE_ERROR("Cannot open Logical Channel!");
            return LE_FAULT;
        }
        pduReq[0] = channel;
        if (pa_sim_SendApdu(pduReq, sizeof(pduReq),
                            NULL, NULL) != LE_OK)
        {
            LE_ERROR("Cannot send APDU message!");
            return LE_FAULT;
        }
        swapApduReqPtr[0] = channel;
    }

    if (pa_sim_SendApdu(swapApduReqPtr,
                        swapApduLen,
                        NULL, NULL) != LE_OK)
    {
        LE_ERROR("Cannot swap subscription!");
        return LE_FAULT;
    }

    if (manufacturer == LE_SIM_G_AND_D)
    {
        if (pa_sim_CloseLogicalChannel(channel) != LE_OK)
        {
            LE_ERROR("Cannot close Logical Channel!");
            return LE_FAULT;
        }
    }

    if ((manufacturer == LE_SIM_OBERTHUR) || (manufacturer == LE_SIM_MORPHO))
    {
        return LE_OK;
    }
    else
    {
        return pa_sim_Refresh();
    }
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
    pa_sim_CardId_t   iccid;
    pa_sim_Imsi_t     imsi;

    switch(state)
    {
        case LE_SIM_ABSENT:
            simPtr->ICCID[0] = '\0';
            simPtr->IMSI[0] = '\0';
            simPtr->isPresent = false;
            break;

        case LE_SIM_INSERTED:
        case LE_SIM_BLOCKED:
            simPtr->isPresent = true;
            simPtr->IMSI[0] = '\0';
            if(pa_sim_GetCardIdentification(iccid) != LE_OK)
            {
                LE_ERROR("Failed to get the ICCID of sim identifier %d.", simPtr->simId);
                simPtr->ICCID[0] = '\0';
            }
            else
            {
                le_utf8_Copy(simPtr->ICCID, iccid, sizeof(simPtr->ICCID), NULL);
            }
            break;

        case LE_SIM_READY:
            simPtr->isPresent = true;
            // Get identification information
            if(pa_sim_GetCardIdentification(iccid) != LE_OK)
            {
                LE_ERROR("Failed to get the ICCID of sim identifier %d.", simPtr->simId);
                simPtr->ICCID[0] = '\0';
            }
            else
            {
                le_utf8_Copy(simPtr->ICCID, iccid, sizeof(simPtr->ICCID), NULL);
            }

            if(pa_sim_GetIMSI(imsi) != LE_OK)
            {
                LE_ERROR("Failed to get the IMSI of sim identifier %d.", simPtr->simId);
                simPtr->IMSI[0] = '\0';
            }
            else
            {
                le_utf8_Copy(simPtr->IMSI, imsi, sizeof(simPtr->IMSI), NULL);
            }
            break;

        case LE_SIM_BUSY:
        case LE_SIM_STATE_UNKNOWN:
            simPtr->isPresent = true;
            break;
    }
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
 * Handler function for new SIM state notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static void NewSimStateHandler
(
    pa_sim_Event_t* eventPtr
)
{
    Sim_t*           simPtr = NULL;
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
        SimList[i].isPresent = false;
        SimList[i].Subscription = UNKNOWN_SUBSCRIPTION;
        GetSimCardInformation(&SimList[i], LE_SIM_ABSENT);
    }

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
    pa_sim_CardId_t  iccid;
    le_result_t      res = LE_FAULT;
    Sim_t*           simPtr = NULL;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if (iccidPtr == NULL)
    {
        LE_KILL_CLIENT("iccidPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if (strlen(simPtr->ICCID) == 0)
    {
        if (SelectSIMCard(simPtr->simId) == LE_OK)
        {
            if (pa_sim_GetState(&state) == LE_OK)
            {
                if ((state == LE_SIM_INSERTED) ||
                    (state == LE_SIM_READY)    ||
                    (state == LE_SIM_BLOCKED))
                {
                    // Get identification information
                    if(pa_sim_GetCardIdentification(iccid) != LE_OK)
                    {
                        LE_ERROR("Failed to get the ICCID of sim identifier.%d", simPtr->simId);
                        simPtr->ICCID[0] = '\0';
                    }
                    else
                    {
                        // Note that it is not valid to truncate the ICCID.
                        res = le_utf8_Copy(simPtr->ICCID, iccid, sizeof(simPtr->ICCID), NULL);
                    }
                }
            }
        }
        else
        {
            LE_ERROR("Failed to get the ICCID of sim identifier.%d", simPtr->simId);
            simPtr->ICCID[0] = '\0';
        }
    }
    else
    {
        res = LE_OK;
    }

    // The ICCID is available. Copy it to the result buffer.
    if (res == LE_OK)
    {
        res = le_utf8_Copy(iccidPtr, simPtr->ICCID, iccidLen, NULL);
    }

    // If the ICCID could not be retrieved for some reason, or a truncation error occurred when
    // copying the result, then ensure the cache is cleared.
    if (res != LE_OK)
    {
        simPtr->ICCID[0] = '\0';
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
    pa_sim_Imsi_t    imsi;
    le_result_t      res = LE_FAULT;
    Sim_t*           simPtr = NULL;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if (imsiPtr == NULL)
    {
        LE_KILL_CLIENT("imsiPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if (strlen(simPtr->IMSI) == 0)
    {
        if (SelectSIMCard(simPtr->simId) == LE_OK)
        {
            if (pa_sim_GetState(&state) == LE_OK)
            {
                if (state == LE_SIM_READY)
                {
                    // Get identification information
                    if(pa_sim_GetIMSI(imsi) != LE_OK)
                    {
                        LE_ERROR("Failed to get the IMSI of sim identifier.%d", simPtr->simId);
                    }
                    else
                    {
                        // Note that it is not valid to truncate the IMSI.
                        res = le_utf8_Copy(simPtr->IMSI, imsi, sizeof(simPtr->IMSI), NULL);
                    }
                }
            }
        }
        else
        {
            LE_ERROR("Failed to get the IMSI of sim identifier.%d", simPtr->simId);
        }
    }
    else
    {
        res = LE_OK;
    }

    // The IMSI is available. Copy it to the result buffer.
    if (res == LE_OK)
    {
        res = le_utf8_Copy(imsiPtr, simPtr->IMSI, imsiLen, NULL);
    }

    // If the IMSI could not be retrieved for some reason, or a truncation error occurred when
    // copying the result, then ensure the cache is cleared.
    if (res != LE_OK)
    {
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
    Sim_t*           simPtr = NULL;

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
    Sim_t*       simPtr = NULL;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if (pinPtr == NULL)
    {
        LE_KILL_CLIENT("pinPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

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
    Sim_t*       simPtr = NULL;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if (oldpinPtr == NULL)
    {
        LE_KILL_CLIENT("oldpinPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if (newpinPtr == NULL)
    {
        LE_KILL_CLIENT("newpinPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

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
    Sim_t*    simPtr = NULL;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if (SelectSIMCard(simId) != LE_OK)
    {
        return LE_NOT_FOUND;
    }

    if (!simPtr->isPresent)
    {
        return LE_NOT_FOUND;
    }

    if(pa_sim_GetPINRemainingAttempts(PA_SIM_PIN, &attempts) != LE_OK)
    {
        LE_ERROR("Failed to get reamining attempts for sim identifier.%d", simId);
        return LE_FAULT;
    }

    return attempts;
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
    Sim_t*        simPtr = NULL;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if (pinPtr == NULL)
    {
        LE_KILL_CLIENT("pinPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

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
    Sim_t*       simPtr = NULL;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if (pinPtr == NULL)
    {
        LE_KILL_CLIENT("pinPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

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
 * @return LE_OUT_OF_RANGE  The PUK code length is not correct (8 digits).
 * @return LE_FAULT         The function failed to unlock the SIM card.
 * @return LE_OK            The function succeeded.
 *
 * @note If new PIN or puk code are too long (max 8 digits), it is a fatal error, the
 *       function will not return.
 *
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
    Sim_t*       simPtr = NULL;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if (pukPtr == NULL)
    {
        LE_KILL_CLIENT("pukPtr is NULL !");
        return LE_BAD_PARAMETER;
    }
    if (newpinPtr == NULL)
    {
        LE_KILL_CLIENT("newpinPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

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
    char           *phoneNumberStr,    ///< [OUT] Phone Number
    size_t          phoneNumberStrSize ///< [IN]  Size of phoneNumberStr
)
{
    le_sim_States_t  state;
    char             phoneNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = {0} ;
    le_result_t      res = LE_FAULT;
    Sim_t*           simPtr = NULL;

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        return LE_BAD_PARAMETER;
    }
    simPtr = &SimList[simId];

    if (phoneNumberStr == NULL)
    {
        LE_KILL_CLIENT("phoneNumberStr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if (strlen(simPtr->phoneNumber) == 0)
    {
        if (SelectSIMCard(simPtr->simId) == LE_OK)
        {
            if (pa_sim_GetState(&state) == LE_OK)
            {
                LE_DEBUG("Try get the Phone Number of sim identifier.%d in state %d",
                                simPtr->simId, state);
                if ((state == LE_SIM_INSERTED) ||
                    (state == LE_SIM_READY)    ||
                    (state == LE_SIM_BLOCKED))
                {
                    // Get identification information
                    if(pa_sim_GetSubscriberPhoneNumber(phoneNumber, sizeof(phoneNumber)) != LE_OK)
                    {
                        LE_ERROR("Failed to get the Phone Number of sim identifier.%d", simPtr->simId);
                        simPtr->phoneNumber[0] = '\0';
                    }
                    else
                    {
                        // Note that it is not valid to truncate the ICCID.
                        res = le_utf8_Copy(simPtr->phoneNumber,
                                           phoneNumber, sizeof(simPtr->phoneNumber), NULL);
                    }
                }
            }
        }
        else
        {
            LE_ERROR("Failed to get the Phone Number of sim identifier.%d", simPtr->simId);
            simPtr->phoneNumber[0] = '\0';
        }
    }
    else
    {
        res = LE_OK;
    }

    // The ICCID is available. Copy it to the result buffer.
    if (res == LE_OK)
    {
        res = le_utf8_Copy(phoneNumberStr,simPtr->phoneNumber,phoneNumberStrSize,NULL);
    }

    // If the ICCID could not be retrieved for some reason, or a truncation error occurred when
    // copying the result, then ensure the cache is cleared.
    if (res != LE_OK)
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
    Sim_t* simPtr = NULL;

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

    if (LocalSwap(manufacturer,
                  EcsSwapApduReq[manufacturer].apduReq,
                  EcsSwapApduReq[manufacturer].apduLength) == LE_OK)
    {
        simPtr->Subscription = ECS;
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
    Sim_t* simPtr = NULL;

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

    if (LocalSwap(manufacturer,
                  CommercialSwapApduReq[manufacturer].apduReq,
                  CommercialSwapApduReq[manufacturer].apduLength) == LE_OK)
    {
        simPtr->Subscription = COMMERCIAL;
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
 *      - LE_BAD_PARAMETER invalid SIM identifier
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
    Sim_t*      simPtr = NULL;

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

    switch (simPtr->Subscription)
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
    le_event_HandlerRef_t handlerRef = NULL;

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
    le_sim_Id_t simId,
        ///< [IN]
        ///< The SIM identifier.

    const uint8_t* commandApduPtr,
        ///< [IN]
        ///< APDU command.

    size_t commandApduNumElements,
        ///< [IN]

    uint8_t* responseApdu,
        ///< [OUT]
        ///< SIM response.

    size_t* responseApduNumElementsPtr
        ///< [INOUT]
)
{
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

    return pa_sim_SendApdu(commandApduPtr,
                           commandApduNumElements,
                           responseApdu,
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
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_SendCommand
(
    le_sim_Id_t simId,
        ///< [IN]
        ///< The SIM identifier.

    le_sim_Command_t command,
        ///< [IN]
        ///< The SIM command.

    const char* fileIdentifier,
        ///< [IN]
        ///< File identifier

    uint8_t p1,
        ///< [IN]
        ///< Parameter P1 passed to the SIM

    uint8_t p2,
        ///< [IN]
        ///< Parameter P2 passed to the SIM

    uint8_t p3,
        ///< [IN]
        ///< Parameter P3 passed to the SIM

    const uint8_t* dataPtr,
        ///< [IN]
        ///< data command.

    size_t dataNumElements,
        ///< [IN]

    const char* path,
        ///< [IN]
        ///< path of the elementary file

    uint8_t* sw1Ptr,
        ///< [OUT]
        ///< SW1 received from the SIM

    uint8_t* sw2Ptr,
        ///< [OUT]
        ///< SW2 received from the SIM

    uint8_t* responsePtr,
        ///< [OUT]
        ///< SIM response.

    size_t* responseNumElementsPtr
        ///< [INOUT]
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
