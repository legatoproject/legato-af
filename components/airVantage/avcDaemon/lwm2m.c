/**
 * @file lwm2m.c
 *
 * Implementation of LWM2M handler sub-component
 *
 * This provides glue logic between QMI PA and assetData
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
// Local Data
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Used to delay reporting REG_UPDATE, so that we don't generate too much message traffic.
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t RegUpdateTimerRef;


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
    size_t payloadLength;

    // Get the operation details from the opRef
    opType = pa_avc_GetOpType(opRef);
    pa_avc_GetOpAddress(opRef, &objPrefixPtr, &objId, &objInstId, &resourceId);
    pa_avc_GetOpPayload(opRef, &payloadPtr, &payloadLength);

    le_result_t result;
    assetData_InstanceDataRef_t instRef;
    int instId;
    uint8_t valueData[256+1];  // +1 for null byte, if storing a string
    size_t bytesWritten;
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

        result = assetData_WriteObjectToTLV(assetRef,
                                            resourceId,
                                            valueData,
                                            sizeof(valueData),
                                            &bytesWritten);

        if ( result == LE_OVERFLOW )
            opErr = PA_AVC_OPERR_OBJ_INST_UNAVAIL;
        else if ( result != LE_OK )
            opErr = PA_AVC_OPERR_INTERNAL;

        if ( opErr != PA_AVC_OPERR_NO_ERROR )
        {
            pa_avc_OperationReportError(opRef, opErr);
        }
        else
        {
            // Send the valid response
            pa_avc_OperationReportSuccess(opRef, valueData, bytesWritten);
        }

        // TODO: Refactor so I don't need a return here.
        return;
    }


    // These operations all need a valid instanceRef.  Ensure that the specified instance exists,
    // and get the instanceRef; this check is common across several of the opTypes.
    if ( (opType == PA_AVC_OPTYPE_READ) ||
         (opType == PA_AVC_OPTYPE_WRITE) ||
         (opType == PA_AVC_OPTYPE_EXECUTE) ||
         (opType == PA_AVC_OPTYPE_DELETE) )
    {
        result = assetData_GetInstanceRefById(newPrefixPtr, objId, objInstId, &instRef);
        if ( result == LE_NOT_FOUND )
            opErr = PA_AVC_OPERR_OBJ_INST_UNAVAIL;
        else if ( result != LE_OK )
            opErr = PA_AVC_OPERR_INTERNAL;

        if ( opErr != PA_AVC_OPERR_NO_ERROR )
        {
            pa_avc_OperationReportError(opRef, opErr);
            return;
        }
    }

    switch ( opType )
    {
        case PA_AVC_OPTYPE_READ:
            LE_DEBUG("PA_AVC_OPTYPE_READ %s/%d/%d/%d", newPrefixPtr, objId, objInstId, resourceId);

            if ( resourceId == -1 )
            {
                result = assetData_WriteFieldListToTLV(instRef,
                                                       valueData,
                                                       sizeof(valueData),
                                                       &bytesWritten);

            }
            else
            {
                result = assetData_server_GetValue(instRef,
                                                   resourceId,
                                                   (char*)valueData,
                                                   sizeof(valueData));
                bytesWritten = strlen((char*)valueData);
            }

            if ( result == LE_NOT_FOUND )
                opErr = PA_AVC_OPERR_RESOURCE_UNSUPPORTED;
            else if ( result == LE_OVERFLOW )
                opErr = PA_AVC_OPERR_OBJ_INST_UNAVAIL;
            else if ( result != LE_OK )
                opErr = PA_AVC_OPERR_INTERNAL;

            if ( opErr != PA_AVC_OPERR_NO_ERROR )
            {
                pa_avc_OperationReportError(opRef, opErr);
                return;
            }

            // Send the valid response
            pa_avc_OperationReportSuccess(opRef, valueData, bytesWritten);
            break;


        case PA_AVC_OPTYPE_WRITE:
            LE_DEBUG("PA_AVC_OPTYPE_WRITE %s/%d/%d/%d", newPrefixPtr, objId, objInstId, resourceId);

            if ( resourceId == -1 )
            {
                result = assetData_ReadFieldListFromTLV((uint8_t*)payloadPtr, payloadLength, instRef);
            }
            else
            {
                // The payload is a string, but can't be guaranteed that it is null terminated,
                // so copy to local buffer and null terminate it.  The payload length has already
                // been checked, so we know it will fit in the buffer.
                memcpy(valueData, payloadPtr, payloadLength);
                valueData[payloadLength] = 0;

                result = assetData_server_SetValue(instRef, resourceId, (const char*)valueData);
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
            result = assetData_ReadFieldListFromTLV((uint8_t*)payloadPtr, payloadLength, instRef);

            // Send the valid response
            FormatString((char*)valueData, sizeof(valueData), "%i", instId);

            pa_avc_OperationReportSuccess(opRef, valueData, strlen((char*)valueData));
            break;


        case PA_AVC_OPTYPE_DELETE:
            LE_DEBUG("PA_AVC_OPTYPE_DELETE %s/%d/%d", newPrefixPtr, objId, objInstId);

            assetData_DeleteInstance(instRef);
            pa_avc_OperationReportSuccess(opRef, NULL, 0);
            break;

        default:
            LE_ERROR("OpType %i not currently supported", opType);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for receiving UpdateRequired indication
 */
