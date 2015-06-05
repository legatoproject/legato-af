//--------------------------------------------------------------------------------------------------
/**
 * @file pa_secStore_default.c
 *
 * Default implementation of @ref c_pa_secStore interface
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "pa_secStore.h"


//--------------------------------------------------------------------------------------------------
/**
 * Writes the data in the buffer to the specified path in secure storage replacing any previously
 * written data at the same path.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NO_MEMORY if there is not enough memory to store the data.  No data will be written in
 *                   this case.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_secStore_Write
(
    const char* pathPtr,            ///< [IN] Path to write to.
    const uint8_t* bufPtr,          ///< [IN] Buffer containing the data to write.
    size_t bufSize                  ///< [IN] Size of the buffer.
)
{
    return LE_UNAVAILABLE;
}


//--------------------------------------------------------------------------------------------------
/**
 * Reads an data from the specified path in secure storage.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer is too small to hold all the data.  No data will be written to the
 *                  buffer in this case.
 *      LE_NOT_FOUND if the path is empty.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_secStore_Read
(
    const char* pathPtr,            ///< [IN] Path to read from.
    uint8_t* bufPtr,                ///< [OUT] Buffer to store the data in.
    size_t* bufSize                 ///< [INOUT] Size of buffer when this function is called.
                                    ///          Number of bytes read when this function returns.
)
{
    return LE_UNAVAILABLE;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes the specified path and everything under it.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the path does not exist.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_secStore_Delete
(
    const char* pathPtr             ///< [IN] Path to delete.
)
{
    return LE_UNAVAILABLE;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the size, in bytes, of the data at the specified path and everything under it.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the path does not exist.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_secStore_GetSize
(
    const char* pathPtr,            ///< [IN] Path.
    size_t* sizePtr                 ///< [OUT] Size in bytes of all items in the path.
)
{
    return LE_UNAVAILABLE;
}


//--------------------------------------------------------------------------------------------------
/**
 * Init this component
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}

