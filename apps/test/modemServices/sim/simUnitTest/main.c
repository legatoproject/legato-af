/**
 * This module implements the unit tests for SIM API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "log.h"
#include "pa_sim_simu.h"
#include "args.h"

#define NB_CLIENT        2
#define INITIAL_PIN_TRY  3

// Task context
typedef struct
{
    uint32_t appId;
    le_thread_Ref_t appThreadRef;
    le_sim_NewStateHandlerRef_t statHandler;
    le_sim_Id_t simId;
    le_sim_States_t simState;
    le_sim_SimToolkitEventHandlerRef_t stkHandler;
    le_sim_StkEvent_t  stkEvent;
} AppContext_t;

static AppContext_t AppCtx[NB_CLIENT];
static le_sem_Ref_t    ThreadSemaphore;
static le_sem_Ref_t    StkHandlerSem;
static le_sim_States_t CurrentSimState = LE_SIM_ABSENT;
static le_sim_Id_t CurrentSimId = LE_SIM_EXTERNAL_SLOT_2;
static le_clk_Time_t TimeToWait ={ 0, 1000000 };
static char Iccid[]="89330123164011144830";
static char PhoneNum[]="+33643537818";
static char Imsi[]="208011700352758";
static char Mcc[]="208";
static char Mnc[]="01";
static char Operator[]="orange";
static char Pin[]="0000";
static char BadPin[]="1234";
static char ShortPin[]="000";
static char Puk[]="12345678";
static char ShortPuk[]="1234567";
static char LongPuk[]="123456789";
static char NewPin[]="6789";
static le_sim_StkEvent_t  StkEvent;

le_result_t le_sim_Init(void);

//--------------------------------------------------------------------------------------------------
/**
 * Set the pa_simu
 *
 */
//--------------------------------------------------------------------------------------------------
static void SetSimCardInfo
(
    void
)
{
    pa_simSimu_SetPIN(Pin);
    pa_simSimu_SetPUK(Puk);
    pa_simSimu_SetIMSI(Imsi);
    pa_simSimu_SetCardIdentification(Iccid);
    pa_simSimu_SetSubscriberPhoneNumber(PhoneNum);
    pa_simSimu_SetHomeNetworkMccMnc(Mcc, Mnc);
    pa_simSimu_SetHomeNetworkOperator(Operator);
}

//--------------------------------------------------------------------------------------------------
/**
 * Synchronize test thread (i.e. main) and tasks
 *
 */
