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
    LE_INFO("############################################################################################");
    LE_INFO("#################### Test1 #################################################################");
    LE_INFO("############################################################################################");

    char outBuffer[1024] = {0};
    size_t outBufferSize = sizeof(outBuffer);
    le_result_t result;


    LE_INFO("=======================   Create a 10-byte file.   ==============================");
    result = le_secStore_Write("file1", (uint8_t*)"string321", 10);
    LE_FATAL_IF(result != LE_OK, "write failed: [%s]", LE_RESULT_TXT(result));


    LE_INFO("=======================   Empty the file.    ==============================");
    result = le_secStore_Write("file1", (uint8_t*)"string321", 0);
    LE_FATAL_IF(result != LE_OK, "write failed: [%s]", LE_RESULT_TXT(result));


    LE_INFO("=======================   Read from the 0-byte file.   =================================");
    result = le_secStore_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_FATAL_IF(result != LE_OK, "read failed: [%s]", LE_RESULT_TXT(result));

    if (strcmp(outBuffer, "") != 0)
    {
        LE_FATAL("Reading secStore item resulting in unexpected item contents: [%s]", outBuffer);
    }
    else
    {
        LE_INFO("secStore item read: [%s]", outBuffer);
    }


    LE_INFO("=======================   Delete the 0-byte file.   ====================================");
    result = le_secStore_Delete("file1");
    LE_FATAL_IF(result != LE_OK, "delete failed: [%s]", LE_RESULT_TXT(result));

    LE_INFO("#################### END OF Test1 #################################################################");
    LE_INFO(" ");
}

// writing a "0-byte" file
// reading a "0-byte" file, with sufficient read buffer size
// delete a "0-byte" file
static void Test2
(
    void
)
{
    LE_INFO("############################################################################################");
    LE_INFO("#################### Test2 #################################################################");
    LE_INFO("############################################################################################");

    char outBuffer[1024] = {0};
    size_t outBufferSize = sizeof(outBuffer);
    le_result_t result;


    LE_INFO("=======================   Create a 0-byte file.    ==============================");
    result = le_secStore_Write("file1", (uint8_t*)"string321", 0);
    LE_FATAL_IF(result != LE_OK, "write failed: [%s]", LE_RESULT_TXT(result));


    LE_INFO("=======================   Read from the 0-byte file.   =================================");
    result = le_secStore_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_FATAL_IF(result != LE_OK, "read failed: [%s]", LE_RESULT_TXT(result));

    if (strcmp(outBuffer, "") != 0)
    {
        LE_FATAL("Reading secStore item resulting in unexpected item contents: [%s]", outBuffer);
    }
    else
    {
        LE_INFO("secStore item read: [%s]", outBuffer);
    }


    LE_INFO("=======================   Delete the 0-byte file.   ====================================");
    result = le_secStore_Delete("file1");
    LE_FATAL_IF(result != LE_OK, "delete failed: [%s]", LE_RESULT_TXT(result));

    LE_INFO("#################### END OF Test2 #################################################################");
    LE_INFO(" ");
}

// writing a "0-byte" file
// reading a "0-byte" file, with 0-byte read buffer size
// delete a "0-byte" file
static void Test3
(
    void
)
{
    LE_INFO("############################################################################################");
    LE_INFO("#################### Test3 #################################################################");
    LE_INFO("############################################################################################");

    char outBuffer[1024] = {0};
    size_t outBufferSize = 0;
    le_result_t result;


    LE_INFO("=======================   Create a 0-byte file.    ==============================");
    result = le_secStore_Write("file1", (uint8_t*)"string321", 0);
    LE_FATAL_IF(result != LE_OK, "write failed: [%s]", LE_RESULT_TXT(result));


    LE_INFO("=======================   Read from the 0-byte file to a 0-byte buffer.   =================================");
    result = le_secStore_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_FATAL_IF(result != LE_OK, "read failed: [%s]", LE_RESULT_TXT(result));

    if (strcmp(outBuffer, "") != 0)
    {
        LE_FATAL("Reading secStore item resulting in unexpected item contents: [%s]", outBuffer);
    }
    else
    {
        LE_INFO("secStore item read: [%s]", outBuffer);
    }


    LE_INFO("=======================   Delete the 0-byte file.   ====================================");
    result = le_secStore_Delete("file1");
    LE_FATAL_IF(result != LE_OK, "delete failed: [%s]", LE_RESULT_TXT(result));

    LE_INFO("#################### END OF Test3 #################################################################");
    LE_INFO(" ");
}

