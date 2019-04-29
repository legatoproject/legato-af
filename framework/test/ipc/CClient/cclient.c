/**
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

#include <setjmp.h>
#include <string.h>

#ifndef CONFIG_LINUX
#define CONFIG_LINUX 0
#endif

#define TEST_CALLBACK_TIMEOUT 5000

/*
 * Timer to trigger timeout if expected event is not received.
 */
static le_timer_Ref_t TestTimeoutTimerRef;

/*
 * Tests -- test a number of types can be passed over IPC, as well as testing a selection of
 * values with NULL outputs.
 */

static void TestEchoSimple(void)
{
    const int32_t inValue = 42;
    int32_t outValue;
    ipcTest_EchoSimple(inValue, &outValue);
    LE_TEST_OK(inValue == outValue, "echo simple value");
}

static void TestEchoSimpleNull(void)
{
    const int32_t inValue = 42;
    ipcTest_EchoSimple(inValue, NULL);
    LE_TEST_OK(true, "echo to null destination");
}

static void TestEchoSmallEnum(void)
{
    const ipcTest_SmallEnum_t inValue = IPCTEST_SE_VALUE4;
    ipcTest_SmallEnum_t outValue = IPCTEST_SE_VALUE1;
    ipcTest_EchoSmallEnum(inValue, &outValue);
    LE_TEST_OK(inValue == outValue,
               "echo small enum (in: %"PRIu32", out; %"PRIu32")",
               (uint32_t)inValue, (uint32_t)outValue);
}

static void TestEchoLargeEnum(void)
{
    const ipcTest_LargeEnum_t inValue = IPCTEST_LE_LARGE_VALUE1;
    ipcTest_LargeEnum_t outValue = IPCTEST_LE_VALUE1;

    ipcTest_EchoLargeEnum(inValue, &outValue);
    LE_TEST_OK(inValue == outValue,
               "echo large enum (in: %"PRIu64", out: %"PRIu64")",
               (uint64_t)inValue, (uint64_t)outValue);
}

static void TestEchoSmallBitMask(void)
{
    const ipcTest_SmallBitMask_t inValue = IPCTEST_SBM_VALUE1 | IPCTEST_SBM_VALUE3;
    ipcTest_SmallBitMask_t outValue = 0;

    ipcTest_EchoSmallBitMask(inValue, &outValue);
    LE_TEST_OK(inValue == outValue,
               "echo small bitmask (in: %"PRIu32", out; %"PRIu32")",
               (uint32_t)inValue, (uint32_t)outValue);
}

static void TestEchoLargeBitMask(void)
{
    const ipcTest_LargeBitMask_t inValue = IPCTEST_LBM_VALUE64 | IPCTEST_LBM_VALUE9;
    ipcTest_LargeBitMask_t outValue = 0;

    ipcTest_EchoLargeBitMask(inValue, &outValue);
    LE_TEST_OK(inValue == outValue,
               "echo large bitmask (in: %"PRIu64", out; %"PRIu64")",
               (uint64_t)inValue, (uint64_t)outValue);
}

static void TestEchoReference(void)
{
    const ipcTest_SimpleRef_t inRef = (ipcTest_SimpleRef_t)0x10000051;
    ipcTest_SimpleRef_t outRef = NULL;

    ipcTest_EchoReference(inRef, &outRef);
    LE_TEST_OK(inRef == outRef,
               "echo simple reference (in: %p, out: %p)",
               inRef, outRef);
}

static void TestEchoErrorReference(void)
{
    const ipcTest_SimpleRef_t inRef = (ipcTest_SimpleRef_t)0;
    ipcTest_SimpleRef_t outRef = NULL;

    ipcTest_EchoReference(inRef, &outRef);
    LE_TEST_OK(inRef == outRef,
               "echo error reference (in: %p, out: %p)",
               inRef, outRef);
}

static void TestEchoReferenceNull(void)
{
    const ipcTest_SimpleRef_t inRef = (ipcTest_SimpleRef_t)0x10000051;

    ipcTest_EchoReference(inRef, NULL);
    LE_TEST_OK(true, "echo null reference");
}

static void TestEchoSmallString(void)
{
    static const char *inString = "Hello World";
    char outString[257];

    ipcTest_EchoString(inString, outString, sizeof(outString));

    LE_TEST_OK(strcmp(inString, outString) == 0,
               "echo small string (in: %s, out: %s)",
               inString, outString);
}

