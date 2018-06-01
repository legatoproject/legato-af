/**
* This module implements the SIM local profile swap tests.
*
* You must issue the following commands:
* @verbatim
  $ app runProc simProfileSwap --exe=bin/simProfileSwap --
  $        <ext/esim> <gemalto/oberthur/gd/morpho/valid> <ecs/commercial>
 @endverbatim
* Copyright (C) Sierra Wireless Inc.
*
*/

#include "legato.h"
#include "interfaces.h"

static le_sim_SimToolkitEventHandlerRef_t   StkHandlerRef = NULL;
static le_sim_NewStateHandlerRef_t          HandlerRef = NULL;
static le_sim_Id_t                          SimIdSelect = LE_SIM_EXTERNAL_SLOT_1;
static le_sim_Manufacturer_t                Manufacturer = LE_SIM_GEMALTO;
static const char*                          Profile = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SIM Toolkit events.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestSimToolkitHandler
(
    le_sim_Id_t       simId,
    le_sim_StkEvent_t stkEvent,
    void*             contextPtr
)
{
    switch(stkEvent)
    {
        case LE_SIM_OPEN_CHANNEL:
            LE_INFO("-TEST- OPEN_CHANNEL SIM Toolkit event for SIM card.%d", simId);
            break;
        case LE_SIM_REFRESH:
            LE_INFO("-TEST- REFRESH SIM Toolkit event for SIM card.%d", simId);
            LE_ERROR_IF((le_sim_AcceptSimToolkitCommand(simId) != LE_OK),
                        "Accept SIM Toolkit failure!");
            break;
        case LE_SIM_STK_EVENT_MAX:
        default:
            LE_INFO("-TEST- Unknown SIM Toolkit event %d for SIM card.%d", stkEvent, simId);
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SIM States Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestSimStateHandler
(
    le_sim_Id_t     simId,
    le_sim_States_t simState,
    void*           contextPtr
)
{
    switch(simState)
    {
        case LE_SIM_INSERTED:
            LE_INFO("-TEST- New state LE_SIM_INSERTED for SIM card.%d", simId);
            break;
        case LE_SIM_ABSENT:
            LE_INFO("-TEST- New state LE_SIM_ABSENT for SIM card.%d", simId);
            break;
        case LE_SIM_READY:
            LE_INFO("-TEST- New state LE_SIM_READY for SIM card.%d", simId);
            break;
        case LE_SIM_BLOCKED:
            LE_INFO("-TEST- New state LE_SIM_BLOCKED for SIM card.%d", simId);
            break;
        case LE_SIM_BUSY:
            LE_INFO("-TEST- New state LE_SIM_BUSY for SIM card.%d", simId);
            break;
        case LE_SIM_POWER_DOWN:
            LE_INFO("-TEST- New state LE_SIM_POWER_DOWN for SIM card.%d", simId);
            break;
        case LE_SIM_STATE_UNKNOWN:
            LE_INFO("-TEST- New state LE_SIM_STATE_UNKNOWN for SIM card.%d", simId);
            break;
        default:
            LE_INFO("-TEST- New state %d for SIM card.%d", simState, simId);
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Get current subscription
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_sim_GetCurrentSubscription
(
    void
)
{
    bool isEcs;
    le_result_t res = le_sim_IsEmergencyCallSubscriptionSelected(SimIdSelect, &isEcs);

    LE_ASSERT(res != LE_FAULT);
    if(res == LE_NOT_FOUND)
    {
        LE_INFO("Cannot determine current subscription");
    }
    else if (res == LE_OK)
    {
        LE_INFO("Current subscription is %s", (isEcs ? "ECS":"Commercial"));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: SwapToEcs
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_sim_SwapToEcs
(
    void
)
{
    bool isEcs;

    LE_INFO("Start Testle_sim_SwapToEcs SimId %d, Manufacture %d", SimIdSelect, Manufacturer);

    LE_ASSERT(le_sim_LocalSwapToEmergencyCallSubscription(SimIdSelect, Manufacturer) == LE_OK);
    LE_ASSERT(le_sim_IsEmergencyCallSubscriptionSelected(SimIdSelect, &isEcs) == LE_OK);
    LE_ASSERT(isEcs == true);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: SwapToCommercial
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_sim_SwapToCommercial
(
    void
)
{
    bool isEcs;

    LE_INFO("Start Testle_sim_SwapToCommercial SimId %d, Manufacture %d",
        SimIdSelect, Manufacturer);

    LE_ASSERT(le_sim_LocalSwapToCommercialSubscription(SimIdSelect, Manufacturer) == LE_OK);
    LE_ASSERT(le_sim_IsEmergencyCallSubscriptionSelected(SimIdSelect, &isEcs) == LE_OK);
    LE_ASSERT(isEcs == false);
}

//--------------------------------------------------------------------------------------------------
/**
 * The signal event handler function for SIGINT/SIGTERM when process dies.
 */
//--------------------------------------------------------------------------------------------------
static void SigHandler
(
    int sigNum
)
{
    le_sim_RemoveSimToolkitEventHandler(StkHandlerRef);
    le_sim_RemoveNewStateHandler(HandlerRef);

    LE_INFO("EXIT SIM local Profile Swap Test");
    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Helper.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage
(
    void
)
{
    int idx;
    bool sandboxed = (getuid() != 0);
    const char * usagePtr[] =
    {
            "Usage of the simProfileSwap app is:",
            "   app runProc simProfileSwap --exe=bin/simProfileSwap -- <ext/esim>"
              " <gemalto/oberthur/gd/morpho/valid> <ecs/commercial>",
    };

    for(idx = 0; idx < NUM_ARRAY_MEMBERS(usagePtr); idx++)
    {
        if(sandboxed)
        {
            LE_INFO("%s", usagePtr[idx]);
        }
        else
        {
            fprintf(stderr, "%s\n", usagePtr[idx]);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the test's arguments.
 */
//--------------------------------------------------------------------------------------------------
static void GetArgs
(
    void
)
{
    const char*  arg;

    // Get SIM type
    arg = le_arg_GetArg(0);
    if (NULL == arg)
    {
        LE_ERROR("arg is NULL");
        exit(EXIT_FAILURE);
    }
    if( 0 == strncmp(arg, "ext", strlen("ext")))
    {
        LE_INFO("external SIM is selected.");
        SimIdSelect = LE_SIM_EXTERNAL_SLOT_1;

    }
    else if( 0 == strncmp(arg, "esim", strlen("esim")))
    {
        LE_INFO("embedded SIM is selected.");
        SimIdSelect = LE_SIM_EMBEDDED;
    }
    else
    {
        PrintUsage();
        LE_INFO("EXIT SIM local Profile Swap Test");
        exit(EXIT_FAILURE);
    }

    // Get Card manufacturer
    arg = le_arg_GetArg(1);
    if (NULL == arg)
    {
        LE_ERROR("arg is NULL");
        exit(EXIT_FAILURE);
    }
    if( 0 == strncmp(arg, "gemalto", strlen("gemalto")))
    {
        LE_INFO("Card manufacturer is Gemalto.");
        Manufacturer = LE_SIM_GEMALTO;
    }
    else if( 0 == strncmp(arg, "oberthur", strlen("oberthur")))
    {
        LE_INFO("Card manufacturer is Oberthur.");
        Manufacturer = LE_SIM_OBERTHUR;
    }
    else if( 0 == strncmp(arg, "gd", strlen("gd")))
    {
        LE_INFO("Card manufacturer is G&D.");
        Manufacturer = LE_SIM_G_AND_D;
    }
    else if( 0 == strncmp(arg, "morpho", strlen("morpho")))
    {
        LE_INFO("Card manufacturer is Morpho.");
        Manufacturer = LE_SIM_MORPHO;
    }
    else if( 0 == strncmp(arg, "valid", strlen("valid")))
    {
        LE_INFO("Card manufacturer is VALID.");
        Manufacturer = LE_SIM_VALID;
    }
    else
    {
        PrintUsage();
        LE_INFO("EXIT SIM local Profile Swap Test");
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    if (le_arg_NumArgs() == 3)
    {
        // Register a signal event handler for SIGINT when user interrupts/terminates process
        signal(SIGINT, SigHandler);

        GetArgs();
        Profile = le_arg_GetArg(2);
        if (NULL == Profile)
        {
            LE_ERROR("Profile is NULL");
            exit(EXIT_FAILURE);
        }
        LE_INFO("======== Start SIM local Profile Swap Test with Profile.%s========", Profile);

        StkHandlerRef=le_sim_AddSimToolkitEventHandler(TestSimToolkitHandler, NULL);
        LE_ASSERT(StkHandlerRef!=NULL);

        if( strncmp(Profile, "ecs", strlen("ecs")) == 0 )
        {
            // Add the state handler
            LE_ASSERT((HandlerRef = le_sim_AddNewStateHandler(TestSimStateHandler, NULL)) != NULL);

            LE_INFO("======== SwapToEcs Test  ========");
            Testle_sim_GetCurrentSubscription();
            Testle_sim_SwapToEcs();
        }
        else if( strncmp(Profile, "commercial", strlen("commercial")) == 0 )
        {
            // Add the state handler
            LE_ASSERT((HandlerRef = le_sim_AddNewStateHandler(TestSimStateHandler, NULL)) != NULL);

            LE_INFO("======== SwapToCommercial Test  ========");
            Testle_sim_GetCurrentSubscription();
            Testle_sim_SwapToCommercial();
        }
        else
        {
            PrintUsage();
            LE_INFO("EXIT SIM local Profile Swap Test");
            exit(EXIT_FAILURE);
        }

        LE_INFO("======== Test SIM local Profile Swap Test SUCCESS ========");
        exit(EXIT_SUCCESS);
    }
    else
    {
        PrintUsage();
        LE_INFO("EXIT SIM local Profile Swap Test");
        exit(EXIT_FAILURE);
    }
}
