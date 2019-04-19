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

#if LE_CONFIG_CUSTOM_OS
#define TEST_COUNT       (6 * 4)
#define REMOVE_TREE()    LE_TEST_INFO(                               \
                                "Remove config: %d",                 \
                                remove("d:/config/test_ConfigTree"))
#else
#define TEST_COUNT       (6 * 5 + 21)
#define REMOVE_TREE()    le_cfg_QuickDeleteNode(TEST_ROOT_NODE)
#endif

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
    char strBuffer[64];

    LE_TEST_OK(le_cfg_GetBool(txnRef, "bool", BOOL_DEFAULT_VALUE) == boolExpectedValue,
               "get bool %s", descriptionStr);
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
    LE_TEST_OK(le_cfg_GetBool(txnRef, "stem/bool", BOOL_DEFAULT_VALUE) == boolExpectedValue,
               "get stem/bool %s", descriptionStr);
}


#if LE_CONFIG_CUSTOM_OS
#define CheckConfigTreeIterated(txnRef, expectSuccess, descriptionStr)
#else /* !CONFIG_CUSTOM_OS */
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
#endif /* end !LE_CONFIG_CUSTOM_OS */

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
    le_cfg_SetBool(txnRef, "stem/bool", BOOL_VALUE);
}


COMPONENT_INIT
{
    le_cfg_IteratorRef_t txnRef;

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

#if !LE_CONFIG_CUSTOM_OS
    // But reading after canceling the write transaction doesn't work.
    txnRef = le_cfg_CreateReadTxn(TEST_ROOT_NODE);
    CheckConfigTree(txnRef, false, "after canceling transaction");
    le_cfg_CancelTxn(txnRef);
#endif /* end !LE_CONFIG_CUSTOM_OS */

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
    CheckConfigTreeIterated(txnRef, true, "in a new session");
    le_cfg_CancelTxn(txnRef);

    // Finally delete the data used in the test
    REMOVE_TREE();
    LE_INFO("============ test_ConfigTree PASSED =============");

    LE_TEST_EXIT;
}
