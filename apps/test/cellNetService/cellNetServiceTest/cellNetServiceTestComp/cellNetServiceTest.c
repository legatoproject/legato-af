/**
 * This module implements the Cellular Network Application Service tests.
 *
 * Pin code HAS TO BE SET in the secure storage before running the test.
 *
 * Four tests can be run with this application:
 * - Get PIN code: launch the application with arguments "1 <simId>" to retrieve PIN code from
 *                 secure storage.
 * - Set PIN code: launch the application with arguments "2 <simId> <PIN CODE>" to insert the PIN
 *                 code in the secure storage, the cellular network service test will run afterwards.
 * - Basic test:   launch the application without arguments to run cellular network service test,
 *                 PIN code being already set.
 * - SIM removal:  launch the application with arguments "3 <simId>" to test SIM detection removal
 *                 and insertion. Tested platform should support hot-swap for this test.
 *
 * The tests can be launched with:
 *  app runProc cellNetServiceTest --exe=cellNetServiceTest -- <testId> <simId> [<PIN>]
 *
 * API Tested:
 *  - le_cellnet_SetSimPinCode()
 *  - le_cellnet_GetSimPinCode()
 *  - le_cellnet_AddStateEventHandler()
 *  - le_cellnet_Request()
 *  - le_cellnet_RemoveStateEventHandler()
 *  - le_cellnet_Release()
 *  - le_cellnet_GetNetworkState()
 *
 * Copyright (C) Sierra Wireless Inc.
 */

/* Legato Framework */
#include "legato.h"


/* Cellular Network Services (Client) */
#include "interfaces.h"

// -------------------------------------------------------------------------------------------------
/**
 *  Safety: maximal number of SIM identifiers
 */
// -------------------------------------------------------------------------------------------------
#define MAX_SIM_IDENTIFIERS    4

// -------------------------------------------------------------------------------------------------
/**
 *  Test identifiers
 */
// -------------------------------------------------------------------------------------------------
#define TEST_GET_PIN    1
#define TEST_SET_PIN    2
#define TEST_NO_SIM     3

// -------------------------------------------------------------------------------------------------
/**
 *  The cellular network reference
 */
// -------------------------------------------------------------------------------------------------
static le_cellnet_RequestObjRef_t RequestRef = NULL;

// -------------------------------------------------------------------------------------------------
/**
 *  cellNet State handler reference
 */
// -------------------------------------------------------------------------------------------------

static le_cellnet_StateEventHandlerRef_t StateHandlerRef = NULL;

// -------------------------------------------------------------------------------------------------
/**
 *  This function will request the default cellular network
 */
// -------------------------------------------------------------------------------------------------
static void SwitchOnCellNet
(
    void
)
{
    if (RequestRef)
    {
        LE_ERROR("A cellular network request already exist.");
        exit(EXIT_FAILURE);
    }

    LE_INFO("Requesting the cellular network.");
    RequestRef = le_cellnet_Request();
    LE_INFO("Received reference: %p.", RequestRef);
}

// -------------------------------------------------------------------------------------------------
/**
 *  The opposite of SwitchOnCellNet, this function will tear down the cellular network.
 */
