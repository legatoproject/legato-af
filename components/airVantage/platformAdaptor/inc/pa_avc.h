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
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


/**
 * @file pa_avc.h
 *
 * Legato @ref c_pa_avc include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
 * The possible actions to take after receiving a pending download or install notification.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_AVC_ACCEPT,          ///< Accept the requested action, i.e. download or install
    PA_AVC_REJECT,          ///< Reject the requested action, i.e. download or install
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
    PA_AVC_OPTYPE_EXECUTE = 0x10,
    PA_AVC_OPTYPE_CREATE = 0x20,
    PA_AVC_OPTYPE_DELETE = 0x40
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
    PA_AVC_OPERR_INTERNAL = 0x06
}
pa_avc_OpErr_t;


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
    le_avc_Status_t updateStatus,
    size_t bytesReceived
);


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report AVMS update status and type
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_avc_AVMSMessageHandlerFunc_t)
(
    le_avc_Status_t updateStatus,
    le_avc_UpdateType_t updateType
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
    void
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
le_result_t pa_avc_StartSession
(
    void
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
le_result_t pa_avc_StopSession
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
le_result_t pa_avc_SendSelection
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
pa_avc_OpType_t pa_avc_GetOpType
(
    pa_avc_LWM2MOperationDataRef_t opRef    ///< [IN] Reference to LWM2M operation
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the operation address for the give LWM2M Operation
 */
//--------------------------------------------------------------------------------------------------
void pa_avc_GetOpAddress
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
void pa_avc_GetOpPayload
(
    pa_avc_LWM2MOperationDataRef_t opRef,   ///< [IN] Reference to LWM2M operation
    const uint8_t** payloadPtrPtr,          ///< [OUT] Pointer to payload, or NULL if no payload
    size_t* payloadNumBytesPtr              ///< [OUT] Payload size in bytes, or 0 if no payload.
                                            ///        If payload is a string, this is strlen()
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
le_result_t pa_avc_OperationReportSuccess
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
le_result_t pa_avc_OperationReportError
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
le_result_t pa_avc_RegistrationUpdate
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
 *  - LE_AVC_DOWNLOAD_COMPLETE, if the download completed successfully
 *  - LE_AVC_DOWNLOAD_FAILED, if there was an error, and the download was stopped
 * Note that handlerRef will only be called once.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_avc_StartURIDownload
(
    const char* uriPtr,                         ///< [IN] URI giving location of file to download
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
le_result_t pa_avc_ReadImage
(
    int* fdPtr,         ///< [OUT] File descriptor for the image, ready for reading.
    size_t numBytes     ///< [IN] Size of image in bytes
);


//--------------------------------------------------------------------------------------------------
/**
 * This function registers a handler for AVMS update status
 *
 * If the handler is NULL, then the previous handler will be removed.
 */
//--------------------------------------------------------------------------------------------------
void pa_avc_SetAVMSMessageHandler
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
void pa_avc_SetLWM2MOperationHandler
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
void pa_avc_SetLWM2MUpdateRequiredHandler
(
    pa_avc_LWM2MUpdateRequiredHandlerFunc_t handlerRef  ///< [IN] Handler for LWM2M Update Required
);


#endif // LEGATO_PA_AVC_INCLUDE_GUARD
