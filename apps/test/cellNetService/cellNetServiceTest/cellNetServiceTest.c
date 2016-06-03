/**
 * This module implements the Cellular Network Application Service tests.
 *
 * Pin code HAS TO BE SET in the config tree before running the test.
 * Three possiblities:
 * use two arguments  : <1> <simId> to retrieve PIN CODE from config tree.
 * use tree arguments : <2> <simId> <PIN CODE> to insert the PIN CODE to the config tree,
 *                      the cellular network service test will run afterward.
 * without arguments  : Running cellular network service test (Pin code is already set)
 *
 * API Tested:
 *  - le_cellnet_SetSimPinCode()
 *  - le_cellnet_GetSimPinCode()
 *  - le_cellnet_AddStateEventHandler()
 *  - le_cellnet_Request()
 *  - le_cellnet_RemoveStateEventHandler()
 *  - le_cellnet_Release()
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

/* Legato Framework */
#include "legato.h"


/* Cellular Network Services (Client) */
#include "interfaces.h"

#define MAX_SIM_IDENTIFIERS    4

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
    if(RequestRef)
    {
        LE_ERROR("A cellular network request already exist.");
        return;
    }

    RequestRef = le_cellnet_Request();
    LE_INFO("Requesting the cellular network: %p.", RequestRef);
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
    if(!RequestRef)
    {
        LE_ERROR("Not existing cellular network reference.");
        LE_INFO("cellNetServiceTest FAILED");
        exit(EXIT_FAILURE);
    }

    le_cellnet_Release(RequestRef);
    LE_INFO("Releasing the cellular network. %p", RequestRef);
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
    LE_INFO("Cellular Network state is %d", state);

    if (state == LE_CELLNET_REG_HOME)
    {
        oldState = LE_CELLNET_REG_HOME;
        SwitchOffCellNet();
        LE_INFO("Verify that Cellular Network is OFF by checking CellNet events.");
    }

    if ((oldState == LE_CELLNET_REG_HOME) && (state == LE_CELLNET_REG_EMERGENCY))
    {
        LE_INFO("Cellular Network is OFF has been checked.");
        le_cellnet_RemoveStateEventHandler(StateHandlerRef);
        LE_INFO("StateHandlerRef (%p) removed", StateHandlerRef);
        LE_INFO("========  cellNetServiceTest TEST PASSED ======== ");
        exit(EXIT_SUCCESS);
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
        // Test the Get/Set SIM pin code operation in the config Tree
        // ---------------------------------------------------------------------
        int testId = atoi(le_arg_GetArg(0));
        int simId =  atoi(le_arg_GetArg(1));
        le_result_t ret;

        if (1 == testId)
        {
            char simPin[LE_SIM_PIN_MAX_BYTES];

            LE_INFO("========  Get existing PIN CODE ======== ");
            ret = le_cellnet_GetSimPinCode(simId, simPin, sizeof(simPin));
            LE_INFO("**** le_cellnet_GetSimPinCode ret =%d , pinCode = %s", ret, simPin);

            exit(EXIT_SUCCESS);
        }
        else if (2 == testId)
        {
            const char* pinPtr = le_arg_GetArg(2);
            // for caution; simId values greater than MAX_SIM_IDENTIFIERS are tracked in the function
            if (simId <= MAX_SIM_IDENTIFIERS)
            {
                simId = 1;
            }

            LE_INFO("========  Set PIN CODE  simId (%d) pinCode (%s) ======", simId, pinPtr);
            ret = le_cellnet_SetSimPinCode(simId, pinPtr);
            LE_INFO(" ********** le_cellnet_SetSimPinCode ret =%d", ret);
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

    exit(EXIT_SUCCESS);
}