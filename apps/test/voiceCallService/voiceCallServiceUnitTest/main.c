/**
 * This module implements the unit tests for VOICE CALL API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Long semaphore timeout in seconds
 */
//--------------------------------------------------------------------------------------------------
#define LONG_TIMEOUT    20

//--------------------------------------------------------------------------------------------------
/**
 * Expected voice call event
 */
//--------------------------------------------------------------------------------------------------
static le_voicecall_Event_t ExpectedVoiceCallEvent;

//--------------------------------------------------------------------------------------------------
/**
 * Destination Phone number.
 */
//--------------------------------------------------------------------------------------------------
static char  DestinationNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];

// -------------------------------------------------------------------------------------------------
/**
 *  Application context structure
 */
// -------------------------------------------------------------------------------------------------
typedef struct
{
    le_sem_Ref_t                        appSemaphore;
    le_thread_Ref_t                     appThreadRef;
    le_voicecall_StateHandlerRef_t      appStateHandlerRef;
    le_voicecall_CallRef_t              appRequestRef;
} AppContext_t;

// -------------------------------------------------------------------------------------------------
/**
 *  Application contexts
 */
// -------------------------------------------------------------------------------------------------
static AppContext_t AppCtx;

//--------------------------------------------------------------------------------------------------
/**
 * Simulate MCC call event state and set expected voice call state
 */
