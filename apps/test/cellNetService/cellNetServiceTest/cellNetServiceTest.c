/**
 * This module implements the Cellular Network Application Service tests.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 * license.
 *
 */

/* Legato Framework */
#include "legato.h"


/* Cellular Network Services (Client) */
#include "interfaces.h"


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


/*
 * Pin code HAVE TO BE SET in the config db.
 * config set /modemServices/sim/1/pin <PIN>
 **/
/*
 * This test gets the Target Hardware Platform information and it displays it in the log and in the shell.
 *
 * API Tested:
 *  - le_cellnet_AddStateEventHandler()
 *  - le_cellnet_Request()
 *  - le_cellnet_RemoveStateEventHandler()
 *  - le_cellnet_Release()
 */

// -------------------------------------------------------------------------------------------------
/**
 *  Test main function.
 */
// -------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("========  Running cellular network service test ======== ");

    // Register handler for cellular network state change
    StateHandlerRef = le_cellnet_AddStateEventHandler(CellNetStateHandler, NULL);
    LE_INFO("CellNetStateHandler added %p", StateHandlerRef);

    SwitchOnCellNet();
    LE_INFO("Verify that Cellular Network is ON by checking CellNet events.");
}

