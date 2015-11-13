/**
* This module implements the SIM Toolkit tests.
*
* You must issue the following commands:
* @verbatim
  $ app start simToolkit
  $ execInApp simToolkit simToolkit <accept/reject/none>
 @endverbatim
* Copyright (C) Sierra Wireless, Inc. Use of this work is subject to license.
*
*/

#include "legato.h"
#include "interfaces.h"


static le_sim_SimToolkitEventHandlerRef_t   HandlerRef = NULL;
static const char*                          AcceptCmdArg = NULL;


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
            break;
        case LE_SIM_STK_EVENT_MAX:
        default:
            LE_INFO("-TEST- Unknown SIM Toolkit event %d for SIM card.%d", stkEvent, simId);
            break;
    }

    if( strncmp(AcceptCmdArg, "accept", strlen("accept")) == 0 )
    {
        LE_INFO("-TEST- Accept SIM Toolkit command");
        LE_ERROR_IF((le_sim_AcceptSimToolkitCommand(simId) != LE_OK),
                    "Accept SIM Toolkit failure!");
    }
    else if( strncmp(AcceptCmdArg, "reject", strlen("reject")) == 0 )
    {
        LE_INFO("-TEST- Reject SIM Toolkit command");
        LE_ERROR_IF((le_sim_RejectSimToolkitCommand(simId) != LE_OK),
                    "Reject SIM Toolkit failure!");
    }
    else if( strncmp(AcceptCmdArg, "none", strlen("none")) == 0 )
    {
        LE_INFO("-TEST- Don't answer to SIM Toolkit command");
    }
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
    le_sim_RemoveSimToolkitEventHandler(HandlerRef);

    LE_INFO("EXIT SIM Toolkit Test");
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
            "Usage of the simToolkit app is:",
            "   execInApp simToolkit simToolkit <accept/reject/none>",
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
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    if (le_arg_NumArgs() == 1)
    {
        // Register a signal event handler for SIGINT when user interrupts/terminates process
        signal(SIGINT, SigHandler);

        LE_INFO("======== Start SIM Toolkit Test ========");

        AcceptCmdArg = le_arg_GetArg(0);
        if( strncmp(AcceptCmdArg, "accept", strlen("accept")) == 0 )
        {
            LE_INFO("SIM Toolkit Test will accept all SIM Toolkit commands.");
        }
        else if( strncmp(AcceptCmdArg, "reject", strlen("reject")) == 0 )
        {
            LE_INFO("SIM Toolkit Test will reject all SIM Toolkit commands.");
        }
        else if( strncmp(AcceptCmdArg, "none", strlen("none")) == 0 )
        {
            LE_INFO("SIM Toolkit Test will not answer to SIM Toolkit commands.");
        }
        else
        {
            PrintUsage();
            LE_INFO("EXIT SIM Toolkit Test");
            exit(EXIT_FAILURE);
        }

        HandlerRef=le_sim_AddSimToolkitEventHandler(TestSimToolkitHandler, NULL);
        LE_ASSERT(HandlerRef!=NULL);
        LE_INFO("======== Test SIM Toolkit SUCCESS ========");
    }
    else
    {
        PrintUsage();
        LE_INFO("EXIT SIM Toolkit Test");
        exit(EXIT_FAILURE);
    }
}
