/**
 * @file avcObject.c
 *
 *  This component handles creation of AVC app object (lwm2m/9)
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_print.h"
#include "appCfg.h"
#include "../avcDaemon/assetData.h"
#include "avcObject.h"
#include "avcShared.h"


//--------------------------------------------------------------------------------------------------
/**
 *  Name of the standard objects in LWM2M.
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
 *  Base path for an Object 9 application binding inside of the configTree.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_OBJECT_INFO_PATH "system:/lwm2m/objectMap"


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
    if (true == le_cfg_QuickGetBool("/lwm2m/hideDefaultApps", true))
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
                "smsInboxService",
                "gpioService",
                "tools",
                "atService",
                "atClient",
                "atServer",
                "spiService",
                "devMode",
                "wifiService",
                "wifiClientTest",
                "wifiApTest",
                "wifiWebAp",
                "wifi"
            };

        size_t i;
        for (i = 0; i < NUM_ARRAY_MEMBERS(appList); i++)
        {
            if (0 == strcmp(appList[i], appName))
            {
                return true;
            }
        }
    }

    return false;
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
        if (LE_OK == assetData_GetInstanceRefById(LWM2M_NAME,
                                                  LWM2M_SOFTWARE_UPDATE,
                                                  instanceId,
                                                  &instanceRef))
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
 *  Update the state of the object 9 instance.  Also, because they are so closely related, update
 *  the update result field while we're at it.
 */
//--------------------------------------------------------------------------------------------------
static void SetObj9State_
(
    assetData_InstanceDataRef_t instanceRef,  ///< THe instance to update.
    UpdateState state,                        ///< The new state.
    UpdateResult result,                      ///< The new result.
    bool isSaveState,                         ///< Save state to configtree?
    const char* functionNamePtr,              ///< Name of the function that called this one.
    size_t line                               ///< The line of this file this function was called
                                              ///<   from.
)
//--------------------------------------------------------------------------------------------------
{
    int instanceId;

    if (instanceRef == NULL)
    {
        LE_WARN("Setting state on NULL object.");
        return;
    }

    assetData_GetInstanceId(instanceRef, &instanceId);
    LE_DEBUG("<%s: %zu>: Set object 9 state/result on instance %d: (%d) %s / (%d) %s",
             functionNamePtr,
             line,
             instanceId,
             state,
             UpdateStateToStr(state),
             result,
             UpdateResultToStr(result));

    LE_ASSERT(assetData_client_SetInt(instanceRef, O9F_UPDATE_STATE, state) == LE_OK);
    LE_ASSERT(assetData_client_SetInt(instanceRef, O9F_UPDATE_RESULT, result) == LE_OK);
}

#define SetObj9State(insref, state, result, isSaveState) \
        SetObj9State_(insref, state, result, isSaveState, __FUNCTION__, __LINE__)


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

        if ((result == LE_OK) &&
            (false == IsHiddenApp(appName)))
        {
            LE_DEBUG("Loading object instance for app, '%s'.", appName);

            assetData_InstanceDataRef_t instanceRef = GetObject9InstanceForApp(appName, false);

            if (appCfg_GetVersion(appIterRef, versionBuffer, sizeof(versionBuffer)) == LE_OVERFLOW)
            {
                LE_WARN("Warning, app, '%s' version string truncated to '%s'.",
                        appName,
                        versionBuffer);
            }

            if (0 == strlen(versionBuffer))
            {
                // Use the application hash if the version is empty
                le_appInfo_GetHash(appName, versionBuffer, sizeof(versionBuffer));
            }

            assetData_client_SetString(instanceRef, O9F_PKG_VERSION, versionBuffer);

            assetData_client_SetBool(instanceRef,   O9F_UPDATE_SUPPORTED_OBJECTS, false);

            // No need to save the status in config tree, while populating object9
            SetObj9State(instanceRef, US_INSTALLED, UR_INSTALLED, false);

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
 * Init this sub-component
 */
//--------------------------------------------------------------------------------------------------
le_result_t avcObject_Init
(
    void
)
{
    PopulateAppInfoObjects();

    assetData_RegistrationUpdate(ASSET_DATA_SESSION_STATUS_CHECK);

    return LE_OK;
}
