//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"


// overwriting an existing file
// reading a "0-byte" file, with sufficient read buffer size
// delete a "0-byte" file
static void Test1
(
    void
)
{
    LE_TEST_INFO("Test1");

    char outBuffer[1024] = {0};
    size_t outBufferSize = sizeof(outBuffer);
    le_result_t result;


    result = le_secStore_Write("file1", (uint8_t*)"string321", 10);
    LE_TEST_OK(result == LE_OK, "Create a 10-byte file: [%s]", LE_RESULT_TXT(result));


    result = le_secStore_Write("file1", (uint8_t*)"string321", 0);
    LE_TEST_OK(result == LE_OK, "Empty the file: [%s]", LE_RESULT_TXT(result));


    result = le_secStore_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_TEST_OK(result == LE_OK, "Read from the 0-byte file [%s]", LE_RESULT_TXT(result));

    LE_TEST_OK(strcmp(outBuffer, "") == 0,
               "Checking secStore item contents '%s', expected ''", outBuffer);

    result = le_secStore_Delete("file1");
    LE_TEST_OK(result == LE_OK, "delete 0-byte file [%s]", LE_RESULT_TXT(result));

    LE_TEST_INFO("End of Test1");
}

// writing a "0-byte" file
// reading a "0-byte" file, with sufficient read buffer size
// delete a "0-byte" file
static void Test2
(
    void
)
{
    LE_TEST_INFO("Test2");

    char outBuffer[1024] = {0};
    size_t outBufferSize = sizeof(outBuffer);
    le_result_t result;

    result = le_secStore_Write("file1", (uint8_t*)"string321", 0);
    LE_TEST_OK(result == LE_OK, "Create a 0-byte file: [%s]", LE_RESULT_TXT(result));


    result = le_secStore_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_TEST_OK(result == LE_OK, "Read from the 0-byte file: [%s]", LE_RESULT_TXT(result));
    LE_TEST_OK(strcmp(outBuffer, "") == 0,
               "Checking secStore item contents '%s', expected ''", outBuffer);

    result = le_secStore_Delete("file1");
    LE_TEST_OK(result == LE_OK, "Delete the 0-byte file: [%s]", LE_RESULT_TXT(result));

    LE_TEST_INFO("End of Test2");
}

// writing a "0-byte" file
// reading a "0-byte" file, with 0-byte read buffer size
// delete a "0-byte" file
static void Test3
(
    void
)
{
    LE_TEST_INFO("Test3");

    char outBuffer[1024] = {0};
    size_t outBufferSize = 0;
    le_result_t result;


    result = le_secStore_Write("file1", (uint8_t*)"string321", 0);
    LE_TEST_OK(result == LE_OK, "Create a 0-byte file: [%s]", LE_RESULT_TXT(result));


    result = le_secStore_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_TEST_OK(result == LE_OK, "Read from the 0-byte file into a 0-byte buffer: [%s]",
               LE_RESULT_TXT(result));

    LE_TEST_OK(strcmp(outBuffer, "") == 0,
               "Checking secStore item contents '%s', expected ''",
               outBuffer);

    result = le_secStore_Delete("file1");
    LE_TEST_OK(result == LE_OK, "Delete the 0-byte file: [%s]", LE_RESULT_TXT(result));

    LE_TEST_INFO("End of Test3");
}

// writing a normal file
// reading a normal file, with sufficient read buffer size
// delete a normal file
static void Test4
(
    void
)
{
    LE_TEST_INFO("Test4");

    char outBuffer[1024] = {0};
    size_t outBufferSize = sizeof(outBuffer);
    le_result_t result;


    result = le_secStore_Write("file1", (uint8_t*)"string321", 10);
    LE_TEST_OK(result == LE_OK, "Create a 10-byte file: [%s]", LE_RESULT_TXT(result));


    result = le_secStore_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_TEST_OK(result == LE_OK, "Read from the 10-byte file: [%s]", LE_RESULT_TXT(result));

    LE_TEST_OK(strcmp(outBuffer, "string321") == 0,
               "Checking secStore item contents '%s', expecting 'string321'",
               outBuffer);

    result = le_secStore_Delete("file1");
    LE_TEST_OK(result == LE_OK, "Delete the 10-byte file: [%s]", LE_RESULT_TXT(result));

    LE_TEST_INFO("End of Test4");
}

