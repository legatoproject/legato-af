
#include "legato.h"
#include "configTypes.h"
#include "le_cfg_interface.h"




#define SMALL_STR_SIZE 5
#define STR_SIZE 513

#define TEST_NAME_SIZE 20



static char TestRootDir[STR_SIZE] = { 0 };




static const char* NodeTypeStr
(
    le_cfg_iteratorRef_t iterRef
)
{
    switch (le_cfg_GetNodeType(iterRef))
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

        case LE_CFG_TYPE_DENIED:
            return "**DENIED**";
    }

    return "unknown";
}




static void DumpTree(le_cfg_iteratorRef_t iterRef, size_t indent)
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

        le_cfg_GetNodeName(iterRef, strBuffer, STR_SIZE);
        le_cfg_nodeType_t type = le_cfg_GetNodeType(iterRef);

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
                le_cfg_GetString(iterRef, "", strBuffer, STR_SIZE);
                printf("%s\n", strBuffer);
                break;
        }
    }
    while (le_cfg_GoToNextSibling(iterRef) == LE_OK);
}




static void ClearTree()
{
    le_result_t result;

    LE_INFO("---- Clearing Out Current Tree -----------------------------------------------------");
    le_cfg_iteratorRef_t iterRef = le_cfg_CreateWriteTxn(TestRootDir);
    LE_FATAL_IF(iterRef == NULL, "Test: %s - Could not create iterator.", TestRootDir);

    DumpTree(iterRef, 0);
    le_cfg_DeleteNode(iterRef, "");

    result = le_cfg_CommitWrite(iterRef);
    LE_FATAL_IF(result != LE_OK,
                "Test: %s - Could not commit changes, result == %s.",
                TestRootDir,
                LE_RESULT_TXT(result));


    iterRef = le_cfg_CreateReadTxn(TestRootDir);
    DumpTree(iterRef, 0);
    le_cfg_DeleteIterator(iterRef);
}




static void QuickFunctionTest()
{
    le_result_t result;

    char pathBuffer[STR_SIZE] = { 0 };

    LE_INFO("---- Quick Function Test -----------------------------------------------------------");

    {
        snprintf(pathBuffer, STR_SIZE, "%s/quickFunctions/strVal", TestRootDir);

        char strBuffer[513] = { 0 };

        result = le_cfg_QuickGetString(pathBuffer, strBuffer, 513);
        LE_FATAL_IF(result != LE_NOT_PERMITTED,
                    "Test: %s - Test failure, result == %s.",
                    TestRootDir,
                    LE_RESULT_TXT(result));
        LE_DEBUG("<<< Get STRING <%s>", strBuffer);

        result = le_cfg_QuickSetString(pathBuffer,
                                       "Something funny is going on!");
        LE_FATAL_IF(result != LE_OK,
                    "Test: %s - Test failure, result == %s.",
                    TestRootDir,
                    LE_RESULT_TXT(result));

        result = le_cfg_QuickGetString(pathBuffer, strBuffer, 513);
        LE_FATAL_IF(result != LE_OK,
                    "Test: %s - Test failure, result == %s.",
                    TestRootDir,
                    LE_RESULT_TXT(result));
        LE_DEBUG("<<< Get STRING <%s>", strBuffer);
    }

    {
        snprintf(pathBuffer, STR_SIZE, "%s/quickFunctions/intVal", TestRootDir);

        int value;

        result = le_cfg_QuickGetInt(pathBuffer, &value);
        LE_FATAL_IF(result != LE_NOT_PERMITTED,
                    "Test: %s - Test failure, result == %s.",
                    TestRootDir,
                    LE_RESULT_TXT(result));
        LE_DEBUG("<<< Get INT <%d>", value);
        result = le_cfg_QuickSetInt(pathBuffer, 1111);
        LE_FATAL_IF(result != LE_OK, "Test: %s - Test failure, result == %s.",
                    TestRootDir,
                    LE_RESULT_TXT(result));

        le_cfg_QuickGetInt(pathBuffer, &value);
        LE_DEBUG("<<< Get INT <%d>", value);
    }

    {
        snprintf(pathBuffer, STR_SIZE, "%s/quickFunctions/floatVal", TestRootDir);

        double value;

        result = le_cfg_QuickGetFloat(pathBuffer, &value);
        LE_FATAL_IF(result != LE_NOT_PERMITTED,
                    "Test: %s - failure, result == %s.",
                    TestRootDir,
                    LE_RESULT_TXT(result));
        LE_DEBUG("<<< Get FLOAT <%f>", value);
        result = le_cfg_QuickSetFloat(pathBuffer, 1024.25);
        LE_FATAL_IF(result != LE_OK,
                    "Test: %s - failure, result == %s.",
                    TestRootDir,
                    LE_RESULT_TXT(result));

        result = le_cfg_QuickGetFloat(pathBuffer, &value);
        LE_FATAL_IF(result != LE_OK,
                    "Test: %s - failure, result == %s.",
                    TestRootDir,
                    LE_RESULT_TXT(result));
        LE_DEBUG("<<< Get FLOAT <%f>", value);
    }

    {
        snprintf(pathBuffer, STR_SIZE, "%s/quickFunctions/boolVal", TestRootDir);

        bool value;

        result = le_cfg_QuickGetBool(pathBuffer, &value);
        LE_FATAL_IF(result != LE_NOT_PERMITTED,
                    "Test: %s - failure, result == %s.",
                    TestRootDir,
                    LE_RESULT_TXT(result));
        LE_DEBUG("<<< Get BOOL <%d>", value);
        result = le_cfg_QuickSetBool(pathBuffer, true);
        LE_FATAL_IF(result != LE_OK,
                    "Test: %s - failure, result == %s.",
                    TestRootDir,
                    LE_RESULT_TXT(result));

        result = le_cfg_QuickGetBool(pathBuffer, &value);
        LE_FATAL_IF(result != LE_OK,
                    "Test: %s - failure, result == %s.",
                    TestRootDir,
                    LE_RESULT_TXT(result));
        LE_DEBUG("<<< Get BOOL <%d>", value);
    }
}




