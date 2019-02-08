 /**
  * This module is an integration test for the SIM component.
  *
  * You must issue the following command to run the test:
  * @verbatim
  * $ app runProc simTest --exe=simTest -- <cmd> [<arg1>] [<arg2]>]
  *
  * Usage:
  * app runProc simTest --exe=simTest -- help
  * @endverbatim
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "main.h"

//--------------------------------------------------------------------------------------------------
/**
 * Structure to hold an enum <=> string relation for @ref le_sim_Id_t.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sim_Id_t  simId;    ///< SIM identifier
    const char * strPtr;   ///< SIM location string
}
SimIdStringAssoc_t;

//--------------------------------------------------------------------------------------------------
/**
 * Array containing all enum <=> string relations for @ref le_sim_Id_t.
 */
//--------------------------------------------------------------------------------------------------
static const SimIdStringAssoc_t SimIdStringAssocs[] =
{
    { LE_SIM_EMBEDDED,          "emb"  },
    { LE_SIM_EXTERNAL_SLOT_1,   "ext1" },
    { LE_SIM_EXTERNAL_SLOT_2,   "ext2" },
    { LE_SIM_REMOTE,            "rem"  },
    { LE_SIM_UNSPECIFIED,       "unsp" }
};

//--------------------------------------------------------------------------------------------------
/**
 * References for SIM events handlers
 */
//--------------------------------------------------------------------------------------------------
static le_sim_IccidChangeHandlerRef_t IccidChangeHandlerRef = NULL;
static le_sim_SimToolkitEventHandlerRef_t StkHandlerRef = NULL;
static le_sim_NewStateHandlerRef_t NewSimHandlerRef = NULL;
static le_sim_ProfileUpdateHandlerRef_t ProfileUpdateHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to print log messages
 */
