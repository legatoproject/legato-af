// -------------------------------------------------------------------------------------------------
/**
 *  Cellular Network Services Server
 *
 *  Copyright (C) Sierra Wireless Inc.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "le_print.h"
#include "watchdogChain.h"

//--------------------------------------------------------------------------------------------------
/**
 *  Nodes in the secure storage used to store PIN codes.
 */
//--------------------------------------------------------------------------------------------------
#define SECSTORE_NODE_SIM   "sim"
#define SECSTORE_NODE_PIN   "pin"

//--------------------------------------------------------------------------------------------------
/**
 * Secure storage max attempt number.
 *
 */
//--------------------------------------------------------------------------------------------------
#define SECSTORE_ATTEMPT_MAX   5

//--------------------------------------------------------------------------------------------------
/**
 * Definitions for sending request/release commands to CellNet thread
 */
//--------------------------------------------------------------------------------------------------
#define REQUEST_COMMAND 1
#define RELEASE_COMMAND 2

//--------------------------------------------------------------------------------------------------
/**
 * The timer interval to kick the watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
#define MS_WDOG_INTERVAL 8

static le_event_Id_t CommandEvent;

//--------------------------------------------------------------------------------------------------
/**
 * Count the number of requests
 */
//--------------------------------------------------------------------------------------------------
static uint32_t RequestCount = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for the request reference
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t RequestRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Event for sending Cellular Network Registration state to applications
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t CellNetStateEvent;

//--------------------------------------------------------------------------------------------------
/**
 * Current cellular network state
 */
//--------------------------------------------------------------------------------------------------
static le_cellnet_State_t CurrentState = LE_CELLNET_REG_UNKNOWN;

//--------------------------------------------------------------------------------------------------
/**
 * List of cellular network state strings
 */
//--------------------------------------------------------------------------------------------------
static const char* cellNetStateStr[] =
{
     "LE_CELLNET_RADIO_OFF",
     "LE_CELLNET_REG_EMERGENCY",
     "LE_CELLNET_REG_HOME",
     "LE_CELLNET_REG_ROAMING",
     "LE_CELLNET_REG_UNKNOWN",
     "LE_CELLNET_SIM_ABSENT"
};


//--------------------------------------------------------------------------------------------------
/**
 * Load the SIM configuration from the secure storage
 */
//--------------------------------------------------------------------------------------------------
static void LoadSimFromSecStore
(
    le_sim_Id_t simId
)
{
    uint32_t attemptCounter = SECSTORE_ATTEMPT_MAX;
    le_result_t result;
    le_sim_States_t simState;

    LE_DEBUG("Start reading SIM-%d information in secure storage",simId);

    do
    {
        simState = le_sim_GetState(simId);

        switch (simState)
        {
            case LE_SIM_INSERTED:
            {
                // Set the secure storage path for the SIM
                char secStorePath[LE_SECSTORE_MAX_NAME_BYTES];
                snprintf(secStorePath, sizeof(secStorePath), "%s/%d/%s",
                         SECSTORE_NODE_SIM, simId, SECSTORE_NODE_PIN);

                char simPin[LE_SIM_PIN_MAX_BYTES] = {0};
                size_t simSize = LE_SIM_PIN_MAX_BYTES;

                // Read PIN code stored in secure storage
                result = le_secStore_Read(secStorePath, (uint8_t *)simPin, &simSize);
                if (LE_NOT_FOUND == result)
                {
                    LE_ERROR("SIM PIN code isn't found in the secure storage");
                    return;
                }
                else if (LE_OVERFLOW == result)
                {
                    LE_WARN("PIN string too large for SIM-%d", simId);
                    return;
                }
                else if (LE_OK != result)
                {
                    LE_ERROR("Unable to retrieve PIN for SIM-%d, error %s",
                             simId, LE_RESULT_TXT(result));
                    return;
                }
                if (0 == strncmp(simPin, "", sizeof(simPin)))
                {
                    LE_WARN("PIN not set for SIM-%d", simId);
                    return;
                }
                if (LE_OK != (result = le_sim_EnterPIN(simId, simPin)))
                {
                    LE_ERROR("Error.%d Failed to enter SIM pin for SIM-%d", result, simId);
                    return;
                }
                LE_DEBUG("Sim-%d is unlocked", simId);

                attemptCounter = 1;
                break;
            }
            case LE_SIM_BLOCKED:
            {
                LE_EMERG("Be careful the sim-%d is BLOCKED, need to enter PUK code",simId);
                attemptCounter = 1;
                break;
            }
            case LE_SIM_BUSY:
                if (attemptCounter==1)
                {
                    LE_WARN("Could not load the configuration because "
                            "the SIM is still busy after %d attempts", SECSTORE_ATTEMPT_MAX);
                }
                else
                {
                    LE_WARN("Sim-%d was busy when loading configuration,"
                            "retry in 1 seconds",simId);
                }
                sleep(1); // Retry in 1 second.
                break;
            case LE_SIM_READY:
                LE_DEBUG("Sim-%d is ready",simId);
                attemptCounter = 1;
                break;
            case LE_SIM_ABSENT:
                LE_WARN("Sim-%d is absent",simId);
                attemptCounter = 1;
                break;
            case LE_SIM_POWER_DOWN:
                LE_WARN("Sim-%d is powered down",simId);
                break;
            case LE_SIM_STATE_UNKNOWN:
                break;
        }
    } while (--attemptCounter);

    LE_DEBUG("Load SIM information is done");
}

