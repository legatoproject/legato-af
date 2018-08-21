/**
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

#include <setjmp.h>
#include <string.h>

#include <CUnit/Console.h>
#include <CUnit/Basic.h>

/*
 * Tests -- test a number of types can be passed over IPC, as well as testing a selection of
 * values with NULL outputs.
 */

static void TestEchoSimple(void)
{
    const int32_t inValue = 42;
    int32_t outValue;
    ipcTest_EchoSimple(inValue, &outValue);
    CU_ASSERT(inValue == outValue);
}

static void TestEchoSimpleNull(void)
{
    const int32_t inValue = 42;
    ipcTest_EchoSimple(inValue, NULL);
    CU_PASS("No crash");
}

static void TestEchoSmallEnum(void)
{
    const ipcTest_SmallEnum_t inValue = IPCTEST_SE_VALUE4;
    ipcTest_SmallEnum_t outValue = IPCTEST_SE_VALUE1;
    ipcTest_EchoSmallEnum(inValue, &outValue);
    CU_ASSERT(inValue == outValue);
}

static void TestEchoLargeEnum(void)
{
    const ipcTest_LargeEnum_t inValue = IPCTEST_LE_LARGE_VALUE1;
    ipcTest_LargeEnum_t outValue = IPCTEST_LE_VALUE1;

    ipcTest_EchoLargeEnum(inValue, &outValue);
    CU_ASSERT(inValue == outValue);
}

static void TestEchoSmallBitMask(void)
{
    const ipcTest_SmallBitMask_t inValue = IPCTEST_SBM_VALUE1 | IPCTEST_SBM_VALUE3;
    ipcTest_SmallBitMask_t outValue = 0;

    ipcTest_EchoSmallBitMask(inValue, &outValue);
    CU_ASSERT(inValue == outValue);
}

static void TestEchoLargeBitMask(void)
{
    const ipcTest_LargeBitMask_t inValue = IPCTEST_LBM_VALUE64 | IPCTEST_LBM_VALUE9;
    ipcTest_LargeBitMask_t outValue = 0;

    ipcTest_EchoLargeBitMask(inValue, &outValue);
    CU_ASSERT(inValue == outValue);
}

static void TestEchoReference(void)
{
    const ipcTest_SimpleRef_t inRef = (ipcTest_SimpleRef_t)0x10000051;
    ipcTest_SimpleRef_t outRef = NULL;

    ipcTest_EchoReference(inRef, &outRef);
    CU_ASSERT(inRef == outRef);
}

static void TestEchoErrorReference(void)
{
    const ipcTest_SimpleRef_t inRef = (ipcTest_SimpleRef_t)0;
    ipcTest_SimpleRef_t outRef = NULL;

    ipcTest_EchoReference(inRef, &outRef);
    CU_ASSERT(inRef == outRef);
}

static void TestEchoReferenceNull(void)
{
    const ipcTest_SimpleRef_t inRef = (ipcTest_SimpleRef_t)0x10000051;

    ipcTest_EchoReference(inRef, NULL);
    CU_PASS("No crash");
}

static void TestEchoSmallString(void)
{
    static const char *inString = "Hello World";
    char outString[257];

    ipcTest_EchoString(inString, outString, sizeof(outString));

    CU_ASSERT(strcmp(inString, outString) == 0);
}

static void TestEchoMaxString(void)
{
    char inString[257];
    char outString[257];
    memset(inString, 'a', 256);
    inString[256] = '\0';

    ipcTest_EchoString(inString, outString, sizeof(outString));

    CU_ASSERT(strcmp(inString, outString) == 0);
}

static void TestEchoStringNull(void)
{
    static const char *inString = "Hello NULL World";

    ipcTest_EchoString(inString, NULL, 0);
    CU_PASS("No crash");
}

static void TestEchoSimpleStruct(void)
{
    static const ipcTest_SimpleStruct_t simpleStructIn =
        {
            .Simple = 5566,
            .Enum = IPCTEST_SE_VALUE3,
            .BitMask = IPCTEST_SBM_VALUE1,
            .Ref = NULL,
            .String = "a test string for testing"
        };
    ipcTest_SimpleStruct_t simpleStructOut = {};

    ipcTest_EchoSimpleStruct(&simpleStructIn, &simpleStructOut);

    CU_ASSERT(memcmp(&simpleStructIn, &simpleStructOut, sizeof(ipcTest_SimpleStruct_t)) == 0);
}

static void TestEchoCompoundStruct(void)
{
    static const ipcTest_CompoundStruct_t compoundStructIn =
        {
            .Struct = {
                .Simple = 5566,
                .Enum = IPCTEST_SE_VALUE3,
                .BitMask = IPCTEST_SBM_VALUE1,
                .Ref = NULL,
                .String = "a test string for testing"
            }
        };
    ipcTest_CompoundStruct_t compoundStructOut = {};

    ipcTest_EchoCompoundStruct(&compoundStructIn, &compoundStructOut);

    CU_PASS("No crash");
}

