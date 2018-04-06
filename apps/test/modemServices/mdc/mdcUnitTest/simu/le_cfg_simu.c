/**
 * @file le_cfg_simu.c
 *
 * Simulation implementation of the config Tree API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "mdmCfgEntries.h"

//--------------------------------------------------------------------------------------------------
/**
 * Reference to a tree iterator object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_cfg_Iterator* le_cfg_IteratorRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Simulated config tree values.
 */
//--------------------------------------------------------------------------------------------------
static bool BytesCounting = true;
static double RxBytes = 0;
static double TxBytes = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Config tree iterator reference
 */
//--------------------------------------------------------------------------------------------------
static le_cfg_IteratorRef_t SimuIteratorRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Length of the strings used by this API.
 */
//--------------------------------------------------------------------------------------------------
#define LE_CFG_STR_LEN          511

//--------------------------------------------------------------------------------------------------
/**
 * Connect the current client thread to the service providing this API (STUBBED FUNCTION)
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_ConnectService
(
    void
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a read transaction and open a new iterator for traversing the config tree.
 *
 * @note This action creates a read lock on the given tree, which will start a read-timeout.
 *        Once the read timeout expires, all active read iterators on that tree will be
 *        expired and the clients will be killed.
 *
 * @note A tree transaction is global to that tree; a long-held read transaction will block other
 *        user's write transactions from being committed.
 *
 * @return This will return a newly created iterator reference.
 */
