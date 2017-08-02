/**
 * @file le_cfg_simu.c
 *
 * Simulation implementation of the configuration Tree API.
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


#define ECALL_SYS_STD_MAX_LEN       (12+1)
#define ECALL_VIN_MAX_LEN           (17+1)
#define ECALL_PROPULSION_MAX_LEN    (16+1)
#define ECALL_VEHICLE_TYPE_MAX_LEN  (16+1)
static char ECallSys[ECALL_SYS_STD_MAX_LEN] = {0};
static char ECallVin[ECALL_VIN_MAX_LEN] = {0};
static char ECallProp[ECALL_PROPULSION_MAX_LEN] = {0};
static char ECallVeh[ECALL_VEHICLE_TYPE_MAX_LEN] = {0};


static char ECallCfg0[255] = {0};
static char ECallCfg1[255] = {0};
static char ECallCfg2[255] = {0};
static char ECallCfg3[255] = {0};

static le_cfg_IteratorRef_t IteratorRefSimu;

#define CFG_NODE_0 "0"
#define CFG_NODE_1 "1"
#define CFG_NODE_2 "2"
#define CFG_NODE_3 "3"

//--------------------------------------------------------------------------------------------------
/**
 * Server Service Reference
 */
//--------------------------------------------------------------------------------------------------
static le_msg_ServiceRef_t _ServerServiceRef;

//--------------------------------------------------------------------------------------------------
/**
 * Client Session Reference for the current message received from a client
 */
