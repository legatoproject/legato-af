#include "legato.h"

// global check variable.
static int checkCount = 0;
static le_event_Id_t delEvent;

static void SigUser1Handler(int sigNum)
{
    LE_ASSERT(sigNum == SIGUSR1);
    LE_INFO("%s received through fd handler.", strsignal(sigNum));

    switch (checkCount)
    {
        case 1:
        {
            // Report event to delete sigs for our thread.
            le_event_Report(delEvent, NULL, 0);

            break;
        }

        default: LE_FATAL("Should not be here.");
    }
}

static void SigUser2Handler(int sigNum)
{
    LE_INFO("%s received through fd handler.", strsignal(sigNum));

    switch (checkCount)
    {
        case 2:
        {
            // Send to the other thread.  But the other thread should not get it.
            checkCount++;
            LE_ASSERT(kill(getpid(), SIGUSR1) == 0);

            // Make sure that the signal to thread 1 is sent first.
            sleep(1);

            // Send to ourself and we should get it
            checkCount++;
            LE_ASSERT(kill(getpid(), SIGUSR2) == 0);

            break;
        }

        case 4:
        {
            LE_INFO("======== Signal Events Test Completed (PASSED) ========");
            exit(EXIT_SUCCESS);
        }

        default: LE_FATAL("Should not be here.");
    }
}


static void DeleteSigs(void* reportPtr)
{
    // Delete all signal events from this thread.
    le_sig_DeleteAll();

    // Send SIGUSR2 to our own process.
    checkCount++;
    LE_ASSERT(kill(getpid(), SIGUSR2) == 0);
}


static void* Thread1(void* context)
{
    // Block unused signal in separate thread.  This should generate a warning.
    le_sig_Block(SIGCHLD);

    // Create delete signal event.
    delEvent = le_event_CreateId("DeleteSigs", 0);
    le_event_AddHandler("DelSigHandler", delEvent, DeleteSigs);

    le_sig_SetEventHandler(SIGUSR1, SigUser1Handler);

    // Start the test procedure by sending a SIGUSR1 signal to our own process.
    checkCount++;
    LE_ASSERT(kill(getpid(), SIGUSR1) == 0);

    le_event_RunLoop();

    return NULL;
}

static void* Thread2(void* context)
{
    le_sig_SetEventHandler(SIGUSR2, SigUser2Handler);

    le_event_RunLoop();

    return NULL;
}


COMPONENT_INIT
{
    LE_INFO("======== Begin Signal Events Test ========");

    // Block signals.  All signals that are to be used in signal events must be blocked here.
    le_sig_Block(SIGUSR1);
    le_sig_Block(SIGUSR2);

    le_thread_Ref_t thread1 = le_thread_Create("Thread1", Thread1, NULL);
    le_thread_Ref_t thread2 = le_thread_Create("Thread2", Thread2, NULL);

    le_thread_Start(thread1);
    le_thread_Start(thread2);
}
