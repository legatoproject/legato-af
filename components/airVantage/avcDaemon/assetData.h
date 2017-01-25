/**
 * @file assetData.h
 *
 * Asset Data Interface
 *
 * This interface provides functions to access the assetData store.  Some functions are client or
 * server specific, while other functions can be used by either the client or the server.
 *
 * The server functions should only be used by code that is directly handling communication with
 * the AirVantage server; the client functions should be used by all other code. The main purpose
 * for the two sets of functions is to enforce access restrictions, e.g. a field may be writable
 * by the server, but only readable by clients.
 *
 *
 * TODO:
 *  - add APIs for getting/setting opaque/binary data
 *  - add APIs for iterating over instances or fields.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#ifndef LEGATO_ASSET_DATA_INCLUDE_GUARD
#define LEGATO_ASSET_DATA_INCLUDE_GUARD

#include "legato.h"
#include "pa_avc.h"


//--------------------------------------------------------------------------------------------------
/**
 *  Name of the Legato framework object.
 */
//--------------------------------------------------------------------------------------------------
#define ASSET_DATA_LEGATO_OBJ_NAME "legato"


//--------------------------------------------------------------------------------------------------
/**
 *  Number of maps (called objects in JSON) in the CBOR encoded data (header, factor & sample).
 */
//--------------------------------------------------------------------------------------------------
#define NUM_TIME_SERIES_MAPS 3


//--------------------------------------------------------------------------------------------------
/**
 *  Number of bytes reserved in the CBOR stream. After all the samples are added to CBOR stream few
 *  more bytes are needed to close the sample array and the stream.
 */
//--------------------------------------------------------------------------------------------------
#define CBOR_RESERVED_BYTES 32


//--------------------------------------------------------------------------------------------------
/**
 * Actions that can happen on field or asset
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    ASSET_DATA_ACTION_CREATE,
    ASSET_DATA_ACTION_DELETE,
    ASSET_DATA_ACTION_READ,
    ASSET_DATA_ACTION_WRITE,        // TODO: should this be UPDATE instead?
    ASSET_DATA_ACTION_EXEC          // TODO: should this be NOTIFY instead?
}
assetData_ActionTypes_t;


//--------------------------------------------------------------------------------------------------
/**
 * Session start information.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    ASSET_DATA_SESSION_AVAILABLE,
    ASSET_DATA_SESSION_UNAVAILABLE
}
assetData_SessionTypes_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reference to asset data.
 */
//--------------------------------------------------------------------------------------------------
typedef struct assetData_AssetData* assetData_AssetDataRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reference to an asset data instance.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_avdata_AssetInstance* assetData_InstanceDataRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reference used by the AddFieldActionHandler/RemoveFieldActionHandler functions.
 */
