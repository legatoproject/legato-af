/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 *  @page api_config Configuration Tree API
 *
 *  @ref le_cfg_interface.h "Click here for the API reference documentation."
 *
 * <HR>
 * @ref cfg_transaction <br>
 * @ref cfg_iteration <br>
 * @ref cfg_transactWrite <br>
 * @ref cfg_transactRead <br>
 * @ref cfg_quick
 *
 *  The configuration tree API allows applications to read and write their specific configuration.
 *  Each application is given an isolated tree.  The system utilities
 *  store their configuration in the "root" tree.
 *
 *  Paths in the tree look like traditional Unix style paths and take the form of:
 *
 *  @code /path/to/my/value @endcode
 *
 *  The path root is the root of the tree where the application has been given access. If the
 *  application has permission to access another tree, the path can also include the name
 *  of the other tree, followed by a colon.
 *
 *  @code secondTree:/path/to/my/value @endcode
 *
 *  In this case, a value named "value" is read from the tree named, "secondTree."
 *
 *  The tree is broken down into stems and leaves.  A stem is a node that has at least one child
 *  node.  While a leaf has no children, but may hold a value.
 *
 *  The configuration tree supports string, signed integer, boolean, floating point, and empty
 *  values.  Storing anything more complex is encouraged to use stems and leafs to enhance
 *  readablity and debug-ablity.  This also sidesteps nasty cross platform alignment
 *  issues.
 *
 *  TODO: Talk about the treeConfig, user limits, tree name, tree access.  Global timeouts.
 *
 *  @section cfg_transaction A Transactional Approach
 *
 *  The configuration tree makes use of simple transactions for working with its data.  Both read
 *  and write transactions are supported.  You want to use read transactions when to
 *  ensure you can atomically read multiple values from your configuration while keeping consistency
 *  with third parties trying to write data.
 *
 * To prevent any single client from locking out other clients, read and
 *  write transactions have their own configurable timeout.
 *
 *  During a write transaction, both reading and writing are allowed.  If you
 *  write a value during a transaction and read from that value again, you will get the same value
 *  you wrote.  Third party clients will continue to see the old value.  It's not until you commit
 *  your transaction that third parties will begin to see your updated value.
 *
 *  During read transactions, writes are not permitted and are thrown away.
 *
 *  Transactions are started by creating an iterator.  Either a read or write iterator can be
 *  created.  To end the transaction, you can either delete the iterator, cancelling the
 *  transaction.  Or, in the case of write transactions, you can commit the iterator.
 *
 *  You can have multiple read transactions against the tree.  They won't
 * block other transactions from being creating.  A read transaction won't block creating a write
 *  transaction either.  A read transaction only blocks a write transaction from being
 *  comitted to the tree.
 *
 *  A write transaction in progress will also block creating another write transaction.
 *  If a write transaction is in progress when the request for another write transaction comes in,
 *  the secondary request will be blocked.  This secondary request will remain blocked until the
 *  first transaction has been comitted or has timed out.
 *
 *
 *  @section cfg_iteration Iterating the Tree
 *
 *  This code example will iterate a given node and print it's contents:
 *
 *  @code
 *  static void PrintNode(le_cfg_IteratorRef_t iteratorRef)
 *  {
 *      do
 *      {
 *          char stringBuffer[MAX_CFG_STRING] = { 0 };
 *
 *          le_cfg_GetNodeName(iteratorRef, &stringBuffer, sizeof(stringBuffer));
 *
 *          switch (le_cfg_GetNodeType(iteratorRef))
 *          {
 *              case LE_CFG_TYPE_STEM:
 *                  {
 *                      printf("%s/\n", stringBuffer);
 *
 *                      if (le_cfg_GoToFirstChild(iteratorRef) == LE_OK)
 *                      {
 *                          PrintNode(iteratorRef);
 *                          le_cfg_GoToNode(iteratorRef, "..");
 *                      }
 *                  }
 *                  break;
 *
 *              case LE_CFG_TYPE_EMPTY:
 *                  printf("%s = *empty*\n", stringBuffer);
 *                  break;
 *
 *              case LE_CFG_TYPE_BOOL:
 *                  {
 *                      bool value = false;
 *
 *                      le_cfg_GetBool(iteratorRef, stringBuffer, &value, false);
 *                      printf("%s = %s\n", stringBuffer, (value ? "true" : "false"));
 *                  }
 *                  break;
 *
 *              case LE_CFG_TYPE_INT:
 *                  {
 *                      int32_t intValue = 0;
 *
 *                      le_cfg_GetInt(iteratorRef, stringBuffer, &intValue, 0);
 *                      printf("%s = %d\n", stringBuffer, intValue);
 *                  }
 *                  break;
 *
 *              case LE_CFG_TYPE_FLOAT:
 *                  {
 *                      double floatValue = 0.0;
 *
 *                      le_cfg_GetFloat(iteratorRef, stringBuffer, &floatValue, 0.0);
 *                      printf("%s = %f\n", stringBuffer, floatValue);
 *                  }
 *                  break;
 *
 *              case LE_CFG_TYPE_STRING:
 *                  printf("%s = ", stringBuffer);
 *                  le_cfg_GetString(iteratorRef, stringBuffer, stringBuffer, "");
 *                  printf("%s\n", stringBuffer);
 *                  break;
 *
 *              case LE_CFG_TYPE_DOESNT_EXIST:
 *                  printf("%s = ** DENIED **\n", stringBuffer);
 *                  break;
 *          }
 *      }
 *      while (le_cfg_GoToNextSibling(iteratorRef) == LE_OK);
 *  }
 *
 *
 *  le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadIterator("/path/to/my/location");
 *
 *  PrintNode(iteratorRef);
 *  le_cfg_CancelTxn(iteratorRef);
 *
 *
 *  @endcode
 *
 *
 *  @section cfg_transactWrite Writing Configuration Data, using a write transaction.
 *
 *  Consider the following example, the caller wants to update the devices IP address.  It does so
 *  in a transaction so that the data is written atomiclly.
 *
 *  @code
 *  static le_result_t SetIp4Static
 *  (
 *      le_cfg_IteratorRef_t currentIterRef,
 *      const char* interfaceNamePtr,
 *      const char* ipAddrPtr,
 *      const char* netMaskPtr
 *  )
 *  {
 *      le_result_t result;
 *
 *      LE_ASSERT(le_cfg_IsWriteable(currentIterRef));
 *
 *      // Change current tree position to the base ip4 node.
 *      char nameBuffer[MAX_CFG_STRING] = { 0 };
 *
 *      sprintf(nameBuffer, "/system/%s/ip4", interfaceNamePtr);
 *
 *      le_cfg_GoToNode(iteratorRef, nameBuffer);
 *
 *      le_cfg_SetString(iteratorRef, "addr", ipAddrPtr);
 *      le_cfg_SetString(iteratorRef, “mask”, netMaskPtr);
 *
 *      result = le_cfg_CommitTxn(iteratorRef);
 *
 *      if (result != LE_OK)
 *      {
 *          LE_CRIT("Failed to write IPv4 configuration for interface '%s'.  Error %s.",
 *                  interfaceNamePtr,
 *                  LE_RESULT_TXT(result));
 *      }
 *
 *      return result;
 *  }
 *  @endcode
 *
 *
 *  @section cfg_transactRead Reading configuration data with a read transaction.
 *
 *  @code
 *  static le_result_t GetIp4Static
 *  (
 *      le_cfg_IteratorRef_t currentIterRef,
 *      const char* interfaceNamePtr,
 *      char* ipAddrPtr,
 *      char* netMaskPtr
 *  )
 *  {
 *      // Change current tree position to the base ip4 node.
 *      char nameBuffer[MAX_CFG_STRING] = { 0 };
 *
 *      sprintf(nameBuffer, "/system/%s/ip4", interfaceNamePtr);
 *      le_cfg_GoToNode(iteratorRef, nameBuffer);
 *      if (le_cfg_NodeExists(iteratorRef, "") == false)
 *      {
 *          LE_WARN("Configuration not found.");
 *          return LE_NOTFOUND;
 *      }
 *
 *      le_cfg_GetString(iteratorRef, "addr", ipAddrPtr, "");
 *      le_cfg_GetString(iteratorRef, "mask", netMaskPtr, "");
 *
 *      return result;
 *  }
 *  @endcode
 *
 *
 *  @section cfg_quick Working without Transactions
 *
 *  It's possible to ignore iterators and transactions entirely (e.g., if all you need to do
 *  is read or write some simple values in the tree).
 *
 *  The non-transactional reads and writes work almost identically to the transactional versions.
 *  They just don't explictly take an iterator object. The "quick" functions internally use an
 *  implicit transaction.  This implicit transaction wraps one get or set, and does not protect
 *  your code from other activity in the system.
 *
 *  Because these functions don't take an explicit transaction, they can't work with relative
 *  paths.  If a relative path is given, the path will be considered relative to the tree's root.
 *
 *  Translating the last examples to their "quick" counterparts, you have the following code.
 *  Because each read is independant, there is no guarantee of
 *  consistency between them.  If another process changes one of the values while you
 *  read/write the other, the two values could be read out of sync.
 *
 *  @code
 *
 *  static void ClearIpInfo
 *  (
 *      const char* interfaceNamePtr
 *  )
 *  {
 *      char pathBuffer[MAX_CFG_STRING] = { 0 };
 *
 *      sprintf(pathBuffer, "/system/%s/ip4/", interfaceNamePtr);
 *      le_cfg_QuickDeleteNode(pathBuffer);
 *  }
 *
 *  @endcode
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */
/** @file le_cfg_interface.h

@ref api_config

*  Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */

