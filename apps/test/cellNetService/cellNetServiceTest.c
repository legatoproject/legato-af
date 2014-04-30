/**
 * This module implements the Cellular Network Application Service tests.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014.  All rights reserved. Use of this work is subject to
 * license.
 *
 */

/* Legato Framework */
#include "legato.h"


/* Cellular Network Services (Client) */
#include "le_cellnet_interface.h"




// -------------------------------------------------------------------------------------------------
/**
 *  The cellular network reference
 */
// -------------------------------------------------------------------------------------------------
static le_cellnet_RequestObjRef_t RequestRef = NULL;


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
        return;
    }

    le_cellnet_Release(RequestRef);
    LE_INFO("Releasing the cellular network.");

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
    LE_INFO("Cellular Network state is %d", state);

    if (state == LE_CELLNET_REG_HOME)
    {
        SwitchOffCellNet();

        LE_INFO("Verify that Cellular Network is OFF by checking CellNet events.");
    }
}

// -------------------------------------------------------------------------------------------------
/**
 *  Main test thread.
 */
// -------------------------------------------------------------------------------------------------
static void* TestCellNetService
(
    void* contextPtr
)
{
    SwitchOnCellNet();

    LE_INFO("Verify that Cellular Network is ON by checking CellNet events.");

    // Run the event loop
    le_event_RunLoop();
    return NULL;
}


// -------------------------------------------------------------------------------------------------
/**
 *  Test main function.
 */
// -------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Running cellular network service test");

    // register handler for cellular network state change
    le_cellnet_AddStateHandler(CellNetStateHandler, NULL);

    le_thread_Start(le_thread_Create("TestCellNetService", TestCellNetService, NULL));
}

