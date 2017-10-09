/**
 * This module implements the Data Connection Service tests:
 * - Define the technologies to use for the default data connection
 * - Check if the technologies are correctly added to the list of technologies to use
 * - Start the default data connection
 * - Check with connection status notifications if the data connection is established
 * - Close the default data connection
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

/* Legato Framework */
#include "legato.h"

/* data Connection Services (Client) */
#include "le_data_interface.h"

//--------------------------------------------------------------------------------------------------
/**
 * The technology string max length
 */
//--------------------------------------------------------------------------------------------------
#define TECH_MAX_LEN        16
#define TECH_MAX_BYTES      (TECH_MAX_LEN+1)

//--------------------------------------------------------------------------------------------------
/**
 *  The Data Connection reference
 */
//--------------------------------------------------------------------------------------------------
static le_data_RequestObjRef_t RequestRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 *  List of technologies to use
 */
//--------------------------------------------------------------------------------------------------
static le_data_Technology_t TechList[LE_DATA_MAX] = {};

//--------------------------------------------------------------------------------------------------
/**
 *  List of technology strings
 */
//--------------------------------------------------------------------------------------------------
static const char techDico[LE_DATA_MAX][TECH_MAX_LEN] = {
    "wifi",
    "cellular"
};

//--------------------------------------------------------------------------------------------------
/**
 *  This function will set the technologies to use
 */
//--------------------------------------------------------------------------------------------------
static void SetTechnologies
(
    void
)
{
    int techCounter = 0;

    LE_INFO("Setting the technologies to use for the data connection.");

    // Add 'Wifi' as the first technology to use
    le_result_t wifiResult = le_data_SetTechnologyRank(1, LE_DATA_WIFI);
    if (LE_OK == wifiResult)
    {
        TechList[techCounter++] = LE_DATA_WIFI;
    }
    else if (LE_UNSUPPORTED == wifiResult)
    {
        LE_INFO("Wifi not available on this platform.");
    }
    else
    {
        LE_ERROR("Error %d when adding wifi to the list of technologies to use.", wifiResult);
        exit(EXIT_FAILURE);
    }

    // Add 'Cellular' as the second technology to use
    LE_ASSERT(LE_OK == le_data_SetTechnologyRank(2, LE_DATA_CELLULAR));
    TechList[techCounter++] = LE_DATA_CELLULAR;
}

//--------------------------------------------------------------------------------------------------
/**
 *  This function will get the technologies to use
 */
//--------------------------------------------------------------------------------------------------
static void CheckTechnologies
(
    void
)
{
    int techCounter = 0;

    LE_INFO("Checking the technologies to use for the data connection.");

    // Get first technology to use
    le_data_Technology_t technology = le_data_GetFirstUsedTechnology();

    if (technology != TechList[techCounter])
    {
        LE_ERROR("Unexpected first technology %d, should be %d.", technology, TechList[techCounter]);
        exit(EXIT_FAILURE);
    }
    else
    {
        techCounter++;
    }

    // Get next technologies to use
    while (LE_DATA_MAX != (technology = le_data_GetNextUsedTechnology()))
    {
        if (technology != TechList[techCounter])
        {
            LE_ERROR("Unexpected technology %d, should be %d.", technology, TechList[techCounter]);
            exit(EXIT_FAILURE);
        }
        else
        {
            techCounter++;
        }
    }
}

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
    if (RequestRef)
    {
        LE_ERROR("A data connection request already exist.");
        exit(EXIT_FAILURE);
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
    if (!RequestRef)
    {
        LE_ERROR("Not existing data connection reference.");
        exit(EXIT_FAILURE);
    }

    // Release the connection
    LE_INFO("Releasing the data connection.");
    le_data_Release(RequestRef);

    // Reset connection reference
    RequestRef = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 *  This function will get the date and time from a time server
 */
//--------------------------------------------------------------------------------------------------
static void GetDateTime
(
    void
)
{
    uint16_t year, month, day, hour, minute, second, millisecond;

    LE_ASSERT_OK(le_data_GetDate(&year, &month, &day));
    LE_ASSERT_OK(le_data_GetTime(&hour, &minute, &second, &millisecond));

    LE_INFO("Time retrieved from server: %04d-%02d-%02d %02d:%02d:%02d:%03d",
            year, month, day, hour, minute, second, millisecond);
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
        // Get the technology used for the data connection
        le_data_Technology_t usedTech = le_data_GetTechnology();
        if (LE_DATA_MAX == usedTech)
        {
            LE_ERROR("Unknown technology used for the data connection of '%s'!", intfName);
        }
        else
        {
            LE_INFO("'%s' connected using the technology '%s'!", intfName, techDico[usedTech]);
        }

        GetDateTime();

        LE_INFO("Wait for 5 seconds before releasing the data connection.");
        sleep(5);
        DisconnectData();
        LE_INFO("Verify that the data connection is released by checking DCS events.");
    }
    else
    {
        LE_INFO("'%s' disconnected!", intfName);
        exit(EXIT_SUCCESS);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Test main function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Running data connection service test");

    // Register handler for data connection state change
    le_data_AddConnectionStateHandler(DcsStateHandler, NULL);

    // Set technologies to use
    SetTechnologies();

    // Check if the technologies list was correctly updated
    CheckTechnologies();

    // Start the default data connection
    ConnectData();

    LE_INFO("Verify that the Data connection is established by checking DCS events.");
}

