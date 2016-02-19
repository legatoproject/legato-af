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

#include "atMachineCommand.h"
#include "atMachineMgrItf.h"
#include "atMachineMgrItf.c"
#include "atPorts.h"
#include "atPortsInternal.h"

static struct atmgr* InterfacePtr=NULL;
#define     DEVICE_PORT     "./test_atmanageritf.log"
static struct atdevice  Device;

static bool IsStop = true;
static atcmd_Ref_t AtCmdPtr=NULL;

static le_event_Id_t EventIdAddUnsolicitedFalse;
static le_event_Id_t EventIdAddUnsolicitedtrue;

#define     UNSOL_FALSE     "Unsolicited False"
static bool IsUnsolFalseSet;
#define     UNSOL_TRUE      "Unsolicited true"
static bool IsUnsolTrueSet;

// NOTE: Stub function
void atmachinemanager_Resume(void* reportPtr)
{
    IsStop = false;
    le_sem_Post(InterfacePtr->waitingSemaphore);
}

// NOTE: Stub function
void atmachinemanager_Suspend(void* reportPtr)
{
    IsStop = true;
    le_sem_Post(InterfacePtr->waitingSemaphore);
}

// NOTE: Stub function
void atmachinemanager_AddUnsolicited(void *reportPtr)
{
    atUnsolicited_t* unsolicitedPtr = reportPtr;

    if ( strcmp(unsolicitedPtr->unsolRsp,UNSOL_FALSE)==0) {
        IsUnsolFalseSet = true;
        return;
    }
    if ( strcmp(unsolicitedPtr->unsolRsp,UNSOL_TRUE)==0) {
        IsUnsolTrueSet = true;
        return;
    }
}

// NOTE: Stub function
void atmachinemanager_RemoveUnsolicited(void *reportPtr)
{
    atUnsolicited_t* unsolicitedPtr = reportPtr;

    if ( strcmp(unsolicitedPtr->unsolRsp,UNSOL_FALSE)==0) {
        IsUnsolFalseSet = false;
        return;
    }
    if ( strcmp(unsolicitedPtr->unsolRsp,UNSOL_TRUE)==0) {
        IsUnsolTrueSet = false;
        return;
    }
}

// NOTE: Stub function
void atmachinemanager_SendCommand(void *reportPtr)
{
    AtCmdPtr = reportPtr;
}

// NOTE: Stub function
void atmachinemanager_CancelCommand(void *report)
{
    return;
}

static int my_open( const char* path)
{
    mkfifo(path,0666);
    return open(path, O_CREAT | O_RDWR | O_SYNC, S_IRUSR | S_IWUSR);
}

