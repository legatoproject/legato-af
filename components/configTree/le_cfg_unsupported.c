/**
 * @file le_cfg_unsupported.c
 *
 * Config tree APIs that are not supported.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

/** Common warning for unsupported calls. */
#define NOT_SUPPORTED(level)    LE_##level("%s not supported", __func__)


// -------------------------------------------------------------------------------------------------
/**
 *  Create a read transaction and open a new iterator for traversing the configuration tree.
 *
 *  @return This will return a newly created iterator reference.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t le_cfg_CreateReadTxn
(
    const char *basePathPtr ///< [IN] Path to the location to create the new iterator.
)
{
    NOT_SUPPORTED(WARN);
    return NULL;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Create a write transaction and open a new iterator for both reading and writing.
 *
 *  @return This will return a newly created iterator reference.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t le_cfg_CreateWriteTxn
(
    const char *basePathPtr ///< [IN] Path to the location to create the new iterator.
)
{
    NOT_SUPPORTED(WARN);
    return NULL;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Close the write iterator and commit the write transaction.  This updates the config tree
 *  with all of the writes that occured using the iterator.
 *
 *  @note: This operation will also delete the iterator object.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_CommitTxn
(
    le_cfg_IteratorRef_t externalRef ///< [IN] Iterator object to commit.
)
{
   NOT_SUPPORTED(WARN);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Close and free the given iterator object.  If the iterator is a write iterator, the transaction
 *  will be canceled.  If the iterator is a read iterator, the transaction will be closed.
 *
 *  @note: This operation will also delete the iterator object.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_CancelTxn
(
    le_cfg_IteratorRef_t externalRef ///< [IN] Iterator object to close.
)
{
    NOT_SUPPORTED(WARN);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Check if the given node is empty.  A node is considered empty if it has no value.  A node is
 *  also considered empty if it doesn't yet exist.
 *
 *  If the path is empty, the iterator's current node is queried for emptiness.
 *
 *  Valid for both read and write transactions.
 *
 *  @return True if the node is considered empty, false if not.
 */