//--------------------------------------------------------------------------------------------------
typedef struct assetData_FieldActionHandler* assetData_FieldActionHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Definition of handler passed to AddFieldActionHandler().
 *
 * @param instanceRef                           ///< [IN] Asset instance the action occurred on.
 * @param fieldId                               ///< [IN] The field the action occurred on.
   @param action,                               ///< [IN] The action that occurred.
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*assetData_FieldActionHandlerFunc_t)
(
    assetData_InstanceDataRef_t instanceRef,
    int fieldId,
    assetData_ActionTypes_t action,
    void* contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Reference used by the AddAssetActionHandler/RemoveAssetActionHandler functions.
 */
//--------------------------------------------------------------------------------------------------
typedef struct assetData_AssetActionHandler* assetData_AssetActionHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Definition of handler passed to AddAssetActionHandler().
 *
 * @param assetRef                              ///< [IN] Asset reference the action occurred on.
 * @param instanceId                            ///< [IN] The instance the action occurred on.
   @param assetData_ActionTypes_t action,       ///< [IN] The action that occurred.
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*assetData_AssetActionHandlerFunc_t)
(
    assetData_AssetDataRef_t assetRef,
    int instanceId,
    assetData_ActionTypes_t action,
    void* contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Write the formatted string to a buffer
 *
 * @return:
 *      - LE_OK on success
 *      - LE_OVERFLOW if buffer is too small
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t FormatString
(
    char* strBufPtr,
    size_t strBufSize,
    const char* formatPtr,
    ...
);


//--------------------------------------------------------------------------------------------------
/**
 * Create a new instance of the given asset
 *
 * @return:
 *      - LE_OK on success
 *      - LE_DUPLICATE if the specified instanceId already exists
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_CreateInstanceById
(
    const char* appNamePtr,                         ///< [IN] App containing the asset
    int assetId,                                    ///< [IN] Asset id within the App
    int instanceId,                                 ///< [IN] If < 0, then generate an instance id,
                                                    ///       otherwise use the given id.
    assetData_InstanceDataRef_t* instanceRefPtr     ///< [OUT] Reference to created instance
);


//--------------------------------------------------------------------------------------------------
/**
 * Create a new instance of the given asset
 *
 * @return:
 *      - LE_OK on success
 *      - LE_DUPLICATE if the specified instanceId already exists
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_CreateInstanceByName
(
    const char* appNamePtr,                         ///< [IN] App containing the asset
    const char* assetNamePtr,                       ///< [IN] Asset name within the App
    int instanceId,                                 ///< [IN] If < 0, then generate an instance id,
                                                    ///       otherwise use the given id.
    assetData_InstanceDataRef_t* instanceRefPtr     ///< [OUT] Reference to created instance
);


//--------------------------------------------------------------------------------------------------
/**
 * Delete the given asset instance
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void assetData_DeleteInstance
(
    assetData_InstanceDataRef_t instanceRef         ///< [IN] Asset instance to delete
);


//--------------------------------------------------------------------------------------------------
/**
 * Delete the given asset instance, and if no more instances, also delete the asset data.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void assetData_DeleteInstanceAndAsset
(
    assetData_InstanceDataRef_t instanceRef         ///< [IN] Asset instance to delete
);


//--------------------------------------------------------------------------------------------------
/**
 * Get a reference for the specified asset
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if asset not found
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_GetAssetRefById
(
    const char* appNamePtr,                         ///< [IN] App containing the asset
    int assetId,                                    ///< [IN] Asset id within the App
    assetData_AssetDataRef_t* assetRefPtr           ///< [OUT] Reference to specified asset
);


//--------------------------------------------------------------------------------------------------
/**
 * Get a reference for the specified asset
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if asset not found
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_GetAssetRefByName
(
    const char* appNamePtr,                         ///< [IN] App containing the asset
    const char* assetNamePtr,                       ///< [IN] Asset name within the App
    assetData_AssetDataRef_t* assetRefPtr           ///< [OUT] Reference to specified asset
);


//--------------------------------------------------------------------------------------------------
/**
 * Get a reference for the specified asset instance
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if instance not found
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_GetInstanceRefById
(
    const char* appNamePtr,                         ///< [IN] App containing the asset
    int assetId,                                    ///< [IN] Asset id within the App
    int instanceId,                                 ///< [IN] Instance of the given asset
    assetData_InstanceDataRef_t* instanceRefPtr     ///< [OUT] Reference to specified instance
);


//--------------------------------------------------------------------------------------------------
/**
 * Get a reference for the specified asset instance
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if instance not found
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_GetInstanceRefByName
(
    const char* appNamePtr,                         ///< [IN] App containing the asset
    const char* assetNamePtr,                       ///< [IN] Asset name within the App
    int instanceId,                                 ///< [IN] Instance of the given asset
    assetData_InstanceDataRef_t* instanceRefPtr     ///< [OUT] Reference to specified instance
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the App name for the specified asset
 *
 * @return:
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_GetAppNameFromAsset
(
    assetData_AssetDataRef_t assetRef,          ///< [IN] Asset data to use
    char* nameBufPtr,                           ///< [OUT] The App name
    size_t nameBufNumBytes                      ///< [IN] Size of nameBuf
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the Asset id for the specified asset
 *
 * @return:
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_GetAssetIdFromAsset
(
    assetData_AssetDataRef_t assetRef,          ///< [IN] Asset data to use
    int* assetIdPtr                             ///< [OUT] The Asset id
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the App name for the specified asset instance
 *
 * @return:
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_GetAppNameFromInstance
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    char* nameBufPtr,                           ///< [OUT] The App name
    size_t nameBufNumBytes                      ///< [IN] Size of nameBuf
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the Asset id for the specified asset instance
 *
 * @return:
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_GetAssetIdFromInstance
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int* assetIdPtr                             ///< [OUT] The Asset id
);


//--------------------------------------------------------------------------------------------------
/**
 * Get a reference to the Asset from the specified asset instance
 *
 * @return:
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
le_result_t assetData_GetAssetRefFromInstance
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    assetData_AssetDataRef_t* assetRefPtr       ///< [OUT] Reference to specified asset
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the instance id for the specified asset instance
 *
 * @return:
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_GetInstanceId
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int* instanceIdPtr                          ///< [OUT] The instance id
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the field id for the given field name
 *
 * @return:
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
le_result_t assetData_GetFieldIdFromName
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    const char* fieldNamePtr,                   ///< [IN] The field name
    int* fieldIdPtr                             ///< [OUT] The field id
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the integer value for the specified field
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_client_GetInt
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to read
    int* valuePtr                               ///< [OUT] The value read
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the float value for the specified field
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_client_GetFloat
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to read
    double* valuePtr                            ///< [OUT] The value read
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the integer value for the specified field
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_client_SetInt
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to write
    int value                                   ///< [IN] The value to write
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the float value for the specified field
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_client_SetFloat
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to write
    double value                                ///< [IN] The value to write
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the bool value for the specified field
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_client_GetBool
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to read
    bool* valuePtr                              ///< [OUT] The value read
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the bool value for the specified field
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_client_SetBool
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to write
    bool value                                  ///< [IN] The value to write
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the string value for the specified field
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_OVERFLOW if string value was truncated when copied to strBufPtr
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_client_GetString
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to read
    char* strBufPtr,                            ///< [OUT] The value read
    size_t strBufNumBytes                       ///< [IN] Size of strBuf
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the string value for the specified field
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_OVERFLOW if the stored string was truncated
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_client_SetString
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to write
    const char* strPtr                          ///< [IN] The value to write
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a handler to be notified on field actions, such as write or execute
 *
 * @return:
 *      - On success, reference for removing handler (with assetData_client_RemoveFieldActionHandler)
 *      - On error, NULL
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED assetData_FieldActionHandlerRef_t assetData_client_AddFieldActionHandler
(
    assetData_AssetDataRef_t assetRef,                  ///< [IN] Asset to monitor
    int fieldId,                                        ///< [IN] Field within asset to monitor
    assetData_FieldActionHandlerFunc_t handlerPtr,      ///< [IN] Handler to call upon action
    void* contextPtr                                    ///< [IN] User specified context pointer
);


//--------------------------------------------------------------------------------------------------
/**
 * Remove a handler previously registered with assetData_client_AddFieldActionHandler
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void assetData_client_RemoveFieldActionHandler
(
    assetData_FieldActionHandlerRef_t handlerRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a handler to be notified on asset actions, such as create or delete instance.
 *
 * @return:
 *      - On success, reference for removing handler (with assetData_client_RemoveAssetActionHandler)
 *      - On error, NULL
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED assetData_AssetActionHandlerRef_t assetData_client_AddAssetActionHandler
(
    assetData_AssetDataRef_t assetRef,                  ///< [IN] Asset to monitor
    assetData_AssetActionHandlerFunc_t handlerPtr,      ///< [IN] Handler to call upon action
    void* contextPtr                                    ///< [IN] User specified context pointer
);


//--------------------------------------------------------------------------------------------------
/**
 * Remove a handler previously registered with assetData_client_AddAssetActionHandler
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void assetData_client_RemoveAssetActionHandler
(
    assetData_AssetActionHandlerRef_t handlerRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the integer value for the specified field
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_server_GetInt
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to read
    int* valuePtr                               ///< [OUT] The value read
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the integer value for the specified field
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_server_SetInt
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to write
    int value                                   ///< [IN] The value to write
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the bool value for the specified field
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_server_GetBool
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to read
    bool* valuePtr                              ///< [OUT] The value read
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the bool value for the specified field
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_server_SetBool
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to write
    bool value                                  ///< [IN] The value to write
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the string value for the specified field
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_OVERFLOW if string value was truncated when copied to strBufPtr
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_server_GetString
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to read
    char* strBufPtr,                            ///< [OUT] The value read
    size_t strBufNumBytes                       ///< [IN] Size of strBuf
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the string value for the specified field
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_OVERFLOW if the stored string was truncated
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_server_SetString
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to write
    const char* strPtr                          ///< [IN] The value to write
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the value for the specified field as a string. This function will return LE_UNAVAILABLE, if
 * a callback function is registered for this operation. A response will be sent to the server
 * after the callback function finishes.
 *
 * If the field is not a string field, then the value will be converted to a string.
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_UNAVAILABLE if a read call back function is registered
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_server_GetValue
(
    pa_avc_LWM2MOperationDataRef_t opRef,       ///< [IN] Reference to LWM2M operation
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to read
    char* strBufPtr,                            ///< [OUT] The value read
    size_t strBufNumBytes                       ///< [IN] Size of strBuf
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the value for the specified field from a string
 *
 * If the field is not a string field, then the string will be converted to the field type
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_OVERFLOW if the stored value was truncated
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_server_SetValue
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to write
    const char* strPtr                          ///< [IN] The value to write
);


//--------------------------------------------------------------------------------------------------
/**
 * Perform an execute action on the specified field
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_server_Execute
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId                                 ///< [IN] Field to execute
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a handler to be notified on field actions, such as write or execute
 *
 * @return:
 *      - On success, reference for removing handler (with assetData_server_RemoveFieldActionHandler)
 *      - On error, NULL
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED assetData_FieldActionHandlerRef_t assetData_server_AddFieldActionHandler
(
    assetData_AssetDataRef_t assetRef,                  ///< [IN] Asset to monitor
    int fieldId,                                        ///< [IN] Field within asset to monitor
    assetData_FieldActionHandlerFunc_t handlerPtr,      ///< [IN] Handler to call upon action
    void* contextPtr                                    ///< [IN] User specified context pointer
);


//--------------------------------------------------------------------------------------------------
/**
 * Remove a handler previously registered with assetData_server_AddFieldActionHandler
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void assetData_server_RemoveFieldActionHandler
(
    assetData_FieldActionHandlerRef_t handlerRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a handler to be notified on asset actions, such as create or delete instance.
 *
 * @return:
 *      - On success, reference for removing handler (with assetData_server_RemoveAssetActionHandler)
 *      - On error, NULL
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED assetData_AssetActionHandlerRef_t assetData_server_AddAssetActionHandler
(
    assetData_AssetDataRef_t assetRef,                  ///< [IN] Asset to monitor
    assetData_AssetActionHandlerFunc_t handlerPtr,      ///< [IN] Handler to call upon action
    void* contextPtr                                    ///< [IN] User specified context pointer
);


//--------------------------------------------------------------------------------------------------
/**
 * Remove a handler previously registered with assetData_server_AddAssetActionHandler
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void assetData_server_RemoveAssetActionHandler
(
    assetData_AssetActionHandlerRef_t handlerRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Set handler to be notified on asset actions, such as create or delete instance, for all assets.
 *
 * @todo: For now, only one handler can be registered.  If support for multiple handlers is needed
 *        then this can be added in the future.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void assetData_server_SetAllAssetActionHandler
(
    assetData_AssetActionHandlerFunc_t handlerPtr,      ///< [IN] Handler to call upon action
    void* contextPtr                                    ///< [IN] User specified context pointer
);


//--------------------------------------------------------------------------------------------------
/**
 * Init this sub-component
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * QMI Specific Functions
 *
 * The remaining functions below are for generating or reading data related to QMI messages.
 * Eventually, these may be moved into lwm2m.c, but are here for now, because they have to iterate
 * over the AssetData instances and fields.  Until an appropriate iteration interface is provided,
 * they need direct access to the data.
 */
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Write a list of readable LWM2M Resource TLVs to the given buffer.
 *
 * @return:
 *      - LE_OK on success
 *      - LE_OVERFLOW if the TLV data could not fit in the buffer
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t assetData_WriteFieldListToTLV
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    uint8_t* bufPtr,                            ///< [OUT] Buffer for writing the header
    size_t bufNumBytes,                         ///< [IN] Size of buffer
    size_t* numBytesWrittenPtr                  ///< [OUT] # bytes written to buffer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Write TLV with all instances of the LWM2M Object to the given buffer.
 *
 * @return:
 *      - LE_OK on success
 *      - LE_OVERFLOW if the TLV data could not fit in the buffer
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t assetData_WriteObjectToTLV
(
    assetData_AssetDataRef_t assetRef,          ///< [IN] Asset to use
    int fieldId,                                ///< [IN] Field to write, or -1 for all fields
    uint8_t* bufPtr,                            ///< [OUT] Buffer for writing the header
    size_t bufNumBytes,                         ///< [IN] Size of buffer
    size_t* numBytesWrittenPtr                  ///< [OUT] # bytes written to buffer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Read a list of LWM2M Resource TLVs from the given buffer and write to the given instance
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t assetData_ReadFieldListFromTLV
(
    uint8_t* bufPtr,                            ///< [IN] Buffer for reading the TLV list
    size_t bufNumBytes,                         ///< [IN] # bytes in the buffer
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to write the fields
    bool isCallHandlers                         ///< [IN] Call field callback handlers?
);


//--------------------------------------------------------------------------------------------------
/**
 * Enables or Disables a field for observe.
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the object instance is not available
 */
//--------------------------------------------------------------------------------------------------
le_result_t assetData_SetObserve
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    bool isObserve,                             ///< [IN] Start or stop observing?
    uint8_t *tokenPtr,                          ///< [IN] Token i.e, request id of the observe request
    uint8_t tokenLength                         ///< [IN] Token Length
);


//--------------------------------------------------------------------------------------------------
/**
 * Set Observe on all instances.
 *
 * @return:
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
le_result_t assetData_SetObserveAllInstances
(
    assetData_AssetDataRef_t assetRef,          ///< [IN] Asset to use
    bool isObserve,                             ///< [IN] true = Observe; false = Cancel Observe
    uint8_t *tokenPtr,                          ///< [IN] Token
    uint8_t tokenLength                         ///< [IN] Token Length
);

//--------------------------------------------------------------------------------------------------
/**
 * Is Observe flag set for object9 state and result fields.
 *
 * @return:
 *      - true if the flags are set
 *      - false if not able to read the flags or if the flags are not set
 */
//--------------------------------------------------------------------------------------------------
bool assetData_IsObject9Observed
(
    assetData_InstanceDataRef_t obj9InstRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Cancels observe on entire asset map.
 */
//--------------------------------------------------------------------------------------------------
void assetData_CancelAllObserve
(
    void
);



//--------------------------------------------------------------------------------------------------
/**
 * Allocate resources and start accumulating time series data on the specified field.
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_BUSY if time series already enabled on this field
 *      - LE_FAULT any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t assetData_client_StartTimeSeries
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to be accumulated
    double factor,                              ///< [IN] Factor used for delta encoding of values
    double timeStampFactor                      ///< [IN] Factor used for delta encoding of time stamp
);




//--------------------------------------------------------------------------------------------------
/**
 * Stop time series on this field and free resources.
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_CLOSED if timeseries already stopped
 */
//--------------------------------------------------------------------------------------------------
le_result_t assetData_client_StopTimeSeries
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId                                 ///< [IN] Field to be stopped from data accumulation
);


//--------------------------------------------------------------------------------------------------
/**
 * Compress the accumulated CBOR encoded time series data and send it to server.
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_CLOSED if time series not enabled on this field
 *      - LE_UNAVAILABLE if observe is not enabled on this field
 *      - LE_FAULT if any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t assetData_client_PushTimeSeries
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to be Flushed
    bool isRestartTimeSeries                    ///< [IN] Restart time series after push?
);



//--------------------------------------------------------------------------------------------------
/**
 * Is time series enabled on this resource, if yes how many data points are recorded so far?
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 */
//--------------------------------------------------------------------------------------------------
le_result_t assetData_client_GetTimeSeriesStatus
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field id
    bool* isTimeSeriesPtr,                      ///< [OUT] Is time series enabled on this field
    int* numDataPointsPtr                       ///< [OUT] Number of data points recorded so far
);


