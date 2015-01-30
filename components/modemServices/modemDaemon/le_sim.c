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
// Data structures.
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * SIM structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sim_Obj
{
    le_sim_Type_t       type;                       ///< The SIM type.
    char                ICCID[LE_SIM_ICCID_BYTES];  ///< The integrated circuit card identifier.
    char                IMSI[LE_SIM_IMSI_BYTES];    ///< The international mobile subscriber identity.
    char                PIN[LE_SIM_PIN_MAX_BYTES];  ///< The PIN code.
    char                PUK[LE_SIM_PUK_MAX_BYTES];  ///< The PUK code.
    char                phoneNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES]; /// < The Phone Number.
    bool                isPresent;                  ///< The 'isPresent' flag.
    void*               ref;                        ///< The safe reference for this object.
    le_dls_Link_t       link;                       ///< The Sim Object node link.
}
Sim_t;

//--------------------------------------------------------------------------------------------------
/**
 * Create and initialize the SIM list.
 *
 */
//--------------------------------------------------------------------------------------------------
le_dls_List_t SimList = LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * session structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_SessionRef_t sessionRef;                 ///< Session reference
    le_sim_ObjRef_t     simRef;                     ///< SIM reference associate to the session
    le_dls_Link_t       link;                       ///< The Sim session node link.
}
SimSession_t;

//--------------------------------------------------------------------------------------------------
/**
 * SIM session list.
 *
 */
//--------------------------------------------------------------------------------------------------
le_dls_List_t SimSessionList = LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for SIM objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   SimPool;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for SIM session objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   SimSessionPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for SIM objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t SimRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Current selected SIM card.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_sim_Type_t  SelectedCard;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for New SIM state notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NewSimStateEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Remove a SIM from the SIM session list
 *
 */
