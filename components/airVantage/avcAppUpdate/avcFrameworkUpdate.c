
//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Legato framework objects.
 *
 *    legato/0/0  -  Legato framework object, this object allows framework introspection and reset
 *                   over lwm2m.
 *
 *    legato/1/0  -  Framework update object.  This object is used to handle framework bundle
 *                   updates over lwm2m.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "../avcDaemon/assetData.h"
#include "../avcDaemon/avcServer.h"
#include "avcUpdateShared.h"
#include "avcUpdateShared.h"
#include "avcFrameworkUpdate.h"




//--------------------------------------------------------------------------------------------------
 /**
 *  Path to the file that stores the Legato version number string.
 */
//--------------------------------------------------------------------------------------------------
#define LEGATO_VERSION_FILE "/legato/systems/current/version"




//--------------------------------------------------------------------------------------------------
 /**
 *  Maximum number of installation attempts.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_INSTALL_COUNT 5




//--------------------------------------------------------------------------------------------------
/**
 *  Field Ids of the Legato application object.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LO0F_VERSION        = 0,  ///< Legato version field.
    LO0F_RESTART        = 1,  ///< Command to restart the Legato framework.
    LO0F_SYSTEM_INDEX   = 2,  ///< The current system index.
    LO0F_PREVIOUS_INDEX = 3   ///< The previous system index, -1 if there was no previous system.
}
LegatoObj0Fids;




//--------------------------------------------------------------------------------------------------
/**
 *  Field Ids of the Legato system update object.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LO1F_PACKAGE_URI              = 1,  ///< URI to use when downloading new system updates.
    LO1F_UPDATE                   = 2,  ///< Command to start a system update.
    LO1F_STATE                    = 3,  ///< The current state of the update object.
    LO1F_UPDATE_SUPPORTED_OBJECTS = 4,  ///< If true, send a firmware update registration when
                                        ///<   updated.
    LO1F_UPDATE_RESULT            = 5   ///< Updated with the result of the last command.
}
LegatoObj1Fids;




//--------------------------------------------------------------------------------------------------
/**
 *  Current state of the update in progress, (if any.)
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    US_IDLE        = 1,
    US_DOWNLOADING = 2,
    US_DOWNLOADED  = 3,
    US_UPDATING    = 4
}
UpdateState;




//--------------------------------------------------------------------------------------------------
/**
 *  The result of the last command.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    UR_DEFAULT                  = 0,
    UR_UPDATE_SUCCESS           = 1,
    UR_OUT_OF_STORAGE           = 2,
    UR_OUT_OF_MEMORY            = 3,
    UR_CONNECTION_LOST          = 4,
    UR_CHECKSUM_FAILED          = 5,
    UR_UNSUPPORTED_PACKAGE_TYPE = 6,
    UR_INVALID_URI              = 7,
    UR_UPDATE_FAILED            = 8
}
UpdateResult;




//--------------------------------------------------------------------------------------------------
/**
 *  Reference to the Legato object 0, framework status.
 */
//--------------------------------------------------------------------------------------------------
assetData_InstanceDataRef_t LegatoStatusObjectRef = NULL;




//--------------------------------------------------------------------------------------------------
/**
 *  Reference to the Legato object 1, framework update.
 */
//--------------------------------------------------------------------------------------------------
assetData_InstanceDataRef_t LegatoInstallObjectRef = NULL;




//--------------------------------------------------------------------------------------------------
/**
 *  Path in the configTree to the framework update status data.
 */
//--------------------------------------------------------------------------------------------------
static const char BaseConfigPath[] = "avcService:/frameworkUpdate";




//--------------------------------------------------------------------------------------------------
/**
 *  Name where the framework update state machine's current state is stored.
 */
//--------------------------------------------------------------------------------------------------
static const char StateValueName[] = "state";




//--------------------------------------------------------------------------------------------------
/**
 *  Name of the place the download uri is stored.
 */
//--------------------------------------------------------------------------------------------------
static const char UriValueName[] = "uri";




//--------------------------------------------------------------------------------------------------
/**
 *  Name of the saved index storage.
 */
//--------------------------------------------------------------------------------------------------
static const char SystemIndexName[] = "SavedSystemIndex";




