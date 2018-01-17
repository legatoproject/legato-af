/*
 * The "real" implementation of the functions on the server side
 */


#include "legato.h"
#include "example_server.h"
#include "le_print.h"

#define BUFFERSIZE 1000

// Need this so we can queue functions to the new thread.
// This will only be used from the main thread.
static le_thread_Ref_t NewThreadRef;


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
    if (bPtr == NULL)
    {
        LE_KILL_CLIENT("bPtr is NULL.");
        return;
    }

    if (outputPtr == NULL)
    {
        LE_KILL_CLIENT("outputPtr is NULL.");
        return;
    }

    int i;

    // If a special value is passed down, return right away without assigning to any of the output
    // parameters.  This could happen in a typical function, if an error is detected.
    if ( a == COMMON_ZERO )
    {
        LE_PRINT_VALUE("%zd", *outputNumElementsPtr);
        LE_ASSERT( *outputNumElementsPtr <= 10 );
        LE_DEBUG("Returning right away");
        return;
    }

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

void example_FileTest
(
    int dataFile,
    int* dataOutPtr
)
{
    if (dataOutPtr == NULL)
    {
        LE_KILL_CLIENT("dataOutPtr is NULL.");
        return;
    }

    // Read and print out whatever is read from the client fd
    char buffer[BUFFERSIZE];
    ssize_t numRead;

    numRead = read(dataFile, buffer, sizeof(buffer));
    if (-1 == numRead)
    {
        LE_INFO("Read error %s", strerror(errno));
    }
    else
    {
        if (BUFFERSIZE == numRead)
        {
            buffer[numRead-1] = '\0';
        }
        else
        {
            buffer[numRead] = '\0';
        }
        LE_PRINT_VALUE("%zd", numRead);
        LE_PRINT_VALUE("%s", buffer);
    }

    // Open a known file to return back to the client
    *dataOutPtr = open("/usr/include/stdio.h", O_RDONLY);
    LE_PRINT_VALUE("%i", *dataOutPtr);

    // Read a bit from the file, to make sure it is okay
    numRead = read(*dataOutPtr, buffer, sizeof(buffer));
    if (-1 == numRead)
    {
        LE_INFO("Read error %s", strerror(errno));
    }
    else
    {
        if (BUFFERSIZE == numRead)
        {
            buffer[numRead-1] = '\0';
        }
        else
        {
            buffer[numRead] = '\0';
        }
        LE_PRINT_VALUE("%zd", numRead);
        LE_PRINT_VALUE("%s", buffer);
    }
}


// Storage for the handler ref
static example_TestAHandlerFunc_t HandlerRef = NULL;
static void* ContextPtr = NULL;

example_TestAHandlerRef_t example_AddTestAHandler
(
    example_TestAHandlerFunc_t handlerRef,
    void* contextPtr
)
{
    HandlerRef = handlerRef;
    ContextPtr = contextPtr;

    // Note: this is just for testing, and is easier than actually creating an event and using
    //       the event loop to call the handler.
    return (example_TestAHandlerRef_t)10;
}

void example_RemoveTestAHandler
(
    example_TestAHandlerRef_t addHandlerRef
)
{
    LE_PRINT_VALUE("%p", addHandlerRef);

    if ( addHandlerRef == (example_TestAHandlerRef_t)10 )
    {
        HandlerRef = NULL;
        ContextPtr = NULL;
    }
    else
    {
        LE_ERROR("Invalid addHandlerRef='%p'\n", addHandlerRef);
    }
}

void example_TriggerTestA
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
 * Callback function testing
 */

// Storage for the handler ref
static example_CallbackTestHandlerFunc_t CallbackTestHandlerRef = NULL;
static void* CallbackTestContextPtr = NULL;

int32_t example_TestCallback
(
    uint32_t someParm,
    const uint8_t* dataArrayPtr,
    size_t dataArrayNumElements,
    example_CallbackTestHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_PRINT_VALUE("%d", someParm);

    CallbackTestHandlerRef = handlerPtr;
    CallbackTestContextPtr = contextPtr;

    return someParm+53;
}


static void CallbackTestHandlerQueued
(
    void* dataPtr,
    void* contextPtr
)
{
    // Test file descriptors passed back to client handler
    int fdToClient;

    // Open a file known to exist
    fdToClient = open("/etc/group", O_RDONLY);

    LE_PRINT_VALUE("%i", fdToClient);

    uint8_t array[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

    // Note that data, which is uint32_t, is just cast to void*, so cast it back.
    CallbackTestHandlerRef(
      *((uint32_t*)dataPtr),
      "some name from server",
      array,
      NUM_ARRAY_MEMBERS(array),
      fdToClient,
      contextPtr);

     // Close the fd
     close(fdToClient);
}


void example_TriggerCallbackTest
(
    uint32_t data
)
{
    /* Static storage for data, so that it can be passed into le_event_QueueFunctionToThread().
     * This will get overwritten on a new call to this function, but given how the this test
     * works, that is not a problem.
     */
    static uint32_t dataStorage;
    dataStorage = data;

    if ( CallbackTestHandlerRef != NULL )
    {
        LE_PRINT_VALUE("%d", data);
        //CallbackTestHandlerRef(data, CallbackTestContextPtr);

        // Trigger the callback from the new thread.
        le_event_QueueFunctionToThread(NewThreadRef,
                                       CallbackTestHandlerQueued,
                                       &dataStorage,
                                       CallbackTestContextPtr);

    }
    else
    {
        LE_ERROR("Handler not registered\n");
    }
}



void* NewThread
(
    void* contextPtr
)
{
    le_event_RunLoop();
    return NULL;
}



COMPONENT_INIT
{
    example_AdvertiseService();

    // Start the second thread
    NewThreadRef = le_thread_Create("New thread", NewThread, NULL);
    le_thread_Start(NewThreadRef);
}
