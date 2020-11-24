/**
 * testConfigTree.c
 *
 * Simple config tree test -- tests read/write without reboot.
 */

#include "legato.h"
#include "interfaces.h"

// Root of config tree to test
#define TEST_ROOT_NODE "/testConfigTree"

// Test values
#define BOOL_VALUE         true
#define BOOL_DEFAULT_VALUE false

#define FLOAT_VALUE          3.14f
#define FLOAT_DEFAULT_VALUE  137.036f
#define FLOAT_EPSILON        0.001f

#define INT_VALUE            42
#define INT_DEFAULT_VALUE    56

#define STRING_VALUE         "hello"
#define STRING_DEFAULT_VALUE "goodbye"

#define BINARY_VALUE         "0123456789abcde"
#define BINARY_SIZE          16
#define BINARY_DEFAULT_VALUE "default"
#define BINARY_DEFAULT_SIZE  8

#define MAX_CFG_TREE_SIZE           8192
#define WRITE_TEST_BENCHMARK_MS     2000
#define WRITE_TEST_ITERATIONS       100
#define TEST_COUNT                  ((2*WRITE_TEST_ITERATIONS) + 74)

#if LE_CONFIG_RTOS
# if LE_CONFIG_TARGET_GILL
#   define REMOVE_TREE()    LE_TEST_INFO("Remove config: %d", le_dir_RemoveRecursive("/config/"))
# elif LE_CONFIG_TARGET_HL78
#   define REMOVE_TREE()    LE_TEST_INFO("Remove config: %d", remove("d:/config/test_ConfigTree"))
# else
#   error "Unknown RTOS Configuration"
# endif
#else
#define REMOVE_TREE()    le_cfg_QuickDeleteNode(TEST_ROOT_NODE)
#endif // LE_CONFIG_RTOS

static uint8_t buf[MAX_CFG_TREE_SIZE] = {0};

// -------------------------------------------------------------------------------------------------
/**
 *  Loop through writing binary arrays
 */
// -------------------------------------------------------------------------------------------------
static void WriteBinaryTest(int num, char* basePath, uint8_t *data, size_t len)
{
    struct timeval tv;
    uint64_t utcMilliSecBeforeTest;
    uint64_t utcMilliSecAfterTest;
    uint64_t timeElapsed;

    LE_INFO("+++ Start a Write binary test on %s", basePath);
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(basePath);
    LE_ASSERT(iterRef != NULL);

    gettimeofday(&tv, NULL);
    utcMilliSecBeforeTest = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;

    for (int i = 0; i < num; i++)
    {
        char path[100];
        sprintf(path, "Binary-%d", i);
        le_cfg_SetBinary(iterRef, path, data, len);
    }

    le_cfg_CommitTxn(iterRef);

    gettimeofday(&tv, NULL);
    utcMilliSecAfterTest = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;

    timeElapsed = utcMilliSecAfterTest - utcMilliSecBeforeTest;
    LE_TEST_INFO("+++ Time (ms) to write %d entries of %d bytes = %" PRIu64,
            num, len, timeElapsed);
    LE_TEST_OK(timeElapsed < WRITE_TEST_BENCHMARK_MS, "Write performance test");
}

// -------------------------------------------------------------------------------------------------
/**
 *  Loop through reading binary arrays
 */