//--------------------------------------------------------------------------------------------------
/**
 * Is this resource enabled for observe notifications?
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 */
//--------------------------------------------------------------------------------------------------
le_result_t assetData_client_IsObserve
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to write
    bool *isObserve                             ///< [IN] Is observe enabled on this field?
);


//--------------------------------------------------------------------------------------------------
/**
 * Sends a registration update to the server.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void assetData_RegistrationUpdate
(
    assetData_SessionStatusCheck_t status       ///< [IN] Session status check
);


//--------------------------------------------------------------------------------------------------
/**
 * Sends a registration update if observe is not enabled. A registration update would also be sent
 * if the instanceRef is not valid.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void assetData_RegUpdateIfNotObserved
(
    assetData_InstanceDataRef_t instanceRef,    ///< The instance of object 9.
    assetData_SessionStatusCheck_t status       ///< [IN] Session status check
);



//--------------------------------------------------------------------------------------------------
/**
 * Record the value of an integer variable field in time series.
 *
 * @note The client will be terminated if the instRef is not valid, or the field doesn't exist
 *
 * @note This function is the same as the SetInt() except that it provides an option to pass the
 *       timestamp. SetInt() can be used to record time series with system time as the timestamp.
 *       Timestamp should be in milli seconds elapsed since epoch.
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_FAULT on any other error
 *      - LE_OVERFLOW if the current entry was NOT added as the time series buffer is full.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_NO_MEMORY if the current entry was added but there is no space for next one.
 *                    (This error is applicable only if time series is enabled on this field)
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_client_RecordInt
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to write
    int value,                                  ///< [IN] The value to write
    uint64_t timeStamp                          ///< [IN] timestamp in msec
);


//--------------------------------------------------------------------------------------------------
/**
 * Record the value of a float variable field in time series.
 *
 * @note The client will be terminated if the instRef is not valid, or the field doesn't exist
 *
 * @note This function is the same as the SetFloat() except that it provides an option to pass the
 *       timestamp. SetFloat() can be used to record time series with system time as the timestamp.
 *       Timestamp should be in milli seconds elapsed since epoch.
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_FAULT on any other error
 *      - LE_OVERFLOW if the current entry was NOT added as the time series buffer is full.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_NO_MEMORY if the current entry was added but there is no space for next one.
 *                    (This error is applicable only if time series is enabled on this field)
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_client_RecordFloat
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to write
    double value,                               ///< [IN] The value to write
    uint64_t timeStamp                          ///< [IN] timestamp in msec
);


//--------------------------------------------------------------------------------------------------
/**
 * Record the value of a boolean variable field in time series.
 *
 * @note The client will be terminated if the instRef is not valid, or the field doesn't exist
 *
 * @note This function is the same as the SetBool() except that it provides an option to pass the
 *       timestamp. SetBool() can be used to record time series with system time as the timestamp.
 *       Timestamp should be in milli seconds elapsed since epoch.
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_FAULT on any other error
 *      - LE_OVERFLOW if the current entry was NOT added as the time series buffer is full.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_NO_MEMORY if the current entry was added but there is no space for next one.
 *                    (This error is applicable only if time series is enabled on this field)
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_client_RecordBool
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to write
    bool value,                                 ///< [IN] The value to write
    uint64_t timeStamp                          ///< [IN] timestamp in msec
);


//--------------------------------------------------------------------------------------------------
/**
 * Record the value of a string variable field in time series
 *
 * @note The client will be terminated if the instRef is not valid, or the field doesn't exist
 *
 * @note This function is the same as the SetBool() except that it provides an option to pass the
 *       timestamp. SetBool() can be used to record time series with system time as the timestamp.
 *       Timestamp should be in milli seconds elapsed since epoch.
 *
 * @return:
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *      - LE_OVERFLOW if the stored string was truncated or
 *                    if the current entry was NOT added as the time series buffer is full.
 *      - LE_NO_MEMORY if the current entry was added but there is no space for next one.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t assetData_client_RecordString
(
    assetData_InstanceDataRef_t instanceRef,    ///< [IN] Asset instance to use
    int fieldId,                                ///< [IN] Field to write
    const char* strPtr,                         ///< [IN] The value to write
    uint64_t timeStamp                          ///< [IN] timestamp in msec
);


//--------------------------------------------------------------------------------------------------
/**
 * Update current status and send pending registration updates.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void assetData_SessionStatus
(
    assetData_SessionTypes_t
);

#endif // LEGATO_ASSET_DATA_INCLUDE_GUARD

