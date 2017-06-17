/**
 * This module implements some stubs for the Cellular Network service unit tests.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Event for new network registration state
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NewNetRegStateId = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Radio state
 */
//--------------------------------------------------------------------------------------------------
static le_onoff_t RadioState = LE_OFF;

//--------------------------------------------------------------------------------------------------
/**
 * Event for new SIM state
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NewSimStateEventId = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * SIM state event
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sim_Id_t      simId;   ///< SIM identifier
    le_sim_States_t  state;   ///< SIM state
}
Sim_Event_t;

//--------------------------------------------------------------------------------------------------
/**
 * Current selected SIM card
 */
//--------------------------------------------------------------------------------------------------
static le_sim_Id_t SelectedCard;

//--------------------------------------------------------------------------------------------------
/**
 * SIM presence
 */
//--------------------------------------------------------------------------------------------------
static bool SimPresence[LE_SIM_ID_MAX];

//--------------------------------------------------------------------------------------------------
/**
 * SimId used in secure storage names
 */
//--------------------------------------------------------------------------------------------------
static const char SimIdNames[LE_SIM_ID_MAX][2] = {"0", "1", "2", "3"};


//--------------------------------------------------------------------------------------------------
// Unit test specific functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Simulate a new MRC state
 */
//--------------------------------------------------------------------------------------------------
void le_mrcTest_SimulateState
(
    le_mrc_NetRegState_t state
)
{
    // Check if event is created before using it
    if (NewNetRegStateId)
    {
        // Notify all the registered client handlers
        le_event_Report(NewNetRegStateId, &state, sizeof(le_mrc_NetRegState_t));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Simulate the SIM presence
 */
//--------------------------------------------------------------------------------------------------
void le_simTest_SetPresent
(
    le_sim_Id_t simId,
    bool presence
)
{
    SimPresence[simId] = presence;
}

//--------------------------------------------------------------------------------------------------
/**
 * Simulate a new SIM state
 */
//--------------------------------------------------------------------------------------------------
void le_simTest_SimulateState
(
    le_sim_Id_t        simId,
    le_sim_States_t    simState
)
{
    // Check if event is created before using it
    if (NewSimStateEventId)
    {
        Sim_Event_t simEvent;

        // Notify all the registered client handlers
        simEvent.simId = simId;
        simEvent.state = simState;
        le_event_Report(NewSimStateEventId, &simEvent, sizeof(simEvent));
    }
}


//--------------------------------------------------------------------------------------------------
// Secure storage service stubbing
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Reads an item from secure storage.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer is too small to hold the entire item.  No data will be written to
 *                  the buffer in this case.
 *      LE_NOT_FOUND if the item does not exist.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_secStore_Read
(
    const char* name,               ///< [IN] Name of the secure storage item.
    uint8_t* bufPtr,                ///< [OUT] Buffer to store the data in.
    size_t* bufNumElementsPtr       ///< [INOUT] Size of buffer.
)
{
    le_result_t result = LE_FAULT;

    if (strstr(name, SimIdNames[LE_SIM_EMBEDDED]))
    {
        // Test #0: PIN not found
        result = LE_NOT_FOUND;
    }
    else if (strstr(name, SimIdNames[LE_SIM_EXTERNAL_SLOT_1]))
    {
        // Test #1: buffer too small
        result = LE_OVERFLOW;
    }
    else if (strstr(name, SimIdNames[LE_SIM_EXTERNAL_SLOT_2]))
    {
        // Test #2: PIN too short
        const char simPin[] = "000";
        memcpy(bufPtr, simPin, sizeof(simPin));
        *bufNumElementsPtr = sizeof(simPin);

        result = LE_OK;
    }
    else if (strstr(name, SimIdNames[LE_SIM_REMOTE]))
    {
        // Test #3: PIN found
        const char simPin[] = "00112233";
        memcpy(bufPtr, simPin, sizeof(simPin));
        *bufNumElementsPtr = sizeof(simPin);

        result = LE_OK;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Writes an item to secure storage.  If the item already exists then it will be overwritten with
 * the new value.  If the item does not already exist then it will be created.  Specifying 0 for
 * buffer size means emptying an existing file or creating a 0-byte file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NO_MEMORY if there is not enough memory to store the item.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_secStore_Write
(
    const char* name,               ///< [IN] Name of the secure storage item.
    const uint8_t* bufPtr,          ///< [IN] Buffer contain the data to store.
    size_t bufNumElements           ///< [IN] Size of buffer.
)
{
    le_result_t result = LE_FAULT;

    if (strstr(name, SimIdNames[LE_SIM_EMBEDDED]))
    {
        // Error
        result = LE_NO_MEMORY;
    }
    else if (strstr(name, SimIdNames[LE_SIM_REMOTE]))
    {
        // Success
        result = LE_OK;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
// SIM service stubbing
//--------------------------------------------------------------------------------------------------

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

    // Create an event Id for new SIM State notification if not already done
    if (!NewSimStateEventId)
    {
        NewSimStateEventId = le_event_CreateId("NewSimStateEventId", sizeof(Sim_Event_t));
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
    return LE_SIM_INSERTED;
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
    SelectedCard = simId;
    return LE_OK;
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
    return SimPresence[simId];
}


//--------------------------------------------------------------------------------------------------
// Modem Radio Control service stubbing
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Network Registration State Change Handler.
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerNetRegStateChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_mrc_NetRegState_t*           statePtr = reportPtr;
    le_mrc_NetRegStateHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(*statePtr, le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register an handler for Network registration state change.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_NetRegStateEventHandlerRef_t le_mrc_AddNetRegStateEventHandler
(
    le_mrc_NetRegStateHandlerFunc_t handlerFuncPtr, ///< [IN] The handler function.
    void*                           contextPtr      ///< [IN] The handler's context.
)
{
    le_event_HandlerRef_t        handlerRef;

    if (handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    // Create an event Id for new Network Registration State notification if not already done
    if (!NewNetRegStateId)
    {
        NewNetRegStateId = le_event_CreateId("NewNetRegState", sizeof(le_mrc_NetRegState_t));
    }

    handlerRef = le_event_AddLayeredHandler("NewNetRegStateHandler",
                                            NewNetRegStateId,
                                            FirstLayerNetRegStateChangeHandler,
                                            (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_mrc_NetRegStateEventHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Network registration state.
 *
 * @return LE_FAULT         The function failed to get the Network registration state.
 * @return LE_BAD_PARAMETER A bad parameter was passed.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetNetRegState
(
    le_mrc_NetRegState_t*   statePtr  ///< [OUT] The Network Registration state.
)
{
    *statePtr = LE_MRC_REG_SEARCHING;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Radio Module power state.
 *
 * @return LE_FAULT         The function failed to get the Radio Module power state.
 * @return LE_BAD_PARAMETER if powerPtr is NULL.
 * @return LE_OK            The function succeed.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetRadioPower
(
    le_onoff_t*    powerPtr   ///< [OUT] The power state.
)
{
    *powerPtr = RadioState;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the power of the Radio Module.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_SetRadioPower
(
    le_onoff_t    power   ///< [IN] The power state.
)
{
    RadioState = power;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start watchdogs 0..N-1.  Typically this is used in COMPONENT_INIT to start all watchdogs needed
 * by the process.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_Init
(
    uint32_t wdogCount
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Begin monitoring the event loop on the current thread.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_MonitorEventLoop
(
    uint32_t watchdog,          ///< Watchdog to use for monitoring
    le_clk_Time_t watchdogInterval ///< Interval at which to check event loop is functioning
)
{
}
