/**
 * This module implements some stub for sim unit test.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * ICCID identifier stored locally for simulation purpose
 */
//--------------------------------------------------------------------------------------------------
static char Iccid[LE_SIM_ICCID_BYTES];

//--------------------------------------------------------------------------------------------------
/**
 * Begin monitoring the event loop on the current thread.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_MonitorEventLoop
(
    uint32_t watchdog,             ///< [IN] Watchdog to use for monitoring
    le_clk_Time_t watchdogInterval ///< [IN] Interval at which to check event loop is functioning
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a write transaction and open a new iterator for both reading and writing.
 * This will respond with a newly created iterator reference.
 *
 * @return This function return a NULL reference
 */
//--------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t le_cfg_CreateWriteTxn
(
    const char *basePathPtr
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close and free the given iterator object. If the iterator is a write iterator, the transaction
 * will be canceled. If the iterator is a read iterator, the transaction will be closed.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_CancelTxn
(
    le_cfg_IteratorRef_t iteratorRef ///< [IN] Iterator object to close

)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Close the write iterator and commit the write transaction. This updates the config tree
 * with all of the writes that occured using the iterator.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_CommitTxn
(
    le_cfg_IteratorRef_t iteratorRef ///< [IN] Iterator object to commit
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a read transaction and open a new iterator for traversing the config tree.
 *
 * @return This function return a NULL reference
 */
//--------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t le_cfg_CreateReadTxn
(
    const char* basePathPtr    ///< [IN] Path to the location to create the new iterator.
)
{
    return NULL;
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
    memcpy(valuePtr, Iccid, valueNumElements);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write a string value to the config tree. Only valid during a write
 * transaction.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_SetString
(
    le_cfg_IteratorRef_t iteratorRef, ///< [IN] Iterator to use as a basis for the transaction
    const char* pathPtr,              ///< [IN] Path to the target node
    const char* valuePtr              ///< [IN] Value to write
)
{
    memcpy(Iccid, valuePtr, LE_SIM_ICCID_BYTES);
};