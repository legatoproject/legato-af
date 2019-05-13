#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference stub for le_posCtrl
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_posCtrl_GetServiceRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference stub for le_posCtrl
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_posCtrl_GetClientSessionRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference stub for le_pos
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_pos_GetServiceRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference stub for le_pos
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_pos_GetClientSessionRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Simulated config tree values.
 */
//--------------------------------------------------------------------------------------------------
static int32_t RateCount = 5000;

//--------------------------------------------------------------------------------------------------
/**
 * Config tree iterator reference
 */
//--------------------------------------------------------------------------------------------------
static le_cfg_IteratorRef_t SimuIteratorRef;

//--------------------------------------------------------------------------------------------------
/**
 * Paths to positioning data in the config tree
 */
//--------------------------------------------------------------------------------------------------

#define CFG_NODE_RATE               "acquisitionRate"
#define CFG_POSITIONING_RATE_PATH   CFG_POSITIONING_PATH"/"CFG_NODE_RATE

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_cfg_Change'
 *
 * This event provides information on changes to the given node object, or any of it's children,
 * where a change could be either a read, write, create or delete operation.
 */
//--------------------------------------------------------------------------------------------------
le_cfg_ChangeHandlerRef_t le_cfg_AddChangeHandler
(
   const char* newPath,
        ///< [IN]
        ///< Path to the object to watch.

    le_cfg_ChangeHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    return NULL;
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
    le_cfg_IteratorRef_t iteratorRef
        ///< [IN]
        ///< Iterator object to close.
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close the write iterator and commit the write transaction. This updates the config tree
 * with all of the writes that occured using the iterator.
 *
 * @note This operation will also delete the iterator object.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_CommitTxn
(
    le_cfg_IteratorRef_t iteratorRef
        ///< [IN]
        ///< Iterator object to commit.
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
// Config Tree service stubbing
//--------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------
/**
 *  Create a read transaction and open a new iterator for traversing the configuration tree.
 *
 *  @note: This action creates a read lock on the given tree.  Which will start a read-timeout.
 *         Once the read timeout expires, then all active read iterators on that tree will be
 *         expired and the clients killed.
 *
 *  @note: A tree transaction is global to that tree; a long held read transaction will block other
 *         users write transactions from being committed.
 *
 *  @return This will return a newly created iterator reference.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t le_cfg_CreateReadTxn
(
    const char* basePath
        ///< [IN]
        ///< Path to the location to create the new iterator.
)
{
    return (le_cfg_IteratorRef_t)SimuIteratorRef;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Create a write transaction and open a new iterator for both reading and writing.
 *
 *  @note: This action creates a write transaction. If the application holds the iterator for
 *         longer than the configured write transaction timeout, the iterator will cancel the
 *         transaction.  All further reads will fail to return data and all writes will be thrown
 *         away.
 *
 *  @note A tree transaction is global to that tree, so a long held write transaction will block
 *        other user's write transactions from being started.  However other trees in the system
 *        will be unaffected.
 *
 *  \b Responds \b With:
 *
 *  This will respond with a newly created iterator reference.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t le_cfg_CreateWriteTxn
(
    const char *    basePath
        ///< [IN]
        ///< Path to the location to create the new iterator.
)
{
    return (le_cfg_IteratorRef_t)SimuIteratorRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set a simulated integer value for a specific node.
 */
//--------------------------------------------------------------------------------------------------
void le_cfgSimu_SetIntNodeValue
(
    le_cfg_IteratorRef_t iteratorRef,   ///< [IN] Iterator to use as a basis for the transaction.
    const char* path,                   ///< [IN] Path to the target node. Can be an absolute path,
                                        ///<      or a path relative from the iterator's current
                                        ///<      position.
    int32_t value                       ///< [IN] Value to write.
)
{
    SimuIteratorRef = iteratorRef;

    if (0 == strncmp(path, CFG_NODE_RATE, strlen(CFG_NODE_RATE)))
    {
        RateCount = value;
    }
    else
    {
        LE_ERROR("Unsupported path '%s'", path);
    }
}

// -------------------------------------------------------------------------------------------------
/**
 *  Read a signed integer value from the configuration tree.
 *
 *  If the underlying value is not an integer, the default value will be returned instead.  The
 *  default value is also returned if the node does not exist or if it's empty.
 *
 *  If the value is a floating point value, it will be rounded and returned as an integer.
 *
 *  Valid for both read and write transactions.
 *
 *  If the path is empty, the iterator's current node will be read.
 */
// -------------------------------------------------------------------------------------------------
int32_t le_cfg_GetInt
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    int32_t defaultValue
        ///< [IN]
        ///< Default value to use if the original can't be
        ///<   read.
)
{
    int32_t value;

    if (0 == strncmp(path, CFG_NODE_RATE, strlen(CFG_NODE_RATE)))
    {
        value = RateCount;
    }
    else
    {
        value = defaultValue;
        LE_ERROR("Unsupported path '%s', using default value %d", path, defaultValue);
    }

    return value;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Write a signed integer value to the configuration tree. Only valid during a write transaction.
 *
 *  If the path is empty, the iterator's current node will be set.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetInt
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Full or relative path to the value to write.

    int32_t value
        ///< [IN]
        ///< Value to write.
)
{
   le_cfgSimu_SetIntNodeValue(iteratorRef, path, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Start watchdogs 0..N-1.  Typically this is used in COMPONENT_INIT to start all watchdogs needed
 * by the process.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_Init
(
    uint32_t wdogCount
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Begin monitoring the event loop on the current thread.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_MonitorEventLoop
(
    uint32_t watchdog,          ///< Watchdog to use for monitoring
    le_clk_Time_t watchdogInterval ///< Interval at which to check event loop is functioning
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop a watchdog.
 *
 * This can also cause the chain to be completely kicked, so check it.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_Stop
(
    uint32_t watchdog
)
{
    // do nothing
}
