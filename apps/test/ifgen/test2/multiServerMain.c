/*
 * The "real" implementation of the functions on the server side
 */


#include "legato.h"
#include "server.h"
#include "le_print.h"


// Event used for registering and triggering handlers
le_event_Id_t TriggerEvent;


//--------------------------------------------------------------------------------------------------
/**
 * Test direct function call
 */
//--------------------------------------------------------------------------------------------------
void allParameters
(
    common_EnumExample_t a,
    uint32_t* bPtr,
    const uint32_t* dataPtr,
    size_t dataNumElements,
    uint32_t* outputPtr,
    size_t* outputNumElementsPtr,
    const char* label,
    char* response,
    size_t responseNumElements,
    char* more,
    size_t moreNumElements
)
{
    int i;

    // Print out received values
    LE_PRINT_VALUE("%i", a);
    LE_PRINT_VALUE("%s", label);
    LE_PRINT_ARRAY("%i", dataNumElements, dataPtr);

    // Generate return values
    *bPtr = a;

    for (i=0; i<*outputNumElementsPtr; i++)
    {
        outputPtr[i] = i*a;
    }

    le_utf8_Copy(response, "response string", responseNumElements, NULL);
    le_utf8_Copy(more, "more info", moreNumElements, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test handler related functions
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerTestAHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    int32_t*                  dataPtr = reportPtr;
    TestAFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(*dataPtr, le_event_GetContextPtr());
}


TestARef_t AddTestA
(
    TestAFunc_t handler,
    void* contextPtr
)
{
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                    "Server",
                                                    TriggerEvent,
                                                    FirstLayerTestAHandler,
                                                    (le_event_HandlerFunc_t)handler);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (TestARef_t)(handlerRef);
}


void RemoveTestA
(
    TestARef_t addHandlerRef
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)addHandlerRef);
}


void TriggerTestA
(
    void
)
{
    // todo: This could instead be the value passed into the trigger function, but need
    //       to change the .api definition for that to work.
    static int32_t triggerCount=0;
    triggerCount++;

    LE_PRINT_VALUE("%d", triggerCount);
    le_event_Report(TriggerEvent, &triggerCount, sizeof(triggerCount));
}


//--------------------------------------------------------------------------------------------------
/**
 * Add these two functions to satisfy the compiler, but don't need to do
 * anything with them, since they are just used to verify bug fixes in
 * the handler specification.
 */
//--------------------------------------------------------------------------------------------------

BugTestRef_t AddBugTest
(
    const char* newPathPtr,
    BugTestFunc_t handlerPtr,
    void* contextPtr
)
{
    return NULL;
}

void RemoveBugTest
(
    BugTestRef_t addHandlerRef
)
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialization
 */
//--------------------------------------------------------------------------------------------------
LE_EVENT_INIT_HANDLER
{
    TriggerEvent = le_event_CreateId("Server Trigger", sizeof(int32_t));
    
    StartServer("Test 2");
}

