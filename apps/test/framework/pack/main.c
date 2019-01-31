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

    ResetBuffer(bufferPtr, sizeof(buffer));

    // Pack
    LE_TEST(le_pack_PackUint8(&bufferPtr, value));
    LE_TEST(bufferPtr != buffer);
    LE_TEST(bufferPtr[0] == CHECK_CHAR);

    // Unpack
    uint8_t valueOut = 0x00;
    bufferPtr = buffer;
    LE_TEST(le_pack_UnpackUint8(&bufferPtr, &valueOut));

    LE_TEST(valueOut == value);
}

static void TestUint8(void)
{
    printf("=> uint8\n");
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
    size_t stringLen = strnlen(stringPtr, BUFFER_SZ);

    ResetBuffer(bufferPtr, sizeof(buffer));

    printf("'%s' - [%zd] buffer[%zd] maxString[%d]:\n",
           stringPtr,
           stringLen,
           reportedBufferSz,
           maxStringCount);

    // Pack
    LE_TEST(expectedRes == le_pack_PackString(&bufferPtr,
                                              stringPtr,
                                              maxStringCount));
    if(!expectedRes)
    {
        printf("   [passed]\n");
        return;
    }

    // Check buffer
    LE_TEST(bufferPtr != buffer);
    LE_TEST(bufferPtr[0] == CHECK_CHAR);

    // Unpack
    char valueOut[BUFFER_SZ];
    bufferPtr = buffer;
    LE_TEST(le_pack_UnpackString(&bufferPtr,
                                 valueOut,
                                 reportedBufferSz,
                                 maxStringCount));

    // Output must be the same as input
    LE_TEST(0 == memcmp(stringPtr, valueOut, stringLen));
    // Output must be null terminated
    LE_TEST(valueOut[stringLen] == '\0');

    printf("   [passed]\n");
}

static void TestString(void)
{
    printf("=> string\n");

    CheckString("normal", 512, 128, true);
    CheckString("buffertooshort", 512, 10, false);
    CheckString("bufferexactlen", 512, 14, true);
    CheckString("buffertooshortby1", 512, 16, false);
    CheckString("", 512, 12, true); // Empty
}

COMPONENT_INIT
{
    printf("======== le_pack Test Started ========\n");

    // Setup the Legato Test Framework.
    LE_TEST_INIT;

    TestUint8();
    TestString();

    printf("======== le_pack Test Complete ========\n");
    printf("\n");

    // Exit with the number of failed tests as the exit code.
    LE_TEST_EXIT;
}

