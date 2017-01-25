//--------------------------------------------------------------------------------------------------
/** @file textLoc.c
 *
 * This app illustrates a sample usage of ultra low power mode API. This app reads the current gps
 * location and then sends it as a text message to a destination cell phone number.  Once the text
 * message has been sent, the device enters ultra low power mode.  The device will wake up from
 * ultra low power mode after a configurable delay.
 *
 * @note This app expects destination cell number to be specified in environment variable section
 * of adef file. If nothing specified in environment variable, it will send message to a default
 * non-existent phone number.
 *
 * Copyright (C) Sierra Wireless Inc.
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
 * @note This is a non-existent phone number (ref:
 * https://en.wikipedia.org/wiki/Fictitious_telephone_number).
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
#define ULPM_EXIT_INTERVAL 30

//--------------------------------------------------------------------------------------------------
/**
 * Gpio used to exit from shutdown/ultralow-power state.
 *
 * @note Please change gpio number as needed.
 */
//--------------------------------------------------------------------------------------------------
#define WAKEUP_GPIO_NUM    38

//--------------------------------------------------------------------------------------------------
/**
 * Pointer to the null terminated string containing the destination phone number.
 */
//--------------------------------------------------------------------------------------------------
static const char *DestPhoneNumberPtr;

//--------------------------------------------------------------------------------------------------
/**
 * Attempts to use the GPS to find the current latitude, longitude and horizontal accuracy within
 * the given timeout constraints.
 *
 * @return
 *      - LE_OK on success
 *      - LE_UNAVAILABLE if positioning services are unavailable
 *      - LE_TIMEOUT if the timeout expires before successfully acquiring the location
 *
 * @note
 *      Blocks until the location has been identified or the timeout has occurred.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetCurrentLocation(
    int32_t *latitudePtr,           ///< [OUT] latitude of device - set to NULL if not needed
    int32_t *longitudePtr,          ///< [OUT] longitude of device - set to NULL if not needed
    int32_t *horizontalAccuracyPtr, ///< [OUT] horizontal accuracy of device - set to NULL if not
                                    ///< needed
    uint32_t timeoutInSeconds       ///< [IN] duration to attempt to acquire location data before
                                    ///< giving up.  A value of 0 means there is no timeout.
)
{
    le_posCtrl_ActivationRef_t posCtrlRef = le_posCtrl_Request();
    if (!posCtrlRef)
    {
        LE_ERROR("Can't activate the Positioning service");
        return LE_UNAVAILABLE;
    }

    le_result_t result;
    const time_t startTime = time(NULL);
    LE_INFO("Checking GPS position");
    while (true)
    {
        result = le_pos_Get2DLocation(latitudePtr, longitudePtr, horizontalAccuracyPtr);
        if (result == LE_OK)
        {
            break;
        }
        else if (
            (timeoutInSeconds != 0) &&
            (difftime(time(NULL), startTime) > (double)timeoutInSeconds))
        {
            result = LE_TIMEOUT;
            break;
        }
        else
        {
            // Sleep for one second before requesting the location again.
            sleep(1);
        }
    }

    le_posCtrl_Release(posCtrlRef);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends an SMS text message to the given destination with the given message content.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SendTextMessage(
    const char *destinationNumberPtr, ///< [IN] Phone number to send the text message to as ASCII
                                      ///< values
    const char *messageBodyPtr        ///< [IN] Text message body content
)
{
    le_result_t result = LE_OK;
    LE_INFO("Sending SMS");

    le_sms_MsgRef_t sms = le_sms_Create();

    if (le_sms_SetDestination(sms, destinationNumberPtr) != LE_OK)
    {
        result = LE_FAULT;
        LE_ERROR("Could not set destination phone number");
        goto sms_done;
    }

    if (le_sms_SetText(sms, messageBodyPtr) != LE_OK)
    {
        result = LE_FAULT;
        LE_ERROR("Could not set text message body");
        goto sms_done;
    }

    if (le_sms_Send(sms) != LE_OK)
    {
        result = LE_FAULT;
        LE_ERROR("Could not send SMS message");
        goto sms_done;
    }

    LE_INFO("SMS Message sent");

sms_done:
    le_sms_Delete(sms);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send the device location as a text message
 *
 * Attempts to send an SMS text message containing the current device location to the destination
 * phone number.
 *
 * @note
 *      No failure notification is provided if location services or SMS send are unsuccessful.
 */
