 /**
  * This module implements the call waiting supplementary service test.
  *
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include "interfaces.h"

#define EVENT_STRING(X) { LE_MCC_EVENT_ ## X, STRINGIZE_EXPAND(LE_MCC_EVENT_ ## X) }
#define TEST_FUNCTION(X) { #X, X }

//--------------------------------------------------------------------------------------------------
/**
 * Call Reference.
 */
//--------------------------------------------------------------------------------------------------
static le_mcc_CallRef_t OutgoingCallRef;

//--------------------------------------------------------------------------------------------------
/**
 * Phone numbers.
 */
//--------------------------------------------------------------------------------------------------
static char  OutgoingNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
static char  IncomingNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];

//--------------------------------------------------------------------------------------------------
/**
 * LE_MCC_EVENT string table
 */
//--------------------------------------------------------------------------------------------------
struct
{
    le_mcc_Event_t event;
    char           eventString[50];
} EventToEventString[LE_MCC_EVENT_MAX] = {  EVENT_STRING(SETUP),
                                            EVENT_STRING(INCOMING),
                                            EVENT_STRING(ORIGINATING),
                                            EVENT_STRING(ALERTING),
                                            EVENT_STRING(CONNECTED),
                                            EVENT_STRING(TERMINATED),
                                            EVENT_STRING(WAITING),
                                            EVENT_STRING(ON_HOLD)
                                        };

//--------------------------------------------------------------------------------------------------
/**
 * Test automaton function prototype
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TestFunction_t)
(
    le_mcc_CallRef_t  callRef,
    le_mcc_Event_t callEvent
);


//--------------------------------------------------------------------------------------------------
/**
 * Test structure definition
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char functionString[50];
    TestFunction_t function;
}  TestFunctionDef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Tests functions prototype
 */
//--------------------------------------------------------------------------------------------------
static void TestOutgoingCallConnected(le_mcc_CallRef_t, le_mcc_Event_t);
static void TestIncomingCallConnected(le_mcc_CallRef_t, le_mcc_Event_t);
static void TestCallsSwitched(le_mcc_CallRef_t, le_mcc_Event_t);
static void ReleaseCallOnHold(le_mcc_CallRef_t, le_mcc_Event_t);
static void ReleaseCallWaiting(le_mcc_CallRef_t, le_mcc_Event_t);
static void ReleaseCallConnected(le_mcc_CallRef_t, le_mcc_Event_t);
static void TestEnd(le_mcc_CallRef_t, le_mcc_Event_t);
static void TestActiveOnHoldAndRelease(le_mcc_CallRef_t, le_mcc_Event_t);
static void TestIncomingCallRejected(le_mcc_CallRef_t, le_mcc_Event_t);

//--------------------------------------------------------------------------------------------------
/**
 * tests of activated call waiting supplementary service
 */
//--------------------------------------------------------------------------------------------------
static TestFunctionDef_t TestFunctionsCallWaitingActivated[] =
    {
        TEST_FUNCTION(TestOutgoingCallConnected),
        TEST_FUNCTION(TestIncomingCallConnected),
        TEST_FUNCTION(TestCallsSwitched),
        TEST_FUNCTION(ReleaseCallOnHold),
        TEST_FUNCTION(ReleaseCallWaiting),
        TEST_FUNCTION(ReleaseCallConnected),
        TEST_FUNCTION(TestActiveOnHoldAndRelease),
        TEST_FUNCTION(TestEnd),
        TEST_FUNCTION(NULL),
    };

//--------------------------------------------------------------------------------------------------
/**
 * tests of deactivated call waiting supplementary service
 */
//--------------------------------------------------------------------------------------------------
static TestFunctionDef_t TestFunctionsCallWaitingDeactivated[] =
    {
        TEST_FUNCTION(TestIncomingCallRejected),
        TEST_FUNCTION(TestEnd),
        TEST_FUNCTION(NULL),
    };

//--------------------------------------------------------------------------------------------------
/**
 * Test state
 */
//--------------------------------------------------------------------------------------------------
static uint32_t TestState = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t Semaphore;

//--------------------------------------------------------------------------------------------------
/**
 * Test: First outgoing call connected
 */