#ifndef LE_CFG_INTERFACE_H_INCLUDE_GUARD
#define LE_CFG_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

// User customizable include file
#include "configTypes.h"


//--------------------------------------------------------------------------------------------------
/**
 * Start the service for the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_StopClient
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Reference type for le_cfg_ChangeHandler handler ADD/REMOVE functions
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_cfg_ChangeHandler* le_cfg_ChangeHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 *  Register a call back on a given node object.  Once registered, if that node or if any of it's
 *  children are read from, written to, created or deleted, then this function will be called.
 *
 *  @return Handle to the event registration.
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
 *  Create a read transaction and open a new iterator for traversing the configuration tree.
 *
 *  @note: This action creates a read lock on the given tree.  Which will start a read-timeout.
 *         Once the read timeout expires, then all active read iterators on that tree will be
 *         expired and the clients killed.
 *
 *  @note: A tree transaction is global to that tree; a long held read transaction will block other
 *         users write transactions from being comitted.
 *
 *  @return This will return a newly created iterator reference.
 */
//--------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t le_cfg_CreateReadTxn
(
    const char* basePath
        ///< [IN]
        ///< Path to the location to create the new iterator.
);

//--------------------------------------------------------------------------------------------------
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
 *  @return This will return a newly created iterator reference.
 */