static void AllAsInt
(
    le_cfg_iteratorRef_t iterRef
)
{
    static char nameBuffer[STR_SIZE] = { 0 };

    le_cfg_GoToFirstChild(iterRef);

    do
    {
        int value;

        le_cfg_GetNodeName(iterRef, nameBuffer, STR_SIZE);
        value = le_cfg_GetInt(iterRef, "");

        LE_DEBUG("Read<%s>: %s: %d", NodeTypeStr(iterRef), nameBuffer, value);
    }
    while (le_cfg_GoToNextSibling(iterRef) == LE_OK);

    le_cfg_GoToParent(iterRef);
}




static void AllAsFloat
(
    le_cfg_iteratorRef_t iterRef
)
{
    static char nameBuffer[STR_SIZE] = { 0 };

    le_cfg_GoToFirstChild(iterRef);

    do
    {
        float value;

        le_cfg_GetNodeName(iterRef, nameBuffer, STR_SIZE);
        value = le_cfg_GetFloat(iterRef, "");

        LE_DEBUG("Read<%s>: %s: %f", NodeTypeStr(iterRef), nameBuffer, value);
    }
    while (le_cfg_GoToNextSibling(iterRef) == LE_OK);

    le_cfg_GoToParent(iterRef);
}




static void AllAsBool
(
    le_cfg_iteratorRef_t iterRef
)
{
    static char nameBuffer[STR_SIZE] = { 0 };

    le_cfg_GoToFirstChild(iterRef);

    do
    {
        bool value;

        le_cfg_GetNodeName(iterRef, nameBuffer, STR_SIZE);
        value = le_cfg_GetBool(iterRef, "");

        LE_DEBUG("Read<%s>: %s: %d", NodeTypeStr(iterRef), nameBuffer, value);
    }
    while (le_cfg_GoToNextSibling(iterRef) == LE_OK);

    le_cfg_GoToParent(iterRef);
}




