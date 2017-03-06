/*
 * Test ServiceLoop() function
 */


#include "legato.h"

void MyRunLoop
(
    void
)
{
    struct pollfd pollControl;
    int pollResult;
    le_result_t result;

    // Get the Legato event loop "readyness" file descriptor and put it in a pollfd struct
    // configured to detect "ready to read".
    pollControl.fd = le_event_GetFd();
    pollControl.events = POLLIN;

    while (true)
    {
        // Block until the file descriptor is "ready to read".
        LE_INFO("Starting poll ...");
        pollResult = poll(&pollControl, 1, -1);

        LE_INFO("pollResult = %i", pollResult);

        if (pollResult > 0)
        {
            while (1)
            {
                // The Legato event loop needs servicing.
                // Keep servicing it until there is nothing left.
                result = le_event_ServiceLoop();
                LE_INFO("result = %i", result);

                // No more work, so break out
                if ( result != LE_OK )
                {
                    LE_INFO("No more events");
                    break;
                }
            }
        }
        else
        {
            LE_FATAL("poll() failed with errno %m.");
        }
    }
}


void StartTestNewThread
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_INFO("Queued function on new thread");
}


void* NewThread
(
    void* contextPtr
)
{
    LE_INFO("New Thread Started");

    // Queue the function several times; if working correctly, all three times should be called.
    le_event_QueueFunction(StartTestNewThread, NULL, NULL);
    le_event_QueueFunction(StartTestNewThread, NULL, NULL);
    le_event_QueueFunction(StartTestNewThread, NULL, NULL);

    MyRunLoop();
    return NULL;
}


COMPONENT_INIT
{
    // Start a 2nd thread for testing ServiceLoop().
    LE_INFO("About to start new thread");
    le_thread_Start( le_thread_Create("New thread", NewThread, NULL) );
}
