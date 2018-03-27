/**
 * This module implements the unit tests for eCall API.
 *
 * @warning ! IMPORTANT ! We need to simulate a different session ref for each thread: 1 for the
 *          main thread, and 2 for the 2 threads created for call handler installation.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */


#include "legato.h"
#include "interfaces.h"
#include "le_mcc_local.h"
#include "log.h"
#include "pa_mcc.h"
#include "pa_mcc_simu.h"


//--------------------------------------------------------------------------------------------------
// Begin Stubbed functions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Server Service Reference
 */
//--------------------------------------------------------------------------------------------------
static le_msg_ServiceRef_t _ServerServiceRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Client Session Reference for the current message received from a client
 */
//--------------------------------------------------------------------------------------------------
static le_msg_SessionRef_t _ClientSessionRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference (STUBBED FUNCTION)
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_mcc_GetServiceRef
(
    void
)
{
    return _ServerServiceRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message (STUBBED FUNCTION)
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_mcc_GetClientSessionRef
(
    void
)
{
    return _ClientSessionRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called whenever one of this service's sessions is closed by
 * the client.  (STUBBED FUNCTION)

 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t le_msg_AddServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
)
{
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Acquire a wakeup source (STUBBED FUNCTION)
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pm_StayAwake
(
    le_pm_WakeupSourceRef_t w
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Release a wakeup source (STUBBED FUNCTION)
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pm_Relax
(
    le_pm_WakeupSourceRef_t w
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a new wakeup source (STUBBED FUNCTION)
 *
 * @return Reference to wakeup source
 */
//--------------------------------------------------------------------------------------------------
le_pm_WakeupSourceRef_t le_pm_NewWakeupSource
(
    uint32_t    opts,
    const char *tag
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called whenever one of this service's sessions is closed by
 * the client. (STUBBED FUNCTION)
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t le_msgSimu_AddServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
// End Stubbed functions.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
// Test functions.
//--------------------------------------------------------------------------------------------------

#define NB_CLIENT               2
#define MAIN_TASK_SESSION_REF   (NB_CLIENT+1)
#define DESTINATION_NUMBER      "0102030405"
#define REMOTE_NUMBER           "8182838485"

// Task context
typedef struct
{
    uint16_t                       appId;
    le_msg_SessionRef_t            sessionRef;
    le_thread_Ref_t                appThreadRef;
    le_mcc_CallEventHandlerRef_t   mccHandlerRef;
    le_mcc_CallRef_t               mccRef;
    le_mcc_Event_t                 mccEvent;
    le_mcc_TerminationReason_t     mccTerm;
    int32_t                        mccTermCode;
} AppContext_t;

static le_mcc_CallRef_t             CurrentCallRef;
static le_mcc_Event_t               CurrentCallEvent;
static le_mcc_TerminationReason_t   CurrentTerm;
static int32_t                      CurrentTermCode;
static le_onoff_t                   ClirStatus = LE_OFF;
static char                         RemoteNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];

static AppContext_t                 AppCtx[NB_CLIENT];
static le_sem_Ref_t                 ThreadSemaphore;
static le_sem_Ref_t                 InitSemaphore;
static le_clk_Time_t                TimeToWait = { 2, 0 };

//--------------------------------------------------------------------------------------------------
/**
 * Mutex used to protect access to le_mcc functions when used in different threads.
 * This test is running multiple threads. In addition, it is linking directly to le_mcc.c in the
 * modemDaemon, rather than using the modemDaemon IPC interface. Combining these two things can
 * create race condition on le_mcc functions accesses.
 */
//--------------------------------------------------------------------------------------------------
static pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;   // POSIX "Fast" mutex.
/// Locks the mutex.
#define LOCK    LE_ASSERT(pthread_mutex_lock(&Mutex) == 0);
/// Unlocks the mutex.
#define UNLOCK  LE_ASSERT(pthread_mutex_unlock(&Mutex) == 0);

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Call Event Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyCallEventHandler
(
    le_mcc_CallRef_t  callRef,
    le_mcc_Event_t    callEvent,
    void*             contextPtr
)
{
    AppContext_t * appCtxPtr = (AppContext_t*) contextPtr;

    _ClientSessionRef = appCtxPtr->sessionRef;

    LE_INFO("Handler of app id.%d for callRef.%p, callEvent.%d",
            appCtxPtr->appId,
            callRef,
            callEvent);

    LE_ASSERT(CurrentCallEvent == callEvent);

    appCtxPtr->mccEvent = callEvent;
    appCtxPtr->mccRef = callRef;

    switch(callEvent)
    {
        case LE_MCC_EVENT_ALERTING:
        {
            LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_EVENT_ALERTING.");
            break;
        }
        case LE_MCC_EVENT_CONNECTED:
        {
            LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_EVENT_CONNECTED.");
            break;
        }
        case LE_MCC_EVENT_TERMINATED:
        {
            LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_EVENT_TERMINATED.");
            LOCK
            appCtxPtr->mccTerm = le_mcc_GetTerminationReason(callRef);
            UNLOCK
            LE_ASSERT(CurrentTerm == appCtxPtr->mccTerm);

            switch(appCtxPtr->mccTerm)
            {
                case LE_MCC_TERM_NETWORK_FAIL:
                    LE_INFO("Termination reason is LE_MCC_TERM_NETWORK_FAIL");
                    break;

                case LE_MCC_TERM_UNASSIGNED_NUMBER:
                    LE_INFO("Termination reason is LE_MCC_TERM_UNASSIGNED_NUMBER");
                    break;

                case LE_MCC_TERM_USER_BUSY:
                    LE_INFO("Termination reason is LE_MCC_TERM_USER_BUSY");
                    break;

                case LE_MCC_TERM_LOCAL_ENDED:
                    LE_INFO("Termination reason is LE_MCC_TERM_LOCAL_ENDED");
                    break;

                case LE_MCC_TERM_REMOTE_ENDED:
                    LE_INFO("Termination reason is LE_MCC_TERM_REMOTE_ENDED");
                    break;

                case LE_MCC_TERM_PLATFORM_SPECIFIC:
                    LOCK
                    appCtxPtr->mccTermCode = le_mcc_GetPlatformSpecificTerminationCode(callRef);
                    UNLOCK
                    LE_ASSERT(CurrentTermCode == appCtxPtr->mccTermCode);
                    LE_INFO("Termination reason is LE_MCC_TERM_PLATFORM_SPECIFIC with code.0x%X",
                             appCtxPtr->mccTermCode);
                    break;

                case LE_MCC_TERM_UNDEFINED:
                    LE_INFO("Termination reason is LE_MCC_TERM_UNDEFINED");
                    break;

                default:
                    LE_INFO("Termination reason is %d", appCtxPtr->mccTerm);
                    break;
            }

            LOCK
            le_mcc_Delete(callRef);
            UNLOCK
            break;
        }
        case LE_MCC_EVENT_INCOMING:
        {
            LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_EVENT_INCOMING.");

            char remoteTel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = {'\0'};
            LOCK
            LE_ASSERT(le_mcc_GetRemoteTel(callRef,
                                          remoteTel,
                                          sizeof(remoteTel)) == LE_OK);
            LE_ASSERT(!strncmp(RemoteNumber, remoteTel, strlen(remoteTel)));
            LE_ASSERT(!le_mcc_IsConnected(callRef));
            LE_ASSERT(le_mcc_Answer(callRef) == LE_OK);
            LE_ASSERT(le_mcc_HangUp(callRef) == LE_OK);
            UNLOCK
            break;
        }
        case LE_MCC_EVENT_ORIGINATING:
        {
            LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_EVENT_ORIGINATING.");
            break;
        }
        case LE_MCC_EVENT_SETUP:
        {
            LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_EVENT_SETUP.");
            break;
        }
        case LE_MCC_EVENT_ON_HOLD:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_MCC_EVENT_ON_HOLD.");
            break;
        }
        default:
        {
            LE_ERROR("Check MyCallEventHandler failed, unknowm event %d.", callEvent);
            break;
        }
    }

    _ClientSessionRef = NULL;
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

    LE_INFO("App id: %d", appCtxPtr->appId);

    LOCK
    // simulate a client session for each thread
    intptr_t tempSessionRef = appCtxPtr->appId +1;
    appCtxPtr->sessionRef = (le_msg_SessionRef_t)(tempSessionRef);
    _ClientSessionRef = appCtxPtr->sessionRef;

    // Subscribe to eCall state handler
    LE_ASSERT((appCtxPtr->mccHandlerRef = le_mcc_AddCallEventHandler(MyCallEventHandler,
                                                                     ctxPtr)) != NULL);
    UNLOCK

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(ThreadSemaphore);

    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Synchronize test thread (i.e. main) and tasks
 *
 */
//--------------------------------------------------------------------------------------------------
static void SynchTest
(
    void
)
{
    int i=0;

    for (;i<NB_CLIENT;i++)
    {
        LE_ASSERT(le_sem_WaitWithTimeOut(ThreadSemaphore, TimeToWait) == LE_OK);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Check the result of the event handlers
 *
 */
//--------------------------------------------------------------------------------------------------
static void CheckEventHandlerResult
(
    le_mcc_Event_t event
)
{
    int i;

    // Check that contexts are correctly updated
    for (i=0; i < NB_CLIENT; i++)
    {
        LE_ASSERT(AppCtx[i].appId == i);
        LE_ASSERT(AppCtx[i].mccEvent == event);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Simulate and check the Outgoing Call events
 *
 */
//--------------------------------------------------------------------------------------------------
static void SimulateAndCheckOutgoingCallEvent
(
    le_mcc_Event_t event
)
{
    CurrentCallEvent = event;

    LE_INFO("Simulate event.%d", event);
    pa_mccSimu_ReportCallEvent(DESTINATION_NUMBER, CurrentCallEvent);

    // The tasks have subscribe to event event handler:
    // wait the handlers' calls
    SynchTest();

    // Check event handler result
    CheckEventHandlerResult(event);

    intptr_t tempSessionRef = MAIN_TASK_SESSION_REF;
    _ClientSessionRef = (le_msg_SessionRef_t)(tempSessionRef);

    switch(event)
    {
        case LE_MCC_EVENT_CONNECTED:
            {
                LE_ASSERT(le_mcc_IsConnected(CurrentCallRef));
                LE_ASSERT(le_mcc_HangUp(CurrentCallRef) == LE_OK);
                break;
            }
        case LE_MCC_EVENT_TERMINATED:
            {
                LE_ASSERT(!le_mcc_IsConnected(CurrentCallRef));
                LE_ASSERT(le_mcc_HangUp(CurrentCallRef) == LE_FAULT);
                break;
            }
        default:
            {
                LE_ASSERT(!le_mcc_IsConnected(CurrentCallRef));
                LE_ASSERT(le_mcc_HangUp(CurrentCallRef) == LE_OK);
                break;
            }
    }
    LE_ASSERT(le_mcc_HangUpAll() == LE_OK);
    _ClientSessionRef = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Simulate and check an Incoming Call event
 *
 */
//--------------------------------------------------------------------------------------------------
static void SimulateAndCheckIncomingCallEvent
(
    void
)
{
    CurrentCallEvent = LE_MCC_EVENT_INCOMING;

    LE_INFO("Simulate event.%d", CurrentCallEvent);

    snprintf(RemoteNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES, REMOTE_NUMBER);
    pa_mccSimu_ReportCallEvent(RemoteNumber, LE_MCC_EVENT_INCOMING);

    // The tasks have subscribe to event event handler:
    // wait the handlers' calls
    SynchTest();

    // Check event handler result
    CheckEventHandlerResult(LE_MCC_EVENT_INCOMING);

    intptr_t tempSessionRef = MAIN_TASK_SESSION_REF;
    _ClientSessionRef = (le_msg_SessionRef_t)(tempSessionRef);
    LE_ASSERT(le_mcc_HangUpAll() == LE_OK);

    CurrentCallEvent = LE_MCC_EVENT_TERMINATED;
    pa_mccSimu_ReportCallEvent(REMOTE_NUMBER, CurrentCallEvent);

    SynchTest();
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove event handlers
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

    LOCK
    _ClientSessionRef = appCtxPtr->sessionRef;
    le_mcc_RemoveCallEventHandler(appCtxPtr->mccHandlerRef);
    UNLOCK

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Check the result of the event handlers
 *
 */
//--------------------------------------------------------------------------------------------------
static void CheckTerminationHandlerResult
(
    void
)
{
    int i;

    // Check that contexts are correctly updated
    for (i=0; i < NB_CLIENT; i++)
    {
        LE_ASSERT(AppCtx[i].appId == i);
        LE_ASSERT(AppCtx[i].mccEvent == CurrentCallEvent);
        LE_ASSERT(AppCtx[i].mccTerm == CurrentTerm);
        if (AppCtx[i].mccTerm == LE_MCC_TERM_PLATFORM_SPECIFIC)
        {
            LE_ASSERT(AppCtx[i].mccTermCode == CurrentTermCode);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Simulate and check the Call event
 *
 */
//--------------------------------------------------------------------------------------------------
static void SimulateAndCheckTermination
(
    le_mcc_TerminationReason_t term,
    int32_t                    termCode
)
{
    static int i;

    CurrentCallEvent = LE_MCC_EVENT_INCOMING;
    snprintf(RemoteNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES, "808283848%d", i++);
    pa_mccSimu_ReportCallEvent(RemoteNumber, LE_MCC_EVENT_INCOMING);

    // The tasks have subscribe to event event handler:
    // wait the handlers' calls
    SynchTest();

    CurrentCallEvent = LE_MCC_EVENT_TERMINATED;
    CurrentTerm = term;
    CurrentTermCode = termCode;

    LE_INFO("Simulate event.%d with term.%d termCode.0x%x", LE_MCC_EVENT_TERMINATED, term, termCode);
    pa_mccSimu_ReportCallTerminationReason(RemoteNumber, CurrentTerm, CurrentTermCode);

    // The tasks have subscribe to event event handler:
    // wait the handlers' calls
    SynchTest();

    // Check event handler result
    CheckTerminationHandlerResult();
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Create and modify CLIR status.
 *
 * API tested:
 * - le_mcc_Create
 * - le_mcc_SetCallerIdRestrict
 * - le_mcc_GetCallerIdRestrict
 * - le_mcc_Delete
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mcc_SetClir
(
    void
)
{
    le_onoff_t localClirStatus;

    intptr_t tempSessionRef = MAIN_TASK_SESSION_REF;
    _ClientSessionRef = (le_msg_SessionRef_t)(tempSessionRef);

    LE_ASSERT((CurrentCallRef = le_mcc_Create(DESTINATION_NUMBER)) != NULL);
    LE_ASSERT(le_mcc_GetCallerIdRestrict(CurrentCallRef, &localClirStatus) == LE_UNAVAILABLE);
    LE_ASSERT(le_mcc_SetCallerIdRestrict(CurrentCallRef, ClirStatus) == LE_OK);
    LE_ASSERT(le_mcc_GetCallerIdRestrict(CurrentCallRef, &localClirStatus) == LE_OK);
    LE_ASSERT(localClirStatus ==  ClirStatus);
    le_mcc_Delete(CurrentCallRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Test call waiting supplementary service.
 *
 * API tested:
 * - le_mcc_SetCallWaitingServiceStatus
 * - le_mcc_GetCallWaitingServiceStatus
 * - le_mcc_ActiveCall
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
void Testle_mcc_callWaiting
(
    void
)
{
    bool callWaitingStatus = false;
    LE_ASSERT(le_mcc_SetCallWaitingService(true) == LE_OK);
    LE_ASSERT(le_mcc_GetCallWaitingService(&callWaitingStatus) == LE_OK);
    LE_ASSERT(callWaitingStatus == LE_ON);
    LE_ASSERT(le_mcc_SetCallWaitingService(false) == LE_OK);
    LE_ASSERT(le_mcc_GetCallWaitingService(&callWaitingStatus) == LE_OK);
    LE_ASSERT(callWaitingStatus == LE_OFF);

    intptr_t tempSessionRef = MAIN_TASK_SESSION_REF;
    _ClientSessionRef = (le_msg_SessionRef_t)(tempSessionRef);

    LE_ASSERT((CurrentCallRef = le_mcc_Create(DESTINATION_NUMBER)) != NULL);
    LE_ASSERT(le_mcc_ActivateCall(CurrentCallRef) == LE_FAULT);
    pa_mccSimu_SetVoiceDialResult(LE_OK);
    LE_ASSERT(le_mcc_Start(CurrentCallRef) == LE_OK);
    CurrentCallEvent = LE_MCC_EVENT_WAITING;
    pa_mccSimu_ReportCallEvent(DESTINATION_NUMBER, LE_MCC_EVENT_WAITING);
    SynchTest();

    _ClientSessionRef = (le_msg_SessionRef_t)(tempSessionRef);
    LE_ASSERT(le_mcc_ActivateCall(CurrentCallRef) == LE_OK);
    CurrentCallEvent = LE_MCC_EVENT_TERMINATED;
    CurrentTerm = 0;
    CurrentTermCode = 0;
    pa_mccSimu_ReportCallEvent(DESTINATION_NUMBER, LE_MCC_EVENT_TERMINATED);

    SynchTest();
    _ClientSessionRef = (le_msg_SessionRef_t)(tempSessionRef);
    le_mcc_Delete(CurrentCallRef);
    _ClientSessionRef = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the test environmeent:
 * - create some tasks (simulate multi app)
 * - create semaphore (to make checkpoints and synchronize test and tasks)
 * - simulate call events
 * - check that event handlers are correctly called
 *
 * API tested:
 * - le_mcc_Create
 * - le_mcc_AddCallEventHandler
 * - le_mcc_Answer (through Call handler functions)
 * - le_mcc_GetRemoteTel (through Call handler functions)
 * - le_mcc_IsConnected
 * - le_mcc_HangUp
 * - le_mcc_HangUpAll
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
void Testle_mcc_AddHandlers
(
    void
)
{
    int i;

    // Create a semaphore to coordinate the test
    ThreadSemaphore = le_sem_Create("HandlerSem",0);

    // int app context
    memset(AppCtx, 0, NB_CLIENT*sizeof(AppContext_t));

    // Start tasks: simulate multi-user of le_mcc
    // each thread subcribes to state handler using le_mcc_AddStateChangeHandler
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

    // ! IMPORTANT !
    // We need to simulate a different session ref for each thread: 1 for the main thread, and 2
    // for the 2 threads created above.
    intptr_t tempSessionRef = MAIN_TASK_SESSION_REF;
    _ClientSessionRef = (le_msg_SessionRef_t)(tempSessionRef);

    LE_ASSERT((CurrentCallRef = le_mcc_Create(DESTINATION_NUMBER)) != NULL);

    // Simulate a failed voice call
    pa_mccSimu_SetVoiceDialResult(LE_FAULT);
    LE_ASSERT(le_mcc_Start(CurrentCallRef) == LE_FAULT);

    // voice call is now possible
    pa_mccSimu_SetVoiceDialResult(LE_OK);
    LE_ASSERT(le_mcc_Start(CurrentCallRef) == LE_OK);
    LE_ASSERT(le_mcc_Start(CurrentCallRef) == LE_BUSY);
    LE_ASSERT(le_mcc_Delete(CurrentCallRef) == LE_FAULT);

    // Simulate outgoing call
    SimulateAndCheckOutgoingCallEvent(LE_MCC_EVENT_SETUP);
    SimulateAndCheckOutgoingCallEvent(LE_MCC_EVENT_ORIGINATING);
    SimulateAndCheckOutgoingCallEvent(LE_MCC_EVENT_ALERTING);
    SimulateAndCheckOutgoingCallEvent(LE_MCC_EVENT_ON_HOLD);
    SimulateAndCheckOutgoingCallEvent(LE_MCC_EVENT_WAITING);
    SimulateAndCheckOutgoingCallEvent(LE_MCC_EVENT_CONNECTED);
    SimulateAndCheckOutgoingCallEvent(LE_MCC_EVENT_TERMINATED);
    _ClientSessionRef = (le_msg_SessionRef_t)(tempSessionRef);
    le_mcc_Delete(CurrentCallRef);

    // Simulate incoming call
    SimulateAndCheckIncomingCallEvent();

    _ClientSessionRef = NULL;
    // Check that no more call of the semaphore
    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the test environmeent:
 * - simulate LE_MCC_EVENT_TERMINATED event with all possible termination reasons
 * - check that event handlers are correctly called with the correct termination reasons
 *
 * API tested:
 * - le_mcc_GetTerminationReason (through Call handler functions)
 * - le_mcc_GetPlatformSpecificTerminationCode (through Call handler functions)
 * - le_mcc_Answer (through Call handler functions)
 * - le_mcc_GetRemoteTel (through Call handler functions)
 * - le_mcc_IsConnected
 * - le_mcc_Delete
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
void Testle_mcc_GetTerminationReason
(
    void
)
{
    SimulateAndCheckTermination(LE_MCC_TERM_LOCAL_ENDED, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_REMOTE_ENDED, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_NETWORK_FAIL, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_UNASSIGNED_NUMBER, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_NO_ROUTE_TO_DESTINATION, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_CHANNEL_UNACCEPTABLE, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_OPERATOR_DETERMINED_BARRING, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_USER_BUSY, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_NO_USER_RESPONDING, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_USER_ALERTING_NO_ANSWER, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_CALL_REJECTED, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_NUMBER_CHANGED, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_PREEMPTION, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_DESTINATION_OUT_OF_ORDER, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_INVALID_NUMBER_FORMAT, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_FACILITY_REJECTED, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_RESP_TO_STATUS_ENQUIRY, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_NORMAL_UNSPECIFIED, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_NO_CIRCUIT_OR_CHANNEL_AVAILABLE, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_NETWORK_OUT_OF_ORDER, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_TEMPORARY_FAILURE, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_SWITCHING_EQUIPMENT_CONGESTION, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_ACCESS_INFORMATION_DISCARDED, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_REQUESTED_CIRCUIT_OR_CHANNEL_NOT_AVAILABLE, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_RESOURCES_UNAVAILABLE_OR_UNSPECIFIED, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_QOS_UNAVAILABLE, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_REQUESTED_FACILITY_NOT_SUBSCRIBED, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_INCOMING_CALLS_BARRED_WITHIN_CUG, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_BEARER_CAPABILITY_NOT_AUTH, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_BEARER_CAPABILITY_UNAVAILABLE, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_SERVICE_OPTION_NOT_AVAILABLE, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_ACM_LIMIT_EXCEEDED, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_BEARER_SERVICE_NOT_IMPLEMENTED, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_REQUESTED_FACILITY_NOT_IMPLEMENTED, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_ONLY_DIGITAL_INFORMATION_BEARER_AVAILABLE, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_SERVICE_OR_OPTION_NOT_IMPLEMENTED, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_INVALID_TRANSACTION_IDENTIFIER, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_USER_NOT_MEMBER_OF_CUG, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_INCOMPATIBLE_DESTINATION, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_INVALID_TRANSIT_NW_SELECTION, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_SEMANTICALLY_INCORRECT_MESSAGE, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_INVALID_MANDATORY_INFORMATION, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_MESSAGE_TYPE_NON_IMPLEMENTED, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_INFORMATION_ELEMENT_NON_EXISTENT, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_CONDITONAL_IE_ERROR, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_RECOVERY_ON_TIMER_EXPIRY, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_PROTOCOL_ERROR_UNSPECIFIED, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_INTERWORKING_UNSPECIFIED, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_NO_SERVICE, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_NOT_ALLOWED, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_FDN_ACTIVE, 0);
    SimulateAndCheckTermination(LE_MCC_TERM_PLATFORM_SPECIFIC, 0x5A);
    SimulateAndCheckTermination(LE_MCC_TERM_UNDEFINED, 0);

    // Check that no more call of the semaphore
    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test remove handlers
 *
 * API tested:
 * - le_mcc_RemoveCallEventHandler
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
void Testle_mcc_RemoveHandlers
(
    void
)
{
    int i;

    // Remove handlers: add le_mcc_RemoveStateChangeHandler to the eventLoop of tasks
    for (i=0; i<NB_CLIENT; i++)
    {
        le_event_QueueFunctionToThread(AppCtx[i].appThreadRef,
                                       RemoveHandler,
                                       &AppCtx[i],
                                       NULL);
    }

    // Wait for the tasks
    SynchTest();

    // Simulate an outgoing call
    intptr_t tempSessionRef = MAIN_TASK_SESSION_REF;
    _ClientSessionRef = (le_msg_SessionRef_t)(tempSessionRef);

    LE_ASSERT((CurrentCallRef = le_mcc_Create(DESTINATION_NUMBER)) != NULL);
    LE_ASSERT(le_mcc_Start(CurrentCallRef) == LE_OK);

    // Provoke event which should call the handlers
    pa_mccSimu_ReportCallEvent(DESTINATION_NUMBER, LE_MCC_EVENT_TERMINATED);

    // Wait for the semaphore timeout to check that handlers are not called
    LE_ASSERT( le_sem_WaitWithTimeOut(ThreadSemaphore, TimeToWait) == LE_TIMEOUT );

    _ClientSessionRef = (le_msg_SessionRef_t)(tempSessionRef);
    le_mcc_Delete(CurrentCallRef);

    for (i=0; i<NB_CLIENT; i++)
    {
        le_thread_Cancel(AppCtx[i].appThreadRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * UnitTestInit thread: this function initializes the test and run an eventLoop
 *
 */
//--------------------------------------------------------------------------------------------------
static void* UnitTestInit
(
    void* ctxPtr
)
{
    // pa simu init
    mcc_simu_Init();

    // init the services
    le_mcc_Init();

    le_sem_Post(InitSemaphore);

    le_event_RunLoop();
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: enable/disable the audio AMR Wideband capability.
 *
 * API tested:
 * - le_mcc_SetAmrWbCapability
 * - le_mcc_GetAmrWbCapability
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mcc_SetGetAmrWbCapability
(
    void
)
{
    bool  AmrWbCapState;

    LE_ASSERT_OK(le_mcc_SetAmrWbCapability(false));
    LE_ASSERT_OK(le_mcc_GetAmrWbCapability(&AmrWbCapState));

    LE_ASSERT(false == AmrWbCapState);

    LE_ASSERT_OK(le_mcc_SetAmrWbCapability(true));
    LE_ASSERT_OK(le_mcc_GetAmrWbCapability(&AmrWbCapState));

    LE_ASSERT(true == AmrWbCapState);
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
    //le_log_SetFilterLevel(LE_LOG_DEBUG);

    // Create a semaphore to coordinate Initialization
    InitSemaphore = le_sem_Create("InitSem",0);
    le_thread_Start(le_thread_Create("UnitTestInit", UnitTestInit, NULL));
    le_sem_Wait(InitSemaphore);

    LE_INFO("======== Start UnitTest of MCC API ========");

    LE_INFO("======== SetGetAmrWbCapability Test  ========");
    Testle_mcc_SetGetAmrWbCapability();
    LE_INFO("======== SetClir Test  ========");
    Testle_mcc_SetClir();
    LE_INFO("======== AddHandlers Test  ========");
    Testle_mcc_AddHandlers();
    LE_INFO("======== GetTerminationReason Test  ========");
    Testle_mcc_GetTerminationReason();
    LE_INFO("======== call waiting Test  ========");
    Testle_mcc_callWaiting();
    LE_INFO("======== RemoveHandlers Test  ========");
    Testle_mcc_RemoveHandlers();

    LE_INFO("======== UnitTest of MCC API ends with SUCCESS ========");

    exit(0);
}