static void TestEchoSimpleStructNull(void)
{
    static const ipcTest_SimpleStruct_t simpleStructIn =
        {
            .Simple = 5566,
            .Enum = IPCTEST_SE_VALUE3,
            .BitMask = IPCTEST_SBM_VALUE1,
            .Ref = NULL,
            .String = "a test string for testing"
        };

    ipcTest_EchoSimpleStruct(&simpleStructIn, NULL);

    CU_PASS("No crash");
}

static void TestEchoCompoundStructNull(void)
{
    static const ipcTest_CompoundStruct_t compoundStructIn =
        {
            .Struct = {
                .Simple = 5566,
                .Enum = IPCTEST_SE_VALUE3,
                .BitMask = IPCTEST_SBM_VALUE1,
                .Ref = NULL,
                .String = "a test string for testing"
            }
        };

    ipcTest_EchoCompoundStruct(&compoundStructIn, NULL);

    CU_PASS("No crash");
}

#if 0
// Not supported by Java
static void TestEchoSmallArray(void)
{
    int64_t inArray = 42;
    int64_t outArray[32];
    size_t outArraySize = 32;

    ipcTest_EchoArray(&inArray, 1, outArray, &outArraySize);
    CU_ASSERT(outArraySize == 1);
    CU_ASSERT(inArray == outArray[0]);
}
#endif

#if 0
// Not supported by Java
static void TestEchoMaxArray(void)
{
    int64_t inArray[32];
    int64_t outArray[32];
    size_t outArraySize = 32;
    int i;

    for (i = 0; i < 32; ++i)
    {
        inArray[i] = 0x8000000000000000ull >> i;
    }

    ipcTest_EchoArray(inArray, 32, outArray, &outArraySize);
    CU_ASSERT(outArraySize == 32);

    for (i = 0; i < 32; ++i)
    {
        CU_ASSERT(inArray[i] == outArray[i]);
    }
}
#endif

#if 0
// Not supported by Java
static void TestEchoArrayNull(void)
{
    int64_t inArray = 42;

    ipcTest_EchoArray(&inArray, 1, NULL, 0);
    CU_PASS("No crash");
}
#endif

// Server exit handler.
static jmp_buf ServerExitJump;

static void TestServerExitHandler(void *contextPtr)
{
    longjmp(ServerExitJump, 1);
}

static void TestServerExit(void)
{
    if (0 == setjmp(ServerExitJump))
    {
        ipcTest_SetServerDisconnectHandler(TestServerExitHandler, NULL);

        ipcTest_ExitServer();
    }
    else
    {
        CU_PASS("No client crash");

        // Reconnect to server so future tests can run
        ipcTest_ConnectService();
    }
}

static void* run_test(void* context)
{
    ipcTest_ConnectService();

    // Initialize the CUnit test registry and register the test suite
    if (CUE_SUCCESS != CU_initialize_registry())
    {
        exit(CU_get_error());
    }

    CU_TestInfo tests[] =
        {
              { "EchoSimple", TestEchoSimple },
              { "EchoSimple with NULL output", TestEchoSimpleNull },
              { "EchoSmallEnum", TestEchoSmallEnum },
              { "EchoLargeEnum", TestEchoLargeEnum },
              { "EchoSmallBitMask", TestEchoSmallBitMask },
              { "EchoLargeBitMask", TestEchoLargeBitMask },
              { "EchoReference", TestEchoReference },
              { "EchoReference with NULL reference", TestEchoErrorReference },
              { "EchoReference with NULL output", TestEchoReferenceNull },
              { "EchoString", TestEchoSmallString },
              { "EchoString with max size string", TestEchoMaxString },
              { "EchoString with NULL output", TestEchoStringNull },
              { "EchoSimpleStruct", TestEchoSimpleStruct },
              { "EchoSimpleStruct with NULL output", TestEchoSimpleStructNull },
              { "EchoCompoundStruct", TestEchoCompoundStruct },
              { "EchoCompoundStruct with NULL output", TestEchoCompoundStructNull },
//              { "EchoArray", TestEchoSmallArray },
//              { "EchoArray with max size array", TestEchoMaxArray },
//              { "EchoArray with NULL output", TestEchoArrayNull },
              { "Server exit", TestServerExit},
              CU_TEST_INFO_NULL
        };


    CU_SuiteInfo suites[] =
    {
        { "IPC tests", NULL, NULL, tests },
        CU_SUITE_INFO_NULL
    };

    if (CUE_SUCCESS != CU_register_suites(suites))
    {
        CU_cleanup_registry();
        exit(CU_get_error());
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    if ( CU_get_number_of_failures() > 0 )
    {
        fprintf(stdout,"\n [START]List of Failure :\n");
        CU_basic_show_failures(CU_get_failure_list());
        fprintf(stdout,"\n [STOP]List of Failure\n");
    }

    le_event_RunLoop();

    return NULL;
}

COMPONENT_INIT
{
    le_thread_Start(le_thread_Create("ipcTest",run_test,NULL));
}
