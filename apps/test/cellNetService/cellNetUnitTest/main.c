/**
 * This module implements the unit tests for the Cellular Network service.
 *
 * Tested API:
 * - le_cellnet_GetSimPinCode
 * - le_cellnet_SetSimPinCode
 * - le_cellnet_AddStateEventHandler
 * - le_cellnet_Request
 * - le_cellnet_GetNetworkState
 * - le_cellnet_Release
 * - le_cellnet_RemoveStateEventHandler
 *
 * Unit test steps:
 *  1. Test SIM PIN configuration through Cellular Network API
 *      a. Test le_cellnet_GetSimPinCode and all error cases
 *      b. Test le_cellnet_SetSimPinCode and all error cases
 *  2. Test Cellular Network service
 *      a. Add application handlers to be notified of cellular network events
 *      b. Several applications request the cellular network
 *      c. All possible MRC events are simulated, triggering cellular network events
 *      d. SIM removal and insertion is simulated, triggering cellular network events
 *      e. The applications release the cellular network
 *      f. Radio off cellular event is simulated
 *      g. Cellular network events handler are removed
 *      h. Simulate a cellular network event to check that handlers are removed
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Number of clients for the cellular network service
 */
//--------------------------------------------------------------------------------------------------
#define CLIENTS_NB  2

//--------------------------------------------------------------------------------------------------
/**
 *  Cellular network state string size
 */
//--------------------------------------------------------------------------------------------------
#define STATE_STR_MAX_SIZE  30

//--------------------------------------------------------------------------------------------------
/**
 *  Short semaphore timeout in seconds
 */
//--------------------------------------------------------------------------------------------------
#define SHORT_TIMEOUT   1

//--------------------------------------------------------------------------------------------------
/**
 *  Long semaphore timeout in seconds
 */
//--------------------------------------------------------------------------------------------------
#define LONG_TIMEOUT    5

//--------------------------------------------------------------------------------------------------
/**
 *  Expected cellular network state
 */
//--------------------------------------------------------------------------------------------------
static le_cellnet_State_t ExpectedCellNetState = LE_CELLNET_REG_UNKNOWN;

// -------------------------------------------------------------------------------------------------
/**
 *  Application context structure
 */
// -------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t                            appId;
    le_sem_Ref_t                        appSemaphore;
    le_thread_Ref_t                     appThreadRef;
    le_cellnet_StateEventHandlerRef_t   appStateHandlerRef;
    le_cellnet_RequestObjRef_t          appRequestRef;
} AppContext_t;

// -------------------------------------------------------------------------------------------------
/**
 *  Application contexts
 */
// -------------------------------------------------------------------------------------------------
static AppContext_t AppCtx[CLIENTS_NB];


//--------------------------------------------------------------------------------------------------
// Utility functions
//--------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------
/**
 *  Convert cellular network state to a string
 */
