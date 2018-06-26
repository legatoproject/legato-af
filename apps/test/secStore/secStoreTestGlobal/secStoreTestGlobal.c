//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"


// overwriting an existing file
// reading a "0-byte" file, with sufficient read buffer size
// delete a "0-byte" file
static void TestWriteRead
(
    void
)
{
    LE_TEST_INFO("Start Write/Read Tests");

    char outBuffer[1024] = {0};
    const char *dataPtr = "string321";
    size_t outBufferSize = sizeof(outBuffer);
    size_t dataSize = strnlen(dataPtr, outBufferSize);
    le_result_t result;


    result = secStoreGlobal_Write("file1", (uint8_t*)dataPtr, dataSize);
    LE_TEST_OK(result == LE_OK, "write to 'file1': [%s]", LE_RESULT_TXT(result));

    result = secStoreGlobal_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_TEST_OK(result == LE_OK, "read from 'file1': [%s]", LE_RESULT_TXT(result));

    LE_TEST_OK(outBufferSize == dataSize, "'file1' data size %u", (unsigned)outBufferSize);
    LE_TEST_OK(0 == memcmp(outBuffer, dataPtr, outBufferSize),
               "'file1' item contents '%s' match expected '%s'", outBuffer, dataPtr);

    result = secStoreGlobal_Write("file1", (uint8_t*)"string321", 0);
    LE_TEST_OK(result == LE_OK, "clear 'file1': [%s]", LE_RESULT_TXT(result));


    outBuffer[0] = '\0';
    outBufferSize = sizeof(outBuffer);
    result = secStoreGlobal_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_TEST_OK(result == LE_OK, "read empty 'file1': [%s]", LE_RESULT_TXT(result));
    LE_TEST_OK(outBufferSize == 0, "empty 'file' size is %d", (unsigned)outBufferSize);

    // File not deleted so it's presence can be checked after deletion of the test app
    LE_TEST_INFO("End Write/Read Tests");
}

// create a file in /global/avms, gets its size and delete it
static void TestGlobalAvms
(
    void
)
{
    LE_TEST_INFO("Start Global AVMS tests");

    char outBuffer[1024] = {0};
    const char *dataPtr = "string321";
    size_t outBufferSize = sizeof(outBuffer);
    size_t dataSize = strnlen(dataPtr, outBufferSize);
    uint64_t outSize;
    le_result_t result;


    result = secStoreGlobal_Write("/avms/file1", (uint8_t*)dataPtr, dataSize);
    LE_TEST_OK(result == LE_OK, "write %u bytes: [%s]", (unsigned)dataSize, LE_RESULT_TXT(result));

    outSize = 0;
    result = secStoreAdmin_GetSize("/global/avms/file1", &outSize);
    LE_TEST_OK(result == LE_OK, "getsize: [%s]", LE_RESULT_TXT(result));
    LE_TEST_OK(outSize == dataSize, "check data size %"PRIu64" (expected %u)",
                outSize, (unsigned)dataSize);

    outBufferSize = sizeof(outBuffer);
    result = secStoreGlobal_Read("/avms/file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_TEST_OK(result == LE_OK, "read: [%s]", LE_RESULT_TXT(result));

    LE_TEST_OK(outBufferSize == dataSize, "check data size %u (expected %u)",
               (unsigned)outBufferSize, (unsigned)dataSize);
    LE_TEST_OK(0 == memcmp(outBuffer, dataPtr, outBufferSize),
               "check data contents %s (expected %s)",
               outBuffer, dataPtr);

    // File not deleted so it's presence can be checked after deletion of the test app

    LE_TEST_INFO("End Global AVMS tests");
}

COMPONENT_INIT
{
    LE_TEST_PLAN(13);

    TestWriteRead();
    TestGlobalAvms();

    LE_TEST_EXIT;
}