//--------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t le_cfg_CreateWriteTxn
(
    const char* basePath
        ///< [IN]
        ///< Path to the location to create the new iterator.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Close the write iterator and commit the write transaction.  This updates the config tree
 *  with all of the writes that occured using the iterator.
 *
 *  @note: This operation will also delete the iterator object.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_CommitTxn
(
    le_cfg_IteratorRef_t iteratorRef
        ///< [IN]
        ///< Iterator object to commit.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Close and free the given iterator object.  If the iterator is a write iterator, the transaction
 *  will be canceled.  If the iterator is a read iterator, the transaction will be closed.
 *
 *  @note: This operation will also delete the iterator object.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_CancelTxn
(
    le_cfg_IteratorRef_t iteratorRef
        ///< [IN]
        ///< Iterator object to close.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Change the node that the iterator is pointing to.  The path passed can be an absolute or a
 *  relative path from the iterators current location.
 *
 *  The target node does not need to exist.  When a write iterator is used to go to a non-existant
 *  node, the node is automaticly created when a value is written to it or any of it's children.
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
);

//--------------------------------------------------------------------------------------------------
/**
 *  Move the iterator to the parent of the node.
 *
 *  @return Return code will be one of the following values:
 *
 *          - LE_OK        - Commit was completed successfully.
 *          - LE_NOT_FOUND - Current node is the root node: has no parent.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfg_GoToParent
(
    le_cfg_IteratorRef_t iteratorRef
        ///< [IN]
        ///< Iterator to move.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Move the iterator to the the first child of the node where the iterator is currently pointed.
 *
 *  For read iterators without children, this function will fail. If the iterator is a write
 *  iterator, then a new node is automatically created.  If this node or newly created
 *  children of this node are not written to, then this node will not persist even if the iterator is
 *  comitted.
 *
 *  @return Return code will be one of the following values:
 *
 *          - LE_OK        - Move was completed successfully.
 *          - LE_NOT_FOUND - The given node has no children.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfg_GoToFirstChild
(
    le_cfg_IteratorRef_t iteratorRef
        ///< [IN]
        ///< Iterator object to move.
);

//--------------------------------------------------------------------------------------------------
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
 *  @return Returns one of the following values:
 *
 *          - LE_OK            - Commit was completed successfully.
 *          - LE_NOT_FOUND     - Iterator has reached the end of the current list of siblings.
 *                               Also returned if the the current node has no siblings.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfg_GoToNextSibling
(
    le_cfg_IteratorRef_t iteratorRef
        ///< [IN]
        ///< Iterator to iterate.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Get path to the node where the iterator is currently pointed.
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
 *  If the iterator was currently pointing at valueA, GetPath would return the following path:
 *
 *  @code
 *  /baseNode/childA/valueA
 *  @endcode
 *
 *  Optionally, a path to another node can be supplied to this function.  So, if the iterator is
 *  again on valueA and the relative path ".." is supplied then this function will return the
 *  following path:
 *
 *  @code
 *  /baseNode/childA/
 *  @endcode
 *
 *  @return - LE_OK            - The write was completed successfully.
 *          - LE_OVERFLOW      - The supplied string buffer was not large enough to hold the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfg_GetPath
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to move.

    const char* path,
        ///< [IN]
        ///< Path to the target node.  Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    char* pathBuffer,
        ///< [OUT]
        ///< Absolute path to the iterator's current node.

    size_t pathBufferNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 *  Get the type of node where the iterator is currently pointing.
 *
 *  @return le_cfg_nodeType_t value indicating the stored value.
 */
//--------------------------------------------------------------------------------------------------
le_cfg_nodeType_t le_cfg_GetNodeType
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator object to use to read from the tree.

    const char* path
        ///< [IN]
        ///< Path to the target node.  Can be an absolute path, or
        ///< a path relative from the iterator's current position.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Get the name of the node where the iterator is currently pointing.
 *
 *  @return - LE_OK       Read was completed successfully.
 *          - LE_OVERFLOW Supplied string buffer was not large enough to hold the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfg_GetNodeName
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator object to use to read from the tree.

    const char* path,
        ///< [IN]
        ///< Path to the target node.  Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    char* name,
        ///< [OUT]
        ///< Read the name of the node object.

    size_t nameNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 *  Change the name of the node that the iterator is currently pointing at.
 *
 *  @return
 *      - LE_OK           Write was completed successfully.
 *      - LE_FORMAT_ERROR The new name included illegial characters, '/' or used one of the reserved
 *                        names: '.', or '..'.  Format error is also returned if the new name is
 *                        NULL or empty.
 *      - LE_DUPLICATE    If there is another node with the new name in the same collection.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfg_SetNodeName
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator object to use to read from the tree.

    const char* path,
        ///< [IN]
        ///< Path to the target node.  Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    const char* name
        ///< [IN]
        ///< New name for the node object.
);

//--------------------------------------------------------------------------------------------------
/**
 * le_cfg_ChangeHandler handler ADD function
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
);

//--------------------------------------------------------------------------------------------------
/**
 * le_cfg_ChangeHandler handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_RemoveChangeHandler
(
    le_cfg_ChangeHandlerRef_t addHandlerRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 *  Delete the node specified by the path.  If the node doesn't exist, nothing happens.  All child
 *  nodes are also deleted.
 *
 *  If the path is empty, then the iterator's current node is deleted.
 *
 *  This function is only valid during a write transaction.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_DeleteNode
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path
        ///< [IN]
        ///< Path to the target node.  Can be an absolute path, or
        ///< a path relative from the iterator's current position.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Check if the given node is empty.  A node is also considered empty if it doesn't yet exist.  A
 *  node is also considered empty if it has no value or is a stem with no children.
 *
 *  If the path is empty, then the iterator's current node is queried for emptiness.
 *
 *  This function is valid for both read and write transactions.
 *
 *  @return A true if the node is considered empty, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool le_cfg_IsEmpty
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path
        ///< [IN]
        ///< Path to the target node.  Can be an absolute path, or
        ///< a path relative from the iterator's current position.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Clear out the nodes's value.  If it doesn't exist it will be created, but have no value.
 *
 *  If the path is empty, then the iterator's current node will be cleared.  If the node is a stem
 *  then all children will be removed from the tree.
 *
 *  This function is only valid during a write transaction.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_SetEmpty
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path
        ///< [IN]
        ///< Path to the target node.  Can be an absolute path, or
        ///< a path relative from the iterator's current position.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Check to see if a given node in the configuration tree exists.
 *
 *  @return True if the specified node exists in the tree.  False if not.
 */
//--------------------------------------------------------------------------------------------------
bool le_cfg_NodeExists
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path
        ///< [IN]
        ///< Path to the target node.  Can be an absolute path, or
        ///< a path relative from the iterator's current position.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Read a string value from the configuration tree.  If the value isn't a string, or if the node is
 *  empty or doesn't exist then the default value will be returned.
 *
 *  This function is valid for both read and write transactions.
 *
 *  If the path is empty, then the iterator's current node will be read.
 *
 *  @return - LE_OK       - Read was completed successfully.
 *          - LE_OVERFLOW - Supplied string buffer was not large enough to hold the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfg_GetString
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node.  Can be an absolute path,
        ///< or a path relative from the iterator's current
        ///< position.

    char* value,
        ///< [OUT]
        ///< Buffer to write the value into.

    size_t valueNumElements,
        ///< [IN]

    const char* defaultValue
        ///< [IN]
        ///< The default value to use if the original can not be
        ///<   read.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Write a string value to the configuration tree.  This function is only valid during a write
 *  transaction.
 *
 *  If the path is empty, then the iterator's current node will be set.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_SetString
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node.  Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    const char* value
        ///< [IN]
        ///< Value to write.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Read a signed integer value from the configuration tree.
 *
 *  If the underlying value is not an integer, the default value will be returned instead.  The
 *  default value is also returned if the node does not exist or if it's empty.
 *
 *  If the value is a floating point value, then it will be rounded and returned as an integer.
 *
 *  This function is valid for both read and write transactions.
 *
 *  If the path is empty, then the iterator's current node will be read.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_cfg_GetInt
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node.  Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    int32_t defaultValue
        ///< [IN]
        ///< The default value to use if the original can not be
        ///<   read.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Write a signed integer value to the configuration tree.  This function is only valid during a
 *  write transaction.
 *
 *  If the path is empty, then the iterator's current node will be set.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_SetInt
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node.  Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    int32_t value
        ///< [IN]
        ///< Value to write.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Read a 64-bit floating point value from the configuration tree.
 *
 *  If the value is an integer then the value will be promoted to a float.  Otherwise, if the
 *  underlying value is not a float or integer, the default value will be returned.
 *
 *  If the path is empty, then the iterator's current node will be read.
 */
//--------------------------------------------------------------------------------------------------
double le_cfg_GetFloat
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node.  Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    double defaultValue
        ///< [IN]
        ///< The default value to use if the original can not be
        ///<   read.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Write a 64-bit floating point value to the configuration tree.  This function is only valid
 *  during a write transaction.
 *
 *  If the path is empty, then the iterator's current node will be set.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_SetFloat
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node.  Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    double value
        ///< [IN]
        ///< Value to write.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Read a value from the tree as a boolean.  If the node is empty or doesn't exist then the default
 *  value is returned.  The default value is also returned if the node is of a different type than
 *  expected.
 *
 *  This function is valid for both read and write transactions.
 *
 *  If the path is empty, then the iterator's current node will be read.
 */
//--------------------------------------------------------------------------------------------------
bool le_cfg_GetBool
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node.  Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    bool defaultValue
        ///< [IN]
        ///< The default value to use if the original can not be
        ///<   read.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Write a boolean value to the configuration tree.  This function is only valid during a write
 *  transaction.
 *
 *  If the path is empty, then the iterator's current node will be set.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_SetBool
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node.  Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    bool value
        ///< [IN]
        ///< Value to write.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Delete the node specified by the path.  If the node doesn't exist, nothing happens.  All child
 *  nodes are also deleted.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_QuickDeleteNode
(
    const char* path
        ///< [IN]
        ///< Path to the node to delete.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Make a given node empty.  If the node doesn't currently exist then it is created as a new empty
 *  node.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_QuickSetEmpty
(
    const char* path
        ///< [IN]
        ///< Absolute or relative path to read from.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Read a string value from the configuration tree.  If the value isn't a string, or if the node is
 *  empty or doesn't exist then the default value will be returned.
 *
 *  @return - LE_OK       - Commit was completed successfully.
 *          - LE_OVERFLOW - Supplied string buffer was not large enough to hold the value.
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
        ///< The default value to use if the original can not be read.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Write a string value to the configuration tree.
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
);

//--------------------------------------------------------------------------------------------------
/**
 *  Read a signed integer value from the configuration tree.  If the value is a floating point
 *  value, then it will be rounded and returned as an integer.  Otherwise If the underlying value is
 *  not an integer or a float, the default value will be returned instead.
 *
 *  If the value is empty or the node doesn't exist then the default value is returned instead.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_cfg_QuickGetInt
(
    const char* path,
        ///< [IN]
        ///< Path to the value to write.

    int32_t defaultValue
        ///< [IN]
        ///< The default value to use if the original can not be read.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Write a signed integer value to the configuration tree.
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
);

//--------------------------------------------------------------------------------------------------
/**
 *  Read a 64-bit floating point value from the configuration tree.  If the value is an integer,
 *  then it is promoted to a float.  Otherwise, if the underlying value is not a float, or an
 *  integer the default value will be returned.
 *
 *  If the value is empty or the node doesn't exist then the default value is returned.
 */
//--------------------------------------------------------------------------------------------------
double le_cfg_QuickGetFloat
(
    const char* path,
        ///< [IN]
        ///< Path to the value to write.

    double defaultValue
        ///< [IN]
        ///< The default value to use if the original can not be read.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Write a 64-bit floating point value to the configuration tree.
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
);

//--------------------------------------------------------------------------------------------------
/**
 *  Read a value from the tree as a boolean.  If the node is empty or doesn't exist then the default
 *  value is returned.  This is also true if the node is of a different type than expected.
 *
 *  If the value is empty or the node doesn't exist then the default value is returned instead.
 */
//--------------------------------------------------------------------------------------------------
bool le_cfg_QuickGetBool
(
    const char* path,
        ///< [IN]
        ///< Path to the value to write.

    bool defaultValue
        ///< [IN]
        ///< The default value to use if the original can not be read.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Write a boolean value to the configuration tree.
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
);


#endif // LE_CFG_INTERFACE_H_INCLUDE_GUARD

