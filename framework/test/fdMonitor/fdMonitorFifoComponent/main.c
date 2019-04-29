//--------------------------------------------------------------------------------------------------
/*
 * Test monitoring a FIFO
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * File descriptor for read end of the fifo.
 */
//--------------------------------------------------------------------------------------------------
static int ReadFD = -1;

//--------------------------------------------------------------------------------------------------
/**
 * File descriptor for write end of the fifo.
 */
//--------------------------------------------------------------------------------------------------
static int WriteFD = -1;

//--------------------------------------------------------------------------------------------------
/**
 * File descriptor monitor
 */
//--------------------------------------------------------------------------------------------------
static le_fdMonitor_Ref_t FifoMonitor;

//--------------------------------------------------------------------------------------------------
/**
 * Test string to push through the FIFO.
 *
 * Must be small so writing to the FIFO doesn't block
 */
//--------------------------------------------------------------------------------------------------
static char TestString[] = "Mary had a little lamb whose fleece was white as snow";


//--------------------------------------------------------------------------------------------------
/**
 * Write to FIFO after a delay to wake up the main thread with the write
 */
//--------------------------------------------------------------------------------------------------
static void *FifoWriter
(
    void *context
)
{
    LE_UNUSED(context);

    // Delay to ensure main thread is sleeping
    LE_TEST_INFO("Delaying write");
    sleep(1);

    if (le_fd_Write(WriteFD, TestString, sizeof(TestString)) != sizeof(TestString))
    {
        LE_TEST_FATAL("Failed to write test string to FIFO");
    }
    LE_TEST_INFO("Test string write finished");

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Run the next test in sequence, and return TRUE if there are more tests to run
 */
//--------------------------------------------------------------------------------------------------
static bool RunNextTest
(
    void
)
{
    static int testNum = 0;
    static le_thread_Ref_t writeThread = NULL;

    switch (testNum++)
    {
        case 0:
            // Test writing to FIFO when it's not currently blocked in the event loop
            LE_TEST_ASSERT(le_fd_Write(WriteFD, TestString, sizeof(TestString))
                           == sizeof(TestString),
                           "Write test string to FIFO");
            return true;

        case 1:
            // Test writing to the FIFO when it's blocked in the event loop by creating a
            // second thread which writes to the FIFO
            LE_TEST_INFO("Creating thread to write to FIFO");
            writeThread = le_thread_Create("FifoWriteThread",
                                           FifoWriter,
                                           NULL);
            if (!writeThread)
            {
                LE_TEST_FATAL("Cannot create writer thread");
            }
            le_thread_Start(writeThread);
            return true;

        default:
        {
            // No more tests
            return false;
        }
    }
}



static void FifoReadHandler
(
    int fd,
    short events
)
{
    char buffer[sizeof(TestString)];
    ssize_t readSize;

    LE_TEST_OK(fd == ReadFD, "Received event from read end of FIFO");
    LE_TEST_OK(events == POLLIN, "Received POLLIN");

    readSize = le_fd_Read(ReadFD, buffer, sizeof(TestString));
    LE_TEST_OK(readSize > 0, "Read succeeded from FIFO");
    LE_TEST_OK(readSize == sizeof(TestString) &&
               memcmp(buffer, TestString, sizeof(TestString)) == 0,
               "Read data '%s' matches test string '%s'", (readSize>0?buffer:""), TestString);

    // Do multiple tests which all re-enter this function.  If RunNextTest() returns true,
    // it means there's another test to run.
    if (RunNextTest())
    {
        return;
    }
    else
    {
        le_fd_Close(ReadFD);
        le_fd_Close(WriteFD);
        le_fdMonitor_Delete(FifoMonitor);
        LE_TEST_EXIT;
    }
}

COMPONENT_INIT
{
    static const char fifoPath[] = "/tmp/fifoMonitorTestDevice";

    LE_TEST_PLAN(10);

    // Prepare FIFO for for test.  Since this is a test of fdMonitor, not FIFO, do
    // not record these as tests.  But we still need to bail if this fails for
    // an unexpected reason
    LE_TEST_INFO("Preparing FIFO for test");
    if (le_fd_MkFifo(fifoPath, S_IRUSR | S_IWUSR) != LE_OK)
    {
        LE_TEST_FATAL("Cannot create fifo test device");
    }

    ReadFD = le_fd_Open(fifoPath, O_RDONLY | O_NONBLOCK);
    if (ReadFD == -1)
    {
        LE_TEST_FATAL("Cannot open read end of test device");
    }

    WriteFD = le_fd_Open(fifoPath, O_WRONLY | O_NONBLOCK);
    if (WriteFD == -1)
    {
        LE_TEST_FATAL("Cannot open write end of test device");
    }

    // Create monitor for FIFO
    FifoMonitor = le_fdMonitor_Create("FIFO",
                                      ReadFD,
                                      FifoReadHandler,
                                      POLLIN);

    LE_TEST_ASSERT(FifoMonitor, "Create FD monitor on FIFO");

    RunNextTest();
}