static void TestValueWrite
(
    le_cfg_iteratorRef_t iterRef,
    const char* namePtr,
    const char* valuePtr,
    le_cfg_nodeType_t exptectedType
)
{
    static char strBuffer[STR_SIZE] = { 0 };

    LE_FATAL_IF(le_cfg_IsEmpty(iterRef, namePtr) != true,
                "Test: %s - %s is non-empty when it shouldn't be.",
                TestRootDir,
                namePtr);

    le_cfg_SetString(iterRef, namePtr, valuePtr);
    le_cfg_GetString(iterRef, namePtr, strBuffer, STR_SIZE);

    LE_DEBUG("Wrote: '%s' to %s, got back '%s'.", valuePtr, namePtr, strBuffer);

    LE_FATAL_IF(strncmp(strBuffer, valuePtr, STR_SIZE) != 0,
                "Test: %s - Did not get back what was written.  Expected '%s', got, '%s'.",
                TestRootDir,
                valuePtr,
                strBuffer);

    le_cfg_GoToNode(iterRef, namePtr);
    LE_FATAL_IF(le_cfg_GetNodeType(iterRef) != exptectedType,
                "Test: %s - Did not get expected type.  Got %s instead.",
                TestRootDir,
                NodeTypeStr(iterRef));

    le_cfg_GoToNode(iterRef, "..");
}




static void TestValueTypes()
{
    LE_INFO("---- Testing Value Type Guessing ---------------------------------------------------");

    char pathBuffer[STR_SIZE] = { 0 };
    snprintf(pathBuffer, STR_SIZE, "%s/valueTypes/", TestRootDir);

    le_cfg_iteratorRef_t iterRef = le_cfg_CreateWriteTxn(pathBuffer);

    TestValueWrite(iterRef, "value0", "false",                            LE_CFG_TYPE_BOOL);
    TestValueWrite(iterRef, "value1", "true",                             LE_CFG_TYPE_BOOL);
    TestValueWrite(iterRef, "value2", "1024",                             LE_CFG_TYPE_INT);
    TestValueWrite(iterRef, "value3", "10.24",                            LE_CFG_TYPE_FLOAT);
    TestValueWrite(iterRef, "value4", "-1024",                            LE_CFG_TYPE_INT);
    TestValueWrite(iterRef, "value5", "-10.24",                           LE_CFG_TYPE_FLOAT);
    TestValueWrite(iterRef, "value6", "Something wicked this way comes.", LE_CFG_TYPE_STRING);
    TestValueWrite(iterRef, "value7", "5.525e-5",                         LE_CFG_TYPE_FLOAT);
    TestValueWrite(iterRef, "value8", "",                                 LE_CFG_TYPE_EMPTY);

    AllAsInt(iterRef);
    AllAsFloat(iterRef);
    AllAsBool(iterRef);

    le_cfg_GoToNode(iterRef, pathBuffer);
    DumpTree(iterRef, 0);

    le_cfg_DeleteIterator(iterRef);
}




static void TestValue
(
    le_cfg_iteratorRef_t iterRef,
    const char* valueNamePtr,
    const char* expectedValue
)
{
    static char strBuffer[STR_SIZE] = { 0 };

    le_cfg_GetString(iterRef, valueNamePtr, strBuffer, STR_SIZE);
    LE_FATAL_IF(strncmp(strBuffer, expectedValue, STR_SIZE) != 0,
                "Test: %s - Expected '%s' but got '%s' instead.",
                TestRootDir,
                valueNamePtr,
                expectedValue);

}