// -------------------------------------------------------------------------------------------------
bool le_cfg_IsEmpty
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr      ///< [IN] Absolute or relative path to read from.
)
{
    NOT_SUPPORTED(WARN);
    return TRUE;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Clear out the node's value.  If it doesn't exist it will be created, but have no value.
 *
 *  If the path is empty, the iterator's current node will be cleared.  If the node is a stem,
 *  all children will be removed from the tree.
 *
 *  Only valid during a write transaction.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetEmpty
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr      ///< [IN] Absolute or relative path to read from.
)
{
    NOT_SUPPORTED(WARN);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Check to see if a given node in the configuration tree exists.
 *
 *  @return True if the specified node exists in the tree.  False if not.
 */
// -------------------------------------------------------------------------------------------------
bool le_cfg_NodeExists
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr      ///< [IN] Absolute or relative path to read from.
)
{
    NOT_SUPPORTED(WARN);
    return FALSE;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Read a string value from the configuration tree.  If the value isn't a string, or if the node is
 *  empty or doesn't exist, the default value will be returned.
 *
 *  Valid for both read and write transactions.
 *
 *  If the path is empty, the iterator's current node will be read.
 *
 *  @return
 *          - LE_OK            - Read was completed successfully.
 *          - LE_OVERFLOW      - Supplied string buffer was not large enough to hold the value.
 *          - LE_BAD_PARAMETER - Config iterator was invalid.
 */
// -------------------------------------------------------------------------------------------------
le_result_t le_cfg_GetString
(
    le_cfg_IteratorRef_t     externalRef,       ///< [IN]  Iterator to use as a basis for the
                                                ///<       transaction.
    const char              *pathPtr,           ///< [IN]  Absolute or relative path to read from.
    char                    *valueStr,          ///< [OUT] Buffer to write the value into.
    size_t                   valueNumElements,  ///< [IN]  Size of value buffer.
    const char              *defaultValueStr    ///< [IN]  Default value to use if none can be read.
)
{
    NOT_SUPPORTED(WARN);
    return LE_NOT_IMPLEMENTED;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Write a string value to the configuration tree.  Only valid during a write
 *  transaction.
 *
 *  If the path is empty, the iterator's current node will be set.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetString
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr,     ///< [IN] Full or relative path to the value to write.
    const char              *valueStr     ///< [IN] Value to write.
)
{
    NOT_SUPPORTED(WARN);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Read a signed integer value from the configuration tree.
 *
 *  If the underlying value is not an integer, the default value will be returned instead.  The
 *  default value is also returned if the node does not exist or if it's empty.
 *
 *  If the value is a floating point value, it will be truncated and returned as an integer.
 *
 *  Valid for both read and write transactions.
 *
 *  If the path is empty, the iterator's current node will be read.
 *
 *  @return The integer value.
 */
// -------------------------------------------------------------------------------------------------
int32_t le_cfg_GetInt
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr,     ///< [IN] Full or relative path to the value to read.
    int32_t                  defaultValue ///< [IN] Default value to use if none can be read.
)
{
    NOT_SUPPORTED(WARN);
    return defaultValue;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Write a signed integer value to the configuration tree.  Only valid during a
 *  write transaction.
 *
 *  If the path is empty, the iterator's current node will be set.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetInt
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr,     ///< [IN] Full or relative path to the value to write.
    int32_t                  value        ///< [IN] Value to write.
)
{
    NOT_SUPPORTED(WARN);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Read a 64-bit floating point value from the configuration tree.
 *
 *  If the value is an integer, the value will be promoted to a float.  Otherwise, if the
 *  underlying value is not a float or integer, the default value will be returned.
 *
 *  If the path is empty, the iterator's current node will be read.
 *
 *  @return The double value.
 */
// -------------------------------------------------------------------------------------------------
double le_cfg_GetFloat
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr,     ///< [IN] Full or relative path to the value to read.
    double                   defaultValue ///< [IN] Default value to use if none can be read.
)
{
    NOT_SUPPORTED(WARN);
    return defaultValue;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Write a 64-bit floating point value to the configuration tree.  Only valid
 *  during a write transaction.
 *
 *  If the path is empty, the iterator's current node will be set.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetFloat
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr,     ///< [IN] Full or relative path to the value to write.
    double                   value        ///< [IN] Value to write.
)
{
    NOT_SUPPORTED(WARN);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Read a value from the tree as a boolean.  If the node is empty or doesn't exist, the default
 *  value is returned.  The default value is also returned if the node is of a different type than
 *  expected.
 *
 *  Valid for both read and write transactions.
 *
 *  If the path is empty, the iterator's current node will be read.
 *
 *  @return The boolean value.
 */
// -------------------------------------------------------------------------------------------------
bool le_cfg_GetBool
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr,     ///< [IN] Full or relative path to the value to read.
    bool                     defaultValue ///< [IN] Default value to use if none can be read.
)
{
    NOT_SUPPORTED(WARN);
    return defaultValue;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Write a boolean value to the configuration tree.  Only valid during a write
 *  transaction.
 *
 *  If the path is empty, the iterator's current node will be set.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetBool
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr,     ///< [IN] Full or relative path to the value to write.
    bool                     value        ///< [IN] Value to write.
)
{
    NOT_SUPPORTED(WARN);
}

// -------------------------------------------------------------------------------------------------
/**
 * Clears the current value of a node. If the node doesn't currently exist then it is created as a
 * new empty node.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetEmpty
(
    const char *pathStr ///< Absolute or relative path to read from.
)
{
    NOT_SUPPORTED(WARN);
}

// -------------------------------------------------------------------------------------------------
/**
 * Reads a string value from the config tree. If the value isn't a string, or if the node is
 * empty or doesn't exist, the default value will be returned.
 *
 * @return - LE_OK       - Commit was completed successfully.
 *         - LE_OVERFLOW - Supplied string buffer was not large enough to hold the value.
 */
// -------------------------------------------------------------------------------------------------
le_result_t le_cfg_QuickGetString
(
    const char  *pathStr,           ///< [IN]  Absolute or relative path to read from.
    char        *valueStr,          ///< [OUT] Buffer to write the value into.
    size_t       valueNumElements,  ///< [IN]  Size of value buffer.
    const char  *defaultValueStr    ///< [IN]  Default value to use if none can be read.
)
{
    NOT_SUPPORTED(WARN);
    return LE_NOT_IMPLEMENTED;
}

// -------------------------------------------------------------------------------------------------
/**
 * Writes a string value to the config tree.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetString
(
    const char *pathStr,    ///< [IN] Absolute or relative path to read from.
    const char *valueStr    ///< [IN] Value to write.
)
{
    NOT_SUPPORTED(WARN);
}

// -------------------------------------------------------------------------------------------------
/**
 * Reads a signed integer value from the config tree. If the value is a floating point
 * value, then it will be rounded and returned as an integer. Otherwise If the underlying value is
 * not an integer or a float, the default value will be returned instead.
 *
 * If the value is empty or the node doesn't exist, the default value is returned instead.
 */
// -------------------------------------------------------------------------------------------------
int32_t le_cfg_QuickGetInt
(
    const char  *pathStr,       ///< [IN]  Absolute or relative path to read from.
    int32_t      defaultValue   ///< [IN]  Default value to use if none can be read.
)
{
    NOT_SUPPORTED(ERROR);
    return defaultValue;
}

// -------------------------------------------------------------------------------------------------
/**
 * Writes a signed integer value to the config tree.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetInt
(
    const char  *pathStr,   ///< [IN] Absolute or relative path to read from.
    int32_t      value      ///< [IN] Value to write.
)
{
    NOT_SUPPORTED(ERROR);
}

// -------------------------------------------------------------------------------------------------
/**
 * Reads a 64-bit floating point value from the config tree. If the value is an integer,
 * then it is promoted to a float. Otherwise, if the underlying value is not a float, or an
 * integer the default value will be returned.
 *
 * If the value is empty or the node doesn't exist, the default value is returned.
 *
 * @note Floating point values will only be stored up to 6 digits of precision.
 */
// -------------------------------------------------------------------------------------------------
double le_cfg_QuickGetFloat
(
    const char  *pathStr,       ///< [IN]  Absolute or relative path to read from.
    double       defaultValue   ///< [IN]  Default value to use if none can be read.
)
{
    NOT_SUPPORTED(WARN);
    return defaultValue;
}

// -------------------------------------------------------------------------------------------------
/**
 * Writes a 64-bit floating point value to the config tree.
 *
 * @note Floating point values will only be stored up to 6 digits of precision.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetFloat
(
    const char  *pathStr,   ///< [IN] Absolute or relative path to read from.
    double       value      ///< [IN] Value to write.
)
{
    NOT_SUPPORTED(WARN);
}

// -------------------------------------------------------------------------------------------------
/**
 * Reads a value from the tree as a boolean. If the node is empty or doesn't exist, the default
 * value is returned. This is also true if the node is a different type than expected.
 *
 * If the value is empty or the node doesn't exist, the default value is returned instead.
 */
// -------------------------------------------------------------------------------------------------
bool le_cfg_QuickGetBool
(
    const char  *pathStr,       ///< [IN]  Absolute or relative path to read from.
    bool         defaultValue   ///< [IN]  Default value to use if none can be read.
)
{
    NOT_SUPPORTED(WARN);
    return defaultValue;
}

// -------------------------------------------------------------------------------------------------
/**
 * Writes a boolean value to the config tree.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetBool
(
    const char  *pathStr,   ///< [IN] Absolute or relative path to read from.
    bool         value      ///< [IN] Value to write.
)
{
    NOT_SUPPORTED(ERROR);
}


// -------------------------------------------------------------------------------------------------
/**
 *  Delete the node specified by the path.  If the node doesn't exist, nothing happens.  All child
 *  nodes are also deleted.
 *
 *  If the path is empty, the iterator's current node is deleted.
 *
 *  Only valid during a write transaction.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_DeleteNode
(
    le_cfg_IteratorRef_t externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char *pathPtr               ///< [IN] Absolute or relative path to the node to delete.
                                      ///<      If absolute path is given, it's rooted off of the
                                      ///<      user's root node.
)
{
    NOT_SUPPORTED(WARN);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Delete the node specified by the path.  If the node doesn't exist, nothing happens.  All child
 *  nodes are also deleted.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickDeleteNode
(
    const char *pathPtr ///< [IN] Path to the node to delete.
)
{
    NOT_SUPPORTED(WARN);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Read a binary data from the configuration tree.  If the the node has a wrong type, is
 *  empty or doesn't exist, the default value will be returned.
 *
 *  Valid for both read and write transactions.
 *
 *  If the path is empty, the iterator's current node will be read.
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *          - LE_OK             - Read was completed successfully.
 *          - LE_FORMAT_ERROR   - if data can't be decoded.
 *          - LE_OVERFLOW       - Supplied buffer was not large enough to hold the value.
 */
// -------------------------------------------------------------------------------------------------
le_result_t le_cfg_GetBinary
(
    le_cfg_IteratorRef_t iteratorRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char* LE_NONNULL path,      ///< [IN] Path to the target node. Can be an absolute path,
                                      ///<      or a path relative from the iterator's current
                                      ///<      position.
    uint8_t* valuePtr,                ///< [OUT] Buffer to write the value into.
    size_t* valueSizePtr,             ///< [INOUT]
    const uint8_t* defaultValuePtr,   ///< [IN] Default value to use if the original can't be
                                      ///<      read.
    size_t defaultValueSize           ///< [IN] Default value size.
)
{
    NOT_SUPPORTED(WARN);
    return LE_NOT_IMPLEMENTED;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Write a binary data to the configuration tree.  Only valid during a write
 *  transaction.
 *
 *  If the path is empty, the iterator's current node will be set.
 *
 *  @note Binary data cannot be written to the 'system' tree.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetBinary
(
    le_cfg_IteratorRef_t iteratorRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char* LE_NONNULL path,      ///< [IN] Path to the target node. Can be an absolute path, or
                                      ///<      a path relative from the iterator's current position.
    const uint8_t* valuePtr,          ///< [IN] Value to write.
    size_t valueSize                  ///< [IN] Size of the binary data.
)
{
    NOT_SUPPORTED(WARN);
}


// -------------------------------------------------------------------------------------------------
/**
 *  Write a binary value to the configuration tree.
 *
 *  @note Binary data cannot be written to the 'system' tree.
 */
// -------------------------------------------------------------------------------------------------
le_result_t le_cfg_QuickGetBinary
(
    const char* LE_NONNULL path,     ///< [IN] Path to the target node.
    uint8_t* valuePtr,               ///< [OUT] Buffer to write the value into.
    size_t* valueSizePtr,            ///< [INOUT]
    const uint8_t* defaultValuePtr,  ///< [IN] Default value to use if the original can't be read
    size_t defaultValueSize          ///< [IN] Size of default value
)
{
    NOT_SUPPORTED(WARN);
    return LE_NOT_IMPLEMENTED;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Write a binary value to the configuration tree.
 *
 *  @note Binary data cannot be written to the 'system' tree.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetBinary
(
    const char* LE_NONNULL path, ///< [IN] Path to the target node.
    const uint8_t* valuePtr,     ///< [IN] Value to write.
    size_t valueSize             ///< [IN] Size of value
)
{
    NOT_SUPPORTED(WARN);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Change the node that the iterator is pointing to.  The path passed can be an absolute or a
 *  relative path from the iterators current location.
 *
 *  The target node does not need to exist.  When a write iterator is used to go to a non-existant
 *  node, the node is automaticly created when a value is written to it or any of it's children.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_GoToNode
(
    le_cfg_IteratorRef_t externalRef, ///< [IN] Iterator to move.
    const char *newPathPtr            ///< [IN] Absolute or relative path from the current location.
)
{
    NOT_SUPPORTED(WARN);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Move the iterator to the parent of the node for the iterator.
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *          - LE_OK        - Commit was completed successfuly.
 *          - LE_NOT_FOUND - Current node is the root node: has no parent.
 */
// -------------------------------------------------------------------------------------------------
le_result_t le_cfg_GoToParent
(
    le_cfg_IteratorRef_t externalRef ///< [IN] Iterator to move.
)
{
    NOT_SUPPORTED(ERROR);
    return LE_NOT_IMPLEMENTED;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Move the iterator to the the first child of the node that the iterator is currently pointed.
 *
 *  For read iterators without children, this function will fail.  If the iterator is a write
 *  iterator, then a new node is automatically created.  If this node or an newly created
 *  children of this node are not written to, then this node will not persist even if the iterator is
 *  comitted.
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *          - LE_OK        - Move was completed successfuly.
 *          - LE_NOT_FOUND - The given node has no children.
 */
// -------------------------------------------------------------------------------------------------
le_result_t le_cfg_GoToFirstChild
(
    le_cfg_IteratorRef_t externalRef ///< [IN] Iterator object to move.
)
{
    NOT_SUPPORTED(ERROR);
    return LE_NOT_IMPLEMENTED;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Jump the iterator to the next child node of the current node.  Assuming the following tree:
 *
 *  @code
 *  baseNode/
 *    childA/
 *      valueA
 *      valueB
 *  @endcode
 *
 *  If the iterator is moved to the path, "/baseNode/childA/valueA".  After the first
 *  GoToNextSibling the iterator will be pointing at valueB.  A second call to GoToNextSibling
 *  will cause the function to return LE_NOT_FOUND.
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *          - LE_OK            - Commit was completed successfuly.
 *          - LE_NOT_FOUND     - Iterator has reached the end of the current list of siblings.
 *                               Also returned if the the current node has no siblings.
 */
// -------------------------------------------------------------------------------------------------
le_result_t le_cfg_GoToNextSibling
(
    le_cfg_IteratorRef_t externalRef ///< [IN] Iterator to iterate.
)
{
    NOT_SUPPORTED(ERROR);
    return LE_NOT_IMPLEMENTED;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Get path to the node that the iterator is currently pointed.
 *
 *  Assuming the following tree:
 *
 *  @code
 *  baseNode/
 *    childA/
 *      valueA
 *      valueB
 *  @endcode
 *
 *  If the iterator was currently pointing at valueA, then GetPath would return the following path:
 *
 *  @code
 *  /baseNode/childA/valueA
 *  @endcode
 *
 *  Optionally, a path to another node can be supplied to this function.  If the iterator is
 *  again on valueA and the relative path "..." is supplied, this function will return the
 *  following path:
 *
 *  @code
 *  /baseNode/childA/
 *  @endcode
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *          - LE_OK            - The write was completed successfuly.
 *          - LE_OVERFLOW      - The supplied string buffer was not large enough to hold the value.
 */
// -------------------------------------------------------------------------------------------------
le_result_t le_cfg_GetPath
(
    le_cfg_IteratorRef_t externalRef, ///< [IN] Iterator to move.
    const char *pathPtr,              ///< [IN] Absolute or relative path to read from.
    char *newPathStr,                 ///< [OUT] Path
    size_t maxNewPath)                ///< [IN] Maximum size of the result string.
{
    NOT_SUPPORTED(WARN);
    return LE_NOT_IMPLEMENTED;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Get the type of node that the iterator is currently pointing at.
 *
 *  \b Responds \b With:
 *
 *  le_cfg_nodeType_t value indicating the stored value.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_nodeType_t le_cfg_GetNodeType
(
    le_cfg_IteratorRef_t externalRef, ///< [IN] Iterator object to use to read from the tree.
    const char *pathPtr               ///< [IN] Absolute or relative path to read from.
)
{
    NOT_SUPPORTED(WARN);
    return LE_CFG_TYPE_STRING;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Get the name of the node that the iterator is currently pointing at.
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *          - LE_OK       Write was completed successfuly.
 *          - LE_OVERFLOW Supplied string buffer was not large enough to hold the value.
 */
// -------------------------------------------------------------------------------------------------
le_result_t le_cfg_GetNodeName
(
    le_cfg_IteratorRef_t externalRef, ///< [IN] Iterator object to use to read from the tree.
    const char* pathPtr,              ///< [IN] Absolute or relative path to read from.
    char *nameStr,                    /// <[OUT] Node name
    size_t maxName                    ///< [IN] Maximum size of the result string.
)
{
    NOT_SUPPORTED(WARN);
    return LE_NOT_IMPLEMENTED;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Register a call back on a given node object.  Once registered, this function is called if the
 *  node or if any of it's children are read from, written to, created or deleted.
 *
 *  @return A handle to the event registration.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_ChangeHandlerRef_t le_cfg_AddChangeHandler
(
    const char *newPathPtr,                ///< [IN] Path to the object to watch.
    le_cfg_ChangeHandlerFunc_t handlerPtr, ///< [IN] Function to call back.
    void *contextPtr                       ///< [IN] Context to give the function when called.
)
{
    NOT_SUPPORTED(ERROR);
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function removes a handler.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_RemoveChangeHandler
(
    le_cfg_ChangeHandlerRef_t handlerRef ///< [IN] Previously registered handler to remove.
)
{
    NOT_SUPPORTED(WARN);
}
