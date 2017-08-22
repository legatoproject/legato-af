//--------------------------------------------------------------------------------------------------
/**
 * @file pa_avc_default.c
 *
 * Default implementation of @ref pa_avc interface
 *
 * Copyright (C) Sierra Wireless Inc.
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
 * Start a timer to watch the activity from the modem.
 */
//--------------------------------------------------------------------------------------------------
void  pa_avc_StartModemActivityTimer
(
    void
)
{
    LE_ERROR("Unsupported function called");
}


//--------------------------------------------------------------------------------------------------
/**
 * Enable user agreement for download and install
 *
 * @return
 *     void
 */
//--------------------------------------------------------------------------------------------------
void pa_avc_EnableUserAgreement
(
    void
)
{
    LE_ERROR("Unsupported function called");
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
 * Is this a request for reading the first block?
 */
//--------------------------------------------------------------------------------------------------
bool pa_avc_IsFirstBlock
(
    pa_avc_LWM2MOperationDataRef_t opRef   ///< [IN] Reference to LWM2M operation
)
{
    LE_ERROR("Unsupported function called");
    return false;
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
 * Get the token for the given LWM2M Operation
 */
//--------------------------------------------------------------------------------------------------
void pa_avc_GetOpToken
(
    pa_avc_LWM2MOperationDataRef_t opRef,   ///< [IN] Reference to LWM2M operation
    const uint8_t** tokenPtrPtr,            ///< [OUT] Pointer to token, or NULL if no token
    uint8_t* tokenLengthPtr                 ///< [OUT] Token Length bytes, or 0 if no token
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
le_result_t pa_avc_AddURIDownloadStatusHandler
(
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
 * This function sets up the modem activity timer. The timeout will default to 20 seconds if
 * user defined value doesn't exist or if the defined value is less than 0.
 */
//--------------------------------------------------------------------------------------------------
void pa_avc_SetModemActivityTimeout
(
    int timeout     ///< [IN] Timeout
)
{
    LE_ERROR("Unsupported function called.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to read the last http status.
 *
 * @return
 *      - HttpStatus as defined in RFC 7231, Section 6.
 */
//--------------------------------------------------------------------------------------------------
uint16_t pa_avc_GetHttpStatus
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to read the session type.
 *
 * @return
 *      - Session Type.
 */
//--------------------------------------------------------------------------------------------------
le_avc_SessionType_t pa_avc_GetSessionType
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to read the retry timers.
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT if not able to read the timers.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_avc_GetRetryTimers
(
    uint16_t* timerValuePtr,                ///< [OUT] Array of 8 retry timers in minutes
    size_t* numTimers                       ///< [OUT] Number of retry timers
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to set the retry timers.
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT if not able to write the timers.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_avc_SetRetryTimers
(
    const uint16_t* timerValuePtr,                  ///< [IN] Array of 8 retry timers in minutes
    size_t numTimers                                ///< [IN] Number of retry timers
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to read the polling timer.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT if not available
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_avc_GetPollingTimer
(
    uint32_t* pollingTimerPtr                       ///< [OUT] polling timer value in minutes
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to set the polling timer.
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT if not able to read the timers.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_avc_SetPollingTimer
(
    uint32_t pollingTimer                            ///< [IN] polling timer value in minutes
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to read the user agreement status
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT if not available
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_avc_GetUserAgreement
(
    pa_avc_UserAgreement_t* configPtr           ///< [OUT] user agreement configuration from modem
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

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
le_result_t pa_avc_GetApnConfig
(
    char* apnName,                      ///< [OUT] APN name string
    size_t apnNameNumElements,          ///< [IN]  Buffer size
    char* userName,                     ///< [OUT] User name string
    size_t uNameNumElements,            ///< [IN]  Buffer size
    char* userPwd,                      ///< [OUT] Password string
    size_t userPwdNumElements           ///< [IN]  Buffer size
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}



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
le_result_t pa_avc_SetApnConfig
(
    const char* apnName,                ///< [IN] APN name string
    const char* userName,               ///< [IN] User name string
    const char* userPwd                 ///< [IN] Password string
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Notify the server when the asset value changes.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_avc_NotifyChange
(
    pa_avc_LWM2MOperationDataRef_t notifyOpRef, ///< [IN] Reference to LWM2M operation
    uint8_t* respPayloadPtr,                    ///< [IN] Payload, or NULL if no payload
    size_t respPayloadNumBytes                  ///< [IN] Payload size in bytes, or 0 if no payload.
                                                ///       If payload is a string, this is strlen()
)
{
    LE_ERROR("Unsupported function called.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Respond to the read call back operation.
 */
//--------------------------------------------------------------------------------------------------
void pa_avc_ReadCallBackReport
(
    pa_avc_LWM2MOperationDataRef_t opRef,       ///< [IN] Reference to LWM2M operation
    uint8_t* respPayloadPtr,                    ///< [IN] Payload, or NULL if no payload
    size_t respPayloadNumBytes                  ///< [IN] Payload size in bytes, or 0 if no payload.
                                                ///       If payload is a string, this is strlen()
)
{
    LE_ERROR("Unsupported function called.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Fill in the data structure required for lwm2m operation.
 *
 * @return
 *      - Reference to lwm2m operation
 */
//--------------------------------------------------------------------------------------------------
pa_avc_LWM2MOperationDataRef_t pa_avc_CreateOpData
(
    char* prefixPtr,
    int objId,
    int objInstId,
    int resourceId,
    pa_avc_OpType_t opType,
    uint16_t contentType,
    uint8_t* tokenPtr,
    uint8_t tokenLength
)
{
    LE_ERROR("Unsupported function called.");
    return NULL;
}



//--------------------------------------------------------------------------------------------------
/**
 * Init this component
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}