//--------------------------------------------------------------------------------------------------
static void SendSmsCurrentLocation(
    void
)
{
    char smsBody[LE_SMS_TEXT_MAX_LEN];
    int32_t latitude;
    int32_t longitude;
    int32_t horizontalAccuracy;

    const le_result_t result =
        GetCurrentLocation(&latitude, &longitude, &horizontalAccuracy, GPSTIMEOUT * 60);
    if (result == LE_OK)
    {
        snprintf(smsBody, sizeof(smsBody), "Loc:%d,%d", latitude, longitude);
    }
    else
    {
        strncpy(smsBody, "Loc:unknown", sizeof(smsBody));
    }
    SendTextMessage(DestPhoneNumberPtr, smsBody);
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the boot source and shutdown MDM.
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

    // Boot on gpio. Please note this is platform dependent, change it when needed.
    if (le_ulpm_BootOnGpio(WAKEUP_GPIO_NUM, LE_ULPM_GPIO_LOW) != LE_OK)
    {
        LE_ERROR("Can't set gpio: %d as boot source", WAKEUP_GPIO_NUM);
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
            SendSmsCurrentLocation();
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
 * Simulate entry into the current NetReg state by calling RegistrationStateHandler
 *
 * RegistrationStateHandler will only be notified of state change events.  This function exists to
 * simulate the change into the current state.
 */
//--------------------------------------------------------------------------------------------------
static void SimulateNetRegStateChangeToCurrentState(
    void *ignoredParam1, ///< Only exists to allow this function to conform to the
                         ///< le_event_DeferredFunc_t declaration
    void *ignoredParam2  ///< Only exists to allow this function to conform to the
                         ///< le_event_DeferredFunc_t declaration
)
{
    le_mrc_NetRegState_t currentNetRegState;
    LE_FATAL_IF(le_mrc_GetNetRegState(&currentNetRegState) != LE_OK, "Couldn't get NetRegState");
    RegistrationStateHandler(currentNetRegState, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the destination phone number.
 **/
//--------------------------------------------------------------------------------------------------
static void GetDestinationCellNo
(
    void
)
{
    DestPhoneNumberPtr = getenv("DEST_CELL_NO");
    if (!DestPhoneNumberPtr)
    {
        LE_WARN(
            "No destination cell number is specified. Using a default non-existent number");
        DestPhoneNumberPtr = DEFAULT_PHONE_NO;
    }

    LE_INFO("Destination phone number = %s", DestPhoneNumberPtr);
}


COMPONENT_INIT
{
    char version[LE_ULPM_MAX_VERS_LEN + 1];

    LE_INFO("TextLoc started");

    // Get ultra low power manager firmware version
    LE_FATAL_IF(
        le_ulpm_GetFirmwareVersion(version, sizeof(version)) != LE_OK,
        "Failed to get ultra low power firmware version");
    LE_INFO("Ultra Low Power Manager Firmware version: %s", version);

    // Now check whether boot was due to timer expiry.
    if (le_bootReason_WasTimer())
    {
        LE_INFO("Booted from timer, not sending another text message.");
    }
    else if (le_bootReason_WasGpio(WAKEUP_GPIO_NUM))
    {
        LE_INFO("Booted from GPIO, not sending another text message.");
    }
    else
    {
        // Get the destination phone number
        GetDestinationCellNo();

        // Register a callback handler for network registration state.
        le_mrc_AddNetRegStateEventHandler(
            (le_mrc_NetRegStateHandlerFunc_t)RegistrationStateHandler, NULL);
        le_event_QueueFunction(&SimulateNetRegStateChangeToCurrentState, NULL, NULL);
    }
}

