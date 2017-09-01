/**
 * @file avData.c
 *
 * Implementation of AirVantage Data sub-component
 *
 * This implements the server side of the avdata API
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"

#include "assetData.h"
#include "avcServer.h"

#include "le_print.h"



//--------------------------------------------------------------------------------------------------
// Macros
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Data associated with a handler registered against field activity events
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_avdata_FieldHandlerFunc_t handlerPtr;    ///< User supplied handler
    void* contextPtr;                           ///< User supplied context pointer
    le_avdata_AssetInstanceRef_t instRef;       ///< Instance registered against
    char fieldName[100];                        ///< Field registered against
    void* safeRef;                              ///< SafeRef for instance registered against
    assetData_FieldActionHandlerRef_t addRef;   ///< Ref returned when registering with assetData
}
FieldEventData_t;


//--------------------------------------------------------------------------------------------------
/**
 * Data associated with an instance reference. This is used for keeping track of which client
 * is using the instance ref, so that everything can be cleaned up when the client dies.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_avdata_AssetInstanceRef_t instRef;       ///< Instance ref
    le_msg_SessionRef_t clientSessionRef;       ///< Client using this instance ref
}
InstanceRefData_t;


//--------------------------------------------------------------------------------------------------
// Local Data
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Field event memory pool.  Initialized in avData_Init().
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t FieldEventDataPoolRef = NULL;



//--------------------------------------------------------------------------------------------------
/**
 * Safe reference map for the avms session request.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t AvSessionRequestRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * Instance reference data memory pool. Used for keeping track of the client that is using a
 * specific instance ref. Initialized in avData_Init().
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t InstanceRefDataPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * This timer is used to delay releasing the session.
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t SessionReleaseTimerRef;


//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for instance references. Initialized in avData_Init().
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t InstanceRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * Event for sending session state to registered applications.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t SessionStateEvent;

//--------------------------------------------------------------------------------------------------
// Local functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Local handler registered with assetData component for field actions
 */
