/*
 * The "real" implementation of the functions on the server side
 */


#include "legato.h"
#include "async_server.h"
#include "le_print.h"


void allParameters
(
    ServerCmdRef_t cmdRef,
    common_EnumExample_t a,
    const uint32_t* dataPtr,
    size_t dataNumElements,
    size_t outputNumElements,
    const char* label,
    size_t responseNumElements,
    size_t moreNumElements
)
{
    int i;

    // Define storage for output parameters
    uint32_t b;
    uint32_t output[outputNumElements];
    char response[responseNumElements];
    char more[moreNumElements];

    // Print out received values
    LE_PRINT_VALUE("%i", a);
    LE_PRINT_VALUE("%s", label);
    LE_PRINT_ARRAY("%i", dataNumElements, dataPtr);

    // Generate return values
    b = a;

    for (i=0; i<outputNumElements; i++)
    {
        output[i] = i*a;
    }

    le_utf8_Copy(response, "response string", responseNumElements, NULL);
    le_utf8_Copy(more, "more info", moreNumElements, NULL);

    allParametersRespond(cmdRef, b, outputNumElements, output, response, more);
}


// Storage for the handler ref
static TestAFunc_t HandlerRef = NULL;
static void* ContextPtr = NULL;

TestARef_t AddTestA
(
    TestAFunc_t handlerRef,
    void* contextPtr
)
{
    HandlerRef = handlerRef;
    ContextPtr = contextPtr;

    // Note: this is just for testing, and is easier than actually creating an event and using
    //       the event loop to call the handler.
    return (TestARef_t)10;
}

void RemoveTestA
(
    TestARef_t addHandlerRef
)
{
    LE_PRINT_VALUE("%p", addHandlerRef);

    if ( addHandlerRef == (TestARef_t)10 )
    {
        HandlerRef = NULL;
        ContextPtr = NULL;
    }
    else
    {
        LE_ERROR("Invalid addHandlerRef='%p'\n", addHandlerRef);
    }
}

void TriggerTestA
(
    ServerCmdRef_t cmdRef
)
{
    if ( HandlerRef != NULL )
    {
        HandlerRef(5, ContextPtr);
    }
    else
    {
        LE_ERROR("Handler not registered\n");
    }

    TriggerTestARespond(cmdRef);

    // This will cause the server to fail, since only one response is allowed.
    LE_WARN("About to crash the server by calling 'Respond' function twice");
    TriggerTestARespond(cmdRef);
}


// Add these two functions to satisfy the compiler, but don't need to do
// anything with them, since they are just used to verify bug fixes in
// the handler specification.
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


LE_EVENT_INIT_HANDLER
{
    StartServer("Test 2");
}
