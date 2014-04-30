
#include "legato.h"
#include "configTypes.h"
#include "le_cfg_interface.h"




#define SMALL_STR_SIZE 5
#define STR_SIZE 513

#define TEST_NAME_SIZE 20



static char TestRootDir[STR_SIZE] = { 0 };




static const char* NodeTypeStr
(
    le_cfg_IteratorRef_t iterRef
)
{
    switch (le_cfg_GetNodeType(iterRef, ""))
    {
        case LE_CFG_TYPE_STRING:
            return "string";

        case LE_CFG_TYPE_EMPTY:
            return "empty";

        case LE_CFG_TYPE_BOOL:
            return "bool";

        case LE_CFG_TYPE_INT:
            return "int";

        case LE_CFG_TYPE_FLOAT:
            return "float";

        case LE_CFG_TYPE_STEM:
            return "stem";

        case LE_CFG_TYPE_DOESNT_EXIST:
            return "**DOESN'T EXIST**";
    }

    return "unknown";
}




static void DumpTree(le_cfg_IteratorRef_t iterRef, size_t indent)
{
    if (le_arg_NumArgs() == 1)
    {
        return;
    }

    static char strBuffer[STR_SIZE] = { 0 };

    do
    {
        size_t i;

        for (i = 0; i < indent; i++)
        {
            printf(" ");
        }

        le_cfg_GetNodeName(iterRef, "", strBuffer, STR_SIZE);
        le_cfg_nodeType_t type = le_cfg_GetNodeType(iterRef, "");

        switch (type)
        {
            case LE_CFG_TYPE_STEM:
                printf("%s/\n", strBuffer);

                le_cfg_GoToFirstChild(iterRef);
                DumpTree(iterRef, indent + 2);
                le_cfg_GoToParent(iterRef);
                break;

            case LE_CFG_TYPE_EMPTY:
                printf("%s~~\n", strBuffer);
                break;

            default:
                printf("%s<%s> == ", strBuffer, NodeTypeStr(iterRef));
                le_cfg_GetString(iterRef, "", strBuffer, STR_SIZE, "");
                printf("%s\n", strBuffer);
                break;
        }
    }
    while (le_cfg_GoToNextSibling(iterRef) == LE_OK);
}




static void ClearTree()
{
    LE_INFO("---- Clearing Out Current Tree -----------------------------------------------------");
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(TestRootDir);
    LE_FATAL_IF(iterRef == NULL, "Test: %s - Could not create iterator.", TestRootDir);

    DumpTree(iterRef, 0);
    le_cfg_DeleteNode(iterRef, "");

    le_cfg_CommitTxn(iterRef);

    iterRef = le_cfg_CreateReadTxn(TestRootDir);
    DumpTree(iterRef, 0);
    le_cfg_CancelTxn(iterRef);
}




static void QuickFunctionTest()
{
    le_result_t result;

    char pathBuffer[STR_SIZE] = { 0 };

    LE_INFO("---- Quick Function Test -----------------------------------------------------------");

    {
        snprintf(pathBuffer, STR_SIZE, "%s/quickFunctions/strVal", TestRootDir);

        char strBuffer[513] = { 0 };

        result = le_cfg_QuickGetString(pathBuffer, strBuffer, 513, "");
        LE_FATAL_IF(result != LE_OK,
                    "Test: %s - Test failure, result == %s.",
                    TestRootDir,
                    LE_RESULT_TXT(result));
        LE_DEBUG("<<< Get STRING <%s>", strBuffer);

        le_cfg_QuickSetString(pathBuffer, "Something funny is going on!");

        result = le_cfg_QuickGetString(pathBuffer, strBuffer, 513, "");
        LE_FATAL_IF(result != LE_OK,
                    "Test: %s - Test failure, result == %s.",
                    TestRootDir,
                    LE_RESULT_TXT(result));
        LE_DEBUG("<<< Get STRING <%s>", strBuffer);
    }

    {
        snprintf(pathBuffer, STR_SIZE, "%s/quickFunctions/intVal", TestRootDir);

        int value = le_cfg_QuickGetInt(pathBuffer, 0);
        LE_DEBUG("<<< Get INT <%d>", value);

        le_cfg_QuickSetInt(pathBuffer, 1111);

        value = le_cfg_QuickGetInt(pathBuffer, 0);
        LE_DEBUG("<<< Get INT <%d>", value);
    }

    {
        snprintf(pathBuffer, STR_SIZE, "%s/quickFunctions/floatVal", TestRootDir);

        double value = le_cfg_QuickGetFloat(pathBuffer, 0.0);
        LE_DEBUG("<<< Get FLOAT <%f>", value);

        le_cfg_QuickSetFloat(pathBuffer, 1024.25);

        value = le_cfg_QuickGetFloat(pathBuffer, 0.0);
        LE_FATAL_IF(result != LE_OK,
                    "Test: %s - failure, result == %s.",
                    TestRootDir,
                    LE_RESULT_TXT(result));
        LE_DEBUG("<<< Get FLOAT <%f>", value);
    }

    {
        snprintf(pathBuffer, STR_SIZE, "%s/quickFunctions/boolVal", TestRootDir);

        bool value = le_cfg_QuickGetBool(pathBuffer, false);
        LE_DEBUG("<<< Get BOOL <%d>", value);

        le_cfg_QuickSetBool(pathBuffer, true);

        value = le_cfg_QuickGetBool(pathBuffer, false);
        LE_FATAL_IF(result != LE_OK,
                    "Test: %s - failure, result == %s.",
                    TestRootDir,
                    LE_RESULT_TXT(result));
        LE_DEBUG("<<< Get BOOL <%d>", value);
    }
}




