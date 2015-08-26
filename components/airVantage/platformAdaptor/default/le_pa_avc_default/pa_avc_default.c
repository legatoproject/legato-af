//--------------------------------------------------------------------------------------------------
/**
 * @file pa_avc_default.c
 *
 * Default implementation of @ref pa_avc interface
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "pa_avc.h"


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
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}


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
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}


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
)
{
    LE_ERROR("Unsupported function called with selection=%i, deferTime=%i", selection, deferTime);
    return LE_FAULT;
}


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
)
{
    LE_ERROR("Unsupported function called");
    return PA_AVC_OPTYPE_READ;
}


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
)
{
    LE_ERROR("Unsupported function called");
}


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
)
{
    LE_ERROR("Unsupported function called");
}


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
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}


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
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}


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
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}


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
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}


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
    int* fdPtr         ///< [OUT] File descriptor for the image, ready for reading.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}


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
)
{
    LE_ERROR("Unsupported function called with %p", handlerRef);
}


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
)
{
    LE_ERROR("Unsupported function called with %p", handlerRef);
}


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
)
{
    LE_ERROR("Unsupported function called with %p", handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Init this component
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}