static void DeleteTest()
{
    le_result_t result;

    static char pathBuffer[STR_SIZE] = { 0 };

    snprintf(pathBuffer, STR_SIZE, "%s/deleteTest/", TestRootDir);

    le_cfg_iteratorRef_t iterRef = le_cfg_CreateWriteTxn(pathBuffer);

    le_cfg_SetString(iterRef, "valueA", "aNewValue");
    le_cfg_SetString(iterRef, "valueB", "aNewValue");
    le_cfg_SetString(iterRef, "valueC", "aNewValue");

    TestValue(iterRef, "valueA", "aNewValue");
    TestValue(iterRef, "valueB", "aNewValue");
    TestValue(iterRef, "valueC", "aNewValue");

    result = le_cfg_CommitWrite(iterRef);
    LE_FATAL_IF(result != LE_OK,
                "Test: %s - Could not commit write.  Reason = %s",
                TestRootDir,
                LE_RESULT_TXT(result));



    iterRef = le_cfg_CreateWriteTxn(pathBuffer);

    le_cfg_DeleteNode(iterRef, "valueB");

    TestValue(iterRef, "valueA", "aNewValue");
    TestValue(iterRef, "valueB", "");
    TestValue(iterRef, "valueC", "aNewValue");

    result = le_cfg_CommitWrite(iterRef);
    LE_FATAL_IF(result != LE_OK,
                "Test: %s - Could not commit write.  Reason = %s",
                TestRootDir,
                LE_RESULT_TXT(result));



    iterRef = le_cfg_CreateReadTxn(pathBuffer);

    TestValue(iterRef, "valueA", "aNewValue");
    TestValue(iterRef, "valueB", "");
    TestValue(iterRef, "valueC", "aNewValue");

    DumpTree(iterRef, 0);

    le_cfg_DeleteIterator(iterRef);
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


    result = le_cfg_QuickSetString(pathBuffer,
                                   "This is a bigger string than may be usual for this test.");
    LE_FATAL_IF(result != LE_OK,
                "Test: %s - Failure, result == %s.",
                TestRootDir,
                LE_RESULT_TXT(result));


    static char buffer[STR_SIZE];

    le_cfg_iteratorRef_t iterRef = le_cfg_CreateReadTxn(pathBuffer);


    result = le_cfg_GetPath(iterRef, buffer, SMALL_STR_SIZE);
    LE_FATAL_IF(result != LE_OVERFLOW,
                "Test: %s - The buffer should have been too small.",
                TestRootDir);
    LE_FATAL_IF(strcmp(buffer, smallPathBuffer) == 0,
                "Test: %s - Unexpected value returned, %s",
                TestRootDir,
                buffer);

    result = le_cfg_GetParentPath(iterRef, buffer, SMALL_STR_SIZE);
    LE_FATAL_IF(result != LE_OVERFLOW,
                "Test: %s - The buffer should have been too small.",
                TestRootDir);
    LE_FATAL_IF(strcmp(buffer, smallParentPathBuffer) == 0,
                "Test: %s - Unexpected value returned, %s",
                TestRootDir,
                buffer);

    result = le_cfg_GetString(iterRef, "", buffer, SMALL_STR_SIZE);
    LE_FATAL_IF(result != LE_OVERFLOW,
                "Test: %s - The buffer should have been too small.",
                TestRootDir);
    LE_FATAL_IF(strcmp(buffer, "This ") == 0,
                "Test: %s - Unexpected value returned, %s",
                TestRootDir,
                buffer);


    result = le_cfg_GetPath(iterRef, buffer, STR_SIZE);
    LE_FATAL_IF(result != LE_OK,
                "Test: %s - The buffer should have been big enough.",
                TestRootDir);
    LE_FATAL_IF(strcmp(buffer, pathBuffer) != 0,
                "Test: %s - Unexpected value returned, %s",
                TestRootDir,
                buffer);

    result = le_cfg_GetParentPath(iterRef, buffer, STR_SIZE);
    LE_FATAL_IF(result != LE_OK, "Test: %s - The buffer should have been big enough.", TestRootDir);
    LE_FATAL_IF(strcmp(buffer, parentPathBuffer) != 0,
                "Test: %s - Unexpected value returned, %s",
                TestRootDir,
                buffer);

    result = le_cfg_GetString(iterRef, "", buffer, STR_SIZE);
    LE_FATAL_IF(result != LE_OK, "Test: %s - The buffer should have been big enough.", TestRootDir);
    LE_FATAL_IF(strcmp(buffer, "This is a bigger string than may be usual for this test.") != 0,
                "Test: %s - Unexpected value returned, %s",
                TestRootDir,
                buffer);


    le_cfg_DeleteIterator(iterRef);


    result = le_cfg_QuickGetString(pathBuffer, buffer, SMALL_STR_SIZE);
    LE_FATAL_IF(result != LE_OVERFLOW,
                "Test: %s - The buffer should have been too small.",
                TestRootDir);
    LE_FATAL_IF(strcmp(buffer, "This ") == 0,
                "Test: %s - Unexpected value returned, %s",
                TestRootDir,
                buffer);

    result = le_cfg_QuickGetString(pathBuffer, buffer, STR_SIZE);
    LE_FATAL_IF(result != LE_OK,
                "Test: %s - The buffer should have been big enough.",
                TestRootDir);
    LE_FATAL_IF(strcmp(buffer, "This is a bigger string than may be usual for this test.") != 0,
                "Test: %s - Unexpected value returned, %s",
                TestRootDir,
                buffer);
}



