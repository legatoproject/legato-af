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
#include "pa_avc.h"
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
 * to Airvantage server needs to be TLV encoded. The complete TLV response must always be constructed.
 * If we build part of the TLV, and then variable length values have changed, then the TLV
 * will be corrupted. This buffer is filled only when the request is for a block offset of 0.
 * For subsequent block reads we can just return data from this buffer without retrieving asset data.
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
 * Instance Id of the asset that is being read.
 */
//--------------------------------------------------------------------------------------------------
static assetData_InstanceDataRef_t CurrentReadInstRef;

//--------------------------------------------------------------------------------------------------
/**
 * AssetRef of the current read operation.
 */
//--------------------------------------------------------------------------------------------------
static assetData_AssetDataRef_t CurrentReadAssetRef;


//--------------------------------------------------------------------------------------------------
// Local functions
//--------------------------------------------------------------------------------------------------

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
    assetData_InstanceDataRef_t instRef;
    int instId;
    pa_avc_OpErr_t opErr = PA_AVC_OPERR_NO_ERROR;

    // In some cases, we need to adjust the prefix string.
    const char* newPrefixPtr = objPrefixPtr;

    // Empty object prefix string should use 'lwm2m' when accessing assetData.
    // TODO: This is a work-around because the modem currently does not handle the 'lwm2m' prefix.
    if ( strlen(objPrefixPtr) == 0 )
    {
        newPrefixPtr = "lwm2m";
        LE_INFO("Defaulting to lwm2m namespace for assetData");
    }
    // Apps will have an "le_" prefix, which needs to be stripped, because the "le_" is not part
    // of the app name that is stored in assetData.
    // TODO: Should the "le_" prefix instead be added to the app name in assetData?
    else if ( strncmp(objPrefixPtr, "le_", 3) == 0 )
    {
        newPrefixPtr = objPrefixPtr+3;
        LE_DEBUG("Adjusting %s to %s", objPrefixPtr, newPrefixPtr);
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

        if ( result == LE_NOT_FOUND )
            opErr = PA_AVC_OPERR_OBJ_UNSUPPORTED;
        else if ( result != LE_OK )
            opErr = PA_AVC_OPERR_INTERNAL;

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
            if ( resourceId != CurrentReadResId ||
                 assetRef != CurrentReadAssetRef )
            {
                opErr = PA_AVC_OPERR_INTERNAL;
                LE_ERROR("Error reading asset data.");
            }
            else
            {
                result = LE_OK;
            }
        }

        if ( result == LE_OVERFLOW )
            opErr = PA_AVC_OPERR_OVERFLOW;
        else if ( result != LE_OK )
            opErr = PA_AVC_OPERR_INTERNAL;

        if ( opErr != PA_AVC_OPERR_NO_ERROR )
        {
            pa_avc_OperationReportError(opRef, opErr);
        }
        else
        {
            // Send the valid response
            pa_avc_OperationReportSuccess(opRef, ValueData, BytesWritten);
        }

        // TODO: Refactor so I don't need a return here.
        return;
    }

    if ((opType == PA_AVC_OPTYPE_OBSERVE_CANCEL) || (opType == PA_AVC_OPTYPE_OBSERVE_RESET))
    {
        LE_DEBUG("PA_AVC_OPTYPE_OBSERVE_CANCEL %s/%d", newPrefixPtr, objId);

        if (objId == -1)
        {
            // Cancel with an objId of -1 means, cancel observe on all objects.
            assetData_CancelAllObserve();

            pa_avc_OperationReportSuccess(opRef, NULL, 0);
            return;
        }
        else
        {
            assetData_AssetDataRef_t assetRef;
            result = assetData_GetAssetRefById(newPrefixPtr, objId, &assetRef);

            if ( result == LE_NOT_FOUND )
                opErr = PA_AVC_OPERR_OBJ_UNSUPPORTED;
            else if ( result != LE_OK )
                opErr = PA_AVC_OPERR_INTERNAL;

            if ( opErr != PA_AVC_OPERR_NO_ERROR )
            {
                LE_ERROR("Failed to read AssetRef.");
                pa_avc_OperationReportError(opRef, opErr);
                return;
            }

            // Cancel observe on all instances of an object.
            result = assetData_SetObserveAllInstances(assetRef, false, NULL, 0);

            if ( result != LE_OK )
            {
                LE_ERROR("Failed to Cancel Observe.");
                pa_avc_OperationReportError(opRef, PA_AVC_OPERR_INTERNAL);
            }
            else
            {
                // At COAP level observe cancel is a read request from the server with observe
                // option flag set to false. So the reponse for cancel should include TLV for
                // entire object.
                result = assetData_WriteObjectToTLV(assetRef,
                                    resourceId,
                                    ValueData,
                                    sizeof(ValueData),
                                    &BytesWritten);

                if ( result == LE_NOT_FOUND )
                    opErr = PA_AVC_OPERR_OBJ_UNSUPPORTED;
                else if ( result != LE_OK )
                    opErr = PA_AVC_OPERR_INTERNAL;

                if ( opErr != PA_AVC_OPERR_NO_ERROR )
                {
                    LE_ERROR("Failed to write TLV of object.");
                    pa_avc_OperationReportError(opRef, opErr);
                    return;
                }

                LE_INFO("Observe cancelled successfully.");

                // Send the valid response
                pa_avc_OperationReportSuccess(opRef, ValueData, BytesWritten);
            }
            return;
        }
    }

    // Observe with an objId of -1 means, observe all instances.
    if ( (opType == PA_AVC_OPTYPE_OBSERVE) && (objInstId == -1) )
    {
        LE_DEBUG("PA_AVC_OPTYPE_OBSERVE %s/%d", newPrefixPtr, objId);

        // Observe not supported on object "/lwm2m/9".
        if ( (strcmp(newPrefixPtr, "lwm2m") == 0) && (objId == 9) )
        {
            LE_DEBUG("Observe not supported on %s/%d/%d", newPrefixPtr, objId, objInstId);
            pa_avc_OperationReportError(opRef, PA_AVC_OPERR_OP_UNSUPPORTED);
            return;
        }

        assetData_AssetDataRef_t assetRef;

        result = assetData_GetAssetRefById(newPrefixPtr, objId, &assetRef);

        if ( result == LE_NOT_FOUND )
            opErr = PA_AVC_OPERR_OBJ_UNSUPPORTED;
        else if ( result != LE_OK )
            opErr = PA_AVC_OPERR_INTERNAL;

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

            if ( result == LE_NOT_FOUND )
                opErr = PA_AVC_OPERR_OBJ_UNSUPPORTED;
            else if ( result != LE_OK )
                opErr = PA_AVC_OPERR_INTERNAL;

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

    // These operations all need a valid instanceRef.  Ensure that the specified instance exists,
    // and get the instanceRef; this check is common across several of the opTypes.
    if ( (opType == PA_AVC_OPTYPE_READ) ||
         (opType == PA_AVC_OPTYPE_WRITE) ||
         (opType == PA_AVC_OPTYPE_EXECUTE) ||
         (opType == PA_AVC_OPTYPE_DELETE))
    {
        result = assetData_GetInstanceRefById(newPrefixPtr, objId, objInstId, &instRef);
        if ( result == LE_NOT_FOUND )
            opErr = PA_AVC_OPERR_OBJ_INST_UNAVAIL;
        else if ( result != LE_OK )
            opErr = PA_AVC_OPERR_INTERNAL;

        if ( opErr != PA_AVC_OPERR_NO_ERROR )
        {
            LE_ERROR("Object instance required for this operation.");
            pa_avc_OperationReportError(opRef, opErr);
            return;
        }
    }

    switch ( opType )
    {
        case PA_AVC_OPTYPE_READ:
            LE_DEBUG("PA_AVC_OPTYPE_READ %s/%d/%d/%d", newPrefixPtr, objId, objInstId, resourceId);

            if ( pa_avc_IsFirstBlock(opRef) )
            {
                CurrentReadResId = resourceId;
                CurrentReadInstRef = instRef;

                if ( resourceId == -1 )
                {
                    result = assetData_WriteFieldListToTLV(instRef,
                                                           ValueData,
                                                           sizeof(ValueData),
                                                           &BytesWritten);
                }
                else
                {
                    result = assetData_server_GetValue(opRef,
                                                       instRef,
                                                       resourceId,
                                                       (char*)ValueData,
                                                       sizeof(ValueData));

                    // If there exists a registered handler on the client side,for read operation, the
                    // handler should finish its operation and send the response - in such case we can
                    // just return.
                    if ( result == LE_UNAVAILABLE )
                    {
                        return;
                    }

                    BytesWritten = strlen((char*)ValueData);
                }
            }
            else
            {
                // We have received a request for a non-zero block offset before a request for a
                // block offset of 0; this is an error
                if ( resourceId != CurrentReadResId ||
                     instRef != CurrentReadInstRef)
                {
                    opErr = PA_AVC_OPERR_INTERNAL;
                    LE_ERROR("Error reading asset data.");
                }
                else
                {
                    result = LE_OK;
                }
            }

            if ( result == LE_NOT_FOUND )
                opErr = PA_AVC_OPERR_RESOURCE_UNSUPPORTED;
            else if ( result == LE_OVERFLOW )
                opErr = PA_AVC_OPERR_OVERFLOW;
            else if ( result != LE_OK )
                opErr = PA_AVC_OPERR_INTERNAL;

            if ( opErr != PA_AVC_OPERR_NO_ERROR )
            {
                pa_avc_OperationReportError(opRef, opErr);
                return;
            }

            // Send the valid response
            pa_avc_OperationReportSuccess(opRef, ValueData, BytesWritten);
            break;


        case PA_AVC_OPTYPE_WRITE:
            LE_DEBUG("PA_AVC_OPTYPE_WRITE %s/%d/%d/%d", newPrefixPtr, objId, objInstId, resourceId);

            if ( resourceId == -1 )
            {
                // Call back asset action handlers on a write to asset instance.
                result = assetData_ReadFieldListFromTLV((uint8_t*)payloadPtr,
                                                        payloadLength,
                                                        instRef,
                                                        true);
            }
            else
            {
                // The payload is a string, but can't be guaranteed that it is null terminated,
                // so copy to local buffer and null terminate it.  The payload length has already
                // been checked, so we know it will fit in the buffer.
                memcpy(ValueData, payloadPtr, payloadLength);
                ValueData[payloadLength] = 0;

                result = assetData_server_SetValue(instRef, resourceId, (const char*)ValueData);
            }

            if ( result == LE_NOT_FOUND )
                opErr = PA_AVC_OPERR_RESOURCE_UNSUPPORTED;
            else if ( result != LE_OK )
                opErr = PA_AVC_OPERR_INTERNAL;

            if ( opErr != PA_AVC_OPERR_NO_ERROR )
            {
                pa_avc_OperationReportError(opRef, opErr);
                return;
            }

            // Send the valid response
            pa_avc_OperationReportSuccess(opRef, NULL, 0);
            break;


        case PA_AVC_OPTYPE_EXECUTE:
            LE_DEBUG("PA_AVC_OPTYPE_EXEC %s/%d/%d/%d", newPrefixPtr, objId, objInstId, resourceId);

            // Execute must be on a specific resource

            result = assetData_server_Execute(instRef, resourceId);
            if ( result == LE_NOT_FOUND )
                opErr = PA_AVC_OPERR_RESOURCE_UNSUPPORTED;
            else if ( result != LE_OK )
                opErr = PA_AVC_OPERR_INTERNAL;

            if ( opErr != PA_AVC_OPERR_NO_ERROR )
            {
                pa_avc_OperationReportError(opRef, opErr);
                return;
            }

            // Send the valid response
            pa_avc_OperationReportSuccess(opRef, NULL, 0);
            break;


        case PA_AVC_OPTYPE_CREATE:
            LE_DEBUG("PA_AVC_OPTYPE_CREATE %s/%d/%d", newPrefixPtr, objId, objInstId);

            // Create is only supported on object "/lwm2m/9".
            if ( (strcmp(newPrefixPtr, "lwm2m") != 0) || (objId != 9) )
            {
                pa_avc_OperationReportError(opRef, PA_AVC_OPERR_OP_UNSUPPORTED);
                return;
            }

            // For now, assume instanceId is always generated.
            result = assetData_CreateInstanceById(newPrefixPtr, objId, -1, &instRef);
            if ( result != LE_OK )
            {
                pa_avc_OperationReportError(opRef, PA_AVC_OPERR_INTERNAL);
                return;
            }

            result = assetData_GetInstanceId(instRef, &instId);
            if ( result != LE_OK )
            {
                pa_avc_OperationReportError(opRef, PA_AVC_OPERR_INTERNAL);
                return;
            }

            // Fill in the new object instance with the received TLV in the payload
            result = assetData_ReadFieldListFromTLV((uint8_t*)payloadPtr, payloadLength, instRef, true);

            // Send the valid response
            FormatString((char*)ValueData, sizeof(ValueData), "%i", instId);

            pa_avc_OperationReportSuccess(opRef, ValueData, strlen((char*)ValueData));
            break;


        case PA_AVC_OPTYPE_DELETE:
            LE_DEBUG("PA_AVC_OPTYPE_DELETE %s/%d/%d", newPrefixPtr, objId, objInstId);

            assetData_DeleteInstance(instRef);
            pa_avc_OperationReportSuccess(opRef, NULL, 0);

            // Send registration update after the instance is removed.
            assetData_RegistrationUpdate(ASSET_DATA_SESSION_STATUS_CHECK);

            break;

        case PA_AVC_OPTYPE_OBSERVE:
            LE_DEBUG("PA_AVC_OPTYPE_OBSERVE %s/%d/%d", newPrefixPtr, objId, objInstId);

            // Observe not supported on object "/lwm2m/9".
            if ( (strcmp(newPrefixPtr, "lwm2m") == 0) && (objId == 9) )
            {
                LE_DEBUG("Observe not supported on %s/%d/%d", newPrefixPtr, objId, objInstId);
                pa_avc_OperationReportError(opRef, PA_AVC_OPERR_OP_UNSUPPORTED);
                return;
            }

            result = assetData_SetObserve(instRef, true, (uint8_t*)tokenPtr, tokenLength);

            if ( result == LE_NOT_FOUND )
                opErr = PA_AVC_OPERR_RESOURCE_UNSUPPORTED;
            else if ( result != LE_OK )
                opErr = PA_AVC_OPERR_INTERNAL;

            if ( opErr != PA_AVC_OPERR_NO_ERROR )
            {
                pa_avc_OperationReportError(opRef, opErr);
                return;
            }

            pa_avc_OperationReportSuccess(opRef, NULL, 0);
            break;

        // Observe cancel and reset are handled in the same way. i.e, stop sending notifications.
        case PA_AVC_OPTYPE_OBSERVE_CANCEL:
        case PA_AVC_OPTYPE_OBSERVE_RESET:
            LE_DEBUG("PA_AVC_OPTYPE_OBSERVE_CANCEL %s/%d/%d", newPrefixPtr, objId, objInstId);

            result = assetData_SetObserve(instRef, false, NULL, 0);

            if ( result == LE_NOT_FOUND )
                opErr = PA_AVC_OPERR_RESOURCE_UNSUPPORTED;
            else if ( result != LE_OK )
                opErr = PA_AVC_OPERR_INTERNAL;

            if ( opErr != PA_AVC_OPERR_NO_ERROR )
            {
                pa_avc_OperationReportError(opRef, opErr);
                return;
            }

            pa_avc_OperationReportSuccess(opRef, NULL, 0);
            break;

        default:
            LE_ERROR("OpType %i not currently supported", opType);
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