//--------------------------------------------------------------------------------------------------
static void FieldActionHandler
(
    assetData_InstanceDataRef_t instanceRef,
    int fieldId,
    assetData_ActionTypes_t action,
    void* contextPtr
)
{
    // Get the handler data from the contextPtr
    LE_ASSERT( contextPtr != NULL );
    FieldEventData_t* handlerDataPtr = contextPtr;

    // Ensure the action happens on the desired instance.  This could happen since we register
    // against the asset, rather than an instance of the asset.
    // NOTE: Don't need to check for fieldId, since they should always match.
    if ( handlerDataPtr->instRef != instanceRef )
    {
        LE_DEBUG("Action %i not expected for this instance, so ignore it", action);
        return;
    }

    LE_DEBUG("Got action=%i, for field='%s'", action, handlerDataPtr->fieldName);

    // Call the user supplied handler
    handlerDataPtr->handlerPtr(handlerDataPtr->safeRef,
                               handlerDataPtr->fieldName,
                               handlerDataPtr->contextPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Session release timer expired
 */
//--------------------------------------------------------------------------------------------------
static void SessionReleaseTimerHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    LE_INFO("SessionRelease timer expired; close session");

    avcServer_ReleaseSession();
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler for client session closes
 */
//--------------------------------------------------------------------------------------------------
static void ClientCloseSessionHandler
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
{
    if ( sessionRef == NULL )
    {
        LE_ERROR("sessionRef is NULL");
        return;
    }

    LE_INFO("Client %p closed, remove allocated resources", sessionRef);

    le_ref_IterRef_t iterRef = le_ref_GetIterator(InstanceRefMap);
    InstanceRefData_t const* instRefDataPtr;

    // Search for the instance references used by the closed client, and clean up any data.
    while ( le_ref_NextNode(iterRef) == LE_OK )
    {
        instRefDataPtr = le_ref_GetValue(iterRef);

        if ( instRefDataPtr->clientSessionRef == sessionRef )
        {
            // Delete instance data, and also delete asset data, if last instance is deleted
            assetData_DeleteInstanceAndAsset(instRefDataPtr->instRef);

            // Delete safe reference and associated data
            le_mem_Release((void*)instRefDataPtr);
            le_ref_DeleteRef( InstanceRefMap, (void*)le_ref_GetSafeRef(iterRef) );
        }
    }

    // Send registration update after the asset is removed.
    assetData_RegistrationUpdate(ASSET_DATA_SESSION_STATUS_CHECK);

    // Search for the session request reference(s) used by the closed client, and clean up any data.
    iterRef = le_ref_GetIterator(AvSessionRequestRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        if (le_ref_GetValue(iterRef) == sessionRef)
        {
            le_avdata_ReleaseSession((void*)le_ref_GetSafeRef(iterRef));
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the real instance ref from the safe ref
 *
 * TODO: Do I really want this as a separate function?
 */
//--------------------------------------------------------------------------------------------------
le_avdata_AssetInstanceRef_t GetInstRefFromSafeRef
(
    void* safeRef,
    const char* funcNamePtr
)
{
    InstanceRefData_t* instRefDataPtr = le_ref_Lookup(InstanceRefMap, safeRef);

    if ( instRefDataPtr == NULL )
    {
        LE_KILL_CLIENT("Invalid reference %p from %s", safeRef, funcNamePtr);
        return NULL;
    }

    return instRefDataPtr->instRef;
}


//--------------------------------------------------------------------------------------------------
// Interface functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_avdata_FieldEvent'
 *
 * This event provides information on field activity
 */
//--------------------------------------------------------------------------------------------------
le_avdata_FieldEventHandlerRef_t le_avdata_AddFieldEventHandler
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    le_avdata_FieldHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    // Map safeRef to desired data
    void* safeRef = instRef;
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;
    assetData_AssetDataRef_t assetRef;
    FieldEventData_t* newHandlerDataPtr;

    // Get the associated field id
    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
        LE_KILL_CLIENT("Field name '%s' is not defined", fieldName);
        return NULL;
    }

    // Get the associated asset ref, since we can't register against a single instance.
    if ( assetData_GetAssetRefFromInstance(instRef, &assetRef) != LE_OK )
    {
        LE_KILL_CLIENT("Invalid instance");
        return NULL;

    }

    // Save the required data, and register our handler
    newHandlerDataPtr = le_mem_ForceAlloc(FieldEventDataPoolRef);
    newHandlerDataPtr->handlerPtr = handlerPtr;
    newHandlerDataPtr->contextPtr = contextPtr;
    newHandlerDataPtr->instRef = instRef;
    newHandlerDataPtr->safeRef = safeRef;
    if ( le_utf8_Copy(newHandlerDataPtr->fieldName,
                      fieldName,
                      sizeof(newHandlerDataPtr->fieldName),
                      NULL) != LE_OK )
    {
        // TODO: This should probably never happen, should it?
        LE_WARN("Field name '%s' truncated", fieldName);
    }

    // Register the handler
    newHandlerDataPtr->addRef = assetData_client_AddFieldActionHandler(assetRef,
                                                                       fieldId,
                                                                       FieldActionHandler,
                                                                       newHandlerDataPtr);

    // Return an appropriate ref
    return (le_avdata_FieldEventHandlerRef_t)newHandlerDataPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_avdata_FieldEvent'
 */
//--------------------------------------------------------------------------------------------------
void le_avdata_RemoveFieldEventHandler
(
    le_avdata_FieldEventHandlerRef_t addHandlerRef
        ///< [IN]
)
{
    FieldEventData_t* handlerDataPtr = (FieldEventData_t*)addHandlerRef;

    // Unregister the handler
    assetData_client_RemoveFieldActionHandler(handlerDataPtr->addRef);

    // Clean-up data
    le_mem_Release(handlerDataPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Create an instance of AirVantage asset
 *
 * @return Reference to the asset instance
 *
 * @note It is a fatal error if the asset is not defined.
 */
//--------------------------------------------------------------------------------------------------
le_avdata_AssetInstanceRef_t le_avdata_Create
(
    const char* assetName
        ///< [IN]
)
{
    // Get the client's credentials.
    pid_t pid;
    uid_t uid;

    if (le_msg_GetClientUserCreds(le_avdata_GetClientSessionRef(), &uid, &pid) != LE_OK)
    {
        LE_KILL_CLIENT("Could not get credentials for the client.");
        return NULL;
    }


    // Look up the process's application name.
    char appName[LE_LIMIT_APP_NAME_LEN+1];

    le_result_t result = le_appInfo_GetName(pid, appName, sizeof(appName));
    LE_FATAL_IF(result == LE_OVERFLOW, "Buffer too small to contain the application name.");

    // TODO: Should this be LE_KILL_CLIENT instead?
    LE_FATAL_IF(result != LE_OK, "Could not get app name");


    // Create an instance of the asset
    assetData_InstanceDataRef_t instRef;
    int instanceId;

    LE_ASSERT( assetData_CreateInstanceByName(appName, assetName, -1, &instRef) == LE_OK );
    LE_ASSERT( instRef != NULL );
    LE_ASSERT( assetData_GetInstanceId(instRef, &instanceId) == LE_OK );
    LE_PRINT_VALUE("%i", instanceId);

    // Return a safe reference for the instance
    InstanceRefData_t* instRefDataPtr = le_mem_ForceAlloc(InstanceRefDataPoolRef);

    instRefDataPtr->clientSessionRef = le_avdata_GetClientSessionRef();
    instRefDataPtr->instRef = instRef;

    instRef = le_ref_CreateRef(InstanceRefMap, instRefDataPtr);

    return instRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete an instance of AirVantage asset
 *
 * @note It is a fatal error if the instRef is not valid
 */
//--------------------------------------------------------------------------------------------------
void le_avdata_Delete
(
    le_avdata_AssetInstanceRef_t instRef
        ///< [IN]
)
{
    LE_ERROR("Not implemented yet");
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the value of an integer setting field
 *
 * @note The client will be terminated if the instRef is not valid, or the field doesn't exist
 */
//--------------------------------------------------------------------------------------------------
void le_avdata_GetInt
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    int32_t* valuePtr
        ///< [OUT]
)
{
    // Map safeRef to desired data
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;

    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
        LE_KILL_CLIENT("Invalid instance '%p' or unknown field name '%s'", instRef, fieldName);
    }

    if ( assetData_client_GetInt(instRef, fieldId, valuePtr) != LE_OK )
    {
        LE_ERROR("Error getting field=%i", fieldId);
        *valuePtr=0;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the value of an integer variable field
 *
 * @note The client will be terminated if the instRef is not valid, or the field doesn't exist
 *
 * @return:
 *      - LE_OK on success
 *      - LE_OVERFLOW if the current entry was NOT added as the time series buffer is full.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_NO_MEMORY if the current entry was added but there is no space for next one.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avdata_SetInt
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    int32_t value
        ///< [IN]
)
{
    le_result_t result;

    // Map safeRef to desired data
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;

    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
        LE_KILL_CLIENT("Invalid instance '%p' or unknown field name '%s'", instRef, fieldName);
    }

    result = assetData_client_SetInt(instRef, fieldId, value);

    if (result == LE_NO_MEMORY)
    {
        LE_WARN("Time series buffer full for field=%i", fieldId);
    }
    else if (result != LE_OK)
    {
        LE_ERROR("Error setting field=%i", fieldId);
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the value of a float setting field
 *
 * @note The client will be terminated if the instRef is not valid, or the field doesn't exist
 *
 */
//--------------------------------------------------------------------------------------------------
void le_avdata_GetFloat
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    double* valuePtr
        ///< [OUT]
)
{
    // Map safeRef to desired data
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;

    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
        LE_KILL_CLIENT("Invalid instance '%p' or unknown field name '%s'", instRef, fieldName);
    }

    if ( assetData_client_GetFloat(instRef, fieldId, valuePtr) != LE_OK )
    {
        LE_ERROR("Error getting field=%i", fieldId);
        *valuePtr=0;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the value of a float variable field
 *
 * @note The client will be terminated if the instRef is not valid, or the field doesn't exist
 *
 * @return:
 *      - LE_OK on success
 *      - LE_OVERFLOW if the current entry was NOT added as the time series buffer is full.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_NO_MEMORY if the current entry was added but there is no space for next one.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avdata_SetFloat
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    double value
        ///< [IN]
)
{
    le_result_t result;

    // Map safeRef to desired data
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;

    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
        LE_KILL_CLIENT("Invalid instance '%p' or unknown field name '%s'", instRef, fieldName);
    }

    result = assetData_client_SetFloat(instRef, fieldId, value);

    if (result == LE_NO_MEMORY)
    {
        LE_WARN("Time series buffer full for field=%i", fieldId);
    }
    else if (result != LE_OK)
    {
        LE_ERROR("Error setting field=%i", fieldId);
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the value of a boolean setting field
 *
 * @note The client will be terminated if the instRef is not valid, or the field doesn't exist
 */
//--------------------------------------------------------------------------------------------------
void le_avdata_GetBool
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    bool* valuePtr
        ///< [OUT]
)
{
    // Map safeRef to desired data
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;

    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
        LE_KILL_CLIENT("Invalid instance '%p' or unknown field name '%s'", instRef, fieldName);
    }

    if ( assetData_client_GetBool(instRef, fieldId, valuePtr) != LE_OK )
    {
        LE_ERROR("Error getting field=%i", fieldId);
        *valuePtr=false;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the value of a boolean variable field
 *
 * @note The client will be terminated if the instRef is not valid, or the field doesn't exist
 *
 * @return:
 *      - LE_OK on success
 *      - LE_OVERFLOW if the current entry was NOT added as the time series buffer is full.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_NO_MEMORY if the current entry was added but there is no space for next one.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avdata_SetBool
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    bool value
        ///< [IN]
)
{
    le_result_t result;

    // Map safeRef to desired data
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;

    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
        LE_KILL_CLIENT("Invalid instance '%p' or unknown field name '%s'", instRef, fieldName);
    }

    result = assetData_client_SetBool(instRef, fieldId, value);

    if (result == LE_NO_MEMORY)
    {
        LE_WARN("Time series buffer full for field=%i", fieldId);
    }
    else if (result != LE_OK)
    {
        LE_ERROR("Error setting field=%i", fieldId);
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the value of a string setting field
 *
 * @note The client will be terminated if the instRef is not valid, or the field doesn't exist
 */
//--------------------------------------------------------------------------------------------------
void le_avdata_GetString
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    char* value,
        ///< [OUT]

    size_t valueNumElements
        ///< [IN]
)
{
    // Map safeRef to desired data
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;

    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
        LE_KILL_CLIENT("Invalid instance '%p' or unknown field name '%s'", instRef, fieldName);
    }

    if ( assetData_client_GetString(instRef, fieldId, value, valueNumElements) != LE_OK )
    {
        LE_ERROR("Error getting field=%i", fieldId);
        *value = '\0';
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the value of a string variable field
 *
 * @note The client will be terminated if the instRef is not valid, or the field doesn't exist
 *
 * @return:
 *      - LE_OK on success
 *      - LE_OVERFLOW if the stored string was truncated or
 *                    if the current entry was NOT added as the time series buffer is full.
 *      - LE_NO_MEMORY if the current entry was added but there is no space for next one.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avdata_SetString
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    const char* value
        ///< [IN]
)
{
    le_result_t result;

    // Map safeRef to desired data
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;

    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
        LE_KILL_CLIENT("Invalid instance '%p' or unknown field name '%s'", instRef, fieldName);
    }

    result = assetData_client_SetString(instRef, fieldId, value);

    if (result == LE_NO_MEMORY)
    {
        LE_WARN("Time series buffer full for field=%i", fieldId);
    }
    else if (result != LE_OK)
    {
        LE_ERROR("Error setting field=%i", fieldId);
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the value of a binary data setting field
 *
 * @note The client will be terminated if the instRef is not valid, or the field doesn't exist
 */
//--------------------------------------------------------------------------------------------------
void le_avdata_GetBinary
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    uint8_t* valuePtr,
        ///< [OUT]

    size_t* valueNumElementsPtr
        ///< [INOUT]
)
{
    LE_ERROR("Not implemented yet");
    *valueNumElementsPtr = 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the value of a binary data variable field
 *
 * @note The client will be terminated if the instRef is not valid, or the field doesn't exist
 */
//--------------------------------------------------------------------------------------------------
void le_avdata_SetBinary
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    const uint8_t* valuePtr,
        ///< [IN]

    size_t valueNumElements
        ///< [IN]
)
{
    LE_ERROR("Not implemented yet");
}



//--------------------------------------------------------------------------------------------------
/**
 * Called by avcServer when the session started or stopped.
 */
//--------------------------------------------------------------------------------------------------
void avData_ReportSessionState
(
    le_avdata_SessionState_t sessionState
)
{
    LE_DEBUG("Reporting session state %d", sessionState);

    // Send the event to interested applications
    le_event_Report(SessionStateEvent, &sessionState, sizeof(sessionState));
}


//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Session State Handler
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerSessionStateHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    bool* eventDataPtr = reportPtr;
    le_avdata_SessionStateHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(*eventDataPtr, le_event_GetContextPtr());
}


//--------------------------------------------------------------------------------------------------
/**
 * This function adds a handler ...
 */
//--------------------------------------------------------------------------------------------------
le_avdata_SessionStateHandlerRef_t le_avdata_AddSessionStateHandler
(
    le_avdata_SessionStateHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_PRINT_VALUE("%p", handlerPtr);
    LE_PRINT_VALUE("%p", contextPtr);

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("AVSessionState",
                                                                  SessionStateEvent,
                                                                  FirstLayerSessionStateHandler,
                                                                  (le_event_HandlerFunc_t)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_avdata_SessionStateHandlerRef_t)(handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function removes a handler ...
 */
//--------------------------------------------------------------------------------------------------
void le_avdata_RemoveSessionStateHandler
(
    le_avdata_SessionStateHandlerRef_t addHandlerRef
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)addHandlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Request to open an avms session.
 */
//--------------------------------------------------------------------------------------------------
le_avdata_RequestSessionObjRef_t le_avdata_RequestSession
(
    void
)
{
    le_result_t result = LE_OK;

    // If this is a duplicate request send the existing reference.
    le_ref_IterRef_t iterRef = le_ref_GetIterator(AvSessionRequestRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        if (le_ref_GetValue(iterRef) == le_avdata_GetClientSessionRef())
        {
            LE_DEBUG("Duplicate session request from client.");
            return (le_avdata_RequestSessionObjRef_t) le_ref_GetSafeRef(iterRef);
        }
    }

    le_timer_Stop(SessionReleaseTimerRef);

    // Ask the avc server to pass the request to control app or to initiate a session.
    result = avcServer_RequestSession();

    // If the fresh request fails, return NULL.
    if (result != LE_OK)
    {
        return NULL;
    }

    // Need to return a unique reference that will be used by release. Use the client session ref
    // as the data, since we need to delete the ref when the client closes.
    le_avdata_RequestSessionObjRef_t requestRef = le_ref_CreateRef(AvSessionRequestRefMap,
                                                                   le_avdata_GetClientSessionRef());

    return requestRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Request to close an avms session.
 */
//--------------------------------------------------------------------------------------------------
void le_avdata_ReleaseSession
(
    le_avdata_RequestSessionObjRef_t  sessionRequestRef
)
{
    le_ref_IterRef_t iterRef;

    // Look up the reference.  If it is NULL, then the reference is not valid.
    // Otherwise, delete the reference and request avcServer to release session.
    void* sessionPtr = le_ref_Lookup(AvSessionRequestRefMap, sessionRequestRef);
    if (sessionPtr == NULL)
    {
        LE_ERROR("Invalid session request reference %p", sessionPtr);
        return;
    }
    else
    {
        LE_PRINT_VALUE("%p", sessionPtr);
        le_ref_DeleteRef(AvSessionRequestRefMap, sessionRequestRef);
    }

    // Stop the session when all clients release their session reference.
    iterRef = le_ref_GetIterator(AvSessionRequestRefMap);

    if (le_ref_NextNode(iterRef) == LE_NOT_FOUND)
    {
        // Close the session if there is no new open request for 2 seconds.
        le_timer_Restart(SessionReleaseTimerRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Is this resource enabled for observe notifications?
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if field not found
 *
 * @note client will be terminated if instRef isn't valid, or the field doesn't exist
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avdata_IsObserve
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    bool* isObserve
        ///< [IN]
)
{
    // Map safeRef to desired data
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;

    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
       LE_KILL_CLIENT("Invalid instance '%p' or unknown field name '%s'", instRef, fieldName);
    }

    if ( assetData_client_IsObserve(instRef, fieldId, isObserve) != LE_OK )
    {
        LE_ERROR("Error getting field=%i", fieldId);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Allocate resources and start accumulating time series data on the specified field.
 *
 * @note client will be terminated if instRef isn't valid, or the field doesn't exist
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avdata_StartTimeSeries
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    double factor,
        ///< [IN]

    double timeStampFactor
        ///< [IN]
)
{
    // Map safeRef to desired data
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;

    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
        LE_KILL_CLIENT("Invalid instance '%p' or unknown field name '%s'", instRef, fieldName);
    }

    if ( assetData_client_StartTimeSeries(instRef, fieldId, factor, timeStampFactor) != LE_OK )
    {
        LE_ERROR("Error setting time series on field =%i", fieldId);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stop time series on this field and free resources.
 *
 * @note client will be terminated if instRef isn't valid, or the field doesn't exist
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avdata_StopTimeSeries
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName
        ///< [IN]
)
{
    // Map safeRef to desired data
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;

    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
        LE_KILL_CLIENT("Invalid instance '%p' or unknown field name '%s'", instRef, fieldName);
    }

    if ( assetData_client_StopTimeSeries(instRef, fieldId) != LE_OK )
    {
        LE_ERROR("Error stopping time series on field =%i", fieldId);
        return LE_FAULT;
    }

    return LE_OK;
}



//--------------------------------------------------------------------------------------------------
/**
 * Compress the accumulated CBOR encoded time series data and send it to server.
 *
 * @note client will be terminated if instRef isn't valid, or the field doesn't exist
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avdata_PushTimeSeries
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    bool isRestartTimeSeries
        ///< [IN]
)
{
    // Map safeRef to desired data
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;

    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
        LE_KILL_CLIENT("Invalid instance '%p' or unknown field name '%s'", instRef, fieldName);
    }

    if ( assetData_client_PushTimeSeries(instRef, fieldId, isRestartTimeSeries) != LE_OK )
    {
        LE_ERROR("Error flushing time series on field =%i", fieldId);
        return LE_FAULT;
    }

    return LE_OK;
}



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
 *      - LE_OVERFLOW if the current entry was NOT added as the time series buffer is full.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_NO_MEMORY if the current entry was added but there is no space for next one.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avdata_RecordInt
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    int32_t value,
        ///< [IN]

    uint64_t timeStamp
        ///< [IN]
)
{
    le_result_t result;

    // Map safeRef to desired data
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;

    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
        LE_KILL_CLIENT("Invalid instance '%p' or unknown field name '%s'", instRef, fieldName);
    }

    result = assetData_client_RecordInt(instRef, fieldId, value, timeStamp);

    if (result == LE_NO_MEMORY)
    {
        LE_WARN("Time series buffer full for field=%i", fieldId);
    }
    else if (result != LE_OK)
    {
        LE_ERROR("Error setting field=%i", fieldId);
    }

    return result;
}



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
 *      - LE_OVERFLOW if the current entry was NOT added as the time series buffer is full.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_NO_MEMORY if the current entry was added but there is no space for next one.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avdata_RecordFloat
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    double value,
        ///< [IN]

    uint64_t timeStamp
        ///< [IN]
)
{
    le_result_t result;

    // Map safeRef to desired data
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;

    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
        LE_KILL_CLIENT("Invalid instance '%p' or unknown field name '%s'", instRef, fieldName);
    }

    result = assetData_client_RecordFloat(instRef, fieldId, value, timeStamp);

    if (result == LE_NO_MEMORY)
    {
        LE_WARN("Time series buffer full for field=%i", fieldId);
    }
    else if (result != LE_OK)
    {
        LE_ERROR("Error setting field=%i", fieldId);
    }

    return result;
}


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
 *      - LE_OVERFLOW if the current entry was NOT added as the time series buffer is full.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_NO_MEMORY if the current entry was added but there is no space for next one.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avdata_RecordBool
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    bool value,
        ///< [IN]

    uint64_t timeStamp
        ///< [IN]
)
{
    le_result_t result;

    // Map safeRef to desired data
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;

    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
        LE_KILL_CLIENT("Invalid instance '%p' or unknown field name '%s'", instRef, fieldName);
    }

    result = assetData_client_RecordBool(instRef, fieldId, value, timeStamp);

    if (result == LE_NO_MEMORY)
    {
        LE_WARN("Time series buffer full for field=%i", fieldId);
    }
    else if (result != LE_OK)
    {
        LE_ERROR("Error setting field=%i", fieldId);
    }

    return result;
}



//--------------------------------------------------------------------------------------------------
/**
 * Record the value of a string variable field in time series
 *
 * @note The client will be terminated if the instRef is not valid, or the field doesn't exist
 *
 * @note This function is the same as the SetString() except that it provides an option to pass the
 *       timestamp. SetString() can be used to record time series with system time as the timestamp.
 *       Timestamp should be in milli seconds elapsed since epoch.
 *
 * @return:
 *      - LE_OK on success
 *      - LE_OVERFLOW if the stored string was truncated or
 *                    if the current entry was NOT added as the time series buffer is full.
 *      - LE_NO_MEMORY if the current entry was added but there is no space for next one.
 *                    (This error is applicable only if time series is enabled on this field)
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avdata_RecordString
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    const char* value,
        ///< [IN]

    uint64_t timeStamp
        ///< [IN]
)
{
    le_result_t result;

    // Map safeRef to desired data
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;

    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
        LE_KILL_CLIENT("Invalid instance '%p' or unknown field name '%s'", instRef, fieldName);
    }

    result = assetData_client_RecordString(instRef, fieldId, value, timeStamp);

    if (result == LE_NO_MEMORY)
    {
        LE_WARN("Time series buffer full for field=%i", fieldId);
    }
    else if (result != LE_OK)
    {
        LE_ERROR("Error setting field=%i", fieldId);
    }

    return result;
}



//--------------------------------------------------------------------------------------------------
/**
 * Is time series enabled on this resource, if yes how many data points are recorded so far?
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT if time series not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avdata_GetTimeSeriesStatus
(
    le_avdata_AssetInstanceRef_t instRef,
        ///< [IN]

    const char* fieldName,
        ///< [IN]

    bool *isTimeSeriesPtr,
        ///< [OUT],

    int *numDataPointsPtr
        ///< [OUT],
)
{
    // Map safeRef to desired data
    instRef = GetInstRefFromSafeRef(instRef, __func__);

    int fieldId;

    if ( assetData_GetFieldIdFromName(instRef, fieldName, &fieldId) != LE_OK )
    {
        LE_KILL_CLIENT("Invalid instance '%p' or unknown field name '%s'", instRef, fieldName);
    }

    if ( assetData_client_GetTimeSeriesStatus(instRef, fieldId,
                                              isTimeSeriesPtr, numDataPointsPtr) != LE_OK )
    {
        LE_ERROR("Error reading time series status on field =%i", fieldId);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Init this sub-component
 */
//--------------------------------------------------------------------------------------------------
void avData_Init
(
    void
)
{
    SessionStateEvent = le_event_CreateId("Session state", sizeof(le_avdata_SessionState_t));

    // Create safe reference map for session request references. The size of the map should be based on
    // the expected number of simultaneous requests for session. 5 of them seems reasonable.
    AvSessionRequestRefMap = le_ref_CreateMap("AVSessionRequestRef", 5);

    FieldEventDataPoolRef = le_mem_CreatePool("Field event data pool", sizeof(FieldEventData_t));
    InstanceRefDataPoolRef = le_mem_CreatePool("Instance ref data pool", sizeof(InstanceRefData_t));

    // Create safe reference map for instance references. The size of the map should be based on
    // the expected number of user data instances across all apps.  For now, budget for 30 apps
    // and 10 instances per app.  This can always be increased/decreased later, if needed.
    InstanceRefMap = le_ref_CreateMap("InstRefMap", 300);

    // Add a handler for client session closes
    le_msg_AddServiceCloseHandler(le_avdata_GetServiceRef(), ClientCloseSessionHandler, NULL);

    // Use a timer to delay releasing the session for 2 seconds.
    le_clk_Time_t timerInterval = { .sec=2, .usec=0 };

    SessionReleaseTimerRef = le_timer_Create("Session Release timer");
    le_timer_SetInterval(SessionReleaseTimerRef, timerInterval);
    le_timer_SetHandler(SessionReleaseTimerRef, SessionReleaseTimerHandler);
}

