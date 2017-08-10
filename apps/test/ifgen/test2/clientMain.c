#include <stdio.h>
#include <string.h>

#include "legato.h"
#include "example_interface.h"
#include "le_print.h"

#define BUFFERSIZE 1000

void banner(char *testName)
{
    int i;
    char banner[41];

    for (i=0; i<sizeof(banner)-1; i++)
        banner[i]='=';
    banner[sizeof(banner)-1] = '\0';

    LE_INFO("\n%s %s %s", banner, testName, banner);
}


void writeFdToLog(int fd)
{
    char buffer[BUFFERSIZE];
    ssize_t numRead;

    numRead = read(fd, buffer, sizeof(buffer));
    if (-1 == numRead)
    {
        LE_INFO("Read error: %s", strerror(errno));
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


void testFinal
(
    void* unusedOnePtr,
    void* unusedTwoPtr
)
{
    // Test starting/stopping service connections.
    // This has to be last test, because it will cause a fatal error.
    banner("Test Final");

    // Start a second connection
    example_ConnectService();

    // Disconnect the second connection, and try calling an API function. This should succeed.
    LE_DEBUG("Disconnect test; success expected");
    example_DisconnectService();
    example_TriggerTestA();

    // Disconnect the first connection, and try calling an API function.  This should fail.
    LE_DEBUG("Disconnect test; fatal error expected");
    example_DisconnectService();
    example_TriggerTestA();
}


static void NewHandleTestA
(
    int32_t x,
    void* contextPtr
)
{
    LE_PRINT_VALUE("%i", x);
}


void CallbackTestHandler
(
    uint32_t data,
    const char* namePtr,
    const uint8_t* array,
    size_t arrayLength,
    int dataFile,
    void* contextPtr
)
{
    LE_PRINT_VALUE("%d", data);
    LE_PRINT_VALUE("'%s'", namePtr);
    LE_PRINT_VALUE("%p", contextPtr);
    LE_PRINT_ARRAY("0x%02X", arrayLength, array);

    LE_PRINT_VALUE("%i", dataFile);

    // Read and print out whatever is read from the dataFile fd.
    writeFdToLog(dataFile);

    // This should fail, because the callback can only be called once.
    LE_DEBUG("Triggering CallbackTest second time -- should FATAL");
    example_TriggerCallbackTest(257);

    // Continue with the next test
    // todo: can't continue because the previous test fails -- maybe need to separate tests
    //le_event_QueueFunction(testFinal, NULL, NULL);
}


void test3(void)
{
    banner("Test 3");

    // Test what happens if an event is triggered, then the handler is removed.  The registered
    // handler should not be called, even if there is a pending event, because the handler has
    // been removed.
    example_TestAHandlerRef_t handlerRef = example_AddTestAHandler(NewHandleTestA, NULL);
    LE_PRINT_VALUE("%p", handlerRef);

    LE_DEBUG("Triggering New TestA\n");
    example_TriggerTestA();

    example_RemoveTestAHandler(handlerRef);


    // Test function callback parameters.
    int result;

    // This is not used in the test; this parameter was added to test a code generation bug fix.
    uint8_t dataArray[] = { 1, 2 };

    result = example_TestCallback(10, dataArray, 2, CallbackTestHandler, NULL);
    LE_PRINT_VALUE("%d", result);

    LE_DEBUG("Triggering CallbackTest");
    example_TriggerCallbackTest(100);

    // Need to allow the event loop to process the trigger.
    // The rest of the test will be continued in the handler.
}


static example_TestAHandlerRef_t HandlerRef;
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
    example_RemoveTestAHandler(HandlerRef);

    LE_DEBUG("Triggering TestA again");
    example_TriggerTestA();

    // Continue with next test
    test3();
}


void test2(void)
{

    HandlerRef = example_AddTestAHandler(HandleTestA, &SomeData);
    LE_PRINT_VALUE("%p", HandlerRef);

    // Try removing the handler and registering again, to ensure that allocated data objects
    // have been released, i.e. the associated client and server pools should not increase.
    example_RemoveTestAHandler(HandlerRef);
    HandlerRef = example_AddTestAHandler(HandleTestA, &SomeData);
    LE_PRINT_VALUE("%p", HandlerRef);

    LE_DEBUG("Triggering TestA\n");
    example_TriggerTestA();

    // Need to allow the event loop to process the trigger.
    // The rest of the test will be continued in the handler.
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

    // Call again with a special value, so that nothing is returned for the 'output', 'response'
    // and 'more' output parameters. This could happen in a typical function, if an error is
    // detected.

    // Make 'length' larger than actually defined for the 'output' parameter to verify that
    // only the maximum defined value is used on the server.
    length = 20;
    example_allParameters(COMMON_ZERO,
                          &value,
                          data,
                          4,
                          output,
                          &length,
                          "new string",
                          response,
                          sizeof(response),
                          more,
                          sizeof(more));

    LE_PRINT_VALUE("%i", value);
    LE_PRINT_ARRAY("%i", length, output);
    LE_PRINT_VALUE("%s", response);
    LE_PRINT_VALUE("%s", more);

    // Test file descriptors
    int fdToServer;
    int fdFromServer;

    // Open a file known to exist
    fdToServer = open("/usr/include/stdio.h", O_RDONLY);

    LE_PRINT_VALUE("%i", fdToServer);
    example_FileTest(fdToServer, &fdFromServer);
    LE_PRINT_VALUE("%i", fdFromServer);

    // Read and print out whatever is read from the server fd
    writeFdToLog(fdFromServer);
    close(fdToServer);
}


void StartTest(void)
{
    banner("Test 1");
    test1();

    // Verify that the client session can be stopped.
    banner("Test Stop/Restart Client");
    example_DisconnectService();

    // Should get an error message if trying to stop the client a second time.
    example_DisconnectService();

    // Re-connect to the service to continue the test
    example_ConnectService();

    banner("Test 2");
    test2();
}


COMPONENT_INIT
{
    banner("Test TryConnect");
    le_result_t result = example_TryConnectService();
    if ( result != LE_OK )
    {
        LE_ERROR("Could not connect to service on first try");

        // Let's wait a bit and try again
        sleep(15);
        result = example_TryConnectService();
        if ( result != LE_OK )
        {
            LE_FATAL("Could not connect to service on second and final try");
        }
    }
    LE_INFO("TryConnect works");

    StartTest();
}