// writing a normal file
// reading a normal file, with insufficient read buffer size (zero)
// delete a normal file
static void Test5
(
    void
)
{
    LE_TEST_INFO("Test5");

    char outBuffer[1024] = {0};
    size_t outBufferSize = 0;
    le_result_t result;


    result = le_secStore_Write("file1", (uint8_t*)"string321", 10);
    LE_TEST_OK(result == LE_OK, "Create a 10-byte file: [%s]", LE_RESULT_TXT(result));

    result = le_secStore_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_TEST_OK(result == LE_OVERFLOW, "Read from 10-byte file to a 0-byte buffer: [%s]",
                LE_RESULT_TXT(result));

    LE_TEST_OK(strcmp(outBuffer, "") == 0,
               "Checking secStore item contents '%s', expecting ''",
               outBuffer);

    result = le_secStore_Delete("file1");
    LE_TEST_OK(result == LE_OK, "Delete the 10-byte file: [%s]", LE_RESULT_TXT(result));

    LE_TEST_INFO("End of Test5");
}

// reading a non-existing file.
static void Test6
(
    void
)
{
    LE_TEST_INFO("Test6");

    char outBuffer[1024] = {0};
    size_t outBufferSize = sizeof(outBuffer);
    le_result_t result;

    result = le_secStore_Read("file2", (uint8_t*)outBuffer, &outBufferSize);
    LE_TEST_OK(result == LE_NOT_FOUND, "Read from a non-existing file: [%s]",
               LE_RESULT_TXT(result));

    LE_TEST_OK(outBuffer[0] == '\0',
               "Checking secStore item contents '%s', expecting ''",
               outBuffer);

    LE_TEST_INFO("End of Test6");
}

// writing a normal file
// reading a normal file, with insufficient read buffer size (one byte short)
// delete a normal file
static void Test7
(
    void
)
{
    LE_TEST_INFO("Test7");

    char outBuffer[1024] = {0};
    size_t outBufferSize = 9;
    le_result_t result;


    result = le_secStore_Write("file1", (uint8_t*)"string321", 10);
    LE_TEST_OK(result == LE_OK, "Create a 10-byte file: [%s]", LE_RESULT_TXT(result));

    result = le_secStore_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_TEST_OK(result == LE_OVERFLOW, "Reading from 10-byte file to a 9-byte buffer: [%s]",
               LE_RESULT_TXT(result));

    LE_TEST_OK(strcmp(outBuffer, "") == 0,
               "Checking secStore item contents '%s', expecting ''",
               outBuffer);

    result = le_secStore_Delete("file1");
    LE_TEST_OK(result == LE_OK, "Delete the 10-byte file: [%s]", LE_RESULT_TXT(result));

    LE_TEST_INFO("End of Test7");
}

