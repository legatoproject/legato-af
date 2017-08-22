/**
 * @page c_pa_avc AirVantage Controller PA Interface
 *
 * todo: Update the remaining comments in this section ...
 *
 * This file contains the prototypes definitions of the wake up message from the Modem Platform.
 *
 *
 * @section pa_remoteMgmt_toc Table of Contents
 *
 *  - @ref pa_remoteMgmt_intro
 *  - @ref pa_remoteMgmt_rational
 *
 *
 * @section pa_remoteMgmt_intro Introduction
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developped upon these APIs.
 *
 *
 * @section pa_remoteMgmt_rational Rational
 * These functions are all blocking functions, so that they return when the modem has answered or
 * when a timeout has occured due to an interrupted communication with the modem.
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */


/**
 * @file pa_avc.h
 *
 * Legato @ref c_pa_avc include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PA_AVC_INCLUDE_GUARD
#define LEGATO_PA_AVC_INCLUDE_GUARD

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
// Definitions.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Content type: Proprietary sierra encoding format (delta + cbor + zlib)
 * This content type is used only for notify messages.
 */
//--------------------------------------------------------------------------------------------------
#define SIERRA_CBOR_ENCODING   12118


//--------------------------------------------------------------------------------------------------
/**
 *  Content type: TLV encoding used for notify and read responses.
 */
//--------------------------------------------------------------------------------------------------
#define TLV_ENCODING    1542


//--------------------------------------------------------------------------------------------------
/**
 * The possible actions to take after receiving a pending download or install notification.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_AVC_ACCEPT,          ///< Accept the requested action, i.e. download or install
    PA_AVC_DEFER            ///< Defer the requested action, i.e. download or install
}
pa_avc_Selection_t;


//--------------------------------------------------------------------------------------------------
/**
 * The possible LWM2M operation types.
 *
 * To make translation easier, the enumerated values are the same as those defined in the QMI Spec.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_AVC_OPTYPE_READ = 0x01,
    PA_AVC_OPTYPE_DISCOVER = 0x02,
    PA_AVC_OPTYPE_WRITE = 0x04,
    PA_AVC_OPTYPE_WRITE_ATTR = 0x08,
    PA_AVC_OPTYPE_EXECUTE = 0x10,
    PA_AVC_OPTYPE_CREATE = 0x20,
    PA_AVC_OPTYPE_DELETE = 0x40,
    PA_AVC_OPTYPE_OBSERVE = 0x80,
    PA_AVC_OPTYPE_NOTIFY = 0x81,
    PA_AVC_OPTYPE_OBSERVE_CANCEL = 0x82,
    PA_AVC_OPTYPE_OBSERVE_RESET = 0x83
}
pa_avc_OpType_t;


//--------------------------------------------------------------------------------------------------
/**
 * The possible LWM2M operation errors.
 *
 * To make translation easier, the enumerated values are the same as those defined in the QMI Spec.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_AVC_OPERR_NO_ERROR = 0x00,
    PA_AVC_OPERR_OP_UNSUPPORTED = 0x01,
    PA_AVC_OPERR_OBJ_UNSUPPORTED = 0x02,
    PA_AVC_OPERR_OBJ_INST_UNAVAIL = 0x03,
    PA_AVC_OPERR_RESOURCE_UNSUPPORTED = 0x04,
    PA_AVC_OPERR_INTERNAL = 0x06,
    PA_AVC_OPERR_OVERFLOW = 0x7
}
pa_avc_OpErr_t;


//--------------------------------------------------------------------------------------------------
/**
 * Session status check flag.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    ASSET_DATA_SESSION_STATUS_IGNORE,
    ASSET_DATA_SESSION_STATUS_CHECK
}
assetData_SessionStatusCheck_t;

//--------------------------------------------------------------------------------------------------
/**
 * User agreement configuration stored in modem.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    bool isAutoDownload;        ///< User agreement configuration for download
    bool isAutoConnect;         ///< User agreement configuration for connection
    bool isAutoUpdate;          ///< User agreement configuration for update
}
pa_avc_UserAgreement_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference to LWM2M Operation Request
 */
