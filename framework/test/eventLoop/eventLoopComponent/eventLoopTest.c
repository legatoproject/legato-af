
#include "legato.h"

// TODO: Open a pipe and test event-driven writing and reading using two threads and a couple
//       of fd event monitors.

static bool TestAPassed = false;
static bool TestBPassed = false;
static bool TestCPassed = false;

static le_event_Id_t EventIdA;
static le_event_Id_t EventIdB;
static le_event_Id_t EventIdC;

static char EventContextA[] = "Context A";

typedef struct
{
    char str[10];
    bool* passedFlagPtr;
}
Report_t;

static Report_t ReportA = { "Report A", &TestAPassed };
static Report_t ReportB = { "Report B", &TestBPassed };
static Report_t ReportC = { "Report C", &TestCPassed };


static void EventHandlerA
(
    void* reportPtr // Non-ref-counted (copied report).
)
{
    Report_t* objPtr = reportPtr;

    LE_INFO("Report = \"%p\"; Context = \"%p\".", reportPtr, le_event_GetContextPtr());
    LE_INFO("ReportA.str = '%s'.", ReportA.str);
    LE_INFO("objPtr->str = '%s'.", objPtr->str);

    LE_TEST_OK(strcmp(ReportA.str, objPtr->str) == 0, "Event A successfully reported.");
    LE_TEST_OK(objPtr != &ReportA, "Report A successfully passed to event handler.");
    LE_TEST_OK(&EventContextA == le_event_GetContextPtr(), "Event A context ptr successfully set.");

    *(objPtr->passedFlagPtr) = true;
}


static void EventHandlerB
(
    void* reportPtr // Ref-counted.
)
{
    Report_t* objPtr = reportPtr;

    LE_INFO("Report = \"%p\"; Context = \"%p\".", reportPtr, le_event_GetContextPtr());

    LE_INFO("ReportB.str = '%s'.", ReportB.str);
    LE_INFO("objPtr->str = '%s'.", objPtr->str);

    LE_TEST_OK(strcmp(ReportB.str, objPtr->str) == 0, "Event B successfully reported.");
    LE_TEST_OK(le_event_GetContextPtr() == NULL, "Event B context ptr not set.");

    le_mem_Release(reportPtr);
}


static void Destructor
(
    void* objPtr
)
{
    Report_t* reportPtr = objPtr;

    LE_INFO("Destructor running.");

    LE_ASSERT(   (strcmp(ReportB.str, reportPtr->str) == 0)
              || (strcmp(ReportC.str, reportPtr->str) == 0) );

    LE_INFO("Destructing reference counted %s.", reportPtr->str);

    LE_TEST_ASSERT(reportPtr->passedFlagPtr != NULL, "Reference counted report is now destructed.");

    *(reportPtr->passedFlagPtr) = true;
}


static void CheckTestResults
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_TEST_OK(param1Ptr == &ReportA, "Report A successfully passed to queued function.");
    LE_TEST_OK(param2Ptr == &ReportB, "Report B successfully passed to queued function.");

    LE_TEST_OK(TestAPassed, "Test Event A passed");
    LE_TEST_OK(TestAPassed, "Test Event B passed");
    LE_TEST_OK(TestAPassed, "Test Event C passed");

    LE_INFO("======== EVENT LOOP TEST COMPLETE (PASSED) ========");
    LE_TEST_EXIT;
}


COMPONENT_INIT
{
    Report_t* reportPtr;

    LE_INFO("======== BEGIN EVENT LOOP TEST ========");

    LE_INFO("%s called!", __func__);

    LE_TEST_PLAN(22);

    EventIdA = le_event_CreateId("Event A", sizeof(ReportA));
    LE_TEST_OK(true, "Created event ID A.");

    EventIdB = le_event_CreateIdWithRefCounting("Event B");
    LE_TEST_OK(true, "Created event ID B.");

    EventIdC = le_event_CreateIdWithRefCounting("Event C");
    LE_TEST_OK(true, "Created event ID C.");

    le_event_HandlerRef_t evtHdlerRefA = le_event_AddHandler("Handler A", EventIdA, EventHandlerA);
    LE_TEST_OK(true, "Added event handler A.");

    le_event_SetContextPtr(evtHdlerRefA, &EventContextA);
    LE_TEST_OK(true, "Set context pointer for event handler A.");

    le_event_AddHandler("Handler B", EventIdB, EventHandlerB);
    LE_TEST_OK(true, "Added event handler B.");
    // Intentionally no handler for ref-counting Event C.

    le_event_Report(EventIdA, &ReportA, sizeof(ReportA));
    LE_TEST_OK(true, "Reporting event A...");

    le_mem_PoolRef_t memPool = le_mem_CreatePool("Report", sizeof(Report_t));
    le_mem_SetDestructor(memPool, Destructor);
    le_mem_ExpandPool(memPool, 2);

    reportPtr = le_mem_ForceAlloc(memPool);
    memcpy(reportPtr, &ReportB, sizeof(*reportPtr));
    le_event_ReportWithRefCounting(EventIdB, reportPtr);
    LE_TEST_OK(true, "Reporting event B with ref counting...");

    reportPtr = le_mem_ForceAlloc(memPool);
    memcpy(reportPtr, &ReportC, sizeof(*reportPtr));
    le_event_ReportWithRefCounting(EventIdC, reportPtr);
    LE_TEST_OK(true, "Reporting event C with ref counting...");

    le_event_QueueFunction(CheckTestResults, &ReportA, &ReportB);
    LE_TEST_OK(true, "Queuing function to check test results for events A and B...");
}
