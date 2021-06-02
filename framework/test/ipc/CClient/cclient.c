/**
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

#include "utils.h"

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

static void TestSimple(void)
{
    int32_t inValue = 42;
    int32_t outValue;
    ipcTest_AddOneSimple(inValue, &outValue);
    LE_TEST_OK(inValue + 1 == outValue,
               "add one to simple positive value: in %"PRId32", out %"PRId32"",
               inValue, outValue);

    inValue = -50;
    ipcTest_AddOneSimple(inValue, &outValue);
    LE_TEST_OK(inValue + 1 == outValue,
               "add one to simple negative value: in %"PRId32", out %"PRId32"",
               inValue, outValue);

    inValue = -50000000;
    ipcTest_AddOneSimple(inValue, &outValue);
    LE_TEST_OK(inValue + 1 == outValue,
               "add one to simple large negative value: in %"PRId32", out %"PRId32"",
               inValue, outValue);

    inValue = 5000000;
    ipcTest_AddOneSimple(inValue, &outValue);
    LE_TEST_OK(inValue + 1 == outValue,
               "add one to simple large positive value: in %"PRId32", out %"PRId32"",
               inValue, outValue);

    inValue = -5;
    ipcTest_AddOneSimple(inValue, &outValue);
    LE_TEST_OK(inValue + 1 == outValue,
               "add one to tiny negative value: in %"PRId32", out %"PRId32"",
               inValue, outValue);

    inValue = 5;
    ipcTest_AddOneSimple(inValue, &outValue);
    LE_TEST_OK(inValue + 1 == outValue,
               "add one to tiny positive value: in %"PRId32", out %"PRId32"",
               inValue, outValue);
}

static void TestSimpleNull(void)
{
    const int32_t inValue = 42;
    ipcTest_AddOneSimple(inValue, NULL);
    LE_TEST_OK(true, "add one with null destination");
}

static void TestSmallEnum(void)
{
    const ipcTest_SmallEnum_t inValue = IPCTEST_SE_VALUE4;
    ipcTest_SmallEnum_t outValue = IPCTEST_SE_VALUE1;
    ipcTest_AddOneSmallEnum(inValue, &outValue);
    LE_TEST_OK(util_IncSmallEnum(inValue) == outValue,
               "increment small enum (in: %"PRIu32", out: %"PRIu32")",
               (uint32_t)inValue, (uint32_t)outValue);
}

static void TestLargeEnum(void)
{
    const ipcTest_LargeEnum_t inValue = IPCTEST_LE_LARGE_VALUE1;
    ipcTest_LargeEnum_t outValue = IPCTEST_LE_VALUE1;

    ipcTest_AddOneLargeEnum(inValue, &outValue);
    LE_TEST_OK(util_IncLargeEnum(inValue) == outValue,
               "increment large enum (in: %"PRIu64", out: %"PRIu64")",
               (uint64_t)inValue, (uint64_t)outValue);
}

static void TestSmallBitMask(void)
{
    const ipcTest_SmallBitMask_t inValue = IPCTEST_SBM_VALUE1 | IPCTEST_SBM_VALUE3;
    ipcTest_SmallBitMask_t outValue = 0;

    ipcTest_NotSmallBitMask(inValue, &outValue);
    LE_TEST_OK(~inValue == outValue,
               "not small bitmask (in: %"PRIu32", out; %"PRIu32")",
               (uint32_t)inValue, (uint32_t)outValue);
}

static void TestLargeBitMask(void)
{
    const ipcTest_LargeBitMask_t inValue = IPCTEST_LBM_VALUE64 | IPCTEST_LBM_VALUE9;
    ipcTest_LargeBitMask_t outValue = 0;

    ipcTest_NotLargeBitMask(inValue, &outValue);
    LE_TEST_OK(~inValue == outValue,
               "not large bitmask (in: %"PRIu64", out; %"PRIu64")",
               (uint64_t)inValue, (uint64_t)outValue);
}

static void TestBoolean(void)
{
    const bool inValue = false;
    bool outValue = false;
    ipcTest_NotBoolean(inValue, &outValue);
    LE_TEST_OK(!inValue == outValue, "simple boolean test");
}

static void TestResult(void)
{
    const le_result_t inValue = LE_IO_ERROR;
    le_result_t outValue = LE_OK;
    ipcTest_NextResult(inValue, &outValue);
    LE_TEST_OK(util_IncResult(inValue) == outValue, "simple result test");
}

static void TestReturnedResult(void)
{
    const le_result_t inValue = LE_IO_ERROR;
    LE_TEST_OK(ipcTest_ReturnNextResult(inValue) == util_IncResult(inValue), "simple return test");
}

static void TestOnOff(void)
{
    const le_onoff_t inValue = LE_OFF;
    le_onoff_t outValue = LE_OFF;
    ipcTest_NotOnOff(inValue, &outValue);
    LE_TEST_OK(util_NotOnOff(inValue) == outValue, "simple onoff test");
}

static void TestDouble(void)
{
    double inValue = 3.1415161718;
    double outValue = 0;
    ipcTest_AddOneDouble(inValue, &outValue);
    LE_TEST_OK(util_IsDoubleEqual(inValue + 1, outValue),
               "increment double value: in %f, out %f", inValue, outValue);

    inValue = NAN;
    ipcTest_AddOneDouble(inValue, &outValue);
    LE_TEST_OK(isnan(outValue) != 0, "add one double NAN (out: %g)", outValue);

    inValue = INFINITY;
    ipcTest_AddOneDouble(inValue, &outValue);
    LE_TEST_OK(isinf(outValue) && outValue > 0, "add one double pos INF (out: %g)", outValue);

    inValue = -INFINITY;
    ipcTest_AddOneDouble(inValue, &outValue);
    LE_TEST_OK(isinf(outValue) && outValue < 0, "add one double neg INF (out: %g)", outValue);
}

static void TestReference(void)
{
    const static ipcTest_SimpleRef_t inRef = (ipcTest_SimpleRef_t)0x10000051;
    const static ipcTest_SimpleRef_t expectedOutRef = (ipcTest_SimpleRef_t)0x10000055;
    ipcTest_SimpleRef_t outRef = NULL;

    ipcTest_AddFourReference(inRef, &outRef);
    LE_TEST_OK(expectedOutRef == outRef,
               "add four simple reference (in: %p, out: %p)",
               inRef, outRef);
}

static void TestErrorReference(void)
{
    const ipcTest_SimpleRef_t inRef = (ipcTest_SimpleRef_t)0;
    ipcTest_SimpleRef_t outRef = NULL;

    ipcTest_AddFourReference(inRef, &outRef);
    LE_TEST_OK(inRef == outRef,
               "echo error reference (in: %p, out: %p)",
               inRef, outRef);
}

static void TestReferenceNull(void)
{
    const ipcTest_SimpleRef_t inRef = (ipcTest_SimpleRef_t)0x10000051;

    ipcTest_AddFourReference(inRef, NULL);
    LE_TEST_OK(true, "null reference");
}

static void TestSmallString(void)
{
    static const char *inString = "Hello World";
    char expectedOutString[257];
    char outString[257];

    util_ROT13String(inString, expectedOutString, sizeof(expectedOutString));

    ipcTest_ROT13String(inString, outString, sizeof(outString));

    LE_TEST_OK(strcmp(expectedOutString, outString) == 0,
               "rot13 small string (in: \"%s\", out: \"%s\")",
               inString, outString);
}

static void TestMaxString(void)
{
    char inString[257];
    char outString[257];
    char expectedOutString[257];
    memset(inString, 'a', 256);
    memset(outString, 0xde, sizeof(outString));
    inString[256] = '\0';
    util_ROT13String(inString, expectedOutString, sizeof(expectedOutString));

    ipcTest_ROT13String(inString, outString, sizeof(outString));

    LE_TEST_OK(strcmp(expectedOutString, outString) == 0,
               "rot13 max string (in: len %"PRIuS", out: len %"PRIuS")",
               strlen(inString), strlen(outString));
}

static void TestStringBound(void)
{
    char inString[99];
    char outString[100];
    int sizeIn = sizeof(inString);
    int sizeOut = sizeof(outString);
    memset(inString, 0xef, sizeIn);
    inString[sizeIn - 1] = '\0';

    memset(outString, 0xde, sizeof(outString));
    ipcTest_ROT13String(inString, outString, sizeOut - 1);

    LE_TEST_OK(outString[sizeOut - 1] == 0xde,
               "last byte shall be keep intact");
    LE_TEST_OK(strlen(outString) + 2 == sizeof(outString),
               "length of string: %"PRIuS", size of buffer: %"PRIuS,
               strlen(outString), sizeof(outString));
}

static void TestStringNull(void)
{
    static const char *inString = "Hello NULL World";

    ipcTest_ROT13String(inString, NULL, 0);
    LE_TEST_OK(true, "rot13 null string");
}

static void TestEmptyString(void)
{
    static const char* inString = "";
    char outString[257];
    memset(outString, 0xde, sizeof(outString));

    ipcTest_ROT13String(inString, outString, sizeof(outString));
    LE_TEST_OK(strnlen(outString, sizeof(outString)) == 0, "rot13 empty string");
}

static void TestSmallArray(void)
{
    int64_t inArray = 42;
    int64_t outArray[32];
    size_t outArraySize = 32;

    ipcTest_AddOneArray(&inArray, 1, outArray, &outArraySize);
    LE_TEST_OK(outArraySize == 1, "small array size is %"PRIuS"", outArraySize);
    LE_TEST_OK(inArray + 1 == outArray[0], "small array element 0: %"PRIu64"", outArray[0]);
}

static void TestMaxArray(void)
{
    int64_t inArray[32];
    int64_t outArray[32];
    size_t outArraySize = 32;
    int i;

    for (i = 0; i < 32; ++i)
    {
        inArray[i] = 0x8000000000000000ull >> i;
    }

    ipcTest_AddOneArray(inArray, 32, outArray, &outArraySize);
    LE_TEST_OK(outArraySize == 32, "exit max array correct size: %"PRIuS"", outArraySize);

    for (i = 0; i < 32; ++i)
    {
        LE_TEST_OK(inArray[i] + 1 == outArray[i], "max array element %d", i);
    }
}

static void TestArrayNull(void)
{
    int64_t inArray = 42;

    ipcTest_AddOneArray(&inArray, 1, NULL, 0);
    LE_TEST_OK(true, "echo null array");
}

static void TestSmallByteString(void)
{
    uint8_t inArray = 42;
    uint8_t expectedOut = ~inArray;
    uint8_t outArray[32];
    size_t outArraySize = 32;

    ipcTest_NotByteString(&inArray, 1, outArray, &outArraySize);
    LE_TEST_OK(outArraySize == 1, "small byte string size %"PRIuS"", outArraySize);
    LE_TEST_OK(expectedOut == outArray[0],
               "small byte string element 0: %"PRIu8"", outArray[0]);
}

/**
 * Test echoing a short CBOR string in a byte array.
 *
 * This string includes embedded NULLs in both forward and reverse direction to test for
 * using string functions on byte strings.
 *
 * Test for regression of LE-15906
 */
