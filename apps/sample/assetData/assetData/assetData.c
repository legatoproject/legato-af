//--------------------------------------------------------------------------------------------------
/**
 * This app provides a realistic usage example of the Asset Data API.
 *
 * We have a room with a number of smart devices:
 * 1. a smart video camera that analyzes the images and determines the number of people and dogs.
 * 2. a thermostat that reports the current temperature and allows the user to set the desired temp.
 * 3. a fancy fan that allows various settings.
 *
 * A Sierra Wireless IoT device connects to these smart devices and also an AirVantage server,
 * which can remotely control these smart devices.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"


//-------------------------------------------------------------------------------------------------
/**
 * Asset Data paths.
 */
//-------------------------------------------------------------------------------------------------

/* variables */

// int
#define NUM_PEOPLE_VAR_RES  "/home1/room1/SmartCam/numPeople"
#define NUM_DOGS_VAR_RES    "/home1/room1/SmartCam/numDogs"

// string
#define ROOM_NAME_VAR_RES   "/home1/room1/roomName"

// bool
#define IS_VACANT_VAR_RES   "/home1/room1/isVacant"

// float
// thermostat temp reading
#define THERMOSTAT_TEMP_READING_VAR_RES "/home1/room1/thermostat/tempReading"


/* settings */

// thermostat temp setting (float)
#define THERMOSTAT_TEMP_SETTING_SET_RES "/home1/room1/thermostat/tempSetting"

// thermostat strength setting (int)
#define THERMOSTAT_STRENGTH_SET_RES "/home1/room1/thermostat/strengthSetting"

// thermostat power setting (bool)
#define THERMOSTAT_POWER_SET_RES "/home1/room1/thermostat/powerSetting"

// thermostat LCD text setting (string)
#define THERMOSTAT_LCDTEXT_SET_RES "/home1/room1/thermostat/lcdtextSetting"


/* commands */
#define FANCONTROL_CMD_RES              "/home1/room1/fan/fanControl"


//-------------------------------------------------------------------------------------------------
/**
 * Counters recording the number of times certain hardwares are accessed.
 */
//-------------------------------------------------------------------------------------------------
static int ReadNumPeopleCounter = 0;
static int WriteTempSettingCounter = 0;
static int ReadTempSettingCounter = 0;


//-------------------------------------------------------------------------------------------------
/**
 * Fancy Fan related declarations.
 */
//-------------------------------------------------------------------------------------------------
#define FANCY_FAN_LCD_TEXT_STR_BYTES    10

typedef enum
{
    FAN_MOVEMENT_SWING,
    FAN_MOVEMENT_ROTATE,
    FAN_MOVEMENT_STOP
}
FanMovement_t;


//////////////////////////////////////////////////////////////////////
/* Functions accessing the actual hardware                          */
//////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
/**
 * Accessing the Smart Cam and obtain the number of people in sight.
 */
//-------------------------------------------------------------------------------------------------
static uint32_t SmartCam_GetNumPpl
(
    void
)
{
    // Smart cam doing its smart thing
    return 135;
}


//-------------------------------------------------------------------------------------------------
/**
 * Accessing the Smart Cam and obtain the number of dogs in sight.
 */
//-------------------------------------------------------------------------------------------------
static uint32_t SmartCam_GetNumDogs
(
    void
)
{
    // Smart cam doing its smart thing
    return 321;
}


//-------------------------------------------------------------------------------------------------
/**
 * Accessing the Thermostat and obtain the current temperature.
 */
//-------------------------------------------------------------------------------------------------
static double Thermostat_GetTemp
(
    void
)
{
    // Getting the current temperature reading
    return 23.456;
}


//-------------------------------------------------------------------------------------------------
/**
 * Controlling the Fancy Fan with the supplied settings.
 */
