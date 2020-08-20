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