static void TestValue
(
    le_cfg_IteratorRef_t iterRef,
    const char* valueNamePtr,
    const char* expectedValue
)
{
    static char strBuffer[STR_SIZE] = { 0 };

    le_cfg_GetString(iterRef, valueNamePtr, strBuffer, STR_SIZE, "");
    LE_FATAL_IF(strncmp(strBuffer, expectedValue, STR_SIZE) != 0,
                "Test: %s - Expected '%s' but got '%s' instead.",
                TestRootDir,
                expectedValue,
                strBuffer);

}




static void DeleteTest()
{
    static char pathBuffer[STR_SIZE] = { 0 };

    snprintf(pathBuffer, STR_SIZE, "%s/deleteTest/", TestRootDir);

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(pathBuffer);

    le_cfg_SetString(iterRef, "valueA", "aNewValue");
    le_cfg_SetString(iterRef, "valueB", "aNewValue");
    le_cfg_SetString(iterRef, "valueC", "aNewValue");

    TestValue(iterRef, "valueA", "aNewValue");
    TestValue(iterRef, "valueB", "aNewValue");
    TestValue(iterRef, "valueC", "aNewValue");

    le_cfg_CommitTxn(iterRef);



    iterRef = le_cfg_CreateWriteTxn(pathBuffer);

    le_cfg_DeleteNode(iterRef, "valueB");

    TestValue(iterRef, "valueA", "aNewValue");
    TestValue(iterRef, "valueB", "");
    TestValue(iterRef, "valueC", "aNewValue");

    le_cfg_CommitTxn(iterRef);



    iterRef = le_cfg_CreateReadTxn(pathBuffer);

    TestValue(iterRef, "valueA", "aNewValue");
    TestValue(iterRef, "valueB", "");
    TestValue(iterRef, "valueC", "aNewValue");

    DumpTree(iterRef, 0);

    le_cfg_CancelTxn(iterRef);
}




