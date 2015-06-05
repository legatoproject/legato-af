/**
 * @page c_pa_secStore Secure Storage PA Interface
 *
 * This module contains a platform independent API for secure storage of data.  This API allows the
 * caller to store data in specific paths.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_PA_SECURE_STORAGE_INCLUDE_GUARD
#define LEGATO_PA_SECURE_STORAGE_INCLUDE_GUARD

#include "legato.h"


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
);


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
    size_t* bufSize                 ///< [IN/OUT] Size of buffer when this function is called.
                                    ///          Number of bytes read when this function returns.
);


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
);


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
);


#endif // LEGATO_PA_SECURE_STORAGE_INCLUDE_GUARD
