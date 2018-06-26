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
// reading a normal file, with insufficient read buffer size
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

    LE_TEST_INFO("=== SecStoreTest2 END ===");

    LE_TEST_EXIT;
}