static void TestCborByteString(void)
{
    static const uint8_t inArray[32] =
        { 0x1B, 0x00, 0x00, 0x01, 0x77, 0x86, 0x93, 0xCA,
          0x72, 0x7F, 'T',  'h',  'i',  's',  ' ',  'i',
          's',  ' ',  'a',  ' ',  't',  'e',  's',  't',
          0xFF, 0x73, 0x74 };
    static const size_t inSize = 26;
    uint8_t outArray[32];
    size_t outSize = 32;
    size_t i;

    ipcTest_NotByteString(inArray, inSize, outArray, &outSize);

    LE_TEST_OK(inSize == outSize,
               "CBOR size is correct (expected:%"PRIuS" actual:%"PRIuS")", inSize, outSize);
    for (i = 0; i < inSize; ++i)
    {
        uint8_t expectedOut = (uint8_t)(~(inArray[i]));
        LE_TEST_OK(expectedOut == outArray[i],
                   "CBOR array element %"PRIuS " matches (in: %02x, out: %02x)",
                   i, inArray[i], outArray[i]);
    }
}

static void TestStruct(void)
{
    static const ipcTest_TheStruct_t inStruct = {.name = "echo", .index = -7};
    ipcTest_TheStruct_t outStruct, expectedOutStruct;

    util_ROT13String(inStruct.name, expectedOutStruct.name, sizeof(expectedOutStruct.name));
    expectedOutStruct.index = inStruct.index + 1;

    ipcTest_AddOneROT13Struct(&inStruct, &outStruct);
    LE_TEST_OK(outStruct.index == expectedOutStruct.index , "simple struct member");
    LE_TEST_OK(strcmp(outStruct.name, expectedOutStruct.name) == 0, "string struct member");
}

