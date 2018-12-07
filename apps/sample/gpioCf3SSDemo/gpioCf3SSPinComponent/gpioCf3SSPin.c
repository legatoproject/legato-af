/**
 * @file gpioCf3SSPin.c
 *
 * This sample app wakes up the device from selective suspend state using GPIO 42.
 * The app will use gpio 42 as falling edge based interrupt. Removing the USB cable
 * from the host will put the modem into selective suspend state. Change the gpio edge
 * to induce falling edge. The device will acquire system wake-lock for 60 seconds,
 * release it and sleep again.
 *
 * Copyright (C) Sierra Wireless, Inc.
 */

#include "legato.h"
#include "interfaces.h"

//-------------------------------------------------------------------------------------------------
/**
 *  Define the wakeup source, timer reference and wakelock string
 */
// -------------------------------------------------------------------------------------------------
static le_pm_WakeupSourceRef_t WakeupSourceRef;
static const char *WakeLockStr="GPIO42_Wakeup";
static le_timer_Ref_t WakeupTimerRef;

//-------------------------------------------------------------------------------------------------
/**
 *  Register Pin42 state change to trigger wake up lock and release
 */
// -------------------------------------------------------------------------------------------------
static void Pin42ChangeCallback
(
    bool    state,        ///< [IN] State of pin
    void    *ctx          ///< [IN] Context pointer
)
{
    LE_INFO("State change %s", state?"TRUE":"FALSE");

    // Check for previous wakelock
    if (!le_timer_IsRunning(WakeupTimerRef))
    {
        LE_FATAL_IF(le_pm_StayAwake(WakeupSourceRef) != LE_OK,"Unable to acquire %s wakeup source\n",
                   WakeLockStr);
        LE_FATAL_IF(le_timer_Start(WakeupTimerRef) != LE_OK, "Could not start timer");
    }
    else
    {
        LE_INFO("Wake lock already held");
    }
}

//-------------------------------------------------------------------------------------------------
/**
 *  Timer Expiry handle to release the wakeup source
 */
// -------------------------------------------------------------------------------------------------
void WakeLockTimerHandler
(
    le_timer_Ref_t    wakeTimerRef        ///< [IN] wakeup timer-reference
)
{
    LE_INFO("Timer expired");

    LE_FATAL_IF(le_pm_Relax(WakeupSourceRef) != LE_OK, "Unable to release %s wakeup source\n",
               WakeLockStr);
    LE_INFO("Wake source %s released successfully", WakeLockStr);

}

//-------------------------------------------------------------------------------------------------
/**
 *  Read value and direction of GPIO pin42 and register callback for wakeup
 */
// -------------------------------------------------------------------------------------------------
static void Pin42RegisterCallback
(
    void
)
{
    bool value;

    // Set GPIO 42 active on low
    le_gpioPin42_SetInput(LE_GPIOPIN42_ACTIVE_LOW);

    // Read the GPIO 42 value
    value = le_gpioPin42_Read();
    LE_INFO("Pin42 read active: %d", value);

    // Set the call back event based on falling edge of GPIO 42
    le_gpioPin42_AddChangeEventHandler(LE_GPIOPIN42_EDGE_FALLING, Pin42ChangeCallback, NULL, 0);
}

COMPONENT_INIT
{
    LE_INFO("This sample app simulates GPIO based wakeup for device in selective suspend\n");
    le_clk_Time_t wakeTimerRefInt;
    const char *wakeupTimer = "Wakeup_Delay_Timer";
    wakeTimerRefInt = (le_clk_Time_t){ .sec = 60, .usec = 0};
    WakeupTimerRef = le_timer_Create(wakeupTimer);
    WakeupSourceRef = le_pm_NewWakeupSource(0, WakeLockStr);

    LE_FATAL_IF(le_timer_SetInterval(WakeupTimerRef, wakeTimerRefInt) != LE_OK,
               "Could not set timer interval");
    LE_FATAL_IF(le_timer_SetHandler(WakeupTimerRef, WakeLockTimerHandler) != LE_OK,
               "Could set timer handler function");
    Pin42RegisterCallback();

    return;
}
