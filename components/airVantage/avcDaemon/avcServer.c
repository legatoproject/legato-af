/**
 * @file avcServer.c
 *
 * AirVantage Controller Daemon
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "interfaces.h"

#include "pa_avc.h"
#include "avcServer.h"
#include "assetData.h"
#include "lwm2m.h"

#include "le_print.h"

//--------------------------------------------------------------------------------------------------
// Definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This ref is returned when a status handler is added/registered.  It is used when the handler is
 * removed.  Only one ref is needed, because only one handler can be registered at a time.
 */
//--------------------------------------------------------------------------------------------------
#define REGISTERED_HANDLER_REF ((le_avc_StatusEventHandlerRef_t)0x1234)

//--------------------------------------------------------------------------------------------------
/**
 * This is the default defer time (in minutes) if an install is blocked by a user app.  Should
 * probably be a prime number.
 *
 * Use small number to ensure deferred installs happen quickly, once no longer deferred.
 */
//--------------------------------------------------------------------------------------------------
#define BLOCKED_DEFER_TIME 3

//--------------------------------------------------------------------------------------------------
/**
 * Current internal state.
 *
 * Used mainly to ensure that API functions don't do anything if in the wrong state.
 *
 * TODO: May need to revisit some of the state transitions here.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    AVC_IDLE,                    ///< No updates pending or in progress
    AVC_DOWNLOAD_PENDING,        ///< Received pending download; no response sent yet
    AVC_DOWNLOAD_IN_PROGRESS,    ///< Accepted download, and in progress
    AVC_INSTALL_PENDING,         ///< Received pending install; no response sent yet
    AVC_INSTALL_IN_PROGRESS      ///< Accepted install, and in progress
}
AvcState_t;


//--------------------------------------------------------------------------------------------------
// Data structures
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * The current state of any update.
 *
 * Although this variable is accessed both in API functions and in UpdateHandler(), access locks
 * are not needed.  This is because this is running as a daemon, and so everything runs in the
 * main thread.
 */
//--------------------------------------------------------------------------------------------------
static AvcState_t CurrentState = AVC_IDLE;

//--------------------------------------------------------------------------------------------------
/**
 * The type of the current update.  Only valid if CurrentState is not AVC_IDLE
 */
//--------------------------------------------------------------------------------------------------
static le_avc_UpdateType_t CurrentUpdateType = LE_AVC_UNKNOWN_UPDATE;

//--------------------------------------------------------------------------------------------------
/**
 * User registered handler to receive status updates.  Only one is allowed
 */
//--------------------------------------------------------------------------------------------------
static le_avc_StatusHandlerFunc_t StatusHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Context pointer associated with the above user registered handler to receive status updates.
 */
//--------------------------------------------------------------------------------------------------
static void* StatusHandlerContextPtr = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Reference for the registered control app.  Only one is allowed
 */
//--------------------------------------------------------------------------------------------------
static le_msg_SessionRef_t RegisteredControlAppRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for the block/unblock references
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t BlockRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Count of the number of allocated safe references from 'BlockRefMap' above.
 *
 * If there was a safeRef wrapper around le_hashmap_Size(), then this value would probably not be
 * needed, although we would then be dependent on the implementation of le_hashmap_Size() not
 * being overly complex.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t BlockRefCount=0;

//--------------------------------------------------------------------------------------------------
/**
 * Handler registered from avcServer_QueryInstall() to receive notification when app install is
 * allowed. Only one registered handler is allowed, and will be set to NULL after being called.
 */
//--------------------------------------------------------------------------------------------------
static avcServer_InstallHandlerFunc_t QueryInstallHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Timer used for deferring app install.
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t DeferTimer;


//--------------------------------------------------------------------------------------------------
// Local functions
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Handler to receive update status notifications from PA
 */
