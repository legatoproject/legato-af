/**
 * This module implements the Data Connection Service tests.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 *
 */

/* Legato Framework */
#include "legato.h"


/* data Connection Services (Client) */
#include "le_data_interface.h"




//--------------------------------------------------------------------------------------------------
/**
 *  The Data Connection reference
 */
//--------------------------------------------------------------------------------------------------
static le_data_RequestObjRef_t RequestRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 *  This function will request the data connection
 */
//--------------------------------------------------------------------------------------------------
static void ConnectData
(
    void
)
{
    if(RequestRef)
    {
        LE_ERROR("A data connection request already exist.");
        return;
    }

    RequestRef = le_data_Request();
    LE_INFO("Requesting the data connection: %p.", RequestRef);
}

//--------------------------------------------------------------------------------------------------
/**
 *  The opposite of ConnectData, this function will tear down the data connection.
 */
//--------------------------------------------------------------------------------------------------
static void DisconnectData
(
    void
)
{
    if(!RequestRef)
    {
        LE_ERROR("Not existing data connection reference.");
        return;
    }

    LE_INFO("Releasing the data connection.");
    le_data_Release(RequestRef);

    RequestRef = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Event callback for data connection state changes.
 */
//--------------------------------------------------------------------------------------------------
static void DcsStateHandler
(
    const char* intfName,
    bool isConnected,
    void* contextPtr
)
{
    if (isConnected)
    {
        LE_INFO("%s connected! Wait for 5 seconds before releasing the data connection.", intfName);
        sleep(5);
        DisconnectData();
        LE_INFO("Verify that the data connection is released by checking DCS events.");
    }
    else
    {
        LE_INFO("%s disconnected!", intfName);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Main test thread.
 */
//--------------------------------------------------------------------------------------------------
static void* TestDcs
(
    void* contextPtr
)
{
    ConnectData();

    LE_INFO("Verify that the Data connection is established by checking DCS events.");

    // Run the event loop
    le_event_RunLoop();
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 *  Test main function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Running data connection service test");

    // register handler for data connection state change
    le_data_AddConnectionStateHandler(DcsStateHandler, NULL);

    le_thread_Start(le_thread_Create("TestDCS", TestDcs, NULL));
}

