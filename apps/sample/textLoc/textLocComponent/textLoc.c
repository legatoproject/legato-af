//--------------------------------------------------------------------------------------------------
/** @file textLoc.c
 *
 * This app illustrates a sample usage of ultra low power mode API. This app reads the current gps
 * location. sends it to a destination cell phone number, then enters into ultra low power mode.
 *
 * @note This app expects destination cell number to be specified in environment variable section
 * of adef file. If nothing specified in environment variable, it will send message to a default
 * non-existent phone number.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------


#include "legato.h"
/* IPC APIs */
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * GPS timeout interval in minutes
 *
 * @note Please change this timeout value as needed.
 */
//--------------------------------------------------------------------------------------------------
#define GPSTIMEOUT 15


//--------------------------------------------------------------------------------------------------
/**
 * Default phone number to send location information.
 *
 * @note This is a non-existent phone number (ref: https://en.wikipedia.org/wiki/Fictitious_telephone_number).
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_PHONE_NO "8005550101"


//--------------------------------------------------------------------------------------------------
/**
 * Timer interval(in seconds) to exit from shutdown/ultralow-power state.
 *
 * @note Please change this interval as needed.
 */
//--------------------------------------------------------------------------------------------------
#define ULPM_EXIT_INTERVAL    3600


//--------------------------------------------------------------------------------------------------
/**
 * Variable to store destination cell phone number.
 *
 * @note
 */
//--------------------------------------------------------------------------------------------------
static char DestPhoneNo[20] = "";


//--------------------------------------------------------------------------------------------------
/**
 * Function to get current location and send it to a specific destination.
 */
//--------------------------------------------------------------------------------------------------
static void GetCurrentLocation
(
    void
)
{
    le_posCtrl_ActivationRef_t posCtrlRef = le_posCtrl_Request();

    le_sms_MsgRef_t smsMessage;
    char smsText[120];

    int32_t     latitude = INT32_MAX;
    int32_t     longitude = INT32_MAX;
    int32_t     hAccuracy = INT32_MAX;

    le_result_t result = LE_FAULT;
    int32_t gpsTimeCount = 0;

    if(posCtrlRef)
    {
        LE_INFO("Positioning service activated");

        while (result != LE_OK)
        {
            sleep(1);

            // Check whether gps time out interval reached.
            if (gpsTimeCount >= (GPSTIMEOUT * 60))
            {
                break;
            }

            LE_INFO("Checking GPS position");
            result = le_pos_Get2DLocation(&latitude, &longitude, &hAccuracy);
            gpsTimeCount++;
        }

        LE_INFO("Sending SMS");
        smsMessage = le_sms_Create();
        le_sms_SetDestination(smsMessage, DestPhoneNo);

        if (latitude != INT32_MAX)
        {
            sprintf(smsText, "Loc:%d,%d", latitude, longitude);
        }
        else
        {
            sprintf(smsText, "Loc: unknown");
        }
        le_sms_SetText(smsMessage, smsText);

        if (le_sms_Send(smsMessage) == LE_OK)
        {
            LE_INFO("SMS Message sent");
        }
        else
        {
            LE_ERROR("Could not send SMS message");
        }

        LE_INFO("Releasing Positioning service");
        le_posCtrl_Release(posCtrlRef);
    }
    else
    {
        LE_ERROR("Can't activate the Positioning service");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to configure boot source and shutdown MDM.
 */
//--------------------------------------------------------------------------------------------------
static void CfgShutDown
(
    void
)
{
    // Boot after specified interval.
    if (le_ulpm_BootOnTimer(ULPM_EXIT_INTERVAL) != LE_OK)
    {
        LE_ERROR("Can't set timer as boot source");
        return;
    }

    // Initiate shutdown.
    if (le_ulpm_ShutDown() != LE_OK)
    {
        LE_ERROR("Can't initiate shutdown.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to check whether MDM booted due to timer expiry.
 */
//--------------------------------------------------------------------------------------------------
void CheckMDMTimerBoot
(
    void
)
{
    if (le_bootReason_WasTimer())
    {
        LE_INFO("Timer-boot");
    }
    else
    {
        LE_INFO("Other boot reason");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Callback function to handle change of network registration state.
 */
//--------------------------------------------------------------------------------------------------
static void RegistrationStateHandler
(
    le_mrc_NetRegState_t state,     ///< [IN] Network registration state.
    void *contextPtr                ///< [IN] Context pointer.
)
{
    switch(state)
    {
        case LE_MRC_REG_HOME:
        case LE_MRC_REG_ROAMING:
            LE_INFO("Registered");
            GetCurrentLocation();
            LE_INFO("Now configure boot source and shutdown MDM");
            CfgShutDown();
            break;
        case LE_MRC_REG_SEARCHING:
            LE_INFO("Searching...");
            break;
        case LE_MRC_REG_NONE:
            LE_INFO("Not registered");
            break;
        case LE_MRC_REG_DENIED:
            LE_ERROR("Registration denied");
            break;
        case LE_MRC_REG_UNKNOWN:
            LE_ERROR("Unknown registration state");
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the destination phone number.
 *
 **/
//--------------------------------------------------------------------------------------------------
static void GetDestinationCellNo
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    const char* envStrPtr = getenv("DEST_CELL_NO");

    if (envStrPtr == NULL)
    {
        LE_WARN("No destination cell number is specified. Setting it to default non-existent number");
        le_utf8_Copy(DestPhoneNo, DEFAULT_PHONE_NO, sizeof(DestPhoneNo), NULL);
    }

    le_utf8_Copy(DestPhoneNo, envStrPtr, sizeof(DestPhoneNo), NULL);

    LE_INFO("Destination phone number = %s", DestPhoneNo);
}


COMPONENT_INIT
{

    LE_INFO("TextLoc started");

    // Get the destination phone number
    GetDestinationCellNo();

    // Now check whether MDM booted due to timer expiry.
    CheckMDMTimerBoot();

    LE_INFO("Request activation of the positioning service");
    LE_FATAL_IF(le_posCtrl_Request() == NULL, "Can't activate the positioning service")

    // Register a callback handler for network registration state.
    le_mrc_AddNetRegStateEventHandler((le_mrc_NetRegStateHandlerFunc_t)RegistrationStateHandler, NULL);
}