// -------------------------------------------------------------------------------------------------
static void SwitchOffCellNet
(
    void
)
{
    if (!RequestRef)
    {
        LE_ERROR("Not existing cellular network reference.");
        LE_INFO("cellNetServiceTest FAILED");
        exit(EXIT_FAILURE);
    }

    LE_INFO("Releasing the cellular network. %p", RequestRef);
    le_cellnet_Release(RequestRef);
    RequestRef = NULL;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Event callback for cellular network state changes.
 */
// -------------------------------------------------------------------------------------------------
static void CellNetStateHandler
(
    le_cellnet_State_t state,
    void*              contextPtr
)
{
    static le_cellnet_State_t oldState = LE_CELLNET_REG_UNKNOWN;
    const char* testNoSimPtr = le_arg_GetArg(0);
    if (NULL == testNoSimPtr)
    {
        LE_ERROR("testNoSimPtr is NULL");
        exit(EXIT_FAILURE);
    }
    // Get current network state to test GetNetworkState API.
    // Note: received and current state might differ if the state changed
    // between the report sending and its treatment by the test application.
    le_cellnet_State_t getState = LE_CELLNET_REG_UNKNOWN;
    LE_ASSERT_OK(le_cellnet_GetNetworkState(&getState));

    LE_INFO("Received Cellular Network state is %d", state);
    LE_INFO("Current Cellular Network state is %d", getState);

    if (   (LE_CELLNET_REG_HOME == state)
        || (LE_CELLNET_REG_ROAMING == state)
       )
    {
        if (   (le_arg_NumArgs() >= 2)
            && (TEST_NO_SIM == atoi(testNoSimPtr))
            && (LE_CELLNET_SIM_ABSENT != oldState)
           )
        {
            LE_INFO("========  Remove SIM card ======== ");
            LE_INFO("Verify that removal is detected.");
        }
        else
        {
            SwitchOffCellNet();
            LE_INFO("Verify that Cellular Network is OFF by checking CellNet events.");
        }

        oldState = state;
    }

    if (   (   (LE_CELLNET_REG_HOME == oldState)
            || (LE_CELLNET_REG_ROAMING == oldState)
           )
        && (LE_CELLNET_RADIO_OFF == state)
       )
    {
        LE_INFO("Cellular Network is OFF has been checked.");
        le_cellnet_RemoveStateEventHandler(StateHandlerRef);
        LE_INFO("StateHandlerRef (%p) removed", StateHandlerRef);
        LE_INFO("========  cellNetServiceTest TEST PASSED ======== ");
        exit(EXIT_SUCCESS);
    }

    if (   (le_arg_NumArgs() >= 2)
        && (TEST_NO_SIM == atoi(testNoSimPtr))
        && (   (LE_CELLNET_REG_HOME == oldState)
            || (LE_CELLNET_REG_ROAMING == oldState)
           )
        && (LE_CELLNET_SIM_ABSENT == state)
       )
    {
        oldState = state;
        LE_INFO("SIM removal detection has been checked.");
        LE_INFO("========  Insert SIM card ======== ");
    }
}


// -------------------------------------------------------------------------------------------------
/**
 *  Test main function.
 */
// -------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("========  cellNetServiceTest starts ======== ");

    if (le_arg_NumArgs() >= 2)
    {
        // ---------------------------------------------------------------------
        // Test the Get/Set SIM pin code operation in the secure storage
        // ---------------------------------------------------------------------
        const char* testIdPtr = le_arg_GetArg(0);
        const char* simIdPtr  = le_arg_GetArg(1);
        if (NULL == testIdPtr)
        {
            LE_ERROR("testIdPtr is NULL");
            exit(EXIT_FAILURE);
        }
        if (NULL == simIdPtr)
        {
            LE_ERROR("simIdPtr is NULL");
            exit(EXIT_FAILURE);
        }
        int testId = atoi(testIdPtr);
        int simId =  atoi(simIdPtr);
        le_result_t ret;

        if (TEST_GET_PIN == testId)
        {
            char simPin[LE_SIM_PIN_MAX_BYTES];

            LE_INFO("========  Get existing PIN CODE ======== ");
            ret = le_cellnet_GetSimPinCode(simId, simPin, sizeof(simPin));
            LE_INFO("**** le_cellnet_GetSimPinCode ret =%d , pinCode = %s", ret, simPin);

            exit(EXIT_SUCCESS);
        }
        else if (TEST_SET_PIN == testId)
        {
            const char* pinPtr = le_arg_GetArg(2);
            if (NULL == pinPtr)
            {
                LE_ERROR("pinPtr is NULL");
                exit(EXIT_FAILURE);
            }
            // simId values greater than MAX_SIM_IDENTIFIERS are tracked in the function
            if (simId <= MAX_SIM_IDENTIFIERS)
            {
                simId = 1;
            }

            LE_INFO("========  Set PIN CODE simId (%d) pinCode (%s) ======", simId, pinPtr);
            ret = le_cellnet_SetSimPinCode(simId, pinPtr);
            LE_INFO(" ********** le_cellnet_SetSimPinCode ret =%d", ret);
        }
        else if (TEST_NO_SIM == testId)
        {
            LE_INFO("========  Test SIM removal detection (hot-swap support necessary) ======== ");
        }
        else
        {
            LE_INFO("Bad test case");
            exit(EXIT_FAILURE);
        }
    }
    else if (le_arg_NumArgs() == 1)
    {
        LE_INFO("Bad test arguments");
        exit(EXIT_FAILURE);
    }

    LE_INFO("========  Running cellular network service test ======== ");
    // Register handler for cellular network state change
    StateHandlerRef = le_cellnet_AddStateEventHandler(CellNetStateHandler, NULL);

    LE_INFO("CellNetStateHandler added %p", StateHandlerRef);
    SwitchOnCellNet();
    LE_INFO("Verify that Cellular Network is ON by checking CellNet events.");
}
