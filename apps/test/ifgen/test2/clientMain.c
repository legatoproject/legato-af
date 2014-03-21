#include <stdio.h>
#include <string.h>

#include "legato.h"
#include "interface.h"
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

    allParameters(3, &value, data, 4, output, &length, "input string", response, sizeof(response));

    LE_PRINT_VALUE("%i", value);
    LE_PRINT_ARRAY("%i", length, output);
    LE_PRINT_VALUE("%s", response);
}

static TestARef_t HandlerRef;
static uint32_t SomeData = 100;


static void HandleTestA
(
    int32_t x,
    void* contextPtr
)
{
    LE_PRINT_VALUE("%i", x);

    if ( contextPtr == &SomeData )
    {
        LE_DEBUG("HandleTestA: context pointer works");
        LE_PRINT_VALUE( "%u", *((uint32_t*)contextPtr) );
    }
    else
    {
        LE_DEBUG("HandleTestA: context pointer fails");
    }

    // continue the rest of the test
    LE_DEBUG("Removing TestA");
    RemoveTestA(HandlerRef);

    LE_DEBUG("Triggering TestA again");
    TriggerTestA();

    // This should get a DBUG message, since the service instance name is the same
    StartClient("Test 2");
 
    // This should get an ERROR message, since the service instance name is different
    StartClient("Different Test 2");
}

void test2(void)
{

    HandlerRef = AddTestA(HandleTestA, &SomeData);
    LE_PRINT_VALUE("%p", HandlerRef);

    // Try removing the handler and registering again, to ensure that allocated data objects
    // have been released, i.e. the associated client and server pools should not increase.
    RemoveTestA(HandlerRef);
    HandlerRef = AddTestA(HandleTestA, &SomeData);
    LE_PRINT_VALUE("%p", HandlerRef);

    LE_DEBUG("Triggering TestA\n");
    TriggerTestA();

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

    // Verify that the client session can be stopped.  A new session will be created
    // as soon as any API function is called.
    banner("Test Stop/Restart Client");
    StopClient();
 
    // Should get an error message if trying to stop the client a second time.
    StopClient();

    banner("Test 2");
    test2();
}

LE_EVENT_INIT_HANDLER
{
    StartClient("Test 2");

    // Start the test once the Event Loop is running.
    le_event_QueueFunction(StartTest, NULL, NULL);
}