static void TestEchoMaxString(void)
{
    char inString[257];
    char outString[257];
    memset(inString, 'a', 256);
    inString[256] = '\0';

    ipcTest_EchoString(inString, outString, sizeof(outString));

    LE_TEST_OK(strcmp(inString, outString) == 0,
               "echo max string (in: len %"PRIuS", out: len %"PRIuS")",
               strlen(inString), strlen(outString));
}

static void TestEchoStringNull(void)
{
    static const char *inString = "Hello NULL World";

    ipcTest_EchoString(inString, NULL, 0);
    LE_TEST_OK(true, "echo null string");
}

#if 0
// Not supported by Java
static void TestEchoSmallArray(void)
{
    int64_t inArray = 42;
    int64_t outArray[32];
    size_t outArraySize = 32;

    ipcTest_EchoArray(&inArray, 1, outArray, &outArraySize);
    LE_TEST_OK(outArraySize == 1, "small array size");
    LE_TEST_OK(inArray == outArray[0], "small array element 0");
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
    LE_TEST_OK(outArraySize == 32, "exit max array correct size");

    for (i = 0; i < 32; ++i)
    {
        LE_TEST_OK(inArray[i] == outArray[i], "max array element %d", i);
    }
}
#endif

#if 0
// Not supported by Java
static void TestEchoArrayNull(void)
{
    int64_t inArray = 42;

    ipcTest_EchoArray(&inArray, 1, NULL, 0);
    LE_TEST_OK(true, "echo null array");
}
#endif

// Server exit handler.
static jmp_buf ServerExitJump;

__attribute__((unused)) static void TestServerExitHandler(void *contextPtr)
{
    LE_UNUSED(contextPtr);
    longjmp(ServerExitJump, 1);
}

static void TestServerExit(void)
{
#if CONFIG_LINUX
    if (0 == setjmp(ServerExitJump))
    {
        ipcTest_SetServerDisconnectHandler(TestServerExitHandler, NULL);

        ipcTest_ExitServer();
    }
    else
    {
        LE_TEST_OK(true, "server exit handler");

        // Reconnect to server so future tests can run
        ipcTest_ConnectService();
    }
#else
    LE_TEST_OK(false, "test not available");
#endif
}

static void CallbackTimeout
(
    le_timer_Ref_t timerRef                   ///< [IN] Timer pointer
)
{
    LE_UNUSED(timerRef);
    LE_TEST_OK(false, "echo event");
    LE_TEST_EXIT;
}

void EchoEventHandler
(
    int32_t value,
    void* context
)
{
    LE_UNUSED(context);
    LE_TEST_OK(value == 42, "echo event");

    le_timer_Stop(TestTimeoutTimerRef);

    LE_TEST_EXIT;
}

// Test IPC callbacks
static void TestCallback
(
    void
)
{
    TestTimeoutTimerRef = le_timer_Create("TestTimeout");

    le_timer_SetHandler(TestTimeoutTimerRef, CallbackTimeout);
    le_timer_SetMsInterval(TestTimeoutTimerRef, TEST_CALLBACK_TIMEOUT);
    le_timer_Start(TestTimeoutTimerRef);

    ipcTest_AddEchoEventHandler(EchoEventHandler, NULL);
    ipcTest_EchoTriggerEvent(42);
}

COMPONENT_INIT
{
    bool skipExitTest = false;

    le_arg_SetFlagVar(&skipExitTest, NULL, "skip-exit");
    le_arg_Scan();

    LE_TEST_PLAN(14);

    ipcTest_ConnectService();
    LE_TEST_INFO("Connected to server");

    TestEchoSimple();
    TestEchoSimpleNull();
    TestEchoSmallEnum();
    TestEchoLargeEnum();
    TestEchoSmallBitMask();
    TestEchoLargeBitMask();
    TestEchoReference();
    TestEchoErrorReference();
    TestEchoReferenceNull();
    TestEchoSmallString();
    TestEchoMaxString();
    TestEchoStringNull();
    // TestEchoSmallArray();
    // TestEchoMaxArray();
    // TestEchoArrayNull();
    LE_TEST_BEGIN_SKIP(!CONFIG_LINUX || skipExitTest, 1);
    TestServerExit();
    LE_TEST_END_SKIP();
    TestCallback();

    // No finish yet -- callback test still running
}
