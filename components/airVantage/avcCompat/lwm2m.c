/**
 * @file lwm2m.c
 *
 * Implementation of LWM2M handler sub-component
 *
 * This provides glue logic between QMI PA and assetData
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "assetData.h"
#include "avcShared.h"
#include "le_print.h"



//--------------------------------------------------------------------------------------------------
// Macros
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * An invalid resource id. Any value >= -1 is valid.
 */
//--------------------------------------------------------------------------------------------------
#define INVALID_RESOURCE_ID -2


//--------------------------------------------------------------------------------------------------
// Local Data
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Buffer for the TLV encoded asset data to be sent to Airvantage server. The data sent from Legato
 * to Airvantage server needs to be TLV encoded. The complete TLV response must always be
 * constructed.
 *
 * If we build part of the TLV, and then variable length values have changed, then the TLV
 * will be corrupted. This buffer is filled only when the request is for a block offset of 0.
 * For subsequent block reads we can just return data from this buffer without retrieving asset
 * data.
 *
 * The buffer size is chosen to support reading object 9 instances of at least 64 apps.
 * The following fields are read for lwm2m/9/appName, i.e. a single instance of object 9.
 *
 * App Name                :  48 bytes
 * App version             : 256 bytes
 * Update state            :   4 byte
 * Update Supported object :   4 byte
 * Update Result           :   4 byte
 * Activation State        :   4 byte
 *
 * The buffer size required to store object 9 for 64 APPS is 64*320 bytes = ~20K
 * Though we need only ~20K bytes, we have allocated 32K bytes for margin of safety.
 */
//--------------------------------------------------------------------------------------------------
static uint8_t ValueData[32*1024];

//--------------------------------------------------------------------------------------------------
/**
 * Size of the asset data that will be sent to the Airvantage server.
 */
//--------------------------------------------------------------------------------------------------
static size_t BytesWritten;

//--------------------------------------------------------------------------------------------------
/**
 * Resource Id of the asset that is being read
 */
//--------------------------------------------------------------------------------------------------
static int CurrentReadResId = INVALID_RESOURCE_ID;

//--------------------------------------------------------------------------------------------------
/**
 * AssetRef of the current read operation.
 */
//--------------------------------------------------------------------------------------------------
static assetData_AssetDataRef_t CurrentReadAssetRef;

// -------------------------------------------------------------------------------------------------
/**
 *  Length of legato application prefix (le_)
 */
// ------------------------------------------------------------------------------------------------
#define APP_PREFIX_LENGTH 3

//--------------------------------------------------------------------------------------------------
/**
 * Flag to indicate whether read operation with unspecified object is received.
 */
//--------------------------------------------------------------------------------------------------
static bool IsReadEventReceived = false;

//--------------------------------------------------------------------------------------------------
// Local functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Checks if read operation notifications is received
 */