//--------------------------------------------------------------------------------------------------
void Print
(
    char* stringPtr  ///< [IN] A NULL terminated string to be printed
)
{
    bool sandboxed = (getuid() != 0);

    if (NULL == stringPtr)
    {
        return;
    }

    if(sandboxed)
    {
        LE_INFO("%s", stringPtr);
    }
    else
    {
        fprintf(stderr, "%s\n", stringPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print the usage of the test
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage
(
    void
)
{
    int idx;
    const char * usagePtr[] =
    {
     "Usage of the 'simTest' application is:",
     "SIM allocation test: app runProc simTest --exe=simTest -- create <ext/emb/unsp> <pin>",
     "SIM state test: app runProc simTest --exe=simTest -- state <ext1/ext2/emb/unsp> <pin>",
     "SIM authentication test: app runProc simTest"
     " --exe=simTest -- auth <ext/emb/unsp> <pin> <puk>",
     "No SIM test: app runProc simTest --exe=simTest -- nosim <ext/emb/unsp>",
     "SIM select: app runProc simTest --exe=simTest -- select",
     "SIM lock test: app runProc simTest --exe=simTest -- lock <emb/ext1/ext2/rem/unsp> <pin>",
     "SIM GetICCID test: app runProc simTest --exe=simTest -- iccid <emb/ext1/ext2/rem/unsp>",
     "SIM GetEID test: app runProc simTest --exe=simTest -- eid <emb/ext1/ext2/rem/unsp>",
     "SIM send apdu test: app runProc simTest --exe=simTest -- access <emb/ext1/ext2/rem/unsp>",
     "SIM allocation test: app runProc simTest --exe=simTest -- powerUpDown",
     "SIM events: app runProc simTest --exe=simTest -- events",
     "SIM auto selection: app runProc simTest --exe=simTest -- auto <1/0>",
     "",
    };

    for (idx = 0; idx < NUM_ARRAY_MEMBERS(usagePtr); idx++)
    {
        Print((char*) usagePtr[idx]);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * This function converts a SIM location string to a SIM identifier.
 *
 * @note If provided string doesn't match any SIM ID, then the application exists.
 *
 * @return SIM identifier
 */
//-------------------------------------------------------------------------------------------------
static le_sim_Id_t GetSimId
(
    const char* strPtr   ///< [IN] SIM location string
)
{
    int i;
    for(i = 0; i < NUM_ARRAY_MEMBERS(SimIdStringAssocs); i++)
    {
        if (0 == strcmp(SimIdStringAssocs[i].strPtr, strPtr))
        {
            return SimIdStringAssocs[i].simId;
        }
    }

    LE_ERROR("Unable to convert '%s' to a le_sim_Id_t", strPtr);
    PrintUsage();
    exit(EXIT_FAILURE);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SIM Toolkit events.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SimToolkitHandler
(
    le_sim_Id_t       simId,     ///< [IN] SIM identifier
    le_sim_StkEvent_t stkEvent,  ///< [IN] SIM Toolkit event
    void*             contextPtr ///< [IN] Context pointer
)
{
    le_sim_StkRefreshMode_t refreshMode;
    le_sim_StkRefreshStage_t refreshStage;
    LE_INFO("SIM Toolkit event for SIM card: %d", simId);

    switch(stkEvent)
    {
        case LE_SIM_OPEN_CHANNEL:
            LE_INFO("STK event: OPEN_CHANNEL");
            break;

        case LE_SIM_REFRESH:
            LE_ASSERT_OK(le_sim_GetSimToolkitRefreshMode(simId, &refreshMode));
            LE_ASSERT_OK(le_sim_GetSimToolkitRefreshStage(simId, &refreshStage));
            LE_INFO("STK event: REFRESH SIM. Mode: %d, Stage: %d", refreshMode, refreshStage);
            break;

        case LE_SIM_STK_EVENT_MAX:
        default:
            LE_INFO("Unknown SIM Toolkit event %d for SIM card.%d", stkEvent, simId);
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for ICCID change notification
 *
 */
//--------------------------------------------------------------------------------------------------
static void IccidChangeHandler
(
    le_sim_Id_t simId,     ///< [IN] SIM identifier
    const char* iccidPtr,  ///< [IN] ICCID string pointer
    void*       contextPtr ///< [IN] Context pointer
)
{
    LE_INFO("ICCID Change event for SIM card: %d", simId);
    LE_INFO("ICCID: %s", iccidPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for new SIM notification
 *
 */
//--------------------------------------------------------------------------------------------------
static void ProfileUpdatehandler
(
    le_sim_Id_t       simId,     ///< [IN] SIM identifier
    le_sim_StkEvent_t stkEvent,  ///< [IN] SIM Toolkit event
    void*             contextPtr ///< [IN] Context pointer
)
{
    LE_INFO("Profile update request");
    LE_INFO("Event: %d", stkEvent);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for profile update notification
 *
 */
//--------------------------------------------------------------------------------------------------
static void NewSimHandler
(
    le_sim_Id_t     simId,     ///< [IN] SIM identifier
    le_sim_States_t simState,  ///< [IN] SIM state
    void*           contextPtr ///< [IN] Context pointer
)
{
    LE_INFO("New SIM event for SIM card: %d", simId);
    LE_INFO("SIM state: %d", simState);
}

//--------------------------------------------------------------------------------------------------
/**
 * Main thread.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    const char* testString = "";
    le_sim_Id_t cardId = 0;
    bool        freeRunningApp = false;

    LE_INFO("Start simTest app.");

    // Get the test identifier and SIM select
    if (le_arg_NumArgs() >= 1)
    {
        testString = le_arg_GetArg(0);
        if (NULL == testString)
        {
            LE_ERROR("testString is NULL");
            exit(EXIT_FAILURE);
        }
    }

    //Test: SIM automatic selection
    if (strcmp(testString, "auto") == 0)
    {
        bool enable = true;
        const char* arg;

        LE_INFO("Enable/Disable automatic SIM selection");
        if (le_arg_NumArgs() >= 2)
        {
            arg = le_arg_GetArg(1);
            if (NULL == arg)
            {
                LE_ERROR("argument is NULL");
                exit(EXIT_FAILURE);
            }
            enable = (*arg == '0') ? false : true;
        }

        LE_ASSERT_OK(le_sim_SetAutomaticSelection(enable));
        LE_ASSERT_OK(le_sim_GetAutomaticSelection(&enable));
        LE_INFO("Automatic SIM selection state: %d", enable);
        exit(EXIT_SUCCESS);
    }

    if (le_arg_NumArgs() > 1)
    {
        const char* cardIdPtr = le_arg_GetArg(1);
        if (NULL == cardIdPtr)
        {
            LE_ERROR("cardIdPtr is NULL");
            exit(EXIT_FAILURE);
        }
        cardId = GetSimId(cardIdPtr);
    }

    // Test: state
    if (strcmp(testString, "state") == 0)
    {
        const char* pin;

        // Get the pin code
        if (le_arg_NumArgs() == 3)
        {
            pin = le_arg_GetArg(2);
            if (NULL == pin)
            {
                LE_ERROR("pin is NULL");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            PrintUsage();
            exit(EXIT_FAILURE);
        }

        // Call the test function
        simTest_State(cardId, pin);
    }
    // Test: create
    else if (strcmp(testString, "create") == 0)
    {
        const char* pin;

        // Get the pin code
        if (le_arg_NumArgs() == 3)
        {
            pin = le_arg_GetArg(2);
            if (NULL == pin)
            {
                LE_ERROR("pin is NULL");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            Print("error");
            PrintUsage();
            exit(EXIT_FAILURE);
        }

        // Call the test function
        simTest_Create(cardId, pin);
    }
    // Test: authentication
    else if (strcmp(testString, "auth") == 0)
    {
        const char* pin;
        const char* puk;

        // Get the pin and puk codes
        if (le_arg_NumArgs() == 4)
        {
            pin = le_arg_GetArg(2);
            puk = le_arg_GetArg(3);
            if (NULL == pin)
            {
                LE_ERROR("pin is NULL");
                exit(EXIT_FAILURE);
            }
            if (NULL == puk)
            {
                LE_ERROR("puk is NULL");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            PrintUsage();
            exit(EXIT_FAILURE);
        }

        // Call the test function
        simTest_Authentication(cardId,pin,puk);
    }
    // Test: no sim
    else if (strcmp(testString, "nosim") == 0)
    {
        // Call the test function
        simTest_SimAbsent(cardId);
    }
    // Test: SIM selection
    else if (strcmp(testString, "select") == 0)
    {
        // Call the test function
        simTest_SimSelect();
    }
    // Test: lock
    else if (strcmp(testString, "lock") == 0)
    {
        const char* pin;

        // Get the pin code
        if (le_arg_NumArgs() == 3)
        {
            pin = le_arg_GetArg(2);
            if (NULL == pin)
            {
                LE_ERROR("pin is NULL");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            Print("error");
            PrintUsage();
            exit(EXIT_FAILURE);
        }

        // Call the test function
        simTest_Lock(cardId, pin);
    }
    // Test: SIM Get ICCID
    else if (strcmp(testString, "iccid") == 0)
    {
        // Call the test function
        simTest_SimGetIccid(cardId);
    }
    else if (strcmp(testString, "powerUpDown") == 0)
    {
        // Powers up current SIM card
        simTest_SimPowerUpDown();
    }
    // Test: SIM Get EID
    else if (strcmp(testString, "eid") == 0)
    {
        // Call the test function
        simTest_SimGetEid(cardId);
    }
    // Test: send apdu
    else if (strcmp(testString, "access") == 0)
    {
       // Call the test function
       LE_INFO("======== Test SIM access Test Started ========");
       simTest_SimAccess(cardId);
       LE_INFO("======== Test SIM access Test SUCCESS ========");
    }
    // Test: SIM events
    else if (strcmp(testString, "events") == 0)
    {
        StkHandlerRef=le_sim_AddSimToolkitEventHandler(SimToolkitHandler, NULL);
        LE_ASSERT(StkHandlerRef!=NULL);

        IccidChangeHandlerRef = le_sim_AddIccidChangeHandler(IccidChangeHandler, NULL);
        LE_ASSERT(IccidChangeHandlerRef!=NULL);

        NewSimHandlerRef = le_sim_AddNewStateHandler(NewSimHandler, NULL);
        LE_ASSERT(NewSimHandlerRef!=NULL);

        ProfileUpdateHandlerRef = le_sim_AddProfileUpdateHandler(ProfileUpdatehandler, NULL);
        LE_ASSERT(ProfileUpdateHandlerRef!=NULL);

        freeRunningApp = true;
    }
    else
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    if (!freeRunningApp)
    {
        LE_INFO("SimTest done");
        exit(EXIT_SUCCESS);
    }
}