/*static void TestValues
(
    le_cfg_iteratorRef_t iterRef,
    bool shouldExist
)
{
    if (shouldExist == false)
    {
        LE_FATAL_IF(le_cfg_IsEmpty(iterRef, ".") == false, "Base node should be empty.");
    }
    else
    {
        char buffer[20] = { 0 };

        LE_FATAL_IF(le_cfg_IsEmpty(iterRef, "anEmptyValue") != true, "Value not empty.");
        LE_FATAL_IF(le_cfg_GetInt(iterRef, "anIntValue") != 2248, "Int value not as expected");
        //LE_FATAL_IF(le_cfg_GetFloat(iterRef, "aSubNode/aFloatValue") != 22.48, "Bad float value.");
        LE_FATAL_IF(le_cfg_GetBool(iterRef, "aBoolValue") != true, "Bad bool value.");

        le_result_t result = le_cfg_GetString(iterRef, "aSubNode/aStringValue", buffer, 20);
        LE_FATAL_IF(result != LE_OK,
                    "Problem with get string, result = %s.",
                    LE_RESULT_TXT(result));

        LE_FATAL_IF(strcmp(buffer, "I am some text!") != 0,
                    "Did not read back expected value, found \'%s\' instead.",
                    buffer);
    }
}*/




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

    le_cfg_iteratorRef_t iterRef = le_cfg_CreateWriteTxn("/suite/fileTests");
    le_result_t result = le_cfg_ImportTree(iterRef, filePath, "");

    LE_FATAL_IF(result != LE_OK,
                "Config file import file.  Could not read from file: %s.  Result = %s",
                filePath,
                LE_RESULT_TXT(result));

    TestValues(iterRef, true);

    result = le_cfg_CommitWrite(iterRef);
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

    le_cfg_DeleteIterator(iterRef);

    TestValues(iterRef, true);*/


    // TODO: Test export...
}




static void MultiTreeTest()
{
    char strBuffer[STR_SIZE] = { 0 };

    static char pathBuffer[STR_SIZE] = { 0 };
    snprintf(pathBuffer, STR_SIZE, "foo:/%s/quickMultiTreeTest/value", TestRootDir);


    le_result_t result = le_cfg_QuickSetString(pathBuffer, "hello world");
    LE_FATAL_IF(result != LE_OK,
                "Test: %s - Could not write value to tree, foo.  Reason = %s",
                TestRootDir,
                LE_RESULT_TXT(result));


    result = le_cfg_QuickGetString(pathBuffer, strBuffer, STR_SIZE);
    LE_FATAL_IF(result != LE_OK,
                "Test: %s - Could not read value from tree, foo.  Reason = %s",
                TestRootDir,
                LE_RESULT_TXT(result));
    LE_FATAL_IF(strcmp(strBuffer, "hello world") != 0,
                "Test: %s - Did not get expected value from tree foo.  Got '%s'.",
                TestRootDir,
                strBuffer);
}




static void IncTestCount
(
    void
)
{
    le_cfg_iteratorRef_t iterRef = le_cfg_CreateWriteTxn("/configTest/testCount");

    int count = le_cfg_GetInt(iterRef, "");
    count++;
    le_cfg_SetInt(iterRef, "", count);

    le_cfg_CommitWrite(iterRef);
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
    TestValueTypes();
    DeleteTest();
    StringSizeTest();
    TestImportExport();
    MultiTreeTest();

    if (le_arg_NumArgs() == 1)
    {
        IncTestCount();
    }

    LE_INFO("---------- All Tests Complete in: %s ----------------------------------", TestRootDir);

    exit(EXIT_SUCCESS);
}
