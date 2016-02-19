/**
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

// Header files for CUnit
#include "Console.h"
#include <Basic.h>

#include "atMachineFsm.h"
#include "atMachineDevice.h"
#include "atMachineParser.h"
#include "atMachineParser.c"

#define BUFFER_1            "OK"
#define BUFFER_1_TEST       "\r\n"BUFFER_1"\r\n"

#define BUFFER_2            "78910"
#define BUFFER_2_TEST_COPY  BUFFER_2"\r\n"
#define BUFFER_2_TEST       "0123456\r\n"BUFFER_2_TEST_COPY

#define BUFFER_CONNECT      "CONNECT 115200"
#define BUFFER_3            "\r\n"BUFFER_CONNECT
#define BUFFER_3_NEXT       "\r\nblabla"

char  Line[ATPARSER_LINE_MAX];

// NOTE: stub function
void atmachinemanager_ProcessLine
(
    ATManagerStateMachineRef_t smRef,
    char *  linePtr,
    size_t  lineSize
)
{
    if (lineSize>ATPARSER_LINE_MAX-1) {
        CU_TEST(false);
    } else {
        CU_TEST(true);
    }

    memcpy(Line,linePtr,lineSize);
    Line[lineSize] = '\0';
}


/* The suite initialization function.
 * Opens the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int init_suite(void)
{
    return 0;
}

/* The suite cleanup function.
 * Closes the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int clean_suite(void)
{
    return 0;
}

// Test this function :
// void atmachineparser_InitializeState(ATParserStateMachineRef_t  smRef);
void testatmachineparser_InitializeState()
{
    ATParserStateMachine_t atParser;

    atmachineparser_InitializeState(&atParser);

    CU_ASSERT_PTR_EQUAL(atParser.curState,StartingState);

    CU_PASS("atmachineparser_InitializeState");
}

// Test this function :
// void read_buffer(ATParserStateMachineRef_t smRef);
void testread_buffer()
{
    ATParserStateMachine_t atParser;

    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------

    memset(&atParser,0,sizeof(atParser));
    memset(Line,0,sizeof(Line));
    atParser.curState = StartingState;
    memcpy(atParser.curContext.buffer,BUFFER_1_TEST,strlen(BUFFER_1_TEST));
    atParser.curContext.endbuffer = strlen(BUFFER_1_TEST);
    atParser.curContext.idx = 0;
    atParser.curContext.idx_lastCRLF = 0;

    atmachinedevice_PrintBuffer("BUFFER_1_TEST",atParser.curContext.buffer,atParser.curContext.endbuffer);
    atmachineparser_ReadBuffer(&atParser);

    CU_ASSERT_EQUAL(atParser.curContext.endbuffer,strlen(BUFFER_1_TEST));
    CU_ASSERT_EQUAL(atParser.curContext.idx,strlen(BUFFER_1_TEST));
    CU_ASSERT_EQUAL(atParser.curContext.idx_lastCRLF,strlen(BUFFER_1_TEST));

    CU_ASSERT_EQUAL(strlen(Line),strlen(BUFFER_1));
    CU_ASSERT_EQUAL(strcmp(Line,BUFFER_1),0);

    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------
    memset(&atParser,0,sizeof(atParser));
    memset(Line,0,sizeof(Line));
    atParser.curState = StartingState;
    memcpy(atParser.curContext.buffer,BUFFER_2_TEST,strlen(BUFFER_2_TEST));
    atParser.curContext.endbuffer = strlen(BUFFER_2_TEST);
    atParser.curContext.idx = 0;
    atParser.curContext.idx_lastCRLF = 0;

    atmachinedevice_PrintBuffer("BUFFER_2_TEST",atParser.curContext.buffer,atParser.curContext.endbuffer);
    atmachineparser_ReadBuffer(&atParser);

    CU_ASSERT_EQUAL(atParser.curContext.endbuffer,strlen(BUFFER_2_TEST));
    CU_ASSERT_EQUAL(atParser.curContext.idx,strlen(BUFFER_2_TEST));
    CU_ASSERT_EQUAL(atParser.curContext.idx_lastCRLF,strlen(BUFFER_2_TEST));

    CU_ASSERT_EQUAL(strlen(Line),strlen(BUFFER_2));
    CU_ASSERT_EQUAL(strcmp(Line,BUFFER_2),0);

    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------
    memset(&atParser,0,sizeof(atParser));
    memset(Line,0,sizeof(Line));
    atParser.curState = StartingState;
    memcpy(atParser.curContext.buffer,BUFFER_2_TEST,strlen(BUFFER_2_TEST)-1);
    atParser.curContext.endbuffer = strlen(BUFFER_2_TEST)-1;
    atParser.curContext.idx = 0;
    atParser.curContext.idx_lastCRLF = 0;

    atmachinedevice_PrintBuffer("BUFFER_2_TEST",atParser.curContext.buffer,atParser.curContext.endbuffer);
    atmachineparser_ReadBuffer(&atParser);

    CU_ASSERT_EQUAL(atParser.curContext.endbuffer,strlen(BUFFER_2_TEST)-1);
    CU_ASSERT_EQUAL(atParser.curContext.idx,strlen(BUFFER_2_TEST)-1);
    CU_ASSERT_EQUAL(atParser.curContext.idx_lastCRLF,9);

    CU_ASSERT_EQUAL(strlen(Line),0);

    CU_PASS("read_buffer");

    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------
    memset(&atParser,0,sizeof(atParser));
    memset(Line,0,sizeof(Line));
    atParser.curState = StartingState;
    memcpy(atParser.curContext.buffer,BUFFER_3,strlen(BUFFER_3));
    atParser.curContext.endbuffer = strlen(BUFFER_3);
    atParser.curContext.idx = 0;
    atParser.curContext.idx_lastCRLF = 0;

    atmachinedevice_PrintBuffer("BUFFER_3",atParser.curContext.buffer,atParser.curContext.endbuffer);
    atmachineparser_ReadBuffer(&atParser);

    CU_ASSERT_EQUAL(atParser.curContext.endbuffer,strlen(BUFFER_3));
    CU_ASSERT_EQUAL(atParser.curContext.idx,strlen(BUFFER_3));
    CU_ASSERT_EQUAL(atParser.curContext.idx_lastCRLF,2);

    CU_ASSERT_EQUAL(strlen(Line),0);

    memcpy(&atParser.curContext.buffer[atParser.curContext.idx],BUFFER_3_NEXT,strlen(BUFFER_3_NEXT));
    atParser.curContext.endbuffer = strlen(BUFFER_3)+strlen(BUFFER_3_NEXT);

    atmachinedevice_PrintBuffer("BUFFER_3_NEXT",atParser.curContext.buffer,atParser.curContext.endbuffer);
    atmachineparser_ReadBuffer(&atParser);

    CU_ASSERT_EQUAL(strlen(Line),strlen(BUFFER_CONNECT));
    CU_ASSERT_EQUAL(strcmp(Line,BUFFER_CONNECT),0);

    CU_PASS("read_buffer");
}

// Test this function :
// void resetBuffer(ATParserStateMachineRef_t smRef);
void testresetBuffer()
{
    ATParserStateMachine_t atParser;

    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------

    memset(&atParser,0,sizeof(atParser));
    atParser.curState = StartingState;
    memcpy(atParser.curContext.buffer,BUFFER_1_TEST,strlen(BUFFER_1_TEST));
    atParser.curContext.endbuffer = strlen(BUFFER_1_TEST);
    atParser.curContext.idx = strlen(BUFFER_1_TEST);
    atParser.curContext.idx_lastCRLF = 0;

    atmachinedevice_PrintBuffer("BUFFER_1_TEST",atParser.curContext.buffer,atParser.curContext.endbuffer);
    atmachineparser_ResetBuffer(&atParser);

    CU_ASSERT_EQUAL(atParser.curContext.endbuffer,strlen(BUFFER_1_TEST));
    CU_ASSERT_EQUAL(atParser.curContext.idx,strlen(BUFFER_1_TEST));
    CU_ASSERT_EQUAL(atParser.curContext.idx_lastCRLF,0);

    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------

    memset(&atParser,0,sizeof(atParser));
    atParser.curState = InitializingState;
    memcpy(atParser.curContext.buffer,BUFFER_1_TEST,strlen(BUFFER_1_TEST));
    atParser.curContext.endbuffer = strlen(BUFFER_1_TEST);
    atParser.curContext.idx = strlen(BUFFER_1_TEST);
    atParser.curContext.idx_lastCRLF = 0;

    atmachinedevice_PrintBuffer("BUFFER_1_TEST",atParser.curContext.buffer,atParser.curContext.endbuffer);
    atmachineparser_ResetBuffer(&atParser);

    CU_ASSERT_EQUAL(atParser.curContext.endbuffer,strlen(BUFFER_1_TEST));
    CU_ASSERT_EQUAL(atParser.curContext.idx,strlen(BUFFER_1_TEST));
    CU_ASSERT_EQUAL(atParser.curContext.idx_lastCRLF,0);

    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------

    memset(&atParser,0,sizeof(atParser));
    atParser.curState = ProcessingState;
    memcpy(atParser.curContext.buffer,BUFFER_1_TEST,strlen(BUFFER_1_TEST));
    atParser.curContext.endbuffer = strlen(BUFFER_1_TEST);
    atParser.curContext.idx = strlen(BUFFER_1_TEST);
    atParser.curContext.idx_lastCRLF = strlen(BUFFER_1_TEST) - 2;

    atmachinedevice_PrintBuffer("BUFFER_1_TEST",atParser.curContext.buffer,atParser.curContext.endbuffer);
    atmachineparser_ResetBuffer(&atParser);

    CU_ASSERT_EQUAL(atParser.curContext.endbuffer,strlen(BUFFER_1_TEST) - 2);
    CU_ASSERT_EQUAL(atParser.curContext.idx,strlen(BUFFER_1_TEST) - 2);
    CU_ASSERT_EQUAL(atParser.curContext.idx_lastCRLF,2);

    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------

    memset(&atParser,0,sizeof(atParser));
    atParser.curState = ProcessingState;
    memcpy(atParser.curContext.buffer,BUFFER_1_TEST,strlen(BUFFER_1_TEST));
    atParser.curContext.endbuffer = strlen(BUFFER_1_TEST)-1;
    atParser.curContext.idx = strlen(BUFFER_1_TEST)-1;
    atParser.curContext.idx_lastCRLF = 2;

    atmachinedevice_PrintBuffer("BUFFER_1_TEST",atParser.curContext.buffer,atParser.curContext.endbuffer);
    atmachineparser_ResetBuffer(&atParser);

    CU_ASSERT_EQUAL(atParser.curContext.endbuffer,strlen(BUFFER_1_TEST)-1);
    CU_ASSERT_EQUAL(atParser.curContext.idx,strlen(BUFFER_1_TEST)-1);
    CU_ASSERT_EQUAL(atParser.curContext.idx_lastCRLF,2);

    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------

    memset(&atParser,0,sizeof(atParser));
    atParser.curState = ProcessingState;
    memcpy(atParser.curContext.buffer,BUFFER_2_TEST,strlen(BUFFER_2_TEST));
    atParser.curContext.endbuffer = strlen(BUFFER_2_TEST);
    atParser.curContext.idx = strlen(BUFFER_2_TEST);
    atParser.curContext.idx_lastCRLF = 9;

    atmachinedevice_PrintBuffer("BUFFER_2_TEST",atParser.curContext.buffer,atParser.curContext.endbuffer);
    atmachineparser_ResetBuffer(&atParser);
    atmachinedevice_PrintBuffer("RESET BUFFER_2_TEST",atParser.curContext.buffer,atParser.curContext.endbuffer);

    CU_ASSERT_EQUAL(atParser.curContext.endbuffer,strlen(BUFFER_2_TEST_COPY)+2);
    CU_ASSERT_EQUAL(atParser.curContext.idx,strlen(BUFFER_2_TEST_COPY)+2);
    CU_ASSERT_EQUAL(atParser.curContext.idx_lastCRLF,2);

    CU_PASS("resetBuffer");
}

COMPONENT_INIT
{
    int result = EXIT_SUCCESS;

    // Init the test case / test suite data structures

    CU_TestInfo test[] =
    {
        { "Test atmachineparser_InitializeState" , testatmachineparser_InitializeState },
        { "Test resetBuffer" , testresetBuffer },
        { "Test read_buffer" , testread_buffer },
        CU_TEST_INFO_NULL,
    };

    CU_SuiteInfo suites[] =
    {
        { "atparsertests",           init_suite, clean_suite, test },
        CU_SUITE_INFO_NULL,
    };

    // Initialize the CUnit test registry and register the test suite
    if (CUE_SUCCESS != CU_initialize_registry())
        exit(CU_get_error());

    if ( CUE_SUCCESS != CU_register_suites(suites))
    {
        CU_cleanup_registry();
        exit(CU_get_error());
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    // Output summary of failures, if there were any
    if ( CU_get_number_of_failures() > 0 )
    {
        fprintf(stdout,"\n [START]List of Failure :\n");
        CU_basic_show_failures(CU_get_failure_list());
        fprintf(stdout,"\n [STOP]List of Failure\n");
        result = EXIT_FAILURE;
    }

    CU_cleanup_registry();
    exit(result);
}