//--------------------------------------------------------------------------------------------------
static void UpdateRequiredHandler
(
    void
)
{
    // This size must the same as OBJ_PATH_MAX_LEN_V01 in qapi_lwm2m_v01.h
    char assetList[4032];
    int listSize;
    int numAssets;

    assetData_GetAssetList(assetList, sizeof(assetList), &listSize, &numAssets);
    pa_avc_RegistrationUpdate(assetList, listSize, numAssets);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for RegUpdateTimerRef expiry
 */
//--------------------------------------------------------------------------------------------------
static void RegUpdateTimerHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    LE_INFO("RegUpdate timer expired; reporting REG_UPDATE");

    UpdateRequiredHandler();
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for instance creation for any asset
 */
//--------------------------------------------------------------------------------------------------
static void AssetActionHandler
(
    assetData_AssetDataRef_t assetRef,
    int instanceId,
    assetData_ActionTypes_t action,
    void* contextPtr
)
{
    char appName[100];
    int assetId;

    if ( assetData_GetAppNameFromAsset(assetRef, appName, sizeof(appName)) != LE_OK )
    {
        LE_ERROR("Can't get app name from assetRef=%p", assetRef);
        return;
    }

    if ( assetData_GetAssetIdFromAsset(assetRef, &assetId) != LE_OK )
    {
        LE_ERROR("Can't get assetId for app '%s' from assetRef=%p", appName, assetRef);
        return;
    }

    // Only interested in CREATE or DELETE actions; anything else is an error
    if ( action == ASSET_DATA_ACTION_CREATE )
    {
        LE_INFO("/%s/%d/%d created.", appName, assetId, instanceId);

        // Start or restart the timer; will only report to the modem when the timer expires.
        // TODO: Probably need to revisit how this is done.
        le_timer_Restart(RegUpdateTimerRef);
    }
    else if ( action == ASSET_DATA_ACTION_DELETE )
    {
        LE_INFO("/%s/%d/%d deleted.", appName, assetId, instanceId);

        // Start or restart the timer; will only report to the modem when the timer expires.
        // TODO: Probably need to revisit how this is done.
        le_timer_Restart(RegUpdateTimerRef);
    }
    else
    {
        LE_ERROR("Unexpected action %i on /%s/%d/%d.", action, appName, assetId, instanceId);
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
    pa_avc_SetLWM2MUpdateRequiredHandler(UpdateRequiredHandler);

    // Get instance creation or deletion events for any asset
    assetData_server_SetAllAssetActionHandler(AssetActionHandler, NULL);

    // Use a timer to delay reporting instance creation events to the modem for 15 seconds after
    // the last creation event.  The timer will only be started when the creation event happens.
    le_clk_Time_t timerInterval = { .sec=15, .usec=0 };

    RegUpdateTimerRef = le_timer_Create("RegUpdate timer");
    le_timer_SetInterval(RegUpdateTimerRef, timerInterval);
    le_timer_SetHandler(RegUpdateTimerRef, RegUpdateTimerHandler);

    return LE_OK;
}

