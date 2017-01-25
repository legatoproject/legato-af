 /**
  * This module implements an eCall stress test.
  *
 * You must issue the following commands:
 * @verbatim
   $ app start eCallStress
   $ app runProc eCallStress --exe=eCallStress -- <PSAP number>
   @endverbatim

  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include "interfaces.h"

static const char *         PsapNumber = NULL;
static le_ecall_CallRef_t   MytECallRef = NULL;
static le_sem_Ref_t         ThreadSemaphore;
static le_ecall_State_t     LastECallState;
static uint32_t             TestCount;


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for eCall state Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyECallEventHandler
(
    le_ecall_CallRef_t  eCallRef,
    le_ecall_State_t    state,
    void*               contextPtr
)
{
    LE_INFO("eCall TEST: New eCall state: %d for eCall ref.%p", state, eCallRef);

    LastECallState = state;

    switch(state)
    {
        case LE_ECALL_STATE_STARTED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_STARTED.");
            le_sem_Post(ThreadSemaphore);
            break;
        }
        case LE_ECALL_STATE_CONNECTED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_CONNECTED.");
            break;
        }
        case LE_ECALL_STATE_DISCONNECTED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_DISCONNECTED.");
            le_sem_Post(ThreadSemaphore);
            break;
        }
        case LE_ECALL_STATE_WAITING_PSAP_START_IND:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_WAITING_PSAP_START_IND.");
            break;
        }
        case LE_ECALL_STATE_PSAP_START_IND_RECEIVED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_PSAP_START_IND_RECEIVED.");
            break;
        }
        case LE_ECALL_STATE_MSD_TX_STARTED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_MSD_TX_STARTED.");
            break;
        }
        case LE_ECALL_STATE_LLNACK_RECEIVED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_LLNACK_RECEIVED.");
            break;
        }
        case LE_ECALL_STATE_LLACK_RECEIVED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_LLACK_RECEIVED.");
            break;
        }
        case LE_ECALL_STATE_MSD_TX_COMPLETED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_MSD_TX_COMPLETED.");
            break;
        }
        case LE_ECALL_STATE_MSD_TX_FAILED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_MSD_TX_FAILED.");
            break;
        }
        case LE_ECALL_STATE_ALACK_RECEIVED_POSITIVE:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_ALACK_RECEIVED_POSITIVE.");
            break;
        }
        case LE_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN.");
            break;
        }
        case LE_ECALL_STATE_STOPPED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_STOPPED.");
            le_sem_Post(ThreadSemaphore);
            break;
        }
        case LE_ECALL_STATE_RESET:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_RESET.");
            break;
        }
        case LE_ECALL_STATE_COMPLETED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_COMPLETED.");
            break;
        }
        case LE_ECALL_STATE_FAILED:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_FAILED.");
            break;
        }
        case LE_ECALL_STATE_END_OF_REDIAL_PERIOD:
        {
            LE_INFO("eCall state is LE_ECALL_STATE_END_OF_REDIAL_PERIOD.");
            break;
        }
        case LE_ECALL_STATE_UNKNOWN:
        default:
        {
            LE_INFO("Unknown eCall state.");
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * ECallLoopThread
 *
 */
//--------------------------------------------------------------------------------------------------
static void* ECallLoopThread
(
    void* ctxPtr
)
{
    le_ecall_ConnectService();

    while(1)
    {
        LE_INFO("Wait for semaphore...");
        le_sem_Wait(ThreadSemaphore);
        sleep(1);

        if (LE_ECALL_STATE_STARTED == LastECallState)
        {
            LE_INFO("Take the semaphore, End eCall...");
            LE_ASSERT(le_ecall_End(MytECallRef) == LE_OK);
            le_ecall_Delete(MytECallRef);
            MytECallRef = NULL;
        }
        else if (   (LE_ECALL_STATE_DISCONNECTED == LastECallState)
                 || (LE_ECALL_STATE_STOPPED == LastECallState)
                )
        {
            TestCount++;
            LE_INFO("Take the semaphore, Start new eCall...");

            LE_ASSERT((MytECallRef=le_ecall_Create()) != NULL);
            LE_ASSERT(le_ecall_SetMsdPosition(MytECallRef, true, +48898064, +2218092, 0) == LE_OK);
            LE_ASSERT(le_ecall_SetMsdPositionN1(MytECallRef, -11, 22) == LE_OK);
            LE_ASSERT(le_ecall_SetMsdPositionN2(MytECallRef, 33, -44) == LE_OK);

            LE_ASSERT(le_ecall_SetMsdPassengersCount(MytECallRef, 3) == LE_OK);
            LE_ASSERT(le_ecall_StartTest(MytECallRef) == LE_OK);
            LE_INFO("Start eCall #%d", TestCount);
        }
    }

    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Create and start a test eCall.
 *
 */
//--------------------------------------------------------------------------------------------------
static void StartStressECall
(
    void
)
{
    LE_INFO("Start StartStressECall");

    LastECallState = LE_ECALL_STATE_DISCONNECTED;
    TestCount = 0;

    ThreadSemaphore = le_sem_Create("ThreadSem",0);

    LE_ASSERT(le_ecall_AddStateChangeHandler(MyECallEventHandler, NULL) != NULL);

    LE_ASSERT(le_ecall_SetPsapNumber(PsapNumber) == LE_OK);

    LE_ASSERT(le_ecall_SetMsdTxMode(LE_ECALL_TX_MODE_PUSH) == LE_OK);

    LE_ASSERT((MytECallRef=le_ecall_Create()) != NULL);

    LE_ASSERT(le_ecall_SetMsdPosition(MytECallRef, true, +48898064, +2218092, 0) == LE_OK);

    LE_ASSERT(le_ecall_SetMsdPositionN1(MytECallRef, -512, -512) == LE_OK);

    LE_ASSERT(le_ecall_SetMsdPositionN2(MytECallRef, 511, 511) == LE_OK);

    LE_ASSERT(le_ecall_SetMsdPassengersCount(MytECallRef, 3) == LE_OK);

    le_thread_Start(le_thread_Create("ECallLoopThread", ECallLoopThread, NULL));
    // Start Test
    le_sem_Post(ThreadSemaphore);
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
    LE_INFO("End and delete last test eCall (TestCount.%d)", TestCount);
    if(MytECallRef)
    {
        le_ecall_End(MytECallRef);
        le_ecall_Delete(MytECallRef);
    }
    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Helper.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage()
{
    int idx;
    bool sandboxed = (getuid() != 0);
    const char * usagePtr[] = {
            "Usage of the eCallStress is:",
            "   app runProc eCallStress --exe=eCallStress -- <PSAP number>",
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

        PsapNumber = le_arg_GetArg(0);
        LE_INFO("======== Start eCallStress Test with PSAP.%s ========", PsapNumber);
        StartStressECall();
        LE_INFO("======== eCallStress Test SUCCESS ========");
    }
    else
    {
        PrintUsage();
        LE_INFO("EXIT eCallStress");
        exit(EXIT_FAILURE);
    }
}
