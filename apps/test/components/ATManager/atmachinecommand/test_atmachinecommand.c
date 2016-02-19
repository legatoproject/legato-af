/**
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

// Header files for CUnit
#include "Console.h"
#include <Basic.h>

#include "legato.h"
#include "atMachineCommand.h"
#include "atMachineString.h"

#define STRING_1    "STRING_1"
#define STRING_2    "STRING_2"

static atcmd_Response_t AtResp;

// NOTE: stub le_event_Report
void le_event_Report(le_event_Id_t eventId,
                     void * payloadPtr,
                     size_t payloadSize
)
{
    if (payloadSize!=sizeof(atcmd_Response_t)) {
        CU_TEST(false);
    } else {
        CU_TEST(true);
    }

    memcpy(&AtResp,payloadPtr,payloadSize);
}

/* The suite initialization function.
 * Opens the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int init_suite(void)
{
    atmachinecommand_Init();
    atmachinestring_Init();

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

static void testatcmd_TimerHandler (le_timer_Ref_t timerRef)
{
}

// Test this function :
// atcmd_Ref_t atcmd_Create(void);
void testatcmd_Create()
{
    uint32_t i;
    struct atcmd* newPtr = atcmd_Create();

    CU_ASSERT_PTR_NOT_NULL(newPtr);
    for (i=0;i< sizeof(newPtr->command);i++) {
        CU_ASSERT_EQUAL(newPtr->command[i],0);
    }
    CU_ASSERT_EQUAL(newPtr->commandSize,0);
    for (i=0;i< sizeof(newPtr->data);i++) {
        CU_ASSERT_EQUAL(newPtr->data[i],0);
    }
    CU_ASSERT_EQUAL(newPtr->dataSize,0);
    CU_ASSERT_PTR_EQUAL(le_dls_NumLinks(&newPtr->finaleResp),0);
    CU_ASSERT_PTR_NULL(newPtr->finalId);
    CU_ASSERT_PTR_EQUAL(le_dls_NumLinks(&newPtr->intermediateResp),0);
    CU_ASSERT_PTR_NULL(newPtr->intermediateId);
    CU_ASSERT_PTR_NOT_NULL(&newPtr->link);
    CU_ASSERT_EQUAL(newPtr->timer,0);
    CU_ASSERT_PTR_NULL(newPtr->timerHandler);
    CU_ASSERT_EQUAL(newPtr->waitExtra,false);
    CU_ASSERT_EQUAL(newPtr->withExtra,false);

    struct atcmd* new2Ptr = atcmd_Create();

    CU_ASSERT_PTR_NOT_NULL(new2Ptr);
    CU_ASSERT_NOT_EQUAL(new2Ptr->commandId,newPtr->commandId);

    le_mem_Release(newPtr);
    le_mem_Release(new2Ptr);

    CU_PASS("testatcmd_Create");
}

// Test this function :
// void atcmd_AddFinalResp(ATCommand_t *atcommandPtr,le_event_Id_t reportId,const char **listFinalPtr);
void testatcmd_AddFinalResp()
{
    uint32_t i;
    le_dls_Link_t* linkPtr;
    atmachinestring_t * currentPtr;
    const char* patternlist[] = {STRING_1,STRING_2,NULL};
    atcmd_Ref_t newPtr = atcmd_Create();
    le_event_Id_t eventId = le_event_CreateIdWithRefCounting("EventTest");

    atcmd_AddFinalResp(newPtr,eventId,patternlist);

    CU_ASSERT_PTR_EQUAL(newPtr->finalId,eventId);
    CU_ASSERT_EQUAL(le_dls_NumLinks(&newPtr->finaleResp),2);
    linkPtr = le_dls_Peek(&newPtr->finaleResp);
    currentPtr = CONTAINER_OF(linkPtr,atmachinestring_t,link);
    CU_ASSERT_EQUAL(strcmp(currentPtr->line,STRING_1),0);
    linkPtr = le_dls_PeekNext(&newPtr->finaleResp,&currentPtr->link);
    currentPtr = CONTAINER_OF(linkPtr,atmachinestring_t,link);
    CU_ASSERT_EQUAL(strcmp(currentPtr->line,STRING_2),0);


    CU_ASSERT_PTR_NOT_NULL(newPtr);
    for (i=0;i< sizeof(newPtr->command);i++) {
        CU_ASSERT_EQUAL(newPtr->command[i],0);
    }
    CU_ASSERT_EQUAL(newPtr->commandSize,0);
    for (i=0;i< sizeof(newPtr->data);i++) {
        CU_ASSERT_EQUAL(newPtr->data[i],0);
    }
    CU_ASSERT_EQUAL(newPtr->dataSize,0);
    CU_ASSERT_PTR_EQUAL(le_dls_NumLinks(&newPtr->intermediateResp),0);
    CU_ASSERT_PTR_NULL(newPtr->intermediateId);
    CU_ASSERT_PTR_NOT_NULL(&newPtr->link);
    CU_ASSERT_EQUAL(newPtr->timer,0);
    CU_ASSERT_PTR_NULL(newPtr->timerHandler);
    CU_ASSERT_EQUAL(newPtr->waitExtra,false);
    CU_ASSERT_EQUAL(newPtr->withExtra,false);

    le_mem_Release(newPtr);

    CU_PASS("testatcmd_AddFinalResp");
}

// Test this function :
// void atcmd_AddIntermediateResp(ATCommand_t *atcommandPtr,le_event_Id_t reportId,const char **listIntermediatePtr);
void testatcmd_AddIntermediateResp()
{
    uint32_t i;
    le_dls_Link_t* linkPtr;
    atmachinestring_t * currentPtr;
    const char* patternlist[] = {STRING_1,STRING_2,NULL};
    atcmd_Ref_t newPtr = atcmd_Create();
    le_event_Id_t eventId = le_event_CreateIdWithRefCounting("EventTest");

    atcmd_AddIntermediateResp(newPtr,eventId,patternlist);

    CU_ASSERT_PTR_EQUAL(newPtr->intermediateId,eventId);
    CU_ASSERT_EQUAL(le_dls_NumLinks(&newPtr->intermediateResp),2);
    linkPtr = le_dls_Peek(&newPtr->intermediateResp);
    currentPtr = CONTAINER_OF(linkPtr,atmachinestring_t,link);
    CU_ASSERT_EQUAL(strcmp(currentPtr->line,STRING_1),0);
    linkPtr = le_dls_PeekNext(&newPtr->intermediateResp,&currentPtr->link);
    currentPtr = CONTAINER_OF(linkPtr,atmachinestring_t,link);
    CU_ASSERT_EQUAL(strcmp(currentPtr->line,STRING_2),0);


    CU_ASSERT_PTR_NOT_NULL(newPtr);
    for (i=0;i< sizeof(newPtr->command);i++) {
        CU_ASSERT_EQUAL(newPtr->command[i],0);
    }
    CU_ASSERT_EQUAL(newPtr->commandSize,0);
    for (i=0;i< sizeof(newPtr->data);i++) {
        CU_ASSERT_EQUAL(newPtr->data[i],0);
    }
    CU_ASSERT_EQUAL(newPtr->dataSize,0);
    CU_ASSERT_PTR_EQUAL(le_dls_NumLinks(&newPtr->finaleResp),0);
    CU_ASSERT_PTR_NULL(newPtr->finalId);
    CU_ASSERT_PTR_NOT_NULL(&newPtr->link);
    CU_ASSERT_EQUAL(newPtr->timer,0);
    CU_ASSERT_PTR_NULL(newPtr->timerHandler);
    CU_ASSERT_EQUAL(newPtr->waitExtra,false);
    CU_ASSERT_EQUAL(newPtr->withExtra,false);

    le_mem_Release(newPtr);

    CU_PASS("testatcmd_AddIntermediateResp");
}

// Test this function :
// void atcmd_AddCommand(ATCommand_t *atcommandPtr,const char *commandPtr,bool extraData);
void testatcmd_AddCommand()
{
    uint32_t i;
    atcmd_Ref_t newPtr = atcmd_Create();

    atcmd_AddCommand(newPtr,STRING_1,true);

    CU_ASSERT_EQUAL(memcmp(newPtr->command,STRING_1,strlen(STRING_1)),0);
    CU_ASSERT_EQUAL(newPtr->commandSize,strlen(STRING_1));
    CU_ASSERT_EQUAL(newPtr->withExtra,true);

    CU_ASSERT_PTR_NOT_NULL(newPtr);
    for (i=0;i< sizeof(newPtr->data);i++) {
        CU_ASSERT_EQUAL(newPtr->data[i],0);
    }
    CU_ASSERT_EQUAL(newPtr->dataSize,0);
    CU_ASSERT_PTR_EQUAL(le_dls_NumLinks(&newPtr->finaleResp),0);
    CU_ASSERT_PTR_NULL(newPtr->finalId);
    CU_ASSERT_PTR_EQUAL(le_dls_NumLinks(&newPtr->intermediateResp),0);
    CU_ASSERT_PTR_NULL(newPtr->intermediateId);
    CU_ASSERT_PTR_NOT_NULL(&newPtr->link);
    CU_ASSERT_EQUAL(newPtr->timer,0);
    CU_ASSERT_PTR_NULL(newPtr->timerHandler);
    CU_ASSERT_EQUAL(newPtr->waitExtra,false);

    le_mem_Release(newPtr);

    CU_PASS("testatcmd_AddCommand");
}

// Test this function :
// void atcmd_AddData(ATCommand_t *atcommandPtr,const char *dataPtr,uint32_t dataSize);
void testatcmd_AddData()
{
    uint32_t i;
    atcmd_Ref_t newPtr = atcmd_Create();

    atcmd_AddData(newPtr,STRING_1,strlen(STRING_1));

    CU_ASSERT_EQUAL(memcmp(newPtr->data,STRING_1,strlen(STRING_1)),0);
    CU_ASSERT_EQUAL(newPtr->dataSize,strlen(STRING_1));

    CU_ASSERT_PTR_NOT_NULL(newPtr);
    for (i=0;i< sizeof(newPtr->command);i++) {
        CU_ASSERT_EQUAL(newPtr->command[i],0);
    }
    CU_ASSERT_EQUAL(newPtr->commandSize,0);
    CU_ASSERT_PTR_EQUAL(le_dls_NumLinks(&newPtr->finaleResp),0);
    CU_ASSERT_PTR_NULL(newPtr->finalId);
    CU_ASSERT_PTR_EQUAL(le_dls_NumLinks(&newPtr->intermediateResp),0);
    CU_ASSERT_PTR_NULL(newPtr->intermediateId);
    CU_ASSERT_PTR_NOT_NULL(&newPtr->link);
    CU_ASSERT_EQUAL(newPtr->timer,0);
    CU_ASSERT_PTR_NULL(newPtr->timerHandler);
    CU_ASSERT_EQUAL(newPtr->waitExtra,false);
    CU_ASSERT_EQUAL(newPtr->withExtra,false);

    le_mem_Release(newPtr);

    CU_PASS("testatcmd_AddData");
}

// Test this function :
// void atcmd_SetTimer(ATCommand_t *atcommandPtr,uint32_t timer,le_timer_ExpiryHandler_t handlerRef);
void testatcmd_SetTimer()
{
    uint32_t i;
    atcmd_Ref_t newPtr = atcmd_Create();

    atcmd_SetTimer(newPtr,3,testatcmd_TimerHandler);

    CU_ASSERT_EQUAL(newPtr->timer,3);
    CU_ASSERT_PTR_EQUAL(newPtr->timerHandler,testatcmd_TimerHandler);

    CU_ASSERT_PTR_NOT_NULL(newPtr);
    for (i=0;i< sizeof(newPtr->command);i++) {
        CU_ASSERT_EQUAL(newPtr->command[i],0);
    }
    CU_ASSERT_EQUAL(newPtr->commandSize,0);
    for (i=0;i< sizeof(newPtr->data);i++) {
        CU_ASSERT_EQUAL(newPtr->data[i],0);
    }
    CU_ASSERT_EQUAL(newPtr->dataSize,0);
    CU_ASSERT_PTR_EQUAL(le_dls_NumLinks(&newPtr->finaleResp),0);
    CU_ASSERT_PTR_NULL(newPtr->finalId);
    CU_ASSERT_PTR_EQUAL(le_dls_NumLinks(&newPtr->intermediateResp),0);
    CU_ASSERT_PTR_NULL(newPtr->intermediateId);
    CU_ASSERT_PTR_NOT_NULL(&newPtr->link);
    CU_ASSERT_EQUAL(newPtr->waitExtra,false);
    CU_ASSERT_EQUAL(newPtr->withExtra,false);

    le_mem_Release(newPtr);

    CU_PASS("testatcmd_SetTimer");
}

// Test this function :
// void atmachinecommand_CheckIntermediate(ATCommand_t *atcommandPtr,char *atLinePtr,size_t atLineSize);
void testatmachinecommand_CheckIntermediate()
{
    atcmd_Ref_t newPtr = atcmd_Create();
    le_event_Id_t eventId = le_event_CreateIdWithRefCounting("EventTest");
    const char* patternlist[] = {STRING_1,STRING_2,NULL};
    memset(&AtResp,0,sizeof(AtResp));

    atcmd_AddIntermediateResp(newPtr,eventId,patternlist);

    atmachinecommand_CheckIntermediate(newPtr,STRING_2,strlen(STRING_2));

    CU_ASSERT_PTR_EQUAL(AtResp.fromWhoRef,newPtr);
    CU_ASSERT_EQUAL(strlen(AtResp.line),strlen(STRING_2));
    CU_ASSERT_EQUAL(memcmp(AtResp.line,STRING_2,strlen(STRING_2)),0);

    le_mem_Release(newPtr);
    CU_PASS("atmachinecommand_CheckIntermediate");
}

// Test this function :
// bool atmachinecommand_CheckFinal(ATCommand_t *atcommandPtr,char *atLinePtr,size_t atLineSize);
void testatmachinecommand_CheckFinal()
{
    atcmd_Ref_t newPtr = atcmd_Create();
    le_event_Id_t eventId = le_event_CreateIdWithRefCounting("EventTest");
    const char* patternlist[] = {STRING_1,STRING_2,NULL};
    memset(&AtResp,0,sizeof(AtResp));

    atcmd_AddFinalResp(newPtr,eventId,patternlist);

    atmachinecommand_CheckFinal(newPtr,STRING_1,strlen(STRING_1));

    CU_ASSERT_PTR_EQUAL(AtResp.fromWhoRef,newPtr);
    CU_ASSERT_EQUAL(strlen(AtResp.line),strlen(STRING_1));
    CU_ASSERT_EQUAL(memcmp(AtResp.line,STRING_1,strlen(STRING_1)),0);

    le_mem_Release(newPtr);
    CU_PASS("atmachinecommand_CheckFinal");
}

#define COMMAND     "AT"
#define DATA        "1234567890"
// Test this function :
// void atmachinecommand_Prepare(ATCommand_t *atcommandPtr);
void testatmachinecommand_Prepare()
{
    atcmd_Ref_t newPtr = atcmd_Create();

    strcpy((char*)newPtr->command,COMMAND);
    newPtr->commandSize = strlen(COMMAND);
    strcpy((char*)newPtr->data,DATA);
    newPtr->dataSize = strlen(DATA);

    atmachinecommand_Prepare(newPtr);

    CU_ASSERT_EQUAL(newPtr->commandSize,strlen(COMMAND)+1);
    CU_ASSERT_EQUAL(newPtr->command[newPtr->commandSize-1],'\r');
    CU_ASSERT_EQUAL(newPtr->dataSize,strlen(DATA)+1);
    CU_ASSERT_EQUAL(newPtr->data[newPtr->dataSize-1],0x1A);

    le_mem_Release(newPtr);
    CU_PASS("atmachinecommand_Prepare");
}

COMPONENT_INIT
{
    int result = EXIT_SUCCESS;

    // Init the test case / test suite data structures

    CU_TestInfo test[] =
    {
        { "Test atcmd_Create", testatcmd_Create },
        { "Test atcmd_AddFinalResp", testatcmd_AddFinalResp },
        { "Test atcmd_AddIntermediateResp", testatcmd_AddIntermediateResp },
        { "Test atcmd_AddCommand", testatcmd_AddCommand },
        { "Test atcmd_AddData", testatcmd_AddData },
        { "Test atcmd_SetTimer", testatcmd_SetTimer },
        { "Test atmachinecommand_Prepare", testatmachinecommand_Prepare },
        { "Test atmachinecommand_CheckIntermediate", testatmachinecommand_CheckIntermediate },
        { "Test atmachinecommand_CheckFinal", testatmachinecommand_CheckFinal },
        CU_TEST_INFO_NULL,
    };

    CU_SuiteInfo suites[] =
    {
        { "AT Command tests",       init_suite, clean_suite, test },
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