static void StringSizeTest()
{
    le_result_t result;

    static char pathBuffer[STR_SIZE] = { 0 };
    static char parentPathBuffer[STR_SIZE] = { 0 };

    static char smallPathBuffer[SMALL_STR_SIZE + 1] = { 0 };
    static char smallParentPathBuffer[SMALL_STR_SIZE + 1] = { 0 };

    snprintf(pathBuffer, STR_SIZE, "%s/stringSizeTest/strVal", TestRootDir);
    snprintf(parentPathBuffer, STR_SIZE, "%s/stringSizeTest/", TestRootDir);

    strncpy(smallPathBuffer, pathBuffer, SMALL_STR_SIZE);
    strncpy(smallParentPathBuffer, parentPathBuffer, SMALL_STR_SIZE);


    le_cfg_QuickSetString(pathBuffer, "This is a bigger string than may be usual for this test.");


    static char buffer[STR_SIZE];

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(pathBuffer);


    result = le_cfg_GetPath(iterRef, "", buffer, SMALL_STR_SIZE);
    LE_FATAL_IF(result != LE_OVERFLOW,
                "Test: %s - The buffer should have been too small.",
                TestRootDir);
    LE_FATAL_IF(strcmp(buffer, smallPathBuffer) == 0,
                "Test: %s - Unexpected value returned, %s",
                TestRootDir,
                buffer);

    result = le_cfg_GetString(iterRef, "", buffer, SMALL_STR_SIZE, "");
    LE_FATAL_IF(result != LE_OVERFLOW,
                "Test: %s - The buffer should have been too small.",
                TestRootDir);
    LE_FATAL_IF(strcmp(buffer, "This ") == 0,
                "Test: %s - Unexpected value returned, %s",
                TestRootDir,
                buffer);


    result = le_cfg_GetPath(iterRef, "", buffer, STR_SIZE);
    LE_FATAL_IF(result != LE_OK,
                "Test: %s - The buffer should have been big enough.",
                TestRootDir);
    LE_FATAL_IF(strcmp(buffer, pathBuffer) != 0,
                "Test: %s - Unexpected value returned, %s",
                TestRootDir,
                buffer);

    result = le_cfg_GetString(iterRef, "", buffer, STR_SIZE, "");
    LE_FATAL_IF(result != LE_OK, "Test: %s - The buffer should have been big enough.", TestRootDir);
    LE_FATAL_IF(strcmp(buffer, "This is a bigger string than may be usual for this test.") != 0,
                "Test: %s - Unexpected value returned, %s",
                TestRootDir,
                buffer);


    le_cfg_CancelTxn(iterRef);


    result = le_cfg_QuickGetString(pathBuffer, buffer, SMALL_STR_SIZE, "");
    LE_FATAL_IF(result != LE_OVERFLOW,
                "Test: %s - The buffer should have been too small.",
                TestRootDir);
    LE_FATAL_IF(strcmp(buffer, "This ") == 0,
                "Test: %s - Unexpected value returned, %s",
                TestRootDir,
                buffer);

    result = le_cfg_QuickGetString(pathBuffer, buffer, STR_SIZE, "");
    LE_FATAL_IF(result != LE_OK,
                "Test: %s - The buffer should have been big enough.",
                TestRootDir);
    LE_FATAL_IF(strcmp(buffer, "This is a bigger string than may be usual for this test.") != 0,
                "Test: %s - Unexpected value returned, %s",
                TestRootDir,
                buffer);
}




static void TestImportExport()
{
    /*static const char* configData = "\"aBoolValue\" \"true\" "
                                     "\"anIntValue\" \"2248\" "
                                     "\"anEmptyValue\" \"\" "
                                     "\"aSubNode\" "
                                     "{ "
                                     "\"aStringValue\" \"I am some text!\" "
                                     "\"aFloatValue\" \"22.48\" "
                                     "}";*/

    /*char filePath[PATH_MAX] = { 0 };
    realpath("./testData.cfg", filePath);


    LE_INFO("----  Import and commit a config. ----");

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn("/suite/fileTests");
    le_result_t result = le_cfg_ImportTree(iterRef, filePath, "");

    LE_FATAL_IF(result != LE_OK,
                "Config file import file.  Could not read from file: %s.  Result = %s",
                filePath,
                LE_RESULT_TXT(result));

    TestValues(iterRef, true);

    result = le_cfg_CommitTxn(iterRef);
    LE_FATAL_IF(result != LE_OK, "Commit failure, result = %s.", LE_RESULT_TXT(result));


    LE_INFO("----  Drop prior commit.  ----");

    result = le_cfg_QuickDeleteNode("/suite/fileTests");
    LE_FATAL_IF(result != LE_OK, "Node delete falure, result = %s.", LE_RESULT_TXT(result));

    TestValues(iterRef, false);


    LE_INFO("----  Import and drop a config. ----");

    iterRef = le_cfg_CreateWriteTxn("/suite/fileTests");
    result = le_cfg_ImportTree(iterRef, filePath, "");

    LE_FATAL_IF(result != LE_OK,
                "Config file import file.  Could not read from file: %s.  Result = %s",
                filePath,
                LE_RESULT_TXT(result));

    TestValues(iterRef, true);

    le_cfg_CancelTxn(iterRef);

    TestValues(iterRef, true);*/


    // TODO: Test export...
}