//--------------------------------------------------------------------------------------------------
static void RemoveSimFromSimSessionList
(
    le_sim_ObjRef_t simRef
)
{
    SimSession_t*   simSessionPtr;
    le_dls_Link_t*  linkPtr;

    linkPtr = le_dls_Peek(&SimSessionList);

    while (linkPtr != NULL)
    {
        // Get the node from SimList
        simSessionPtr = CONTAINER_OF(linkPtr, SimSession_t, link);
        // Check the node.
        if (simSessionPtr->simRef == simRef)
        {
            // Move to the next node.
            linkPtr = le_dls_PeekNext(&SimSessionList, linkPtr);

            // Remove the link from the list.
            le_dls_Remove(&SimSessionList, &simSessionPtr->link);
        }
        else
        {
            // Move to the next node.
            linkPtr = le_dls_PeekNext(&SimSessionList, linkPtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Add a client to the SIM session list
 *
 */
//--------------------------------------------------------------------------------------------------
static void AddClientInSimSessionList
(
    le_sim_ObjRef_t simRef
)
{
    SimSession_t* simSessionPtr=NULL;

    // Create the message node.
    simSessionPtr = (SimSession_t*)le_mem_ForceAlloc(SimSessionPool);

    simSessionPtr->sessionRef = le_sim_GetClientSessionRef();
    simSessionPtr->simRef = simRef;
    simSessionPtr->link = LE_DLS_LINK_INIT;

    // Insert the message in the Sim Session List.
    le_dls_Queue(&SimSessionList, &(simSessionPtr->link));
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
                LE_ERROR("Failed to get the ICCID of SIM type %d.", simPtr->type);
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
                LE_ERROR("Failed to get the ICCID of SIM type %d.", simPtr->type);
                simPtr->ICCID[0] = '\0';
            }
            else
            {
                le_utf8_Copy(simPtr->ICCID, iccid, sizeof(simPtr->ICCID), NULL);
            }

            if(pa_sim_GetIMSI(imsi) != LE_OK)
            {
                LE_ERROR("Failed to get the IMSI of SIM type %d.", simPtr->type);
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
 * Creates a new SIM object for a given SIM type.
 *
 * @return A pointer to the SIM object.
 */
//--------------------------------------------------------------------------------------------------
static Sim_t* CreateSim
(
    le_sim_Type_t simType,    ///< [in] The SIM type
    le_sim_States_t state       ///< [in] The current state of the SIM.
)
{
    Sim_t            *simPtr=NULL;

    // Create the message node.
    simPtr = (Sim_t*)le_mem_ForceAlloc(SimPool);

    simPtr->type= simType;
    simPtr->ICCID[0] = '\0';
    simPtr->IMSI[0] = '\0';
    simPtr->isPresent = false;
    simPtr->ref = NULL;
    simPtr->link = LE_DLS_LINK_INIT;
    GetSimCardInformation(simPtr, state);

    // Create the safeRef for the SIM object
    simPtr->ref = le_ref_CreateRef(SimRefMap, simPtr);
    LE_DEBUG("Created ref=%p for ptr=%p", simPtr->ref, simPtr);

    // Insert the message in the Sim List.
    le_dls_Queue(&SimList, &(simPtr->link));

    return simPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sim destructor.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SimDestructor
(
    void* objPtr
)
{
    // Invalidate the Safe Reference.
    le_ref_DeleteRef(SimRefMap, ((Sim_t*)objPtr)->ref);

    // Remove the SIM object from the SIM List.
    le_dls_Remove(&SimList, &(((Sim_t*)objPtr)->link));
}

//--------------------------------------------------------------------------------------------------
/**
 * Searches the SIM List for a SIM matching a given SIM type.
 *
 * @return A pointer to the SIM object, or NULL if not found.
 */
//--------------------------------------------------------------------------------------------------
static Sim_t* FindSim
(
    le_sim_Type_t simType    ///< [in] The SIM type
)
{
    Sim_t*          simPtr;
    le_dls_Link_t*  linkPtr;

    linkPtr = le_dls_Peek(&SimList);
    while (linkPtr != NULL)
    {
        // Get the node from SimList
        simPtr = CONTAINER_OF(linkPtr, Sim_t, link);
        // Check the node.
        if (simPtr->type == simType)
        {
            return simPtr;
        }

        // Move to the next node.
        linkPtr = le_dls_PeekNext(&SimList, linkPtr);
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * SIM card selector.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SelectSIMCard
(
    le_sim_Type_t simType
)
{
    if(simType != SelectedCard)
    {
        // Select the SIM card
        LE_DEBUG("Try to select SIM type.%d", simType);
        if(pa_sim_SelectCard(simType) != LE_OK)
        {
            LE_ERROR("Failed to select SIM type.%d", simType);
            return LE_NOT_FOUND;
        }
        SelectedCard = simType;
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
    le_sim_ObjRef_t*             referencePtr = reportPtr;
    le_sim_NewStateHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(*referencePtr, le_event_GetContextPtr());
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
    LE_DEBUG("New SIM state.%d for type.%d (eventPtr %p)", eventPtr->state, eventPtr->simType, eventPtr);

    Sim_t* simPtr = FindSim(eventPtr->simType);

    // Create the SIM object if it does not exist yet
    if (simPtr == NULL)
    {
        LE_INFO("NO SIM object found, create a new one");
        simPtr = CreateSim(eventPtr->simType, eventPtr->state);
    }
    // Otherwise, just update the one we found.
    else
    {
        LE_DEBUG("Found SIM object for SIM type %d.", eventPtr->simType);
        GetSimCardInformation(simPtr, eventPtr->state);
    }

    // Discard transitional states
    switch (eventPtr->state)
    {
        case LE_SIM_BUSY:
        case LE_SIM_STATE_UNKNOWN:
            LE_DEBUG("Discarding report for type.%d, state.%d", eventPtr->simType, eventPtr->state);
            return;

        default:
            break;
    }

    // Notify all the registered client handlers
    le_event_Report(NewSimStateEventId, &simPtr->ref, sizeof(le_sim_ObjRef_t));
    LE_DEBUG("Report on SIM reference %p", simPtr->ref);

    le_mem_Release(eventPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * handler function to the close session service
 *
 */
//--------------------------------------------------------------------------------------------------
static void CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
{
    LE_INFO("Client killed");

   if(!sessionRef)
    {
        LE_ERROR("ERROR sessionRef is NULL");
        return;
    }

    SimSession_t*   simSessionPtr;
    le_dls_Link_t*  linkPtr;

    linkPtr = le_dls_Peek(&SimSessionList);
    while (linkPtr != NULL)
    {
        // Get the node from SimList
        simSessionPtr = CONTAINER_OF(linkPtr, SimSession_t, link);
        // Check the node.
        if (simSessionPtr->sessionRef == sessionRef)
        {
            // Move to the next node.
            linkPtr = le_dls_PeekNext(&SimSessionList, linkPtr);

            Sim_t* simPtr = le_ref_Lookup(SimRefMap, simSessionPtr->simRef);

            if (simPtr == NULL)
            {
                LE_ERROR("Invalid reference (%p) ", simSessionPtr->simRef);
                return;
            }

            LE_DEBUG( "Release simRef %p", simSessionPtr->simRef );

            // release the SIM object
            le_mem_Release(simPtr);

            // Remove the link from the list.
            le_dls_Remove(&SimSessionList, &simSessionPtr->link);
        }
        else
        {
            // Move to the next node.
            linkPtr = le_dls_PeekNext(&SimSessionList, linkPtr);
        }

    }
}

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the SIM operations component
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sim_Init
(
    void
)
{
    // Create a pool for SIM objects
    SimPool = le_mem_CreatePool("SimPool", sizeof(Sim_t));
    le_mem_SetDestructor(SimPool, SimDestructor);
    le_mem_ExpandPool(SimPool, LE_SIM_TYPE_MAX);

    // Create a pool for SIM Session objects
    SimSessionPool = le_mem_CreatePool("SimSessionPool", sizeof(SimSession_t));
    le_mem_ExpandPool(SimSessionPool, 10);

    // Create the Safe Reference Map to use for SIM object Safe References.
    SimRefMap = le_ref_CreateMap("SimMap", LE_SIM_TYPE_MAX);

    LE_FATAL_IF((pa_sim_GetSelectedCard(&SelectedCard) != LE_OK), "Unable to get selected card.");

    LE_DEBUG("SIM %u is selected.", SelectedCard);

    // Create an event Id for new SIM state notifications
    NewSimStateEventId = le_event_CreateId("NewSimState", sizeof(le_sim_ObjRef_t));

    // Register a handler function for new SIM state notification
    LE_FATAL_IF((pa_sim_AddNewStateHandler(NewSimStateHandler) == NULL),
                "Add new SIM state handler failed");

    // Add a handler to the close session service
    le_msg_AddServiceCloseHandler( le_sim_GetServiceRef(),
                                   CloseSessionEventHandler,
                                   NULL );
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the current selected card.
 *
 * @return The number of the current selected SIM card.
 */
//--------------------------------------------------------------------------------------------------
le_sim_Type_t le_sim_GetSelectedCard
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
    le_sim_ObjRef_t  simRef    ///< [IN] The SIM object.
)
{
    Sim_t*                  simPtr;

    simPtr = le_ref_Lookup(SimRefMap, simRef);

    if (simPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", simRef);
        return LE_FAULT;
    }

    // Select the SIM card
    if (SelectSIMCard(simPtr->type) != LE_OK)
    {
        LE_ERROR("Unable to select Sim Card slot %d !", simPtr->type);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create a SIM object.
 *
 * @return A reference to the SIM object.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_sim_ObjRef_t le_sim_Create
(
    le_sim_Type_t simType
)
{
    Sim_t* simPtr = FindSim(simType);
    // Create the SIM object if it does not exist yet
    if (simPtr == NULL)
    {
        LE_INFO("NO SIM object found, create a new one");

        simPtr = CreateSim(simType, LE_SIM_STATE_UNKNOWN);
    }
    // If the SIM already exists, then just increment the reference count on it.
    else
    {
        le_mem_AddRef(simPtr);
    }

    // Add the client into the session list
    AddClientInSimSessionList(simPtr->ref);

    // Return a Safe Reference for this Sim object.
    return (simPtr->ref);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete a SIM object.
 *
 * It deletes the SIM object, all the allocated memory is freed. However if several Users own the
 * SIM object (for example in the case of several handler functions registered for SIM state
 * notification) the SIM object will be actually deleted only if it remains one User owning the SIM
 * object.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sim_Delete
(
    le_sim_ObjRef_t  simRef    ///< [IN] The SIM object.
)
{
    Sim_t*                  simPtr;

    simPtr = le_ref_Lookup(SimRefMap, simRef);
    if (simPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", simRef);
        return;
    }

    // Remove the link from the list.
    RemoveSimFromSimSessionList(simRef);

    // release the SIM object
    le_mem_Release(simPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to retrieve the slot number of the SIM card.
 *
 * @return The slot number of the SIM card.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_sim_GetSlotNumber
(
    le_sim_ObjRef_t simRef   ///< [IN] The SIM object.
)
{
    Sim_t* simPtr = le_ref_Lookup(SimRefMap, simRef);

    if (simPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", simRef);
        return 0;
    }

    return simPtr->type;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function retrieves the integrated circuit card identifier (ICCID) of the SIM card (20 digits)
 *
 * @return LE_OK            The ICCID was successfully retrieved.
 * @return LE_OVERFLOW      The iccidPtr buffer was too small for the ICCID.
 * @return LE_BAD_PARAMETER if a parameter is invalid
 * @return LE_FAULT         The ICCID could not be retrieved.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetICCID
(
    le_sim_ObjRef_t simRef, ///< [IN] The SIM object.
    char * iccidPtr,        ///< [OUT] Buffer to hold the ICCID.
    size_t iccidLen         ///< [IN] Buffer length
)
{
    le_sim_States_t  state;
    pa_sim_CardId_t  iccid;
    Sim_t*           simPtr = le_ref_Lookup(SimRefMap, simRef);
    le_result_t      res = LE_FAULT;

    if (simPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", simRef);
        return LE_BAD_PARAMETER;
    }
    if (iccidPtr == NULL)
    {
        LE_KILL_CLIENT("iccidPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if (strlen(simPtr->ICCID) == 0)
    {
        if (SelectSIMCard(simPtr->type) == LE_OK)
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
                        LE_ERROR("Failed to get the ICCID of SIM type.%d", simPtr->type);
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
            LE_ERROR("Failed to get the ICCID of SIM type.%d", simPtr->type);
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
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetIMSI
(
    le_sim_ObjRef_t simRef, ///< [IN] The SIM object.
    char * imsiPtr,         ///< [OUT] Buffer to hold the IMSI.
    size_t imsiLen          ///< [IN] Buffer length
)
{
    le_sim_States_t  state;
    pa_sim_Imsi_t    imsi;
    Sim_t*           simPtr = le_ref_Lookup(SimRefMap, simRef);
    le_result_t      res = LE_FAULT;

    if (simPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", simRef);
        return LE_BAD_PARAMETER;
    }
    if (imsiPtr == NULL)
    {
        LE_KILL_CLIENT("imsiPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if (strlen(simPtr->IMSI) == 0)
    {
        if (SelectSIMCard(simPtr->type) == LE_OK)
        {
            if (pa_sim_GetState(&state) == LE_OK)
            {
                if (state == LE_SIM_READY)
                {
                    // Get identification information
                    if(pa_sim_GetIMSI(imsi) != LE_OK)
                    {
                        LE_ERROR("Failed to get the IMSI of SIM type.%d", simPtr->type);
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
            LE_ERROR("Failed to get the IMSI of SIM type.%d", simPtr->type);
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
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
bool le_sim_IsPresent
(
    le_sim_ObjRef_t simRef    ///< [IN] The SIM object.
)
{
    le_sim_States_t  state;
    Sim_t*           simPtr = le_ref_Lookup(SimRefMap, simRef);

    if (simPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", simRef);
        return false;
    }

    if (SelectSIMCard(simPtr->type) != LE_OK)
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
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
bool le_sim_IsReady
(
    le_sim_ObjRef_t simRef    ///< [IN] The SIM object.
)
{
    le_sim_States_t state;
    Sim_t*          simPtr = le_ref_Lookup(SimRefMap, simRef);

    if (simPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", simRef);
        return false;
    }

    if (SelectSIMCard(simPtr->type) != LE_OK)
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
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_EnterPIN
(
    le_sim_ObjRef_t simRef, ///< [IN] The SIM object.
    const char*  pinPtr     ///< [IN] The PIN code.
)
{
    pa_sim_Pin_t pinloc;
    Sim_t*       simPtr = le_ref_Lookup(SimRefMap, simRef);

    if (simPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", simRef);
        return LE_BAD_PARAMETER;
    }

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

    if (SelectSIMCard(simPtr->type) != LE_OK)
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
        LE_ERROR("Failed to enter PIN.%s SIM type.%d", pinPtr, simPtr->type);
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
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_ChangePIN
(
    le_sim_ObjRef_t simRef, ///< [IN] The SIM object.
    const char*  oldpinPtr, ///< [IN] The old PIN code.
    const char*  newpinPtr  ///< [IN] The new PIN code.
)
{
    pa_sim_Pin_t oldpinloc;
    pa_sim_Pin_t newpinloc;
    Sim_t*       simPtr = le_ref_Lookup(SimRefMap, simRef);

    if (simPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", simRef);
        return LE_BAD_PARAMETER;
    }
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

    if (SelectSIMCard(simPtr->type) != LE_OK)
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
        LE_ERROR("Failed to set new PIN.%s of SIM type.%d", newpinPtr, simPtr->type);
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
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_sim_GetRemainingPINTries
(
    le_sim_ObjRef_t simRef    ///< [IN] The SIM object.
)
{
    uint32_t  attempts=0;
    Sim_t*    simPtr = le_ref_Lookup(SimRefMap, simRef);

    if (simPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", simRef);
        return LE_FAULT;
    }

    if (SelectSIMCard(simPtr->type) != LE_OK)
    {
        return LE_NOT_FOUND;
    }

    if (!simPtr->isPresent)
    {
        return LE_NOT_FOUND;
    }

    if(pa_sim_GetPINRemainingAttempts(PA_SIM_PIN, &attempts) != LE_OK)
    {
        LE_ERROR("Failed to get reamining attempts for SIM type.%d", simPtr->type);
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
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_Unlock
(
    le_sim_ObjRef_t simRef, ///< [IN] The SIM object.
    const char*     pinPtr  ///< [IN] The PIN code.
)
{
    pa_sim_Pin_t pinloc;
    Sim_t*       simPtr = le_ref_Lookup(SimRefMap, simRef);

    if (simPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", simRef);
        return LE_BAD_PARAMETER;
    }

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

    if (SelectSIMCard(simPtr->type) != LE_OK)
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
        LE_ERROR("Failed to unlock SIM type.%d", simPtr->type);
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
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_Lock
(
    le_sim_ObjRef_t simRef, ///< [IN] The SIM object.
    const char*     pinPtr  ///< [IN] The PIN code.
)
{
    pa_sim_Pin_t pinloc;
    Sim_t*       simPtr = le_ref_Lookup(SimRefMap, simRef);

    if (simPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", simRef);
        return LE_BAD_PARAMETER;
    }
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

    if (SelectSIMCard(simPtr->type) != LE_OK)
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
        LE_ERROR("Failed to Lock SIM type.%d", simPtr->type);
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
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_Unblock
(
    le_sim_ObjRef_t simRef,    ///< [IN] The SIM object.
    const char*     pukPtr,    ///< [IN] The PUK code.
    const char*     newpinPtr  ///< [IN] The new PIN code.
)
{
    pa_sim_Puk_t pukloc;
    pa_sim_Pin_t newpinloc;
    Sim_t*       simPtr = le_ref_Lookup(SimRefMap, simRef);

    if (simPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", simRef);
        return LE_BAD_PARAMETER;
    }
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

    if(strlen(pukPtr) > LE_SIM_PIN_MAX_LEN)
    {
        LE_KILL_CLIENT("strlen(pukPtr) > %d", LE_SIM_PIN_MAX_LEN);
        return LE_BAD_PARAMETER;
    }

    if(strlen(newpinPtr) < LE_SIM_PIN_MIN_LEN)
    {
        return LE_UNDERFLOW;
    }

    if (SelectSIMCard(simPtr->type) != LE_OK)
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
        LE_ERROR("Failed to unblock SIM type.%d", simPtr->type);
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
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sim_States_t le_sim_GetState
(
    le_sim_ObjRef_t simRef    ///< [IN] The SIM object.
)
{
    le_sim_States_t state;
    Sim_t*          simPtr = le_ref_Lookup(SimRefMap, simRef);

    if (simPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", simRef);
        return LE_SIM_STATE_UNKNOWN;
    }

    if (SelectSIMCard(simPtr->type) != LE_OK)
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
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_sim_NewStateHandlerRef_t le_sim_AddNewStateHandler
(
    le_sim_NewStateHandlerFunc_t handlerFuncPtr, ///< [IN] The handler function for New State notification.
    void*                        contextPtr      ///< [IN] The handler's context.
)
{
    le_event_HandlerRef_t handlerRef;

    if (handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("NewSimStateHandler",
                                            NewSimStateEventId,
                                            FirstLayerNewSimStateHandler,
                                            (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_sim_NewStateHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister a handler function
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_sim_RemoveNewStateHandler
(
    le_sim_NewStateHandlerRef_t   handlerRef ///< [IN] The handler reference.
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
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetSubscriberPhoneNumber
(
    le_sim_ObjRef_t simRef,            ///< [IN]  SIM object.
    char           *phoneNumberStr,    ///< [OUT] The phone Number
    size_t          phoneNumberStrSize ///< [IN]  Size of phoneNumberStr
)
{
    le_sim_States_t  state;
    char             phoneNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = {0} ;
    Sim_t*           simPtr = le_ref_Lookup(SimRefMap, simRef);
    le_result_t      res = LE_FAULT;

    if (simPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", simRef);
        return LE_BAD_PARAMETER;
    }
    if (phoneNumberStr == NULL)
    {
        LE_KILL_CLIENT("phoneNumberStr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if (strlen(simPtr->phoneNumber) == 0)
    {
        if (SelectSIMCard(simPtr->type) == LE_OK)
        {
            if (pa_sim_GetState(&state) == LE_OK)
            {
                LE_DEBUG("Try get the Phone Number of SIM type.%d in state %d", simPtr->type, state);
                if ((state == LE_SIM_INSERTED) ||
                    (state == LE_SIM_READY)    ||
                    (state == LE_SIM_BLOCKED))
                {
                    // Get identification information
                    if(pa_sim_GetSubscriberPhoneNumber(phoneNumber, sizeof(phoneNumber)) != LE_OK)
                    {
                        LE_ERROR("Failed to get the Phone Number of SIM type.%d", simPtr->type);
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
            LE_ERROR("Failed to get the Phone Number of SIM type.%d", simPtr->type);
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
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetHomeNetworkOperator
(
    char       *nameStr,               ///< [OUT] the home network Name
    size_t      nameStrSize            ///< [IN] the nameStr size
)
{
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
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 *
 */
//--------------------------------------------------------------------------------------------------
 le_result_t le_sim_GetHomeNetworkMccMnc
(
    char     *mccPtr,                ///< [OUT] Mobile Country Code
    size_t    mccPtrSize,            ///< [IN] mccPtr buffer size
    char     *mncPtr,                ///< [OUT] Mobile Network Code
    size_t    mncPtrSize             ///< [IN] mncPtr buffer size
)
{
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
 *      - LE_BUSY when a profile swap is already in progress
 *      - LE_FAULT for unexpected error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_LocalSwapToEmergencyCallSubscription
(
    le_sim_ObjRef_t       simRef,         ///< [IN]  SIM object.
    le_sim_Manufacturer_t manufacturer    ///< [IN]  The card manufacturer.
)
{
    // TODO: implement this function
    return LE_FAULT;
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
 *      - LE_BUSY when a profile swap is already in progress
 *      - LE_FAULT for unexpected error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_LocalSwapToCommercialSubscription
(
    le_sim_ObjRef_t       simRef,         ///< [IN]  SIM object.
    le_sim_Manufacturer_t manufacturer    ///< [IN]  The card manufacturer.
)
{
    // TODO: implement this function
    return LE_FAULT;
}