//--------------------------------------------------------------------------------------------------
static void SynchTest( void )
{
    int i=0;

    for (;i<NB_CLIENT;i++)
    {
        LE_ASSERT(le_sem_WaitWithTimeOut(ThreadSemaphore, TimeToWait) == LE_OK);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Check the result of the state handlers
 *
 */
//--------------------------------------------------------------------------------------------------
static void CheckStateHandlerResult
(
    void
)
{
    int i;

    // Check that contexts are correctly updated
    for (i=0; i < NB_CLIENT; i++)
    {
        LE_ASSERT(AppCtx[i].appId == i);
        LE_ASSERT(AppCtx[i].simState == CurrentSimState);
        LE_ASSERT(AppCtx[i].simId == CurrentSimId);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * SIM state handler: this handler is subcribes by test tasks and it is called on SIM state
 * modification
 *
 */
//--------------------------------------------------------------------------------------------------
static void SimStateHandler
(
    le_sim_Id_t simId,
    le_sim_States_t simState,
    void* contextPtr
)
{
    AppContext_t * appCtxPtr = (AppContext_t*) contextPtr;

    LE_DEBUG("App id: %d", appCtxPtr->appId);

    LE_ASSERT(CurrentSimState == simState);
    LE_ASSERT(CurrentSimId == simId);

    appCtxPtr->simState = simState;
    appCtxPtr->simId = simId;

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test tasks: this function handles the task and run an eventLoop
 *
 */
//--------------------------------------------------------------------------------------------------
static void* AppHandler
(
    void* ctxPtr
)
{
    AppContext_t * appCtxPtr = (AppContext_t*) ctxPtr;

    LE_DEBUG("App id: %d", appCtxPtr->appId);

    // Subscribe to SIM state handler
    appCtxPtr->statHandler = le_sim_AddNewStateHandler(SimStateHandler, ctxPtr);
    LE_ASSERT(appCtxPtr->statHandler != NULL);

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(ThreadSemaphore);

    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Stk handler: this handler is called on stk event
 *
 */
//--------------------------------------------------------------------------------------------------
static void StkHandler
(
    le_sim_Id_t simId,
    le_sim_StkEvent_t stkEvent,
    void* contextPtr
)
{
    AppContext_t * appCtxPtr = (AppContext_t*) contextPtr;
    LE_ASSERT(CurrentSimId == simId);
    LE_ASSERT(StkEvent == stkEvent);

    appCtxPtr->stkEvent = stkEvent;

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add stk handler: this function is used to add a stk event handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void AddStkHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    AppContext_t * appCtxPtr = (AppContext_t*) param1Ptr;

    // internal semaphore: le_sim internal variable SimToolkitHandlerCount must be correctly updated
    // before calling le_sim_AddSimToolkitEventHandler again
    le_sem_Wait(StkHandlerSem);

    appCtxPtr->stkHandler = le_sim_AddSimToolkitEventHandler(StkHandler, appCtxPtr);
    LE_ASSERT(appCtxPtr->stkHandler != NULL);

    le_sem_Post(StkHandlerSem);

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove states and stk event handlers
 *
 */
//--------------------------------------------------------------------------------------------------
static void RemoveHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    AppContext_t * appCtxPtr = (AppContext_t*) param1Ptr;

    le_sim_RemoveNewStateHandler( appCtxPtr->statHandler );

    le_sim_RemoveSimToolkitEventHandler( appCtxPtr->stkHandler );

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * swap SIM profile
 *
 */
//--------------------------------------------------------------------------------------------------
static void LocalSwap
(
    void* param1Ptr,
    void* param2Ptr
)
{
    static bool firstCall = true;

    // The core test is alling this funcrtion rwice to test both functions
    if (firstCall)
    {
        LE_ASSERT(LE_OK == le_sim_LocalSwapToCommercialSubscription(CurrentSimId,
                                                                    LE_SIM_OBERTHUR));
    }
    else
    {
        LE_ASSERT(LE_OK == le_sim_LocalSwapToEmergencyCallSubscription(CurrentSimId,
                                                                       LE_SIM_MORPHO));
    }

    // we have no semaphore here as these functions are blocking (waiting for a SIM refresh done by
    // the core test
}

//--------------------------------------------------------------------------------------------------
/**
 * Test sim states
 *
 * API tested:
 * - le_sim_IsPresent
 * - le_sim_IsReady
 * - le_sim_GetState
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
static void CheckSimStates
(
    void
)
{
    bool isPresent;
    bool isReady;

    switch (CurrentSimState)
    {
        case LE_SIM_INSERTED:
            isPresent = true;
            isReady = false;
        break;
        case LE_SIM_READY:
            isPresent = true;
            isReady = true;
        break;
        case LE_SIM_BLOCKED:
        case LE_SIM_BUSY:
            isPresent = true;
            isReady = false;
        break;
        case LE_SIM_ABSENT:
        case LE_SIM_STATE_UNKNOWN:
        default:
            isPresent = false;
            isReady = false;

    }

    LE_ASSERT( le_sim_IsPresent(CurrentSimId)==isPresent );
    LE_ASSERT( le_sim_IsReady(CurrentSimId)==isReady );
    LE_ASSERT( le_sim_GetState(CurrentSimId)==CurrentSimState );
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the test environmeent:
 * - create some tasks (simulate multi app)
 * - create semaphore (to make checkpoints and synchronize test and tasks)
 * - simulate an empty rack
 * - check that state handlers are correctly called
 *
 * API tested:
 * - le_sim_AddNewStateHandler
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestSim_AddHandlers
(
    void
)
{
    int i;

    // Create a semaphore to coordinate the test
    ThreadSemaphore = le_sem_Create("HandlerSem",0);

    // int app context
    memset(AppCtx, 0, NB_CLIENT*sizeof(AppContext_t));

    // Start tasks: simulate multi-user of le_sim
    // each thread subcribes to state handler using le_sim_AddNewStateHandler
    for (i=0; i < NB_CLIENT; i++)
    {
        char string[20]={0};
        snprintf(string,20,"app%dhandler", i);
        AppCtx[i].appId = i;
        AppCtx[i].appThreadRef = le_thread_Create(string, AppHandler, &AppCtx[i]);
        le_thread_Start(AppCtx[i].appThreadRef);
    }

    // Wait that the tasks have started before continuing the test
    SynchTest();

    // Sim is absent in this test
    CurrentSimState = LE_SIM_ABSENT;

    pa_simSimu_SetSelectCard(CurrentSimId);
    le_sim_SelectCard(CurrentSimId);
    pa_simSimu_ReportSIMState(CurrentSimState);

    // The tasks have subscribe to state event handler:
    // wait the handlers' calls
    SynchTest();

    // Check state handler result
    CheckStateHandlerResult();

    // Check that we are in the expected states (sim absent)
    CheckSimStates();

    // Check that no more call of the semaphore
    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test PIN and PUK
 *
 * API tested:
 * - le_sim_EnterPIN
 * - le_sim_Unblock
 * - le_sim_GetRemainingPINTries
 * - le_sim_GetRemainingPUKTries
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestSim_PinPuk
(
    void
)
{
    uint32_t remainingPukTries = 0;
    uint32_t triesPuk = 0;

    // Check the states
    CheckSimStates();

    // The test is started with no sim inserted (end of the previous test)

    // Sim absent, test Pin and Puk (error expected as no sim inserted)
    LE_ASSERT(le_sim_EnterPIN(CurrentSimId, BadPin) == LE_NOT_FOUND);
    LE_ASSERT(le_sim_Unblock(CurrentSimId, Puk, BadPin) == LE_NOT_FOUND);
    LE_ASSERT(le_sim_GetRemainingPINTries(CurrentSimId) == LE_NOT_FOUND);
    LE_ASSERT(LE_NOT_FOUND == le_sim_GetRemainingPUKTries(CurrentSimId, &remainingPukTries));

    // Insert SIM (busy state)
    CurrentSimState = LE_SIM_BUSY;
    pa_simSimu_ReportSIMState(CurrentSimState);

    // Note that in LE_SIM_BUSY state, no handlers are called

    // Check that we are in the expected states (sim busy)
    CheckSimStates();

    // test pin and puk: error is expected as sim in BUSY state
    LE_ASSERT(le_sim_EnterPIN(CurrentSimId, BadPin) == LE_FAULT);
    LE_ASSERT(le_sim_Unblock(CurrentSimId, Puk, BadPin) == LE_FAULT);
    LE_ASSERT(le_sim_GetRemainingPINTries(CurrentSimId) == LE_FAULT);
    LE_ASSERT(LE_FAULT == le_sim_GetRemainingPUKTries(CurrentSimId, &remainingPukTries));

    // SIM is inserted
    CurrentSimState = LE_SIM_INSERTED;
    pa_simSimu_ReportSIMState(CurrentSimState);

    // Wait the handlers' calls
    SynchTest();

    // Check state handler result
    CheckStateHandlerResult();

    // Check that we are in the expected states (sim busy)
    CheckSimStates();

    // block the pin
    int i=3;
    for (;i!=0;i--)
    {
        // Test the remaining PIN tries
        LE_ASSERT(le_sim_GetRemainingPINTries(CurrentSimId) == i);
        if (i==1)
        {
            // Update CurrentSimState for handlers' checks
            CurrentSimState = LE_SIM_BLOCKED;
        }

        // Try to enter the PIN: the PIN is wrong, an error is expected
        LE_ASSERT(le_sim_EnterPIN(CurrentSimId, BadPin) == LE_FAULT);

        // Don't try to unlock the SIM in the last failed PIN, PUK tests are done behind
        if (i != 1)
        {
            // Try to unblock the SIM whereas the sim is not in PUK state (error expected)
            LE_ASSERT(le_sim_Unblock(CurrentSimId, Puk, BadPin) == LE_FAULT);
        }
        else
        {
            // The last time, check that handlers have been called (the SIM is in BLOCKED state,
            // handers have been called to warn the state modification)
            SynchTest();
        }

        // Get the remaining PIN tries
        LE_ASSERT(le_sim_GetRemainingPINTries(CurrentSimId) == i-1);

        // Check that we are in the expected states
        CheckSimStates();
    }

    // Try the puk (bad puk or bad pin, errors are expected)
    LE_ASSERT(le_sim_Unblock(CurrentSimId, ShortPuk, BadPin) == LE_OUT_OF_RANGE);
    LE_ASSERT(le_sim_Unblock(CurrentSimId, LongPuk, BadPin) == LE_OUT_OF_RANGE);
    LE_ASSERT(le_sim_Unblock(CurrentSimId, Puk, ShortPin) == LE_UNDERFLOW);

    // The next operation unblocks the pin: we change the global variable here to be ready when the
    // handlers will be called
    CurrentSimState = LE_SIM_READY;

    // Get the remaining PUK entries
    LE_ASSERT_OK(le_sim_GetRemainingPUKTries(CurrentSimId, &remainingPukTries));

    // Unblock the SIM (OK expected)
    LE_ASSERT(le_sim_Unblock(CurrentSimId, Puk, Pin) == LE_OK);
    // Get the remaining PIN tries
    LE_ASSERT(le_sim_GetRemainingPINTries(CurrentSimId) == INITIAL_PIN_TRY);

    // Get the remaining PUK entries
    LE_ASSERT_OK(le_sim_GetRemainingPUKTries(CurrentSimId, &triesPuk));
    LE_ASSERT(0 == remainingPukTries-triesPuk);

    // Wait the handlers' calls (SIM is supposed to be in READY state now)
    SynchTest();

    // Return into INSERTED state: check PIN
    CurrentSimState = LE_SIM_INSERTED;
    pa_simSimu_ReportSIMState(CurrentSimState);

    // Wait the handlers' calls
    SynchTest();

    // Check state handler result
    CheckStateHandlerResult();

    // Now, try to enter the PIN (bad pin code, error expected)
    LE_ASSERT(le_sim_EnterPIN(CurrentSimId, ShortPin) == LE_UNDERFLOW);

    // Enter the correct PIN (OK expected)
    CurrentSimState = LE_SIM_READY;
    LE_ASSERT(le_sim_EnterPIN(CurrentSimId, Pin) == LE_OK);

    // Wait the handlers' calls
    SynchTest();

    // Check that we are in the expected states
    CheckSimStates();

    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test lock, unlock and change PIN
 *
 * API tested:
 * - le_sim_Lock
 * - le_sim_Unlock
 * - le_sim_ChangePIN
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestSim_LockUnlockChange
(
    void
)
{
    // Go into ABSENT state
    CurrentSimState = LE_SIM_ABSENT;
    pa_simSimu_ReportSIMState(CurrentSimState);

    // Wait the handlers' calls
    SynchTest();

    // Check state handler result
    CheckStateHandlerResult();

    // Check the states
    CheckSimStates();

    // Try to lock/unlock/change PIN w/o SIN inserted: errors are expected
    LE_ASSERT(le_sim_Unlock(CurrentSimId,Pin) == LE_NOT_FOUND);
    LE_ASSERT(le_sim_Lock(CurrentSimId,Pin) == LE_NOT_FOUND);
    LE_ASSERT(le_sim_ChangePIN(CurrentSimId,Pin,NewPin) == LE_NOT_FOUND);

    // Go in READY state
    CurrentSimState = LE_SIM_READY;
    pa_simSimu_ReportSIMState(CurrentSimState);

    // Wait the handlers' calls
    SynchTest();

    // Check state handler result
    CheckStateHandlerResult();

    // Try to lock/unclock/Change PIN with bad PIN: errors are expected
    LE_ASSERT(le_sim_Lock(CurrentSimId, BadPin) == LE_FAULT);
    LE_ASSERT(le_sim_Unlock(CurrentSimId, BadPin) == LE_FAULT);
    LE_ASSERT(le_sim_ChangePIN(CurrentSimId,ShortPin,NewPin) == LE_UNDERFLOW);
    LE_ASSERT(le_sim_ChangePIN(CurrentSimId,Pin,ShortPin) == LE_UNDERFLOW);
    LE_ASSERT(le_sim_ChangePIN(CurrentSimId,ShortPin,ShortPin) == LE_UNDERFLOW);
    LE_ASSERT(le_sim_ChangePIN(CurrentSimId,BadPin,NewPin) == LE_FAULT);

    // Lock/unlock/change PIN with correct values: OK is expected
    LE_ASSERT(le_sim_Lock(CurrentSimId,Pin) == LE_OK);
    LE_ASSERT(le_sim_Unlock(CurrentSimId,Pin) == LE_OK);
    LE_ASSERT(le_sim_ChangePIN(CurrentSimId,Pin,NewPin) == LE_OK);

    // Check that all handlers have been called as expected
    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test SIM card information
 *
 * API tested:
 * - le_sim_GetICCID
 * - le_sim_GetIMSI
 * - le_sim_GetSubscriberPhoneNumber
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestSim_SimCardInformation
(
    void
)
{
    char iccid[PA_SIM_CARDID_MAX_LEN+1];
    char imsi[PA_SIM_IMSI_MAX_LEN+1];
    char phoneNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];

    // Start in ABSENT state
    CurrentSimState = LE_SIM_ABSENT;
    pa_simSimu_ReportSIMState(CurrentSimState);

    // Wait the handlers' calls
    SynchTest();

    // Check state handler result
    CheckStateHandlerResult();

    // Try to get information: error are expected as there's no SIM
    LE_ASSERT( le_sim_GetICCID(CurrentSimId, iccid, PA_SIM_CARDID_MAX_LEN) == LE_FAULT );
    LE_ASSERT( le_sim_GetIMSI(CurrentSimId, imsi, PA_SIM_IMSI_MAX_LEN) == LE_FAULT );
    LE_ASSERT( le_sim_GetSubscriberPhoneNumber( CurrentSimId,
                                                phoneNumber,
                                                LE_MDMDEFS_PHONE_NUM_MAX_BYTES ) == LE_FAULT );

    // SIM is now ready
    CurrentSimState = LE_SIM_READY;
    pa_simSimu_ReportSIMState(CurrentSimState);

    // Wait the handlers' calls
    SynchTest();

    // Check state handler result
    CheckStateHandlerResult();

    // Try to get informations, check values (OK expected)
    LE_ASSERT( le_sim_GetICCID(CurrentSimId, iccid, PA_SIM_CARDID_MAX_LEN+1) == LE_OK );
    LE_ASSERT( strncmp(iccid,Iccid,PA_SIM_CARDID_MAX_LEN) == 0 );
    LE_ASSERT( le_sim_GetIMSI(CurrentSimId, imsi, PA_SIM_IMSI_MAX_LEN+1) == LE_OK );
    LE_ASSERT( strncmp(imsi,Imsi,PA_SIM_IMSI_MAX_LEN) == 0 );
    LE_ASSERT( le_sim_GetSubscriberPhoneNumber( CurrentSimId,
                                                phoneNumber,
                                                LE_MDMDEFS_PHONE_NUM_MAX_BYTES ) == LE_OK );
    LE_ASSERT( strncmp(phoneNumber,PhoneNum,LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == 0 );

    // Try to get infomartions with a too small buffer (error expected)
    LE_ASSERT( le_sim_GetICCID(CurrentSimId, iccid, PA_SIM_CARDID_MAX_LEN) == LE_OVERFLOW );
    LE_ASSERT( le_sim_GetIMSI(CurrentSimId, imsi, PA_SIM_IMSI_MAX_LEN) == LE_OVERFLOW );

    // Check that all handlers have been called as expected
    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test home network API
 *
 * API tested:
 * - le_sim_GetHomeNetworkMccMnc
 * - le_sim_GetHomeNetworkOperator
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestSim_HomeNetwork
(
    void
)
{
    char mcc[LE_MRC_MCC_BYTES];
    char mnc[LE_MRC_MNC_BYTES];
    char homeNetwork[20];


    // Start in ABSENT state
    CurrentSimState = LE_SIM_ABSENT;
    pa_simSimu_ReportSIMState(CurrentSimState);

    // Wait the handlers' calls
    SynchTest();

    // Check state handler result
    CheckStateHandlerResult();

    // Try to get home network: error are expected as there's no SIM
    LE_ASSERT(le_sim_GetHomeNetworkMccMnc(CurrentSimId,mcc,LE_MRC_MCC_BYTES,mnc,LE_MRC_MNC_BYTES)
                                                                                    == LE_FAULT);
    LE_ASSERT(le_sim_GetHomeNetworkOperator(CurrentSimId,homeNetwork,20) == LE_FAULT);

    // SIM is now ready
    CurrentSimState = LE_SIM_READY;
    pa_simSimu_ReportSIMState(CurrentSimState);

    // Wait the handlers' calls
    SynchTest();

    // Check state handler result
    CheckStateHandlerResult();

    // Try to get home network, check values (OK expected)
    LE_ASSERT(le_sim_GetHomeNetworkMccMnc(CurrentSimId,mcc,LE_MRC_MCC_BYTES,mnc,LE_MRC_MNC_BYTES)
                                                                                    == LE_OK);
    LE_ASSERT(strcmp(mcc,Mcc)==0);
    LE_ASSERT(strcmp(mnc,Mnc)==0);
    LE_ASSERT(le_sim_GetHomeNetworkOperator(CurrentSimId,homeNetwork,20) == LE_OK);
    LE_ASSERT(strcmp(homeNetwork,Operator)==0);

    // Check that all handlers have been called as expected
    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test sim toolkit
 *
 * API tested:
 * - le_sim_AddSimToolkitEventHandler
 * - le_sim_AcceptSimToolkitCommand
 * - le_sim_RejectSimToolkitCommand
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestSim_Stk
(
    void
)
{
    // Stk handler semaphore
    StkHandlerSem = (le_sem_Ref_t) le_sem_Create("StkHandlerSem",1);

    // each thread subscribes to state handler using le_sim_AddSimToolkitEventHandler
    // This API has to be called by threads
    int i=0;
    for (; i<NB_CLIENT; i++)
    {
        le_event_QueueFunctionToThread( AppCtx[i].appThreadRef,
                                        AddStkHandler,
                                        &AppCtx[i],
                                        NULL );
    }

    // Wait for the tasks
    SynchTest();

    StkEvent = LE_SIM_REFRESH;

    // Invoke Stk event
    pa_simSimu_ReportSTKEvent(StkEvent);

    // Wait for the call of the handlers
    SynchTest();

    // Check the result
    for (i=0; i<NB_CLIENT; i++)
    {
        LE_ASSERT(AppCtx[i].stkEvent == StkEvent);
        AppCtx[i].stkEvent = 0;
    }

    // Test le_sim_AcceptSimToolkitCommand and le_sim_RejectSimToolkitCommand
    pa_simSimu_SetExpectedSTKConfirmationCommand(true);
    LE_ASSERT( le_sim_AcceptSimToolkitCommand(CurrentSimId) == LE_OK );
    pa_simSimu_SetExpectedSTKConfirmationCommand(false);
    LE_ASSERT( le_sim_RejectSimToolkitCommand(CurrentSimId) == LE_OK );

    // Check that all handlers have been called as expected
    LE_ASSERT( le_sem_GetValue(ThreadSemaphore) == 0 );
}

//--------------------------------------------------------------------------------------------------
/**
 * Test the multi-profile eUICC swap
 *
 * API tested:
 * - le_sim_LocalSwapToCommercialSubscription
 * - le_sim_LocalSwapToEmergencyCallSubscription
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestSim_LocalSwap
(
    void
)
{
    int i;
    int doItAgain = 2;

    // Swap functions may generate an automatically accepted Refresh command
    pa_simSimu_SetExpectedSTKConfirmationCommand(true);

    while (doItAgain)
    {
        doItAgain--;

        // The thread is calling le_sim_LocalSwapToCommercialSubscription and
        // le_sim_LocalSwapToEmergencyCallSubscription (that's why we do this operation twice)
        le_event_QueueFunctionToThread( AppCtx[0].appThreadRef,
                                        LocalSwap,
                                        &AppCtx[0],
                                        NULL );

        // Wait a while for le_sim treatment
        sleep(1);

        // There's a semaphore in the le_sim to wait for the refresh => report refresh event
        StkEvent = LE_SIM_REFRESH;
        // Invoke Stk event
        pa_simSimu_ReportSTKEvent(StkEvent);

        // Wait for the call of the handlers
        SynchTest();

        // Check the result
        for (i=0; i<NB_CLIENT; i++)
        {
            LE_ASSERT(AppCtx[i].stkEvent == StkEvent);
            AppCtx[i].stkEvent = 0;
        }

        // Check that all handlers have been called as expected
        LE_ASSERT( le_sem_GetValue(ThreadSemaphore) == 0 );
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test remove handlers
 *
 * API tested:
 * - le_sim_RemoveNewStateHandler
 * - le_sim_RemoveSimToolkitEventHandler
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestSim_RemoveHandlers
(
    void
)
{
    int i;

    // Remove handlers: add le_sim_RemoveNewStateHandler and le_sim_RemoveSimToolkitEventHandler
    // to the eventLoop of tasks
    for (i=0; i<NB_CLIENT; i++)
    {
        le_event_QueueFunctionToThread( AppCtx[i].appThreadRef,
                                        RemoveHandler,
                                        &AppCtx[i],
                                        NULL );
    }

    // Wait for the tasks
    SynchTest();

    // Provoke events which called the handlers (sim state event, and stk event)

    // Go into ABSENT state
    CurrentSimState = LE_SIM_ABSENT;
    pa_simSimu_ReportSIMState(CurrentSimState);

    // Invoke Stk event, and check that no handler is called
    pa_simSimu_ReportSTKEvent(StkEvent);

    // Wait for the semaphore timeout to check that handlers are not called
    LE_ASSERT( le_sem_WaitWithTimeOut(ThreadSemaphore, TimeToWait) == LE_TIMEOUT );
}

//--------------------------------------------------------------------------------------------------
/**
 * Test SIM access
 *
 * API tested:
 * - le_sim_SendApdu
 * - le_sim_RemoveSimToolkitEventHandler
 *
 * Exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestSim_SimAccess
(
    void
)
{
    pa_simSimu_SetSIMAccessTest(true);

    uint8_t apdu[]={ 0x00, 0xA4, 0x00, 0x0C, 0x02, 0x6F, 0x07 };
    uint8_t simResponse[10];
    size_t rspLen = sizeof(simResponse);
    uint8_t expectedResult[] = {0x90, 0x00};

    // Test le_sim_SendApdu() API
    LE_ASSERT( le_sim_SendApdu( CurrentSimId,
                                apdu,
                                sizeof(apdu),
                                simResponse,
                                &rspLen) == LE_OK );

    LE_ASSERT( rspLen == sizeof(expectedResult) );
    LE_ASSERT( memcmp(simResponse,expectedResult, rspLen) == 0 );

    // Test pa_sim_SendSimCommand() API
    uint8_t p1=1,p2=2,p3=3;
    uint8_t dataAdn[] = {0x4A,0x61,0x63,0x6B,0x79,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x05,
                     0x81,0x10,0x92,0x90,0x71,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    rspLen = 100;
    uint8_t rsp[rspLen];

    struct
    {
        le_sim_Command_t command;
        le_result_t      result;
    } test[LE_SIM_COMMAND_MAX+1] = {
                    {
                        LE_SIM_READ_RECORD,
                        LE_OK
                    },
                    {
                        LE_SIM_READ_BINARY,
                        LE_OK
                    },
                    {
                        LE_SIM_UPDATE_RECORD,
                        LE_OK
                    },
                    {
                        LE_SIM_UPDATE_BINARY,
                        LE_OK
                    },
                    {
                        LE_SIM_COMMAND_MAX,
                        LE_BAD_PARAMETER
                    }
               };

    int runningTest = 0;
    uint8_t swi1, swi2;

    for (; runningTest <= LE_SIM_COMMAND_MAX; runningTest++)
    {
        swi1 = swi2 = 0;

        LE_ASSERT( le_sim_SendCommand(CurrentSimId,
                                      test[runningTest].command,
                                      "6F3A",
                                      p1,
                                      p2,
                                      p3,
                                      dataAdn,
                                      sizeof(dataAdn),
                                      "3F007F10",
                                      &swi1,
                                      &swi2,
                                      rsp,
                                      &rspLen) == test[runningTest].result );

        if (test[runningTest].result == LE_OK)
        {
            LE_ASSERT(swi1 == 0x90);
            LE_ASSERT(swi2 == 0x00);
        }
    }

    pa_simSimu_SetSIMAccessTest(false);
}

//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // To reactivate for all DEBUG logs
 //   le_log_SetFilterLevel(LE_LOG_DEBUG);

    // Init pa simu
    pa_simSimu_Init();

    // Set Sim card info in the pa_simu
    SetSimCardInfo();

    // Init le_sim
    le_sim_Init();

    LE_INFO("======== Start UnitTest of SIM API ========");

    LE_INFO("======== AddHandlers Test  ========");
    TestSim_AddHandlers();

    LE_INFO("======== PIN/PUK Test  ========");
    TestSim_PinPuk();

    LE_INFO("======== lock/unlock/change pin Test  ========");
    TestSim_LockUnlockChange();

    LE_INFO("======== Sim card information Test  ========");
    TestSim_SimCardInformation();

    LE_INFO("======== Home network Test  ========");
    TestSim_HomeNetwork();

    LE_INFO("======== Sim toolkit Test  ========");
    TestSim_Stk();

    LE_INFO("======== multi-profile Test  ========");
    TestSim_LocalSwap();

    LE_INFO("======== SIM access Test  ========");
    TestSim_SimAccess();

    LE_INFO("======== Handlers removal Test  ========");
    TestSim_RemoveHandlers();

    LE_INFO("======== UnitTest of SIM API ends with SUCCESS ========");
    exit(0);
}
