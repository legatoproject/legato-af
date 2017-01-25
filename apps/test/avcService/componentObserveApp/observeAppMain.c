//--------------------------------------------------------------------------------------------------
/**
 * @file observeAppMain.c
 *
 * This component is used for testing the AirVantage observe feature.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"

#include "le_print.h"


static le_avdata_RequestSessionObjRef_t SessionReqRef;

//--------------------------------------------------------------------------------------------------
/**
 * Generate a random number between limits.
 */
//--------------------------------------------------------------------------------------------------
static float RandBetween(int min, int max)
{
    int diff = max-min;
    return (((double)(diff+1)/RAND_MAX) * rand() + min);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function that will compute the health of the battery and send a read response when
 * it is done. This is an example of read call back functionality.
 */
//--------------------------------------------------------------------------------------------------
static void ComputeBatteryHealthHandler
(
    le_avdata_AssetInstanceRef_t instRef,
    const char* fieldName,
    void* contextPtr
)
{
    LE_INFO("Registered handler called for %s", fieldName);

    // Write the result to the assetData. The assetData handler will send this response back to
    // the server
    le_avdata_SetFloat(instRef, "ComputeBatteryHealth", RandBetween(20, 100));
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks state of the car periodically and notifies events.
 */
//--------------------------------------------------------------------------------------------------
static void NotifyTimer
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    le_avdata_AssetInstanceRef_t instZeroRef;

    instZeroRef = (le_avdata_AssetInstanceRef_t) le_timer_GetContextPtr(timerRef);

    le_avdata_SetInt(instZeroRef, "Speed", (int) RandBetween(0, 100));
    le_avdata_SetFloat(instZeroRef, "InteriorTemperature", RandBetween(20, 30));
    le_avdata_SetBool(instZeroRef, "LowFuelWarning", (bool) (rand() % 2));
}


//--------------------------------------------------------------------------------------------------
/**
 * Receives notification from avdata about session state.
 */
//--------------------------------------------------------------------------------------------------
static void SessionHandler
(
    le_avdata_SessionState_t sessionState,
    void* contextPtr
)
{
    if (sessionState == LE_AVDATA_SESSION_STARTED)
    {
        LE_INFO("Airvantage session started.");
    }
    else
    {
        LE_INFO("Airvantage session stopped.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Init the component
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_avdata_AssetInstanceRef_t instZeroRef;
    instZeroRef = le_avdata_Create("myCar");

    // Register handlers that will actually set the value of the 'variable' field on read
    le_avdata_AddFieldEventHandler(instZeroRef,
                                   "ComputeBatteryHealth",
                                   ComputeBatteryHealthHandler,
                                   NULL);

    le_avdata_AddSessionStateHandler(SessionHandler, NULL);

    SessionReqRef = le_avdata_RequestSession();
    LE_FATAL_IF(SessionReqRef == NULL, "Session request failed.");

    // Initialize a timer that checks the state of the car every minute.
    le_clk_Time_t timerInterval = { .sec = 60, .usec = 0 };
    le_timer_Ref_t notifyTimerRef;
    notifyTimerRef = le_timer_Create("NotifyTimer");
    le_timer_SetInterval(notifyTimerRef, timerInterval);
    le_timer_SetContextPtr(notifyTimerRef, (void*)instZeroRef);
    le_timer_SetRepeat(notifyTimerRef, 0);
    le_timer_SetHandler(notifyTimerRef, NotifyTimer);
    le_timer_Start(notifyTimerRef);

}