//-------------------------------------------------------------------------------------------------
static le_result_t FancyFanControl
(
    bool isOn,                 // false is off, true is on
    double fanSpeed,           // from 0 to 1, from weakest to the strongest
    FanMovement_t fanMovement, // 3 settings from 1 to 3 (swing, rotate, stop)
    char* customText           // text to be printed on the fan's LCD display
)
{
    // Do the actual fan control
    LE_INFO("----------------------- Fancy Fan is blowing swiftly with these settings: "
            "On [%d], Speed [%g], movement [%d], text [%s]",
            isOn, fanSpeed, fanMovement, customText);

    // This is demonstrating the ability of the command execution result being able to be sent back
    // to the AV server. This faulty fancy fan can be turned on but cannot be turned off.
    if (isOn)
    {
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
}


//////////////////////////////////////////////////////////////////////
/* Asset data handlers                                              */
//////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
/**
 * Handler to call when AV server reads number of people.
 */
//-------------------------------------------------------------------------------------------------
static void ReadNumPeopleHandler
(
    const char* path,
    le_avdata_AccessType_t accessType,
    le_avdata_ArgumentListRef_t argumentList,
    void* contextPtr
)
{
    ReadNumPeopleCounter++;

    LE_INFO("------------------- Server reads number of people in the room [%d] times ------------",
            ReadNumPeopleCounter);
}


//-------------------------------------------------------------------------------------------------
/**
 * Handler to call when AV server reads the thermostat temp reading, or writes to the thermostat
 * temp setting.
 */
//-------------------------------------------------------------------------------------------------
static void AccessTempSettingHandler
(
    const char* path,
    le_avdata_AccessType_t accessType,
    le_avdata_ArgumentListRef_t argumentList,
    void* contextPtr
)
{
    if (accessType == LE_AVDATA_ACCESS_WRITE)
    {
        WriteTempSettingCounter++;

        LE_INFO("------------------- Server writes temperature setting [%d] times ------------",
                WriteTempSettingCounter);
    }
    else if (accessType == LE_AVDATA_ACCESS_READ)
    {
        ReadTempSettingCounter++;

        LE_INFO("------------------- Server reads temperature setting [%d] times ------------",
                ReadTempSettingCounter);
    }
    else
    {
        // Note that this should never happen, since AVC daemon is already performing access type
        // check.
        LE_WARN("AV server attempts to perform action (likely execute) on a read/write resource ");
    }
}


//-------------------------------------------------------------------------------------------------
/**
 * Handler to call when AV server executes a command to control the Fancy Fan.
 */
//-------------------------------------------------------------------------------------------------
static void ExecFanCtrlCmd
(
    const char* path,
    le_avdata_AccessType_t accessType,
    le_avdata_ArgumentListRef_t argumentList,
    void* contextPtr
)
{
    LE_INFO("-------- Executing fancy fan control command ---------------------------------------");

    // Determine the size of customText
    int argLength;
    int customTextBytes;

    if (le_avdata_GetStringArgLength(argumentList, "customText", &argLength) == LE_OK)
    {
        customTextBytes = ((argLength + 1) > FANCY_FAN_LCD_TEXT_STR_BYTES) ?
                          FANCY_FAN_LCD_TEXT_STR_BYTES : (argLength + 1);
    }
    else
    {
        customTextBytes = FANCY_FAN_LCD_TEXT_STR_BYTES;
    }

    // Command arguments
    bool isOn = false;
    double fanSpeed = 0;
    int32_t fanMovement = FAN_MOVEMENT_STOP;
    char customText[customTextBytes];

    // Note that the argument names (isOn, fanSpeed, etc.) and types are communicated to the av
    // server via the app model that the app writer also composes.
    if (le_avdata_GetBoolArg(argumentList, "isOn", &isOn) != LE_OK)
    {
        LE_WARN("Failed to get argument 'isOn'. Use the default value of [%d]", isOn);
    }

    if (le_avdata_GetFloatArg(argumentList, "fanSpeed", &fanSpeed) != LE_OK)
    {
        LE_WARN("Failed to get argument 'fanSpeed'. Use the default value of [%g]", fanSpeed);
    }

    if (le_avdata_GetIntArg(argumentList, "fanMovement", &fanMovement) != LE_OK)
    {
        LE_WARN("Failed to get argument 'fanMovement'. Use the default value of [%d]", fanMovement);
    }


    le_result_t result = le_avdata_GetStringArg(argumentList, "customText", customText,
                                                customTextBytes);

    if (result != LE_OK)
    {
        LE_WARN("Failed to get argument 'customText'. Use the default value of [%s]", customText);
    }

    // Perform the actual fan control with the arguments obtained from AV server, and reply the
    // result of the command execution to AVC Daemon (and thus AV server).
    le_result_t cmdExeResult = FancyFanControl(isOn, fanSpeed, fanMovement, customText);

    // Reply AV server with the command execution result.
    LE_INFO("command result is: [%s]", LE_RESULT_TXT(cmdExeResult));
    le_avdata_ReplyExecResult(argumentList, cmdExeResult);
}


//-------------------------------------------------------------------------------------------------
/**
 * Reports the asset data values stored in AVC Daemon.
 * Note that this is more for testing purposes.
 */
//-------------------------------------------------------------------------------------------------
static void RoomStatusReport
(
    le_timer_Ref_t timerRef
)
{
    LE_INFO("---------------- Room Status Report BEGIN-------------------------------------------");

    int32_t intVal = 0;
    char strVal[50] = {0};
    bool boolVal = false;
    double doubleVal = 0;
    le_result_t result;

    LE_INFO("::: VARIABLES :::::::::");

    le_avdata_GetInt(NUM_PEOPLE_VAR_RES, &intVal);
    LE_INFO("-- num people: [%d]", intVal);

    intVal = 0;
    le_avdata_GetInt(NUM_DOGS_VAR_RES, &intVal);
    LE_INFO("-- num dogs: [%d]", intVal);

    le_avdata_GetString(ROOM_NAME_VAR_RES, strVal, 50);
    LE_INFO("-- room name: [%s]", strVal);

    le_avdata_GetBool(IS_VACANT_VAR_RES, &boolVal);
    LE_INFO("-- room vacancy: [%d]", boolVal);

    le_avdata_GetFloat(THERMOSTAT_TEMP_READING_VAR_RES, &doubleVal);
    LE_INFO("-- thermostat temp reading: [%f]", doubleVal);


    LE_INFO("::: SETTINGS :::::::::");

    doubleVal = 0;
    result = le_avdata_GetFloat(THERMOSTAT_TEMP_SETTING_SET_RES, &doubleVal);
    if (LE_OK == result)
    {
        LE_INFO("-- thermostat temp setting: [%f]", doubleVal);
    }
    else if (LE_UNAVAILABLE == result)
    {
        LE_INFO("-- thermostat temp setting: not initialized");
    }
    else
    {
        LE_FATAL("unexpected result %s", LE_RESULT_TXT(result));
    }

    intVal = 0;
    result = le_avdata_GetInt(THERMOSTAT_STRENGTH_SET_RES, &intVal);
    if (LE_OK == result)
    {
        LE_INFO("-- thermostat strength: [%d]", intVal);
    }
    else if (LE_UNAVAILABLE == result)
    {
        LE_INFO("-- thermostat strength: not initialized");
    }
    else
    {
        LE_FATAL("unexpected result %s", LE_RESULT_TXT(result));
    }

    boolVal = false;
    result = le_avdata_GetBool(THERMOSTAT_POWER_SET_RES, &boolVal);
    if (LE_OK == result)
    {
        LE_INFO("-- thermostat power: [%d]", boolVal);
    }
    else if (LE_UNAVAILABLE == result)
    {
        LE_INFO("-- thermostat power: not initialized");
    }
    else
    {
        LE_FATAL("unexpected result %s", LE_RESULT_TXT(result));
    }

    memset(strVal, 0, sizeof(strVal));
    result = le_avdata_GetString(THERMOSTAT_LCDTEXT_SET_RES, strVal, 50);
    if (LE_OK == result)
    {
        LE_INFO("-- thermostat LCD text: [%s]", strVal);
    }
    else if (LE_UNAVAILABLE == result)
    {
        LE_INFO("-- thermostat LCD text: not initialized");
    }
    else
    {
        LE_FATAL("unexpected result %s", LE_RESULT_TXT(result));
    }

    LE_INFO("---------------- Room Status Report END---------------------------------------------");
}


//-------------------------------------------------------------------------------------------------
/**
 * Reads sensor/hardware data and Updates the the asset data values stored in AVC Daemon.
 */
//-------------------------------------------------------------------------------------------------
static void ValueUpdate
(
    le_timer_Ref_t timerRef
)
{
    LE_INFO("---------------- Updating values from hardware -------------------------------------");

    le_avdata_SetInt(NUM_PEOPLE_VAR_RES, SmartCam_GetNumPpl());
    le_avdata_SetInt(NUM_DOGS_VAR_RES, SmartCam_GetNumDogs());
    le_avdata_SetString(ROOM_NAME_VAR_RES, "Super Awesome Room!");
    le_avdata_SetBool(IS_VACANT_VAR_RES, false);
    le_avdata_SetFloat(THERMOSTAT_TEMP_READING_VAR_RES, Thermostat_GetTemp());
}


//////////////////////////////////////////////////////////////////////
/* Functions relevant to AV server connection                       */
//////////////////////////////////////////////////////////////////////


//-------------------------------------------------------------------------------------------------
/**
 * Status handler for avcService updates.
 */
//-------------------------------------------------------------------------------------------------
static void StatusHandler
(
    le_avc_Status_t updateStatus,
    int32_t totalNumBytes,
    int32_t downloadProgress,
    void* contextPtr
)
{
    const char* statusPtr = NULL;

    switch (updateStatus)
    {
        case LE_AVC_NO_UPDATE:
            statusPtr = "NO_UPDATE";
            break;
        case LE_AVC_DOWNLOAD_PENDING:
            statusPtr = "DOWNLOAD_PENDING";
            break;
        case LE_AVC_DOWNLOAD_IN_PROGRESS:
            statusPtr = "DOWNLOAD_IN_PROGRESS";
            break;
        case LE_AVC_DOWNLOAD_COMPLETE:
            statusPtr = "DOWNLOAD_COMPLETE";
            break;
        case LE_AVC_DOWNLOAD_FAILED:
            statusPtr = "DOWNLOAD_FAILED";
            break;
        case LE_AVC_INSTALL_PENDING:
            statusPtr = "INSTALL_PENDING";
            break;
        case LE_AVC_INSTALL_IN_PROGRESS:
            statusPtr = "INSTALL_IN_PROGRESS";
            break;
        case LE_AVC_INSTALL_COMPLETE:
            statusPtr = "INSTALL_COMPLETE";
            break;
        case LE_AVC_INSTALL_FAILED:
            statusPtr = "INSTALL_FAILED";
            break;
        case LE_AVC_UNINSTALL_PENDING:
            statusPtr = "UNINSTALL_PENDING";
            break;
        case LE_AVC_UNINSTALL_IN_PROGRESS:
            statusPtr = "UNINSTALL_IN_PROGRESS";
            break;
        case LE_AVC_UNINSTALL_COMPLETE:
            statusPtr = "UNINSTALL_COMPLETE";
            break;
        case LE_AVC_UNINSTALL_FAILED:
            statusPtr = "UNINSTALL_FAILED";
            break;
        case LE_AVC_SESSION_STARTED:
            statusPtr = "SESSION_STARTED";
            break;
        case LE_AVC_SESSION_STOPPED:
            statusPtr = "SESSION_STOPPED";
            break;
        case LE_AVC_REBOOT_PENDING:
            statusPtr = "REBOOT_PENDING";
            break;
        case LE_AVC_CONNECTION_REQUIRED:
            statusPtr = "CONNECTION_REQUIRED";
            break;
        case LE_AVC_AUTH_STARTED:
            statusPtr = "AUTHENTICATION_STARTED";
            break;
        case LE_AVC_AUTH_FAILED:
            statusPtr = "AUTHENTICATION_FAILED";
            break;
    }

    if (statusPtr == NULL)
    {
        LE_ERROR("Air Vantage agent reported unexpected update status: %d", updateStatus);
    }
    else
    {
        LE_INFO("Air Vantage agent reported update status: %s", statusPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Component initializer. Starts an Air Vantage connection, create asset data, and create timers to
 * periodically update the asset data values with those from the hardware, and also report the asset
 * data values.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Air Vantage Connection Controller started.");

    // Register Air Vantage status report handler.
    le_avc_AddStatusEventHandler(StatusHandler, NULL);

    // Start an AV session.
    le_result_t res = le_avc_StartSession();
    if (res != LE_OK)
    {
        LE_ERROR("Failed to connect to AirVantage: %s", LE_RESULT_TXT(res));

        LE_INFO("Attempting to stop previous session, in case one is still active...");
        res = le_avc_StopSession();
        if (res != LE_OK)
        {
            LE_ERROR("Failed to stop session: %s", LE_RESULT_TXT(res));
        }
        else
        {
            LE_INFO("Successfully stopped session.  Attempting to start a new one.");
            res = le_avc_StartSession();
            if (res != LE_OK)
            {
                LE_FATAL("Failed to connect to AirVantage: %s", LE_RESULT_TXT(res));
            }
        }
    }

    LE_INFO("Air Vantage session started successfully.");

    /* Create resources */
    le_avdata_CreateResource(NUM_PEOPLE_VAR_RES, LE_AVDATA_ACCESS_VARIABLE);
    le_avdata_CreateResource(NUM_DOGS_VAR_RES, LE_AVDATA_ACCESS_VARIABLE);
    le_avdata_CreateResource(ROOM_NAME_VAR_RES, LE_AVDATA_ACCESS_VARIABLE);
    le_avdata_CreateResource(IS_VACANT_VAR_RES, LE_AVDATA_ACCESS_VARIABLE);

    le_avdata_CreateResource(THERMOSTAT_TEMP_READING_VAR_RES, LE_AVDATA_ACCESS_VARIABLE);
    le_avdata_CreateResource(THERMOSTAT_TEMP_SETTING_SET_RES, LE_AVDATA_ACCESS_SETTING);
    le_avdata_CreateResource(THERMOSTAT_STRENGTH_SET_RES, LE_AVDATA_ACCESS_SETTING);
    le_avdata_CreateResource(THERMOSTAT_POWER_SET_RES, LE_AVDATA_ACCESS_SETTING);
    le_avdata_CreateResource(THERMOSTAT_LCDTEXT_SET_RES, LE_AVDATA_ACCESS_SETTING);

    le_avdata_CreateResource(FANCONTROL_CMD_RES, LE_AVDATA_ACCESS_COMMAND);

    /* Register handlers */
    le_avdata_AddResourceEventHandler(NUM_PEOPLE_VAR_RES, ReadNumPeopleHandler, NULL);

    le_avdata_AddResourceEventHandler(THERMOSTAT_TEMP_SETTING_SET_RES,
                                      AccessTempSettingHandler, NULL);

    le_avdata_AddResourceEventHandler(FANCONTROL_CMD_RES, ExecFanCtrlCmd, NULL);

    /* Read values from the data sources (sensors, hardware, etc.) and update AVC daemon's
       internal record. */
    le_timer_Ref_t ValueUpdateTimer = le_timer_Create("ValueUpdateTimer");
    le_clk_Time_t interval = {20, 0};
    le_timer_SetInterval(ValueUpdateTimer, interval);
    le_timer_SetRepeat(ValueUpdateTimer, 0); // repeat indefinitely
    le_timer_SetHandler(ValueUpdateTimer, ValueUpdate);
    le_timer_Start(ValueUpdateTimer);

    /* Reports the current room status */
    le_timer_Ref_t RoomStatusReportTimer = le_timer_Create("RoomStatusReportTimer");
    interval.sec = 10;
    le_timer_SetInterval(RoomStatusReportTimer, interval);
    le_timer_SetRepeat(RoomStatusReportTimer, 0); // repeat indefinitely
    le_timer_SetHandler(RoomStatusReportTimer, RoomStatusReport);
    le_timer_Start(RoomStatusReportTimer);

    // Initialize values
    ValueUpdate(ValueUpdateTimer);
}
