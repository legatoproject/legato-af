/** @file secStoreServer.c
 *
 * Secure Storage Daemon.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * Writes an item to secure storage.  If the item already exists then it will be overwritten with
 * the new value.  If the item does not already exist then it will be created.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NO_MEMORY if there is not enough memory to store the item.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_secStore_Write
(
    const char* name,               ///< [IN] Name of the secure storage item.
    const uint8_t* bufPtr,          ///< [IN] Buffer contain the data to store.
    size_t bufNumElements           ///< [IN] Size of buffer.
)
{
    return LE_UNAVAILABLE;
}


//--------------------------------------------------------------------------------------------------
/**
 * Reads an item from secure storage.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer is too small to hold the entire item.  No data will be written to
 *                  the buffer in this case.
 *      LE_NOT_FOUND if the item does not exist.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_secStore_Read
(
    const char* name,               ///< [IN] Name of the secure storage item.
    uint8_t* bufPtr,                ///< [OUT] Buffer to store the data in.
    size_t* bufNumElementsPtr       ///< [INOUT] Size of buffer.
)
{
    return LE_UNAVAILABLE;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes an item from secure storage.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the item does not exist.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_secStore_Delete
(
    const char* name    ///< [IN] Name of the secure storage item.
)
{
    return LE_UNAVAILABLE;
}


//--------------------------------------------------------------------------------------------------
/**
 * The secure storage daemon's initialization function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}