//--------------------------------------------------------------------------------------------------
static le_msg_SessionRef_t _ClientSessionRef;

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_cfg_GetServiceRef
(
    void
)
{
    return _ServerServiceRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_cfg_GetClientSessionRef
(
    void
)
{
    return _ClientSessionRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the server and advertise the service.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_AdvertiseService
(
    void
)
{
    return ;
}

//--------------------------------------------------------------------------------------------------
/**
 * Identifies the type of node.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_CFG_TYPE_EMPTY,
        ///< A node with no value.

    LE_CFG_TYPE_STRING,
        ///< A string encoded as utf8.

    LE_CFG_TYPE_BOOL,
        ///< Boolean value.

    LE_CFG_TYPE_INT,
        ///< Signed 32-bit.

    LE_CFG_TYPE_FLOAT,
        ///< 64-bit floating point value.

    LE_CFG_TYPE_STEM,
        ///< Non-leaf node, this node is the parent of other nodes.

    LE_CFG_TYPE_DOESNT_EXIST
        ///< Node doesn't exist.
}
le_cfg_nodeType_t;


//--------------------------------------------------------------------------------------------------
/**
 * Length of the strings used by this API.
 */
//--------------------------------------------------------------------------------------------------
#define LE_CFG_STR_LEN 511


//--------------------------------------------------------------------------------------------------
/**
 * Length of the strings used by this API, including the trailing NULL.
 */
//--------------------------------------------------------------------------------------------------
#define LE_CFG_STR_LEN_BYTES 512


//--------------------------------------------------------------------------------------------------
/**
 * Allowed length of a node name.
 */
//--------------------------------------------------------------------------------------------------
#define LE_CFG_NAME_LEN 63


//--------------------------------------------------------------------------------------------------
/**
 * The node name length, including a trailing NULL.
 */
//--------------------------------------------------------------------------------------------------
#define LE_CFG_NAME_LEN_BYTES 64


//--------------------------------------------------------------------------------------------------
/**
 * Reference type used by Add/Remove functions for EVENT 'le_cfg_Change'
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_cfg_ChangeHandler* le_cfg_ChangeHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Handler for node change notifications.
 *
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_cfg_ChangeHandlerFunc_t)
(
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Create a read transaction and open a new iterator for traversing the config tree.
 *
 * @note This action creates a read lock on the given tree, which will start a read-timeout.
 *        Once the read timeout expires, all active read iterators on that tree will be
 *        expired and the clients will be killed.
 *
 * @note A tree transaction is global to that tree; a long-held read transaction will block other
 *        user's write transactions from being comitted.
 *
 * @return This will return a newly created iterator reference.
 */
//--------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t le_cfg_CreateReadTxn
(
    const char* basePath
        ///< [IN]
        ///< Path to the location to create the new iterator.
)
{
    return (le_cfg_IteratorRef_t)IteratorRefSimu;
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
    const char* basePath
        ///< [IN]
        ///< Path to the location to create the new iterator.
)
{
    return (le_cfg_IteratorRef_t)IteratorRefSimu;
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
    return ;
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
    return ;
}

//--------------------------------------------------------------------------------------------------
/**
 * Change the node where the iterator is pointing. The path passed can be an absolute or a
 * relative path from the iterators current location.
 *
 * The target node does not need to exist. Writing a value to a non-existant node will
 * automatically create that node and any ancestor nodes (parent, parent's parent, etc.) that
 * also don't exist.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_GoToNode
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to move.

    const char* newPath
        ///< [IN]
        ///< Absolute or relative path from the current location.
)
{
    return ;
}

//--------------------------------------------------------------------------------------------------
/**
 * Move the iterator to the parent of the node.
 *
 * @return Return code will be one of the following values:
 *
 *         - LE_OK        - Commit was completed successfully.
 *         - LE_NOT_FOUND - Current node is the root node: has no parent.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfg_GoToParent
(
    le_cfg_IteratorRef_t iteratorRef
        ///< [IN]
        ///< Iterator to move.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Move the iterator to the the first child of the node where the iterator is currently pointed.
 *
 * For read iterators without children, this function will fail. If the iterator is a write
 * iterator, then a new node is automatically created. If this node or newly created
 * children of this node are not written to, then this node will not persist even if the iterator is
 * comitted.
 *
 * @return Return code will be one of the following values:
 *
 *         - LE_OK        - Move was completed successfully.
 *         - LE_NOT_FOUND - The given node has no children.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfg_GoToFirstChild
(
    le_cfg_IteratorRef_t iteratorRef
        ///< [IN]
        ///< Iterator object to move.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Jump the iterator to the next child node of the current node. Assuming the following tree:
 *
 * @code
 * baseNode/
 *   childA/
 *     valueA
 *     valueB
 * @endcode
 *
 * If the iterator is moved to the path, "/baseNode/childA/valueA". After the first
 * GoToNextSibling the iterator will be pointing at valueB. A second call to GoToNextSibling
 * will cause the function to return LE_NOT_FOUND.
 *
 * @return Returns one of the following values:
 *
 *         - LE_OK            - Commit was completed successfully.
 *         - LE_NOT_FOUND     - Iterator has reached the end of the current list of siblings.
 *                              Also returned if the the current node has no siblings.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfg_GoToNextSibling
(
    le_cfg_IteratorRef_t iteratorRef
        ///< [IN]
        ///< Iterator to iterate.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get path to the node where the iterator is currently pointed.
 *
 * Assuming the following tree:
 *
 * @code
 * baseNode/
 *   childA/
 *     valueA
 *     valueB
 * @endcode
 *
 * If the iterator was currently pointing at valueA, GetPath would return the following path:
 *
 * @code
 * /baseNode/childA/valueA
 * @endcode
 *
 * Optionally, a path to another node can be supplied to this function. So, if the iterator is
 * again on valueA and the relative path ".." is supplied then this function will return the
 * following path:
 *
 * @code
 * /baseNode/childA/
 * @endcode
 *
 * @return - LE_OK            - The write was completed successfully.
 *         - LE_OVERFLOW      - The supplied string buffer was not large enough to hold the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfg_GetPath
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to move.

    const char* path,
        ///< [IN]
        ///< Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    char* pathBuffer,
        ///< [OUT]
        ///< Absolute path to the iterator's current node.

    size_t pathBufferNumElements
        ///< [IN]
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the type of node where the iterator is currently pointing.
 *
 * @return le_cfg_nodeType_t value indicating the stored value.
 */
//--------------------------------------------------------------------------------------------------
le_cfg_nodeType_t le_cfg_GetNodeType
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator object to use to read from the tree.

    const char* path
        ///< [IN]
        ///< Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.
)
{
    return LE_CFG_TYPE_INT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the name of the node where the iterator is currently pointing.
 *
 * @return - LE_OK       Read was completed successfully.
 *         - LE_OVERFLOW Supplied string buffer was not large enough to hold the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfg_GetNodeName
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator object to use to read from the tree.

    const char* path,
        ///< [IN]
        ///< Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    char* name,
        ///< [OUT]
        ///< Read the name of the node object.

    size_t nameNumElements
        ///< [IN]
)
{
    return LE_OK;
}

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
 * Remove handler function for EVENT 'le_cfg_Change'
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_RemoveChangeHandler
(
    le_cfg_ChangeHandlerRef_t addHandlerRef
        ///< [IN]
)
{
    return ;
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete the node specified by the path. If the node doesn't exist, nothing happens. All child
 * nodes are also deleted.
 *
 * If the path is empty, the iterator's current node is deleted.
 *
 * Only valid during a write transaction.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_DeleteNode
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path
        ///< [IN]
        ///< Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.
)
{
    return ;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if the given node is empty. A node is also considered empty if it doesn't yet exist. A
 * node is also considered empty if it has no value or is a stem with no children.
 *
 * If the path is empty, the iterator's current node is queried for emptiness.
 *
 * Valid for both read and write transactions.
 *
 * @return A true if the node is considered empty, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool le_cfg_IsEmpty
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path
        ///< [IN]
        ///< Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.
)
{
    char * strPtr = NULL;
    if ( strncmp(path, CFG_NODE_SYSTEM_STD, strlen(CFG_NODE_SYSTEM_STD)) == 0 )
    {
        strPtr = &ECallSys[0];
    }
    else if ( strncmp(path, CFG_NODE_VIN, strlen(CFG_NODE_VIN)) == 0 )
    {
        strPtr = &ECallVin[0];
    }
    else if ( strncmp(path, CFG_NODE_VEH, strlen(CFG_NODE_VEH)) == 0 )
    {
        strPtr = &ECallVeh[0];
    }
    else if ( strncmp(path, CFG_NODE_PROP, strlen(CFG_NODE_PROP)) == 0 )
    {
        strPtr = &ECallProp[0];
    }
    else if ( strncmp(path, CFG_NODE_0, strlen(CFG_NODE_0)) == 0 )
    {
        strPtr = &ECallCfg0[0];
    }
    else if ( strncmp(path, CFG_NODE_1, strlen(CFG_NODE_1)) == 0 )
    {
        strPtr = &ECallCfg1[0];
    }
    else if ( strncmp(path, CFG_NODE_2, strlen(CFG_NODE_2)) == 0 )
    {
        strPtr = &ECallCfg2[0];
    }
    else if ( strncmp(path, CFG_NODE_3, strlen(CFG_NODE_3)) == 0 )
    {
        strPtr = &ECallCfg3[0];
    }
    else
    {
        LE_WARN("le_cfg_IsEmpty DID not find path %s", path);
        return false;
    }

    if( NULL != strPtr )
    {
        if( 0 == strlen(strPtr))
        {
            return true;
        }
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear out the nodes's value. If it doesn't exist it will be created, but have no value.
 *
 * If the path is empty, the iterator's current node will be cleared. If the node is a stem
 * then all children will be removed from the tree.
 *
 * Only valid during a write transaction.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_SetEmpty
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path
        ///< [IN]
        ///< Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.
)
{
    char * strPtr = NULL;
    if ( strncmp(path, CFG_NODE_SYSTEM_STD, strlen(CFG_NODE_SYSTEM_STD)) == 0 )
    {
        strPtr = &ECallSys[0];
    }
    else if ( strncmp(path, CFG_NODE_VIN, strlen(CFG_NODE_VIN)) == 0 )
    {
        strPtr = &ECallVin[0];
    }
    else if ( strncmp(path, CFG_NODE_VEH, strlen(CFG_NODE_VEH)) == 0 )
    {
        strPtr = &ECallVeh[0];
    }
    else if ( strncmp(path, CFG_NODE_PROP, strlen(CFG_NODE_PROP)) == 0 )
    {
        strPtr = &ECallProp[0];
    }
    else if ( strncmp(path, CFG_NODE_0, strlen(CFG_NODE_0)) == 0 )
    {
        strPtr = &ECallCfg0[0];
    }
    else if ( strncmp(path, CFG_NODE_1, strlen(CFG_NODE_1)) == 0 )
    {
        strPtr = &ECallCfg1[0];
    }
    else if ( strncmp(path, CFG_NODE_2, strlen(CFG_NODE_2)) == 0 )
    {
        strPtr = &ECallCfg2[0];
    }
    else if ( strncmp(path, CFG_NODE_3, strlen(CFG_NODE_3)) == 0 )
    {
        strPtr = &ECallCfg3[0];
    }
    else
    {
        LE_WARN("le_cfg_SetEmpty DID not find path %s", path);
        return;
    }

    if( NULL != strPtr )
    {
        strPtr[0] = '\0';
    }
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check to see if a given node in the config tree exists.
 *
 * @return True if the specified node exists in the tree. False if not.
 */
//--------------------------------------------------------------------------------------------------
bool le_cfg_NodeExists
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path
        ///< [IN]
        ///< Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.
)
{
    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set a simulated value for a specific node.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_cfgSimu_SetStringNodeValue
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node. Can be an absolute path,
        ///< or a path relative from the iterator's current
        ///< position.

    const char* value
        ///< [IN]
        ///< Buffer to write the value into.
)
{
    IteratorRefSimu = iteratorRef;

    if (0 == strncmp(path, CFG_NODE_SYSTEM_STD, strlen(CFG_NODE_SYSTEM_STD)))
    {
        memset(ECallSys, 0, sizeof(ECallSys));
        // Check whether length of source string is higher than length of destination string
        if (strlen(value) <= sizeof(ECallSys))
        {
            strncpy(ECallSys, value, strlen(value));
        }
    }
    else if (0 == strncmp(path, CFG_NODE_VIN, strlen(CFG_NODE_VIN)))
    {
        memset(ECallVin, 0, sizeof(ECallVin));
        strncpy(ECallVin, value, strlen(value));
    }
    else if (0 == strncmp(path, CFG_NODE_VEH, strlen(CFG_NODE_VEH)))
    {
        memset(ECallVeh, 0, sizeof(ECallVeh));
        strncpy(ECallVeh, value, strlen(value));
    }
    else if (0 == strncmp(path, CFG_NODE_PROP, strlen(CFG_NODE_PROP)))
    {
        memset(ECallProp, 0, sizeof(ECallProp));
        strncpy(ECallProp, value, strlen(value));
    }
    else if (0 == strncmp(path, CFG_NODE_0, strlen(CFG_NODE_0)))
    {
        memset(ECallCfg0, 0, sizeof(ECallCfg0));
        strncpy(ECallCfg0, value, strlen(value));
    }
    else if (0 == strncmp(path, CFG_NODE_1, strlen(CFG_NODE_1)))
    {
        memset(ECallCfg1, 0, sizeof(ECallCfg1));
        strncpy(ECallCfg1, value, strlen(value));
    }
    else if (0 == strncmp(path, CFG_NODE_2, strlen(CFG_NODE_2)))
    {
        memset(ECallCfg2, 0, sizeof(ECallCfg2));
        strncpy(ECallCfg2, value, strlen(value));
    }
    else if (0 == strncmp(path, CFG_NODE_3, strlen(CFG_NODE_3)))
    {
        memset(ECallCfg3, 0, sizeof(ECallCfg3));
        strncpy(ECallCfg3, value, strlen(value));
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a string value from the config tree. If the value isn't a string, or if the node is
 * empty or doesn't exist, the default value will be returned.
 *
 * Valid for both read and write transactions.
 *
 * If the path is empty, the iterator's current node will be read.
 *
 * @return - LE_OK       - Read was completed successfully.
 *         - LE_OVERFLOW - Supplied string buffer was not large enough to hold the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfg_GetString
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node. Can be an absolute path,
        ///< or a path relative from the iterator's current
        ///< position.

    char* value,
        ///< [OUT]
        ///< Buffer to write the value into.

    size_t valueNumElements,
        ///< [IN]

    const char* defaultValue
        ///< [IN]
        ///< Default value to use if the original can't be
        ///<   read.
)
{
    le_result_t res = LE_FAULT;

    if ( strncmp(path, CFG_NODE_SYSTEM_STD, strlen(CFG_NODE_SYSTEM_STD)) == 0 )
    {
        strncpy(value, ECallSys, strlen(ECallSys));
        res = LE_OK;
    }
    else if ( strncmp(path, CFG_NODE_VIN, strlen(CFG_NODE_VIN)) == 0 )
    {
        strncpy(value, ECallVin, strlen(ECallVin));
        res = LE_OK;
    }
    else if ( strncmp(path, CFG_NODE_VEH, strlen(CFG_NODE_VEH)) == 0 )
    {
        strncpy(value, ECallVeh, strlen(ECallVeh));
        res = LE_OK;
    }
    else if ( strncmp(path, CFG_NODE_0, strlen(CFG_NODE_0)) == 0 )
    {
        strncpy(value, ECallCfg0, strlen(ECallCfg0));
        res = LE_OK;
    }
    else if ( strncmp(path, CFG_NODE_1, strlen(CFG_NODE_1)) == 0 )
    {
        strncpy(value, ECallCfg1, strlen(ECallCfg1));
        res = LE_OK;
    }
    else if ( strncmp(path, CFG_NODE_2, strlen(CFG_NODE_2)) == 0 )
    {
        strncpy(value, ECallCfg2, strlen(ECallCfg2));
        res = LE_OK;
    }
    else if ( strncmp(path, CFG_NODE_3, strlen(CFG_NODE_3)) == 0 )
    {
        strncpy(value, ECallCfg3, strlen(ECallCfg3));
        res = LE_OK;
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write a string value to the config tree. Only valid during a write
 * transaction.
 *
 * If the path is empty, the iterator's current node will be set.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_SetString
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    const char* value
        ///< [IN]
        ///< Value to write.
)
{
    le_cfgSimu_SetStringNodeValue( iteratorRef, path, value);
    return ;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a signed integer value from the config tree.
 *
 * If the underlying value is not an integer, the default value will be returned instead. The
 * default value is also returned if the node does not exist or if it's empty.
 *
 * If the value is a floating point value, then it will be rounded and returned as an integer.
 *
 * Valid for both read and write transactions.
 *
 * If the path is empty, the iterator's current node will be read.
 */
//--------------------------------------------------------------------------------------------------
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
    return 1;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write a signed integer value to the config tree. Only valid during a
 * write transaction.
 *
 * If the path is empty, the iterator's current node will be set.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_SetInt
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    int32_t value
        ///< [IN]
        ///< Value to write.
)
{
    return ;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a 64-bit floating point value from the config tree.
 *
 * If the value is an integer then the value will be promoted to a float. Otherwise, if the
 * underlying value is not a float or integer, the default value will be returned.
 *
 * If the path is empty, the iterator's current node will be read.
 *
 * @note Floating point values will only be stored up to 6 digits of precision.
 */
//--------------------------------------------------------------------------------------------------
double le_cfg_GetFloat
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    double defaultValue
        ///< [IN]
        ///< Default value to use if the original can't be
        ///<   read.
)
{
    return 1.1;
}
//--------------------------------------------------------------------------------------------------
/**
 * Write a 64-bit floating point value to the config tree. Only valid
 * during a write transaction.
 *
 * If the path is empty, the iterator's current node will be set.
 *
 * @note Floating point values will only be stored up to 6 digits of precision.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_SetFloat
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    double value
        ///< [IN]
        ///< Value to write.
)
{
    return ;
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
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    bool defaultValue
        ///< [IN]
        ///< Default value to use if the original can't be
        ///<   read.
)
{
    return true;
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
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    bool value
        ///< [IN]
        ///< Value to write.
)
{
    return ;
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete the node specified by the path. If the node doesn't exist, nothing happens. All child
 * nodes are also deleted.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_QuickDeleteNode
(
    const char* path
        ///< [IN]
        ///< Path to the node to delete.
)
{
    return ;
}

//--------------------------------------------------------------------------------------------------
/**
 * Make a given node empty. If the node doesn't currently exist then it is created as a new empty
 * node.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_QuickSetEmpty
(
    const char* path
        ///< [IN]
        ///< Absolute or relative path to read from.
)
{
    return ;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a string value from the config tree. If the value isn't a string, or if the node is
 * empty or doesn't exist, the default value will be returned.
 *
 * @return - LE_OK       - Commit was completed successfully.
 *         - LE_OVERFLOW - Supplied string buffer was not large enough to hold the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfg_QuickGetString
(
    const char* path,
        ///< [IN]
        ///< Path to read from.

    char* value,
        ///< [OUT]
        ///< Value read from the requested node.

    size_t valueNumElements,
        ///< [IN]

    const char* defaultValue
        ///< [IN]
        ///< Default value to use if the original can't be read.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write a string value to the config tree.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_QuickSetString
(
    const char* path,
        ///< [IN]
        ///< Path to the value to write.

    const char* value
        ///< [IN]
        ///< Value to write.
)
{
    return ;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a signed integer value from the config tree. If the value is a floating point
 * value, then it will be rounded and returned as an integer. Otherwise If the underlying value is
 * not an integer or a float, the default value will be returned instead.
 *
 * If the value is empty or the node doesn't exist, the default value is returned instead.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_cfg_QuickGetInt
(
    const char* path,
        ///< [IN]
        ///< Path to the value to write.

    int32_t defaultValue
        ///< [IN]
        ///< Default value to use if the original can't be read.
)
{
    return 1;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write a signed integer value to the config tree.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_QuickSetInt
(
    const char* path,
        ///< [IN]
        ///< Path to the value to write.

    int32_t value
        ///< [IN]
        ///< Value to write.
)
{
    return ;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a 64-bit floating point value from the config tree. If the value is an integer,
 * then it is promoted to a float. Otherwise, if the underlying value is not a float, or an
 * integer the default value will be returned.
 *
 * If the value is empty or the node doesn't exist, the default value is returned.
 *
 * @note Floating point values will only be stored up to 6 digits of precision.
 */
//--------------------------------------------------------------------------------------------------
double le_cfg_QuickGetFloat
(
    const char* path,
        ///< [IN]
        ///< Path to the value to write.

    double defaultValue
        ///< [IN]
        ///< Default value to use if the original can't be read.
)
{
    return 1.1;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write a 64-bit floating point value to the config tree.
 *
 * @note Floating point values will only be stored up to 6 digits of precision.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_QuickSetFloat
(
    const char* path,
        ///< [IN]
        ///< Path to the value to write.

    double value
        ///< [IN]
        ///< Value to write.
)
{
    return ;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a value from the tree as a boolean. If the node is empty or doesn't exist, the default
 * value is returned. This is also true if the node is a different type than expected.
 *
 * If the value is empty or the node doesn't exist, the default value is returned instead.
 */
//--------------------------------------------------------------------------------------------------
bool le_cfg_QuickGetBool
(
    const char* path,
        ///< [IN]
        ///< Path to the value to write.

    bool defaultValue
        ///< [IN]
        ///< Default value to use if the original can't be read.
)
{
    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write a boolean value to the config tree.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_QuickSetBool
(
    const char* path,
        ///< [IN]
        ///< Path to the value to write.

    bool value
        ///< [IN]
        ///< Value to write.
)
{
    return ;
}



