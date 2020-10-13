/**
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

#include <setjmp.h>
#include <string.h>

#ifndef LE_CONFIG_LINUX
#define LE_CONFIG_LINUX 0
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
    int32_t inValue = 42;
    int32_t outValue;
    ipcTest_EchoSimple(inValue, &outValue);
    LE_TEST_OK(inValue == outValue,
               "echo simple positive value: want %"PRId32", got %"PRId32"",
               inValue, outValue);

    inValue = -50;
    ipcTest_EchoSimple(inValue, &outValue);
    LE_TEST_OK(inValue == outValue,
               "echo simple negative value: want %"PRId32", got %"PRId32"",
               inValue, outValue);

    inValue = -50000000;
    ipcTest_EchoSimple(inValue, &outValue);
    LE_TEST_OK(inValue == outValue,
               "echo simple large negative value: want %"PRId32", got %"PRId32"",
               inValue, outValue);

    inValue = 5000000;
    ipcTest_EchoSimple(inValue, &outValue);
    LE_TEST_OK(inValue == outValue,
               "echo simple large positive value: want %"PRId32", got %"PRId32"",
               inValue, outValue);

    inValue = -5;
    ipcTest_EchoSimple(inValue, &outValue);
    LE_TEST_OK(inValue == outValue,
               "echo tiny negative value: want %"PRId32", got %"PRId32"",
               inValue, outValue);

    inValue = 5;
    ipcTest_EchoSimple(inValue, &outValue);
    LE_TEST_OK(inValue == outValue,
               "echo tiny positive value: want %"PRId32", got %"PRId32"",
               inValue, outValue);
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

static void TestEchoBoolean(void)
{
    const bool inValue = false;
    bool outValue = true;
    ipcTest_EchoBoolean(inValue, &outValue);
    LE_TEST_OK(inValue == outValue, "simple boolean test");
}

static void TestEchoResult(void)
{
    const le_result_t inValue = LE_IO_ERROR;
    le_result_t outValue = LE_OK;
    ipcTest_EchoResult(inValue, &outValue);
    LE_TEST_OK(inValue == outValue, "simple result test");
}

static void TestReturnedResult(void)
{
    const le_result_t inValue = LE_IO_ERROR;
    LE_TEST_OK(ipcTest_ReturnResult(inValue) == inValue, "simple return test");
}

static void TestEchoOnOff(void)
{
    const le_onoff_t inValue = LE_OFF;
    le_onoff_t outValue = LE_ON;
    ipcTest_EchoOnOff(inValue, &outValue);
    LE_TEST_OK(inValue == outValue, "simple onoff test");
}

static void TestEchoDouble(void)
{
    double inValue = 3.1415161718;
    double outValue = 0;
    ipcTest_EchoDouble(inValue, &outValue);
    LE_TEST_OK(inValue == outValue, "echo double value: want %f, got %f", inValue, outValue);

    inValue = NAN;
    ipcTest_EchoDouble(inValue, &outValue);
    LE_TEST_OK(isnan(outValue) != 0, "echo double NAN");

    inValue = INFINITY;
    ipcTest_EchoDouble(inValue, &outValue);
    LE_TEST_OK(isinf(outValue) == 1, "echo double pos INF");

    inValue = -INFINITY;
    ipcTest_EchoDouble(inValue, &outValue);
    LE_TEST_OK(isinf(outValue) == -1, "echo double neg INF");
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
               "echo small string (in: \"%s\", out: \"%s\")",
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

static void TestEchoEmptyString(void)
{
    static const char* inString = "";
    char outString[257] = { 0xed };

    ipcTest_EchoString(inString, outString, sizeof(outString));
    LE_TEST_OK(strnlen(outString, sizeof(outString)) == 0, "echo empty string");
}

static void TestEchoSmallArray(void)
{
    int64_t inArray = 42;
    int64_t outArray[32];
    size_t outArraySize = 32;

    ipcTest_EchoArray(&inArray, 1, outArray, &outArraySize);
    LE_TEST_OK(outArraySize == 1, "small array size is %"PRIuS"", outArraySize);
    LE_TEST_OK(inArray == outArray[0], "small array element 0: %"PRIu64"", outArray[0]);
}

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
    LE_TEST_OK(outArraySize == 32, "exit max array correct size: %"PRIuS"", outArraySize);

    for (i = 0; i < 32; ++i)
    {
        LE_TEST_OK(inArray[i] == outArray[i], "max array element %d", i);
    }
}

static void TestEchoArrayNull(void)
{
    int64_t inArray = 42;

    ipcTest_EchoArray(&inArray, 1, NULL, 0);
    LE_TEST_OK(true, "echo null array");
}

static void TestEchoSmallByteString(void)
{
    uint8_t inArray = 42;
    uint8_t outArray[32];
    size_t outArraySize = 32;

    ipcTest_EchoByteString(&inArray, 1, outArray, &outArraySize);
    LE_TEST_OK(outArraySize == 1, "small byte string size %"PRIuS"", outArraySize);
    LE_TEST_OK(inArray == outArray[0], "small byte string element 0: %"PRIu8"", outArray[0]);
}

static void TestEchoStruct(void)
{
    ipcTest_TheStruct_t inStruct = {.name = "echo", .index = -7};
    ipcTest_TheStruct_t outStruct;
    ipcTest_EchoStruct(&inStruct, &outStruct);
    LE_TEST_OK(outStruct.index == -7 , "simple struct member");
    LE_TEST_OK(strcmp(outStruct.name, "echo") == 0, "string struct member");
}

static void TestEchoStructArray(void)
{
    ipcTest_TheStruct_t inStructArray[2] = {{"echo1", -7} , {"echo2", 7}};
    ipcTest_TheStruct_t outStructArray[2];
    size_t outStructArraySize = NUM_ARRAY_MEMBERS(outStructArray);

    ipcTest_EchoStructArray(inStructArray, NUM_ARRAY_MEMBERS(inStructArray), outStructArray,
                            &outStructArraySize);

    LE_TEST_OK(outStructArraySize == NUM_ARRAY_MEMBERS(inStructArray), "struct array size");
    for(int i = 0; i < NUM_ARRAY_MEMBERS(inStructArray); i++)
    {
        LE_TEST_OK(outStructArray[i].index == inStructArray[i].index, "simple member struct array");
        LE_TEST_OK(strcmp(outStructArray[i].name , inStructArray[i].name) == 0,
                   "string member struct array");
    }
}

// Server exit handler.
static jmp_buf ServerExitJump;

__attribute__((unused)) static void TestServerExitHandler(void *contextPtr)
{
    LE_UNUSED(contextPtr);
    longjmp(ServerExitJump, 1);
}

static void TestServerExit(void)
{
#if LE_CONFIG_LINUX
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


static ipcTest_EchoComplexEventHandlerRef_t complexHandler;
static ipcTest_EchoEventHandlerRef_t handler;

void EchoComplexEventHandler
(
    int32_t value,
    const char* LE_NONNULL cookieString,
    const int16_t* cookieArrayPtr,
    size_t cookieArraySize,
    void* context
)
{
    LE_TEST_OK(value == 40, "echo event");
    LE_UNUSED(context);

    LE_TEST_OK(strcmp(cookieString, "Triggered") == 0, "echo event string");
    for (int i = 0 ; i < cookieArraySize; i++)
    {
        LE_TEST_OK(cookieArrayPtr[i] == 45, "echo event array item %d",i);
    }

    le_timer_Stop(TestTimeoutTimerRef);

    ipcTest_RemoveEchoComplexEventHandler(complexHandler);

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

    ipcTest_RemoveEchoEventHandler(handler);

    complexHandler = ipcTest_AddEchoComplexEventHandler(EchoComplexEventHandler, NULL);

    le_timer_Start(TestTimeoutTimerRef);
    char name[] = "Triggered";
    int16_t a[]={45,45,45,45};
    ipcTest_EchoTriggerComplexEvent(40, name, a, NUM_ARRAY_MEMBERS(a));
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

    handler = ipcTest_AddEchoEventHandler(EchoEventHandler, NULL);
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
    TestEchoBoolean();
    TestEchoResult();
    TestReturnedResult();
    TestEchoOnOff();
    TestEchoDouble();
    TestEchoReference();
    TestEchoErrorReference();
    TestEchoReferenceNull();
    TestEchoSmallString();
    TestEchoMaxString();
    TestEchoSmallByteString();
    TestEchoStringNull();
    TestEchoEmptyString();
    TestEchoSmallArray();
    TestEchoMaxArray();
    TestEchoArrayNull();
    TestEchoStruct();
    TestEchoStructArray();
    LE_TEST_BEGIN_SKIP(!LE_CONFIG_LINUX || skipExitTest, 1);
    TestServerExit();
    LE_TEST_END_SKIP();
    TestCallback();

    // No finish yet -- callback test still running
}