//--------------------------------------------------------------------------------------------------
/**
 *  Name of the install count storage.
 */
//--------------------------------------------------------------------------------------------------
static const char SystemInstallCount[] = "SavedInstallCount";




//--------------------------------------------------------------------------------------------------
/**
 *  Enumeration used to keep track of the framework update state machine.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    IS_IDLE               = 0,
    IS_DOWNLOAD_REQUESTED = 1,
    IS_DOWNLOAD_BAD_URI   = 2,
    IS_DOWNLOAD_FAILED    = 3,
    IS_DOWNLOAD_SUCCEEDED = 4,
    IS_UPDATE_REQUESTED   = 5,
    IS_UPDATE_BAD_PACKAGE = 6,
    IS_UPDATE_STARTED     = 7,
    IS_UPDATE_SUCCEEDED   = 8,
    IS_UPDATE_FAILED      = 9
}
InstallState;




//--------------------------------------------------------------------------------------------------
/**
 *  Convert an update state to a string for reporting in the device log.
 */
//--------------------------------------------------------------------------------------------------
static char* UpdateStateToStr
(
    UpdateState state  ///< The update state to convert.
)
//--------------------------------------------------------------------------------------------------
{
    char* result;

    switch (state)
    {
        case US_IDLE:        result = "US_IDLE";        break;
        case US_DOWNLOADING: result = "US_DOWNLOADING"; break;
        case US_DOWNLOADED:  result = "US_DOWNLOADED";  break;
        case US_UPDATING:    result = "US_UPDATING";    break;
        default:             result = "Unknown";        break;
    }

    return result;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Convert an update result to a string for reporting in the device log.
 */
//--------------------------------------------------------------------------------------------------
static char* UpdateResultToStr
(
    UpdateResult updateResult  ///< The update result to convert.
)
//--------------------------------------------------------------------------------------------------
{
    char* result;

    switch (updateResult)
    {
        case UR_DEFAULT:                  result = "UR_DEFAULT";                  break;
        case UR_UPDATE_SUCCESS:           result = "UR_UPDATE_SUCCESS";           break;
        case UR_OUT_OF_STORAGE:           result = "UR_OUT_OF_STORAGE";           break;
        case UR_OUT_OF_MEMORY:            result = "UR_OUT_OF_MEMORY";            break;
        case UR_CONNECTION_LOST:          result = "UR_CONNECTION_LOST";          break;
        case UR_CHECKSUM_FAILED:          result = "UR_CHECKSUM_FAILED";          break;
        case UR_UNSUPPORTED_PACKAGE_TYPE: result = "UR_UNSUPPORTED_PACKAGE_TYPE"; break;
        case UR_INVALID_URI:              result = "UR_INVALID_URI";              break;
        default:                          result = "Unknown";                     break;
    }

    return result;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Set the state/result fields of the install object.
 */
//--------------------------------------------------------------------------------------------------
static void SetInstallObjState_
(
    UpdateState state,            ///< New value for the object state.
    UpdateResult result,          ///< New value for the last result.
    const char* functionNamePtr,  ///< Name of the function that called this one.
    size_t line                   ///< Line of  source code this was called from.
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("### <%s: %zu>: Set object Legato/1 state/result: (%d) %s / (%d) %s",
             functionNamePtr,
             line,
             state,
             UpdateStateToStr(state),
             result,
             UpdateResultToStr(result));

    LE_ASSERT(assetData_client_SetInt(LegatoInstallObjectRef, LO1F_STATE, state) == LE_OK);
    LE_ASSERT(assetData_client_SetInt(LegatoInstallObjectRef, LO1F_UPDATE_RESULT, result) == LE_OK);

    assetData_RegUpdateIfNotObserved(LegatoInstallObjectRef, ASSET_DATA_SESSION_STATUS_IGNORE);
}

#define SetInstallObjState(state, result) SetInstallObjState_(state, result, __FUNCTION__, __LINE__)




//--------------------------------------------------------------------------------------------------
/**
 *  Get the value of the state field of the install object.
 */
//--------------------------------------------------------------------------------------------------
static UpdateState GetInstallObjState
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    int state;

    LE_ASSERT(assetData_client_GetInt(LegatoInstallObjectRef, LO1F_STATE, &state) == LE_OK);
    return (UpdateState)state;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Record the current state of the state machine, in case we are rebooted mid-update.
 */
//--------------------------------------------------------------------------------------------------
static void SaveInstallState
(
    InstallState newState,  ///< The state to save.
    const char* uriPtr,     ///< Optional, the URI we're downloading from.
    int32_t systemIndex,    ///< Optional, use -1 to ignore. Used to save the index of the current
                            ///<   system.
    int32_t installCount    ///< Optional, use -1 to ignore. Used to save the number installs
)
//--------------------------------------------------------------------------------------------------
{
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(BaseConfigPath);

    le_cfg_SetInt(iterRef, StateValueName, newState);

    if (uriPtr != NULL)
    {
        le_cfg_SetString(iterRef, UriValueName, uriPtr);
    }
    else if (le_cfg_NodeExists(iterRef, UriValueName))
    {
        le_cfg_DeleteNode(iterRef, UriValueName);
    }

    if (systemIndex != -1)
    {
        le_cfg_SetInt(iterRef, SystemIndexName, systemIndex);
    }
    else if (le_cfg_NodeExists(iterRef, SystemIndexName))
    {
        le_cfg_DeleteNode(iterRef, SystemIndexName);
    }

    if (installCount != -1)
    {
        le_cfg_SetInt(iterRef, SystemInstallCount, installCount);
    }
    else if (le_cfg_NodeExists(iterRef, SystemInstallCount))
    {
        le_cfg_DeleteNode(iterRef, SystemInstallCount);
    }

    le_cfg_CommitTxn(iterRef);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Load the stored system index value from the config tree.
 */
//--------------------------------------------------------------------------------------------------
static int32_t RestorePreviousSystemInfo
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(BaseConfigPath);

    int32_t index = le_cfg_GetInt(iterRef, SystemIndexName, -1);
    le_cfg_CommitTxn(iterRef);

    return index;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Get the number of installation attempts from the config tree.
 */
//--------------------------------------------------------------------------------------------------
static int32_t RestoreInstallCount
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(BaseConfigPath);

    int32_t index = le_cfg_GetInt(iterRef, SystemInstallCount, 0);
    le_cfg_CommitTxn(iterRef);

    return index;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Download status update handler.
 */
//--------------------------------------------------------------------------------------------------
static void OnUriDownloadUpdate
(
    le_avc_Status_t updateStatus   ///< Status of the download in question.
)
//--------------------------------------------------------------------------------------------------
{
//    le_cfg_IteratorRef_t iterRef;

    switch (updateStatus)
    {
        case LE_AVC_DOWNLOAD_COMPLETE:
            LE_DEBUG("Download complete.");
            SetInstallObjState(US_DOWNLOADED, UR_DEFAULT);
            SaveInstallState(IS_DOWNLOAD_SUCCEEDED, NULL, -1, -1);
            break;

        case LE_AVC_DOWNLOAD_FAILED:
            LE_DEBUG("Download failed.");
            // TODO: Find out the real reason this failed.
            SetInstallObjState(US_DOWNLOADED, UR_CHECKSUM_FAILED);
            SaveInstallState(IS_DOWNLOAD_FAILED, NULL, -1, -1);
            break;

        case LE_AVC_DOWNLOAD_PENDING:
            LE_DEBUG("Download pending.");
            break;

        case LE_AVC_DOWNLOAD_IN_PROGRESS:
            if (GetInstallObjState() == US_IDLE)
            {
                LE_DEBUG("Download started.");
                SetInstallObjState(US_DOWNLOADING, UR_DEFAULT);
            }
            else
            {
                LE_DEBUG("Download in progress.");
            }
            break;

        default:
            LE_ERROR("Received unexpected updateStatus %i", updateStatus);
            break;
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Called to attempt to start an install.
 */
//--------------------------------------------------------------------------------------------------
static void StartInstall
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("Install system update from SWI FOTA.");

    int firmwareFd;
    le_result_t result = pa_avc_ReadImage(&firmwareFd);

    if (result == LE_OK)
    {
        if (le_update_Start(firmwareFd) != LE_OK)
        {
            LE_ERROR("Could not start update.");

            SetInstallObjState(US_IDLE, UR_UNSUPPORTED_PACKAGE_TYPE);
            SaveInstallState(IS_UPDATE_BAD_PACKAGE, NULL, -1, -1);
        }
        else
        {
            int32_t currentInstallCount = RestoreInstallCount();
            SetInstallObjState(US_UPDATING, UR_DEFAULT);
            SaveInstallState(IS_UPDATE_REQUESTED, NULL, le_update_GetCurrentSysIndex(), ++currentInstallCount);
        }
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Query to see if we can initiate an update.  If not, the queue the request for later, otherwise
 *  kick off the update now.
 */
//--------------------------------------------------------------------------------------------------
static void RequestInstall
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result = avcServer_QueryInstall(StartInstall);

    LE_FATAL_IF(result == LE_FAULT,
                "Unexpected error in query install: %s",
                LE_RESULT_TXT(result));

    if (result != LE_BUSY)
    {
        StartInstall();
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Called to start a download.
 */
//--------------------------------------------------------------------------------------------------
static void RequestDownload
(
    const char* uri  ///< The uri to download from.
)
//--------------------------------------------------------------------------------------------------
{
    SetInstallObjState(US_IDLE, UR_DEFAULT);

    if (pa_avc_StartURIDownload(uri, OnUriDownloadUpdate) == LE_OK)
    {
        LE_DEBUG("Download request successful.");
        SaveInstallState(IS_DOWNLOAD_REQUESTED, uri, -1, -1);
    }
    else
    {
        LE_ERROR("Download request failed.");
        SetInstallObjState(US_IDLE, UR_INVALID_URI);
        SaveInstallState(IS_DOWNLOAD_BAD_URI, NULL, -1, -1);
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Attempt to read the Legato version string from the file system.
 */
//--------------------------------------------------------------------------------------------------
static void LoadLegatoVersionStr
(
    char versionBuffer[MAX_VERSION_STR_BYTES]  ///< Buffer to hold the string.
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("Read the Legato version string.");

    FILE* versionFile = NULL;

    do
    {
        versionFile = fopen(LEGATO_VERSION_FILE, "r");
    }
    while (   (versionFile != NULL)
           && (errno == EINTR));

    if (versionFile == NULL)
    {
        LE_ERROR("Could not open Legato version file.");
        return;
    }

    if (fgets(versionBuffer, MAX_VERSION_STR_BYTES, versionFile) != NULL)
    {
        char* newLine = strchr(versionBuffer, '\n');

        if (newLine != NULL)
        {
            *newLine = 0;
        }

        LE_DEBUG("The current Legato framework version is, '%s'.", versionBuffer);
    }
    else
    {
        LE_ERROR("Could not read Legato version.");
    }

    fclose(versionFile);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Called when "interesting" activity happens on fields of the Legato framework object.
 */
//--------------------------------------------------------------------------------------------------
static void LegatoStatusObjectHandler
(
    assetData_InstanceDataRef_t instanceRef,  ///< The instance of the Legato object.
    int fieldId,                              ///< The field that updated.
    assetData_ActionTypes_t action,           ///< The action performed.
    void* contextPtr                          ///< Useful context.
)
//--------------------------------------------------------------------------------------------------
{
    switch (fieldId)
    {
        // case LO0F_VERSION:

        case LO0F_RESTART:
            if (action == ASSET_DATA_ACTION_EXEC)
            {
                LE_WARN("Legato OTA restart requested.");

                if (le_framework_Restart(false) != LE_OK)
                {
                    LE_WARN("Legato restart request rejected.  Shutdown must be underway already.");
                }
            }
            break;

        // LO0F_STATUS
        // LO0F_HASH
        // LO0F_RESET_CAUSE

        default:
            LE_ERROR("Legato/0: Unexpected request id: %d on field id %d received.",
                     action,
                     fieldId);
            break;
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Event handler for the Legato object 1, the bundle/framework install object.
 */
//--------------------------------------------------------------------------------------------------
static void LegatoInstallObjectFieldHandler
(
    assetData_InstanceDataRef_t instanceRef,  ///< The instance of the Legato  object.
    int fieldId,                              ///< The field that updated.
    assetData_ActionTypes_t action,           ///< The action performed.
    void* contextPtr                          ///< Useful context.
)
//--------------------------------------------------------------------------------------------------
{
    switch (fieldId)
    {
        case LO1F_PACKAGE_URI:
            LE_DEBUG("LO1F_PACKAGE_URI");
            if (action == ASSET_DATA_ACTION_WRITE)
            {
                char uri[MAX_URI_STR_BYTES] = "";

                assetData_client_GetString(instanceRef, LO1F_PACKAGE_URI, uri, sizeof(uri));
                LE_DEBUG("Attempt to download from Url: %s", uri);

                RequestDownload(uri);
            }
            break;

        case LO1F_UPDATE:
            LE_DEBUG("LO1F_UPDATE");
            if (action == ASSET_DATA_ACTION_EXEC)
            {
                SaveInstallState(IS_UPDATE_REQUESTED, NULL, -1, -1);
                RequestInstall();
            }
            break;

        // LO1F_STATE
        // LO1F_UPDATE_SUPPORTED_OBJECTS
        // LO1F_UPDATE_RESULT

        default:
            LE_ERROR("Legato/1: Unexpected request id: %d on field id %d received.",
                     action,
                     fieldId);
            break;
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Create an object instance and register field handlers for it.
 */
//--------------------------------------------------------------------------------------------------
static assetData_InstanceDataRef_t CreateObjectInstance
(
    const int monitorFields[],                           ///< The fields we're interested in.
    size_t monFieldCount,                                ///< Count of fields to monitor.
    assetData_FieldActionHandlerFunc_t fieldHandlerPtr,  ///< Function to be called
    int objectId                                         ///<
)
//--------------------------------------------------------------------------------------------------
{
    assetData_InstanceDataRef_t newObjRef = NULL;

    aus_RegisterFieldEventHandlers(ASSET_DATA_LEGATO_OBJ_NAME,
                                   objectId,
                                   NULL,
                                   monitorFields,
                                   monFieldCount,
                                   fieldHandlerPtr);

    LE_ASSERT(assetData_CreateInstanceById(ASSET_DATA_LEGATO_OBJ_NAME,
                                           objectId,
                                           0,
                                           &newObjRef) == LE_OK);

    return newObjRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Query the config tree and check to see if the process was restarted in the middle of an update.
 *  If it was, restore our state from the saved configuration and resume where we left off.
 */
//--------------------------------------------------------------------------------------------------
static void RestoreInstallState
(
    void
)
//--------------------------------------------------------------------------------------------------
{


    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(BaseConfigPath);

    InstallState lastState = (InstallState)le_cfg_GetInt(iterRef, StateValueName, IS_IDLE);
    char uri[MAX_URI_STR_BYTES] = "";

    if (   (le_cfg_NodeExists(iterRef, UriValueName))
        && (le_cfg_GetString(iterRef, UriValueName, uri, sizeof(uri), "") == LE_OVERFLOW))
    {
        LE_ERROR("Bad download URI stored in framework update config.");
        lastState = IS_IDLE;
    }

    le_cfg_CommitTxn(iterRef);

    // Force the type of the install to framework.
    avcServer_SetUpdateType(LE_AVC_FRAMEWORK_UPDATE);

    switch (lastState)
    {
        case IS_IDLE:
            SetInstallObjState(US_IDLE, UR_INVALID_URI);
            break;

        case IS_DOWNLOAD_REQUESTED:
            SetInstallObjState(US_IDLE, UR_DEFAULT);

            // The firmware should be resuming the download.  If the handler registration fails,
            // attempt to restart the download.
            if (pa_avc_AddURIDownloadStatusHandler(OnUriDownloadUpdate) != LE_OK)
            {
                RequestDownload(uri);
            }
            break;

        case IS_DOWNLOAD_BAD_URI:
            SetInstallObjState(US_IDLE, UR_INVALID_URI);
            break;

        case IS_DOWNLOAD_FAILED:
            SetInstallObjState(US_DOWNLOADED, UR_CHECKSUM_FAILED);
            break;

        case IS_DOWNLOAD_SUCCEEDED:
            SetInstallObjState(US_DOWNLOADED, UR_DEFAULT);
            break;

        case IS_UPDATE_REQUESTED:
        case IS_UPDATE_STARTED:
            {
                int32_t recordedIndex = RestorePreviousSystemInfo();
                int32_t currentIndex = le_update_GetCurrentSysIndex();

                // If install has never been started
                if (recordedIndex == -1)
                {
                    RequestInstall();
                }
                else
                {
                    if (recordedIndex >= currentIndex)
                    {
                        int currentInstallCount = RestoreInstallCount();

                        if (currentInstallCount < MAX_INSTALL_COUNT)
                        {
                            // It looks like the install could have been interrupted, try installing again.
                            RequestInstall();
                        }
                        else
                        {
                            // If 5 install attempts have been made, mark system update as failed.
                            SaveInstallState(IS_UPDATE_FAILED, NULL, -1, -1);
                            SetInstallObjState(US_IDLE, UR_UPDATE_FAILED);
                        }
                    }
                    else
                    {
                        // Looks like the install was successful.
                        SaveInstallState(IS_UPDATE_SUCCEEDED, NULL, -1, -1);
                        SetInstallObjState(US_IDLE, UR_UPDATE_SUCCESS);
                    }
                }
            }
            break;

        case IS_UPDATE_BAD_PACKAGE:
            SetInstallObjState(US_DOWNLOADED, UR_CHECKSUM_FAILED);
            break;

        case IS_UPDATE_SUCCEEDED:
            SetInstallObjState(US_IDLE, UR_UPDATE_SUCCESS);
            break;

        case IS_UPDATE_FAILED:
            SetInstallObjState(US_IDLE, UR_UPDATE_FAILED);
            break;
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Create the framework object instances.
 */
//--------------------------------------------------------------------------------------------------
void InitLegatoObjects
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    static int legatoStatusObjMonitorFields[] =
        {
            // LO0F_VERSION,
            LO0F_RESTART,
            // LO0F_SYSTEM_INDEX,
            // LO0F_PREVIOUS_INDEX
        };

    static int legatoInstallObj1MonitorFields[] =
        {
            LO1F_PACKAGE_URI,
            LO1F_UPDATE,
            // LO1F_STATE,
            // LO1F_UPDATE_SUPPORTED_OBJECTS,
            // LO1F_UPDATE_RESULT
        };

    LegatoStatusObjectRef = CreateObjectInstance(legatoStatusObjMonitorFields,
                                                 NUM_ARRAY_MEMBERS(legatoStatusObjMonitorFields),
                                                 LegatoStatusObjectHandler,
                                                 0);

    LegatoInstallObjectRef = CreateObjectInstance(legatoInstallObj1MonitorFields,
                                                  NUM_ARRAY_MEMBERS(legatoInstallObj1MonitorFields),
                                                  LegatoInstallObjectFieldHandler,
                                                  1);

    UpdateLegatoObject();
    RestoreInstallState();
}




//--------------------------------------------------------------------------------------------------
/**
 *  Update Legato object; specifically the version.
 */
//--------------------------------------------------------------------------------------------------
void UpdateLegatoObject
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    char versionBuffer[MAX_VERSION_STR_BYTES] = "";

    LE_ASSERT(assetData_GetInstanceRefById(ASSET_DATA_LEGATO_OBJ_NAME,
                                           0,
                                           0,
                                           &LegatoStatusObjectRef) == LE_OK);


    LoadLegatoVersionStr(versionBuffer);

    le_result_t result = assetData_client_SetString(LegatoStatusObjectRef,
                                                    LO0F_VERSION,
                                                    versionBuffer);

    LE_ERROR_IF(result != LE_OK,
                "Could not update Legato version field (%d): %s",
                result,
                LE_RESULT_TXT(result));

    int32_t index = le_update_GetCurrentSysIndex();
    LE_ASSERT(assetData_client_SetInt(LegatoStatusObjectRef, LO0F_SYSTEM_INDEX, index) == LE_OK);

    index = le_update_GetPreviousSystemIndex(index);
    LE_ASSERT(assetData_client_SetInt(LegatoStatusObjectRef, LO0F_PREVIOUS_INDEX, index) == LE_OK);
}