//--------------------------------------------------------------------------------------------------
bool lwm2m_IsReadEventReceived
(
    void
)
{
    return IsReadEventReceived;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for receiving Operation indication
 *
 * TODO: This function needs to be refactored.
 */
//--------------------------------------------------------------------------------------------------
static void OperationHandler
(
    pa_avc_LWM2MOperationDataRef_t opRef

)
{
    pa_avc_OpType_t opType;
    const char* objPrefixPtr;
    int objId;
    int objInstId;
    int resourceId;
    const uint8_t* payloadPtr;
    const uint8_t* tokenPtr;
    uint8_t tokenLength;
    size_t payloadLength;

    // Get the operation details from the opRef
    opType = pa_avc_GetOpType(opRef);
    pa_avc_GetOpAddress(opRef, &objPrefixPtr, &objId, &objInstId, &resourceId);
    pa_avc_GetOpPayload(opRef, &payloadPtr, &payloadLength);

    pa_avc_GetOpToken(opRef, &tokenPtr, &tokenLength);

    le_result_t result;
    pa_avc_OpErr_t opErr = PA_AVC_OPERR_NO_ERROR;

    // In some cases, we need to adjust the prefix string.
    const char* newPrefixPtr = objPrefixPtr;

    // Empty object prefix string should use 'lwm2m' when accessing assetData.
    // TODO: This is a work-around because the modem currently does not handle the 'lwm2m' prefix.
    if ( 0 == strlen(objPrefixPtr) )
    {
        newPrefixPtr = "lwm2m";
        LE_INFO("Defaulting to lwm2m namespace for assetData");
    }
    // Apps will have an "le_" prefix, which needs to be stripped, because the "le_" is not part
    // of the app name that is stored in assetData.
    // TODO: Should the "le_" prefix instead be added to the app name in assetData?
    else
    {
        if ( 0 == strncmp(objPrefixPtr, "le_", APP_PREFIX_LENGTH) )
        {
            newPrefixPtr = objPrefixPtr+3;
            LE_DEBUG("Adjusting %s to %s", objPrefixPtr, newPrefixPtr);
        }
        else
        {
            opErr = PA_AVC_OPERR_OBJ_UNSUPPORTED;
            pa_avc_OperationReportError(opRef, opErr);
            return;
        }
    }

    LE_DEBUG("Operation: %s/%d/%d/%d <%d>", newPrefixPtr, objId, objInstId, resourceId, opType);

    // Reinitailize CurrentReadResId to an invalid value.
    if ( opType != PA_AVC_OPTYPE_READ )
    {
        CurrentReadResId = INVALID_RESOURCE_ID;
    }

    // Special handling for READ if object instance is not specified (-1)
    // TODO: restructure
    if ( (opType == PA_AVC_OPTYPE_READ) && (objInstId == -1) )
    {
        LE_DEBUG("PA_AVC_OPTYPE_READ %s/%d", newPrefixPtr, objId);

        assetData_AssetDataRef_t assetRef;

        result = assetData_GetAssetRefById(newPrefixPtr, objId, &assetRef);

        if ( result != LE_OK )
        {
            if ( result == LE_NOT_FOUND )
            {
                opErr = PA_AVC_OPERR_OBJ_UNSUPPORTED;
            }
            else
            {
                opErr = PA_AVC_OPERR_INTERNAL;
            }
        }

        if ( opErr != PA_AVC_OPERR_NO_ERROR )
        {
            pa_avc_OperationReportError(opRef, opErr);
            return;
        }

        // Read the asset data only when the request is for the first block. For subsequent block
        // reads, return asset data stored in the buffer. It is assumed that unless the read
        // of the first block is successful, no subsequent requests will be made by the server.
        if ( pa_avc_IsFirstBlock(opRef) )
        {
            CurrentReadResId = resourceId;
            CurrentReadAssetRef = assetRef;
            result = assetData_WriteObjectToTLV(assetRef,
                                                resourceId,
                                                ValueData,
                                                sizeof(ValueData),
                                                &BytesWritten);
        }
        else
        {
            // We have received a request for a non-zero block offset before a request for a
            // block offset of 0; this is an error
            if ( (resourceId != CurrentReadResId) ||
                 (assetRef != CurrentReadAssetRef) )
            {
                opErr = PA_AVC_OPERR_INTERNAL;
                LE_ERROR("Error reading asset data.");
            }
            else
            {
                result = LE_OK;
            }
        }

        if ( result != LE_OK )
        {
            if ( result == LE_OVERFLOW )
            {
                opErr = PA_AVC_OPERR_OVERFLOW;
            }
            else
            {
                opErr = PA_AVC_OPERR_INTERNAL;
            }
        }

        if ( opErr != PA_AVC_OPERR_NO_ERROR )
        {
            pa_avc_OperationReportError(opRef, opErr);
        }
        else
        {
            // Send the valid response
            pa_avc_OperationReportSuccess(opRef, ValueData, BytesWritten);
        }

        IsReadEventReceived = true;

        // TODO: Refactor so I don't need a return here.
        return;
    }

    // Observe with an objId of -1 means, observe all instances.
    if ( (opType == PA_AVC_OPTYPE_OBSERVE) && (objInstId == -1) )
    {
        LE_DEBUG("PA_AVC_OPTYPE_OBSERVE %s/%d", newPrefixPtr, objId);

        // Observe not supported on object "/lwm2m/9".
        if ( (0 == strcmp(newPrefixPtr, "lwm2m")) && (LWM2M_SOFTWARE_UPDATE == objId) )
        {
            LE_DEBUG("Observe not supported on %s/%d/%d", newPrefixPtr, objId, objInstId);
            pa_avc_OperationReportError(opRef, PA_AVC_OPERR_OP_UNSUPPORTED);
            return;
        }

        assetData_AssetDataRef_t assetRef;

        result = assetData_GetAssetRefById(newPrefixPtr, objId, &assetRef);

        if ( result != LE_OK )
        {
            if ( result == LE_NOT_FOUND )
            {
                opErr = PA_AVC_OPERR_OBJ_UNSUPPORTED;
            }
            else
            {
                opErr = PA_AVC_OPERR_INTERNAL;
            }
        }

        if ( opErr != PA_AVC_OPERR_NO_ERROR )
        {
            LE_ERROR("Failed to read AssetRef.");
            pa_avc_OperationReportError(opRef, opErr);
            return;
        }

        // Set observe on all instances of an object.
        result = assetData_SetObserveAllInstances(assetRef, true, (uint8_t*)tokenPtr, tokenLength);

        if ( result != LE_OK )
        {
            LE_ERROR("Failed to Set Observe.");
            pa_avc_OperationReportError(opRef, PA_AVC_OPERR_INTERNAL);
        }
        else
        {
            result = assetData_WriteObjectToTLV(assetRef,
                                                resourceId,
                                                ValueData,
                                                sizeof(ValueData),
                                                &BytesWritten);
            if ( result != LE_OK )
            {
                if ( result == LE_NOT_FOUND )
                {
                    opErr = PA_AVC_OPERR_OBJ_UNSUPPORTED;
                }
                else
                {
                    opErr = PA_AVC_OPERR_INTERNAL;
                }
            }

            if ( opErr != PA_AVC_OPERR_NO_ERROR )
            {
                LE_ERROR("Failed to write TLV of object.");
                pa_avc_OperationReportError(opRef, opErr);
                return;
            }

            LE_INFO("Observe set successfully.");

            // Send the valid response
            pa_avc_OperationReportSuccess(opRef, ValueData, BytesWritten);
        }

        return;
    }
}




//--------------------------------------------------------------------------------------------------
// Interface functions
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Init this sub-component
 */
//--------------------------------------------------------------------------------------------------
le_result_t lwm2m_Init
(
    void
)
{
    // Register handlers for Operation and UpdateRequired indications
    pa_avc_SetLWM2MOperationHandler(OperationHandler);
    pa_avc_SetLWM2MUpdateRequiredHandler(assetData_RegistrationUpdate);

    return LE_OK;
}
