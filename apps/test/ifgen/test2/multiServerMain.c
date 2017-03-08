/*
 * The "real" implementation of the functions on the server side
 */


#include "legato.h"
#include "example_server.h"
#include "le_print.h"


// Event used for registering and triggering handlers
le_event_Id_t TriggerEvent;


//--------------------------------------------------------------------------------------------------
/**
 * Test direct function call
 */
//--------------------------------------------------------------------------------------------------
void example_allParameters
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
 * Empty stub since this is already tested by other code
 */
//--------------------------------------------------------------------------------------------------
void example_FileTest
(
    int dataFile,
    int* dataOutPtr
)
{
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
    example_TestAHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(*dataPtr, le_event_GetContextPtr());
}


example_TestAHandlerRef_t example_AddTestAHandler
(
    example_TestAHandlerFunc_t handler,
    void* contextPtr
)
{
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                    "Server",
                                                    TriggerEvent,
                                                    FirstLayerTestAHandler,
                                                    (le_event_HandlerFunc_t)handler);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (example_TestAHandlerRef_t)(handlerRef);
}


void example_RemoveTestAHandler
(
    example_TestAHandlerRef_t addHandlerRef
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)addHandlerRef);
}


void example_TriggerTestA
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

example_BugTestHandlerRef_t example_AddBugTestHandler
(
    const char* newPathPtr,
    example_BugTestHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    return NULL;
}

void example_RemoveBugTestHandler
(
    example_BugTestHandlerRef_t addHandlerRef
)
{
}


/*
 * Add these two functions to satisfy the compiler, but leave empty for now.  The callback
 * parameters tests are done elsewhere.
 */
int32_t example_TestCallback
(
    uint32_t someParm,
    const uint8_t* dataArrayPtr,
    size_t dataArrayNumElements,
    example_CallbackTestHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    return 0;
}

void example_TriggerCallbackTest
(
    uint32_t data
)
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialization
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    TriggerEvent = le_event_CreateId("Server Trigger", sizeof(int32_t));

    example_AdvertiseService();
}