//--------------------------------------------------------------------------------------------------
/**
 * Translate MRC states to CellNet states
 */
//--------------------------------------------------------------------------------------------------
static le_cellnet_State_t TranslateToCellNetState
(
    le_mrc_NetRegState_t state
)
{
    le_sim_Id_t simSelected = le_sim_GetSelectedCard();

    // Check if the SIM card is present
    if (!le_sim_IsPresent(simSelected))
    {
        // SIM card absent
        return LE_CELLNET_SIM_ABSENT;
    }

    // SIM card present, translate the MRC network state
    switch(state)
    {
        case LE_MRC_REG_NONE:
        {
            le_onoff_t  radioState;
            le_result_t result;

            // In this state, the radio should be OFF.
            if ((result = le_mrc_GetRadioPower(&radioState)) != LE_OK)
            {
                LE_WARN("Failed to get the radio power. Result: %d", result);
                return LE_CELLNET_REG_UNKNOWN;
            }
            else
            {
                if (radioState == LE_OFF)
                {
                    // The radio is OFF
                    return LE_CELLNET_RADIO_OFF;
                }
                else
                {
                    // The radio is ON
                    return LE_CELLNET_REG_EMERGENCY;
                }
            }
        }
        case LE_MRC_REG_SEARCHING:
        case LE_MRC_REG_DENIED:
            return LE_CELLNET_REG_EMERGENCY;
        case LE_MRC_REG_HOME:
            return LE_CELLNET_REG_HOME;
        case LE_MRC_REG_ROAMING:
            return LE_CELLNET_REG_ROAMING;
        default:
            return LE_CELLNET_REG_UNKNOWN;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Report connection state event to the registered applications
 */
//--------------------------------------------------------------------------------------------------
static void ReportCellNetStateEvent
(
    le_cellnet_State_t state
)
{
    LE_DEBUG("Report cellular network state %d (%s)", state, cellNetStateStr[state]);

    // Update current network cell state
    CurrentState = state;

    // Send the event to interested applications
    le_event_Report(CellNetStateEvent, &state, sizeof(state));
}

//--------------------------------------------------------------------------------------------------
/**
 * Send connection state event
 */
//--------------------------------------------------------------------------------------------------
static void GetAndSendCellNetStateEvent
(
    void
)
{
    le_mrc_NetRegState_t state        = LE_MRC_REG_UNKNOWN;
    le_cellnet_State_t   cellNetState = LE_CELLNET_REG_UNKNOWN;

    // Retrieve network registration state
    if (LE_OK == le_mrc_GetNetRegState(&state))
    {
        cellNetState = TranslateToCellNetState(state);
    }
    else
    {
        LE_ERROR("Impossible to retrieve network registration state!");
    }

    LE_DEBUG("MRC network state %d translated to Cellular network state %d (%s)",
             state, cellNetState, cellNetStateStr[cellNetState]);

    // Send the state event to applications
    ReportCellNetStateEvent(cellNetState);
}

//--------------------------------------------------------------------------------------------------
/**
 * Start Cellular Network Service Timer Handler.
 * When the timer expires, verify if the radio is ON, if NOT retry to power it up and rearm the
 * timer.
 */
//--------------------------------------------------------------------------------------------------
static void StartCellNetTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    le_onoff_t  radioState;
    le_result_t result;

    if(RequestCount == 0)
    {
        // Release has been requested in the meantime, I must cancel the Request command process
        le_timer_Delete(timerRef);
    }
    else
    {
        result=le_mrc_GetRadioPower(&radioState);
        if ((result == LE_OK) && (radioState == LE_ON))
        {
            // The radio is ON, stop and delete the Timer.
            le_timer_Delete(timerRef);

            // Load SIM configuration from secure storage
            le_sim_Id_t simSelected = le_sim_GetSelectedCard();

            if (le_sim_IsPresent(simSelected))
            {
                LoadSimFromSecStore(simSelected);
            }

            // Notify the applications even if the SIM is absent
            GetAndSendCellNetStateEvent();
        }
        else
        {
            le_mrc_SetRadioPower(LE_ON);
            // TODO: find a solution to get off of this infinite loop
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start Cellular Network Service.
 * Loads the SIM configuration into the modem for the first SIM slot with a SIM present.
 * Turns on the radio first if it is off or if the radio power setting cannot be detected.
 */
//--------------------------------------------------------------------------------------------------
static void StartCellularNetwork
(
    void
)
{
    le_onoff_t  radioState;
    le_result_t result;

    result=le_mrc_GetRadioPower(&radioState);
    if ((result == LE_OK) && (radioState == LE_ON))
    {
        // Load SIM configuration from secure storage
        le_sim_Id_t simSelected = le_sim_GetSelectedCard();

        if (le_sim_IsPresent(simSelected))
        {
            LoadSimFromSecStore(simSelected);
        }

        // Notify the applications even if the SIM is absent
        GetAndSendCellNetStateEvent();
    }
    else
    {
        // Try to power ON the radio anyway
        le_mrc_SetRadioPower(LE_ON);

        // Set a timer that gets the current position.
        le_timer_Ref_t startCellNetTimer = le_timer_Create("StartCellNetTimer");
        le_clk_Time_t interval = {15, 0}; // 15 seconds

        if ( (le_timer_SetHandler(startCellNetTimer, StartCellNetTimerHandler) != LE_OK) ||
             (le_timer_SetRepeat(startCellNetTimer, 0) != LE_OK) ||
             (le_timer_SetInterval(startCellNetTimer, interval) != LE_OK) ||
             (le_timer_Start(startCellNetTimer) != LE_OK) )
        {
            LE_ERROR("Could not start the StartCellNet timer!");
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop Cellular Network Service Timer Handler.
 * When the timer expires, verify if the radio is OFF, if NOT retry to shutdown it and rearm the
 * timer.
 */
//--------------------------------------------------------------------------------------------------
static void StopCellNetTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    le_onoff_t  radioState;
    le_result_t result;

    if(RequestCount != 0)
    {
        // Request has been requested in the meantime, I must cancel the Release command process
        le_timer_Delete(timerRef);
    }
    else
    {
        result=le_mrc_GetRadioPower(&radioState);
        if ((result == LE_OK) && (radioState == LE_OFF))
        {
            // The radio is OFF, stop and delete the Timer.
            le_timer_Delete(timerRef);
        }
        else
        {
            le_mrc_SetRadioPower(LE_OFF);
            // TODO: find a solution to get off of this infinite loop
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Power Off the radio
 */
//--------------------------------------------------------------------------------------------------
static void StopCellularNetwork
(
    void
)
{
    le_onoff_t  radioState;
    le_result_t result;

    result=le_mrc_GetRadioPower(&radioState);
    if ((result == LE_OK) && (radioState == LE_OFF))
    {
        return;
    }
    else
    {
        // Try to shutdown the radio anyway
        le_mrc_SetRadioPower(LE_OFF);

        // Set a timer that gets the current position.
        le_timer_Ref_t stopCellNetTimer = le_timer_Create("StopCellNetTimer");
        le_clk_Time_t interval = {5, 0}; // 5 seconds

        if ( (le_timer_SetHandler(stopCellNetTimer, StopCellNetTimerHandler) != LE_OK) ||
             (le_timer_SetRepeat(stopCellNetTimer, 0) != LE_OK) ||
             (le_timer_SetInterval(stopCellNetTimer, interval) != LE_OK) ||
             (le_timer_Start(stopCellNetTimer) != LE_OK) )
        {
            LE_ERROR("Could not start the StopCellNet timer!");
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler to process a command
 */
//--------------------------------------------------------------------------------------------------
static void ProcessCommand
(
    void* reportPtr
)
{
    uint32_t    command = *(uint32_t*)reportPtr;

    LE_PRINT_VALUE("%i", command);

    if (command == REQUEST_COMMAND)
    {
        RequestCount++;
        StartCellularNetwork();
    }
    else if (command == RELEASE_COMMAND)
    {
        // Don't decrement below zero, as it will wrap-around.
        if ( RequestCount > 0 )
        {
            RequestCount--;
        }

        if (RequestCount == 0)
        {
            StopCellularNetwork();
        }
    }
    else
    {
        LE_ERROR("Command %i is not valid", command);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SIM States Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SimStateHandler
(
    le_sim_Id_t     simId,
    le_sim_States_t simState,
    void*           contextPtr
)
{
    if (LE_SIM_INSERTED == simState)
    {
        // SIM card inserted: load the configuration and notify the applications
        LoadSimFromSecStore(simId);
        GetAndSendCellNetStateEvent();
    }

    if ((LE_SIM_ABSENT == simState) || (LE_SIM_POWER_DOWN == simState))
    {
        // SIM card removed or powered down: notify the applications
        GetAndSendCellNetStateEvent();
    }
}

// -------------------------------------------------------------------------------------------------
/**
 *  Event callback for Cellular Network Registration state changes.
 */
// -------------------------------------------------------------------------------------------------
static void MrcNetRegHandler
(
    le_mrc_NetRegState_t state,
    void*  contextPtr
)
{
    le_cellnet_State_t cellNetState = TranslateToCellNetState(state);

    LE_DEBUG("MRC network state %d translated to Cellular network state %d (%s)",
             state, cellNetState, cellNetStateStr[cellNetState]);

    // Send the state event to applications
    ReportCellNetStateEvent(cellNetState);
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Connection State Handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerCellNetStateHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_cellnet_State_t* eventDataPtr = reportPtr;
    le_cellnet_StateHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(*eventDataPtr,
                      le_event_GetContextPtr());
}


//--------------------------------------------------------------------------------------------------
/**
 * This function adds a handler ...
 */
//--------------------------------------------------------------------------------------------------
le_cellnet_StateEventHandlerRef_t le_cellnet_AddStateEventHandler
(
    le_cellnet_StateHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_PRINT_VALUE("%p", handlerPtr);
    LE_PRINT_VALUE("%p", contextPtr);

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                    "CellNetState",
                                                    CellNetStateEvent,
                                                    FirstLayerCellNetStateHandler,
                                                    handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_cellnet_StateEventHandlerRef_t)(handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function removes a handler ...
 */
//--------------------------------------------------------------------------------------------------
void le_cellnet_RemoveStateEventHandler
(
    le_cellnet_StateEventHandlerRef_t addHandlerRef
)
{
    LE_PRINT_VALUE("%p", addHandlerRef);

    le_event_RemoveHandler((le_event_HandlerRef_t)addHandlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Request a cellular network
 *
 * @return Reference to the cellular network
 */
//--------------------------------------------------------------------------------------------------
le_cellnet_RequestObjRef_t le_cellnet_Request
(
    void
)
{
    uint32_t command = REQUEST_COMMAND;
    le_event_Report(CommandEvent, &command, sizeof(command));

    // Need to return a unique reference that will be used by Release.
    return le_ref_CreateRef(RequestRefMap, (void*)1);
}


//--------------------------------------------------------------------------------------------------
/**
 * Release a previously requested cellular network
 */
//--------------------------------------------------------------------------------------------------
void le_cellnet_Release
(
    le_cellnet_RequestObjRef_t requestRef ///< Reference to a previously requested cellular network
)
{
    // Look up the reference.  If it is NULL, then the reference is not valid.
    // Otherwise, delete the reference and send the release command to the CellNet thread.
    void* cellNetPtr = le_ref_Lookup(RequestRefMap, requestRef);
    if ( cellNetPtr == NULL )
    {
        LE_ERROR("Invalid cellular network request reference %p", requestRef);
    }
    else
    {
        LE_PRINT_VALUE("%p", requestRef);
        le_ref_DeleteRef(RequestRefMap, requestRef);

        uint32_t command = RELEASE_COMMAND;
        le_event_Report(CommandEvent, &command, sizeof(command));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the PIN code in the secure storage.
 *
 * @return
 *  - LE_OUT_OF_RANGE    Invalid simId
 *  - LE_FORMAT_ERROR    PIN code is not in string format.
 *  - LE_UNDERFLOW       The PIN code is not long enough (min 4 digits).
 *  - LE_OK              The function succeeded.
 *  - LE_FAULT           The function failed on any other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cellnet_SetSimPinCode
(
    le_sim_Id_t simId,
        ///< [IN]
        ///< SIM identifier.

    const char* pinCodePtr
        ///< [IN]
        ///< PIN code to insert in the secure storage.
)
{

    le_result_t result=LE_OK;
    size_t pinCodeLength = strlen(pinCodePtr);

    LE_DEBUG("simId= %d",simId);

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        result = LE_OUT_OF_RANGE;
    }
    else
    {
        //void entry is taken into account
        if (strncmp(pinCodePtr,"",LE_SIM_PIN_MAX_LEN)!=0)
        {
            if (pinCodeLength > LE_SIM_PIN_MAX_LEN)
            {
                LE_KILL_CLIENT("PIN code exceeds %d", LE_SIM_PIN_MAX_LEN);
                return LE_FAULT;
            }
            else if (pinCodeLength < LE_SIM_PIN_MIN_LEN)
            {
                LE_ERROR("SIM PIN code is not long enough (min 4 digits)");
                result = LE_UNDERFLOW;
            }
            else
            {
                // test SIM pincode format
                int i;
                bool test_ok = true;
                for (i=0; ((i<pinCodeLength) && test_ok); i++)
                {
                    if ((pinCodePtr[i] < 0x30) || (pinCodePtr[i] > 0x39))
                    {
                        test_ok = false;
                        break;
                    }
                }
                if (false == test_ok)
                {
                    LE_ERROR("SIM PIN code format error");
                    result = LE_FORMAT_ERROR;
                }
            }
        }
    }

    if (LE_OK == result)
    {
        // Set the secure storage path for the SIM
        char secStorePath[LE_SECSTORE_MAX_NAME_BYTES];
        snprintf(secStorePath, sizeof(secStorePath), "%s/%d/%s",
                 SECSTORE_NODE_SIM, simId, SECSTORE_NODE_PIN);

        result = le_secStore_Write(secStorePath, (uint8_t*)pinCodePtr,
                                   strnlen(pinCodePtr, LE_SIM_PIN_MAX_BYTES));

        if (LE_OK == result)
        {
            LE_DEBUG("SIM PIN code correctly inserted in secure storage");

            // New SIM pincode is taken into account
            LoadSimFromSecStore(simId);
            GetAndSendCellNetStateEvent();
        }
        else
        {
            LE_ERROR("Unable to store PIN code, error %s\n", LE_RESULT_TXT(result));
        }
    }

    return result;
}



//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the PIN code from the secure storage.
 *
 * @return
 *  - LE_OUT_OF_RANGE    Invalid simId
 *  - LE_NOT_FOUND       SIM PIN node isn't found in the secure storage.
 *  - LE_OVERFLOW        PIN code exceeds the maximum length of 8 digits.
 *  - LE_UNDERFLOW       The PIN code is not long enough (min 4 digits).
 *  - LE_OK              The function succeeded.
 *  - LE_FAULT           If there are some other errors.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cellnet_GetSimPinCode
(
    le_sim_Id_t simId,
        ///< [IN]
        ///< SIM identifier.

    char* pinCodePtr,
        ///< [OUT]
        ///< Read the PIN code from the secure storage.

    size_t pinCodeNumElements
        ///< [IN]

)
{
    if (pinCodePtr == NULL)
    {
        LE_KILL_CLIENT("pinCodePtr is NULL.");
        return LE_FAULT;
    }

    le_result_t result=LE_OK;

    LE_DEBUG("simId= %d",simId);

    if (simId >= LE_SIM_ID_MAX)
    {
        LE_ERROR("Invalid simId (%d) provided!", simId);
        result = LE_OUT_OF_RANGE;
    }
    else
    {
        // Set the secure storage path for the SIM
        char secStorePath[LE_SECSTORE_MAX_NAME_BYTES];
        snprintf(secStorePath, sizeof(secStorePath), "%s/%d/%s",
                 SECSTORE_NODE_SIM, simId, SECSTORE_NODE_PIN);

        char simPin[LE_SIM_PIN_MAX_BYTES] = {0};
        size_t simSize = LE_SIM_PIN_MAX_BYTES;

        // Read PIN code stored in secure storage
        result = le_secStore_Read(secStorePath, (uint8_t *)simPin, &simSize);

        if (LE_NOT_FOUND == result)
        {
            LE_ERROR("SIM PIN code isn't found in the secure storage");
        }
        else if (LE_OVERFLOW == result)
        {
            LE_ERROR("Retrieved SIM PIN code exceeds the supplied buffer");
        }
        else if (LE_OK != result)
        {
            LE_ERROR("Unable to retrieve PIN, error %s", LE_RESULT_TXT(result));
        }
        else
        {
            // void entry is taken into account
            if (   (0 != strncmp(simPin, "", LE_SIM_PIN_MAX_LEN))
                && (strlen(simPin) < LE_SIM_PIN_MIN_LEN))
            {
                LE_ERROR("Retrieved SIM PIN code is not long enough (min 4 digits)");
                result = LE_UNDERFLOW;
            }
            else
            {
                // Copy pincode
                result = le_utf8_Copy( pinCodePtr, simPin, LE_SIM_PIN_MAX_BYTES, NULL);
                if (result == LE_OK)
                {
                    LE_DEBUG("SIM PIN code retrieved OK");
                }
                else
                {
                    LE_DEBUG("SIM PIN code not retrieved: too long for buffer");
                }
            }
        }
    }
    return result;
}

// -------------------------------------------------------------------------------------------------
/**
 * Retrieve the current cellular network state.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAILED         The function failed
 *  - LE_BAD_PARAMETER  A bad parameter was passed.
 *
 * @note If the caller passes a null pointer to this function, this is a fatal error and the
 *       function will not return.
 */
// -------------------------------------------------------------------------------------------------
le_result_t le_cellnet_GetNetworkState
(
    le_cellnet_State_t* statePtr    ///< Cellular network state.
)
{
    if (statePtr == NULL)
    {
        LE_KILL_CLIENT("statePtr is NULL!");
        return LE_BAD_PARAMETER;
    }

    *statePtr = CurrentState;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Server Initialization
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Initialize the various events
    CommandEvent = le_event_CreateId("CellNet Command", sizeof(uint32_t));
    CellNetStateEvent = le_event_CreateId("CellNet State", sizeof(le_cellnet_State_t));

    // Create safe reference map for request references. The size of the map should be based on
    // the expected number of simultaneous cellular network requests, so take a reasonable guess.
    RequestRefMap = le_ref_CreateMap("CellNet Requests", 5);

    // Register for command events
    le_event_AddHandler("ProcessCommand",
                        CommandEvent,
                        ProcessCommand);

    // Register for SIM state changes
    le_sim_AddNewStateHandler(SimStateHandler, NULL);

    // Register for MRC Network Registration state changes
    le_mrc_AddNetRegStateEventHandler(MrcNetRegHandler, NULL);

    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);

    LE_INFO("Cellular Network Server is ready");
}