// -------------------------------------------------------------------------------------------------
static void CellNetStateStr
(
    le_cellnet_State_t state,
    char* stateStrPtr,
    size_t stateStrLen
)
{
    switch (state)
    {
        case LE_CELLNET_RADIO_OFF:
            snprintf(stateStrPtr, stateStrLen, "Radio off");
            break;

        case LE_CELLNET_REG_EMERGENCY:
            snprintf(stateStrPtr, stateStrLen, "Emergency");
            break;

        case LE_CELLNET_REG_HOME:
            snprintf(stateStrPtr, stateStrLen, "Home network");
            break;

        case LE_CELLNET_REG_ROAMING:
            snprintf(stateStrPtr, stateStrLen, "Roaming");
            break;

        case LE_CELLNET_SIM_ABSENT:
            snprintf(stateStrPtr, stateStrLen, "SIM absent");
            break;

        case LE_CELLNET_REG_UNKNOWN:
        default:
            snprintf(stateStrPtr, stateStrLen, "Unknown state");
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Synchronize test thread (i.e. main) and application threads
 */
//--------------------------------------------------------------------------------------------------
static void SynchronizeTest
(
    void
)
{
    int i = 0;
    le_clk_Time_t timeToWait = {LONG_TIMEOUT, 0};

    for (i = 0; i < CLIENTS_NB; i++)
    {
        LE_ASSERT_OK(le_sem_WaitWithTimeOut(AppCtx[i].appSemaphore, timeToWait));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Simulate network registration state
 */
//--------------------------------------------------------------------------------------------------
static void SimulateMrcStateAndSetExpectedCellNetState
(
    le_mrc_NetRegState_t netRegState,
    le_cellnet_State_t   cellNetState
)
{
    LE_DEBUG("Simulate MRC state %d", netRegState);
    ExpectedCellNetState = cellNetState;
    le_mrcTest_SimulateState(netRegState);
}

//--------------------------------------------------------------------------------------------------
/**
 * Simulate SIM state
 */
//--------------------------------------------------------------------------------------------------
static void SimulateSimStateAndSetExpectedCellNetState
(
    le_sim_Id_t        simId,
    le_sim_States_t    simState,
    le_cellnet_State_t cellNetState
)
{
    LE_DEBUG("Simulate state %d for SIM %d", simState, simId);
    if (LE_SIM_ABSENT == simState)
    {
        le_simTest_SetPresent(simId, false);
    }
    else
    {
        le_simTest_SetPresent(simId, true);
    }
    ExpectedCellNetState = cellNetState;
    le_simTest_SimulateState(simId, simState);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Request cellular network
 */
// -------------------------------------------------------------------------------------------------
static void CellNetRequest
(
    void* param1Ptr,
    void* param2Ptr
)
{
    AppContext_t* appCtxPtr = (AppContext_t*) param1Ptr;

    // Request cellular network
    LE_INFO("Request of cellular network by application #%d", appCtxPtr->appId);
    appCtxPtr->appRequestRef = le_cellnet_Request();
    LE_ASSERT(NULL != appCtxPtr->appRequestRef);
    LE_INFO("Received reference: %p", appCtxPtr->appRequestRef);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Release cellular network
 */
// -------------------------------------------------------------------------------------------------
static void CellNetRelease
(
    void* param1Ptr,
    void* param2Ptr
)
{
    AppContext_t* appCtxPtr = (AppContext_t*) param1Ptr;

    // Release cellular network
    LE_INFO("Release of cellular network by application #%d", appCtxPtr->appId);
    LE_INFO("Releasing the cellular network reference %p", appCtxPtr->appRequestRef);
    le_cellnet_Release(appCtxPtr->appRequestRef);
    appCtxPtr->appRequestRef = NULL;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Remove cellular network handler
 */
// -------------------------------------------------------------------------------------------------
static void CellNetRemoveHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    AppContext_t* appCtxPtr = (AppContext_t*) param1Ptr;

    // Deregister handler
    le_cellnet_RemoveStateEventHandler(appCtxPtr->appStateHandlerRef);
    LE_INFO("CellNetStateHandler %p removed for application #%d",
             appCtxPtr->appStateHandlerRef, appCtxPtr->appId);
    appCtxPtr->appStateHandlerRef = NULL;

    le_sem_Post(appCtxPtr->appSemaphore);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Event callback for cellular network state changes
 */
// -------------------------------------------------------------------------------------------------
static void CellNetStateHandler
(
    le_cellnet_State_t state,
    void*              contextPtr
)
{
    AppContext_t* appCtxPtr = (AppContext_t*) contextPtr;
    char receivedStateStr[STATE_STR_MAX_SIZE];
    char expectedStateStr[STATE_STR_MAX_SIZE];
    char currentStateStr[STATE_STR_MAX_SIZE];

    // Get current network state to test GetNetworkState API.
    le_cellnet_State_t getState = LE_CELLNET_REG_UNKNOWN;
    LE_ASSERT_OK(le_cellnet_GetNetworkState(&getState));

    CellNetStateStr(state, receivedStateStr, sizeof(receivedStateStr));
    CellNetStateStr(ExpectedCellNetState, expectedStateStr, sizeof(expectedStateStr));
    CellNetStateStr(getState, currentStateStr, sizeof(currentStateStr));

    LE_INFO("Received Cellular Network state is %d (%s)", state, receivedStateStr);
    LE_DEBUG("Expected Cellular Network state is %d (%s)", ExpectedCellNetState, expectedStateStr);
    LE_DEBUG("Current Cellular Network state is %d (%s)", getState, currentStateStr);

    // Check if the current state matches the expected state
    LE_ASSERT(ExpectedCellNetState == state);
    LE_ASSERT(getState == state);

    le_sem_Post(appCtxPtr->appSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Thread used to simulate an application
 */
//--------------------------------------------------------------------------------------------------
static void* AppHandler
(
    void* ctxPtr
)
{
    AppContext_t* appCtxPtr = (AppContext_t*) ctxPtr;

    // Register handler for cellular network state change
    appCtxPtr->appStateHandlerRef = le_cellnet_AddStateEventHandler(CellNetStateHandler, ctxPtr);
    LE_ASSERT(NULL != appCtxPtr->appStateHandlerRef);
    LE_INFO("CellNetStateHandler %p added for application #%d",
             appCtxPtr->appStateHandlerRef, appCtxPtr->appId);

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(appCtxPtr->appSemaphore);

    // Run the event loop
    le_event_RunLoop();

    return NULL;
}


//--------------------------------------------------------------------------------------------------
// Test functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Set and Get SIM PIN
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void Testle_cellnet_PIN
(
    void
)
{
    le_sim_Id_t simId;
    char simPin[LE_SIM_PIN_MAX_BYTES];

    // In these tests, SIM id is used to discriminate the cases and create all possible error cases.
    // LE_SIM_REMOTE is the case where everything is OK.

    LE_INFO("-------- Get SIM PIN --------");

    // Invalid SIM id
    simId = LE_SIM_ID_MAX;
    LE_ASSERT(LE_OUT_OF_RANGE == le_cellnet_GetSimPinCode(simId, simPin, sizeof(simPin)));

    // PIN code not found
    simId = LE_SIM_EMBEDDED;
    LE_ASSERT(LE_NOT_FOUND == le_cellnet_GetSimPinCode(simId, simPin, sizeof(simPin)));

    // Buffer too small
    simId = LE_SIM_EXTERNAL_SLOT_1;
    LE_ASSERT(LE_OVERFLOW == le_cellnet_GetSimPinCode(simId, simPin, sizeof(simPin)));

    // PIN code too short
    simId = LE_SIM_EXTERNAL_SLOT_2;
    LE_ASSERT(LE_UNDERFLOW == le_cellnet_GetSimPinCode(simId, simPin, sizeof(simPin)));

    // PIN code successfully retrieved
    simId = LE_SIM_REMOTE;
    LE_ASSERT_OK(le_cellnet_GetSimPinCode(simId, simPin, sizeof(simPin)));
    LE_DEBUG("le_cellnet_GetSimPinCode: pinCode = %s", simPin);
    LE_ASSERT(0 == strncmp(simPin, "00112233", sizeof(simPin)));


    LE_INFO("-------- Set SIM PIN --------");

    // Successfully setting SIM PIN triggers a notification of the current cellular network status,
    // add an application handler to catch it
    memset(AppCtx, 0, CLIENTS_NB * sizeof(AppContext_t));
    AppCtx[0].appId = 0;
    AppCtx[0].appSemaphore = le_sem_Create("pinSem", 0);
    AppCtx[0].appThreadRef = le_thread_Create("pinHandler", AppHandler, &AppCtx[0]);
    le_thread_SetJoinable(AppCtx[0].appThreadRef);
    le_thread_Start(AppCtx[0].appThreadRef);
    // Wait for thread start
    le_clk_Time_t timeToWait = {LONG_TIMEOUT, 0};
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(AppCtx[0].appSemaphore, timeToWait));

    // Invalid SIM id
    simId = LE_SIM_ID_MAX;
    LE_ASSERT(LE_OUT_OF_RANGE == le_cellnet_SetSimPinCode(simId, simPin));

    // PIN code too long
    simId = LE_SIM_EMBEDDED;
    const char simPinTooLong[]  = "123456789";
    LE_ASSERT(LE_FAULT == le_cellnet_SetSimPinCode(simId, simPinTooLong));

    // PIN code too short
    const char simPinTooShort[] = "12";
    LE_ASSERT(LE_UNDERFLOW == le_cellnet_SetSimPinCode(simId, simPinTooShort));

    // Wrong characters in PIN code
    snprintf(simPin, sizeof(simPin), "123A");
    LE_ASSERT(LE_FORMAT_ERROR == le_cellnet_SetSimPinCode(simId, simPin));

    // PIN code unsuccessfully stored
    snprintf(simPin, sizeof(simPin), "1234");
    LE_ASSERT(LE_NO_MEMORY == le_cellnet_SetSimPinCode(simId, simPin));

    // PIN code successfully stored
    // As no SIM is inserted, a SIM_ABSENT notification should be received
    ExpectedCellNetState = LE_CELLNET_SIM_ABSENT;
    simId = LE_SIM_REMOTE;
    LE_ASSERT(LE_OK == le_cellnet_SetSimPinCode(simId, simPin));

    // Wait for the notification
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(AppCtx[0].appSemaphore, timeToWait));

    // Deregister handler
    le_event_QueueFunctionToThread(AppCtx[0].appThreadRef, CellNetRemoveHandler, &AppCtx[0], NULL);
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(AppCtx[0].appSemaphore, timeToWait));
    // Stop the thread and delete the semaphore
    le_sem_Delete(AppCtx[0].appSemaphore);
    le_thread_Cancel(AppCtx[0].appThreadRef);
    le_thread_Join(AppCtx[0].appThreadRef, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test cellular network service
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void Testle_cellnet_Service
(
    void
)
{
    int i;

    // Initialize application contexts
    memset(AppCtx, 0, CLIENTS_NB * sizeof(AppContext_t));

    // Start threads in order to simulate multi-users of cellular network service
    for (i = 0; i < CLIENTS_NB; i++)
    {
        char threadString[20] = {0};
        char semString[20]    = {0};
        snprintf(threadString, 20, "app%dHandler", i);
        snprintf(semString, 20, "app%dSem", i);
        AppCtx[i].appId = i;
        // Create a semaphore to coordinate the test
        AppCtx[i].appSemaphore = le_sem_Create(semString, 0);
        AppCtx[i].appThreadRef = le_thread_Create(threadString, AppHandler, &AppCtx[i]);
        le_thread_Start(AppCtx[i].appThreadRef);
    }

    // Wait for threads start
    SynchronizeTest();

    // Indicate that radio is powered on for test purpose
    le_mrc_SetRadioPower(LE_ON);
    // Indicate that SIM is present for test purpose
    // As LE_SIM_REMOTE is the case where everything is OK for the PIN tests,
    // this SIM id is used before requesting the cellular network
    le_sim_SelectCard(LE_SIM_REMOTE);
    le_simTest_SetPresent(LE_SIM_REMOTE, true);

    // After requesting the cellular network, the cellular network state should be emergency state
    ExpectedCellNetState = LE_CELLNET_REG_EMERGENCY;
    // Each application requests the cellular network: the API has therefore to be called
    // by the application threads
    for (i = 0; i < CLIENTS_NB; i++)
    {
        le_event_QueueFunctionToThread(AppCtx[i].appThreadRef, CellNetRequest, &AppCtx[i], NULL);

        // Wait for the handlers call
        SynchronizeTest();
    }

    // Simulate all possible MRC states
    SimulateMrcStateAndSetExpectedCellNetState(LE_MRC_REG_NONE, LE_CELLNET_REG_EMERGENCY);
    SynchronizeTest();
    SimulateMrcStateAndSetExpectedCellNetState(LE_MRC_REG_SEARCHING, LE_CELLNET_REG_EMERGENCY);
    SynchronizeTest();
    SimulateMrcStateAndSetExpectedCellNetState(LE_MRC_REG_DENIED, LE_CELLNET_REG_EMERGENCY);
    SynchronizeTest();
    SimulateMrcStateAndSetExpectedCellNetState(LE_MRC_REG_HOME, LE_CELLNET_REG_HOME);
    SynchronizeTest();
    SimulateMrcStateAndSetExpectedCellNetState(LE_MRC_REG_ROAMING, LE_CELLNET_REG_ROAMING);
    SynchronizeTest();
    SimulateMrcStateAndSetExpectedCellNetState(LE_MRC_REG_UNKNOWN, LE_CELLNET_REG_UNKNOWN);
    SynchronizeTest();

    // All MRC states are now simulated, simulate SIM removal
    SimulateSimStateAndSetExpectedCellNetState(LE_SIM_REMOTE, LE_SIM_ABSENT,
                                               LE_CELLNET_SIM_ABSENT);
    SynchronizeTest();
    // Simulate SIM insertion after SIM removal
    SimulateSimStateAndSetExpectedCellNetState(LE_SIM_REMOTE, LE_SIM_INSERTED,
                                               LE_CELLNET_REG_EMERGENCY);
    SynchronizeTest();

    // Indicate that radio is powered off for test purpose
    le_mrc_SetRadioPower(LE_OFF);

    // Each application releases the cellular network: the API has therefore to be called
    // by the application threads
    for (i = 0; i < CLIENTS_NB; i++)
    {
        le_event_QueueFunctionToThread(AppCtx[i].appThreadRef, CellNetRelease, &AppCtx[i], NULL);
    }

    // Simulate a Radio Off event
    SimulateMrcStateAndSetExpectedCellNetState(LE_MRC_REG_NONE, LE_CELLNET_RADIO_OFF);
    SynchronizeTest();

    // Each application removes the cellular network handler: the API has therefore to be called
    // by the application threads
    for (i = 0; i < CLIENTS_NB; i++)
    {
        le_event_QueueFunctionToThread(AppCtx[i].appThreadRef, CellNetRemoveHandler,
                                       &AppCtx[i], NULL);
    }
    SynchronizeTest();

    // Simulate a new Radio Off event
    SimulateMrcStateAndSetExpectedCellNetState(LE_MRC_REG_NONE, LE_CELLNET_RADIO_OFF);
    // Wait for the semaphore timeout to check that handlers are not called
    le_clk_Time_t timeToWait = {SHORT_TIMEOUT, 0};
    for (i = 0; i < CLIENTS_NB; i++)
    {
        LE_ASSERT(LE_TIMEOUT == le_sem_WaitWithTimeOut(AppCtx[i].appSemaphore, timeToWait));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This thread is used to launch the cellular network unit tests
 */
//--------------------------------------------------------------------------------------------------
static void* CellNetUnitTestThread
(
    void* contextPtr
)
{
    LE_INFO("CellNet UT Thread Started");

    LE_INFO("======== Test SIM PIN through CellNet ========");
    Testle_cellnet_PIN();

    LE_INFO("======== Test CellNet service ========");
    Testle_cellnet_Service();

    LE_INFO("======== Test CellNet success! ========");
    exit(EXIT_SUCCESS);

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Main of the test
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // To reactivate for all DEBUG logs
//    le_log_SetFilterLevel(LE_LOG_DEBUG);

    LE_INFO("======== Start UnitTest of Cellular Network service ========");

    // Start the unit test thread
    le_thread_Start(le_thread_Create("CellNet UT Thread", CellNetUnitTestThread, NULL));
}
