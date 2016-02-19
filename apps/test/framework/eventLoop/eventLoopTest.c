
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

    LE_ASSERT(strcmp(ReportA.str, objPtr->str) == 0);
    LE_ASSERT(objPtr != &ReportA);
    LE_ASSERT(&EventContextA == le_event_GetContextPtr());

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

    LE_ASSERT(strcmp(ReportB.str, objPtr->str) == 0);
    LE_ASSERT(le_event_GetContextPtr() == NULL);

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

    LE_ASSERT(reportPtr->passedFlagPtr != NULL);

    *(reportPtr->passedFlagPtr) = true;
}


static void CheckTestResults
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_ASSERT(param1Ptr == &ReportA);
    LE_ASSERT(param2Ptr == &ReportB);

    LE_ASSERT(TestAPassed);
    LE_ASSERT(TestBPassed);
    LE_ASSERT(TestCPassed);

    LE_INFO("======== EVENT LOOP TEST COMPLETE (PASSED) ========");
    exit(EXIT_SUCCESS);
}


COMPONENT_INIT
{
    Report_t* reportPtr;

    LE_INFO("======== BEGIN EVENT LOOP TEST ========");

    LE_INFO("%s called!", __func__);

    EventIdA = le_event_CreateId("Event A", sizeof(ReportA));
    EventIdB = le_event_CreateIdWithRefCounting("Event B");
    EventIdC = le_event_CreateIdWithRefCounting("Event C");

    le_event_SetContextPtr(le_event_AddHandler("Handler A", EventIdA, EventHandlerA), &EventContextA);
    le_event_AddHandler("Handler B", EventIdB, EventHandlerB);
    // Intentionally no handler for ref-counting Event C.

    le_event_Report(EventIdA, &ReportA, sizeof(ReportA));

    le_mem_PoolRef_t memPool = le_mem_CreatePool("Report", sizeof(Report_t));
    le_mem_SetDestructor(memPool, Destructor);
    le_mem_ExpandPool(memPool, 2);

    reportPtr = le_mem_ForceAlloc(memPool);
    memcpy(reportPtr, &ReportB, sizeof(*reportPtr));
    le_event_ReportWithRefCounting(EventIdB, reportPtr);

    reportPtr = le_mem_ForceAlloc(memPool);
    memcpy(reportPtr, &ReportC, sizeof(*reportPtr));
    le_event_ReportWithRefCounting(EventIdC, reportPtr);

    le_event_QueueFunction(CheckTestResults, &ReportA, &ReportB);
}