// writing a normal file
// reading a normal file, with sufficient read buffer size
// delete a normal file
static void Test4
(
    void
)
{
    LE_INFO("############################################################################################");
    LE_INFO("#################### Test4 #################################################################");
    LE_INFO("############################################################################################");

    char outBuffer[1024] = {0};
    size_t outBufferSize = sizeof(outBuffer);
    le_result_t result;


    LE_INFO("=======================   Create a 10-byte file.    ==============================");
    result = le_secStore_Write("file1", (uint8_t*)"string321", 10);
    LE_FATAL_IF(result != LE_OK, "write failed: [%s]", LE_RESULT_TXT(result));


    LE_INFO("=======================   Read from the 10-byte file.   =================================");
    result = le_secStore_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_FATAL_IF(result != LE_OK, "read failed: [%s]", LE_RESULT_TXT(result));

    if (strcmp(outBuffer, "string321") != 0)
    {
        LE_FATAL("Reading secStore item resulting in unexpected item contents: [%s]", outBuffer);
    }
    else
    {
        LE_INFO("secStore item read: [%s]", outBuffer);
    }


    LE_INFO("=======================   Delete the 10-byte file.   ====================================");
    result = le_secStore_Delete("file1");
    LE_FATAL_IF(result != LE_OK, "delete failed: [%s]", LE_RESULT_TXT(result));

    LE_INFO("#################### END OF Test4 #################################################################");
    LE_INFO(" ");
}

// writing a normal file
// reading a normal file, with insufficient read buffer size
// delete a normal file
static void Test5
(
    void
)
{
    LE_INFO("############################################################################################");
    LE_INFO("#################### Test5 #################################################################");
    LE_INFO("############################################################################################");

    char outBuffer[1024] = {0};
    size_t outBufferSize = 0;
    le_result_t result;


    LE_INFO("=======================   Create a 10-byte file.    ==============================");
    result = le_secStore_Write("file1", (uint8_t*)"string321", 10);
    LE_FATAL_IF(result != LE_OK, "write failed: [%s]", LE_RESULT_TXT(result));


    LE_INFO("=======================   Read from 10-byte file to a 0-byte buffer.   =================================");
    result = le_secStore_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_FATAL_IF(result != LE_OVERFLOW, "read failed: [%s]", LE_RESULT_TXT(result));

    if (strcmp(outBuffer, "") != 0)
    {
        LE_FATAL("Reading secStore item resulting in unexpected item contents: [%s]", outBuffer);
    }
    else
    {
        LE_INFO("secStore item read: [%s]", outBuffer);
    }


    LE_INFO("=======================   Delete the 10-byte file.   ====================================");
    result = le_secStore_Delete("file1");
    LE_FATAL_IF(result != LE_OK, "delete failed: [%s]", LE_RESULT_TXT(result));

    LE_INFO("#################### END OF Test5 #################################################################");
    LE_INFO(" ");
}

// reading a non-existing file.
static void Test6
(
    void
)
{
    LE_INFO("############################################################################################");
    LE_INFO("#################### Test6 #################################################################");
    LE_INFO("############################################################################################");

    char outBuffer[1024] = {0};
    size_t outBufferSize = sizeof(outBuffer);
    le_result_t result;

    LE_INFO("=======================   Read from a non-existing file.   =================================");
    result = le_secStore_Read("file2", (uint8_t*)outBuffer, &outBufferSize);
    LE_FATAL_IF(result != LE_NOT_FOUND, "read failed: [%s]", LE_RESULT_TXT(result));

    if (outBuffer[0] != '\0')
    {
        LE_FATAL("Reading secStore item resulting in unexpected item contents: [%s]", outBuffer);
    }
    else
    {
        LE_INFO("secStore item read: [%s]", outBuffer);
    }

    LE_INFO("#################### END OF Test6 #################################################################");
    LE_INFO(" ");
}


COMPONENT_INIT
{
    LE_INFO("=====================================================================");
    LE_INFO("==================== SecStoreTest2 BEGIN ===========================");
    LE_INFO("=====================================================================");

    Test1();
    Test2();
    Test3();
    Test4();
    Test5();
    Test6();

    LE_INFO("============ SecStoreTest2 PASSED =============");

    exit(EXIT_SUCCESS);
}