/* The suite initialization function.
 * Opens the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int init_suite(void)
{
    atmachinecommand_Init();
    atmachinemgritf_Init();
    atmachineunsolicited_Init();

    sprintf(Device.name,"device");
    sprintf(Device.path,DEVICE_PORT);

    Device.deviceItf.open         =(le_da_DeviceOpenFunc_t)my_open;
    Device.deviceItf.read         =(le_da_DeviceReadFunc_t)read;
    Device.deviceItf.write        =(le_da_DeviceWriteFunc_t)write;
    Device.deviceItf.io_control   =(le_da_DeviceIoControlFunc_t)fcntl;
    Device.deviceItf.close        =(le_da_DeviceCloseFunc_t)close;

    InterfacePtr = atmgr_CreateInterface(&Device);
    atports_SetInterface(ATPORT_COMMAND, InterfacePtr);

    return 0;
}

/* The suite cleanup function.
 * Closes the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int clean_suite(void)
{
    close(Device.handle);
    unlink(DEVICE_PORT);

    return 0;
}

// atmgr_t* atmgr_Create(void);
void testatmgr_Create()
{
    struct atmgr* newPtr = CreateInterface();

    CU_ASSERT_PTR_NOT_NULL(newPtr);
    CU_ASSERT_PTR_NOT_NULL(&(newPtr->atManager));
    CU_ASSERT_PTR_NOT_NULL(newPtr->resumeInterfaceId);
    CU_ASSERT_PTR_NOT_NULL(newPtr->suspendInterfaceId);
    CU_ASSERT_PTR_NOT_NULL(newPtr->sendCommandId);
    CU_ASSERT_PTR_NOT_NULL(newPtr->subscribeUnsolicitedId);
    CU_ASSERT_PTR_NOT_NULL(newPtr->unSubscribeUnsolicitedId);
    CU_ASSERT_PTR_NOT_NULL(newPtr->waitingSemaphore);

    CU_PASS("atmgr_Create");
}

// Test this function :
// atmgr_StartInterface(atmgr_Ref_t atmanageritfPtr);
void testatmgr_StartInterface()
{
    CU_ASSERT_EQUAL(IsStop,true);

    atmgr_StartInterface(InterfacePtr);

    sleep(1);

    CU_ASSERT_EQUAL(IsStop,false);

    CU_PASS("atmgr_StartInterface");
}

// Test this function :
// atmgr_StopInterface(atmgr_Ref_t atmanageritfPtr);
void testatmgr_StopInterface()
{
    CU_ASSERT_EQUAL(IsStop,false);

    atmgr_StopInterface(InterfacePtr);

    sleep(1);

    CU_ASSERT_EQUAL(IsStop,true);

    CU_PASS("atmgr_StartInterface");
}

// Test this function :
// void atmgr_SubscribeUnsolReq(atmgr_Ref_t atmanageritfPtr,le_event_Id_t unsolicitedReportId,char *unsolRspPtr,bool withExtraData);
void testatmgr_SubscribeUnsolReq()
{
    EventIdAddUnsolicitedFalse = le_event_CreateIdWithRefCounting("testIdfalse");
    EventIdAddUnsolicitedtrue = le_event_CreateIdWithRefCounting("testIdtrue");

    CU_ASSERT_PTR_NOT_NULL(InterfacePtr);

    IsUnsolFalseSet = false;
    atmgr_SubscribeUnsolReq(InterfacePtr,EventIdAddUnsolicitedFalse,UNSOL_FALSE,false);

    sleep(1);
    CU_ASSERT_EQUAL(IsUnsolFalseSet,true);

    IsUnsolTrueSet = false;
    atmgr_SubscribeUnsolReq(InterfacePtr,EventIdAddUnsolicitedFalse,UNSOL_TRUE,true);

    sleep(1);
    CU_ASSERT_EQUAL(IsUnsolTrueSet,true);

    CU_PASS("atmgr_SubscribeUnsolReq");
}

// Test this function :
// void atmgr_UnSubscribeUnsolReq(atmgr_Ref_t atmanageritfPtr,le_event_Id_t unsolicitedReportId,char * unsolRspPtr);
void testatmgr_UnSubscribeUnsolReq()
{
    CU_ASSERT_PTR_NOT_NULL(InterfacePtr);

    atmgr_UnSubscribeUnsolReq(InterfacePtr,EventIdAddUnsolicitedFalse,UNSOL_FALSE);

    sleep(1);
    CU_ASSERT_EQUAL(IsUnsolFalseSet,false);

    atmgr_UnSubscribeUnsolReq(InterfacePtr,EventIdAddUnsolicitedFalse,UNSOL_TRUE);

    sleep(1);
    CU_ASSERT_EQUAL(IsUnsolTrueSet,false);

    CU_PASS("atmgr_UnSubscribeUnsolReq");
}

// Test this function :
// void atmgr_SendCommandRequest(atmgr_Ref_t atmanageritfPtr,ATCommand_t *atcommandToSendPtr);
void testatmgr_SendCommandRequest()
{
    atcmd_Ref_t atCmdPtr = atcmd_Create();

    atmgr_SendCommandRequest(InterfacePtr,atCmdPtr);

    sleep(1);

    CU_ASSERT_PTR_EQUAL(atCmdPtr,AtCmdPtr);

    CU_PASS("atmgr_SendCommandRequest");
}


COMPONENT_INIT
{
    int result = EXIT_SUCCESS;

    // Init the test case / test suite data structures

    CU_TestInfo test[] =
    {
        { "Test atmgr_Create" , testatmgr_Create },
        { "Test atmgr_StartInterface" , testatmgr_StartInterface },
        { "Test atmgr_StopInterface" , testatmgr_StopInterface },
        { "Test atmgr_SubscribeUnsolReq" , testatmgr_SubscribeUnsolReq },
        { "Test atmgr_UnSubscribeUnsolReq" , testatmgr_UnSubscribeUnsolReq },
        { "Test atmgr_SendCommandRequest" , testatmgr_SendCommandRequest },
        CU_TEST_INFO_NULL,
    };

    CU_SuiteInfo suites[] =
    {
        { "atmanageritf tests",           init_suite, clean_suite, test },
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
