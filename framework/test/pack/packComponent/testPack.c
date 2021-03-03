 /**
  * This module is for unit testing the le_pack module in the legato
  * runtime library (liblegato.so).
  *
  * It goes through each Pack/Unpack functions and checks for normal behavior
  * and corner cases.
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#include "legato.h"
#include <stdio.h>

#define BUFFER_SZ 1024
#define CHECK_CHAR 0x42

static void ResetBuffer(uint8_t* bufferPtr, size_t bufferSz)
{
    memset(bufferPtr, CHECK_CHAR, bufferSz);
}

/** Uint8 **/

static void CheckUint8(uint8_t value)
{
    uint8_t buffer[BUFFER_SZ];
    uint8_t* bufferPtr = buffer;
    bool res;

    ResetBuffer(bufferPtr, sizeof(buffer));

    // Pack
    res = le_pack_PackUint8(&bufferPtr, value);
    LE_TEST_OK(res == true, "Pack a uint8_t into a buffer");
    LE_TEST_OK(bufferPtr != buffer, "Increment the buffer pointer as appropriate");
    LE_TEST_OK(bufferPtr[0] == CHECK_CHAR, "Incremented buffer pointer is ready");

    // Unpack
    uint8_t valueOut = 0x00;
    bufferPtr = buffer;
    res = le_pack_UnpackUint8(&bufferPtr, &valueOut);
    LE_TEST_OK(res == true, "Unpack a buffer into a uint8_t");
    LE_TEST_OK(valueOut == value, "Unpacked uint8_t is correct");
}

static void TestUint8(void)
{
    LE_TEST_INFO("=> Testing packing/unpacking uint8_t\n");

    CheckUint8(0x00);
    CheckUint8(0xAB);
    CheckUint8(0xFF);
}

/** String **/

static void CheckString
(
    const char* stringPtr,      ///< Test string
    size_t reportedBufferSz,    ///< Buffer size reported to pack/unpack
    uint32_t maxStringCount,    ///< Max string size
    bool expectedRes            ///< Expected result
)
{
    uint8_t buffer[BUFFER_SZ];
    uint8_t* bufferPtr = buffer;
    size_t stringLen = strlen(stringPtr);
    bool res;

    if (stringLen >= BUFFER_SZ)
    {
        stringLen = BUFFER_SZ - 1;
    }

    ResetBuffer(bufferPtr, sizeof(buffer));

    LE_TEST_INFO("'%s' - [%"PRIuS"] buffer[%"PRIuS"] maxString[%"PRIu32"]:\n",
            stringPtr,
            stringLen,
            reportedBufferSz,
            maxStringCount);

    // Pack
    res = le_pack_PackString(&bufferPtr, stringPtr, maxStringCount);
    LE_TEST_OK(res == expectedRes, "Pack a string into a buffer");
    if (!expectedRes)
    {
        return;
    }

    LE_TEST_OK(bufferPtr != buffer, "Increment the buffer pointer as appropriate");
    LE_TEST_OK(bufferPtr[0] == CHECK_CHAR, "Incremented buffer pointer is ready");

    // Unpack
    char valueOut[BUFFER_SZ];
    bufferPtr = buffer;
    res = le_pack_UnpackString(&bufferPtr, valueOut, reportedBufferSz, maxStringCount);
    LE_TEST_OK(res == expectedRes, "Unpack a buffer into a string");
    LE_TEST_OK(0 == memcmp(stringPtr, valueOut, stringLen), "Unpacked string is correct");
    LE_TEST_OK(valueOut[stringLen] == '\0', "Unpacked string is null terminated");
}

static void TestString(void)
{
    LE_TEST_INFO("=> Testing packing/unpacking string\n");

    CheckString("normal", 512, 128, true);
    CheckString("buffertooshort", 512, 10, false);
    CheckString("bufferexactlen", 512, 14, true);
    CheckString("buffertooshortby1", 512, 16, false);
    CheckString("", 512, 12, true); // Empty
}

COMPONENT_INIT
{
    LE_TEST_INIT;
    LE_TEST_INFO("======== le_pack Test Started ========\n");

    TestUint8();
    TestString();

    LE_TEST_INFO("======== le_pack Test Complete ========\n");
    LE_TEST_EXIT;
}
