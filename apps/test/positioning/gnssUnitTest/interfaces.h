/**
 * interfaces.h
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#ifndef _INTERFACES_H
#define _INTERFACES_H

#include "le_gnss_interface.h"
#include "le_cfg_interface.h"

#undef LE_KILL_CLIENT
#define LE_KILL_CLIENT LE_ERROR

//--------------------------------------------------------------------------------------------------
/**
 * Client emum definition.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CLIENT1,
    CLIENT2
}
Client_t;

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_gnss_GetServiceRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_gnss_GetClientSessionRef
(
    void
);

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
);

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
    const char *basePath
);

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
    const char *path,
    int32_t defaultValue
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the client number for simulation of le_gnss_GetClientSessionRef() API.
 */
//--------------------------------------------------------------------------------------------------
void le_gnss_SetClientSimu
(
    Client_t client
);

#endif /* interfaces.h */