//--------------------------------------------------------------------------------------------------
typedef struct pa_avc_LWM2MOperationData* pa_avc_LWM2MOperationDataRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler used with pa_avc_StartURIDownload() to return download status.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_avc_URIDownloadHandlerFunc_t)
(
    le_avc_Status_t updateStatus
);


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report AVMS update status and type
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_avc_AVMSMessageHandlerFunc_t)
(
    le_avc_Status_t updateStatus,
    le_avc_UpdateType_t updateType,
    int32_t totalNumBytes,
    int32_t dloadProgress,
    le_avc_ErrorCode_t errorCode
);


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report LWM2M Operation
 *
 * @param opRef     ///< [IN] Reference to LWM2M operation; released after handler returns.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_avc_LWM2MOperationHandlerFunc_t)
(
    pa_avc_LWM2MOperationDataRef_t opRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report LWM2M Update Required
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_avc_LWM2MUpdateRequiredHandlerFunc_t)
(
    assetData_SessionStatusCheck_t status
);


//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Start a session with the AirVantage server
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_avc_StartSession
(
    void
);



//--------------------------------------------------------------------------------------------------
/**
 * Start a timer to watch the activity from the modem.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void  pa_avc_StartModemActivityTimer
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Enable user agreement for download and install
 *
 * @return
 *     void
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_avc_EnableUserAgreement
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Fill in the data structure required for lwm2m notify operation.
 *
 * @return
 *      - Reference to lwm2m operation
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED pa_avc_LWM2MOperationDataRef_t pa_avc_CreateOpData
(
    char* prefixPtr,
    int objId,
    int objInstId,
    int resourceId,
    pa_avc_OpType_t opType,
    uint16_t contentType,
    uint8_t* tokenPtr,
    uint8_t tokenLength
);


//--------------------------------------------------------------------------------------------------
/**
 * Notify the server when the asset value changes.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_avc_NotifyChange
(
    pa_avc_LWM2MOperationDataRef_t notifyOpRef, ///< [IN] Reference to LWM2M operation
    uint8_t* respPayloadPtr,                    ///< [IN] Payload, or NULL if no payload
    size_t respPayloadNumBytes                  ///< [IN] Payload size in bytes, or 0 if no payload.
                                                ///       If payload is a string, this is strlen()
);


//--------------------------------------------------------------------------------------------------
/**
 * Respond to the read call back operation.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_avc_ReadCallBackReport
(
    pa_avc_LWM2MOperationDataRef_t opRef,       ///< [IN] Reference to LWM2M operation
    uint8_t* respPayloadPtr,                    ///< [IN] Payload, or NULL if no payload
    size_t respPayloadNumBytes                  ///< [IN] Payload size in bytes, or 0 if no payload.
                                                ///       If payload is a string, this is strlen()
);


//--------------------------------------------------------------------------------------------------
/**
 * Stop a session with the AirVantage server
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_avc_StopSession
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Disable the AirVantage agent.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BUSY if the agent cannot be interrupted at the moment
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_avc_Disable
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Send the selection for the current pending update
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on error
 *
 * TODO: Should this take an additional parameter to specify who is doing the download/install
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_avc_SendSelection
(
    pa_avc_Selection_t selection,   ///< [IN] Action to take for pending download or install
    uint32_t deferTime              ///< [IN] If action is DEFER, then defer time in minutes
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the operation type for the give LWM2M Operation
 *
 * @return opType
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED pa_avc_OpType_t pa_avc_GetOpType
(
    pa_avc_LWM2MOperationDataRef_t opRef    ///< [IN] Reference to LWM2M operation
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the operation address for the give LWM2M Operation
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_avc_GetOpAddress
(
    pa_avc_LWM2MOperationDataRef_t opRef,   ///< [IN] Reference to LWM2M operation
    const char** objPrefixPtrPtr,           ///< [OUT] Pointer to object prefix string
    int* objIdPtr,                          ///< [OUT] Object id
    int* objInstIdPtr,                      ///< [OUT] Object instance id, or -1 if not available
    int* resourceIdPtr                      ///< [OUT] Resource id, or -1 if not available
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the operation payload for the give LWM2M Operation
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_avc_GetOpPayload
(
    pa_avc_LWM2MOperationDataRef_t opRef,   ///< [IN] Reference to LWM2M operation
    const uint8_t** payloadPtrPtr,          ///< [OUT] Pointer to payload, or NULL if no payload
    size_t* payloadNumBytesPtr              ///< [OUT] Payload size in bytes, or 0 if no payload.
                                            ///        If payload is a string, this is strlen()
);



//--------------------------------------------------------------------------------------------------
/**
 * Get the token for the given LWM2M Operation
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_avc_GetOpToken
(
    pa_avc_LWM2MOperationDataRef_t opRef,   ///< [IN] Reference to LWM2M operation
    const uint8_t** tokenPtrPtr,            ///< [OUT] Pointer to token, or NULL if no token
    uint8_t* tokenLengthPtr                 ///< [OUT] Token Length bytes, or 0 if no token
);



//--------------------------------------------------------------------------------------------------
/**
 * Is this a request for reading the first block?
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED bool pa_avc_IsFirstBlock
(
    pa_avc_LWM2MOperationDataRef_t opRef   ///< [IN] Reference to LWM2M operation
);


//--------------------------------------------------------------------------------------------------
/**
 * Respond to the previous LWM2M Operation indication with success
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_avc_OperationReportSuccess
(
    pa_avc_LWM2MOperationDataRef_t opRef,   ///< [IN] Reference to LWM2M operation
    const uint8_t* respPayloadPtr,          ///< [IN] Payload, or NULL if no payload
    size_t respPayloadNumBytes              ///< [IN] Payload size in bytes, or 0 if no payload.
                                            ///       If payload is a string, this is strlen()
);


//--------------------------------------------------------------------------------------------------
/**
 * Respond to the previous LWM2M Operation indication with error
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_avc_OperationReportError
(
    pa_avc_LWM2MOperationDataRef_t opRef,   ///< [IN] Reference to LWM2M operation
    pa_avc_OpErr_t opError                  ///< [IN] Operation error
);


//--------------------------------------------------------------------------------------------------
/**
 * Send updated list of assets and asset instances
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_avc_RegistrationUpdate
(
    const char* updatePtr,          ///< [IN] List formatted for QMI_LWM2M_REG_UPDATE_REQ
    size_t updateNumBytes,          ///< [IN] Size of the update list
    size_t updateCount              ///< [IN] Count of assets + asset instances
);


//--------------------------------------------------------------------------------------------------
/**
 * Start a download from the specified URI
 *
 * The status of the download will passed to handlerRef:
 *  - LE_AVC_DOWNLOAD_IN_PROGRESS, if the download is in progress
 *  - LE_AVC_DOWNLOAD_COMPLETE, if the download completed successfully
 *  - LE_AVC_DOWNLOAD_FAILED, if there was an error, and the download was stopped
 * Note that handlerRef will be cleared after download complete or failed.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_avc_StartURIDownload
(
    const char* uriPtr,                         ///< [IN] URI giving location of file to download
    pa_avc_URIDownloadHandlerFunc_t handlerRef  ///< [IN] Handler to receive download status,
);



//--------------------------------------------------------------------------------------------------
/**
 * Add the URIDownload Handler Ref
 *
 * The status of the download will passed to handlerRef:
 *  - LE_AVC_DOWNLOAD_IN_PROGRESS, if the download is in progress
 *  - LE_AVC_DOWNLOAD_COMPLETE, if the download completed successfully
 *  - LE_AVC_DOWNLOAD_FAILED, if there was an error, and the download was stopped
 * Note that handlerRef will be cleared after download complete or failed.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_avc_AddURIDownloadStatusHandler
(
    pa_avc_URIDownloadHandlerFunc_t handlerRef  ///< [IN] Handler to receive download status,
);


//--------------------------------------------------------------------------------------------------
/**
 * Read the image file from the modem.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_avc_ReadImage
(
    int* fdPtr        ///< [OUT] File descriptor for the image, ready for reading.
);


//--------------------------------------------------------------------------------------------------
/**
 * This function registers a handler for AVMS update status
 *
 * If the handler is NULL, then the previous handler will be removed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_avc_SetAVMSMessageHandler
(
    pa_avc_AVMSMessageHandlerFunc_t handlerRef          ///< [IN] Handler for AVMS update status
);


//--------------------------------------------------------------------------------------------------
/**
 * This function registers a handler for LWM2M Operation
 *
 * If the handler is NULL, then the previous handler will be removed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_avc_SetLWM2MOperationHandler
(
    pa_avc_LWM2MOperationHandlerFunc_t handlerRef       ///< [IN] Handler for LWM2M Operation
);


//--------------------------------------------------------------------------------------------------
/**
 * This function registers a handler for LWM2M Update Required
 *
 * If the handler is NULL, then the previous handler will be removed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_avc_SetLWM2MUpdateRequiredHandler
(
    pa_avc_LWM2MUpdateRequiredHandlerFunc_t handlerRef  ///< [IN] Handler for LWM2M Update Required
);


//--------------------------------------------------------------------------------------------------
/**
 * This function sets up the modem activity timer. The timeout will default to 20 seconds if
 * user defined value doesn't exist or if the defined value is less than 0.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_avc_SetModemActivityTimeout
(
    int timeout     ///< [IN] Timeout
);


//--------------------------------------------------------------------------------------------------
/**
 * Function to read the last http status.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED uint16_t pa_avc_GetHttpStatus
(
    void
);



//--------------------------------------------------------------------------------------------------
/**
 * Function to read the session type.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_avc_SessionType_t pa_avc_GetSessionType
(
    void
);



//--------------------------------------------------------------------------------------------------
/**
 * Function to read APN configuration.
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT if there is any error while reading.
 *      - LE_OVERFLOW if the buffer provided is too small.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_avc_GetApnConfig
(
    char* apnName,                      ///< [OUT] APN name string
    size_t apnNameNumElements,          ///< [IN]  Buffer size
    char* userName,                     ///< [OUT] User name string
    size_t uNameNumElements,            ///< [IN]  Buffer size
    char* userPwd,                      ///< [OUT] Password string
    size_t userPwdNumElements           ///< [IN]  Buffer size
);



//--------------------------------------------------------------------------------------------------
/**
 * Function to write APN configuration.
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT if not able to write the APN configuration.
 *      - LE_OVERFLOW if one of the input strings is too long.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_avc_SetApnConfig
(
    const char* apnName,                ///< [IN] APN name string
    const char* userName,               ///< [IN] User name string
    const char* userPwd                 ///< [IN] Password string
);



//--------------------------------------------------------------------------------------------------
/**
 * Function to read the retry timers.
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT if not able to read the timers.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_avc_GetRetryTimers
(
    uint16_t* timerValuePtr,                ///< [OUT] Array of 8 retry timers in minutes
    size_t* numTimers                       ///< [OUT] Number of retry timers
);

//--------------------------------------------------------------------------------------------------
/**
 * Function to set the retry timers.
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT if not able to write the timers.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_avc_SetRetryTimers
(
    const uint16_t* timerValuePtr,                  ///< [IN] Array of 8 retry timers in minutes
    size_t numTimers                                ///< [IN] Number of retry timers
);

//--------------------------------------------------------------------------------------------------
/**
 * Function to read the polling timer.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT if not available
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_avc_GetPollingTimer
(
    uint32_t* pollingTimerPtr                       ///< [OUT] polling timer value in minutes
);

//--------------------------------------------------------------------------------------------------
/**
 * Function to set the polling timer.
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT if not able to read the timers.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_avc_SetPollingTimer
(
    uint32_t pollingTimer                            ///< [IN] polling timer value in minutes
);

//--------------------------------------------------------------------------------------------------
/**
 * Function to read the user agreement status
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT if not available
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_avc_GetUserAgreement
(
    pa_avc_UserAgreement_t* configPtr               ///< [OUT] user agreement configuration
);

#endif // LEGATO_PA_AVC_INCLUDE_GUARD
