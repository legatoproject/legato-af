/*
 * Test program for multiple clients and multiple threads per client.
 */

#include "legato.h"
#include "example_interface.h"
#include "le_print.h"


void banner(char *testName)
{
    int i;
    char banner[41];

    for (i=0; i<sizeof(banner)-1; i++)
        banner[i]='=';
    banner[sizeof(banner)-1] = '\0';

    LE_INFO("\n%s %s %s", banner, testName, banner);
}


void test1(void)
{
    uint32_t value=10;
    uint32_t data[] = {1, 2, 3, 4};
    size_t length=10;
    uint32_t output[length];
    char response[21];
    char more[21];

    example_allParameters(COMMON_TWO,
                          &value,
                          data,
                          4,
                          output,
                          &length,
                          "input string",
                          response,
                          sizeof(response),
                          more,
                          sizeof(more));

    LE_PRINT_VALUE("%i", value);
    LE_PRINT_ARRAY("%i", length, output);
    LE_PRINT_VALUE("%s", response);
    LE_PRINT_VALUE("%s", more);
}

static example_TestAHandlerRef_t HandlerRef;
static const char* ClientMessage = "initial value";


static void HandleTestA
(
    int32_t x,
    void* contextPtr
)
{
    static int count=0;
    count++;

    LE_PRINT_VALUE("%i", x);

    if ( contextPtr == ClientMessage )
    {
        LE_DEBUG("HandleTestA: context pointer works");
        LE_PRINT_VALUE( "'%s'", (char *)contextPtr );
    }
    else
    {
        LE_DEBUG("HandleTestA: context pointer fails");
    }

    // Re-do the test again for the given number of times.
    if ( count < 2 )
    {
        banner("Test 2 again");
        LE_PRINT_VALUE("%i", count);

        HandlerRef = example_AddTestAHandler(HandleTestA, (void*)ClientMessage);
        LE_PRINT_VALUE("%p", HandlerRef);

        LE_DEBUG("Triggering TestA yet again for count=%i\n", count);
        example_TriggerTestA();
    }
}

void test2(void)
{

    HandlerRef = example_AddTestAHandler(HandleTestA, (void*)ClientMessage);
    LE_PRINT_VALUE("%p", HandlerRef);

    LE_DEBUG("Triggering TestA\n");
    example_TriggerTestA();

    // Need to allow the event loop to process the trigger.
    // The rest of the test will be continued in the handler.
}


void StartTest
(
    void* param1Ptr,
    void* param2Ptr
)
{
    banner("Test 1");
    test1();
    banner("Test 2");
    test2();
}


void test3(void)
{
    uint32_t value=5;
    uint32_t data[] = {3, 9, 4, 1};
    size_t length=14;
    uint32_t output[length];
    char response[21];
    char more[21];

    example_allParameters(COMMON_THREE,
                          &value,
                          data,
                          4,
                          output,
                          &length,
                          "new thread string",
                          response,
                          sizeof(response),
                          more,
                          sizeof(more));

    LE_PRINT_VALUE("%i", value);
    LE_PRINT_ARRAY("%i", length, output);
    LE_PRINT_VALUE("%s", response);
    LE_PRINT_VALUE("%s", more);
}


void StartTestNewThread
(
    void* param1Ptr,
    void* param2Ptr
)
{
    banner("Test 3 on the new thread");
    test3();
}


void* NewThread
(
    void* contextPtr
)
{
    // Init IPC for the new thread
    example_ConnectService();

    banner("New Thread Started");

    // Wait a few seconds so that the output of the two tests does not overlap.  It makes it
    // much easier to verify the results.  Yes, this could be done with timers, but no harm
    // just using sleep() here, since this is not the main thread.
    sleep(10);

    // Start the test once the Event Loop is running.
    le_event_QueueFunction(StartTestNewThread, NULL, NULL);

    le_event_RunLoop();
    return NULL;
}


COMPONENT_INIT
{
    // Init IPC for the main thread
    example_ConnectService();

    // Get the client message from the first parameter on the command line
    if (le_arg_NumArgs() > 0)
    {
        ClientMessage = le_arg_GetArg(0);
    }

    // Start the test once the Event Loop is running.
    le_event_QueueFunction(StartTest, NULL, NULL);

    // Start a 2nd client thread
    le_thread_Start( le_thread_Create("New thread", NewThread, NULL) );
}