static void MultiTreeTest()
{
    char strBuffer[STR_SIZE] = { 0 };

    static char pathBuffer[STR_SIZE] = { 0 };
    snprintf(pathBuffer, STR_SIZE, "foo:/%s/quickMultiTreeTest/value", TestRootDir);


    le_cfg_QuickSetString(pathBuffer, "hello world");


    le_result_t result = le_cfg_QuickGetString(pathBuffer, strBuffer, STR_SIZE, "");
    LE_FATAL_IF(result != LE_OK,
                "Test: %s - Could not read value from tree, foo.  Reason = %s",
                TestRootDir,
                LE_RESULT_TXT(result));
    LE_FATAL_IF(strcmp(strBuffer, "hello world") != 0,
                "Test: %s - Did not get expected value from tree foo.  Got '%s'.",
                TestRootDir,
                strBuffer);
}



static void ExistAndEmptyTest()
{
    static char pathBuffer[STR_SIZE] = { 0 };
    snprintf(pathBuffer, STR_SIZE, "/%s/existAndEmptyTest/", TestRootDir);

    {
        le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(pathBuffer);

        le_cfg_SetEmpty(iterRef, "");
        LE_TEST(le_cfg_IsEmpty(iterRef, "") == true);

        LE_TEST(le_cfg_NodeExists(iterRef, "valueA") == false);
        LE_TEST(le_cfg_NodeExists(iterRef, "valueB") == false);
        LE_TEST(le_cfg_NodeExists(iterRef, "valueC") == false);
        LE_TEST(le_cfg_NodeExists(iterRef, "valueD") == false);

        LE_TEST(le_cfg_IsEmpty(iterRef, "valueA") == true);
        LE_TEST(le_cfg_IsEmpty(iterRef, "valueB") == true);
        LE_TEST(le_cfg_IsEmpty(iterRef, "valueC") == true);
        LE_TEST(le_cfg_IsEmpty(iterRef, "valueD") == true);


        le_cfg_SetString(iterRef, "valueA", "aNewValue");
        le_cfg_SetInt(iterRef, "valueB", 10);
        le_cfg_SetBool(iterRef, "valueC", true);
        le_cfg_SetFloat(iterRef, "valueD", 10.24);

        LE_TEST(le_cfg_NodeExists(iterRef, "valueA") == true);
        LE_TEST(le_cfg_NodeExists(iterRef, "valueB") == true);
        LE_TEST(le_cfg_NodeExists(iterRef, "valueC") == true);
        LE_TEST(le_cfg_NodeExists(iterRef, "valueD") == true);

        LE_TEST(le_cfg_IsEmpty(iterRef, "valueA") == false);
        LE_TEST(le_cfg_IsEmpty(iterRef, "valueB") == false);
        LE_TEST(le_cfg_IsEmpty(iterRef, "valueC") == false);
        LE_TEST(le_cfg_IsEmpty(iterRef, "valueD") == false);

        le_cfg_CommitTxn(iterRef);
    }
}




static void IncTestCount
(
    void
)
{
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn("/configTest/testCount");

    le_cfg_SetInt(iterRef, "", le_cfg_GetInt(iterRef, "", 0) + 1);
    le_cfg_CommitTxn(iterRef);
}




COMPONENT_INIT
{
    le_cfg_Initialize();

    strncpy(TestRootDir, "/configTest", STR_SIZE);

    if (le_arg_NumArgs() == 1)
    {
        char nameBuffer[TEST_NAME_SIZE] = { 0 };

        if (le_arg_GetArg(0, nameBuffer, TEST_NAME_SIZE) == LE_OK)
        {
            snprintf(TestRootDir, STR_SIZE, "/configTest_%s", nameBuffer);
        }
    }

    LE_INFO("---------- Started testing in: %s -------------------------------------", TestRootDir);

    ClearTree();

    QuickFunctionTest();
    DeleteTest();
    StringSizeTest();
    TestImportExport();
    MultiTreeTest();
    ExistAndEmptyTest();

    if (le_arg_NumArgs() == 1)
    {
        IncTestCount();
    }

    LE_INFO("---------- All Tests Complete in: %s ----------------------------------", TestRootDir);

    exit(EXIT_SUCCESS);
}
