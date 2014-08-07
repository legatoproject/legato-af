/*
 * The "real" implementation of the functions on the server side
 */


#include "legato.h"
#include "server.h"
#include "le_print.h"


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

void FileTest
(
    int dataFile,
    int* dataOutPtr
)
{
    // Read and print out whatever is read from the client fd
    char buffer[1000];
    ssize_t numRead;

    numRead = read(dataFile, buffer, sizeof(buffer));
    buffer[numRead] = '\0';
    LE_PRINT_VALUE("%zd", numRead);
    LE_PRINT_VALUE("%s", buffer);

    // Open a known file to return back to the client
    *dataOutPtr = open("/usr/include/stdio.h", O_RDONLY);
    LE_PRINT_VALUE("%i", *dataOutPtr);

    // Read a bit from the file, to make sure it is okay
    numRead = read(*dataOutPtr, buffer, sizeof(buffer));
    buffer[numRead] = '\0';
    LE_PRINT_VALUE("%zd", numRead);
    LE_PRINT_VALUE("%s", buffer);

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
    void
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


COMPONENT_INIT
{
    StartServer("Test 2");
}