//--------------------------------------------------------------------------------------------------
static void SimulateMccStateAndSetExpectedVoiceCallEvent
(
    le_mcc_Event_t event,
    le_voicecall_Event_t voiceCallEvent
)
{
    LE_DEBUG("Simulate MCC event %d", event);
    ExpectedVoiceCallEvent = voiceCallEvent;
    le_mccTest_SimulateState(event);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Call Event Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void VoiceCallStateHandler
(
    le_voicecall_CallRef_t reference,
    const char* identifier,
    le_voicecall_Event_t callEvent,
    void* contextPtr
)
{
    le_voicecall_TerminationReason_t term = LE_VOICECALL_TERM_UNDEFINED;

    LE_INFO("New Call event: %d for Call %p", callEvent, reference);
    LE_INFO("Expected Call event: %d", ExpectedVoiceCallEvent);

    switch (callEvent)
    {
        case LE_VOICECALL_EVENT_ALERTING:
        {
            LE_INFO("Event is LE_VOICECALL_EVENT_ALERTING.");
        }
        break;
        case LE_VOICECALL_EVENT_CONNECTED:
        {
            LE_INFO("Event is LE_VOICECALL_EVENT_CONNECTED.");
        }
        break;
        case LE_VOICECALL_EVENT_TERMINATED:
        {
            LE_INFO("Event is LE_VOICECALL_EVENT_TERMINATED.");
            LE_ASSERT_OK(le_voicecall_GetTerminationReason(reference, &term));

            switch (term)
            {
                case LE_VOICECALL_TERM_NETWORK_FAIL:
                {
                    LE_ERROR("Termination reason is LE_VOICECALL_TERM_NETWORK_FAIL");
                }
                break;

                case LE_VOICECALL_TERM_BUSY:
                {
                    LE_ERROR("Termination reason is LE_VOICECALL_TERM_BUSY");
                }
                break;

                case LE_VOICECALL_TERM_LOCAL_ENDED:
                {
                    LE_INFO("LE_VOICECALL_TERM_LOCAL_ENDED");
                }
                break;

                case LE_VOICECALL_TERM_REMOTE_ENDED:
                {
                    LE_INFO("Termination reason is LE_VOICECALL_TERM_REMOTE_ENDED");
                }
                break;

                case LE_VOICECALL_TERM_UNDEFINED:
                {
                    LE_INFO("Termination reason is LE_VOICECALL_TERM_UNDEFINED");
                }
                break;

                default:
                {
                    LE_ERROR("Termination reason is %d", term);
                }
                break;
            }
        }
        break;
        case LE_VOICECALL_EVENT_INCOMING:
        {
            LE_INFO("Event is LE_VOICECALL_EVENT_INCOMING.");

            // Update new call reference
            AppCtx.appRequestRef = reference;
        }
        break;
        case LE_VOICECALL_EVENT_CALL_END_FAILED:
        {
            LE_INFO("Event is LE_VOICECALL_EVENT_CALL_END_FAILED.");
        }
        break;
        case LE_VOICECALL_EVENT_CALL_ANSWER_FAILED:
        {
            LE_INFO("Event is LE_VOICECALL_EVENT_CALL_ANSWER_FAILED.");
        }
        break;
        default:
        {
            LE_ERROR("Unknown event %d.", callEvent);
        }
        break;
    }

    // Post the semaphore
    le_sem_Post(AppCtx.appSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Synchronize test thread (i.e. main) and application threads
 */
//--------------------------------------------------------------------------------------------------
static void SynchronizeTest
(
    void
)
{
    le_clk_Time_t timeToWait = {LONG_TIMEOUT, 0};

    LE_ASSERT_OK(le_sem_WaitWithTimeOut(AppCtx.appSemaphore, timeToWait));
}

//--------------------------------------------------------------------------------------------------
/**
 *  Thread used to simulate an application
 */
//--------------------------------------------------------------------------------------------------
static void* AppHandler
(
    void* ctxPtr
)
{
    AppContext_t* appCtxPtr = (AppContext_t*) ctxPtr;

    // Register handler for cellular network state change
    appCtxPtr->appStateHandlerRef = le_voicecall_AddStateHandler(VoiceCallStateHandler, ctxPtr);
    LE_ASSERT(NULL != appCtxPtr->appStateHandlerRef);
    LE_INFO("VoiceCallStateHandler %p added", appCtxPtr->appStateHandlerRef);

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(appCtxPtr->appSemaphore);

    // Run the event loop
    le_event_RunLoop();

    return NULL;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Request data connection
 */
// -------------------------------------------------------------------------------------------------
static void Testle_voicecall_Start
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_INFO("======== START Testle_voicecall_Start() ========");
    AppContext_t* appCtxPtr = (AppContext_t*) param1Ptr;

    LE_ASSERT_OK(le_utf8_Copy(DestinationNumber, "987654321",
                              LE_MDMDEFS_PHONE_NUM_MAX_BYTES, NULL));

    appCtxPtr->appRequestRef = le_voicecall_Start(DestinationNumber);
    LE_ASSERT(NULL != appCtxPtr->appRequestRef);

    LE_INFO("Received reference: %p", appCtxPtr->appRequestRef);

    le_sem_Post(appCtxPtr->appSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test : le_voicecall_Answer
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_voicecall_Answer
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_INFO("======== START Testle_voicecall_Answer() ========");
    AppContext_t* appCtxPtr = (AppContext_t*) param1Ptr;
    LE_ASSERT(LE_NOT_FOUND == le_voicecall_Answer(NULL));

    LE_ASSERT_OK(le_voicecall_Answer(appCtxPtr->appRequestRef));

    le_sem_Post(appCtxPtr->appSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test : le_voicecall_GetRxAudioStream
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_voicecall_GetRxAudioStream
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_INFO("======== START Testle_voicecall_GetRxAudioStream() ========");
    AppContext_t* appCtxPtr = (AppContext_t*) param1Ptr;

    LE_ASSERT(NULL == le_voicecall_GetRxAudioStream(NULL));

    LE_ASSERT(NULL != le_voicecall_GetRxAudioStream(appCtxPtr->appRequestRef));

    le_sem_Post(appCtxPtr->appSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test : le_voicecall_GetTxAudioStream
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_voicecall_GetTxAudioStream
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_INFO("======== START Testle_voicecall_GetTxAudioStream() ========");
    AppContext_t* appCtxPtr = (AppContext_t*) param1Ptr;

    LE_ASSERT(NULL == le_voicecall_GetTxAudioStream(NULL));

    LE_ASSERT(NULL != le_voicecall_GetTxAudioStream(appCtxPtr->appRequestRef));

    le_sem_Post(appCtxPtr->appSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test : le_voicecall_GetTerminationReason
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_voicecall_GetTerminationReason
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_INFO("======== START Testle_voicecall_GetTerminationReason() ========");
    AppContext_t* appCtxPtr = (AppContext_t*) param1Ptr;
    le_voicecall_TerminationReason_t termReason = LE_VOICECALL_TERM_UNDEFINED;

    LE_ASSERT(LE_NOT_FOUND == le_voicecall_GetTerminationReason(NULL, &termReason));

    LE_ASSERT_OK(le_voicecall_GetTerminationReason(appCtxPtr->appRequestRef, &termReason));
    LE_INFO("Voice call termination reason %d", termReason);

    le_sem_Post(appCtxPtr->appSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: le_voicecall_End().
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_voicecall_End
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_INFO("======== START Testle_voicecall_End() ========");
    AppContext_t* appCtxPtr = (AppContext_t*) param1Ptr;

    LE_ASSERT(LE_NOT_FOUND == le_voicecall_End(NULL));

    LE_ASSERT_OK(le_voicecall_End(appCtxPtr->appRequestRef));

    // Simulate termination reason
    le_mccTest_SimulateTerminationReason(LE_MCC_TERM_LOCAL_ENDED);

    le_sem_Post(appCtxPtr->appSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: le_voicecall_Delete().
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_voicecall_Delete
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_INFO("======== START Testle_voicecall_Delete() ========");
    AppContext_t* appCtxPtr = (AppContext_t*) param1Ptr;

    LE_ASSERT(LE_NOT_FOUND == le_voicecall_Delete(NULL));

    LE_ASSERT_OK(le_voicecall_Delete(appCtxPtr->appRequestRef));
    le_sem_Post(appCtxPtr->appSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test : le_voicecall_RemoveStateHandler
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_voicecall_RemoveStateHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_INFO("======== START Testle_voicecall_RemoveStateHandler() ========");
    AppContext_t* appCtxPtr = (AppContext_t*) param1Ptr;

    le_voicecall_RemoveStateHandler(appCtxPtr->appStateHandlerRef);
    le_sem_Post(appCtxPtr->appSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Create and start a voice call.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartVoiceCallTest
(
    void
)
{
    // Mcc initialization
    le_mcc_Init();

    // Initialize application contexts
    memset(&AppCtx, 0, sizeof(AppContext_t));

    // Create a semaphore to coordinate the test
    AppCtx.appSemaphore = le_sem_Create("voiceCallSem", 0);

    AppCtx.appThreadRef = le_thread_Create("voiceCallThread", AppHandler, &AppCtx);
    le_thread_Start(AppCtx.appThreadRef);
    // Wait for thread start
    SynchronizeTest();

    // Testle_voicecall_Start
    le_event_QueueFunctionToThread(AppCtx.appThreadRef,
                                   Testle_voicecall_Start, &AppCtx, NULL);
    SynchronizeTest();

    // Wait for le_mcc_Start().
    le_mccTest_SimulateWaitMccStart();

    SimulateMccStateAndSetExpectedVoiceCallEvent(LE_MCC_EVENT_ALERTING,
                                                 LE_VOICECALL_EVENT_ALERTING);
    SynchronizeTest();

    SimulateMccStateAndSetExpectedVoiceCallEvent(LE_MCC_EVENT_CONNECTED,
                                                 LE_VOICECALL_EVENT_CONNECTED);
    SynchronizeTest();

    // Testle_voicecall_GetRxAudioStream
    le_event_QueueFunctionToThread(AppCtx.appThreadRef,
                                   Testle_voicecall_GetRxAudioStream, &AppCtx, NULL);
    SynchronizeTest();

    // Testle_voicecall_GetTxAudioStream
    le_event_QueueFunctionToThread(AppCtx.appThreadRef,
                                   Testle_voicecall_GetTxAudioStream, &AppCtx, NULL);
    SynchronizeTest();

    // Testle_voicecall_End
    le_event_QueueFunctionToThread(AppCtx.appThreadRef,
                                   Testle_voicecall_End, &AppCtx, NULL);
    SynchronizeTest();

    SimulateMccStateAndSetExpectedVoiceCallEvent(LE_MCC_EVENT_TERMINATED,
                                                 LE_VOICECALL_EVENT_TERMINATED);
    SynchronizeTest();

    // Testle_voicecall_GetTerminationReason
    le_event_QueueFunctionToThread(AppCtx.appThreadRef,
                                   Testle_voicecall_GetTerminationReason, &AppCtx, NULL);
    SynchronizeTest();

    // Testle_voicecall_Delete
    le_event_QueueFunctionToThread(AppCtx.appThreadRef,
                                   Testle_voicecall_Delete, &AppCtx, NULL);
    SynchronizeTest();

    // Testle_voicecall_Answer
    SimulateMccStateAndSetExpectedVoiceCallEvent(LE_MCC_EVENT_INCOMING,
                                                LE_VOICECALL_EVENT_INCOMING);
    SynchronizeTest();

    le_event_QueueFunctionToThread(AppCtx.appThreadRef,
                                   Testle_voicecall_Answer, &AppCtx, NULL);
    SynchronizeTest();

    SimulateMccStateAndSetExpectedVoiceCallEvent(LE_MCC_EVENT_CONNECTED,
                                                 LE_VOICECALL_EVENT_CONNECTED);
    SynchronizeTest();

    // Testle_voicecall_End
    le_event_QueueFunctionToThread(AppCtx.appThreadRef,
                                   Testle_voicecall_End, &AppCtx, NULL);
    SynchronizeTest();

    SimulateMccStateAndSetExpectedVoiceCallEvent(LE_MCC_EVENT_TERMINATED,
                                                 LE_VOICECALL_EVENT_TERMINATED);
    SynchronizeTest();

    // Testle_voicecall_GetTerminationReason
    le_event_QueueFunctionToThread(AppCtx.appThreadRef,
                                   Testle_voicecall_GetTerminationReason, &AppCtx, NULL);
    SynchronizeTest();

    // Testle_voicecall_Delete
    le_event_QueueFunctionToThread(AppCtx.appThreadRef,
                                   Testle_voicecall_Delete, &AppCtx, NULL);
    SynchronizeTest();

    // Testle_voicecall_RemoveStateHandler
    le_event_QueueFunctionToThread(AppCtx.appThreadRef,
                                   Testle_voicecall_RemoveStateHandler, &AppCtx, NULL);
    SynchronizeTest();

    LE_INFO("======== UnitTest of VOICE CALL API FINISHED ========");

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This thread is used to start voice call unit tests
 */
//--------------------------------------------------------------------------------------------------
static void* VoiceCallUnitTestThread
(
    void* contextPtr
)
{
    LE_INFO("VoiceCall UT Thread Started");

    StartVoiceCallTest();

    exit(EXIT_SUCCESS);

    return NULL;
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
    // le_log_SetFilterLevel(LE_LOG_DEBUG);

    LE_INFO("======== START UnitTest of VOICE CALL API ========");

    // Start the unit test thread
    le_thread_Start(le_thread_Create("VoiceCall UT Thread", VoiceCallUnitTestThread, NULL));

}
