 /**
  * This module is for integration testing of the flash component (dual system case)
  *
  * * You must issue the following commands:
  * @verbatim
  * $ app start flashTest
  * $ app runProc flashTest --exe=flashTest -- <arg1> [<arg2>]
  *
  * Example:
  * $ app runProc flashTest --exe=flashTest -- help
  * @endverbatim
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Detection handler
 *
 */
//--------------------------------------------------------------------------------------------------
static le_flash_BadImageDetectionHandlerRef_t DetectionHandler = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Print function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Print
(
    const char* string
)
{
    bool sandboxed = (getuid() != 0);

    if(sandboxed)
    {
        LE_INFO("%s", string);
    }
    else
    {
        fprintf(stderr, "%s\n", string);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Help.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage
(
    void
)
{
    int idx;
    const char * usagePtr[] = {
            "Usage of the 'flashTest' application is:",
            "flashTest -- start: start the bad image notification",
            "",
    };

    for(idx = 0; idx < NUM_ARRAY_MEMBERS(usagePtr); idx++)
    {
        Print(usagePtr[idx]);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Bad image handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void BadImageHandler
(
    const char *imageName,
    void *contextPtr
)
{
    LE_INFO("imageName=%s", imageName);
    Print (imageName);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Add State Change Handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void* TestThread(void* context)
{

    le_flash_ConnectService();

    LE_INFO("Add bad image Handler");
    DetectionHandler = le_flash_AddBadImageDetectionHandler(BadImageHandler, NULL);
    LE_INFO("flashTest: DetectionHandler 0x%x", (uint32_t)DetectionHandler);

    LE_INFO("No event loop");
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * The signal event handler function for SIGINT/SIGTERM when process dies.
 */
//--------------------------------------------------------------------------------------------------
static void SigHandler
(
    int sigNum
)
{
    LE_INFO("End and delete test flash");
    le_flash_RemoveBadImageDetectionHandler(DetectionHandler);
    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Main thread.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    const char* testString = "";
    char string[100];

    LE_INFO("Start flashTest app.");
    memset (string, 0, sizeof(string));

    // Register a signal event handler for SIGINT when user interrupts/terminates process
    signal(SIGINT, SigHandler);

    /* Get the test identifier */
    if (le_arg_NumArgs() >= 1)
    {
        testString = le_arg_GetArg(0);
    }

    if (0 == strcmp(testString, "help"))
    {
        PrintUsage();
        exit(0);
    }
    else if (0 == strcmp(testString, "start"))
    {
        // Add State Change Handler
        le_thread_Start(le_thread_Create("TestThread",TestThread,NULL));
    }
    else
    {
        LE_DEBUG ("flashTest: not supported arg");
        PrintUsage();
        exit(EXIT_FAILURE);
    }
}