//--------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t le_cfg_CreateReadTxn
(
    const char* basePathPtr    ///< [IN] Path to the location to create the new iterator
)
{
    return (le_cfg_IteratorRef_t)SimuIteratorRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a write transaction and open a new iterator for both reading and writing.
 *
 * @note This action creates a write transaction. If the app holds the iterator for
 *        longer than the configured write transaction timeout, the iterator will cancel the
 *        transaction. Other reads will fail to return data, and all writes will be thrown
 *        away.
 *
 * @note A tree transaction is global to that tree; a long-held write transaction will block
 *       other user's write transactions from being started. Other trees in the system
 *       won't be affected.
 *
 * @return This will return a newly created iterator reference.
 */
//--------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t le_cfg_CreateWriteTxn
(
    const char* basePathPtr    ///< [IN] Path to the location to create the new iterator
)
{
    return (le_cfg_IteratorRef_t)SimuIteratorRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close the write iterator and commit the write transaction. This updates the config tree
 * with all of the writes that occurred using the iterator.
 *
 * @note This operation will also delete the iterator object.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_CommitTxn
(
    le_cfg_IteratorRef_t iteratorRef    ///< [IN] Iterator object to commit
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Close and free the given iterator object. If the iterator is a write iterator, the transaction
 * will be canceled. If the iterator is a read iterator, the transaction will be closed.
 *
 * @note This operation will also delete the iterator object.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_CancelTxn
(
    le_cfg_IteratorRef_t iteratorRef    ///< [IN] Iterator object to close
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Set a simulated 64-bit floating point value for a specific node.
 */
//--------------------------------------------------------------------------------------------------
void le_cfgSimu_SetFloatNodeValue
(
    le_cfg_IteratorRef_t iteratorRef,   ///< [IN] Iterator to use as a basis for the transaction
    const char* pathPtr,                ///< [IN] Path to the target node. Can be an absolute path,
                                        ///<      or a path relative from the iterator's current
                                        ///<      position
    double value                        ///< [IN] Value to write
)
{
    SimuIteratorRef = iteratorRef;

    if (0 == strncmp(pathPtr, CFG_NODE_RX_BYTES, strlen(CFG_NODE_RX_BYTES)))
    {
        RxBytes = value;
    }
    else if (0 == strncmp(pathPtr, CFG_NODE_TX_BYTES, strlen(CFG_NODE_TX_BYTES)))
    {
        TxBytes = value;
    }
    else
    {
        LE_ERROR("Unsupported path '%s'", pathPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set a simulated boolean value for a specific node.
 */
//--------------------------------------------------------------------------------------------------
void le_cfgSimu_SetBoolNodeValue
(
    le_cfg_IteratorRef_t iteratorRef,   ///< [IN] Iterator to use as a basis for the transaction
    const char* pathPtr,                ///< [IN] Path to the target node. Can be an absolute path,
                                        ///<      or a path relative from the iterator's current
                                        ///<      position
    bool value                          ///< [IN] Value to write
)
{
    SimuIteratorRef = iteratorRef;

    if (0 == strncmp(pathPtr, CFG_NODE_COUNTING, strlen(CFG_NODE_COUNTING)))
    {
        BytesCounting = value;
    }
    else
    {
        LE_ERROR("Unsupported path '%s'", pathPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a 64-bit floating point value from the configuration tree.
 *
 * If the value is an integer, the value will be promoted to a float. Otherwise, if the
 * underlying value is not a float or integer, the default value will be returned.
 *
 * If the path is empty, the iterator's current node will be read.
 */
//--------------------------------------------------------------------------------------------------
double le_cfg_GetFloat
(
    le_cfg_IteratorRef_t iteratorRef,   ///< [IN] Iterator to use as a basis for the transaction
    const char* pathPtr,                ///< [IN] Path to the target node. Can be an absolute path,
                                        ///<      or a path relative from the iterator's current
                                        ///<      position
    double defaultValue                 ///< [IN] Default value to use if the original can't be
                                        ///<      read
)
{
    double value;

    if (0 == strncmp(pathPtr, CFG_NODE_RX_BYTES, strlen(CFG_NODE_RX_BYTES)))
    {
        value = RxBytes;
    }
    else if (0 == strncmp(pathPtr, CFG_NODE_TX_BYTES, strlen(CFG_NODE_TX_BYTES)))
    {
        value = TxBytes;
    }
    else
    {
        value = defaultValue;
        LE_ERROR("Unsupported path '%s', using default value %f", pathPtr, defaultValue);
    }

    return value;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write a 64-bit floating point value to the configuration tree. Only valid
 * during a write transaction.
 *
 * If the path is empty, the iterator's current node will be set.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_SetFloat
(
    le_cfg_IteratorRef_t iteratorRef,   ///< [IN] Iterator to use as a basis for the transaction
    const char* pathPtr,                ///< [IN] Path to the target node. Can be an absolute path,
                                        ///<      or a path relative from the iterator's current
                                        ///<      position
    double value                        ///< [IN] Value to write
)
{
    le_cfgSimu_SetFloatNodeValue(iteratorRef, pathPtr, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a value from the tree as a boolean. If the node is empty or doesn't exist, the default
 * value is returned. Default value is also returned if the node is a different type than
 * expected.
 *
 * Valid for both read and write transactions.
 *
 * If the path is empty, the iterator's current node will be read.
 */
//--------------------------------------------------------------------------------------------------
bool le_cfg_GetBool
(
    le_cfg_IteratorRef_t iteratorRef,   ///< [IN] Iterator to use as a basis for the transaction
    const char* pathPtr,                ///< [IN] Path to the target node. Can be an absolute path,
                                        ///<      or a path relative from the iterator's current
                                        ///<      position
    bool defaultValue                   ///< [IN] Default value to use if the original can't be
                                        ///<      read
)
{
    bool value;

    if (0 == strncmp(pathPtr, CFG_NODE_COUNTING, strlen(CFG_NODE_COUNTING)))
    {
        value = BytesCounting;
    }
    else
    {
        value = defaultValue;
        LE_ERROR("Unsupported path '%s', using default value %d", pathPtr, defaultValue);
    }

    return value;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write a boolean value to the config tree. Only valid during a write
 * transaction.
 *
 * If the path is empty, the iterator's current node will be set.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_SetBool
(
    le_cfg_IteratorRef_t iteratorRef,   ///< [IN] Iterator to use as a basis for the transaction
    const char* pathPtr,                ///< [IN] Path to the target node. Can be an absolute path,
                                        ///<      or a path relative from the iterator's current
                                        ///<      position
    bool value                          ///< [IN] Value to write
)
{
    le_cfgSimu_SetBoolNodeValue(iteratorRef, pathPtr, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a string value from the config tree. If the value isn't a string, or if the node is
 * empty or doesn't exist, the default value will be returned.
 *
 * @return LE_OK       Read was completed successfully
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfg_GetString
(
    le_cfg_IteratorRef_t iteratorRef, ///< [IN] Iterator to use as a basis for the transaction
    const char* pathPtr,              ///< [IN] Path to the target node
    char* valuePtr,                   ///< [OUT] Buffer to write the value into
    size_t valueNumElements,          ///< [IN] Number of elements to copy
    const char* defaultValuePtr       ///< [IN] Default value to use if the original can't be read
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write a string value to the config tree. Only valid during a write transaction.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_SetString
(
    le_cfg_IteratorRef_t iteratorRef, ///< [IN] Iterator to use as a basis for the transaction
    const char* pathPtr,              ///< [IN] Path to the target node
    const char* valuePtr              ///< [IN] Value to write
)
{
}