static void TestStructArray(void)
{
    static const ipcTest_TheStruct_t inStructArray[2] = {{"echo1", -7} , {"echo2", 7}};
    ipcTest_TheStruct_t outStructArray[2], expectedOutStructArray[2];
    size_t outStructArraySize = NUM_ARRAY_MEMBERS(outStructArray);
    size_t i;

    for (i = 0; i < NUM_ARRAY_MEMBERS(inStructArray); ++i)
    {
        util_ROT13String(inStructArray[i].name,
                         expectedOutStructArray[i].name,
                         sizeof(expectedOutStructArray[i].name));
        expectedOutStructArray[i].index = inStructArray[i].index + 1;
    }

    ipcTest_AddOneROT13StructArray(inStructArray, NUM_ARRAY_MEMBERS(inStructArray),
                                   outStructArray, &outStructArraySize);

    LE_TEST_OK(outStructArraySize == NUM_ARRAY_MEMBERS(inStructArray), "struct array size");
    for(size_t i = 0; i < NUM_ARRAY_MEMBERS(inStructArray); i++)
    {
        LE_TEST_OK(outStructArray[i].index == expectedOutStructArray[i].index,
                   "simple member struct array");
        LE_TEST_OK(strcmp(outStructArray[i].name , expectedOutStructArray[i].name) == 0,
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


static ipcTest_AddOneROT13EventHandlerRef_t complexHandler;
static ipcTest_AddOneEventHandlerRef_t handler;

static const int32_t InEventValue = 42;
static const char *InEventString = "Triggered";
static const int16_t InEventArray[] = {45,-45,0,-1};

void ComplexEventHandler
(
    int32_t value,
    const char* LE_NONNULL cookieString,
    const int16_t* cookieArrayPtr,
    size_t cookieArraySize,
    void* context
)
{
    LE_UNUSED(context);

    char expectedEventString[16];

    util_ROT13String(InEventString, expectedEventString, sizeof(expectedEventString));

    LE_TEST_OK(value == InEventValue + 1, "complex event value");
    LE_TEST_OK(strcmp(cookieString, expectedEventString) == 0, "complex event string");
    LE_TEST_OK(cookieArraySize == NUM_ARRAY_MEMBERS(InEventArray), "complex cookie array size");
    for (size_t i = 0 ; i < cookieArraySize; i++)
    {
        LE_TEST_OK(cookieArrayPtr[i] == InEventArray[i] + 1,
                   "complex event array item %"PRIuS" (in: %"PRIi16" out: %"PRIi16")",
                   i, InEventArray[i], cookieArrayPtr[i]);
    }

    le_timer_Stop(TestTimeoutTimerRef);

    ipcTest_RemoveAddOneROT13EventHandler(complexHandler);

    LE_TEST_EXIT;
}

void EchoEventHandler
(
    int32_t value,
    void* context
)
{
    LE_UNUSED(context);

    LE_TEST_OK(value == InEventValue + 1, "event value");
    le_timer_Stop(TestTimeoutTimerRef);

    ipcTest_RemoveAddOneEventHandler(handler);

    complexHandler = ipcTest_AddAddOneROT13EventHandler(ComplexEventHandler, NULL);

    le_timer_Start(TestTimeoutTimerRef);

    ipcTest_TriggerAddOneROT13Event(InEventValue, InEventString,
                                    InEventArray, NUM_ARRAY_MEMBERS(InEventArray));
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

    handler = ipcTest_AddAddOneEventHandler(EchoEventHandler, NULL);
    ipcTest_TriggerAddOneEvent(InEventValue);
}

COMPONENT_INIT
{
    bool skipExitTest = false;

    le_arg_SetFlagVar(&skipExitTest, NULL, "skip-exit");
    le_arg_Scan();

    LE_TEST_PLAN(LE_TEST_NO_PLAN);
    ipcTest_ConnectService();
    LE_TEST_INFO("Connected to server");

    TestSimple();
    TestSimpleNull();
    TestSmallEnum();
    TestLargeEnum();
    TestSmallBitMask();
    TestLargeBitMask();
    TestBoolean();
    TestResult();
    TestReturnedResult();
    TestOnOff();
    TestDouble();
    TestReference();
    TestErrorReference();
    TestReferenceNull();
    TestSmallString();
    TestMaxString();
    TestStringBound();
    TestSmallByteString();
    TestCborByteString();
    TestStringNull();
    TestEmptyString();
    TestSmallArray();
    TestMaxArray();
    TestArrayNull();
    TestStruct();
    TestStructArray();
    LE_TEST_BEGIN_SKIP(!LE_CONFIG_LINUX || skipExitTest, 1);
    TestServerExit();
    LE_TEST_END_SKIP();
    TestCallback();

    // No finish yet -- callback test still running
}
