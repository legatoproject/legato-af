
//--------------------------------------------------------------------------------------------------
/**
 *  This component handles managing application update over LWM2M as well as the Legato application
 *  objects.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "appCfg.h"
#include "../avcDaemon/assetData.h"
#include "../avcDaemon/avcServer.h"
#include "pa_avc.h"
#include "exec.h"




//--------------------------------------------------------------------------------------------------
/**
 *  Name of the standard objects in LW M2M.
 */
//--------------------------------------------------------------------------------------------------
#define LWM2M_NAME "lwm2m"




//--------------------------------------------------------------------------------------------------
/**
 *  Name of this service.
 */
//--------------------------------------------------------------------------------------------------
#define AVC_SERVICE_NAME "avcService"




//--------------------------------------------------------------------------------------------------
/**
 *  String to return when an application does not include it's own version string.
 */
//--------------------------------------------------------------------------------------------------
#define VERSION_UNKNOWN "unknown"




//--------------------------------------------------------------------------------------------------
/**
 *  Maximum allowed size for application name strings.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_APP_NAME        LE_LIMIT_APP_NAME_LEN
#define MAX_APP_NAME_BYTES  (MAX_APP_NAME + 1)




//--------------------------------------------------------------------------------------------------
/**
 *  Maximum allowed size for application process name strings.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_PROC_NAME        LE_LIMIT_PROC_NAME_LEN
#define MAX_PROC_NAME_BYTES  (MAX_PROC_NAME + 1)




//--------------------------------------------------------------------------------------------------
/**
 *  Maximum allowed size for URI strings.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_URI_STR        255
#define MAX_URI_STR_BYTES  (MAX_URI_STR + 1)




//--------------------------------------------------------------------------------------------------
/**
 *  Maximum allowed size for for a Legato framework version string.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_VERSION_STR 100
#define MAX_VERSION_STR_BYTES (MAX_VERSION_STR + 1)




//--------------------------------------------------------------------------------------------------
/**
 *  Base path for an Object 9 application binding inside of the configTree.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_OBJECT_INFO_PATH "system:/lwm2m/objectMap"




//--------------------------------------------------------------------------------------------------
/**
 *  Path to the file that stores the Legato version number string.
 */
//--------------------------------------------------------------------------------------------------
#define LEGATO_VERSION_FILE "/opt/legato/version"




//--------------------------------------------------------------------------------------------------
/**
 *  Used to keep track of the object 9 status state machine.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    US_INITIAL          = 1,  ///< The object has no application associated with it.
    US_DOWNLOAD_STARTED = 2,  ///< An application download has been started.
    US_DOWNLOADED       = 3,  ///< The application has been downloaded, but need to be checked for
                              ///<   validity.
    US_DELIVERED        = 4,  ///< The application has passed validity checks and is now ready to be
                              ///<   installed.
    US_INSTALLED        = 5   ///< The application has been installed.
}
UpdateState;




//--------------------------------------------------------------------------------------------------
/**
 *  Enumeration to track the LWM2M object 9 update result field.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    UR_INITIAL_VALUE        = 0,  ///< The object in it's default state after creation.
    UR_DOWNLOADING          = 1,  ///< An application is currently downloading.
    UR_INSTALLED            = 2,  ///< An application has been successfully installed.
    UR_OUT_OF_STORAGE       = 3,  ///< Application failed to install due to storage issues.
    UR_OUT_OF_MEMORY        = 4,  ///< Application failed to install due to memory issues.
    UR_CONNECTION_LOST      = 5,  ///< Connection was lost during application download.
    UR_BAD_CHECKSUM         = 6,  ///< Application failed verification.
    UR_UNKNOWN_PACKAGE      = 7,  ///< Unknown type of application package downloaded.
    UR_INVALID_URI          = 8,  ///< Could not access application download URI.
    UR_DEVICE_UPDATE_ERROR  = 9,  ///< Device failure during application download.
    UR_INSTALLATION_FAILURE = 10  ///< Device failure during application installation.
}
UpdateResult;




//--------------------------------------------------------------------------------------------------
/**
 *  Indices for all of the fields of object 9.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    O9F_PKG_NAME                 = 0,   ///< Application name.
    O9F_PKG_VERSION              = 1,   ///< Application version.
    O9F_PACKAGE                  = 2,   ///< <Not supported>
    O9F_PACKAGE_URI              = 3,   ///< Uri for downloading a new application.
    O9F_INSTALL                  = 4,   ///< Command to start an install operation.
    O9F_CHECKPOINT               = 5,   ///< <Not supported>
    O9F_UNINSTALL                = 6,   ///< Command to remove an application.
    O9F_UPDATE_STATE             = 7,   ///< The install state of the application.
    O9F_UPDATE_SUPPORTED_OBJECTS = 8,   ///<
    O9F_UPDATE_RESULT            = 9,   ///< The result of the last install request.
    O9F_ACTIVATE                 = 10,  ///< Command to start the application.
    O9F_DEACTIVATE               = 11,  ///< Command to stop the application.
    O9F_ACTIVATION_STATE         = 12,  ///< Report if the application is running.
    O9F_PACKAGE_SETTINGS         = 13   ///< <Not supported>
}
LwObj9Fids;




//--------------------------------------------------------------------------------------------------
/**
 *  Field Ids of the Legato application object.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LO0F_VERSION = 0,  ///< Version field for the framework.
    LO0F_RESTART = 1   ///< Executable field to restart the framework.
}
LegatoObj0Fids;




//--------------------------------------------------------------------------------------------------
/**
 *  Fields for the application object..
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    AO0F_VERSION    = 0,  ///< Version of the application.
    AO0F_NAME       = 1,  ///< Name of the application.
    AO0F_STATE      = 2,  ///< Legato state for the application.
    AO0F_START_MODE = 3   ///< Start mode field.
}
AppObj0Fids;




//--------------------------------------------------------------------------------------------------
/**
 *  Fields for the application process object.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LO1F_NAME         = 0,  ///<
    LO1F_EXEC_NAME    = 1,  ///<
    LO1F_STATE        = 2,  ///<
    LO1F_FAULT_ACTION = 3   ///<
}
AppObj1Fids;




//--------------------------------------------------------------------------------------------------
/**
 *  The current instance of object 9 that is being downloaded to.  NULL if no downloads or
 *  installations are taking place.
 */
//--------------------------------------------------------------------------------------------------
static assetData_InstanceDataRef_t CurrentObj9 = NULL;