#if LE_CONFIG_LINUX
// writing a large file
// reading a large file, with sufficient read buffer size
// delete a large file
static void Test8
(
    void
)
{
    LE_TEST_INFO("Test8");

    uint8_t inBuffer[5000] = {0};
    uint8_t outBuffer[5000] = {0};
    size_t inBufferSize = sizeof(outBuffer);
    size_t outBufferSize = sizeof(outBuffer);
    le_result_t result;

    int i;
    for (i = 0; i < sizeof(inBuffer); i++)
    {
        inBuffer[i] = i % 256;
    }
    result = le_secStore_Write("file1", inBuffer, inBufferSize);
    LE_TEST_OK(result == LE_OK, "Create a 5000-byte file: [%s]", LE_RESULT_TXT(result));


    result = le_secStore_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_TEST_OK(result == LE_OK, "Read from the 5000-byte file: [%s]", LE_RESULT_TXT(result));

    int mismatch = 0;
    for (i = 0; i < sizeof(outBuffer); i++)
    {
        if (outBuffer[i] != i % 256)
        {
            mismatch++;
        }
    }
    LE_TEST_OK(mismatch == 0,
               "Checking secStore item contents: %d bytes mismatch",
               mismatch);

    result = le_secStore_Delete("file1");
    LE_TEST_OK(result == LE_OK, "Delete the 5000-byte file: [%s]", LE_RESULT_TXT(result));

    LE_TEST_INFO("End of Test8");
}
#endif

// Write 2 normal files
// Read 2 normal files, with sufficient read buffer size
// Delete both files, i.e. delete all the contents of an application using "*" name
// Verify both files cannot be found
static void Test9
(
    void
)
{
    LE_TEST_INFO("Test9");

    char outBuffer[1024] = {0};
    size_t outBufferSize = sizeof(outBuffer);
    le_result_t result;

    result = le_secStore_Write("file1", (uint8_t*)"string321", 10);
    LE_TEST_OK(result == LE_OK, "Create a 10-byte file: [%s]", LE_RESULT_TXT(result));


    result = le_secStore_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_TEST_OK(result == LE_OK, "Read from the 10-byte file: [%s]", LE_RESULT_TXT(result));

    LE_TEST_OK(strcmp(outBuffer, "string321") == 0,
               "Checking secStore item contents '%s', expecting 'string321'",
               outBuffer);

    result = le_secStore_Write("file2", (uint8_t*)"string789", 10);
    LE_TEST_OK(result == LE_OK, "Create a 10-byte file: [%s]", LE_RESULT_TXT(result));


    result = le_secStore_Read("file2", (uint8_t*)outBuffer, &outBufferSize);
    LE_TEST_OK(result == LE_OK, "Read from the 10-byte file: [%s]", LE_RESULT_TXT(result));

    LE_TEST_OK(strcmp(outBuffer, "string789") == 0,
               "Checking secStore item contents '%s', expecting 'string789'",
               outBuffer);

    result = le_secStore_Delete("*");
    LE_TEST_OK(result == LE_OK, "Delete the app contents: [%s]", LE_RESULT_TXT(result));

    result = le_secStore_Read("*", (uint8_t*)outBuffer, &outBufferSize);
    LE_TEST_OK(result == LE_FAULT, "Invalid special name for reading: [%s]", LE_RESULT_TXT(result));

    result = le_secStore_Write("*", (uint8_t*)"string456", 10);
    LE_TEST_OK(result == LE_FAULT, "Invalid special name for writing: [%s]", LE_RESULT_TXT(result));

    result = le_secStore_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_TEST_OK(result == LE_NOT_FOUND, "Read from the file with deleted contents: [%s]",
               LE_RESULT_TXT(result));

    result = le_secStore_Read("file2", (uint8_t*)outBuffer, &outBufferSize);
    LE_TEST_OK(result == LE_NOT_FOUND, "Read from the file with contents deleted: [%s]",
               LE_RESULT_TXT(result));

    LE_TEST_INFO("End of Test9");
}

COMPONENT_INIT
{
    LE_TEST_PLAN(23);

    LE_TEST_INFO("=== SecStoreTest2 BEGIN ===");

    Test1();
    Test2();
    Test3();
    Test4();
    Test5();
    Test6();
    Test7();
// Test8 with large stack size requirement causes crash on ThreadX.
#if LE_CONFIG_LINUX
    Test8();
#endif
    Test9();
    LE_TEST_INFO("=== SecStoreTest2 END ===");

    LE_TEST_EXIT;
}
