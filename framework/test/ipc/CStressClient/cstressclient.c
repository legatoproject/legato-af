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
#define TEST_ITERATIONS 500

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
    LE_TEST_OK(inValue == outValue, "echo small enum");
}

static void TestEchoLargeEnum(void)
{
    const ipcTest_LargeEnum_t inValue = IPCTEST_LE_LARGE_VALUE1;
    ipcTest_LargeEnum_t outValue = IPCTEST_LE_VALUE1;

    ipcTest_EchoLargeEnum(inValue, &outValue);
    LE_TEST_OK(inValue == outValue, "echo large enum");
}

static void TestEchoSmallBitMask(void)
{
    const ipcTest_SmallBitMask_t inValue = IPCTEST_SBM_VALUE1 | IPCTEST_SBM_VALUE3;
    ipcTest_SmallBitMask_t outValue = 0;

    ipcTest_EchoSmallBitMask(inValue, &outValue);
    LE_TEST_OK(inValue == outValue, "echo small bitmask");
}

static void TestEchoLargeBitMask(void)
{
    const ipcTest_LargeBitMask_t inValue = IPCTEST_LBM_VALUE64 | IPCTEST_LBM_VALUE9;
    ipcTest_LargeBitMask_t outValue = 0;

    ipcTest_EchoLargeBitMask(inValue, &outValue);
    LE_TEST_OK(inValue == outValue, "echo large bitmask");
}

static void TestEchoReference(void)
{
    const ipcTest_SimpleRef_t inRef = (ipcTest_SimpleRef_t)0x10000051;
    ipcTest_SimpleRef_t outRef = NULL;

    ipcTest_EchoReference(inRef, &outRef);
    LE_TEST_OK(inRef == outRef, "echo simple reference");
}

static void TestEchoErrorReference(void)
{
    const ipcTest_SimpleRef_t inRef = (ipcTest_SimpleRef_t)0;
    ipcTest_SimpleRef_t outRef = NULL;

    ipcTest_EchoReference(inRef, &outRef);
    LE_TEST_OK(inRef == outRef, "echo error reference");
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

    LE_TEST_OK(strcmp(inString, outString) == 0, "echo small string");
}

static void TestEchoMaxString(void)
{
    char inString[257];
    char outString[257];
    memset(inString, 'a', 256);
    inString[256] = '\0';

    ipcTest_EchoString(inString, outString, sizeof(outString));

    LE_TEST_OK(strcmp(inString, outString) == 0, "echo max string");
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

COMPONENT_INIT
{
    LE_TEST_PLAN(12*TEST_ITERATIONS);

    ipcTest_ConnectService();
    LE_TEST_INFO("Connected to server");

    int i;
    for (i = 0;i < TEST_ITERATIONS; ++i)
    {
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
    }

    LE_TEST_EXIT;
}