//--------------------------------------------------------------------------------------------------
/**
 *  Was the uninstall being handled initiated locally, or remotely?
 */
//--------------------------------------------------------------------------------------------------
static bool IsLocalUninstall = false;



//--------------------------------------------------------------------------------------------------
/**
 *  Convert an UpdateState value to a string for debugging.
 *
 *  @return String version of the supplied enumeration value.
 */
//--------------------------------------------------------------------------------------------------
static const char* UpdateStateToStr
(
    UpdateState state  ///< The enumeration value to convert.
)
//--------------------------------------------------------------------------------------------------
{
    char* result;

    switch (state)
    {
        case US_INITIAL:           result = "US_INITIAL";          break;
        case US_DOWNLOAD_STARTED:  result = "US_DOWNLOAD_STARTED"; break;
        case US_DOWNLOADED:        result = "US_DOWNLOADED";       break;
        case US_DELIVERED:         result = "US_DELIVERED";        break;
        case US_INSTALLED:         result = "US_INSTALLED";        break;
        default:                   result = "Unknown";             break;
    }

    return result;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Convert an UpdateResult value to a string for debugging.
 *
 *  @return String version of the supplied enumeration value.
 */
//--------------------------------------------------------------------------------------------------
static const char* UpdateResultToStr
(
    UpdateResult state  ///< The enumeration value to convert.
)
//--------------------------------------------------------------------------------------------------
{
    char* result;

    switch (state)
    {
        case UR_INITIAL_VALUE:        result = "UR_INITIAL_VALUE";        break;
        case UR_DOWNLOADING:          result = "UR_DOWNLOADING";          break;
        case UR_INSTALLED:            result = "UR_INSTALLED";            break;
        case UR_OUT_OF_STORAGE:       result = "UR_OUT_OF_STORAGE";       break;
        case UR_OUT_OF_MEMORY:        result = "UR_OUT_OF_MEMORY";        break;
        case UR_CONNECTION_LOST:      result = "UR_CONNECTION_LOST";      break;
        case UR_BAD_CHECKSUM:         result = "UR_BAD_CHECKSUM";         break;
        case UR_UNKNOWN_PACKAGE:      result = "UR_UNKNOWN_PACKAGE";      break;
        case UR_INVALID_URI:          result = "UR_INVALID_URI";          break;
        case UR_DEVICE_UPDATE_ERROR:  result = "UR_DEVICE_UPDATE_ERROR";  break;
        case UR_INSTALLATION_FAILURE: result = "UR_INSTALLATION_FAILURE"; break;
        default:                      result = "Unknown";                 break;
    }

    return result;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Convert an object 9 field index to a string for debugging.
 *
 *  @return String version of the supplied enumeration value.
 */
//--------------------------------------------------------------------------------------------------
static const char* Obj9FieldToStr
(
    LwObj9Fids fieldId  ///< The enumeration value to convert.
)
//--------------------------------------------------------------------------------------------------
{
    char* result;

    switch (fieldId)
    {
        case O9F_PKG_NAME:                 result = "O9F_PKG_NAME";                 break;
        case O9F_PKG_VERSION:              result = "O9F_PKG_VERSION";              break;
        case O9F_PACKAGE:                  result = "O9F_PACKAGE";                  break;
        case O9F_PACKAGE_URI:              result = "O9F_PACKAGE_URI";              break;
        case O9F_INSTALL:                  result = "O9F_INSTALL";                  break;
        case O9F_CHECKPOINT:               result = "O9F_CHECKPOINT";               break;
        case O9F_UNINSTALL:                result = "O9F_UNINSTALL";                break;
        case O9F_UPDATE_STATE:             result = "O9F_UPDATE_STATE";             break;
        case O9F_UPDATE_SUPPORTED_OBJECTS: result = "O9F_UPDATE_SUPPORTED_OBJECTS"; break;
        case O9F_UPDATE_RESULT:            result = "O9F_UPDATE_RESULT";            break;
        case O9F_ACTIVATE:                 result = "O9F_ACTIVATE";                 break;
        case O9F_DEACTIVATE:               result = "O9F_DEACTIVATE";               break;
        case O9F_ACTIVATION_STATE:         result = "O9F_ACTIVATION_STATE";         break;
        case O9F_PACKAGE_SETTINGS:         result = "O9F_PACKAGE_SETTINGS";         break;
        default:                           result = "Unknown";                      break;
    }

    return result;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Convert an LWM2M action ID to a string for debugging.
 *
 *  @return String version of the supplied enumeration value.
 */
//--------------------------------------------------------------------------------------------------
static const char* ActionToStr
(
    assetData_ActionTypes_t action  ///< The enumeration value to convert.
)
//--------------------------------------------------------------------------------------------------
{
    char* result;

    switch (action)
    {
        case ASSET_DATA_ACTION_CREATE: result = "ASSET_DATA_ACTION_CREATE"; break;
        case ASSET_DATA_ACTION_DELETE: result = "ASSET_DATA_ACTION_DELETE"; break;
        case ASSET_DATA_ACTION_READ:   result = "ASSET_DATA_ACTION_READ";   break;
        case ASSET_DATA_ACTION_WRITE:  result = "ASSET_DATA_ACTION_WRITE";  break;
        case ASSET_DATA_ACTION_EXEC:   result = "ASSET_DATA_ACTION_EXEC";   break;
        default:                       result = "Unknown";                  break;
    }

    return result;
}




//--------------------------------------------------------------------------------------------------
/**
 *  If a given app is in the "disapproved" list, is is not exposed through LWM2M.
 *
 *  @return True if the app is hidden from lwm2m, false if not.
 */
//--------------------------------------------------------------------------------------------------
static bool IsHiddenApp
(
    const char* appName  ///< Name of the application to check.
)
//--------------------------------------------------------------------------------------------------
{
    if (le_cfg_QuickGetBool("/lwm2m/hideDefaultApps", true) == true)
    {
        static char* appList[] =
            {
                "airvantage",
                "audioService",
                "avcService",
                "cellNetService",
                "dataConnectionService",
                "modemService",
                "positioningService",
                "powerMgr",
                "secStore",
                "voiceCallService",
                "fwupdateService",
                "smsInboxService"
            };

        for (size_t i = 0; i < NUM_ARRAY_MEMBERS(appList); i++)
        {
            if (strcmp(appList[i], appName) == 0)
            {
                return true;
            }
        }
    }

    return false;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Attempt to read the Legato version string from the file system.
 */
//--------------------------------------------------------------------------------------------------
static void GetLegatoVersionStr
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

    int retVal = -1;

    do
    {
        retVal = fclose(versionFile);
    }
    while (   (retVal == -1)
           && (errno == EINTR));
}




//--------------------------------------------------------------------------------------------------
/**
 *  Called to register lwm2m object and field event handlers.
 */
//--------------------------------------------------------------------------------------------------
static void RegisterFieldEventHandlers
(
    const char* appNamePtr,                              ///< Namespace for the object.
    int objectId,                                        ///< The Id of the object.
    assetData_AssetActionHandlerFunc_t assetHandlerPtr,  ///< Handler for object level events.
    int* monitorFields,                                  ///< List of fields to monitor.
    size_t monFieldCount,                                ///< Size of the field list.
    assetData_FieldActionHandlerFunc_t fieldHandlerPtr   ///< Handler to be called for field
                                                         ///<   activity.
)
//--------------------------------------------------------------------------------------------------
{
    assetData_AssetDataRef_t assetRef = NULL;

    LE_DEBUG("Registering on %s/%d.", appNamePtr, objectId);

    LE_FATAL_IF(assetData_GetAssetRefById(appNamePtr, objectId, &assetRef) != LE_OK,
                "Could not reference object %s/%d data.",
                appNamePtr,
                objectId);

    if (assetHandlerPtr != NULL)
    {
        LE_DEBUG("Registering AssetActionHandler");

        LE_FATAL_IF(assetData_client_AddAssetActionHandler(assetRef,
                                                           assetHandlerPtr,
                                                           NULL) == NULL,
                    "Could not register for instance activity on %s/%d.",
                    appNamePtr,
                    objectId);
    }

    for (size_t i = 0; i < monFieldCount; i++)
    {
        LE_DEBUG("Registering %s/%d/%d field handler.", appNamePtr, objectId, monitorFields[i]);

        LE_FATAL_IF(assetData_client_AddFieldActionHandler(assetRef,
                                                           monitorFields[i],
                                                           fieldHandlerPtr,
                                                           NULL) == NULL,
                    "Could not register for object %s/%d field activity.",
                    appNamePtr,
                    objectId);
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Set the LwM2m object 9 instance mapping for the application.  If NULL is passed for the instance
 *  reference, then any association is cleared.
 */
//--------------------------------------------------------------------------------------------------
static void SetObject9InstanceForApp
(
    const char* appName,                     ///< The name of the application in question.
    assetData_InstanceDataRef_t instanceRef  ///< The instance of object 9 to link to.  Pass NULL if
                                             ///<   the link is to be cleared.
)
//--------------------------------------------------------------------------------------------------
{
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(CFG_OBJECT_INFO_PATH);

    if (instanceRef != NULL)
    {
        int instanceId;
        LE_ASSERT(assetData_GetInstanceId(instanceRef, &instanceId) == LE_OK);

        le_cfg_GoToNode(iterRef, appName);
        le_cfg_SetInt(iterRef, "oiid", instanceId);

        LE_DEBUG("Application '%s' mapped to instance %d.", appName, instanceId);
    }
    else
    {
        le_cfg_DeleteNode(iterRef, appName);
    }

    le_cfg_CommitTxn(iterRef);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Try to get the current object 9 instance for the given application.  If one can not be found
 *  then create one.
 */
//--------------------------------------------------------------------------------------------------
static assetData_InstanceDataRef_t GetObject9InstanceForApp
(
    const char* appName,  ///< Name of the application in question.
    bool mapIfNotFound    ///< If an instance was created, should a mapping be created for it?
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("Getting object 9 instance for application '%s'.", appName);

    // Attempt to read the mapping from the configuration.
    assetData_InstanceDataRef_t instanceRef = NULL;
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(CFG_OBJECT_INFO_PATH);

    le_cfg_GoToNode(iterRef, appName);
    int instanceId = le_cfg_GetInt(iterRef, "oiid", -1);
    le_cfg_CancelTxn(iterRef);

    if (instanceId != -1)
    {
        LE_DEBUG("Was mapped to instance, %d.", instanceId);

        // Looks like there was a mapping.  Try to get that instance and make sure it's not taken
        // by another application.  If the instance was taken by another application, remap this
        // application to a new instance and update the mapping.
        if (assetData_GetInstanceRefById(LWM2M_NAME, 9, instanceId, &instanceRef) == LE_OK)
        {
            char newName[MAX_APP_NAME_BYTES] = "";
            LE_ASSERT(assetData_client_GetString(instanceRef,
                                                 O9F_PKG_NAME,
                                                 newName,
                                                 sizeof(newName)) == LE_OK);

            if (strcmp(newName, appName) != 0)
            {
                LE_DEBUG("Instance has been taken by '%s', creating new.", newName);

                LE_ASSERT(assetData_CreateInstanceById(LWM2M_NAME, 9, -1, &instanceRef) == LE_OK);
                LE_ASSERT(assetData_client_SetString(instanceRef, O9F_PKG_NAME, appName) == LE_OK);

                if (mapIfNotFound)
                {
                    LE_DEBUG("Recording new instance id.");
                    SetObject9InstanceForApp(appName, instanceRef);
                }
            }
            else
            {
                LE_DEBUG("Instance is existing and has been reused.");
            }
        }
        else
        {
            LE_DEBUG("No instance found, creating new as mapped.");

            LE_ASSERT(assetData_CreateInstanceById(LWM2M_NAME,
                                                   9,
                                                   instanceId,
                                                   &instanceRef) == LE_OK);

            LE_ASSERT(assetData_client_SetString(instanceRef, O9F_PKG_NAME, appName) == LE_OK);
        }
    }
    else
    {
        LE_DEBUG("No instance mapping found, creating new.");

        // A mapping was not found.  So create a new object, and let the data store assign an
        // instance Id.  If desired, at this point record the instance mapping for later use.
        LE_ASSERT(assetData_CreateInstanceById(LWM2M_NAME, 9, -1, &instanceRef) == LE_OK);
        LE_ASSERT(assetData_client_SetString(instanceRef, O9F_PKG_NAME, appName) == LE_OK);

        if (mapIfNotFound)
        {
            LE_DEBUG("Recording new instance id.");
            SetObject9InstanceForApp(appName, instanceRef);
        }
    }

    return instanceRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Event handler for the application specific object 0.  This function dynamically queries the
 *  associated application state and updates the object state field.
 */
//--------------------------------------------------------------------------------------------------
static void App0FieldHandler
(
    assetData_InstanceDataRef_t instanceRef,  ///< Object instance being acted on.
    int fieldId,                              ///< The field of that instance being acted on.
    assetData_ActionTypes_t action,           ///< The action being performed.
    void* contextPtr                          ///< Context for this action.
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("App0FieldHandler");

    if (   (fieldId == AO0F_STATE)
        && (action == ASSET_DATA_ACTION_READ))
    {
        char appName[MAX_APP_NAME_BYTES] = "";

        if (assetData_client_GetString(instanceRef, AO0F_NAME, appName, sizeof(appName)) == LE_OK)
        {
            int state = le_appInfo_GetState(appName);

            LE_DEBUG("Got application %s state as %d.", appName, state);
            LE_ASSERT(assetData_client_SetInt(instanceRef, AO0F_STATE, state) == LE_OK);
        }
        else
        {
            LE_ERROR("Application name truncated while reading state.");
        }
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Event handler for the application specific object 1.
 */
//--------------------------------------------------------------------------------------------------
static void App1FieldHandler
(
    assetData_InstanceDataRef_t instanceRef,  ///< Object instance being acted on.
    int fieldId,                              ///< The field of that instance being acted on.
    assetData_ActionTypes_t action,           ///< The action being performed.
    void* contextPtr                          ///< Context for this action.
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("App1FieldHandler");

    if (   (fieldId == AO0F_STATE)
        && (action == ASSET_DATA_ACTION_READ))
    {
        char appName[MAX_APP_NAME_BYTES] = "";
        char procName[MAX_PROC_NAME_BYTES] = "";

        if (assetData_GetAppNameFromInstance(instanceRef, appName, sizeof(appName)) != LE_OK)
        {
            LE_ERROR("Could not read app name for object instance.");
            return;
        }

        if (assetData_client_GetString(instanceRef, LO1F_NAME, procName, sizeof(procName)) != LE_OK)
        {
            LE_ERROR("Could not read process name for app, '%s', from asset data.", appName);
            return;
        }

        le_appInfo_ProcState_t state = le_appInfo_GetProcState(appName, procName);
        LE_ASSERT(assetData_client_SetInt(instanceRef, LO1F_STATE, state) == LE_OK);

        LE_DEBUG("Application '%s' process, '%s' state read as %d", appName, procName, state);
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Create custom Legato objects for the given application.
 */
//--------------------------------------------------------------------------------------------------
static void CreateLegatoObjectsForApp
(
    const char* appName  ///< Name of the application to create objects for.
)
//--------------------------------------------------------------------------------------------------
{
    char objNameBuffer[MAX_APP_NAME_BYTES + 3] = "";

    snprintf(objNameBuffer, sizeof(objNameBuffer), "%s", appName);

    assetData_InstanceDataRef_t objectRef = NULL;
    appCfg_Iter_t appIterRef = appCfg_FindApp(appName);

    LE_FATAL_IF(appIterRef == NULL, "Configuration for known application was not found.");


    // Create object 0.
    LE_ASSERT(assetData_CreateInstanceById(objNameBuffer, 0, 0, &objectRef) == LE_OK);

    static int obj0MonitorFields[] =
        {
            // AO0F_VERSION,
            // AO0F_NAME,
            AO0F_STATE,
            // AO0F_START_MODE
        };

    static int obj1MonitorFields[] =
        {
            // LO1F_NAME,
            // LO1F_EXEC_NAME,
            LO1F_STATE,
            // LO1F_FAULT_ACTION
        };

    RegisterFieldEventHandlers(objNameBuffer,
                               0,
                               NULL,
                               obj0MonitorFields,
                               NUM_ARRAY_MEMBERS(obj0MonitorFields),
                               App0FieldHandler);

    RegisterFieldEventHandlers(objNameBuffer,
                               1,
                               NULL,
                               obj1MonitorFields,
                               NUM_ARRAY_MEMBERS(obj1MonitorFields),
                               App1FieldHandler);

    char stringBuffer[512] = "";

    if (appCfg_GetVersion(appIterRef, stringBuffer, sizeof(stringBuffer)) == LE_OVERFLOW)
    {
        LE_WARN("Application %s version string truncated.", appName);
    }

    assetData_client_SetString(objectRef, AO0F_VERSION, stringBuffer);
    assetData_client_SetString(objectRef, AO0F_NAME, appName);
    assetData_client_SetInt(objectRef, AO0F_STATE, le_appInfo_GetState(appName));
    assetData_client_SetInt(objectRef, AO0F_START_MODE, appCfg_GetStartMode(appIterRef));


    // Create object 1 for each process.
    appCfg_Iter_t procIterRef = appCfg_CreateAppProcIter(appIterRef);
    int instanceId = 0;

    while (appCfg_GetNextItem(procIterRef) == LE_OK)
    {
        le_result_t result = assetData_CreateInstanceById(objNameBuffer, 1, instanceId, &objectRef);
        instanceId++;

        if (result != LE_OK)
        {
            LE_ERROR("Could not create process instance for application '%s'.  Reason, %s.",
                     appName,
                     LE_RESULT_TXT(result));
            continue;
        }

        if (appCfg_GetProcName(procIterRef, stringBuffer, sizeof(stringBuffer)) == LE_OVERFLOW)
        {
            LE_WARN("Application %s process name truncated.", appName);
        }

        assetData_client_SetString(objectRef, LO1F_NAME, stringBuffer);


        if (appCfg_GetProcExecName(procIterRef, stringBuffer, sizeof(stringBuffer)) == LE_OVERFLOW)
        {
            LE_WARN("Application %s process exec name truncated.", appName);
        }

        assetData_client_SetString(objectRef, LO1F_EXEC_NAME, stringBuffer);

        LE_DEBUG("Process running state hard coded to 1.");
        assetData_client_SetInt(objectRef, LO1F_STATE, 1);

        assetData_client_SetInt(objectRef,
                                LO1F_FAULT_ACTION,
                                appCfg_GetProcFaultAction(procIterRef));
    }

    appCfg_DeleteIter(procIterRef);
    appCfg_DeleteIter(appIterRef);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Create an instance of the Legato object.
 */
//--------------------------------------------------------------------------------------------------
static void CreateLegatoObject
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    assetData_InstanceDataRef_t legatoObjectRef = NULL;

    LE_ASSERT(assetData_CreateInstanceById(ASSET_DATA_LEGATO_OBJ_NAME,
                                           0,
                                           0,
                                           &legatoObjectRef) == LE_OK);

    char versionBuffer[MAX_VERSION_STR_BYTES] = "";

    GetLegatoVersionStr(versionBuffer);

    le_result_t result = assetData_client_SetString(legatoObjectRef, LO0F_VERSION, versionBuffer);
    LE_ERROR_IF(result != LE_OK,
                "Could not update Legato version field (%d): %s",
                result,
                LE_RESULT_TXT(result));
}




//--------------------------------------------------------------------------------------------------
/**
 *  Delete the Object instances for the given application.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteLegatoObjectsForApp
(
    const char* appName  ///< The application that's being removed.
)
//--------------------------------------------------------------------------------------------------
{
    assetData_InstanceDataRef_t instanceRef = NULL;
    le_result_t result = LE_OK;

    // Delete object 0.
    if (assetData_GetInstanceRefById(appName, 0, 0, &instanceRef) == LE_OK)
    {
        assetData_DeleteInstance(instanceRef);

        // Delete object 1 for each process.
        for (int instanceId = 0; result == LE_OK; instanceId++)
        {
            result = assetData_GetInstanceRefById(appName, 1, instanceId, &instanceRef);

            if (result == LE_OK)
            {
                assetData_DeleteInstance(instanceRef);
            }
        }
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Read the current state of the given object 9 instance.
 */
//--------------------------------------------------------------------------------------------------
static UpdateState GetOb9State
(
    assetData_InstanceDataRef_t instanceRef  ///< The object instance to read.
)
//--------------------------------------------------------------------------------------------------
{
    int state;

    LE_ASSERT(assetData_client_GetInt(instanceRef, O9F_UPDATE_STATE, &state) == LE_OK);
    return (UpdateState)state;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Update the state of the object 9 instance.  Also, because they are so closely related, update
 *  the update result field while we're at it.
 */
//--------------------------------------------------------------------------------------------------
static void SetObj9State_
(
    assetData_InstanceDataRef_t instanceRef,  ///< THe instance to update.
    UpdateState state,                        ///< The new state.
    UpdateResult result,                      ///< The new result.
    const char* functionNamePtr,              ///< Name of the function that called this one.
    size_t line                               ///< The line of this file this function was called
                                              ///<   from.
)
//--------------------------------------------------------------------------------------------------
{
    int instanceId;
    assetData_GetInstanceId(instanceRef, &instanceId);
    LE_DEBUG("<%s: %zu>: Set object 9 state/result on instance %d: (%d) %s / (%d) %s",
             functionNamePtr,
             line,
             instanceId,
             state,
             UpdateStateToStr(state),
             result,
             UpdateResultToStr(result));

    if (instanceRef == NULL)
    {
        LE_WARN("Setting state on NULL object.");
        return;
    }

    LE_ASSERT(assetData_client_SetInt(instanceRef, O9F_UPDATE_STATE, state) == LE_OK);
    LE_ASSERT(assetData_client_SetInt(instanceRef, O9F_UPDATE_RESULT, result) == LE_OK);
}

#define SetObj9State(insref, state, result) SetObj9State_(insref, state, result, __FUNCTION__, __LINE__)




//--------------------------------------------------------------------------------------------------
/**
 *  Notification handler that's called when an application is installed.
 */
//--------------------------------------------------------------------------------------------------
static void AppInstallHandler
(
    const char* appName,  ///< Name of the new application.
    void* contextPtr      ///< Registered context for this callback.
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("Application, '%s,' has been installed.", appName);

    if (IsHiddenApp(appName) == true)
    {
        LE_DEBUG("Application is hidden.");
        return;
    }

    assetData_InstanceDataRef_t instanceRef = NULL;

    // If an object 9 instance initiated this install, update it now.
    if (CurrentObj9 != NULL)
    {
        instanceRef = CurrentObj9;
        CurrentObj9 = NULL;

        LE_ASSERT(assetData_client_SetString(instanceRef, O9F_PKG_NAME, appName) == LE_OK);
        SetObject9InstanceForApp(appName, instanceRef);
    }
    else
    {
        // Otherwise, create one for this application that was installed outside of LWM2M.
        instanceRef = GetObject9InstanceForApp(appName, true);
    }

    // Mark the application as being installed.
    SetObj9State(instanceRef, US_INSTALLED, UR_INSTALLED);

    // Update the application's version string.
    appCfg_Iter_t appIterRef = appCfg_FindApp(appName);
    char versionBuffer[MAX_VERSION_STR_BYTES] = "";

    if (appCfg_GetVersion(appIterRef, versionBuffer, sizeof(versionBuffer)) == LE_OVERFLOW)
    {
        LE_WARN("Warning, app, '%s' version string truncated to '%s'.",
                appName,
                versionBuffer);
    }

    if (strlen(versionBuffer) == 0)
    {
        le_utf8_Copy(versionBuffer, VERSION_UNKNOWN, sizeof(versionBuffer), NULL);
    }

    assetData_client_SetString(instanceRef, O9F_PKG_VERSION, versionBuffer);

    appCfg_DeleteIter(appIterRef);

    // Finally, don't forget to create Legato objects for this app.
    CreateLegatoObjectsForApp(appName);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Handler that's called when an application is uninstalled.
 */
//--------------------------------------------------------------------------------------------------
static void AppUninstallHandler
(
    const char* appName,  ///< App being removed.
    void* contextPtr      ///< Context for this function.
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("Application, '%s,' has been uninstalled.", appName);

    if (IsHiddenApp(appName) == true)
    {
        LE_DEBUG("Application is hidden.");
        return;
    }

    // If this uninstall was requested by AirVantage, then move the object into it's initial state.
    // Otherwise this is a local uninstall, so check for an instance of object 9 for this
    // application and delete that instance if found.
    if (CurrentObj9 != NULL)
    {
        LE_DEBUG("LWM2M Uninstall of application.");

        SetObj9State(CurrentObj9, US_INITIAL, UR_INITIAL_VALUE);
        CurrentObj9 = NULL;
    }
    else
    {
        LE_DEBUG("Local Uninstall of application.");

        IsLocalUninstall = true;
        assetData_InstanceDataRef_t objectRef = GetObject9InstanceForApp(appName, false);

        if (objectRef != NULL)
        {
            assetData_DeleteInstance(objectRef);
        }
    }

    // Now, delete any app objects.
    DeleteLegatoObjectsForApp(appName);
    IsLocalUninstall = false;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Callback for when the uninstall script has completed.
 */
//--------------------------------------------------------------------------------------------------
static void OnUninstallCompleted
(
    int result,       ///< Command line result.
    void* contextPtr  ///< Context.
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("OnUninstallCompleted");

    assetData_InstanceDataRef_t instanceRef = (assetData_InstanceDataRef_t)contextPtr;

    char appName[MAX_APP_NAME_BYTES] = "";
    assetData_client_GetString(instanceRef, O9F_PKG_NAME, appName, sizeof(appName));

    LE_ERROR_IF(result != 0, "Uninstall of application '%s' failed.", appName);
    LE_DEBUG_IF(result == 0, "Uninstall of application '%s' completed.", appName);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Called during an application install.
 */
//--------------------------------------------------------------------------------------------------
static void UpdateProgressHandler
(
  le_update_State_t updateState,  ///< State of the update in question.
  uint percentDone,               ///< How much work has been done.
  void* contextPtr                ///< Context for the callback.
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("UpdateProgressHandler");

    switch (updateState)
    {
        case LE_UPDATE_STATE_NEW:
            LE_DEBUG("New update initialized.");
            break;

        case LE_UPDATE_STATE_UNPACKING:
            LE_DEBUG("Unpacking package.");
            break;

        case LE_UPDATE_STATE_APPLYING:
            LE_DEBUG("Doing update.");
            break;

        case LE_UPDATE_STATE_SUCCESS:
            LE_DEBUG("Install completed.");
            break;

        case LE_UPDATE_STATE_FAILED:
            LE_DEBUG("Install/uninstall failed.");
            SetObj9State(CurrentObj9, US_INITIAL, UR_INSTALLATION_FAILURE);
            CurrentObj9 = NULL;
            break;

        default:
            perror("Bad state\n");
            break;
     }
 }




//--------------------------------------------------------------------------------------------------
/**
 *  Called during application download.
 */
//--------------------------------------------------------------------------------------------------
void OnUriDownloadUpdate
(
    le_avc_Status_t updateStatus  ///< Status of the download in question.
)
//--------------------------------------------------------------------------------------------------
{

    switch (updateStatus)
    {
        case LE_AVC_DOWNLOAD_COMPLETE:
            LE_DEBUG("Download complete.");
            SetObj9State(CurrentObj9, US_DELIVERED, UR_INITIAL_VALUE);
            CurrentObj9 = NULL;
            break;

        case LE_AVC_DOWNLOAD_FAILED:
            LE_DEBUG("Download failed.");
            // TODO: Find out the real reason this failed.
            SetObj9State(CurrentObj9, US_INITIAL, UR_INSTALLATION_FAILURE);
            CurrentObj9 = NULL;
            break;

        // We don't care as much about these states as they are to deal with FOTA.
        case LE_AVC_DOWNLOAD_PENDING:
        case LE_AVC_DOWNLOAD_IN_PROGRESS:
            LE_DEBUG("Download pending/in progress.");
            break;

        case LE_AVC_NO_UPDATE:
            LE_DEBUG("No update.");
            break;

        case LE_AVC_INSTALL_FAILED:
            LE_DEBUG("Install failed.");
            break;

        case LE_AVC_INSTALL_COMPLETE:
            LE_DEBUG("Install complete.");
            break;

        case LE_AVC_INSTALL_IN_PROGRESS:
            LE_DEBUG("Install in progress.");
            break;

        case LE_AVC_INSTALL_PENDING:
            LE_DEBUG("Install pending.");
            break;

        // Should never get these values, so ignore them.
        case LE_AVC_SESSION_STARTED:
        case LE_AVC_SESSION_STOPPED:
            LE_INFO("Received unexpected updateStatus %i", updateStatus);
            break;
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Function called to kick off the install of a Legato application.
 */
//--------------------------------------------------------------------------------------------------
static void StartInstall
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    int firmwareFd;
    LE_DEBUG("Install application from SWI FOTA.");

    le_result_t result = pa_avc_ReadImage(&firmwareFd);

    if (result == LE_OK)
    {
        le_update_HandleRef_t updateHandle;

        if ((updateHandle = le_update_Create(firmwareFd)) == NULL)
        {
            LE_ERROR("Could not init connection to update daemon.");
            SetObj9State(CurrentObj9, US_INITIAL, UR_INSTALLATION_FAILURE);
            CurrentObj9 = NULL;
            return;
        }

        if (le_update_AddProgressHandler(updateHandle,
                                         UpdateProgressHandler,
                                         NULL) == NULL)
        {
            LE_ERROR("Could not register update handler.");
            SetObj9State(CurrentObj9, US_INITIAL, UR_INSTALLATION_FAILURE);
            CurrentObj9 = NULL;
            return;
        }

        if (le_update_Start(updateHandle) != LE_OK)
        {
            LE_ERROR("Could not register update handler.");
            CurrentObj9 = NULL;
            return;
        }
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Function called to kick off an application uninstall.
 */
//--------------------------------------------------------------------------------------------------
static void StartUninstall
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    char appName[MAX_APP_NAME_BYTES] = "";
    assetData_client_GetString(CurrentObj9, O9F_PKG_NAME, appName, sizeof(appName));

    LE_DEBUG("Application '%s' uninstall requested.", appName);
    LE_DEBUG("Send uninstall request.");

    const char* command[] = { "/usr/local/bin/app", "remove", appName, NULL };
    exec_RunProcess("/usr/local/bin/app",
                    command,
                    -1,
                    -1,
                    -1,
                    OnUninstallCompleted,
                    CurrentObj9);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Start up the requested app.
 */
//--------------------------------------------------------------------------------------------------
static void StartApp
(
    assetData_InstanceDataRef_t instanceRef,  ///< Instance of object 9 for this app.
    const char* appName                       ///< Name of the application.
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("Application '%s' start requested.", appName);

    if (GetOb9State(instanceRef) != US_INSTALLED)
    {
        LE_DEBUG("Application '%s' not installed.", appName);
        return;
    }

    LE_DEBUG("Send start request.");
    le_sup_ctrl_StartApp(appName);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Stop a Legato application.
 */
//--------------------------------------------------------------------------------------------------
static void StopApp
(
    assetData_InstanceDataRef_t instanceRef,  ///< Instance of object 9 for this app.
    const char* appName                       ///< Name of the app to stop.
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("Application '%s' stop requested.", appName);

    if (GetOb9State(instanceRef) != US_INSTALLED)
    {
        LE_DEBUG("Application '%s' not installed.", appName);
        return;
    }

    LE_DEBUG("Send stop request.");
    le_sup_ctrl_StopApp(appName);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Called when "interesting" activity happens on fields of the Legato framework object.
 */
//--------------------------------------------------------------------------------------------------
static void Legato0FieldActivityHandler
(
    assetData_InstanceDataRef_t instanceRef,  ///< The instance of the Legato object.
    int fieldId,                              ///< The field that updated.
    assetData_ActionTypes_t action,           ///< The action performed.
    void* contextPtr                          ///< Useful context.
)
//--------------------------------------------------------------------------------------------------
{
    int instanceId;
    LE_ASSERT(assetData_GetInstanceId(instanceRef, &instanceId) == LE_OK);

    if (instanceId != 0)
    {
        LE_ERROR("Legato objects outside of instance 0 are not supported.");
        return;
    }

    if (   (fieldId == LO0F_RESTART)
        && (action == ASSET_DATA_ACTION_EXEC))
    {
        const char* command[] = { "/usr/local/bin/legato", "restart", NULL };

        exec_RunProcess("/usr/local/bin/legato", command, -1, -1, -1, NULL, NULL);
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Called when "interesting" activity happens on fields of object 9 that we're watching.
 */
//--------------------------------------------------------------------------------------------------
static void Object9FieldActivityHandler
(
    assetData_InstanceDataRef_t instanceRef,  ///< The instance of object 9.
    int fieldId,                              ///< The field that updated.
    assetData_ActionTypes_t action,           ///< The action performed.
    void* contextPtr                          ///< Useful context.
)
//--------------------------------------------------------------------------------------------------
{
    char appName[MAX_APP_NAME_BYTES] = "";
    assetData_client_GetString(instanceRef, O9F_PKG_NAME, appName, sizeof(appName));

    int instanceId;
    LE_ASSERT(assetData_GetInstanceId(instanceRef, &instanceId) == LE_OK);

    LE_DEBUG("Object 9 instance %d: field: %s, action: %s.",
             instanceId,
             Obj9FieldToStr(fieldId),
             ActionToStr(action));

    bool isAvcService = false;

    if (strcmp(appName, AVC_SERVICE_NAME) == 0)
    {
        isAvcService = true;
    }

    switch (fieldId)
    {
        case O9F_PACKAGE_URI:
            LE_DEBUG("O9F_PACKAGE_URI");

            if (action == ASSET_DATA_ACTION_WRITE)
            {
                if (isAvcService)
                {
                    LE_ERROR("Installing %s over the air is not supported.", AVC_SERVICE_NAME);
                    return;
                }

                static char uri[MAX_URI_STR_BYTES] = "";
                assetData_client_GetString(instanceRef, O9F_PACKAGE_URI, uri, sizeof(uri));
                LE_DEBUG("Attempt to download from Url: %s", uri);

                if (CurrentObj9 != NULL)
                {
                    LE_WARN("Duplicate attempt detected.");
                    return;
                }

                SetObj9State(instanceRef, US_DOWNLOAD_STARTED, UR_DOWNLOADING);

                if (pa_avc_StartURIDownload(uri, OnUriDownloadUpdate) != LE_OK)
                {
                    SetObj9State(instanceRef, US_INITIAL, UR_INVALID_URI);
                }

                CurrentObj9 = instanceRef;
            }
            break;

        case O9F_INSTALL:
            LE_DEBUG("O9F_INSTALL");

            if (action == ASSET_DATA_ACTION_EXEC)
            {
                if (isAvcService)
                {
                    LE_ERROR("Installing %s over the air is not supported.", AVC_SERVICE_NAME);
                    return;
                }

                if (CurrentObj9 != NULL)
                {
                    LE_WARN("Duplicate install attempt detected.");
                    return;
                }

                CurrentObj9 = instanceRef;
                le_result_t result = avcServer_QueryInstall(StartInstall);

                LE_FATAL_IF(result == LE_FAULT,
                            "Unexpected error in query install: %s",
                            LE_RESULT_TXT(result));

                if (result != LE_BUSY)
                {
                    StartInstall();
                }
            }
            break;

        case O9F_UNINSTALL:
            LE_DEBUG("O9F_UNINSTALL");

            if (action == ASSET_DATA_ACTION_EXEC)
            {
                if (isAvcService)
                {
                    LE_ERROR("Uninstalling %s over the air is not supported.", AVC_SERVICE_NAME);
                    return;
                }

                if (CurrentObj9 != NULL)
                {
                    LE_WARN("Duplicate attempt detected.");
                    return;
                }

                CurrentObj9 = instanceRef;
                IsLocalUninstall = false;

                LE_DEBUG("Ignoring Uninstall.");
                // StartUninstall();

                // signal the server that the app is removed, though it is actually running
                LE_DEBUG("LWM2M Uninstall - Reset State to initial.");

                SetObj9State(CurrentObj9, US_INITIAL, UR_INITIAL_VALUE);
                CurrentObj9 = NULL;
            }
            break;

        case O9F_ACTIVATE:
            LE_DEBUG("O9F_ACTIVATE");

            if (action == ASSET_DATA_ACTION_EXEC)
            {
                if (isAvcService)
                {
                    LE_ERROR("Activating %s over the air is not supported.", AVC_SERVICE_NAME);
                    return;
                }

                StartApp(instanceRef, appName);
            }
            break;

        case O9F_DEACTIVATE:
            LE_DEBUG("O9F_DEACTIVATE");

            if (action == ASSET_DATA_ACTION_EXEC)
            {
                if (isAvcService)
                {
                    LE_ERROR("Deactivating %s over the air is not supported.", AVC_SERVICE_NAME);
                    return;
                }

                StopApp(instanceRef, appName);
            }
            break;

        case O9F_ACTIVATION_STATE:
            LE_DEBUG("O9F_ACTIVATION_STATE");

            if (action == ASSET_DATA_ACTION_READ)
            {
                le_appInfo_State_t state = le_appInfo_GetState(appName);
                LE_DEBUG("Read of application state, '%s' was found to be: %d", appName, state);
                LE_ASSERT(assetData_client_SetBool(instanceRef,
                                                   O9F_ACTIVATION_STATE,
                                                   state == LE_APPINFO_RUNNING) == LE_OK);
            }
            break;

        default:
            LE_FATAL("Unexpected field update encountered.");
            break;
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Callback for when instances of object 9 are created or destroyed.
 */
//--------------------------------------------------------------------------------------------------
static void Object9ActivityHandler
(
    assetData_AssetDataRef_t assetRef,  ///< The object in question.
    int instanceId,                     ///< Instance of the object being acted on.
    assetData_ActionTypes_t action,     ///< The action performed on the object.
    void* contextPtr                    ///< Context.
)
//--------------------------------------------------------------------------------------------------
{
    switch (action)
    {
        case ASSET_DATA_ACTION_CREATE:
            LE_DEBUG("LWM2M Object 9 instance (%d) created.", instanceId);
            break;

        case ASSET_DATA_ACTION_DELETE:
            {
                LE_DEBUG("LWM2M Object 9 instance (%d) deleted.", instanceId);

                assetData_InstanceDataRef_t instanceRef;

                LE_ASSERT(assetData_GetInstanceRefById(LWM2M_NAME,
                                                       9,
                                                       instanceId,
                                                       &instanceRef) == LE_OK);
                char appName[MAX_APP_NAME_BYTES] = "";
                LE_ASSERT(assetData_client_GetString(instanceRef,
                                                     O9F_PKG_NAME,
                                                     appName,
                                                     sizeof(appName)) == LE_OK);

                int state;
                LE_ASSERT(assetData_client_GetInt(instanceRef, O9F_UPDATE_STATE, &state) == LE_OK);

                // remove the application regardless of the state of the application
                if ( strlen(appName) > 0)
                {
                    if ( IsLocalUninstall == false )
                    {
                        CurrentObj9 = instanceRef;
                        StartUninstall();
                    }
                    CurrentObj9 = NULL;
                }
            }
            break;

        case ASSET_DATA_ACTION_WRITE:
        case ASSET_DATA_ACTION_EXEC:
        case ASSET_DATA_ACTION_READ:
            break;
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Create instances of object 9 and the Legato objects for all currently installed applications.
 */
//--------------------------------------------------------------------------------------------------
static void PopulateAppInfoObjects
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    appCfg_Iter_t appIterRef = appCfg_CreateAppsIter();
    char appName[MAX_APP_NAME_BYTES] = "";
    char versionBuffer[MAX_VERSION_STR_BYTES] = "";
    le_result_t result;

    int foundAppCount = 0;

    result = appCfg_GetNextItem(appIterRef);

    while (result == LE_OK)
    {
        result = appCfg_GetAppName(appIterRef, appName, sizeof(appName));

        if (   (result == LE_OK)
            && (IsHiddenApp(appName) == false))
        {
            LE_DEBUG("Loading object instance for app, '%s'.", appName);

            assetData_InstanceDataRef_t instanceRef = GetObject9InstanceForApp(appName, false);

            if (appCfg_GetVersion(appIterRef, versionBuffer, sizeof(versionBuffer)) == LE_OVERFLOW)
            {
                LE_WARN("Warning, app, '%s' version string truncated to '%s'.",
                        appName,
                        versionBuffer);
            }

            if (strlen(versionBuffer) == 0)
            {
                le_utf8_Copy(versionBuffer, VERSION_UNKNOWN, sizeof(versionBuffer), NULL);
            }

            assetData_client_SetString(instanceRef, O9F_PKG_VERSION, versionBuffer);

            assetData_client_SetBool(instanceRef,   O9F_UPDATE_SUPPORTED_OBJECTS, false);
            SetObj9State(instanceRef, US_INSTALLED, UR_INSTALLED);

            CreateLegatoObjectsForApp(appName);
            foundAppCount++;
        }
        else
        {
            LE_WARN("Application name too large or is hidden, '%s.'", appName);
        }

        result = appCfg_GetNextItem(appIterRef);
    }

    appCfg_DeleteIter(appIterRef);
    LE_FATAL_IF(result != LE_NOT_FOUND,
                "Application cache initialization, unexpected error returned, (%d): \"%s\"",
                result,
                LE_RESULT_TXT(result));

    int index = 0;

    LE_DEBUG("Found app count %d.", foundAppCount);

    while (foundAppCount > 0)
    {
        assetData_InstanceDataRef_t instanceRef = NULL;
        le_result_t result = assetData_GetInstanceRefById(LWM2M_NAME, 9, index, &instanceRef);

        LE_DEBUG("Index %d.", index);

        if (result == LE_OK)
        {
            assetData_client_GetString(instanceRef, O9F_PKG_NAME, appName, sizeof(appName));

            LE_DEBUG("Mapping app '%s'.", appName);

            SetObject9InstanceForApp(appName, instanceRef);
            foundAppCount--;
        }

        index++;
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Initialize this component and create instances of all of the installed application objects.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    exec_Init();

    // Make sure that we're notified when applications are installed and removed from the system.
    le_instStat_AddAppInstallEventHandler(AppInstallHandler, NULL);
    le_instStat_AddAppUninstallEventHandler(AppUninstallHandler, NULL);

    // Register for Legato Object 0 Events.
    static int legatoObjMonitorFields[] =
        {
            // LO0F_VERSION,
            LO0F_RESTART
        };

    // Register for Object 9 Events.
    static int obj9MonitorFields[] =
        {
            // O9F_PKG_NAME,
            // O9F_PKG_VERSION,
            // O9F_PACKAGE,
            O9F_PACKAGE_URI,
            O9F_INSTALL,
            // O9F_CHECKPOINT,
            O9F_UNINSTALL,
            // O9F_UPDATE_STATE,
            // O9F_UPDATE_SUPPORTED_OBJECTS,
            // O9F_UPDATE_RESULT,
            O9F_ACTIVATE,
            O9F_DEACTIVATE,
            O9F_ACTIVATION_STATE,
            // O9F_PACKAGE_SETTINGS
        };


    RegisterFieldEventHandlers(ASSET_DATA_LEGATO_OBJ_NAME,
                               0,
                               NULL,
                               legatoObjMonitorFields,
                               NUM_ARRAY_MEMBERS(legatoObjMonitorFields),
                               Legato0FieldActivityHandler);

    RegisterFieldEventHandlers(LWM2M_NAME,
                               9,
                               Object9ActivityHandler,
                               obj9MonitorFields,
                               NUM_ARRAY_MEMBERS(obj9MonitorFields),
                               Object9FieldActivityHandler);

    PopulateAppInfoObjects();
    CreateLegatoObject();
}