//--------------------------------------------------------------------------------------------------
static void TestOutgoingCallConnected
(
    le_mcc_CallRef_t  callRef,
    le_mcc_Event_t callEvent
)
{
    if (callEvent == LE_MCC_EVENT_CONNECTED)
    {
        char telNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];

        LE_ASSERT(le_mcc_GetRemoteTel(callRef, telNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == LE_OK);
        LE_ASSERT( strncmp(telNumber, OutgoingNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == 0 );
        LE_INFO("First incoming call connected. Now, make an incoming call");
        TestState++;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Incoming call is waiting, then connect. Outgoing call is put on hold
 */
//--------------------------------------------------------------------------------------------------
static void TestIncomingCallConnected
(
    le_mcc_CallRef_t  callRef,
    le_mcc_Event_t callEvent
)
{
    char telNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    static uint32_t receivedCallEventCount = 0;

    memset(telNumber, 0, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);

    LE_ASSERT(le_mcc_GetRemoteTel(callRef, telNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == LE_OK);

    if (callEvent == LE_MCC_EVENT_WAITING)
    {
        LE_INFO("Answer call waiting. Previous call we be placed on hold");
        LE_ASSERT(le_mcc_ActivateCall(callRef) == LE_OK);
        strncpy(IncomingNumber, telNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
    }
    else if (callEvent == LE_MCC_EVENT_ON_HOLD)
    {
        // outgoing call is on hold
        LE_ASSERT( strncmp(telNumber, OutgoingNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == 0 );
        receivedCallEventCount++;
    }
    else if (callEvent == LE_MCC_EVENT_CONNECTED)
    {
        // incoming call is active
        LE_ASSERT( strncmp(telNumber, IncomingNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == 0 );
        receivedCallEventCount++;
    }

    if (receivedCallEventCount == 2)
    {
        sleep(5);
        LE_INFO("Switch both calls");
        LE_ASSERT(le_mcc_ActivateCall(callRef) == LE_OK);
        TestState++;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Switch calls: incoming call on hold, outgoing call active
 */
//--------------------------------------------------------------------------------------------------
static void TestCallsSwitched
(
    le_mcc_CallRef_t  callRef,
    le_mcc_Event_t callEvent
)
{
    static uint32_t receivedCallEventCount = 0;
    char telNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    memset(telNumber,0,LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
    static le_mcc_CallRef_t incomingCallRef = NULL;

    LE_ASSERT(le_mcc_GetRemoteTel(callRef, telNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == LE_OK);

    if (callEvent == LE_MCC_EVENT_ON_HOLD)
    {
        // incoming call is on hold
        LE_ASSERT( strncmp(telNumber, IncomingNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == 0 );
        incomingCallRef = callRef;
        receivedCallEventCount++;
    }
    else if (callEvent == LE_MCC_EVENT_CONNECTED)
    {
        // outgoing call is active
        LE_ASSERT( strncmp(telNumber, OutgoingNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == 0 );
        receivedCallEventCount++;
    }

    if (receivedCallEventCount == 2)
    {
        LE_INFO("switch done");
        sleep(5);
        LE_INFO("Hang-up incoming call");
        TestState++;
        LE_ASSERT(incomingCallRef != NULL);
        le_mcc_HangUp(incomingCallRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: release on hold call call (incoming call)
 */
//--------------------------------------------------------------------------------------------------
static void ReleaseCallOnHold
(
    le_mcc_CallRef_t  callRef,
    le_mcc_Event_t callEvent
)
{
    char telNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    memset(telNumber,0,LE_MDMDEFS_PHONE_NUM_MAX_BYTES);

    LE_ASSERT(le_mcc_GetRemoteTel(callRef, telNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == LE_OK);

    if (callEvent == LE_MCC_EVENT_TERMINATED)
    {
        // incoming call is released
        LE_ASSERT( strncmp(telNumber, IncomingNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == 0 );
        le_mcc_Delete(callRef);

        // Re-dial
        LE_INFO("Dial again the target");
        TestState++;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Release new incoming (waiting) call
 */
//--------------------------------------------------------------------------------------------------
static void ReleaseCallWaiting
(
    le_mcc_CallRef_t  callRef,
    le_mcc_Event_t callEvent
)
{
    char telNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    memset(telNumber,0,LE_MDMDEFS_PHONE_NUM_MAX_BYTES);

    LE_ASSERT(le_mcc_GetRemoteTel(callRef, telNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == LE_OK);

    if (callEvent == LE_MCC_EVENT_WAITING)
    {
        LE_ASSERT( strncmp(telNumber, IncomingNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == 0 );
        LE_INFO("Release call waiting");
        le_mcc_HangUp(callRef);
    }
    else if (callEvent == LE_MCC_EVENT_TERMINATED)
    {
        LE_ASSERT( strncmp(telNumber, IncomingNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == 0 );
        // Re-dial
        LE_INFO("Dial again the target");
        le_mcc_Delete(callRef);
        TestState++;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Release active call (incoming call) when a call is on hold
 */
//--------------------------------------------------------------------------------------------------
static void ReleaseCallConnected
(
    le_mcc_CallRef_t  callRef,
    le_mcc_Event_t callEvent
)
{
    static uint32_t receivedCallEventCount = 0;
    char telNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    memset(telNumber,0,LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
    static le_mcc_CallRef_t incomingCallRef = NULL;

    LE_ASSERT(le_mcc_GetRemoteTel(callRef, telNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == LE_OK);

    if (callEvent == LE_MCC_EVENT_WAITING)
    {
        LE_INFO("Answer call waiting");
        LE_ASSERT( strncmp(telNumber, IncomingNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == 0 );
        LE_ASSERT(le_mcc_ActivateCall(callRef) == LE_OK);
        receivedCallEventCount++;
    }
    else if (callEvent == LE_MCC_EVENT_CONNECTED)
    {
        // incoming call is active
        LE_ASSERT( strncmp(telNumber, IncomingNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == 0 );
        incomingCallRef = callRef;
        sleep(5);
        receivedCallEventCount++;
    }
    else if (callEvent == LE_MCC_EVENT_ON_HOLD)
    {
        // outgoing call is on hold
        LE_ASSERT( strncmp(telNumber, OutgoingNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == 0 );
        receivedCallEventCount++;
    }

    if (receivedCallEventCount == 3)
    {
        LE_ASSERT(incomingCallRef != NULL);
        le_mcc_HangUp(incomingCallRef);
        TestState++;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Set the on hold call (outgoing call) active and release
 */
//--------------------------------------------------------------------------------------------------
static void TestActiveOnHoldAndRelease
(
    le_mcc_CallRef_t  callRef,
    le_mcc_Event_t callEvent
)
{
    char telNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    memset(telNumber,0,LE_MDMDEFS_PHONE_NUM_MAX_BYTES);

    LE_ASSERT(le_mcc_GetRemoteTel(callRef, telNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == LE_OK);

    if (callEvent == LE_MCC_EVENT_TERMINATED)
    {
        LE_ASSERT( strncmp(telNumber, IncomingNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == 0 );
        le_mcc_Delete(callRef);
        sleep(5);
        // Activate the on hold call
        LE_ASSERT(le_mcc_ActivateCall(OutgoingCallRef) == LE_OK);
    }
    else if (callEvent == LE_MCC_EVENT_CONNECTED)
    {
        LE_ASSERT( strncmp(telNumber, OutgoingNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == 0 );
        sleep(5);
        // End call
        le_mcc_HangUp(callRef);
        TestState++;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: end
 */
//--------------------------------------------------------------------------------------------------
static void TestEnd
(
    le_mcc_CallRef_t  callRef,
    le_mcc_Event_t callEvent
)
{
    char telNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    memset(telNumber,0,LE_MDMDEFS_PHONE_NUM_MAX_BYTES);

    LE_ASSERT(le_mcc_GetRemoteTel(callRef, telNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == LE_OK);

    if (callEvent == LE_MCC_EVENT_TERMINATED)
    {
        LE_ASSERT( strncmp(telNumber, OutgoingNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == 0 );
        le_mcc_Delete(callRef);
        le_sem_Post(Semaphore);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: call waiting deactivated. No call waiting event should be received
 */
//--------------------------------------------------------------------------------------------------
static void TestIncomingCallRejected
(
    le_mcc_CallRef_t  callRef,
    le_mcc_Event_t callEvent
)
{
    if (callEvent == LE_MCC_EVENT_CONNECTED)
    {
        char telNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];

        LE_ASSERT(le_mcc_GetRemoteTel(callRef, telNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == LE_OK);
        LE_ASSERT( strncmp(telNumber, OutgoingNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES) == 0 );
        LE_INFO("First incoming call connected. Now, make an incoming call (should be rejected)");
        TestState++;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Call Event Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void CallWaitingTestEventHandler
(
    le_mcc_CallRef_t  callRef,
    le_mcc_Event_t callEvent,
    void* contextPtr
)
{
    TestFunctionDef_t* testFunctionsArrayPtr = contextPtr;

    LE_ASSERT(callEvent < LE_MCC_EVENT_MAX);

    if (EventToEventString[callEvent].event != callEvent)
    {
        LE_ERROR("Issue in EventToEventString, please revue the test");
        exit(EXIT_FAILURE);
    }

    LE_INFO( "%s", EventToEventString[callEvent].eventString);

    if (testFunctionsArrayPtr[TestState].function)
    {
        LE_INFO("Call %s", testFunctionsArrayPtr[TestState].functionString);
        testFunctionsArrayPtr[TestState].function(callRef, callEvent);
    }
    else
    {
        LE_ERROR("Missing test function");
        exit(EXIT_FAILURE);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: call waiting/call on hold.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Test_CallWaitingState
(
    void
)
{
    bool callWaitingStatus, callWaitingStatusUpdated;

    // Get the current call waiting supplementary service status
    LE_ASSERT(le_mcc_GetCallWaitingService(&callWaitingStatus) == LE_OK);
    if (callWaitingStatus == true)
    {
        callWaitingStatusUpdated = false;
    }
    else
    {
        callWaitingStatusUpdated = true;
    }

    // swap the satus
    LE_ASSERT(le_mcc_SetCallWaitingService(callWaitingStatusUpdated) == LE_OK);
    LE_ASSERT(le_mcc_GetCallWaitingService(&callWaitingStatusUpdated) == LE_OK);
    if (callWaitingStatus == true)
    {
        LE_ASSERT(callWaitingStatusUpdated == false);
        callWaitingStatusUpdated = true;
    }
    else
    {
        LE_ASSERT(callWaitingStatusUpdated == true);
        callWaitingStatusUpdated = false;
    }

    // swap again the status
    LE_ASSERT(le_mcc_SetCallWaitingService(callWaitingStatusUpdated) == LE_OK);
    LE_ASSERT(le_mcc_GetCallWaitingService(&callWaitingStatusUpdated) == LE_OK);
    LE_ASSERT(callWaitingStatusUpdated == callWaitingStatus);
}


//--------------------------------------------------------------------------------------------------
/**
 * Thread for deactivated call waiting supplementary service tests
 *
 */
//--------------------------------------------------------------------------------------------------
static void* TestCallWaitingActivatedThread
(
    void* ctxPtr
)
{
    le_mcc_ConnectService();

    TestState = 0;

    OutgoingCallRef = le_mcc_Create(OutgoingNumber);
    LE_ASSERT(OutgoingCallRef != NULL);

    LE_ASSERT(le_mcc_AddCallEventHandler(CallWaitingTestEventHandler,
                                         TestFunctionsCallWaitingActivated) != NULL);

    // activate call waiting
    LE_ASSERT(le_mcc_SetCallWaitingService(true) == LE_OK);

    // start call
    LE_ASSERT(le_mcc_Start(OutgoingCallRef) == LE_OK );

    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread for activated call waiting supplementary service tests
 *
 */
//--------------------------------------------------------------------------------------------------
static void* TestCallWaitingDeactivatedThread
(
    void* ctxPtr
)
{
    le_mcc_ConnectService();

    TestState = 0;

    OutgoingCallRef = le_mcc_Create(OutgoingNumber);
    LE_ASSERT(OutgoingCallRef != NULL);

    LE_ASSERT(le_mcc_AddCallEventHandler(CallWaitingTestEventHandler,
                                         TestFunctionsCallWaitingDeactivated) != NULL);

    // activate call waiting
    LE_ASSERT(le_mcc_SetCallWaitingService(false) == LE_OK);

    // start call
    LE_ASSERT(le_mcc_Start(OutgoingCallRef) == LE_OK );

    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test when call waiting supplementary service is activated
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestCallWaitingActivated
(
    void
)
{
    le_thread_Ref_t threadRef = le_thread_Create("TestCallWaitingActivated",
                                     TestCallWaitingActivatedThread, NULL);

    le_thread_Start(threadRef);

    le_sem_Wait(Semaphore);

    le_thread_Cancel(threadRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test when call waiting supplementary service is deactivated
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestCallWaitingDeactivated
(
    void
)
{
    le_thread_Ref_t threadRef = le_thread_Create("TestCallWaitingDeactivated",
                                     TestCallWaitingDeactivatedThread, NULL);

    le_thread_Start(threadRef);

    le_sem_Wait(Semaphore);

    le_thread_Cancel(threadRef);
}

//--------------------------------------------------------------------------------------------------
/*
 * COMPONENT INIT
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    int nbArgument = 0;

    nbArgument = le_arg_NumArgs();

    Semaphore = le_sem_Create("CallWaitingSem",0);

    if (nbArgument == 1)
    {
        // This function gets the telephone number from the User (interactive case).
        const char* phoneNumber = le_arg_GetArg(0);
        if (NULL == phoneNumber)
        {
            LE_ERROR("phoneNumber is NULL");
            exit(EXIT_FAILURE);
        }
        strncpy(OutgoingNumber, phoneNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);

        LE_INFO("Phone number %s", OutgoingNumber);

        // Test: call waiting state
        Test_CallWaitingState();

        // Test: call waiting supplementary service activated
        TestCallWaitingActivated();

        // Test: call waiting supplementary service activated
        TestCallWaitingDeactivated();

        exit(EXIT_SUCCESS);
    }
    else
    {
        LE_ERROR("PRINT USAGE => app runProc mccCallWaitingTest --exe=mccCallWaitingTest -- \
                <Destination phone number>");
        exit(EXIT_FAILURE);
    }
}