//--------------------------------------------------------------------------------------------------
static void UpdateHandler
(
    le_avc_Status_t updateStatus,
    le_avc_UpdateType_t updateType
)
{
    // Keep track of the state of any pending downloads or installs.
    switch ( updateStatus )
    {
        case LE_AVC_DOWNLOAD_PENDING:
            CurrentState = AVC_DOWNLOAD_PENDING;

            // Only store the new update type, when we get download pending.
            LE_DEBUG("Update type for DOWNLOAD is %d", updateType);
            CurrentUpdateType = updateType;
            break;

        case LE_AVC_INSTALL_PENDING:
            CurrentState = AVC_INSTALL_PENDING;

            // If the device resets during a FOTA download, then the CurrentUpdateType is lost
            // and needs to be assigned again.  Since we don't easily know if a reset happened,
            // always re-assign the value.
            LE_DEBUG("Update type for INSTALL is %d", updateType);
            CurrentUpdateType = updateType;
            break;

        case LE_AVC_DOWNLOAD_IN_PROGRESS:
        case LE_AVC_DOWNLOAD_COMPLETE:
        case LE_AVC_INSTALL_IN_PROGRESS:
            // These events do not cause a state transition
            break;

        case LE_AVC_NO_UPDATE:
        case LE_AVC_DOWNLOAD_FAILED:
        case LE_AVC_INSTALL_FAILED:
        case LE_AVC_INSTALL_COMPLETE:
            // There is no longer any current update, so go back to idle
            CurrentState = AVC_IDLE;
            break;
    }


    if ( StatusHandlerRef == NULL )
    {
        // There is no registered control app; automatically accept any pending downloads.
        if ( updateStatus == LE_AVC_DOWNLOAD_PENDING )
        {
            LE_INFO("Automatically accepting download");
            pa_avc_SendSelection(PA_AVC_ACCEPT, 0);
            CurrentState = AVC_DOWNLOAD_IN_PROGRESS;
        }

        // There is no registered control app; automatically accept any pending installs,
        // if there are no blocking apps, otherwise, defer the install.
        else if ( updateStatus == LE_AVC_INSTALL_PENDING )
        {
            if ( BlockRefCount == 0 )
            {
                LE_INFO("Automatically accepting install");
                pa_avc_SendSelection(PA_AVC_ACCEPT, 0);
                CurrentState = AVC_INSTALL_IN_PROGRESS;
            }
            else
            {
                LE_INFO("Automatically deferring install");
                pa_avc_SendSelection(PA_AVC_DEFER, BLOCKED_DEFER_TIME);

                // Since the decision is not to install at this time, go back to idle
                CurrentState = AVC_IDLE;
            }
        }

        // Otherwise, log a message
        else
        {
            LE_DEBUG("No handler registered to receive status %i", updateStatus);
        }
    }

    else
    {
        LE_DEBUG("Reporting status %d", updateStatus);
        StatusHandlerRef(updateStatus, StatusHandlerContextPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler to client session closes
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

    // Search for the block reference(s) used by the closed client, and clean up any data.
    le_ref_IterRef_t iterRef = le_ref_GetIterator(BlockRefMap);

    while ( le_ref_NextNode(iterRef) == LE_OK )
    {
        if ( le_ref_GetValue(iterRef) == sessionRef )
        {
            le_ref_DeleteRef( BlockRefMap, (void*)le_ref_GetSafeRef(iterRef) );
            BlockRefCount--;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Determine whether the current client is the registered control app client.
 *
 * As a side-effect, will kill the client if it is not the registered control app client.
 */
//--------------------------------------------------------------------------------------------------
static bool IsValidControlAppClient
(
    void
)
{
    if ( RegisteredControlAppRef == NULL ||
         RegisteredControlAppRef != le_avc_GetClientSessionRef() )
    {
        LE_KILL_CLIENT("Client is not registered as control app");
        return false;
    }
    else
        return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Query if it's okay to proceed with an application install
 *
 * @return
 *      - LE_OK if install can proceed right away
 *      - LE_BUSY if install is deferred
 */
//--------------------------------------------------------------------------------------------------
static le_result_t QueryInstall
(
    void
)
{
    le_result_t result = LE_BUSY;

    if ( StatusHandlerRef == NULL )
    {
        // There is no registered control app; automatically accept any pending installs,
        // if there are no blocking apps, otherwise, defer the install.
        if ( BlockRefCount == 0 )
        {
            LE_INFO("Automatically accepting install");
            CurrentState = AVC_INSTALL_IN_PROGRESS;
            result = LE_OK;
        }
        else
        {
            LE_INFO("Automatically deferring install");

            // Since the decision is not to install at this time, go back to idle
            CurrentState = AVC_IDLE;

            // Try the install later
            le_clk_Time_t interval = { .sec = BLOCKED_DEFER_TIME*60 };

            le_timer_SetInterval(DeferTimer, interval);
            le_timer_Start(DeferTimer);
        }
    }
    else
    {
        // Notify registered control app.
        LE_DEBUG("Reporting status LE_AVC_INSTALL_PENDING");
        CurrentState = AVC_INSTALL_PENDING;
        StatusHandlerRef(LE_AVC_INSTALL_PENDING, StatusHandlerContextPtr);
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Called when the defer timer expires.
 */
//--------------------------------------------------------------------------------------------------
void TimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< Timer that expired
)
{
    if ( QueryInstall() == LE_OK )
    {
        // Notify the registered handler to proceed with the install; only called once.
        QueryInstallHandlerRef();
        QueryInstallHandlerRef = NULL;
    }
}



//--------------------------------------------------------------------------------------------------
// Internal interface functions
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Query the AVC Server if it's okay to proceed with an application install
 *
 * If an install can't proceed right away, then the handlerRef function will be called when it is
 * okay to proceed with an install. Note that handlerRef will be called at most once.
 *
 * @return
 *      - LE_OK if install can proceed right away (handlerRef will not be called)
 *      - LE_BUSY if handlerRef will be called later to notify when install can proceed
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
le_result_t avcServer_QueryInstall
(
    avcServer_InstallHandlerFunc_t handlerRef  ///< [IN] Handler to receive install response.
)
{
    le_result_t result;

    if ( QueryInstallHandlerRef != NULL )
    {
        LE_ERROR("Duplicate install attempt");
        return LE_FAULT;
    }

    result = QueryInstall();

    // Store the handler to call later, once install is allowed.
    if ( result == LE_BUSY )
    {
        QueryInstallHandlerRef = handlerRef;
    }
    else
    {
        QueryInstallHandlerRef = NULL;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
// API functions
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * le_avc_StatusHandler handler ADD function
 */
//--------------------------------------------------------------------------------------------------
le_avc_StatusEventHandlerRef_t le_avc_AddStatusEventHandler
(
    le_avc_StatusHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    // handlerPtr must be valid
    if ( handlerPtr == NULL )
    {
        LE_KILL_CLIENT("Null handlerPtr");
    }

    // Only allow the handler to be registered, if nothing is currently registered. In this way,
    // only one user app is allowed to register at a time.
    if ( StatusHandlerRef == NULL )
    {
        StatusHandlerRef = handlerPtr;
        StatusHandlerContextPtr = contextPtr;

        // Store the client session ref, to ensure only the registered client can call the other
        // control related API functions.
        RegisteredControlAppRef = le_avc_GetClientSessionRef();

        // Register our local handler with the PA, and this handler will in turn call the user
        // specified handler. Although UpdateHandler is registered in the COMPONENT_INIT, we
        // re-register it here, to ensure callbacks will be performed in the event loop of the
        // current thread; we use the PA to keep track of this for us.
        pa_avc_SetAVMSMessageHandler(UpdateHandler);

        return REGISTERED_HANDLER_REF;
    }
    else
    {
        LE_ERROR("Handler already registered");
        return NULL;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * le_avc_StatusHandler handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void le_avc_RemoveStatusEventHandler
(
    le_avc_StatusEventHandlerRef_t addHandlerRef
        ///< [IN]
)
{
    if ( addHandlerRef != REGISTERED_HANDLER_REF )
    {
        LE_KILL_CLIENT("Invalid ref = %p", addHandlerRef);
    }

    if ( StatusHandlerRef == NULL )
    {
        LE_KILL_CLIENT("Handler not registered");
    }

    // Clear all info related to the registered handler.  Note that our local 'UpdateHandler'
    // must stay registered with the PA to ensure that automatic actions are performed, and
    // the state is properly tracked.
    StatusHandlerRef = NULL;
    StatusHandlerContextPtr = NULL;
    RegisteredControlAppRef = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Start a session with the AirVantage server
 *
 * This will also cause a query to be sent to the server, for pending updates.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avc_StartSession
(
    void
)
{
    if ( ! IsValidControlAppClient() )
        return LE_FAULT;

    return pa_avc_StartSession();
}


//--------------------------------------------------------------------------------------------------
/**
 * Stop a session with the AirVantage server
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avc_StopSession
(
    void
)
{
    if ( ! IsValidControlAppClient() )
        return LE_FAULT;

    return pa_avc_StopSession();
}


//--------------------------------------------------------------------------------------------------
/**
 * Accept the currently pending download
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avc_AcceptDownload
(
    void
)
{
    if ( ! IsValidControlAppClient() )
        return LE_FAULT;

    if ( CurrentState != AVC_DOWNLOAD_PENDING )
    {
        LE_ERROR("Expected AVC_DOWNLOAD_PENDING state; current state is %i", CurrentState);
        return LE_FAULT;
    }

    le_result_t result = pa_avc_SendSelection(PA_AVC_ACCEPT, 0);

    if ( result == LE_OK )
    {
        CurrentState = AVC_DOWNLOAD_IN_PROGRESS;
    }
    else
    {
        CurrentState = AVC_IDLE;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Defer the currently pending download, for the given number of minutes
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avc_DeferDownload
(
    uint32_t deferMinutes
        ///< [IN]
)
{
    if ( ! IsValidControlAppClient() )
        return LE_FAULT;

    if ( CurrentState != AVC_DOWNLOAD_PENDING )
    {
        LE_ERROR("Expected AVC_DOWNLOAD_PENDING state; current state is %i", CurrentState);
        return LE_FAULT;
    }

    // Since the decision is not to download at this time, go back to idle
    CurrentState = AVC_IDLE;

    return pa_avc_SendSelection(PA_AVC_DEFER, deferMinutes);
}


//--------------------------------------------------------------------------------------------------
/**
 * Reject the currently pending download
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avc_RejectDownload
(
    void
)
{
    if ( ! IsValidControlAppClient() )
        return LE_FAULT;

    if ( CurrentState != AVC_DOWNLOAD_PENDING )
    {
        LE_ERROR("Expected AVC_DOWNLOAD_PENDING state; current state is %i", CurrentState);
        return LE_FAULT;
    }

    // Since the decision is not to download, go back to idle
    CurrentState = AVC_IDLE;

    return pa_avc_SendSelection(PA_AVC_REJECT, 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Accept the currently pending firmware install
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AcceptInstallFirmware
(
    void
)
{
    le_result_t result;

    // If a user app is blocking the install, then just defer for some time.  Hopefully, the
    // next time this function is called, the user app will no longer be blocking the install.
    //
    // todo: If there is an app that periodically blocks updates, then we don't want to use a
    //       BLOCKED_DEFER_TIME that is related to the period of the blocking app, as this might
    //       mean that we never accept the install.  This needs to be revisited. and perhaps a
    //       simple random number generator can be used to give varying defer times, or a
    //       completely different mechanism needs to be used.
    if ( BlockRefCount > 0 )
    {
        // Since the decision is not to install at this time, go back to idle
        CurrentState = AVC_IDLE;

        // This will cause another LE_AVC_INSTALL_PENDING to be sent to the control app. The API
        // documentation does not explicitly describe this behaviour, but it is implied.
        // todo: It may be worth either revisiting the documentation or changing the behaviour, but
        //       leave this decision for later.
        result = pa_avc_SendSelection(PA_AVC_DEFER, BLOCKED_DEFER_TIME);
    }
    else
    {
        result = pa_avc_SendSelection(PA_AVC_ACCEPT, 0);

        if ( result == LE_OK )
        {
            CurrentState = AVC_INSTALL_IN_PROGRESS;
        }
        else
        {
            CurrentState = AVC_IDLE;
        }
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Accept the currently pending application install
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AcceptInstallApplication
(
    void
)
{
    // If a user app is blocking the install, then just defer for some time.  Hopefully, the
    // next time this function is called, the user app will no longer be blocking the install.
    if ( BlockRefCount > 0 )
    {
        // Since the decision is not to install at this time, go back to idle
        CurrentState = AVC_IDLE;

        // Try the install later
        le_clk_Time_t interval = { .sec = BLOCKED_DEFER_TIME*60 };

        le_timer_SetInterval(DeferTimer, interval);
        le_timer_Start(DeferTimer);
    }
    else
    {
        // Notify the registered handler to proceed with the install; only called once.
        CurrentState = AVC_INSTALL_IN_PROGRESS;
        QueryInstallHandlerRef();
        QueryInstallHandlerRef = NULL;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Accept the currently pending install
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avc_AcceptInstall
(
    void
)
{
    if ( ! IsValidControlAppClient() )
    {
        return LE_FAULT;
    }

    if ( CurrentState != AVC_INSTALL_PENDING )
    {
        LE_ERROR("Expected AVC_INSTALL_PENDING state; current state is %i", CurrentState);
        return LE_FAULT;
    }

    if ( CurrentUpdateType == LE_AVC_FIRMWARE_UPDATE )
    {
        return AcceptInstallFirmware();
    }
    else if ( CurrentUpdateType == LE_AVC_APPLICATION_UPDATE )
    {
        return AcceptInstallApplication();
    }
    else
    {
        LE_ERROR("Unknown update type");
        return LE_FAULT;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Defer the currently pending install
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avc_DeferInstall
(
    uint32_t deferMinutes
        ///< [IN]
)
{
    if ( ! IsValidControlAppClient() )
    {
        return LE_FAULT;
    }

    if ( CurrentState != AVC_INSTALL_PENDING )
    {
        LE_ERROR("Expected AVC_INSTALL_PENDING state; current state is %i", CurrentState);
        return LE_FAULT;
    }

    // Since the decision is not to install at this time, go back to idle
    CurrentState = AVC_IDLE;

    if ( CurrentUpdateType == LE_AVC_FIRMWARE_UPDATE )
    {
        return pa_avc_SendSelection(PA_AVC_DEFER, deferMinutes);
    }
    else if ( CurrentUpdateType == LE_AVC_APPLICATION_UPDATE )
    {
        // Try the install later
        le_clk_Time_t interval = { .sec = (deferMinutes*60) };

        le_timer_SetInterval(DeferTimer, interval);
        le_timer_Start(DeferTimer);

        return LE_OK;
    }
    else
    {
        LE_ERROR("Unknown update type");
        return LE_FAULT;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the update type of the currently pending update
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT if not available
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avc_GetUpdateType
(
    le_avc_UpdateType_t* updateTypePtr
        ///< [OUT]
)
{
    if ( ! IsValidControlAppClient() )
        return LE_FAULT;

    if ( CurrentState == AVC_IDLE )
    {
        LE_ERROR("In AVC_IDLE state; no update pending or in progress");
        return LE_FAULT;
    }

    *updateTypePtr = CurrentUpdateType;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the name for the currently pending application update
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT if not available, or is not APPL_UPDATE type
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_avc_GetAppUpdateName
(
    char* updateName,
        ///< [OUT]

    size_t updateNameNumElements
        ///< [IN]
)
{
    if ( ! IsValidControlAppClient() )
        return LE_FAULT;

    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Prevent any pending updates from being installed.
 *
 * @return
 *      - Reference for block update request (to be used later for unblocking updates)
 *      - NULL if the operation was not successful
 */
//--------------------------------------------------------------------------------------------------
le_avc_BlockRequestRef_t le_avc_BlockInstall
(
    void
)
{
    // Need to return a unique reference that will be used by Unblock. Use the client session ref
    // as the data, since we need to delete the ref when the client closes.
    le_avc_BlockRequestRef_t blockRef = le_ref_CreateRef(BlockRefMap,
                                                         le_avc_GetClientSessionRef());

    // Keep track of how many refs have been allocated
    BlockRefCount++;

    return blockRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Allow any pending updates to be installed
 */
//--------------------------------------------------------------------------------------------------
void le_avc_UnblockInstall
(
    le_avc_BlockRequestRef_t blockRef
        ///< [IN]
        ///< block request ref returned by le_avc_BlockInstall
)
{
    // Look up the reference.  If it is NULL, then the reference is not valid.
    // Otherwise, delete the reference and update the count.
    void* dataRef = le_ref_Lookup(BlockRefMap, blockRef);
    if ( dataRef == NULL )
    {
        LE_KILL_CLIENT("Invalid block request reference %p", blockRef);
    }
    else
    {
        LE_PRINT_VALUE("%p", blockRef);
        le_ref_DeleteRef(BlockRefMap, blockRef);
        BlockRefCount--;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialization function for AVC Daemon
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Register for status updates even if no user function is registered.  This is necessary
    // to ensure that automatic actions are performed, and the state is properly tracked.
    pa_avc_SetAVMSMessageHandler(UpdateHandler);

    // Create safe reference map for block references. The size of the map should be based on
    // the expected number of simultaneous block requests, so take a reasonable guess.
    BlockRefMap = le_ref_CreateMap("BlockRef", 5);

    // Add a handler for client session closes
    le_msg_AddServiceCloseHandler( le_avc_GetServiceRef(), ClientCloseSessionHandler, NULL );

    // Init shared timer for deferring app install
    DeferTimer = le_timer_Create("install defer timer");
    le_timer_SetHandler(DeferTimer, TimerExpiryHandler);

    // Initialize the sub-components
    assetData_Init();
    lwm2m_Init();
}

