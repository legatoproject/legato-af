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
    LE_INFO("###################################################################################");
    LE_INFO("#################### TestWriteRead ################################################");
    LE_INFO("###################################################################################");

    char outBuffer[1024] = {0};
    const char *dataPtr = "string321";
    size_t outBufferSize = sizeof(outBuffer);
    size_t dataSize = strnlen(dataPtr, outBufferSize);
    le_result_t result;


    LE_INFO("=======================   Create a %zu-byte file.   =====================", dataSize);
    result = secStoreGlobal_Write("file1", (uint8_t*)dataPtr, dataSize);
    LE_FATAL_IF(result != LE_OK, "write failed: [%s]", LE_RESULT_TXT(result));

    LE_INFO("=======================   Read from the %zu-byte file.   ================", dataSize);
    result = secStoreGlobal_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_FATAL_IF(result != LE_OK, "read failed: [%s]", LE_RESULT_TXT(result));

    LE_INFO("=======================   Check file content.   =========================");
    LE_FATAL_IF(outBufferSize != dataSize, "unexpected data size %zu", outBufferSize);
    if (0 != memcmp(outBuffer, dataPtr, outBufferSize))
    {
        LE_FATAL("Reading secStore item resulting in unexpected item contents: [%s]", outBuffer);
    }
    else
    {
        LE_INFO("secStore item read: [%s]", outBuffer);
    }

    LE_INFO("=======================   Empty the file.    ============================");
    result = secStoreGlobal_Write("file1", (uint8_t*)"string321", 0);
    LE_FATAL_IF(result != LE_OK, "write failed: [%s]", LE_RESULT_TXT(result));


    LE_INFO("=======================   Read from the 0-byte file.   ==================");
    outBuffer[0] = '\0';
    outBufferSize = sizeof(outBuffer);
    result = secStoreGlobal_Read("file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_FATAL_IF(result != LE_OK, "read failed: [%s]", LE_RESULT_TXT(result));
    LE_FATAL_IF(outBufferSize != 0, "unexpected data size %zu", outBufferSize);

    // File not deleted so it's presence can be checked after deletion of the test app

    LE_INFO("#################### END OF TestWriteRead ################################");
    LE_INFO(" ");
}

// create a file in /global/avms, gets its size and delete it
static void TestGlobalAvms
(
    void
)
{
    LE_INFO("###################################################################################");
    LE_INFO("#################### TestGlobalAvms ###############################################");
    LE_INFO("###################################################################################");

    char outBuffer[1024] = {0};
    const char *dataPtr = "string321";
    size_t outBufferSize = sizeof(outBuffer);
    size_t dataSize = strnlen(dataPtr, outBufferSize);
    uint64_t outSize;
    le_result_t result;


    LE_INFO("=======================   Create a %zu-byte file.   =====================", dataSize);
    result = secStoreGlobal_Write("/avms/file1", (uint8_t*)dataPtr, dataSize);
    LE_FATAL_IF(result != LE_OK, "write failed: [%s]", LE_RESULT_TXT(result));

    LE_INFO("=======================   Get size from the %zu-byte file.   ============", dataSize);
    outSize = 0;
    result = secStoreAdmin_GetSize("/global/avms/file1", &outSize);
    LE_FATAL_IF(result != LE_OK, "getsize failed: [%s]", LE_RESULT_TXT(result));
    LE_FATAL_IF(outSize != dataSize, "unexpected data size %"PRIu64, outSize);

    LE_INFO("=======================   Read from the %zu-byte file.   ================", dataSize);
    outBufferSize = sizeof(outBuffer);
    result = secStoreGlobal_Read("/avms/file1", (uint8_t*)outBuffer, &outBufferSize);
    LE_FATAL_IF(result != LE_OK, "read failed: [%s]", LE_RESULT_TXT(result));

    LE_INFO("=======================   Check file content.   ===========================");
    LE_FATAL_IF(outBufferSize != dataSize, "unexpected data size %zu", outBufferSize);
    if (0 != memcmp(outBuffer, dataPtr, outBufferSize))
    {
        LE_FATAL("Reading secStore item resulting in unexpected item contents: [%s]", outBuffer);
    }
    else
    {
        LE_INFO("secStore item read: [%s]", outBuffer);
    }

    // File not deleted so it's presence can be checked after deletion of the test app

    LE_INFO("#################### END OF TestWriteRead ###################################");
    LE_INFO(" ");
}

COMPONENT_INIT
{
    LE_INFO("=====================================================================");
    LE_INFO("==================== SecStoreTestGlobal BEGIN =======================");
    LE_INFO("=====================================================================");

    TestWriteRead();
    TestGlobalAvms();

    LE_INFO("============ SecStoreTestGlobal PASSED =============");

    exit(EXIT_SUCCESS);
}

