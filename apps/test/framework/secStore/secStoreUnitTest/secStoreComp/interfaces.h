#include "secStoreAdmin_interface.h"
#include "le_secStore_interface.h"
#include "le_appInfo_interface.h"
#include "le_update_interface.h"

#define LE_APPINFO_DEFAULT_APPNAME "secStoreUnitTest"

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t secStoreAdmin_GetClientSessionRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_secStore_GetClientSessionRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t secStoreAdmin_GetServiceRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/*
 * FIXME: Declaring secStoreGlobal here since I can't seem to be able to include an api as another
 * name than the api.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Writes an item to secure storage.  If the item already exists then it will be overwritten with
 * the new value.  If the item does not already exist then it will be created.  Specifying 0 for
 * buffer size means emptying an existing file or creating a 0-byte file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NO_MEMORY if there is not enough memory to store the item.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t secStoreGlobal_Write
(
    const char* name,               ///< [IN] Name of the secure storage item.
    const uint8_t* bufPtr,          ///< [IN] Buffer contain the data to store.
    size_t bufNumElements           ///< [IN] Size of buffer.
);

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
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t secStoreGlobal_Read
(
    const char* name,               ///< [IN] Name of the secure storage item.
    uint8_t* bufPtr,                ///< [OUT] Buffer to store the data in.
    size_t* bufNumElementsPtr       ///< [INOUT] Size of buffer.
);

//--------------------------------------------------------------------------------------------------
/**
 * Deletes an item from secure storage.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the item does not exist.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t secStoreGlobal_Delete
(
    const char* name    ///< [IN] Name of the secure storage item.
);