// -------------------------------------------------------------------------------------------------
static void ReadBinaryTest(int num, char* basePath, uint8_t *data, size_t len)
{
    struct timeval tv;
    uint64_t utcMilliSecBeforeTest;
    uint64_t utcMilliSecAfterTest;
    uint64_t timeElapsed;
    le_result_t result;
    size_t originalLen = len;
    char path[100];

    LE_TEST_INFO("+++ Start a Read binary test on %s", basePath);
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(basePath);
    LE_ASSERT(iterRef != NULL);

    gettimeofday(&tv, NULL);
    utcMilliSecBeforeTest = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;

    for (int i = 0; i < num; i++)
    {
        sprintf(path, "Binary-%d", i);
        result = le_cfg_GetBinary(iterRef, path, buf, &len, (uint8_t *) "no_good", 7);

        LE_TEST_OK(result == LE_OK, "Getting %d binary from %s, result : %s",
                    len, path, LE_RESULT_TXT(result));

        LE_TEST_OK(len == originalLen, "Requested %d bytes and read %d bytes for %s",
                    originalLen, len, path);
    }

    gettimeofday(&tv, NULL);
    utcMilliSecAfterTest = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;

    timeElapsed = utcMilliSecAfterTest - utcMilliSecBeforeTest;
    LE_TEST_INFO("+++ Time (ms) to read %d entries of %d bytes = %" PRIu64,
            num, len, timeElapsed);
    LE_TEST_OK(0 == memcmp(buf, data, len), "Compare binary test");

    len = 10;
    result = le_cfg_GetBinary(iterRef, path, buf, &len, (uint8_t *) "no_good", 7);
    LE_TEST_OK(result == LE_OVERFLOW,
        "Ensure LE_OVERFLOW is returned for too small of a buffer. Result = %s",
        LE_RESULT_TXT(result));

    le_cfg_CancelTxn(iterRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Check all items we expect to write in a config tree are present (or not present)
 */
//--------------------------------------------------------------------------------------------------
static void CheckConfigTree
(
    le_cfg_IteratorRef_t txnRef,   ///< [IN] Config tree iterator for base of tree
    bool expectSuccess,            ///< [IN] Should reads succeed
    const char* descriptionStr     ///< [IN] description of test
)
{
    bool boolExpectedValue       = (expectSuccess?BOOL_VALUE  :BOOL_DEFAULT_VALUE  );
    float floatExpectedValue     = (expectSuccess?FLOAT_VALUE :FLOAT_DEFAULT_VALUE );
    int intExpectedValue         = (expectSuccess?INT_VALUE   :INT_DEFAULT_VALUE   );
    const char* strExpectedValue = (expectSuccess?STRING_VALUE:STRING_DEFAULT_VALUE);
    const uint8_t* binExpectedValue = (expectSuccess?(uint8_t *) BINARY_VALUE :
                                                     (uint8_t *) BINARY_DEFAULT_VALUE);
    size_t binExpectedSize = (expectSuccess? BINARY_SIZE :
                                             BINARY_DEFAULT_SIZE);
    char strBuffer[64];
    uint8_t binBuffer[64];

    LE_TEST_OK(le_cfg_GetBool(txnRef, "bool", BOOL_DEFAULT_VALUE) == boolExpectedValue,
               "get bool %s", descriptionStr);
    LE_TEST_OK(le_cfg_GetFloat(txnRef, "bool", FLOAT_DEFAULT_VALUE) == FLOAT_DEFAULT_VALUE,
               "use the wrong API %s", descriptionStr);
    LE_TEST_OK(fabs(le_cfg_GetFloat(txnRef, "float", FLOAT_DEFAULT_VALUE) - floatExpectedValue)
               < FLOAT_EPSILON,
               "get float %s", descriptionStr);
    LE_TEST_OK(le_cfg_GetInt(txnRef, "int", INT_DEFAULT_VALUE) == intExpectedValue,
               "get int %s", descriptionStr);
    LE_TEST_OK(le_cfg_GetString(txnRef, "string",
                                strBuffer, sizeof(strBuffer),
                                STRING_DEFAULT_VALUE) == LE_OK,
               "get string result %s", descriptionStr);
    LE_TEST_OK(strcmp(strExpectedValue, strBuffer) == 0,
               "get string value %s", descriptionStr);
    size_t binSize = sizeof(binBuffer);
    le_result_t rc;
    rc = le_cfg_GetBinary(txnRef, "binary",
                                (uint8_t*) binBuffer, &binSize,
                                (uint8_t*) BINARY_DEFAULT_VALUE, BINARY_DEFAULT_SIZE);
    LE_TEST_OK(LE_OK == rc,
               "get binary result %s", descriptionStr);
    LE_TEST_OK(memcmp(binExpectedValue, binBuffer, binExpectedSize) == 0,
               "get binary value %s", descriptionStr);
    LE_TEST_OK(le_cfg_GetBool(txnRef, "stem/bool", BOOL_DEFAULT_VALUE) == boolExpectedValue,
               "get stem/bool %s", descriptionStr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Check all items we expect to write in a config tree are present (or not present), using
 * le_cfg_GoToFirstChild()/le_cfg_GoToNextSibling() API.
 *
 * Note: does not check values.
 */
//--------------------------------------------------------------------------------------------------
static void CheckConfigTreeIterated
(
    le_cfg_IteratorRef_t txnRef,   ///< [IN] Config tree iterator for base of tree
    bool expectSuccess,            ///< [IN] Should reads succeed
    const char* descriptionStr     ///< [IN] description of test
)
{
    char nodeName[64];
    int boolNodes = 0,
        floatNodes = 0,
        intNodes = 0,
        stringNodes = 0,
        stemNodes = 0;
    bool boolNodeNameOk = true,
        floatNodeNameOk = true,
        intNodeNameOk = true,
        stringNodeNameOk = true,
        stemNodeNameOk = true;
    int i;

    LE_TEST_OK(le_cfg_GoToFirstChild(txnRef) == LE_OK,
               "go to first child");
    // Expect to see exactly 5 children
    for (i = 0; i < 5; ++i)
    {
        LE_TEST_OK(le_cfg_GetNodeName(txnRef, "", nodeName, sizeof(nodeName)) == LE_OK,
                   "get node %d name", i);
        switch (le_cfg_GetNodeType(txnRef, ""))
        {
            case LE_CFG_TYPE_STRING:
                ++stringNodes;
                stringNodeNameOk = stringNodeNameOk && (strcmp(nodeName, "string") == 0);
                break;
            case LE_CFG_TYPE_BOOL:
                ++boolNodes;
                boolNodeNameOk = boolNodeNameOk && (strcmp(nodeName, "bool") == 0);
                break;
            case LE_CFG_TYPE_INT:
                ++intNodes;
                intNodeNameOk = intNodeNameOk && (strcmp(nodeName, "int") == 0);
                break;
            case LE_CFG_TYPE_FLOAT:
                ++floatNodes;
                floatNodeNameOk = floatNodeNameOk && (strcmp(nodeName, "float") == 0);
                break;
            case LE_CFG_TYPE_STEM:
                ++stemNodes;
                stemNodeNameOk = stemNodeNameOk && (strcmp(nodeName, "stem") == 0);
                break;
            default:
                LE_TEST_FATAL("Unexpected node type");
        }
        LE_TEST_OK(le_cfg_GoToNextSibling(txnRef) == ((i == 4)?LE_NOT_FOUND:LE_OK),
                   "get next sibling of node %d", i);
    }
    LE_TEST_OK(1 == boolNodes, "one bool node");
    LE_TEST_OK(boolNodeNameOk, "correct name for bool node");
    LE_TEST_OK(1 == floatNodes, "one float node");
    LE_TEST_OK(floatNodeNameOk, "correct name for float node");
    LE_TEST_OK(1 == intNodes, "one int node");
    LE_TEST_OK(intNodeNameOk, "correct name for int node");
    LE_TEST_OK(1 == stringNodes, "one string node");
    LE_TEST_OK(stringNodeNameOk, "correct name for string node");
    LE_TEST_OK(1 == stemNodes, "one stem node");
    LE_TEST_OK(stemNodeNameOk, "correct name for stem node");
}

//--------------------------------------------------------------------------------------------------
/**
 * Write some values to a tree.
 *
 * Performs no checks -- check is done by reading back the value
 */
//--------------------------------------------------------------------------------------------------
static void WriteConfigTree
(
    le_cfg_IteratorRef_t txnRef   ///< [IN] Config tree iterator for base of tree
)
{
    le_cfg_SetBool(txnRef, "bool", BOOL_VALUE);
    le_cfg_SetFloat(txnRef, "float", FLOAT_VALUE);
    le_cfg_SetInt(txnRef, "int", INT_VALUE);
    le_cfg_SetString(txnRef, "string", STRING_VALUE);
    le_cfg_SetBinary(txnRef, "binary", (uint8_t *) BINARY_VALUE, BINARY_SIZE);
    le_cfg_SetBool(txnRef, "stem/bool", BOOL_VALUE);
}

//--------------------------------------------------------------------------------------------------
/**
 * Create and remove nodes
 *
 * API's tested:
 *  - le_cfg_NodeExists()
 *  - le_cfg_IsEmpty()
 *  - le_cfg_DeleteNode()
 */
//--------------------------------------------------------------------------------------------------
void CreateDeleteNodeTest()
{
    const char path1[] = "CreateTest1:/dir1/dir2";
    const char fullPath1[] = "/config/CreateTest1/dir1/dir2";
    const char path2[] = "CreateTest1:/dir1";
    const char fullPath2[] = "/config/CreateTest1/dir1";
    const char path3[] = "/";
    const char fullPath3[] = "/config/configTreeSecStoreTest";
    const char emptyNode[] = "EmptyNode";

    const char basePath[] = "DoubleTest:/";

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(path1);
    LE_TEST_OK(!le_cfg_NodeExists(iterRef, emptyNode),
                "%s/%s doesn't exist test", fullPath1, emptyNode);
    le_cfg_SetEmpty(iterRef, emptyNode);
    LE_TEST_OK(le_cfg_NodeExists(iterRef, emptyNode), "%s/%s exists test", fullPath1, emptyNode);
    LE_TEST_OK(le_cfg_IsEmpty(iterRef, emptyNode), "%s/%s is empty test", fullPath1, emptyNode);

    // Set Integer in what was an empty node
    le_cfg_SetInt(iterRef, emptyNode, 1);
    LE_TEST_OK(!le_cfg_IsEmpty(iterRef, emptyNode), "%s/%s is no longer empty test",
        fullPath1, emptyNode);
    le_cfg_SetEmpty(iterRef, emptyNode);
    LE_TEST_OK(le_cfg_NodeExists(iterRef, emptyNode), "%s/%s exists test", fullPath1, emptyNode);
    LE_TEST_OK(le_cfg_IsEmpty(iterRef, emptyNode), "%s/%s is empty test", fullPath1, emptyNode);

    le_cfg_CommitTxn(iterRef);

    iterRef = le_cfg_CreateWriteTxn(path2);
    le_cfg_SetEmpty(iterRef, emptyNode);
    LE_TEST_OK(le_cfg_NodeExists(iterRef, emptyNode), "%s/%s exists test", fullPath2, emptyNode);
    LE_TEST_OK(le_cfg_IsEmpty(iterRef, emptyNode), "%s/%s is empty test", fullPath2, emptyNode);
    LE_TEST_OK(le_cfg_NodeExists(iterRef, "dir2"), "Check stem exists");
    LE_TEST_OK(le_cfg_IsEmpty(iterRef, "dir2"), "Check stem is empty");
    le_cfg_SetInt(iterRef, "dir2", 1);
    LE_TEST_OK(!le_cfg_IsEmpty(iterRef, "dir2"), "Check stem is no longer empty");
    le_cfg_DeleteNode(iterRef, "dir2");
    LE_TEST_OK(!le_cfg_NodeExists(iterRef, "dir2"), "Check stem is deleted");
    le_cfg_CommitTxn(iterRef);

    iterRef = le_cfg_CreateWriteTxn(path3);
    le_cfg_SetEmpty(iterRef, emptyNode);
    LE_TEST_OK(le_cfg_NodeExists(iterRef, emptyNode), "%s/%s exists test", fullPath3, emptyNode);
    le_cfg_DeleteNode(iterRef, emptyNode);
    LE_TEST_OK(!le_cfg_NodeExists(iterRef, emptyNode),
        "Check %s/%s is deleted", fullPath3, emptyNode);
    le_cfg_CommitTxn(iterRef);

    iterRef = le_cfg_CreateWriteTxn(basePath);
    le_cfg_DeleteNode(iterRef, "/");
    LE_TEST_OK(!le_cfg_NodeExists(iterRef, "/"), "Check %s is deleted", basePath);
    le_cfg_CommitTxn(iterRef);
}

COMPONENT_INIT
{
    le_cfg_IteratorRef_t txnRef;
    static uint8_t randBuf[MAX_CFG_TREE_SIZE];

    LE_TEST_PLAN(TEST_COUNT);

    LE_INFO("********** Start test_ConfigTree Test ***********");
    // First delete any left over test tree,
    REMOVE_TREE();

    // and make sure the test tree doesn't exist after deleting it
    txnRef = le_cfg_CreateReadTxn(TEST_ROOT_NODE);
    CheckConfigTree(txnRef, false, "read before creating data");
    le_cfg_CancelTxn(txnRef);

    // Check writing in a write transaction works, and is immediately visible
    txnRef = le_cfg_CreateWriteTxn(TEST_ROOT_NODE);
    WriteConfigTree(txnRef);
    CheckConfigTree(txnRef, true, "after write in a write transaction");
    le_cfg_CancelTxn(txnRef);

    // But reading after cancelling the write transaction doesn't work (for Linux).
    LE_TEST_BEGIN_SKIP(LE_CONFIG_IS_ENABLED(LE_CONFIG_RTOS), 7);
    txnRef = le_cfg_CreateReadTxn(TEST_ROOT_NODE);
    CheckConfigTree(txnRef, false, "after canceling transaction");
    le_cfg_CancelTxn(txnRef);
    LE_TEST_END_SKIP();

    // Now write and commit a transaction
    txnRef = le_cfg_CreateWriteTxn(TEST_ROOT_NODE);
    WriteConfigTree(txnRef);
    le_cfg_CommitTxn(txnRef);

    // And verify it's visible in a new transaction
    txnRef = le_cfg_CreateReadTxn(TEST_ROOT_NODE);
    CheckConfigTree(txnRef, true, "after committing transaction");
    le_cfg_CancelTxn(txnRef);

    le_cfg_DisconnectService();

    le_cfg_ConnectService();

    // And continues to be visible even after disconnecting & reconnecting to config tree.
    // Also test iterator functions (GoToFirstChild/GoToNextSibling, etc.)
    txnRef = le_cfg_CreateReadTxn(TEST_ROOT_NODE);
    CheckConfigTree(txnRef, true, "in a new session");

    // Iterating through the config tree is not yet supported
    LE_TEST_BEGIN_SKIP(LE_CONFIG_IS_ENABLED(LE_CONFIG_RTOS), 21);
    CheckConfigTreeIterated(txnRef, true, "in a new session");
    le_cfg_CancelTxn(txnRef);
    LE_TEST_END_SKIP();

    REMOVE_TREE();

    // Read and write binary data to config tree and output how long it takes
    le_rand_GetBuffer(randBuf, sizeof(randBuf));
    LE_TEST_BEGIN_SKIP(LE_CONFIG_IS_ENABLED(LE_CONFIG_TARGET_HL78), 2*WRITE_TEST_ITERATIONS + 2);
    WriteBinaryTest(WRITE_TEST_ITERATIONS, "/binary", (uint8_t *) randBuf, 1024);
    ReadBinaryTest(WRITE_TEST_ITERATIONS, "/binary", (uint8_t *) randBuf, 1024);
    LE_TEST_END_SKIP();

    // Test creating and deleting nodes (HL78 does not currently support deleting nodes)
    LE_TEST_BEGIN_SKIP(LE_CONFIG_IS_ENABLED(LE_CONFIG_TARGET_HL78), 15);
    CreateDeleteNodeTest();
    LE_TEST_END_SKIP();

    // Finally delete the data used in the test
    REMOVE_TREE();
    LE_TEST_INFO("============ test_ConfigTree FINISHED =============");

    LE_TEST_EXIT;
}
